#include "cascade7/game.h"

#include "bn_keypad.h"
#include "bn_sound_items.h"
#include "bn_sram.h"

#include "bn_algorithm.h"

#include "cascade7/scoring.h"

namespace cascade7
{
    namespace
    {
        struct save_data
        {
            char format_tag[8] = {};
            int high_score = 0;
        };

        constexpr int settle_frames = 6;
        constexpr int flash_frames = 16;
        constexpr int clear_frames = 10;
        constexpr int gravity_step_frames = 4;
        constexpr int rise_frames = 18;
        constexpr int starting_turns_per_level = 29;
        constexpr int minimum_turns_per_level = 5;
        constexpr int initial_repeat_frames = 8;
        constexpr int held_repeat_frames = 3;
        constexpr std::array<int, max_disc_value + 1> base_value_weights = { 0, 100, 100, 110, 110, 100, 90, 110 };
        constexpr const char save_format_tag[] = "C7SAVE1";

        [[nodiscard]] constexpr int turns_for_level(int level)
        {
            const int turns = starting_turns_per_level - (level - 1);
            return turns > minimum_turns_per_level ? turns : minimum_turns_per_level;
        }
    }

    game::game() :
        _next_piece(_generate_piece())
    {
        _load_save();
        _reset_new_game();
    }

    void game::update()
    {
        if(_score_popup_timer > 0)
        {
            --_score_popup_timer;
        }

        if(_phase != resolution_phase::idle)
        {
            _update_resolution();
            return;
        }

        if(bn::keypad::left_pressed())
        {
            _move_cursor(-1);
            _cursor_repeat_direction = -1;
            _cursor_repeat_frames = initial_repeat_frames;
        }
        else if(bn::keypad::right_pressed())
        {
            _move_cursor(1);
            _cursor_repeat_direction = 1;
            _cursor_repeat_frames = initial_repeat_frames;
        }
        else if(bn::keypad::l_pressed())
        {
            _cursor_column = 0;
            _cursor_repeat_direction = 0;
            _cursor_repeat_frames = 0;
        }
        else if(bn::keypad::r_pressed())
        {
            _cursor_column = board_size - 1;
            _cursor_repeat_direction = 0;
            _cursor_repeat_frames = 0;
        }
        else if(_cursor_repeat_direction < 0 && bn::keypad::left_held())
        {
            if(_cursor_repeat_frames > 0)
            {
                --_cursor_repeat_frames;
            }
            else
            {
                _move_cursor(-1);
                _cursor_repeat_frames = held_repeat_frames;
            }
        }
        else if(_cursor_repeat_direction > 0 && bn::keypad::right_held())
        {
            if(_cursor_repeat_frames > 0)
            {
                --_cursor_repeat_frames;
            }
            else
            {
                _move_cursor(1);
                _cursor_repeat_frames = held_repeat_frames;
            }
        }
        else
        {
            _cursor_repeat_direction = 0;
            _cursor_repeat_frames = 0;
        }

        if(bn::keypad::a_pressed())
        {
            _drop_piece();
        }

        if(bn::keypad::select_pressed())
        {
            _reset_new_game();
        }

        if(bn::keypad::start_pressed())
        {
            _reset_empty();
        }
    }

    const board& game::board_state() const
    {
        return _board;
    }

    const cell& game::next_piece() const
    {
        return _next_piece;
    }

    int game::cursor_column() const
    {
        return _cursor_column;
    }

    int game::score() const
    {
        return _score;
    }

    int game::level() const
    {
        return _level;
    }

    int game::turn() const
    {
        return _turn;
    }

    int game::high_score() const
    {
        return _high_score;
    }

    int game::current_chain_depth() const
    {
        return _cascade_chain_count;
    }

    int game::highest_chain() const
    {
        return _highest_chain;
    }

    int game::discs_cleared() const
    {
        return _discs_cleared;
    }

    int game::last_move_score() const
    {
        return _last_move_score;
    }

