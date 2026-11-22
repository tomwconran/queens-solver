#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <istream>
#include <optional>

#include "timer.h"

using State_t = std::uint32_t;

inline constexpr int maxSize{26};

struct State {
    std::array<State_t, maxSize> queens{};
    std::array<State_t, maxSize> mask{};
    State_t queenColours{};
};

class Puzzle {
   private:
    static constexpr State_t square(int c) {
        State_t m{column(c)};
        return m << 1 | m | m >> 1;
    }

    std::array<std::array<State_t, maxSize>, maxSize> m_colourMasks{};
    std::array<std::array<int, maxSize>, maxSize> m_region{};
    int m_size{};
    State_t m_rowMask{};

    const std::array<State_t, maxSize>& getColourMask(int r, int c) const {
        return m_colourMasks[m_region[r][c]];
    }

   public:
    static constexpr State_t column(int c) { return State_t{1} << c; }

    explicit Puzzle(std::istream& file) {
        char ch{};
        file >> m_size;
        assert(0 < m_size && m_size <= maxSize && "UNSUPPORTED SIZE");
        m_rowMask = (State_t{1} << m_size) - 1;
        for (int r{0}; r < m_size; ++r) {
            for (int c{0}; c < m_size; ++c) {
                file >> ch;
                assert(0 <= ch - 'A' && ch - 'A' < m_size &&
                       "INVALID CHAR IN BOARD");
                m_colourMasks[ch - 'A'][r] |= column(c);
                m_region[r][c] = ch - 'A';
            }
        }
    }

    static auto& transposeBitmap(auto& bitmap) {
        for (std::size_t r{0}; r < bitmap.size(); ++r) {
            for (std::size_t c{r + 1}; c < bitmap.size(); ++c) {
                if (((bitmap[r] >> c) & 1) == ((bitmap[c] >> r) & 1)) continue;
                bitmap[r] ^= Puzzle::column(c);
                bitmap[c] ^= Puzzle::column(r);
            }
        }
        return bitmap;
    }

    Puzzle transpose() const {
        Puzzle t{*this};
        for (int colour{0}; colour < m_size; ++colour) {
            transposeBitmap(t.m_colourMasks[colour]);
        }
        for (int r{0}; r < m_size; ++r) {
            for (int c{r + 1}; c < m_size; ++c) {
                std::swap(t.m_region[r][c], t.m_region[c][r]);
            }
        }
        return t;
    }

    static State& transposeState(State& state) {
        transposeBitmap(state.queens);
        transposeBitmap(state.mask);
        return state;
    }

    bool canPlace(const State& state, int r, int c) const {
        return !(state.mask[r] & column(c));
    }

    State& place(State& state, int r, int c) const {
        assert(canPlace(state, r, c) && "CANT MAKE THE MOVE");
        state.queenColours |= column(m_region[r][c]);
        State_t cMask{column(c)};
        State_t sMask{square(c)};
        const auto& colourMask{getColourMask(r, c)};
        state.queens[r] = cMask;
        for (int i{0}; i < m_size; ++i) {
            switch (r - i) {
                case 0:
                    state.mask[i] |= m_rowMask;
                    break;
                case 1:
                case -1:
                    state.mask[i] |= sMask;
                    [[fallthrough]];
                default:
                    state.mask[i] |= cMask | colourMask[i];
            }
        }
        return state;
    }

    State& cross(State& state, int r, int c) const {
        assert(!(state.mask[r] & column(c)) && "CELL IS ALREADY OCCUPIED");
        state.mask[r] |= column(c);
        return state;
    }

    State& crossRow(State& state, int r, State_t crossMask) const {
        assert(!(state.mask[r] & crossMask) && "A CELL IS ALREADY OCCUPIED");
        state.mask[r] |= crossMask;
        return state;
    }

    std::ostream& print(std::ostream& out) const {
        for (int r{0}; r < m_size; ++r) {
            for (int c{0}; c < m_size; ++c) {
                out << static_cast<char>(m_region[r][c] + 'A');
            }
            out << '\n';
        }

        return out;
    }

    int size() const { return m_size; }
    auto& colourMask(int colour) const { return m_colourMasks[colour]; }
    State_t rowMask() const { return m_rowMask; }
    int region(int r, int c) const { return m_region[r][c]; }
};

std::ostream& operator<<(std::ostream& out, const Puzzle& puzzle) {
    return puzzle.print(out);
}

struct Context {
    const Puzzle& puzzle;
    const Puzzle& transposed;
};

class BacktrackSolver {
   private:
    int m_calls{};

    std::optional<State> backtrack(const Puzzle& puzzle, State state, int r) {
        ++m_calls;
        if (puzzle.size() == r) return state;
        if (state.queens[r]) return backtrack(puzzle, state, r + 1);
        for (int c{0}; c < puzzle.size(); ++c) {
            if (!puzzle.canPlace(state, r, c)) continue;
            State nextState{state};
            std::optional<State> result{
                backtrack(puzzle, puzzle.place(nextState, r, c), r + 1)};
            if (result) return result;
        }
        return std::nullopt;
    }

   public:
    BacktrackSolver() = default;

    std::optional<State> solve(const Puzzle& puzzle, State s = State{}) {
        m_calls = 0;
        return backtrack(puzzle, s, 0);
    }
    int calls() const { return m_calls; }
};

class DeductionSolver {
   private:
    int m_passes{};

