"""Turns data/processed/*.tsv into the small files in packed/ that are stored in git.

Run from the project folder:  python scripts/pack_data.py
You only need this if you rebuild the data; the packed/ folder is already in git.

packed/actors.txt   one actor name per line, in actors.tsv order
packed/movies.txt   one movie title per line, in movies.tsv order
packed/credits.bin  3 little-endian uint32 (actors, movies, credits), then for each
                    movie: varint cast size, then the actor indexes sorted and
                    delta-coded as varints
"""
import os
import struct

SRC = os.path.join("data", "processed")
OUT = "packed"


def rows(name):
    with open(os.path.join(SRC, name), encoding="utf-8", newline="") as f:
        lines = f.read().split("\n")[1:]
    if lines and lines[-1] == "":
        lines.pop()
    return [l[:-1] if l.endswith("\r") else l for l in lines]


def varint(n, out):
    while n >= 0x80:
        out.append((n & 0x7F) | 0x80)
        n >>= 7
    out.append(n)


actor_index, names = {}, []
for i, line in enumerate(rows("actors.tsv")):
    key, name = line.split("\t", 1)
    actor_index[key] = i
    names.append(name)

movie_index, titles = {}, []
for i, line in enumerate(rows("movies.tsv")):
    key, title = line.split("\t", 1)
    movie_index[key] = i
    titles.append(title)

cast = [[] for _ in titles]
for line in rows("credits.tsv"):
    tconst, nconst = line.split("\t")
    m, a = movie_index.get(tconst), actor_index.get(nconst)
    if m is not None and a is not None:
        cast[m].append(a)

body, edges = bytearray(), 0
for members in cast:
    members = sorted(set(members))
    varint(len(members), body)
    prev = 0
    for a in members:
        varint(a - prev, body)
        prev = a
    edges += len(members)

os.makedirs(OUT, exist_ok=True)
with open(os.path.join(OUT, "actors.txt"), "wb") as f:
    f.write("\n".join(names).encode("utf-8"))
with open(os.path.join(OUT, "movies.txt"), "wb") as f:
    f.write("\n".join(titles).encode("utf-8"))
with open(os.path.join(OUT, "credits.bin"), "wb") as f:
    f.write(struct.pack("<III", len(names), len(titles), edges) + bytes(body))
print("Wrote", OUT, "-", len(names), "actors,", len(titles), "movies,", edges, "credits")
