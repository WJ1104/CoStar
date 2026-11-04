#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <chrono>

// ---------- Bidirectional BFS (added) ----------
struct BidirResult {
    bool found = false;
    std::vector<int> actors;   // chain of actors, start -> target
    std::vector<int> movies;   // movies[i] connects actors[i] and actors[i+1]
    long long actorsChecked = 0;
    long long moviesChecked = 0;
    double ms = 0.0;
};

// Expands one full level at a time from the smaller frontier. Finishing the
// whole level before stopping guarantees the shortest chain.
BidirResult bidirectionalBfs(const std::vector<std::vector<int>>& actorMovies,
                             const std::vector<std::vector<int>>& movieActors,
                             int start, int target) {
    BidirResult res;
    auto t0 = std::chrono::steady_clock::now();
    int n = static_cast<int>(actorMovies.size());

    if (start == target) {
        res.found = true;
        res.actors.push_back(start);
        res.actorsChecked = 1;
        return res;
    }

    // Index 0 = forward side (from start), 1 = backward side (from target).
    std::vector<int> dist[2], parentActor[2], parentMovie[2];
    std::vector<bool> movieSeen[2];
    std::vector<int> frontier[2];
    for (int s = 0; s < 2; s++) {
        dist[s].assign(n, -1);
        parentActor[s].assign(n, -1);
        parentMovie[s].assign(n, -1);
        movieSeen[s].assign(movieActors.size(), false);
    }
    dist[0][start] = 0;
    frontier[0].push_back(start);
    dist[1][target] = 0;
    frontier[1].push_back(target);

    int best = -1, meet = -1;

    while (!frontier[0].empty() && !frontier[1].empty() && best == -1) {
        int side = (frontier[0].size() <= frontier[1].size()) ? 0 : 1;
        int other = 1 - side;
        std::vector<int> next;

        for (int cur : frontier[side]) {
            res.actorsChecked++;
            for (int m : actorMovies[cur]) {
                if (movieSeen[side][m]) continue;
                movieSeen[side][m] = true;
                res.moviesChecked++;
                for (int nb : movieActors[m]) {
                    if (dist[side][nb] != -1) continue;
                    dist[side][nb] = dist[side][cur] + 1;
                    parentActor[side][nb] = cur;
                    parentMovie[side][nb] = m;
                    next.push_back(nb);
                    if (dist[other][nb] != -1) {
                        int total = dist[side][nb] + dist[other][nb];
                        if (best == -1 || total < best) {
                            best = total;
                            meet = nb;
                        }
                    }
                }
            }
        }
        frontier[side] = std::move(next);
    }

    if (best != -1) {
        res.found = true;
        for (int c = meet; c != start; c = parentActor[0][c]) {
            res.actors.push_back(c);
            res.movies.push_back(parentMovie[0][c]);
        }
        res.actors.push_back(start);
        std::reverse(res.actors.begin(), res.actors.end());
        std::reverse(res.movies.begin(), res.movies.end());
        for (int c = meet; c != target; c = parentActor[1][c]) {
            res.movies.push_back(parentMovie[1][c]);
            res.actors.push_back(parentActor[1][c]);
        }
    }
    res.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return res;
}