    static bool singleSquare(const Context& context, State& state) {
        bool found{false};
        for (int colour{0}; colour < context.puzzle.size(); ++colour) {
            if (state.queenColours & Puzzle::column(colour)) continue;
            bool valid{false};
            int qr{};
            int qc{};
            const auto& colourMask{context.puzzle.colourMask(colour)};
            for (int r{0}; r < context.puzzle.size(); ++r) {
                auto freeSpaces{colourMask[r] & ~state.mask[r]};
                if (!freeSpaces) continue;
                if (std::popcount(freeSpaces) != 1) {
                    valid = false;
                    break;
                }
                if (valid) {
                    valid = false;
                    break;
                }
                valid = true;
                qr = r;
                qc = std::countr_zero(freeSpaces);
            }
            if (valid) {
                found = true;
                state = context.puzzle.place(state, qr, qc);
            }
        }
        return found;
    }

    static bool singleRow(const Context& context, State& state) {
        bool found{false};
        for (int r{0}; r < context.puzzle.size(); ++r) {
            if (state.queens[r]) continue;
            State_t freeSpaces = ~state.mask[r] & context.puzzle.rowMask();
            if (std::popcount(freeSpaces) != 1) continue;
            found = true;
            context.puzzle.place(state, r, std::countr_zero(freeSpaces));
        }
        return found;
    }

    static bool singleColumn(const Context& context, State& state) {
        bool found{singleRow(Context{context.transposed, context.puzzle},
                             Puzzle::transposeState(state))};
        Puzzle::transposeState(state);
        return found;
    }

    static bool blocksRow(const Context& context, State& state) {
        bool found{false};
        for (int colour{0}; colour < context.puzzle.size(); ++colour) {
            if (state.queenColours & Puzzle::column(colour)) continue;
            bool valid{false};
            int qr{};
            State_t crossMask{};
            const auto& colourMask{context.puzzle.colourMask(colour)};
            for (int r{0}; r < context.puzzle.size(); ++r) {
                auto freeSpaces{colourMask[r] & ~state.mask[r]};
                if (!freeSpaces) continue;
                if (valid) {
                    valid = false;
                    break;
                }
                valid = true;
                qr = r;
                crossMask =
                    ~state.mask[r] & ~colourMask[r] & context.puzzle.rowMask();
            }
            if (valid && crossMask) {
                found = true;
                context.puzzle.crossRow(state, qr, crossMask);
            }
        }
        return found;
    }

    static bool blocksColumn(const Context& context, State& state) {
        bool found{blocksRow(Context{context.transposed, context.puzzle},
                             Puzzle::transposeState(state))};
        Puzzle::transposeState(state);
        return found;
    }

    static bool regionInRow(const Context& context, State& state) {
        bool found{false};
        for (int r{0}; r < context.puzzle.size(); ++r) {
            if (state.queens[r]) continue;
            State_t freeCells{~state.mask[r] & context.puzzle.rowMask()};
            if (!freeCells) continue;
            const int colour{
                context.puzzle.region(r, std::countr_zero(freeCells))};
            if (freeCells & ~context.puzzle.colourMask(colour)[r]) continue;
            for (int i{0}; i < context.puzzle.size(); ++i) {
                if (r == i) continue;
                State_t crossMask{context.puzzle.colourMask(colour)[i] &
                                  ~state.mask[i]};
                if (!crossMask) continue;
                found = true;
                context.puzzle.crossRow(state, i, crossMask);
            }
        }
        return found;
    }

    static bool regionInColumn(const Context& context, State& state) {
        bool found{regionInRow(Context{context.transposed, context.puzzle},
                               Puzzle::transposeState(state))};
        Puzzle::transposeState(state);
        return found;
    }

    static constexpr std::array rules{
        &singleSquare, &singleRow,   &singleColumn,  &blocksRow,
        &blocksColumn, &regionInRow, &regionInColumn};

    std::optional<State> deduce(const Context& context, State state) {
        bool progress{true};
        while (progress) {
            progress = false;
            ++m_passes;
            for (std::size_t i{0}; i < rules.size(); ++i) {
                if (rules[i](context, state)) {
                    progress = true;
                    break;
                }
            }
        }
        if (std::popcount(state.queenColours) == context.puzzle.size())
            return state;
        std::cout << "DEDUCTION FAILED USING BACKTRACKING\n";
        BacktrackSolver bs{};
        return bs.solve(context.puzzle, state);
    }

   public:
    DeductionSolver() = default;

    std::optional<State> solve(const Puzzle& puzzle, State s = State{}) {
        m_passes = 0;
        const Context c{puzzle, Puzzle{puzzle.transpose()}};
        return deduce(c, s);
    }
    int calls() const { return m_passes; }
};

struct Tester {
    void testPuzzle(std::istream& file, auto& solver) {
        Puzzle p{file};
        State_t testMask{};
        char ch{};
        Timer t{};
        std::optional<State> result = solver.solve(p);
        double time{t.elapsed()};
        bool correct{result};
        if (result) {
            for (int r{0}; r < p.size(); ++r) {
                testMask = 0;
                for (int c{0}; c < p.size(); ++c) {
                    file >> ch;
                    assert(0 <= ch - '0' && ch - '0' <= 1 &&
                           "INVALID BIT IN ANSWER");
                    if (ch - '0') testMask |= Puzzle::column(c);
                }
                correct = correct && !(testMask ^ result.value().queens[r]);
            }
        }
        std::cout << "Correct: " << std::boolalpha << correct
                  << " Calls: " << solver.calls() << " Time: " << time << '\n';
    }
};

int main(int argc, char const* argv[]) {
    assert(argc > 1 && "NO TEST FILE GIVEN");
    std::ifstream file{argv[1]};
    assert(file && "NO SUCH FILE EXISTS");
    DeductionSolver ds{};
    Tester t{};
    t.testPuzzle(file, ds);
    return 0;
}