    int game::score_popup_value() const
    {
        return _score_popup_value;
    }

    int game::score_popup_chain() const
    {
        return _score_popup_chain;
    }

    int game::score_popup_timer() const
    {
        return _score_popup_timer;
    }

    int game::last_clear_count() const
    {
        return _last_clear_count;
    }

    int game::last_chain_count() const
    {
        return _last_chain_count;
    }

    int game::blocks_remaining() const
    {
        return _blocks_remaining;
    }

    int game::max_blocks_remaining() const
    {
        return _max_blocks;
    }

    bool game::game_over() const
    {
        return _game_over;
    }

    bool game::resolving() const
    {
        return _phase != resolution_phase::idle;
    }

    resolution_phase game::phase() const
    {
        return _phase;
    }

    int game::phase_timer() const
    {
        return _phase_timer;
    }

    const clear_mask& game::pending_clear_mask() const
    {
        return _pending_clear_mask;
    }

    bool game::has_pending_rise_row() const
    {
        return _has_pending_rise_row;
    }

    const std::array<cell, board_size>& game::pending_rise_row() const
    {
        return _pending_rise_row;
    }

    const bn::string<48>& game::status_text() const
    {
        return _status;
    }

    void game::_update_resolution()
    {
        if(_phase_timer > 0)
        {
            --_phase_timer;
            return;
        }

        switch(_phase)
        {
        case resolution_phase::settle:
            _pending_clear_mask = rules::find_matches(_board);

            if(_pending_clear_mask.count)
            {
                _phase = resolution_phase::flashing;
                _phase_timer = flash_frames;
                _set_status("MATCH");
            }
            else
            {
                _finish_resolution_step();
            }
            break;

        case resolution_phase::flashing:
            _phase = resolution_phase::clearing;
            _phase_timer = clear_frames;
            _set_status("CLEAR");
            break;

        case resolution_phase::clearing:
        {
            const clear_result clear_result = rules::clear_cells(_board, _pending_clear_mask);
            ++_cascade_chain_count;
            _cascade_cleared_cells += clear_result.cleared_numbered_cells;
            _discs_cleared += clear_result.cleared_numbered_cells;

            if(clear_result.cleared_numbered_cells)
            {
                _play_clear_sound(clear_result.cleared_numbered_cells);

                const int chain_score = scoring::per_disc_score(_cascade_chain_count) *
                                        clear_result.cleared_numbered_cells;
                _score += chain_score;
                _last_move_score += chain_score;
            }

            _phase = resolution_phase::gravity;
            _phase_timer = gravity_step_frames;
            _set_status("FALL");
            break;
        }

        case resolution_phase::gravity:
            if(_board.apply_gravity_step())
            {
                _phase_timer = gravity_step_frames;
                _set_status("FALL");
            }
            else
            {
                _phase = resolution_phase::settle;
                _phase_timer = settle_frames;
                _set_status("SETTLE");
            }
            break;

        case resolution_phase::rising:
            if(! _board.rise(_pending_rise_row))
            {
                _game_over = true;
                _set_status("OVERFLOW");
                _store_high_score_if_needed();
                _play_game_over_sound();
                _phase = resolution_phase::idle;
                _has_pending_rise_row = false;
                return;
            }

            _has_pending_rise_row = false;
            _phase = resolution_phase::settle;
            _phase_timer = settle_frames;
            _set_status("ROW RISE");
            break;

        case resolution_phase::idle:
        default:
            break;
        }
    }

    cell game::_generate_piece()
    {
        const int value = _generate_value();
        const int blank_roll = _random.get_int(100);

        if(blank_roll < _blank_spawn_chance())
        {
            const cell result{cell_kind::blank, value};
            _remember_generated_piece(result);
            return result;
        }

        return _generate_numbered_piece();
    }

    cell game::_generate_numbered_piece()
    {
        const cell result{cell_kind::numbered, _generate_value()};
        _remember_generated_piece(result);
        return result;
    }

