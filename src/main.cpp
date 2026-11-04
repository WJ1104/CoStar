#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

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

    return 0;
}