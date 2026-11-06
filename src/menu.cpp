
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

#include <queue>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>


struct Graph {
    std::vector<std::string> actorNames;
    std::vector<std::string> movieTitles;
    std::vector<std::vector<int>> actorMovies;  // actor -> movies
    std::vector<std::vector<int>> movieActors;  // movie -> actors
    std::unordered_map<std::string, std::vector<int>> byName;  // lowercase name -> actor ids
    long connections = 0;
};

static std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static std::string trim(const std::string& s) {
    std::size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) a++;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) b--;
    return s.substr(a, b - a);
}

static std::string movieCount(std::size_t n) {
    return std::to_string(n) + (n == 1 ? " movie" : " movies");
}


template <typename F>
static bool forEachRow(const std::string& path, F onRow) {
    std::ifstream in(path);
    if (!in) {
        std::cout << "Could not open " << path << "\n";
        return false;
    }
    std::string line;
    std::getline(in, line);  // skip the column labels
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::size_t tab = line.find('\t');
        if (tab == std::string::npos) continue;
        onRow(line.substr(0, tab), line.substr(tab + 1));
    }
    return true;
}

static bool loadGraph(Graph& g) {
    std::unordered_map<std::string, int> movieIndex, actorIndex;
    movieIndex.reserve(800000);
    actorIndex.reserve(1500000);

    bool ok = forEachRow("data/processed/movies.tsv", [&](std::string id, std::string title) {
        movieIndex[id] = static_cast<int>(g.movieTitles.size());
        g.movieTitles.push_back(std::move(title));
    });
    if (!ok) return false;

    ok = forEachRow("data/processed/actors.tsv", [&](std::string id, std::string name) {
        actorIndex[id] = static_cast<int>(g.actorNames.size());
        g.actorNames.push_back(std::move(name));
        g.actorMovies.emplace_back();
    });
    if (!ok) return false;

    g.movieActors.assign(g.movieTitles.size(), {});
    ok = forEachRow("data/processed/credits.tsv", [&](std::string tconst, std::string nconst) {
        auto mit = movieIndex.find(tconst);
        auto ait = actorIndex.find(nconst);
        if (mit == movieIndex.end() || ait == actorIndex.end()) return;
        g.actorMovies[ait->second].push_back(mit->second);
        g.movieActors[mit->second].push_back(ait->second);
        g.connections++;
    });
    if (!ok) return false;

    // Name lookup: lowercase name -> every actor with that name.
    g.byName.reserve(g.actorNames.size());
    for (std::size_t i = 0; i < g.actorNames.size(); i++)
        g.byName[lower(g.actorNames[i])].push_back(static_cast<int>(i));
    return true;
}



struct SearchResult {
    bool found = false;
    std::vector<int> actors;
    std::vector<int> movies;
    long actorsChecked = 0;
    long moviesChecked = 0;
    double ms = 0;
    std::size_t bytes = 0;
};

static SearchResult bfsSearch(const Graph& g, int src, int dst) {
    auto t0 = std::chrono::steady_clock::now();
    SearchResult r;
    std::size_t A = g.actorNames.size(), M = g.movieTitles.size();

    std::vector<int> parentActor(A, -2);  
    std::vector<int> parentMovie(A, -1);
    std::vector<char> movieSeen(M, 0);
    std::queue<int> q;
    parentActor[src] = -1;
    q.push(src);
    std::size_t peakQueue = 1;
    bool found = (src == dst);

    while (!q.empty() && !found) {
        int u = q.front();
        q.pop();
        r.actorsChecked++;
        for (int m : g.actorMovies[u]) {
            if (movieSeen[m]) continue;
            movieSeen[m] = 1;
            r.moviesChecked++;
            for (int v : g.movieActors[m]) {
                if (parentActor[v] != -2) continue;
                parentActor[v] = u;
                parentMovie[v] = m;
                q.push(v);
                if (v == dst) { found = true; break; }
            }
            if (found) break;
        }
        peakQueue = std::max(peakQueue, q.size());
    }

    if (found) {
        r.found = true;
        for (int cur = dst; cur != -1; cur = parentActor[cur]) {
            r.actors.push_back(cur);
            if (parentActor[cur] != -1) r.movies.push_back(parentMovie[cur]);
        }
        std::reverse(r.actors.begin(), r.actors.end());
        std::reverse(r.movies.begin(), r.movies.end());
    }
    r.bytes = A * 2 * sizeof(int) + M + peakQueue * sizeof(int);
    r.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return r;
}

