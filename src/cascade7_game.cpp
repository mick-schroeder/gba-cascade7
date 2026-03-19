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
            int standard_high_score = 0;
            int fast_high_score = 0;
            bool has_run_state = false;
            int mode = 0;
            std::array<cell, board_size * board_size> board_cells{};
            cell next_piece;
            clear_mask pending_clear_mask;
            int cursor_column = board_size / 2;
            int score = 0;
            int level = 1;
            int turn = 0;
            int highest_chain = 0;
            int discs_cleared = 0;
            int last_move_score = 0;
            int score_popup_value = 0;
            int score_popup_chain = 0;
            int score_popup_timer = 0;
            int last_clear_count = 0;
            int last_chain_count = 0;
            int blocks_remaining = 5;
            int max_blocks = 5;
            int phase_timer = 0;
            int cascade_cleared_cells = 0;
            int cascade_chain_count = 0;
            int turns_until_rise = 5;
            int cursor_repeat_frames = 0;
            int cursor_repeat_direction = 0;
            int menu_selection = 0;
            int blank_effect_timer = 0;
            int cracked_blank_count = 0;
            int revealed_blank_count = 0;
            int landing_timer = 0;
            int last_drop_row = -1;
            int last_drop_column = -1;
            int all_clear_timer = 0;
            int rise_impact_timer = 0;
            int level_up_timer = 0;
            int status_timer = 0;
            bool game_over = false;
            bool has_pending_rise_row = false;
            int phase = 0;
            int overlay = 0;
            int mode_select_return_overlay = 0;
            std::array<cell, board_size> pending_rise_row{};
            std::array<bool, board_size * board_size> cracked_effect_mask{};
            std::array<bool, board_size * board_size> revealed_effect_mask{};
            std::array<int, 4> recent_piece_values{};
            std::array<cell_kind, 4> recent_piece_kinds{};
            int recent_piece_count = 0;
            char status[48] = {};
            bn::random random;
        };

        constexpr int settle_frames = 6;
        constexpr int flash_frames = 18;
        constexpr int clear_frames = 12;
        constexpr int gravity_step_frames = 3;
        constexpr int rise_frames = 18;
        constexpr int starting_turns_per_level = 29;
        constexpr int minimum_turns_per_level = 5;
        constexpr int initial_repeat_frames = 8;
        constexpr int held_repeat_frames = 3;
        constexpr std::array<int, max_disc_value + 1> base_value_weights = { 0, 102, 94, 96, 110, 92, 80, 112 };
        constexpr const char save_format_tag[] = "C7SAVE2";
        constexpr int clear_sound_priority = 0;
        constexpr int crack_sound_priority = 1;
        constexpr int reveal_sound_priority = 2;
        constexpr int chain_sound_priority = 3;
        constexpr int rise_sound_priority = 4;
        constexpr int game_over_sound_priority = 5;
        constexpr int status_frames = 48;

        [[nodiscard]] constexpr int mode_index(game_mode mode)
        {
            return mode == game_mode::fast ? 1 : 0;
        }

        [[nodiscard]] constexpr int turns_for_level(game_mode mode, int level)
        {
            if(mode == game_mode::fast)
            {
                return 5;
            }

            // The extracted Drop7 sequence declines by one turn per level until it
            // hits a short late-game floor. Keep the same pacing shape, but keep
            // actual piece values random.
            const int turns = starting_turns_per_level - level + 1;
            return turns > minimum_turns_per_level ? turns : minimum_turns_per_level;
        }

        [[nodiscard]] constexpr resolution_phase sanitize_phase(int saved_phase)
        {
            return saved_phase >= int(resolution_phase::idle) && saved_phase <= int(resolution_phase::rising) ?
                        resolution_phase(saved_phase) : resolution_phase::idle;
        }

        [[nodiscard]] constexpr overlay_mode sanitize_overlay(int saved_overlay)
        {
            return saved_overlay >= int(overlay_mode::none) && saved_overlay <= int(overlay_mode::about_screen) ?
                        overlay_mode(saved_overlay) : overlay_mode::none;
        }
    }

    game::game() :
        _next_piece(cell{cell_kind::numbered, 1})
    {
        _load_save();
    }

    void game::update()
    {
        if(_score_popup_timer > 0)
        {
            --_score_popup_timer;
        }

        if(_blank_effect_timer > 0)
        {
            --_blank_effect_timer;
        }

        if(_landing_timer > 0)
        {
            --_landing_timer;
        }

        if(_all_clear_timer > 0)
        {
            --_all_clear_timer;
        }

        if(_rise_impact_timer > 0)
        {
            --_rise_impact_timer;
        }

        if(_level_up_timer > 0)
        {
            --_level_up_timer;
        }

        if(_status_timer > 0)
        {
            --_status_timer;
        }

        // Overlay pages own input until they are dismissed.
        if(_overlay != overlay_mode::none)
        {
            _handle_overlay_input();
            return;
        }

        // Resolution phases advance automatically between player moves.
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

        if(bn::keypad::start_pressed() || bn::keypad::select_pressed())
        {
            _open_pause_menu();
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
        return _high_scores[mode_index(_mode)];
    }

    game_mode game::mode() const
    {
        return _mode;
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

    overlay_mode game::overlay() const
    {
        return _overlay;
    }

    int game::menu_selection() const
    {
        return _menu_selection;
    }

    int game::blank_effect_timer() const
    {
        return _blank_effect_timer;
    }

    int game::cracked_blank_count() const
    {
        return _cracked_blank_count;
    }

    int game::revealed_blank_count() const
    {
        return _revealed_blank_count;
    }

    int game::landing_timer() const
    {
        return _landing_timer;
    }

    int game::last_drop_row() const
    {
        return _last_drop_row;
    }

    int game::last_drop_column() const
    {
        return _last_drop_column;
    }

    int game::all_clear_timer() const
    {
        return _all_clear_timer;
    }

    int game::rise_impact_timer() const
    {
        return _rise_impact_timer;
    }

    int game::level_up_timer() const
    {
        return _level_up_timer;
    }

    int game::status_timer() const
    {
        return _status_timer;
    }

    const clear_mask& game::pending_clear_mask() const
    {
        return _pending_clear_mask;
    }

    const std::array<bool, board_size * board_size>& game::cracked_effect_mask() const
    {
        return _cracked_effect_mask;
    }

    const std::array<bool, board_size * board_size>& game::revealed_effect_mask() const
    {
        return _revealed_effect_mask;
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

    void game::_handle_overlay_input()
    {
        int option_count = 1;

        if(_overlay == overlay_mode::pause_menu)
        {
            option_count = 4;
        }
        else if(_overlay == overlay_mode::mode_select)
        {
            option_count = 2;
        }

        if(bn::keypad::up_pressed())
        {
            _menu_selection = (_menu_selection + option_count - 1) % option_count;
            return;
        }

        if(bn::keypad::down_pressed())
        {
            _menu_selection = (_menu_selection + 1) % option_count;
            return;
        }

        if((_overlay == overlay_mode::pause_menu ||
            _overlay == overlay_mode::mode_select ||
            _overlay == overlay_mode::help_screen ||
            _overlay == overlay_mode::about_screen) &&
           (bn::keypad::b_pressed() || bn::keypad::start_pressed() || bn::keypad::select_pressed()))
        {
            _close_overlay();
            return;
        }

        if(bn::keypad::a_pressed())
        {
            _confirm_overlay_selection();
        }
    }

    void game::_open_pause_menu()
    {
        if(_game_over || _phase != resolution_phase::idle)
        {
            return;
        }

        _overlay = overlay_mode::pause_menu;
        _menu_selection = 0;
        _set_status("PAUSED");
        _store_save();
    }

    void game::_open_game_over_menu()
    {
        _overlay = overlay_mode::game_over_menu;
        _menu_selection = 0;
        _store_save();
    }

    void game::_open_mode_select(overlay_mode return_overlay)
    {
        _mode_select_return_overlay = return_overlay;
        _overlay = overlay_mode::mode_select;
        _menu_selection = mode_index(_mode);
        _set_status("SELECT MODE");
        _store_save();
    }

    void game::_close_overlay()
    {
        if(_overlay == overlay_mode::mode_select)
        {
            _overlay = _mode_select_return_overlay;
        }
        else
        {
            _overlay = overlay_mode::none;
        }

        _menu_selection = 0;
        _set_status(_overlay == overlay_mode::pause_menu ? "PAUSED" :
                    (_overlay == overlay_mode::game_over_menu ? "GAME OVER" : "READY"));
        _store_save();
    }

    void game::_confirm_overlay_selection()
    {
        // The pause menu can branch into read-only pages or restart the run.
        if(_overlay == overlay_mode::pause_menu)
        {
            switch(_menu_selection)
            {
            case 0:
                _close_overlay();
                break;

            case 1:
                _overlay = overlay_mode::help_screen;
                _menu_selection = 0;
                break;

            case 2:
                _overlay = overlay_mode::about_screen;
                _menu_selection = 0;
                break;

            case 3:
                _open_mode_select(overlay_mode::pause_menu);
                break;

            default:
                break;
            }

            return;
        }

        if(_overlay == overlay_mode::mode_select)
        {
            const game_mode selected_mode = _menu_selection == 1 ? game_mode::fast : game_mode::standard;
            _overlay = overlay_mode::none;
            _menu_selection = 0;
            _mode = selected_mode;
            _reset_new_game();
            return;
        }

        if(_overlay == overlay_mode::help_screen || _overlay == overlay_mode::about_screen)
        {
            _overlay = overlay_mode::pause_menu;
            _menu_selection = 0;
            _set_status("PAUSED");
            return;
        }

        switch(_menu_selection)
        {
        case 0:
            _open_mode_select(overlay_mode::game_over_menu);
            break;

        default:
            break;
        }
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
            // Matches are only checked after discs have finished settling.
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
            _cracked_blank_count = clear_result.cracked_blank_cells;
            _revealed_blank_count = clear_result.revealed_numbered_cells;
            _cracked_effect_mask = clear_result.cracked_cells;
            _revealed_effect_mask = clear_result.revealed_cells;
            _blank_effect_timer = (_cracked_blank_count || _revealed_blank_count) ? 18 : 0;

            if(clear_result.cleared_numbered_cells)
            {
                const int chain_score = scoring::per_disc_score(_cascade_chain_count) *
                                        clear_result.cleared_numbered_cells;
                _score += chain_score;
                _last_move_score += chain_score;
            }

            if(_revealed_blank_count)
            {
                _play_reveal_sound(_revealed_blank_count);
            }
            else if(_cracked_blank_count)
            {
                _play_crack_sound(_cracked_blank_count);
            }
            else if(clear_result.cleared_numbered_cells)
            {
                _play_clear_sound(clear_result.cleared_numbered_cells);
            }

            _phase = resolution_phase::gravity;
            _phase_timer = gravity_step_frames;
            if(_revealed_blank_count)
            {
                _set_status("REVEAL!");
            }
            else if(_cracked_blank_count)
            {
                _set_status("CRACK!");
            }
            else
            {
                _set_status("FALL");
            }
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
                _open_game_over_menu();
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
        if(_mode == game_mode::fast)
        {
            return _generate_numbered_piece();
        }

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
        // Keep values random, but with a light distribution shape and recent-piece
        // dampening so the game avoids obvious streaks.
        std::array<int, max_disc_value + 1> weights = base_value_weights;
        int occupied_cells = 0;

        for(int row = 0; row < board_size; ++row)
        {
            for(int column = 0; column < board_size; ++column)
            {
                if(_board.at(row, column).occupied())
                {
                    ++occupied_cells;
                }
            }
        }

        if(occupied_cells <= 14 || prefer_low_values)
        {
            weights[1] += 14;
            weights[2] += 8;
            weights[3] += 8;
            weights[6] -= 6;
            weights[7] -= 4;
        }
        else if(occupied_cells >= 26 || prefer_high_values)
        {
            weights[1] -= 8;
            weights[2] -= 5;
            weights[5] += 8;
            weights[6] += 12;
            weights[7] += 14;
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
        // Blank pressure should be readable and predictable: slowly increase with
        // level, spike a bit near row rise, and back off if blanks are already
        // stacking up.
        int blank_cells = 0;

        for(int row = 0; row < board_size; ++row)
        {
            for(int column = 0; column < board_size; ++column)
            {
                const cell& board_cell = _board.at(row, column);

                if(board_cell.blank() || board_cell.cracked_blank())
                {
                    ++blank_cells;
                }
            }
        }

        int chance = 18 + bn::min(_level - 1, 8);

        if(_turns_until_rise <= 2)
        {
            chance += 5;
        }
        else if(_turns_until_rise <= 4)
        {
            chance += 3;
        }

        if(_last_clear_count == 0)
        {
            chance += 2;
        }

        if(blank_cells >= 8)
        {
            chance -= 9;
        }
        else if(blank_cells >= 5)
        {
            chance -= 4;
        }

        if(_recent_piece_count >= 2 &&
           _recent_piece_kinds[0] == cell_kind::blank &&
           _recent_piece_kinds[1] == cell_kind::blank)
        {
            chance -= 14;
        }

        return bn::clamp(chance, 12, 34);
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

        int drop_row = 0;

        if(! _board.drop(_cursor_column, _next_piece, drop_row))
        {
            _set_status("COLUMN FULL");
            return;
        }

        _last_drop_column = _cursor_column;
        _last_drop_row = drop_row;
        _landing_timer = 8;
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

        // The preview is always rolled immediately after a successful drop.
        _next_piece = _generate_piece();
        _store_save();
    }

    void game::_finish_resolution_step()
    {
        // One turn ends only after all clears, gravity steps and row rises finish.
        _phase = resolution_phase::idle;
        _last_clear_count = _cascade_cleared_cells;
        _last_chain_count = _cascade_chain_count;
        _pending_clear_mask = clear_mask{};
        _cracked_blank_count = 0;
        _revealed_blank_count = 0;
        _cracked_effect_mask.fill(false);
        _revealed_effect_mask.fill(false);
        _highest_chain = bn::max(_highest_chain, _cascade_chain_count);

        if(_cascade_cleared_cells)
        {
            if(_cascade_chain_count >= scoring::large_chain_threshold)
            {
                _play_chain_sound();
            }
        }

        if(_board.empty())
        {
            _score += scoring::full_clear_bonus;
            _last_move_score += scoring::full_clear_bonus;
            _all_clear_timer = 24;
        }

        if(_last_move_score > 0)
        {
            if(_board.empty())
            {
                _set_score_status("ALL CLEAR", _last_move_score);
            }
            else
            {
                _set_combo_score_status(_last_chain_count, _last_move_score);
            }
       
        }
        else
        {
            _set_status("PLACED");
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
            _turns_until_rise = turns_for_level(_mode, _level);
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
            _open_game_over_menu();
        }
        else
        {
            _store_save();
        }
    }

    void game::_set_status(const char* text)
    {
        _status = text;
        _status_timer = status_frames;
    }

    void game::_set_score_status(const char* prefix, int score)
    {
        _status.clear();

        if(prefix[0])
        {
            _status += prefix;
            _status += ' ';
        }

        _status += '+';
        _status += bn::to_string<10>(score);
        _status_timer = status_frames;
        _store_save();
    }

    void game::_set_combo_score_status(int chain_count, int score)
    {
        _status.clear();
        _status += "";
        _status += bn::to_string<4>(chain_count);
        _status += "x +";
        _status += bn::to_string<10>(score);
        _status_timer = status_frames;
        _store_save();
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
            _high_scores[0] = stored_data.standard_high_score;
            _high_scores[1] = stored_data.fast_high_score;
            if(stored_data.has_run_state)
            {
                _mode = stored_data.mode == 1 ? game_mode::fast : game_mode::standard;

                for(int row = 0; row < board_size; ++row)
                {
                    for(int column = 0; column < board_size; ++column)
                    {
                        _board.at(row, column) = stored_data.board_cells[(row * board_size) + column];
                    }
                }

                _next_piece = stored_data.next_piece;
                _pending_clear_mask = stored_data.pending_clear_mask;
                _cursor_column = stored_data.cursor_column;
                _score = stored_data.score;
                _level = stored_data.level;
                _turn = stored_data.turn;
                _highest_chain = stored_data.highest_chain;
                _discs_cleared = stored_data.discs_cleared;
                _last_move_score = stored_data.last_move_score;
                _score_popup_value = stored_data.score_popup_value;
                _score_popup_chain = stored_data.score_popup_chain;
                _score_popup_timer = stored_data.score_popup_timer;
                _last_clear_count = stored_data.last_clear_count;
                _last_chain_count = stored_data.last_chain_count;
                _blocks_remaining = stored_data.blocks_remaining;
                _max_blocks = stored_data.max_blocks;
                _phase_timer = stored_data.phase_timer;
                _cascade_cleared_cells = stored_data.cascade_cleared_cells;
                _cascade_chain_count = stored_data.cascade_chain_count;
                _turns_until_rise = stored_data.turns_until_rise;
                _cursor_repeat_frames = stored_data.cursor_repeat_frames;
                _cursor_repeat_direction = stored_data.cursor_repeat_direction;
                _menu_selection = stored_data.menu_selection;
                _blank_effect_timer = stored_data.blank_effect_timer;
                _cracked_blank_count = stored_data.cracked_blank_count;
                _revealed_blank_count = stored_data.revealed_blank_count;
                _landing_timer = stored_data.landing_timer;
                _last_drop_row = stored_data.last_drop_row;
                _last_drop_column = stored_data.last_drop_column;
                _all_clear_timer = stored_data.all_clear_timer;
                _rise_impact_timer = stored_data.rise_impact_timer;
                _level_up_timer = stored_data.level_up_timer;
                _status_timer = stored_data.status_timer;
                _game_over = stored_data.game_over;
                _has_pending_rise_row = stored_data.has_pending_rise_row;
                _phase = sanitize_phase(stored_data.phase);
                _overlay = sanitize_overlay(stored_data.overlay);
                _mode_select_return_overlay = sanitize_overlay(stored_data.mode_select_return_overlay);
                _pending_rise_row = stored_data.pending_rise_row;
                _cracked_effect_mask = stored_data.cracked_effect_mask;
                _revealed_effect_mask = stored_data.revealed_effect_mask;
                _recent_piece_values = stored_data.recent_piece_values;
                _recent_piece_kinds = stored_data.recent_piece_kinds;
                _recent_piece_count = stored_data.recent_piece_count;
                _status.clear();

                for(char character : stored_data.status)
                {
                    if(! character)
                    {
                        break;
                    }

                    _status += character;
                }

                _random = stored_data.random;
                return;
            }

            _reset_new_game();
            return;
        }

        _high_scores[0] = 0;
        _high_scores[1] = 0;

        save_data new_data;

        // Initialize SRAM the first time we see an unknown save signature.
        for(int index = 0; index < 8; ++index)
        {
            new_data.format_tag[index] = save_format_tag[index];
        }

        new_data.standard_high_score = 0;
        new_data.fast_high_score = 0;
        bn::sram::write(new_data);
        _reset_new_game();
    }

    void game::_store_save() const
    {
        save_data stored_data;

        for(int index = 0; index < 8; ++index)
        {
            stored_data.format_tag[index] = save_format_tag[index];
        }

        stored_data.standard_high_score = _high_scores[0];
        stored_data.fast_high_score = _high_scores[1];
        stored_data.has_run_state = true;
        stored_data.mode = mode_index(_mode);

        for(int row = 0; row < board_size; ++row)
        {
            for(int column = 0; column < board_size; ++column)
            {
                stored_data.board_cells[(row * board_size) + column] = _board.at(row, column);
            }
        }

        stored_data.next_piece = _next_piece;
        stored_data.pending_clear_mask = _pending_clear_mask;
        stored_data.cursor_column = _cursor_column;
        stored_data.score = _score;
        stored_data.level = _level;
        stored_data.turn = _turn;
        stored_data.highest_chain = _highest_chain;
        stored_data.discs_cleared = _discs_cleared;
        stored_data.last_move_score = _last_move_score;
        stored_data.score_popup_value = _score_popup_value;
        stored_data.score_popup_chain = _score_popup_chain;
        stored_data.score_popup_timer = _score_popup_timer;
        stored_data.last_clear_count = _last_clear_count;
        stored_data.last_chain_count = _last_chain_count;
        stored_data.blocks_remaining = _blocks_remaining;
        stored_data.max_blocks = _max_blocks;
        stored_data.phase_timer = _phase_timer;
        stored_data.cascade_cleared_cells = _cascade_cleared_cells;
        stored_data.cascade_chain_count = _cascade_chain_count;
        stored_data.turns_until_rise = _turns_until_rise;
        stored_data.cursor_repeat_frames = _cursor_repeat_frames;
        stored_data.cursor_repeat_direction = _cursor_repeat_direction;
        stored_data.menu_selection = _menu_selection;
        stored_data.blank_effect_timer = _blank_effect_timer;
        stored_data.cracked_blank_count = _cracked_blank_count;
        stored_data.revealed_blank_count = _revealed_blank_count;
        stored_data.landing_timer = _landing_timer;
        stored_data.last_drop_row = _last_drop_row;
        stored_data.last_drop_column = _last_drop_column;
        stored_data.all_clear_timer = _all_clear_timer;
        stored_data.rise_impact_timer = _rise_impact_timer;
        stored_data.level_up_timer = _level_up_timer;
        stored_data.status_timer = _status_timer;
        stored_data.game_over = _game_over;
        stored_data.has_pending_rise_row = _has_pending_rise_row;
        stored_data.phase = int(_phase);
        stored_data.overlay = int(_overlay);
        stored_data.mode_select_return_overlay = int(_mode_select_return_overlay);
        stored_data.pending_rise_row = _pending_rise_row;
        stored_data.cracked_effect_mask = _cracked_effect_mask;
        stored_data.revealed_effect_mask = _revealed_effect_mask;
        stored_data.recent_piece_values = _recent_piece_values;
        stored_data.recent_piece_kinds = _recent_piece_kinds;
        stored_data.recent_piece_count = _recent_piece_count;
        for(int index = 0; index < _status.size() && index < 47; ++index)
        {
            stored_data.status[index] = _status[index];
        }

        stored_data.status[bn::min(_status.size(), 47)] = 0;
        stored_data.random = _random;
        bn::sram::write(stored_data);
    }

    void game::_store_high_score_if_needed()
    {
        const int current_mode_index = mode_index(_mode);

        if(_score <= _high_scores[current_mode_index])
        {
            return;
        }

        _high_scores[current_mode_index] = _score;

        _store_save();
    }

    void game::_raise_blank_row()
    {
        _generate_rise_row();

        _has_pending_rise_row = true;
        _phase = resolution_phase::rising;
        _phase_timer = rise_frames;
        _rise_impact_timer = 16;
        _level_up_timer = 48;
        ++_level;
        _max_blocks = turns_for_level(_mode, _level);
        _score += scoring::rise_bonus;
        _last_move_score += scoring::rise_bonus;
        _set_score_status("LEVEL UP", scoring::rise_bonus);
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
        _max_blocks = turns_for_level(_mode, _level);
        _turns_until_rise = _max_blocks;
        _blocks_remaining = _turns_until_rise;
        _game_over = false;
        _has_pending_rise_row = false;
        _phase = resolution_phase::idle;
        _phase_timer = 0;
        _overlay = overlay_mode::none;
        _menu_selection = 0;
        _cascade_cleared_cells = 0;
        _cascade_chain_count = 0;
        _blank_effect_timer = 0;
        _cracked_blank_count = 0;
        _revealed_blank_count = 0;
        _landing_timer = 0;
        _last_drop_row = -1;
        _last_drop_column = -1;
        _all_clear_timer = 0;
        _rise_impact_timer = 0;
        _level_up_timer = 0;
        _status_timer = 0;
        _cursor_repeat_frames = 0;
        _cursor_repeat_direction = 0;
        _recent_piece_count = 0;
        _recent_piece_values.fill(0);
        _recent_piece_kinds.fill(cell_kind::empty);
        _pending_clear_mask = clear_mask{};
        _cracked_effect_mask.fill(false);
        _revealed_effect_mask.fill(false);
        _seed_opening_board();
        _next_piece = _generate_piece();
        _set_status("READY");
        _store_save();
    }

    void game::_reset_new_game()
    {
        _reset_empty();
    }

    void game::_seed_opening_board()
    {
        constexpr int seeded_disc_count = 9;
        const std::array<int, 4> starting_recent_values = _recent_piece_values;
        const std::array<cell_kind, 4> starting_recent_kinds = _recent_piece_kinds;
        const int starting_recent_count = _recent_piece_count;

        // Retry a few times to get a random opening that has spread and no
        // immediate auto-clear before the player acts.
        for(int attempt = 0; attempt < 64; ++attempt)
        {
            _board.clear();
            _recent_piece_values = starting_recent_values;
            _recent_piece_kinds = starting_recent_kinds;
            _recent_piece_count = starting_recent_count;

            const int blank_budget = _mode == game_mode::fast ? 0 : (_level <= 2 ? 1 : 1 + _random.get_int(2));
            int blanks_used = 0;
            bool placement_failed = false;
            std::array<int, board_size> column_heights = {};
            std::array<bool, board_size> used_columns = {};
            const int target_columns = 4 + _random.get_int(3);

            for(int seeded_disc = 0; seeded_disc < seeded_disc_count; ++seeded_disc)
            {
                std::array<int, board_size> column_weights = {};
                int total_weight = 0;

                for(int column = 0; column < board_size; ++column)
                {
                    if(column_heights[column] >= 4)
                    {
                        continue;
                    }

                    int weight = 12;

                    if(! used_columns[column])
                    {
                        weight = used_columns[0] || used_columns[1] || used_columns[2] ||
                                 used_columns[3] || used_columns[4] || used_columns[5] || used_columns[6] ?
                                 (int(used_columns[0]) + int(used_columns[1]) + int(used_columns[2]) +
                                  int(used_columns[3]) + int(used_columns[4]) + int(used_columns[5]) +
                                  int(used_columns[6]) < target_columns ? 28 : 6) :
                                 28;
                    }
                    else
                    {
                        weight += column_heights[column] * 10;
                    }

                    if(column_heights[column] == 0)
                    {
                        weight += 6;
                    }

                    column_weights[column] = weight;
                    total_weight += weight;
                }

                if(total_weight <= 0)
                {
                    break;
                }

                int roll = _random.get_int(total_weight);
                int column = 0;

                for(; column < board_size; ++column)
                {
                    roll -= column_weights[column];

                    if(roll < 0)
                    {
                        break;
                    }
                }

                if(column >= board_size)
                {
                    column = board_size - 1;
                }

                bool prefer_low_values = seeded_disc < 4;
                cell new_cell = _generate_numbered_piece();

                if(blanks_used < blank_budget && _random.get_int(100) < (_level <= 2 ? 18 : 24))
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

                used_columns[column] = true;
                ++column_heights[column];
            }

            if(placement_failed || rules::find_matches(_board).count)
            {
                continue;
            }

            int occupied_columns = 0;
            int max_column_height = 0;
            int distinct_values = 0;
            int promising_cells = 0;
            std::array<bool, max_disc_value + 1> seen_values = {};

            for(int column = 0; column < board_size; ++column)
            {
                bool column_occupied = false;
                int column_height = 0;

                for(int row = 0; row < board_size; ++row)
                {
                    const cell& board_cell = _board.at(row, column);

                    if(board_cell.occupied())
                    {
                        column_occupied = true;
                        ++column_height;

                        if(board_cell.numbered())
                        {
                            seen_values[board_cell.value] = true;

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

                            if(board_cell.value == horizontal_span - 1 || board_cell.value == horizontal_span + 1 ||
                               board_cell.value == vertical_span - 1 || board_cell.value == vertical_span + 1)
                            {
                                ++promising_cells;
                            }
                        }
                    }
                }

                occupied_columns += column_occupied;
                max_column_height = bn::max(max_column_height, column_height);
            }

            for(int value = min_disc_value; value <= max_disc_value; ++value)
            {
                distinct_values += seen_values[value];
            }

            const int bottom_row_count = [&]() {
                int count = 0;

                for(int column = 0; column < board_size; ++column)
                {
                    count += _board.at(board_size - 1, column).occupied();
                }

                return count;
            }();

            if(occupied_columns >= 4 && occupied_columns <= 6 &&
               max_column_height >= 2 && max_column_height <= 4 &&
               bottom_row_count <= 6 &&
               distinct_values >= 4 && promising_cells >= 2)
            {
                return;
            }
        }

        _board.clear();
        _recent_piece_values = starting_recent_values;
        _recent_piece_kinds = starting_recent_kinds;
        _recent_piece_count = starting_recent_count;
        std::array<int, board_size> column_heights = {};

        for(int seeded_disc = 0; seeded_disc < seeded_disc_count; ++seeded_disc)
        {
            int column = _random.get_int(board_size);

            for(int retry = 0; retry < board_size && column_heights[column] >= 4; ++retry)
            {
                column = (column + 1) % board_size;
            }

            int ignored_row = 0;
            _board.drop(column, _generate_numbered_piece(), ignored_row);
            ++column_heights[column];
        }
    }

    void game::_generate_rise_row()
    {
        // Rise rows stay random, but constrained so they don't degenerate into
        // ugly floods of the same value.
        std::array<int, board_size> row_values = {};

        for(int attempt = 0; attempt < 16; ++attempt)
        {
            std::array<bool, max_disc_value + 1> used_values = {};
            std::array<int, max_disc_value + 1> value_counts = {};

            int distinct_values = 4 + _random.get_int(3);

            if(_level <= 3)
            {
                distinct_values = 5 + _random.get_int(2);
            }

            distinct_values = bn::min(distinct_values, board_size);

            std::array<int, board_size> chosen_values = {};

            for(int index = 0; index < distinct_values; ++index)
            {
                const bool prefer_high_values = _level >= 6;
                const bool prefer_low_values = _level <= 3;
                chosen_values[index] = _generate_value(prefer_high_values, prefer_low_values, true, used_values);
                used_values[chosen_values[index]] = true;
                row_values[index] = chosen_values[index];
                value_counts[chosen_values[index]] = 1;
            }

            int write_index = distinct_values;
            const int duplicate_cap = _level >= 18 ? 3 : 2;

            while(write_index < board_size)
            {
                std::array<int, board_size> weights = {};
                int total_weight = 0;

                for(int index = 0; index < distinct_values; ++index)
                {
                    const int value = chosen_values[index];

                    if(value_counts[value] >= duplicate_cap)
                    {
                        continue;
                    }

                    int weight = value_counts[value] == 1 ? 18 : 7;
                    weights[index] = weight;
                    total_weight += weight;
                }

                if(total_weight <= 0)
                {
                    break;
                }

                int roll = _random.get_int(total_weight);
                int chosen_index = 0;

                for(; chosen_index < distinct_values; ++chosen_index)
                {
                    roll -= weights[chosen_index];

                    if(roll < 0)
                    {
                        break;
                    }
                }

                const int value = chosen_values[bn::min(chosen_index, distinct_values - 1)];
                row_values[write_index] = value;
                ++value_counts[value];
                ++write_index;
            }

            if(write_index == board_size)
            {
                break;
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
        bn::sound_items::cascade7_clear.play_with_priority(clear_sound_priority, volume);
    }

    void game::_play_crack_sound(int cracked_blank_cells) const
    {
        const bn::fixed volume = cracked_blank_cells >= 2 ? bn::fixed(0.78) : bn::fixed(0.66);
        bn::sound_items::cascade7_crack.play_with_priority(crack_sound_priority, volume);
    }

    void game::_play_reveal_sound(int revealed_numbered_cells) const
    {
        const bn::fixed volume = revealed_numbered_cells >= 2 ? bn::fixed(0.95) : bn::fixed(0.82);
        bn::sound_items::cascade7_reveal.play_with_priority(reveal_sound_priority, volume);
    }

    void game::_play_chain_sound() const
    {
        bn::sound_items::cascade7_chain.play_with_priority(chain_sound_priority, 0.85);
    }

    void game::_play_rise_sound() const
    {
        bn::sound_items::cascade7_rise.play_with_priority(rise_sound_priority, 0.8);
    }

    void game::_play_game_over_sound() const
    {
        bn::sound_items::cascade7_game_over.play_with_priority(game_over_sound_priority, 0.9);
    }
}
