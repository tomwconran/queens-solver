#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <istream>
#include <vector>

#include "timer.h"

using Board_t = std::uint16_t;

class Board {
   private:
    static constexpr int maxSize{16};
    static constexpr Board_t column(int c) { return Board_t{1} << c; }
    static constexpr Board_t square(int c) {
        Board_t m{column(c)};
        return m << 1 | m | m >> 1;
    }

    std::vector<Board_t> m_queens;
    std::vector<Board_t> m_mask;
    std::vector<std::vector<Board_t>> m_colourMasks;
    std::vector<std::vector<int>> m_region;
    int m_size{};
    Board_t m_rowMask{};

    std::vector<Board_t>& getColourMask(int r, int c) {
        return m_colourMasks[m_region[r][c]];
    }

   public:
    explicit Board(std::istream& file)
        : m_queens(maxSize),
          m_mask(maxSize),
          m_colourMasks(maxSize, std::vector<Board_t>(maxSize)),
          m_region(maxSize, std::vector<int>(maxSize)) {
        char ch{};
        file >> m_size;
        assert(0 < m_size && m_size <= maxSize && "UNSUPPORTED SIZE");
        m_rowMask = (Board_t{1} << m_size) - 1;
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

    bool canPlace(int r, int c) const { return !(m_mask[r] & column(c)); }

    Board& place(int r, int c) {
        assert(canPlace(r, c) && "CANT MAKE THE MOVE");

        Board_t cMask{column(c)};
        Board_t sMask{square(c)};
        const auto& colourMask{getColourMask(r, c)};
        m_queens[r] = cMask;
        for (int i{0}; i < m_size; ++i) {
            switch (r - i) {
                case 0:
                    m_mask[i] |= m_rowMask;
                    break;
                case 1:
                case -1:
                    m_mask[i] |= sMask;
                    [[fallthrough]];
                default:
                    m_mask[i] |= cMask | colourMask[i];
            }
        }
        return *this;
    }

    bool isDone() {
        Board_t check{m_rowMask};
        for (auto q : m_queens) {
            if (!(check & q)) return false;
            check ^= q;
        }
        if (check) return false;
        return true;
    }

    std::ostream& print(std::ostream& out) const {
        for (int r{0}; r < m_size; ++r) {
            for (int c{0}; c < m_size; ++c) {
                std::cout << static_cast<char>(m_region[r][c] + 'A');
            }
            std::cout << '\n';
        }

        return out;
    }

    int size() const { return m_size; }
};

std::ostream& operator<<(std::ostream& out, const Board& board) {
    return board.print(out);
}

class BacktrackSolver {
   private:
    int m_calls{};

    bool backtrack(const Board& board, int r) {
        ++m_calls;
        if (board.size() == r) return true;
        for (int c{0}; c < board.size(); ++c) {
            if (!board.canPlace(r, c)) continue;
            if (backtrack(Board{board}.place(r, c), r + 1)) return true;
        }
        return false;
    }

   public:
    BacktrackSolver() = default;

    void solve(const Board& board) {
        m_calls = 0;
        Timer t{};
        std::cout << "Success: " << std::boolalpha << backtrack(board, 0)
                  << " Calls: " << m_calls << " Time: " << t.elapsed() << '\n';
    }
};

int main(int argc, char const* argv[]) {
    assert(argc > 1 && "NO TEST FILE GIVEN");
    std::ifstream file{argv[1]};
    assert(file && "NO SUCH FILE EXISTS");
    Board b{file};
    std::cout << b << '\n';
    BacktrackSolver bs{};
    bs.solve(b);
    return 0;
}