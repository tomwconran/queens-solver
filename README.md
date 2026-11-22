# Queens Solver

<!-- One or two sentences: what this is and the headline result.
     e.g. what it solves, the approach in a phrase, and the 26×26 result. -->

<!-- Screenshot: a coloured solved board from the terminal, e.g.
     ![Solved 12×12 board](docs/solved.png) -->

## The puzzle

<!-- The rules in two or three sentences: N×N grid, N coloured regions,
     one queen per row, column and region, and no two queens touching
     (including diagonally). Mention it's the puzzle popularised by LinkedIn. -->

## Approach

### Representation

<!-- Bitboards: one 32-bit mask per row for blocked cells and for queens,
     per-region masks, and why this makes each rule a few bitwise operations
     (popcount, countr_zero). -->

### Deduction rules

<!-- What each rule does, in plain words. Order matches the code:
     - singleSquare / singleRow / singleColumn
     - blocksRow / blocksColumn (region confined to one line)
     - regionInRow / regionInColumn (line confined to one region)
     - kRegionsInkRows / kRegionsInkColumns (k regions confined to k lines)
     - blockedSquares (cells blocked by every placement of a region's queen)
     Note the restart loop: cheapest rules first, restart after any progress. -->

### Transposition

<!-- How column rules reuse row rules by transposing the puzzle (once)
     and the state (per call), instead of writing each rule twice. -->

### Search

<!-- Branch and propagate: deduce until stuck, check consistency, then
     branch on the row with the fewest free cells, deducing again after
     each guess. Contrast with plain row-by-row backtracking. -->

## Benchmarks

<!-- One or two sentences of setup:
     Apple Silicon MacBook Air, AppleClang 21, Release build (-O3),
     one run per puzzle, plain backtracking capped at 5 seconds.
     20×20 and 26×26 boards are generated and may have several solutions. -->

| Board | Puzzles | Hybrid: median time | Hybrid: worst time | Hybrid: guesses (median / max) | Solved with no guesses | Backtracking: solved in 5 s | Backtracking: guesses (median / max) |
|-------|---------|---------------------|--------------------|--------------------------------|------------------------|-----------------------------|--------------------------------------|
| 8×8   | 5  | 0.03 ms | 0.03 ms | 0 / 0   | 5 | 5 / 5  | 178 / 753         |
| 9×9   | 5  | 0.04 ms | 0.08 ms | 0 / 0   | 5 | 5 / 5  | 432 / 2,389       |
| 10×10 | 5  | 0.03 ms | 0.06 ms | 0 / 0   | 5 | 5 / 5  | 976 / 2,219       |
| 11×11 | 5  | 0.03 ms | 0.06 ms | 1 / 2   | 2 | 5 / 5  | 9,597 / 23,511    |
| 12×12 | 5  | 0.04 ms | 0.05 ms | 0 / 0   | 5 | 5 / 5  | 4,892 / 175,208   |
| 20×20 | 1  | 0.86 ms | 0.86 ms | 14      | 0 | 0 / 1  | timed out         |
| 26×26 | 30 | 4.0 ms  | 7.6 ms  | 21 / 25 | 0 | 0 / 30 | timed out         |

<!-- Two or three sentences interpreting it: deduction alone solves most
     small boards; backtracking is faster in wall-clock time on small boards
     (say why); from 20×20 up only the hybrid finishes. -->

## Building and running

Requires CMake 3.20+ and a C++20 compiler.

```sh
cmake -B build
cmake --build build
./build/queens tests/small/q11_1.txt          # hybrid solver (default)
./build/queens tests/small/q11_1.txt backtrack # plain backtracking
```

The build defaults to Release (`-O3`). Note that this removes `assert`s.

### Puzzle format

The first line is the board size N, followed by N lines of N letters, one letter per region (`A` onwards):

```
8
DDDGGGGG
DHHABAAG
...
```

## Tests

<!-- What's in tests/: small/ (8×8–12×12, unique solutions), q20_hard.txt,
     big26/ (30 generated 26×26 boards). How you verify results (isSolution). -->

## What I'd do next

<!-- A few bullets, e.g. the reverse k-for-k rule (rows confined to regions),
     a fast bit-matrix transpose, lookahead with propagation as a rule,
     replacing input asserts with proper error handling. -->