int main() {
    std::ifstream file("data/processed/movies.tsv");

    if (!file) {
        std::cout << "Could not open movies.tsv\n";
        return 1;
    }

    std::string line;
    std::getline(file, line); // Skip the column labels

    std::unordered_map<std::string, int> movieIndex;
    std::vector<std::string> movieIds;
    std::vector<std::string> movieTitles;

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::size_t tab = line.find('\t');
        if (tab == std::string::npos) {
            continue;
        }

        std::string id = line.substr(0, tab);
        std::string title = line.substr(tab + 1);

        movieIndex[id] = static_cast<int>(movieIds.size());
        movieIds.push_back(id);
        movieTitles.push_back(title);
    }

    std::ifstream actorFile("data/processed/actors.tsv");

    if (!actorFile) {
        std::cout << "Could not open actors.tsv\n";
        return 1;
    }

    std::string actorLine;
    std::getline(actorFile, actorLine); // Skip the column labels

    std::unordered_map<std::string, int> actorIndex;
    std::vector<std::string> actorNames;
    std::vector<std::vector<int>> actorMovies;

    while (std::getline(actorFile, actorLine)) {
        if (!actorLine.empty() && actorLine.back() == '\r') {
            actorLine.pop_back();
        }

        std::size_t tab = actorLine.find('\t');
        if (tab == std::string::npos) {
            continue;
        }

        std::string id = actorLine.substr(0, tab);
        std::string name = actorLine.substr(tab + 1);

        actorIndex[id] = static_cast<int>(actorNames.size());
        actorNames.push_back(name);
        actorMovies.emplace_back();
    }

    // Read credits.tsv 
    std::ifstream creditFile("data/processed/credits.tsv");

    if (!creditFile) {
        std::cout << "Could not open credits.tsv\n";
        return 1;
    }

    std::string creditLine;
    std::getline(creditFile, creditLine); // Skip the column labels

    std::vector<std::vector<int>> movieActors(movieIds.size());
    int connections = 0;

    while (std::getline(creditFile, creditLine)) {
        if (!creditLine.empty() && creditLine.back() == '\r') {
            creditLine.pop_back();
        }

        std::size_t tab = creditLine.find('\t');
        if (tab == std::string::npos) {
            continue;
        }

        std::string tconst = creditLine.substr(0, tab);
        std::string nconst = creditLine.substr(tab + 1);

        std::unordered_map<std::string, int>::iterator mit = movieIndex.find(tconst);
        std::unordered_map<std::string, int>::iterator ait = actorIndex.find(nconst);

        if (mit != movieIndex.end() && ait != actorIndex.end()) {
            int movieIdx = mit->second;
            int actorIdx = ait->second;

            actorMovies[actorIdx].push_back(movieIdx);
            movieActors[movieIdx].push_back(actorIdx);
            connections++;
        }
    }

    std::cout << "Movies loaded: " << movieIds.size() << "\n";
    std::cout << "Actors loaded: " << actorNames.size() << "\n";
    std::cout << "Connections loaded: " << connections << "\n";




    //BFS Implementation

    // Hardcoded start and target actors for demonstration purposes
    int startActorIdx = 0;
    int targetActorIdx = 0;
    if (actorNames.size() > 100) {
        targetActorIdx = 100;
    } else if (!actorNames.empty()) {
        targetActorIdx = static_cast<int>(actorNames.size()) - 1;
    }

    std::cout << "Searching path between: [" << actorNames[startActorIdx] 
              << "] and [" << actorNames[targetActorIdx] << "]\n\n";


    std::vector<bool> visited(actorNames.size(), false);
    std::vector<int> parentActor(actorNames.size(), -1);
    std::vector<int> parentMovie(actorNames.size(), -1);

    std::queue<int> q;
    visited[startActorIdx] = true;
    q.push(startActorIdx);

    bool found = false;

    while (!q.empty()) {
        int currentActor = q.front();
        q.pop();

        if (currentActor == targetActorIdx) {
            found = true;
            break;
        }

        // Loop through the actor's movies using standard indexing
        for (int i = 0; i < static_cast<int>(actorMovies[currentActor].size()); i++) {
            int mIdx = actorMovies[currentActor][i];

            // Loop through the movie's actors using standard indexing
            for (int j = 0; j < static_cast<int>(movieActors[mIdx].size()); j++) {
                int neighborActor = movieActors[mIdx][j];

                if (!visited[neighborActor]) {
                    visited[neighborActor] = true;
                    parentActor[neighborActor] = currentActor;
                    parentMovie[neighborActor] = mIdx;
                    q.push(neighborActor);
                }
            }
        }
    }

    // Rebuild and print the shortest chain if found
    if (found) {
        std::cout << "\nTarget found! Shortest chain:\n";
        int curr = targetActorIdx;
        std::vector<int> chainActors;
        std::vector<int> chainMovies;

        while (curr != startActorIdx) {
            chainActors.push_back(curr);
            chainMovies.push_back(parentMovie[curr]);
            curr = parentActor[curr];
        }
        chainActors.push_back(startActorIdx);

        for (int i = static_cast<int>(chainActors.size()) - 1; i >= 0; i--) {
            std::cout << actorNames[chainActors[i]];
            if (i > 0) {
                std::cout << " appeared in [" << movieTitles[chainMovies[i - 1]] << "] -> ";
            }
        }
        std::cout << "\n";
    } else {
        std::cout << "\nNo connection found between the selected actors.\n";
    }

    // ---------- Bidirectional BFS run (added) ----------
    BidirResult bidi = bidirectionalBfs(actorMovies, movieActors, startActorIdx, targetActorIdx);

    std::cout << "\nBidirectional BFS chain:\n";
    if (bidi.found) {
        for (std::size_t i = 0; i < bidi.actors.size(); i++) {
            std::cout << actorNames[bidi.actors[i]];
            if (i + 1 < bidi.actors.size()) {
                std::cout << " appeared in [" << movieTitles[bidi.movies[i]] << "] -> ";
            }
        }
        std::cout << "\n";
    } else {
        std::cout << "No connection found between the selected actors.\n";
    }
    std::cout << "Bidirectional: " << bidi.ms << " ms, "
              << bidi.actorsChecked << " actors expanded, "
              << bidi.moviesChecked << " movies expanded\n";
    if (bidi.found != found) {
        std::cout << "WARNING: BFS and bidirectional BFS disagree!\n";
    }

    return 0;
}