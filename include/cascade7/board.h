#ifndef CASCADE7_BOARD_H
#define CASCADE7_BOARD_H

#include <array>

#include "cascade7/constants.h"
#include "cascade7/types.h"

namespace cascade7
{
    class board
    {
    public:
        board();

        void clear();
        void load_demo_layout();

        [[nodiscard]] const cell& at(int row, int column) const;
        [[nodiscard]] cell& at(int row, int column);

        [[nodiscard]] bool can_drop(int column) const;
        bool drop(int column, const cell& new_cell, int& out_row);

        void apply_gravity();
        bool apply_gravity_step();
        [[nodiscard]] bool empty() const;
        bool rise(const std::array<cell, board_size>& new_row);

    private:
        [[nodiscard]] static int _index(int row, int column);

        std::array<cell, board_size * board_size> _cells;
    };
}

#endif
