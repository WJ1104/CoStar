# CoStar

Group 28 project: how many movies apart are two actors? CoStar searches the IMDb actor–movie graph and compares two algorithms:

- **BFS** (breadth-first search) from one actor.
- **Bidirectional BFS**, which searches from both actors at once and stops when the searches meet.

The chain is printed as `Actor -> Movie -> Actor -> ...`, and "N movies apart" is the number of movies in that chain.

## Quick start (no data download needed)

The cleaned IMDb data is already in the `packed/` folder (about 45 MB), so you only need a C++17 compiler. From the project folder:

```
g++ -std=c++17 -O2 -o costar_menu src/menu.cpp
./costar_menu
```

On Windows PowerShell use `.\costar_menu.exe`. Loading takes a few seconds. The program finds `packed/` in the current folder or one or two folders above it, so it also works when an IDE starts it from a build folder.

| Option | What it does |
|---|---|
| 1. Find the connection between 2 actors | Type two names (capitals don't matter). Both searches run; the chain is printed with actors checked, movies checked and time for each. If several actors share a name, pick one by number. Press Enter on an empty line to go back. |
| 2. Run benchmark | Runs BFS and bidirectional BFS on random pairs (fixed seed, so results repeat), skips unconnected pairs, prints a table by chain length, and saves every search to `results/benchmark.csv`. |
| 3. Exit | |

`results/benchmark.csv` columns: `algorithm` (`bfs` or `bidirectional`), `length`, `time_ms`, `actors_checked`, `movies_checked`, `bytes` (estimated memory).

## Graph GUI (in a browser)

`gui/index.html` is a visual version that runs both searches on the full IMDb data in the browser. It draws the chain and a sample of the nodes each search checked. It reads the same `packed/` folder, so nothing needs to be uploaded.

From the **project folder** (not from `gui/`):

```
python -m http.server 8000
```

Then open http://localhost:8000/gui/. With VS Code Live Server, right-click `gui/index.html` and choose "Open with Live Server". Opening `index.html` by double-clicking won't work, because the browser blocks loading the data files that way.

## Original single-pair program

`src/main.cpp` runs one hardcoded pair through both searches and warns if they disagree on the chain length. It reads `data/processed/*.tsv`, so it needs the data from the next section.

```
g++ -std=c++17 -O2 -o costar src/main.cpp
./costar
```

Run it from the project folder. Do not build `src/main.cpp` and `src/menu.cpp` together: each has its own `main()`.

## Rebuilding the data (optional)

You only need this for `src/main.cpp` or to rebuild `packed/`.

1. Download `title.basics.tsv.gz`, `title.principals.tsv.gz` and `name.basics.tsv.gz` from https://datasets.imdbws.com/ into `data/raw/` (about 1.3 GB).
2. `python scripts/prepare_data.py` creates `data/processed/movies.tsv`, `actors.tsv` and `credits.tsv`.
3. `python scripts/pack_data.py` creates the small files in `packed/` from those.

`data/` is ignored by git because it is too large. `packed/` is in git.

## Project layout

```
src/menu.cpp         menu, name lookup, benchmark (BFS vs bidirectional)
src/main.cpp         BFS and bidirectional BFS, single hardcoded pair
packed/              cleaned data used by the menu program and the GUI
gui/                 browser version with graph drawing
scripts/             prepare_data.py (raw -> data/processed), pack_data.py (-> packed/)
results/             benchmark.csv written by the menu program
data/                raw and processed data (not in git)
```

## Notes

- The graph has two kinds of nodes, actors and movies, so a path of N+1 actors has N movies between them.
- Bidirectional BFS finishes the whole level it is on before stopping, which keeps the chain as short as possible.
- On very short chains BFS can be faster because bidirectional BFS has more setup. On longer chains bidirectional BFS checks far fewer actors and movies. It uses more memory because it keeps arrays for both sides.
- About one in three random pairs has no connection, and the benchmark skips them.
