#include <fstream>
#include <iostream>
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

    std::cout << "Movies loaded: " << movieIds.size() << "\n";
    std::cout << "Actors loaded: " << actorNames.size() << "\n";
    return 0;
}