    int game::_generate_value(bool prefer_high_values, bool prefer_low_values, bool unique_only,
                              const std::array<bool, max_disc_value + 1>& used_values)
    {
        std::array<int, max_disc_value + 1> weights = base_value_weights;
        int occupied_cells = 0;
        int blank_cells = 0;

        for(int row = 0; row < board_size; ++row)
        {
            for(int column = 0; column < board_size; ++column)
            {
                const cell& board_cell = _board.at(row, column);

                if(! board_cell.occupied())
                {
                    continue;
                }

                ++occupied_cells;

                if(board_cell.blank() || board_cell.cracked_blank())
                {
                    ++blank_cells;
                }

                if(board_cell.numbered())
                {
                    int horizontal_span = 1;

                    for(int left = column - 1; left >= 0 && _board.at(row, left).occupied(); --left)
                    {
                        ++horizontal_span;
                    }

                    for(int right = column + 1; right < board_size && _board.at(row, right).occupied(); ++right)
                    {
                        ++horizontal_span;
                    }

                    int vertical_span = 1;

                    for(int up = row - 1; up >= 0 && _board.at(up, column).occupied(); --up)
                    {
                        ++vertical_span;
                    }

                    for(int down = row + 1; down < board_size && _board.at(down, column).occupied(); ++down)
                    {
                        ++vertical_span;
                    }

                    for(int value = min_disc_value; value <= max_disc_value; ++value)
                    {
                        if(value == horizontal_span || value == vertical_span)
                        {
                            weights[value] += 20;
                        }
                        else if(value == horizontal_span - 1 || value == horizontal_span + 1 ||
                                value == vertical_span - 1 || value == vertical_span + 1)
                        {
                            weights[value] += 8;
                        }
                    }
                }
            }
        }

        if(occupied_cells <= 14 || prefer_low_values)
        {
            weights[1] += 15;
            weights[2] += 12;
            weights[3] += 10;
            weights[6] -= 8;
            weights[7] -= 12;
        }
        else if(occupied_cells >= 26 || prefer_high_values)
        {
            weights[1] -= 10;
            weights[2] -= 6;
            weights[5] += 8;
            weights[6] += 12;
            weights[7] += 15;
        }

        if(blank_cells >= 9)
        {
            weights[1] += 8;
            weights[2] += 8;
            weights[6] -= 6;
            weights[7] -= 8;
        }

        for(int index = 0; index < _recent_piece_count; ++index)
        {
            const int recent_value = _recent_piece_values[index];

            if(recent_value >= min_disc_value && recent_value <= max_disc_value)
            {
                weights[recent_value] = (weights[recent_value] * 4) / 5;
            }
        }

        if(_recent_piece_count >= 2)
        {
            const int latest_value = _recent_piece_values[0];

            if(latest_value == _recent_piece_values[1])
            {
                weights[latest_value] /= 2;
            }
        }

        int total_weight = 0;

        for(int value = min_disc_value; value <= max_disc_value; ++value)
        {
            if(unique_only && used_values[value])
            {
                weights[value] = 0;
            }
            else if(weights[value] < 12)
            {
                weights[value] = 12;
            }

            total_weight += weights[value];
        }

        if(total_weight <= 0)
        {
            return _random.get_int(min_disc_value, max_disc_value + 1);
        }

        int roll = _random.get_int(total_weight);

        for(int value = min_disc_value; value <= max_disc_value; ++value)
        {
            roll -= weights[value];

            if(roll < 0)
            {
                return value;
            }
        }

        return max_disc_value;
    }