static void printChain(const Graph& g, const SearchResult& r) {
    if (!r.found) { std::cout << "No connection found between these actors.\n"; return; }
    for (std::size_t i = 0; i < r.actors.size(); i++) {
        std::cout << g.actorNames[r.actors[i]];
        if (i < r.movies.size()) std::cout << " -> " << g.movieTitles[r.movies[i]] << " -> ";
    }
    std::size_t n = r.movies.size();
    std::cout << "\n(" << n << " movie" << (n == 1 ? "" : "s") << " apart, found with BFS)\n";
}



static std::vector<int> sortedByMovieCount(const Graph& g, std::vector<int> ids) {
    std::sort(ids.begin(), ids.end(), [&](int a, int b) {
        return g.actorMovies[a].size() > g.actorMovies[b].size();
    });
    return ids;
}


static void printSuggestions(const Graph& g, const std::string& text) {
    std::vector<int> hits;
    for (std::size_t i = 0; i < g.actorNames.size(); i++)
        if (lower(g.actorNames[i]).find(text) != std::string::npos) hits.push_back(static_cast<int>(i));
    if (hits.empty()) { std::cout << "No actor with that name. Check the spelling.\n"; return; }
    hits = sortedByMovieCount(g, hits);
    std::cout << "No exact match. Did you mean:\n";
    for (std::size_t i = 0; i < hits.size() && i < 5; i++)
        std::cout << "  " << g.actorNames[hits[i]] << " (" << movieCount(g.actorMovies[hits[i]].size()) << ")\n";
}


static int chooseActor(const Graph& g, const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        std::string input;
        if (!std::getline(std::cin, input)) return -1;
        input = lower(trim(input));
        if (input.empty()) return -1;

        auto it = g.byName.find(input);
        if (it == g.byName.end()) { printSuggestions(g, input); continue; }
        if (it->second.size() == 1) return it->second[0];

        std::vector<int> ids = sortedByMovieCount(g, it->second);
        std::size_t shown = std::min<std::size_t>(ids.size(), 10);
        std::cout << ids.size() << " actors have that name:\n";
        for (std::size_t i = 0; i < shown; i++) {
            const std::vector<int>& movies = g.actorMovies[ids[i]];
            std::cout << "  " << i + 1 << ". " << g.actorNames[ids[i]] << " (" << movieCount(movies.size());
            if (!movies.empty()) std::cout << ", e.g. " << g.movieTitles[movies[0]];
            std::cout << ")\n";
        }
        std::cout << "Pick a number: ";
        std::string pick;
        if (!std::getline(std::cin, pick)) return -1;
        try {
            int k = std::stoi(pick);
            if (k >= 1 && static_cast<std::size_t>(k) <= shown) return ids[k - 1];
        } catch (...) {}
        std::cout << "Not a valid choice.\n";
    }
}



struct Totals {
    long count = 0;
    double ms = 0, actors = 0, movies = 0, bytes = 0;
    void add(const SearchResult& r) {
        count++; ms += r.ms; actors += r.actorsChecked; movies += r.moviesChecked; bytes += r.bytes;
    }
};

