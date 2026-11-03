import csv
import gzip
import os
import sys


raw_folder = "data/raw"
output_folder = "data/processed"

required_files = [
    "title.basics.tsv.gz",
    "title.principals.tsv.gz",
    "name.basics.tsv.gz",
]

for filename in required_files:
    path = os.path.join(raw_folder, filename)
    if not os.path.exists(path):
        print("Missing file:", path)
        print("Download the IMDb files and place them in data/raw.")
        sys.exit(1)

os.makedirs(output_folder, exist_ok=True)

# Pick out the movies and keep their IDs handy.
movie_titles = {}

with gzip.open(
    os.path.join(raw_folder, "title.basics.tsv.gz"),
    "rt",
    encoding="utf-8",
) as file:
    rows = csv.DictReader(file, delimiter="\t")

    for row in rows:
        if row["titleType"] == "movie":
            movie_titles[row["tconst"]] = row["primaryTitle"]

print("Movies found:", len(movie_titles))

# Grab the actor IDs linked to those movies.
actor_ids = set()

with gzip.open(
    os.path.join(raw_folder, "title.principals.tsv.gz"),
    "rt",
    encoding="utf-8",
) as file:
    rows = csv.DictReader(file, delimiter="\t")

    for row in rows:
        is_movie = row["tconst"] in movie_titles
        is_actor = row["category"] == "actor" or row["category"] == "actress"

        if is_movie and is_actor:
            actor_ids.add(row["nconst"])

print("Actors found:", len(actor_ids))

# Match those actor IDs with names.
actor_names = {}

with gzip.open(
    os.path.join(raw_folder, "name.basics.tsv.gz"),
    "rt",
    encoding="utf-8",
) as file:
    rows = csv.DictReader(file, delimiter="\t")

    for row in rows:
        if row["nconst"] in actor_ids:
            actor_names[row["nconst"]] = row["primaryName"]

print("Actor names found:", len(actor_names))

# Write out the movie list.
with open(
    os.path.join(output_folder, "movies.tsv"),
    "w",
    encoding="utf-8",
    newline="",
) as file:
    writer = csv.writer(file, delimiter="\t", lineterminator="\n")
    writer.writerow(["tconst", "title"])

    for movie_id, title in movie_titles.items():
        writer.writerow([movie_id, title])

# Write out the actor list.
with open(
    os.path.join(output_folder, "actors.tsv"),
    "w",
    encoding="utf-8",
    newline="",
) as file:
    writer = csv.writer(file, delimiter="\t", lineterminator="\n")
    writer.writerow(["nconst", "name"])

    for actor_id, name in actor_names.items():
        writer.writerow([actor_id, name])

# Save the movie-actor links.
credit_count = 0

with gzip.open(
    os.path.join(raw_folder, "title.principals.tsv.gz"),
    "rt",
    encoding="utf-8",
) as source:
    rows = csv.DictReader(source, delimiter="\t")

    with open(
        os.path.join(output_folder, "credits.tsv"),
        "w",
        encoding="utf-8",
        newline="",
    ) as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(["tconst", "nconst"])

        for row in rows:
            is_movie = row["tconst"] in movie_titles
            is_actor = row["category"] == "actor" or row["category"] == "actress"
            has_name = row["nconst"] in actor_names

            if is_movie and is_actor and has_name:
                writer.writerow([row["tconst"], row["nconst"]])
                credit_count += 1

print("Saved movies.tsv and actors.tsv")
print("Credits saved:", credit_count)
print("Output folder:", output_folder)
