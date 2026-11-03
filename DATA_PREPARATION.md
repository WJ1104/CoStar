# IMDb Data Preparation

This project uses IMDb's public non-commercial TSV datasets to make three smaller files for the C++ program. The script keeps feature films and people listed as actors or actresses in IMDb's principal credits.

## Prepare the files

Download these three compressed files from [IMDb's dataset download page](https://datasets.imdbws.com/):

- `title.basics.tsv.gz`
- `title.principals.tsv.gz`
- `name.basics.tsv.gz`

Put them in `data/raw/` without unzipping them. From the repository folder, run:

```powershell
py scripts/prepare_data.py
```

If `py` does not find Python on a teammate's computer, use:

```powershell
python scripts/prepare_data.py
```

The script uses only Python's built-in libraries. It checks that all three input files exist before starting. It reads the IMDb files, then creates the output folder if needed.

## Output files

The script creates these files in `data/processed/`:

| File | Columns | Use |
|---|---|---|
| `movies.tsv` | `tconst`, `title` | IMDb movie ID and display title |
| `actors.tsv` | `nconst`, `name` | IMDb person ID and display name |
| `credits.tsv` | `tconst`, `nconst` | One movie-to-actor connection per row |

Use the IMDb IDs to join the files. Names and movie titles are for display; they are not unique identifiers.

With the local dataset snapshot processed on November 2, 2026, the script produced 759,672 movies, 1,368,582 actor names, and 4,161,792 credits. IMDb refreshes the source files daily, so a later run may produce different totals.

## Important data limitation

`title.principals.tsv.gz` contains principal credits. It does not promise a complete cast list. This project therefore builds a network from the actor and actress principal credits available in that dataset. Describe the results as connections in this dataset's network, rather than claiming it contains every actor in every movie. See the [IMDb dataset schema](https://data.imdb.com/non-commercial-datasets/).

IMDb describes the datasets as for personal, non-commercial use and sets limits on redistribution, republishing, and repurposing. Because this project transforms the data into a graph, ask the instructor how those terms apply before using or sharing the raw or processed files. Keep the data files out of GitHub unless the instructor confirms that sharing them is allowed. Include IMDb's required attribution in project materials:

> Information courtesy of IMDb (https://www.imdb.com). Used with permission.

The repository ignores `data/`, so the raw and generated data files do not appear in GitHub. The script and this guide can be shared with the group.

## Next work for the group

1. **Finish the C++ graph loader.** Read all three processed files. Use `nconst` and `tconst` as IDs, then build actor-to-movie and movie-to-actor adjacency lists. Print the number of loaded actors, movies, and connections.
2. **Implement regular BFS.** Search from one actor, remember each actor's parent and connecting movie, and rebuild the shortest chain when the target is found.
3. **Implement bidirectional BFS.** Search from both actors, detect when the searches meet, and rebuild the chain. Compare its shortest chain length with regular BFS.
4. **Add the menu and benchmarks.** Let a user search for two actor names, compare both searches, then time both methods over the same random actor pairs and report work counts.

Suggested three-person split: one person finishes the graph loader and lookup by actor name; one implements regular BFS and path display; one implements bidirectional BFS and benchmarking. Integrate the work together and make sure both searches use the same graph and return the same shortest chain length.
