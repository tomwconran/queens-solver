#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <istream>
#include <optional>
#include <ostream>
#include <string_view>

#include "timer.h"

using State_t = std::uint32_t;

struct RGB {
    int r{};
    int g{};
    int b{};
    std::ostream& backgroundColour(std::ostream& out) const {
        return out << "\x1b[48;2;" << r << ";" << g << ";" << b << "m";
    }
    std::ostream& textColour(std::ostream& out) const {
        return out << "\x1b[38;2;" << r << ";" << g << ";" << b << "m";
    }
};

inline constexpr int maxSize{26};
inline constexpr std::array<RGB, maxSize> palette{{
    {224, 224, 224},  // A  light grey
    {184, 178, 160},  // B  stone
    {183, 164, 222},  // C  lavender
    {158, 189, 250},  // D  light blue
    {247, 203, 154},  // E  peach
    {188, 222, 166},  // F  light green
    {238, 130, 104},  // G  coral
    {232, 243, 149},  // H  pale yellow
    {211, 161, 190},  // I  mauve
    {139, 231, 225},  // J  aqua
    {164, 201, 205},  // K  slate teal
    {230, 204, 147},  // L  wheat
    {212, 223, 171},  // M  sage
    {204, 164, 124},  // N  tan brown
    {255, 160, 200},  // O  rose
    {150, 200, 120},  // P  olive green
    {255, 230, 100},  // Q  gold
    {170, 150, 255},  // R  violet
    {120, 200, 170},  // S  sea green
    {255, 140, 60},   // T  orange
    {200, 200, 90},   // U  mustard
    {150, 180, 220},  // V  steel blue
    {255, 200, 230},  // W  pink
    {170, 120, 90},   // X  brown
    {130, 210, 255},  // Y  sky blue
    {220, 170, 255},  // Z  lilac
}};
inline constexpr RGB black{0, 0, 0};

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
        black.textColour(out);
        for (int r{0}; r < m_size; ++r) {
            for (int c{0}; c < m_size; ++c) {
                palette[m_region[r][c]].backgroundColour(out);
                out << static_cast<char>(m_region[r][c] + 'A') << ' ';
            }
            out << "\x1b[49m\n";
        }
        out << "\x1b[39m";
        return out;
    }

    std::ostream& print(std::ostream& out, const State& state) const {
        black.textColour(out);
        for (int r{0}; r < m_size; ++r) {
            for (int c{0}; c < m_size; ++c) {
                palette[m_region[r][c]].backgroundColour(out);
                if ((state.mask[r] >> c) & 1)
                    out << (((state.queens[r] >> c) & 1) ? "♛ " : "  ");
                else
                    out << static_cast<char>(m_region[r][c] + 'A') << ' ';
            }
            out << "\x1b[49m\n";
        }
        out << "\x1b[39m";
        return out;
    }

    bool isConsistent(const State& state) const {
        for (int colour{0}; colour < m_size; ++colour) {
            if (state.queenColours & Puzzle::column(colour)) continue;
            bool openCell{false};
            for (int r{0}; r < m_size; ++r) {
                if (!(m_colourMasks[colour][r] & ~state.mask[r])) continue;
                openCell = true;
                break;
            }
            if (!openCell) return false;
        }

        for (int r{0}; r < m_size; ++r) {
            if (state.queens[r]) continue;
            if (!(~state.mask[r] & m_rowMask)) return false;
        }

        State_t columnQueens{};
        State_t freeColumns{};
        for (int r{0}; r < m_size; ++r) {
            columnQueens |= state.queens[r];
            freeColumns |= ~state.mask[r];
        }
        freeColumns &= m_rowMask;
        if ((freeColumns | columnQueens) != m_rowMask) return false;

        return true;
    }

    bool isSolution(const State& state) const {
        State_t columnQueens{};
        for (int r{0}; r < m_size; ++r) {
            if (!state.queens[r]) return false;
            columnQueens |= state.queens[r];
        }
        return columnQueens == m_rowMask && state.queenColours == m_rowMask;
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

class DeductionSolver {
   public:
    enum class Status {
        solved,
        stuck,
        contradiction,
    };

   private:
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

    static bool blockkRegion(const Context& context, State& state, State_t rows,
                             State_t group) {
        bool found{false};
        while (rows) {
            int r{std::countr_zero(rows)};
            rows &= rows - 1;
            State_t colours{group};
            State_t colourCells{};
            while (colours) {
                int colour{std::countr_zero(colours)};
                colours &= colours - 1;
                colourCells |= context.puzzle.colourMask(colour)[r];
            }
            State_t crossMask{~colourCells & ~state.mask[r] &
                              context.puzzle.rowMask()};
            if (crossMask) {
                found = true;
                context.puzzle.crossRow(state, r, crossMask);
            }
        }
        return found;
    }

    static bool searchkRegions(const Context& context, State& state,
                               const auto& colourRows, const int k, int next,
                               int count, State_t rows, State_t group) {
        if (std::popcount(rows) > k) return false;
        if (count == k) {
            if (std::popcount(rows) == k) {
                return blockkRegion(context, state, rows, group);
            }
            return false;
        }
        for (int c{next}; c <= context.puzzle.size() - k + count; ++c) {
            if (!colourRows[c]) continue;
            if (std::popcount(colourRows[c]) > k) continue;
            if (searchkRegions(context, state, colourRows, k, c + 1, count + 1,
                               rows | colourRows[c],
                               group | Puzzle::column(c))) {
                return true;
            };
        }
        return false;
    }

    static bool kRegionsInkRows(const Context& context, State& state) {
        std::array<State_t, maxSize> colourRows{};
        for (int colour{0}; colour < context.puzzle.size(); ++colour) {
            if (state.queenColours & Puzzle::column(colour)) continue;
            const auto& colourMask{context.puzzle.colourMask(colour)};
            for (int r{0}; r < context.puzzle.size(); ++r) {
                if (colourMask[r] & ~state.mask[r])
                    colourRows[colour] |= Puzzle::column(r);
            }
        }

        for (int k{2};
             k <= context.puzzle.size() - std::popcount(state.queenColours) - 1;
             ++k) {
            if (searchkRegions(context, state, colourRows, k, 0, 0, State_t{},
                               State_t{}))
                return true;
        }

        return false;
    }

    static bool kRegionsInkColumns(const Context& context, State& state) {
        bool found{kRegionsInkRows(Context{context.transposed, context.puzzle},
                                   Puzzle::transposeState(state))};
        Puzzle::transposeState(state);
        return found;
    }

    static bool blockedSquares(const Context& context, State& state) {
        bool found{false};
        for (int colour{0}; colour < context.puzzle.size(); ++colour) {
            if (state.queenColours & Puzzle::column(colour)) continue;
            auto& colourMask{context.puzzle.colourMask(colour)};
            std::array<State_t, maxSize> crossMask{};
            crossMask.fill(~State_t{0});
            bool empty{true};
            for (int r{0}; r < context.puzzle.size(); ++r) {
                State_t tempColourMask{colourMask[r] & ~state.mask[r]};
                while (tempColourMask) {
                    empty = false;
                    int c{std::countr_zero(tempColourMask)};
                    tempColourMask &= tempColourMask - 1;
                    State newState{state};
                    context.puzzle.place(newState, r, c);
                    for (int i{0}; i < context.puzzle.size(); ++i) {
                        crossMask[i] &= newState.mask[i];
                    }
                }
            }
            if (empty) continue;
            for (int r{0}; r < context.puzzle.size(); ++r) {
                crossMask[r] &=
                    ~colourMask[r] & ~state.mask[r] & context.puzzle.rowMask();
                if (!crossMask[r]) continue;
                found = true;
                context.puzzle.crossRow(state, r, crossMask[r]);
            }
        }
        return found;
    }

    static constexpr std::array rules{
        &singleSquare,       &singleRow,     &singleColumn,   &blocksRow,
        &blocksColumn,       &regionInRow,   &regionInColumn, &kRegionsInkRows,
        &kRegionsInkColumns, &blockedSquares};
    static constexpr std::array<std::string_view, rules.size()> ruleNames{
        "singleSquare",   "singleRow",       "singleColumn",
        "blocksRow",      "blocksColumn",    "regionInRow",
        "regionInColumn", "kRegionsInkRows", "kRegionsInkColumns",
        "blockedSquares"};

   public:
    struct Stats {
        int branches{};
        int passes{};
        std::array<int, rules.size()> ruleCounts{};
    };

    static Status deduce(const Context& context, State& state, Stats& stats) {
        bool progress{true};
        while (progress) {
            ++stats.passes;
            progress = false;
            for (std::size_t i{0}; i < rules.size(); ++i) {
                if (rules[i](context, state)) {
                    ++stats.ruleCounts[i];
                    progress = true;
                    break;
                }
            }
            if (!context.puzzle.isConsistent(state))
                return Status::contradiction;
        }
        if (std::popcount(state.queenColours) == context.puzzle.size())
            return Status::solved;
        return Status::stuck;
    }

    static void printStats(std::ostream& out, const Stats& stats) {
        out << "Branches: " << stats.branches << " Passes: " << stats.passes;
        for (std::size_t i{0}; i < rules.size(); ++i) {
            if (!(i % 4)) out << "\n";
            out << ruleNames[i] << ": " << stats.ruleCounts[i] << ' ';
        }
        out << '\n';
    }
};

class BacktrackSolver {
   private:
    DeductionSolver::Stats m_stats{};

    std::optional<State> backtrack(const Puzzle& puzzle, State state, int r) {
        if (puzzle.size() == r) return state;
        if (state.queens[r]) return backtrack(puzzle, state, r + 1);
        for (int c{0}; c < puzzle.size(); ++c) {
            if (!puzzle.canPlace(state, r, c)) continue;
            ++m_stats.branches;
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
        m_stats = {};
        return backtrack(puzzle, s, 0);
    }
    const DeductionSolver::Stats& stats() const { return m_stats; }
};

class HybridSolver {
   private:
    DeductionSolver::Stats m_stats{};

    std::optional<State> backtrack(const Context& context, State state, int r) {
        if (context.puzzle.size() == r) return state;
        if (!context.puzzle.isConsistent(state)) return std::nullopt;
        if (state.queens[r]) return backtrack(context, state, r + 1);
        auto status{DeductionSolver::deduce(context, state, m_stats)};
        using enum DeductionSolver::Status;
        if (status == solved) return state;
        if (status == contradiction) return std::nullopt;
        for (int c{0}; c < context.puzzle.size(); ++c) {
            if (!context.puzzle.canPlace(state, r, c)) continue;
            ++m_stats.branches;
            State nextState{state};
            std::optional<State> result{backtrack(
                context, context.puzzle.place(nextState, r, c), r + 1)};
            if (result) return result;
        }
        return std::nullopt;
    }

   public:
    HybridSolver() = default;

    std::optional<State> solve(const Puzzle& puzzle, State s = State{}) {
        m_stats = {};
        const Puzzle t{puzzle.transpose()};
        const Context c{puzzle, t};
        return backtrack(c, s, 0);
    }
    const DeductionSolver::Stats& stats() const { return m_stats; }
};

struct Tester {
    void testPuzzle(std::istream& file, auto& solver) {
        Puzzle p{file};
        Timer t{};
        std::optional<State> result = solver.solve(p);
        double time{t.elapsed()};
        if (result) p.print(std::cout, result.value());
        std::cout << "Correct: " << std::boolalpha
                  << (result ? p.isSolution(result.value()) : false)
                  << " Time: " << time << '\n';
        DeductionSolver::printStats(std::cout, solver.stats());
    }
};

int main(int argc, char const* argv[]) {
    if (argc <= 1) {
        std::cerr << "No test file given" << '\n';
        return 1;
    }
    std::ifstream file{argv[1]};
    if (!file) {
        std::cerr << "File " << argv[1] << " does not exist" << '\n';
        return 1;
    }
    std::string_view solver{argc > 2 ? argv[2] : "hybrid"};
    Tester t{};
    if (solver == "hybrid") {
        HybridSolver hs{};
        t.testPuzzle(file, hs);
    } else if (solver == "backtrack") {
        BacktrackSolver bs{};
        t.testPuzzle(file, bs);
    } else {
        std::cerr << "Invalid solver" << '\n';
        return 1;
    }

    return 0;
}