#include "cascade7/game.h"

#include "bn_keypad.h"

#include "bn_algorithm.h"

#include "cascade7/scoring.h"

namespace cascade7
{
    namespace
    {
        constexpr int settle_frames = 6;
        constexpr int flash_frames = 16;
        constexpr int clear_frames = 10;
        constexpr int gravity_step_frames = 4;
        constexpr int rise_frames = 18;
        constexpr int starting_turns_per_level = 20;
        constexpr int minimum_turns_per_level = 5;
        constexpr int initial_repeat_frames = 8;
        constexpr int held_repeat_frames = 3;

        [[nodiscard]] constexpr int turns_for_level(int level)
        {
            const int turns = starting_turns_per_level - (level - 1);
            return turns > minimum_turns_per_level ? turns : minimum_turns_per_level;
        }

        void seed_opening_discs(board& board, bn::random& random, const cell& (*generate_piece_fn)(void) = nullptr) = delete;
    }

    game::game() :
        _next_piece(_generate_piece())
    {
        _reset_demo();
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
            _reset_demo();
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
        const int piece_type = _random.get_int(5);
        const int value = _random.get_int(min_disc_value, max_disc_value + 1);

        if(piece_type == 0)
        {
            return cell{cell_kind::blank, value};
        }

        return cell{cell_kind::numbered, value};
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
        }
    }

    void game::_set_status(const char* text)
    {
        _status = text;
    }

    void game::_raise_blank_row()
    {
        for(cell& row_cell : _pending_rise_row)
        {
            row_cell = cell{cell_kind::blank, _random.get_int(min_disc_value, max_disc_value + 1)};
        }

        _has_pending_rise_row = true;
        _phase = resolution_phase::rising;
        _phase_timer = rise_frames;
        ++_level;
        _max_blocks = turns_for_level(_level);
        _score += scoring::rise_bonus;
        _last_move_score += scoring::rise_bonus;
        _set_status("ROW RISE");
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
        _pending_clear_mask = clear_mask{};

        for(int seeded_disc = 0; seeded_disc < 7; ++seeded_disc)
        {
            int row = _random.get_int(1, board_size);
            int column = _random.get_int(board_size);

            while(_board.at(row, column).occupied())
            {
                row = _random.get_int(1, board_size);
                column = _random.get_int(board_size);
            }

            _board.at(row, column) = _generate_piece();
        }

        _board.apply_gravity();
        _next_piece = _generate_piece();
        _set_status("READY");
    }

    void game::_reset_demo()
    {
        _reset_empty();
    }
}
