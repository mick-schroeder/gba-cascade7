#ifndef CASCADE7_RULES_H
#define CASCADE7_RULES_H

#include <array>

#include "cascade7/board.h"

namespace cascade7
{
    struct clear_mask
    {
        std::array<bool, board_size * board_size> cells{};
        int count = 0;
    };

    struct resolution_result
    {
        int cleared_cells = 0;
        int chains = 0;
    };

    struct clear_result
    {
        int cleared_numbered_cells = 0;
        int cracked_blank_cells = 0;
        int revealed_numbered_cells = 0;
    };

    class rules
    {
    public:
        [[nodiscard]] static resolution_result resolve(board& board);
        [[nodiscard]] static clear_mask find_matches(const board& board);
        [[nodiscard]] static clear_result clear_cells(board& board, const clear_mask& clear_mask);

    private:
        static void _apply_blank_hits(board& board, int row, int column, int hit_count, clear_result& result);
        [[nodiscard]] static int _horizontal_span(const board& board, int row, int column);
        [[nodiscard]] static int _vertical_span(const board& board, int row, int column);
    };
}

#endif