    int game::_blank_spawn_chance() const
    {
        int occupied_cells = 0;
        int blank_cells = 0;

        for(int row = 0; row < board_size; ++row)
        {
            for(int column = 0; column < board_size; ++column)
            {
                const cell& board_cell = _board.at(row, column);

                if(board_cell.occupied())
                {
                    ++occupied_cells;

                    if(board_cell.blank() || board_cell.cracked_blank())
                    {
                        ++blank_cells;
                    }
                }
            }
        }

        int chance = 18 + bn::min(_level - 1, 8);

        if(_turns_until_rise <= 3)
        {
            chance += 5;
        }

        if(_last_clear_count == 0 && occupied_cells >= 18)
        {
            chance += 3;
        }

        if(blank_cells >= 8)
        {
            chance -= 8;
        }

        if(occupied_cells >= 30)
        {
            chance -= 6;
        }

        if(_recent_piece_count >= 2 &&
           _recent_piece_kinds[0] == cell_kind::blank &&
           _recent_piece_kinds[1] == cell_kind::blank)
        {
            chance -= 10;
        }

        return bn::clamp(chance, 12, 36);
    }

    void game::_move_cursor(int delta)
    {
        _cursor_column += delta;

        if(_cursor_column < 0)
        {
            _cursor_column = board_size - 1;
        }
        else if(_cursor_column >= board_size)
        {
            _cursor_column = 0;
        }
    }

    void game::_drop_piece()
    {
        if(_game_over || _phase != resolution_phase::idle)
        {
            _set_status("PRESS START");
            return;
        }

        [[maybe_unused]] int ignored_row = 0;

        if(! _board.drop(_cursor_column, _next_piece, ignored_row))
        {
            _set_status("COLUMN FULL");
            return;
        }

        ++_turn;
        --_turns_until_rise;
        _blocks_remaining = _turns_until_rise;
        _last_move_score = 0;
        _last_clear_count = 0;
        _last_chain_count = 0;
        _cascade_cleared_cells = 0;
        _cascade_chain_count = 0;
        _pending_clear_mask = clear_mask{};
        _phase = resolution_phase::settle;
        _phase_timer = settle_frames;
        _set_status("DROP");

        _next_piece = _generate_piece();
    }

    void game::_finish_resolution_step()
    {
        _phase = resolution_phase::idle;
        _last_clear_count = _cascade_cleared_cells;
        _last_chain_count = _cascade_chain_count;
        _pending_clear_mask = clear_mask{};
        _highest_chain = bn::max(_highest_chain, _cascade_chain_count);

        if(_cascade_cleared_cells)
        {
            _set_status(_cascade_chain_count > 1 ? "COMBO!" : "CASCADE!");

            if(_cascade_chain_count >= scoring::large_chain_threshold)
            {
                _play_chain_sound();
            }
        }
        else
        {
            _set_status("PLACED");
        }

        if(_board.empty())
        {
            _score += scoring::full_clear_bonus;
            _last_move_score += scoring::full_clear_bonus;
            _set_status("ALL CLEAR!");
        }

        if(_last_move_score >= scoring::large_popup_score_threshold ||
           _last_chain_count >= scoring::large_chain_threshold)
        {
            _score_popup_value = _last_move_score;
            _score_popup_chain = _last_chain_count;
            _score_popup_timer = scoring::popup_frames;
        }

        if(_turns_until_rise <= 0)
        {
            _raise_blank_row();
            _turns_until_rise = turns_for_level(_level);
            _blocks_remaining = _turns_until_rise;
            return;
        }
        else
        {
            _blocks_remaining = _turns_until_rise;
        }

        bool any_moves = false;

        for(int column = 0; column < board_size; ++column)
        {
            if(_board.can_drop(column))
            {
                any_moves = true;
                break;
            }
        }

        if(! any_moves)
        {
            _game_over = true;
            _set_status("BOARD FULL");
            _store_high_score_if_needed();
            _play_game_over_sound();
        }
    }

    void game::_set_status(const char* text)
    {
        _status = text;
    }

