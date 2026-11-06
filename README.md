# CoStar

Group 28 project: how many movies apart are two actors? CoStar searches the IMDb actor–movie graph and compares two algorithms:

- **BFS** (breadth-first search) from one actor.
- **Bidirectional BFS**, which searches from both actors at once and stops when the searches meet.

The chain is printed as `Actor -> Movie -> Actor -> ...`, and "N movies apart" is the number of movies in that chain.

## Requirements

- A C++17 compiler (`g++`)
- Python 3 (only for preparing the data, and for the optional GUI server)
- The IMDb datasets: `title.basics.tsv.gz`, `title.principals.tsv.gz`, `name.basics.tsv.gz` from https://datasets.imdbws.com/

## 1. Prepare the data

Put the three IMDb files in `data/raw/`, then from the project folder run:

```
python scripts/prepare_data.py
```

This creates `data/processed/movies.tsv`, `actors.tsv` and `credits.tsv`. The `data/` folder is ignored by git, so every teammate prepares it once on their own computer.

## 2. Run the menu program

From the project folder (it reads `data/processed/` using paths relative to where you run it):

```
g++ -std=c++17 -O2 -o costar_menu src/menu.cpp
./costar_menu
```

On Windows PowerShell use `.\costar_menu.exe`. Loading takes about 10–15 seconds.

| Option | What it does |
|---|---|
| 1. Find the connection between 2 actors | Type two names (capitals don't matter). Both searches run; the chain is printed with actors checked, movies checked and time for each. If several actors share a name, pick one by number. Press Enter on an empty line to go back. |
| 2. Run benchmark | Runs BFS and bidirectional BFS on random pairs (fixed seed, so results repeat), skips unconnected pairs, prints a table by chain length, and saves every search to `results/benchmark.csv`. |
| 3. Exit | |

`results/benchmark.csv` columns: `algorithm` (`bfs` or `bidirectional`), `length`, `time_ms`, `actors_checked`, `movies_checked`, `bytes` (estimated memory).

## 3. Run the original single-pair program

```
g++ -std=c++17 -O2 -o costar src/main.cpp
./costar
```

This runs one hardcoded pair through both searches and warns if they disagree on the chain length.

## 4. Graph GUI (optional, in a browser)

The `gui/` folder is a visual version that runs both searches on the full IMDb data in the browser, with no file upload. It draws the chain and a sample of the nodes each search checked.

```
cd gui
python -m http.server 8000
```

Open http://localhost:8000. With VS Code Live Server, right-click `gui/index.html` and choose "Open with Live Server". Opening `index.html` by double-clicking won't work because the browser blocks loading the data files.

`gui/data/` is about 36 MB, so keep `gui/` out of git.

## Project layout

```
src/main.cpp         BFS and bidirectional BFS, single hardcoded pair
src/menu.cpp         menu, name lookup, benchmark (BFS vs bidirectional)
scripts/prepare_data.py   cleans the IMDb files into data/processed/
gui/                 browser version with graph drawing
data/                raw and processed data (not in git)
results/             benchmark.csv written by the menu program
```

## Notes

- The graph has two kinds of nodes, actors and movies, so a path of N+1 actors has N movies between them.
- Bidirectional BFS finishes the whole level it is on before stopping, which keeps the chain as short as possible.
- On very short chains BFS can be faster because bidirectional BFS has more setup. On longer chains bidirectional BFS checks far fewer actors and movies. It uses more memory because it keeps arrays for both sides.
- About one in three random pairs has no connection, and the benchmark skips them.
