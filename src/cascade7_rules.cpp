#include "cascade7/rules.h"

#include <array>

namespace cascade7
{
    clear_mask rules::find_matches(const board& board)
    {
        clear_mask result;

        for(int row = 0; row < board_size; ++row)
        {
            for(int column = 0; column < board_size; ++column)
            {
                const cell current = board.at(row, column);

                if(current.numbered())
                {
                    const int horizontal_span = _horizontal_span(board, row, column);
                    const int vertical_span = _vertical_span(board, row, column);

                    if(current.value == horizontal_span || current.value == vertical_span)
                    {
                        const int index = (row * board_size) + column;

                        if(! result.cells[index])
                        {
                            result.cells[index] = true;
                            ++result.count;
                        }
                    }
                }
            }
        }

        return result;
    }

    clear_result rules::clear_cells(board& board, const clear_mask& clear_mask)
    {
        clear_result result;
        std::array<int, board_size * board_size> blank_hits{};

        for(int row = 0; row < board_size; ++row)
        {
            for(int column = 0; column < board_size; ++column)
            {
                if(clear_mask.cells[(row * board_size) + column])
                {
                    ++result.cleared_numbered_cells;

                    constexpr int row_offsets[] = { -1, 1, 0, 0 };
                    constexpr int column_offsets[] = { 0, 0, -1, 1 };

                    for(int direction = 0; direction < 4; ++direction)
                    {
                        const int target_row = row + row_offsets[direction];
                        const int target_column = column + column_offsets[direction];

                        if(target_row >= 0 && target_row < board_size &&
                           target_column >= 0 && target_column < board_size)
                        {
                            const cell& target_cell = board.at(target_row, target_column);

                            if(target_cell.blank() || target_cell.cracked_blank())
                            {
                                ++blank_hits[(target_row * board_size) + target_column];
                            }
                        }
                    }

                    board.at(row, column) = cell{};
                }
            }
        }

        for(int row = 0; row < board_size; ++row)
        {
            for(int column = 0; column < board_size; ++column)
            {
                const int hit_count = blank_hits[(row * board_size) + column];

                if(hit_count)
                {
                    _apply_blank_hits(board, row, column, hit_count, result);
                }
            }
        }

        return result;
    }

    resolution_result rules::resolve(board& board)
    {
        resolution_result result;

        while(true)
        {
            const cascade7::clear_mask clear_mask = find_matches(board);

            if(! clear_mask.count)
            {
                break;
            }

            ++result.chains;
            result.cleared_cells += clear_cells(board, clear_mask).cleared_numbered_cells;
            board.apply_gravity();
        }

        return result;
    }

    int rules::_horizontal_span(const board& board, int row, int column)
    {
        int left = column;

        while(left > 0 && board.at(row, left - 1).occupied())
        {
            --left;
        }

        int right = column;

        while(right + 1 < board_size && board.at(row, right + 1).occupied())
        {
            ++right;
        }

        return (right - left) + 1;
    }

    int rules::_vertical_span(const board& board, int row, int column)
    {
        int top = row;

        while(top > 0 && board.at(top - 1, column).occupied())
        {
            --top;
        }

        int bottom = row;

        while(bottom + 1 < board_size && board.at(bottom + 1, column).occupied())
        {
            ++bottom;
        }

        return (bottom - top) + 1;
    }

    void rules::_apply_blank_hits(board& board, int row, int column, int hit_count, clear_result& result)
    {
        cell& target_cell = board.at(row, column);

        if(target_cell.blank())
        {
            if(hit_count >= 2)
            {
                target_cell.kind = cell_kind::numbered;
                ++result.revealed_numbered_cells;
                result.revealed_cells[(row * board_size) + column] = true;
            }
            else
            {
                target_cell.kind = cell_kind::cracked_blank;
                ++result.cracked_blank_cells;
                result.cracked_cells[(row * board_size) + column] = true;
            }
        }
        else if(target_cell.cracked_blank())
        {
            target_cell.kind = cell_kind::numbered;
            ++result.revealed_numbered_cells;
            result.revealed_cells[(row * board_size) + column] = true;
        }
    }
}