    void game::_load_save()
    {
        save_data stored_data;
        bn::sram::read(stored_data);

        bool valid = true;

        for(int index = 0; index < 8; ++index)
        {
            if(stored_data.format_tag[index] != save_format_tag[index])
            {
                valid = false;
                break;
            }
        }

        if(valid)
        {
            _high_score = stored_data.high_score;
            return;
        }

        _high_score = 0;

        save_data new_data;

        for(int index = 0; index < 8; ++index)
        {
            new_data.format_tag[index] = save_format_tag[index];
        }

        new_data.high_score = 0;
        bn::sram::write(new_data);
    }

    void game::_store_high_score_if_needed()
    {
        if(_score <= _high_score)
        {
            return;
        }

        _high_score = _score;

        save_data stored_data;

        for(int index = 0; index < 8; ++index)
        {
            stored_data.format_tag[index] = save_format_tag[index];
        }

        stored_data.high_score = _high_score;
        bn::sram::write(stored_data);
    }

    void game::_raise_blank_row()
    {
        _generate_rise_row();

        _has_pending_rise_row = true;
        _phase = resolution_phase::rising;
        _phase_timer = rise_frames;
        ++_level;
        _max_blocks = turns_for_level(_level);
        _score += scoring::rise_bonus;
        _last_move_score += scoring::rise_bonus;
        _set_status("ROW RISE");
        _play_rise_sound();
    }

    void game::_reset_empty()
    {
        _board.clear();
        _cursor_column = board_size / 2;
        _score = 0;
        _level = 1;
        _turn = 0;
        _highest_chain = 0;
        _discs_cleared = 0;
        _last_move_score = 0;
        _score_popup_value = 0;
        _score_popup_chain = 0;
        _score_popup_timer = 0;
        _last_clear_count = 0;
        _last_chain_count = 0;
        _max_blocks = turns_for_level(_level);
        _turns_until_rise = _max_blocks;
        _blocks_remaining = _turns_until_rise;
        _game_over = false;
        _has_pending_rise_row = false;
        _phase = resolution_phase::idle;
        _phase_timer = 0;
        _cascade_cleared_cells = 0;
        _cascade_chain_count = 0;
        _cursor_repeat_frames = 0;
        _cursor_repeat_direction = 0;
        _recent_piece_count = 0;
        _recent_piece_values.fill(0);
        _recent_piece_kinds.fill(cell_kind::empty);
        _pending_clear_mask = clear_mask{};
        _seed_opening_board();
        _next_piece = _generate_piece();
        _set_status("READY");
    }

    void game::_reset_new_game()
    {
        _reset_empty();
    }

    void game::_seed_opening_board()
    {
        constexpr int seeded_disc_count = 7;
        const std::array<int, 4> starting_recent_values = _recent_piece_values;
        const std::array<cell_kind, 4> starting_recent_kinds = _recent_piece_kinds;
        const int starting_recent_count = _recent_piece_count;

        for(int attempt = 0; attempt < 32; ++attempt)
        {
            _board.clear();
            _recent_piece_values = starting_recent_values;
            _recent_piece_kinds = starting_recent_kinds;
            _recent_piece_count = starting_recent_count;

            std::array<int, board_size> columns = { 0, 1, 2, 3, 4, 5, 6 };

            for(int index = board_size - 1; index > 0; --index)
            {
                const int swap_index = _random.get_int(index + 1);
                const int temp = columns[index];
                columns[index] = columns[swap_index];
                columns[swap_index] = temp;
            }

            const int distinct_columns = 5 + _random.get_int(2);
            const int blank_budget = 1 + _random.get_int(2);
            int blanks_used = 0;
            bool placement_failed = false;

            for(int seeded_disc = 0; seeded_disc < seeded_disc_count; ++seeded_disc)
            {
                int column = columns[seeded_disc % distinct_columns];

                if(seeded_disc >= distinct_columns)
                {
                    column = columns[_random.get_int(distinct_columns)];
                }

                bool prefer_low_values = seeded_disc < 4;
                cell new_cell = _generate_numbered_piece();

                if(blanks_used < blank_budget && _random.get_int(100) < 25)
                {
                    new_cell = cell{cell_kind::blank, _generate_value(false, prefer_low_values)};
                    _remember_generated_piece(new_cell);
                    ++blanks_used;
                }

                int ignored_row = 0;

                if(! _board.drop(column, new_cell, ignored_row))
                {
                    placement_failed = true;
                    break;
                }
            }

            if(placement_failed || rules::find_matches(_board).count)
            {
                continue;
            }

            int occupied_columns = 0;

            for(int column = 0; column < board_size; ++column)
            {
                bool column_occupied = false;

                for(int row = 0; row < board_size; ++row)
                {
                    if(_board.at(row, column).occupied())
                    {
                        column_occupied = true;
                        break;
                    }
                }

                occupied_columns += column_occupied;
            }

            if(occupied_columns >= 5)
            {
                return;
            }
        }

        _board.clear();
        _recent_piece_values = starting_recent_values;
        _recent_piece_kinds = starting_recent_kinds;
        _recent_piece_count = starting_recent_count;

        for(int seeded_disc = 0; seeded_disc < seeded_disc_count; ++seeded_disc)
        {
            int ignored_row = 0;
            _board.drop(seeded_disc, _generate_numbered_piece(), ignored_row);
        }
    }