static void runBenchmark(const Graph& g, int pairs) {
    std::filesystem::create_directories("results");
    std::ofstream csv("results/benchmark.csv");
    if (!csv) { std::cout << "Could not write results/benchmark.csv\n"; return; }
    csv << "length,time_ms,actors_checked,movies_checked,bytes\n";

    std::vector<int> eligible;  
    for (std::size_t i = 0; i < g.actorMovies.size(); i++)
        if (!g.actorMovies[i].empty()) eligible.push_back(static_cast<int>(i));
    if (eligible.size() < 2) { std::cout << "Not enough actors to benchmark.\n"; return; }

    std::mt19937 rng(42);  
    std::uniform_int_distribution<std::size_t> pick(0, eligible.size() - 1);

    std::map<int, Totals> byLength;
    Totals all;
    int skipped = 0;
    for (long tries = 0; all.count < pairs && tries < static_cast<long>(pairs) * 20; tries++) {
        int a = eligible[pick(rng)], b = eligible[pick(rng)];
        if (a == b) continue;
        SearchResult r = bfsSearch(g, a, b);
        if (!r.found) { skipped++; continue; }  
        int len = static_cast<int>(r.movies.size());
        csv << len << ',' << r.ms << ',' << r.actorsChecked << ',' << r.moviesChecked << ',' << r.bytes << '\n';
        byLength[len].add(r);
        all.add(r);
        if (all.count % 50 == 0) std::cout << "  " << all.count << " / " << pairs << " pairs done\n";
    }

    std::printf("\nBFS on %ld random pairs (%d unconnected pairs skipped)\n", all.count, skipped);
    if (all.count == 0) return;
    std::printf("%-8s %6s %12s %16s %16s %14s\n", "Movies", "Pairs", "Avg ms", "Avg actors", "Avg movies", "Avg MB");
    auto row = [](const char* label, const Totals& t) {
        std::printf("%-8s %6ld %12.2f %16.0f %16.0f %14.1f\n", label, t.count, t.ms / t.count,
                    t.actors / t.count, t.movies / t.count, t.bytes / t.count / 1048576.0);
    };
    for (const auto& kv : byLength) row(std::to_string(kv.first).c_str(), kv.second);
    row("all", all);
    std::cout << "Saved every search to results/benchmark.csv\n";
}



int main() {
    std::cout << "Loading data (this takes a few seconds)...\n";
    Graph g;
    if (!loadGraph(g)) return 1;

    while (true) {
        std::cout << "\n+-------------------------------------------+\n"
                  << "|                 CoStar                    |\n"
                  << "   " << g.actorNames.size() << " actors | " << g.movieTitles.size() << " movies loaded\n"
                  << "+-------------------------------------------+\n"
                  << "  1. Find the connection between 2 actors\n"
                  << "  2. Run benchmark on random pairs\n"
                  << "  3. Exit\n> ";
        std::string line;
        if (!std::getline(std::cin, line)) break;
        int choice = 0;
        try { choice = std::stoi(line); } catch (...) {}

        if (choice == 1) {
            std::cout << "(press Enter on an empty line to go back)\n";
            int a = chooseActor(g, "First actor:  ");
            if (a < 0) continue;
            int b = chooseActor(g, "Second actor: ");
            if (b < 0) continue;
            SearchResult r = bfsSearch(g, a, b);
            std::cout << "\n";
            printChain(g, r);
            std::printf("Checked %ld actors and %ld movies in %.2f ms.\n", r.actorsChecked, r.moviesChecked, r.ms);
        } else if (choice == 2) {
            std::cout << "How many random pairs? (default 1000) ";
            std::string n;
            std::getline(std::cin, n);
            int pairs = 1000;
            try { if (!trim(n).empty()) pairs = std::stoi(n); } catch (...) {}
            if (pairs < 1) pairs = 1000;
            runBenchmark(g, pairs);
        } else if (choice == 3) {
            break;
        } else {
            std::cout << "Please type 1, 2 or 3.\n";
        }
    }
    return 0;
}
