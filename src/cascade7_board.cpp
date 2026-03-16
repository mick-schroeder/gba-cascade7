#include "cascade7/board.h"

#include "bn_assert.h"

namespace cascade7
{
    namespace
    {
        constexpr cell empty_cell{};

        constexpr cell numbered_cell(int value)
        {
            return cell{cell_kind::numbered, value};
        }

        constexpr cell blank_cell(int value)
        {
            return cell{cell_kind::blank, value};
        }

        constexpr cell cracked_blank_cell(int value)
        {
            return cell{cell_kind::cracked_blank, value};
        }
    }

    board::board()
    {
        clear();
    }

    void board::clear()
    {
        _cells.fill(empty_cell);
    }

    void board::load_seeded_layout()
    {
        clear();

        at(6, 0) = blank_cell(3);
        at(6, 1) = blank_cell(4);
        at(6, 2) = blank_cell(2);
        at(6, 3) = blank_cell(5);
        at(6, 4) = blank_cell(6);
        at(6, 5) = blank_cell(4);
        at(6, 6) = blank_cell(7);

        at(5, 0) = cracked_blank_cell(2);
        at(5, 1) = blank_cell(6);
        at(5, 2) = blank_cell(1);
        at(5, 3) = cracked_blank_cell(7);
        at(5, 4) = blank_cell(3);
        at(5, 5) = blank_cell(5);
        at(5, 6) = blank_cell(2);

        at(4, 1) = blank_cell(2);
        at(4, 2) = numbered_cell(7);
        at(4, 3) = numbered_cell(2);
        at(4, 4) = numbered_cell(3);
        at(4, 5) = numbered_cell(6);

        at(3, 1) = numbered_cell(2);
        at(3, 2) = numbered_cell(1);
        at(3, 3) = numbered_cell(3);
        at(3, 4) = numbered_cell(3);
        at(3, 5) = numbered_cell(6);

        at(2, 2) = numbered_cell(2);
        at(2, 3) = blank_cell(4);
        at(2, 4) = numbered_cell(6);
    }

    const cell& board::at(int row, int column) const
    {
        BN_ASSERT(row >= 0 && row < board_size, "Invalid row: ", row);
        BN_ASSERT(column >= 0 && column < board_size, "Invalid column: ", column);
        return _cells[_index(row, column)];
    }

    cell& board::at(int row, int column)
    {
        BN_ASSERT(row >= 0 && row < board_size, "Invalid row: ", row);
        BN_ASSERT(column >= 0 && column < board_size, "Invalid column: ", column);
        return _cells[_index(row, column)];
    }

    bool board::can_drop(int column) const
    {
        BN_ASSERT(column >= 0 && column < board_size, "Invalid column: ", column);
        return ! at(0, column).occupied();
    }

    bool board::drop(int column, const cell& new_cell, int& out_row)
    {
        if(! can_drop(column))
        {
            return false;
        }

        for(int row = board_size - 1; row >= 0; --row)
        {
            cell& slot = at(row, column);

            if(! slot.occupied())
            {
                slot = new_cell;
                out_row = row;
                return true;
            }
        }

        return false;
    }

    void board::apply_gravity()
    {
        for(int column = 0; column < board_size; ++column)
        {
            int write_row = board_size - 1;

            for(int row = board_size - 1; row >= 0; --row)
            {
                const cell source = at(row, column);

                if(source.occupied())
                {
                    if(write_row != row)
                    {
                        at(write_row, column) = source;
                        at(row, column) = empty_cell;
                    }

                    --write_row;
                }
            }

            for(; write_row >= 0; --write_row)
            {
                at(write_row, column) = empty_cell;
            }
        }
    }

    bool board::apply_gravity_step()
    {
        bool moved = false;

        for(int column = 0; column < board_size; ++column)
        {
            for(int row = board_size - 2; row >= 0; --row)
            {
                cell& source = at(row, column);
                cell& destination = at(row + 1, column);

                if(source.occupied() && ! destination.occupied())
                {
                    destination = source;
                    source = empty_cell;
                    moved = true;
                }
            }
        }

        return moved;
    }

    bool board::empty() const
    {
        for(const cell& current_cell : _cells)
        {
            if(current_cell.occupied())
            {
                return false;
            }
        }

        return true;
    }

    bool board::rise(const std::array<cell, board_size>& new_row)
    {
        for(int column = 0; column < board_size; ++column)
        {
            if(at(0, column).occupied())
            {
                return false;
            }
        }

        for(int row = 0; row < board_size - 1; ++row)
        {
            for(int column = 0; column < board_size; ++column)
            {
                at(row, column) = at(row + 1, column);
            }
        }

        for(int column = 0; column < board_size; ++column)
        {
            at(board_size - 1, column) = new_row[column];
        }

        return true;
    }

    int board::_index(int row, int column)
    {
        return (row * board_size) + column;
    }
}