    void game::_generate_rise_row()
    {
        static const int count_patterns[][board_size] = {
            { 2, 2, 2, 1, 0, 0, 0 },
            { 3, 2, 1, 1, 0, 0, 0 },
            { 2, 2, 1, 1, 1, 0, 0 },
            { 3, 1, 1, 1, 1, 0, 0 }
        };

        const int pattern_index = _random.get_int(4);
        const int distinct_values = count_patterns[pattern_index][4] ? 5 : 4;
        std::array<bool, max_disc_value + 1> used_values = {};
        std::array<int, board_size> row_values = {};
        int write_index = 0;

        for(int value_index = 0; value_index < distinct_values; ++value_index)
        {
            const bool prefer_high_values = _level >= 5;
            const int value = _generate_value(prefer_high_values, ! prefer_high_values, true, used_values);
            used_values[value] = true;

            for(int repeat = 0; repeat < count_patterns[pattern_index][value_index]; ++repeat)
            {
                row_values[write_index] = value;
                ++write_index;
            }
        }

        for(int index = board_size - 1; index > 0; --index)
        {
            const int swap_index = _random.get_int(index + 1);
            const int temp = row_values[index];
            row_values[index] = row_values[swap_index];
            row_values[swap_index] = temp;
        }

        for(int index = 0; index < board_size; ++index)
        {
            _pending_rise_row[index] = cell{cell_kind::blank, row_values[index]};
        }
    }

    void game::_remember_generated_piece(const cell& generated_piece)
    {
        for(int index = _recent_piece_count; index > 0; --index)
        {
            if(index < int(_recent_piece_values.size()))
            {
                _recent_piece_values[index] = _recent_piece_values[index - 1];
                _recent_piece_kinds[index] = _recent_piece_kinds[index - 1];
            }
        }

        _recent_piece_values[0] = generated_piece.value;
        _recent_piece_kinds[0] = generated_piece.kind;
        _recent_piece_count = bn::min(_recent_piece_count + 1, int(_recent_piece_values.size()));
    }

    void game::_play_clear_sound(int cleared_numbered_cells) const
    {
        const bn::fixed volume = cleared_numbered_cells >= 4 ? bn::fixed(0.9) : bn::fixed(0.7);
        bn::sound_items::cascade7_clear.play(volume);
    }

    void game::_play_chain_sound() const
    {
        bn::sound_items::cascade7_chain.play(0.85);
    }

    void game::_play_rise_sound() const
    {
        bn::sound_items::cascade7_rise.play(0.8);
    }

    void game::_play_game_over_sound() const
    {
        bn::sound_items::cascade7_game_over.play(0.9);
    }
}
