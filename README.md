# Highway Route Planner

## Overview

The program simulates an electric-vehicle highway where service stations are spaced along a one-dimensional road. Each station sits at a unique integer distance (km) from the start and holds a fleet of up to 512 rental cars, each with a given battery range. A driver renting a car at station *s* can reach any station whose distance from *s* does not exceed that car's range.

The core task is **route planning**: given a departure and an arrival station, find the path with the fewest stops. When multiple paths tie on stop count, the tiebreaker favours the path whose stops are closest to the start of the highway (formally, no other path may share the same final *k* stops while having an earlier stop before them).

## Supported Commands

The program reads commands from **stdin** and writes responses to **stdout**, one per line.

| Command | Description | Output |
|---|---|---|
| `aggiungi-stazione d n a₁ … aₙ` | Add a station at distance *d* with *n* cars of ranges *a₁ … aₙ*. No-op if a station at *d* already exists. | `aggiunta` / `non aggiunta` |
| `demolisci-stazione d` | Remove the station at distance *d*. | `demolita` / `non demolita` |
| `aggiungi-auto d a` | Add a car with range *a* to the station at distance *d*. | `aggiunta` / `non aggiunta` |
| `rottama-auto d a` | Remove one car with range *a* from the station at distance *d*. | `rottamata` / `non rottamata` |
| `pianifica-percorso s e` | Plan the shortest-stop route from station *s* to station *e*. Both stations are guaranteed to exist. | Space-separated stop distances, or `nessun percorso` |

## Example

Given this highway:

```
Station (km):    0     20          30       45       50
Cars (range):  —     5,10,15,25   40       30       20,25
```

- **20 → 50** produces `20 30 50` (not `20 45 50` — tiebreaker picks earlier intermediate stops).
- **50 → 20** produces `50 30 20` (backward travel is supported).

## Data Structures

- **Sorted station array** — stations are stored in a dynamically resized array kept sorted by distance, enabling O(log n) lookup via binary search.
- **Max-heap per station** — each station's car fleet is a 1-indexed max-heap (element `[0]` = size). The root always holds the maximum range, giving O(1) access to the best car available, which is the only value the route planner needs.
- **Linked list** — used during path reconstruction to build the route in the correct traversal order via front-insertion.

## Algorithm

Route planning uses a greedy/dynamic-programming sweep from start to end:

1. Iterate through stations from departure toward arrival.
2. At each station, use the best car (heap root) to determine the farthest reachable station.
3. For every reachable station, record the minimum hop count and predecessor.
4. After the sweep, backtrack from the arrival through predecessors to reconstruct the path.

Forward (left-to-right) and backward (right-to-left) travel are handled as two symmetric cases.

## Building and Running

```bash
gcc -O2 -o planner prova_finale.c
./planner < input.txt > output.txt
```

All integer values fit in 32 bits. Input and output use plain text, one command/response per line.

## Project Constraints

- At most 512 cars per station.
- All distances and ranges are non-negative 32-bit integers.
- No external libraries required — standard C (`stdio.h`, `stdlib.h`, `string.h`).
