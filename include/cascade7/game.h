#ifndef CASCADE7_GAME_H
#define CASCADE7_GAME_H

#include <array>

#include "bn_random.h"
#include "bn_string.h"

#include "cascade7/board.h"
#include "cascade7/rules.h"

namespace cascade7
{
    enum class resolution_phase
    {
        idle,
        settle,
        flashing,
        clearing,
        gravity,
        rising
    };

    enum class overlay_mode
    {
        none,
        pause_menu,
        game_over_menu,
        mode_select,
        help_screen,
        about_screen
    };

    enum class game_mode
    {
        standard,
        fast
    };

    class game
    {
    public:
        game();

        void update();

        [[nodiscard]] const board& board_state() const;
        [[nodiscard]] const cell& next_piece() const;
        [[nodiscard]] int cursor_column() const;
        [[nodiscard]] int score() const;
        [[nodiscard]] int level() const;
        [[nodiscard]] int turn() const;
        [[nodiscard]] int high_score() const;
        [[nodiscard]] game_mode mode() const;
        [[nodiscard]] int current_chain_depth() const;
        [[nodiscard]] int highest_chain() const;
        [[nodiscard]] int discs_cleared() const;
        [[nodiscard]] int last_move_score() const;
        [[nodiscard]] int score_popup_value() const;
        [[nodiscard]] int score_popup_chain() const;
        [[nodiscard]] int score_popup_timer() const;
        [[nodiscard]] int last_clear_count() const;
        [[nodiscard]] int last_chain_count() const;
        [[nodiscard]] int blocks_remaining() const;
        [[nodiscard]] int max_blocks_remaining() const;
        [[nodiscard]] bool game_over() const;
        [[nodiscard]] bool resolving() const;
        [[nodiscard]] resolution_phase phase() const;
        [[nodiscard]] int phase_timer() const;
        [[nodiscard]] overlay_mode overlay() const;
        [[nodiscard]] int menu_selection() const;
        [[nodiscard]] int blank_effect_timer() const;
        [[nodiscard]] int cracked_blank_count() const;
        [[nodiscard]] int revealed_blank_count() const;
        [[nodiscard]] int landing_timer() const;
        [[nodiscard]] int last_drop_row() const;
        [[nodiscard]] int last_drop_column() const;
        [[nodiscard]] int all_clear_timer() const;
        [[nodiscard]] int rise_impact_timer() const;
        [[nodiscard]] int level_up_timer() const;
        [[nodiscard]] int status_timer() const;
        [[nodiscard]] const clear_mask& pending_clear_mask() const;
        [[nodiscard]] const std::array<bool, board_size * board_size>& cracked_effect_mask() const;
        [[nodiscard]] const std::array<bool, board_size * board_size>& revealed_effect_mask() const;
        [[nodiscard]] bool has_pending_rise_row() const;
        [[nodiscard]] const std::array<cell, board_size>& pending_rise_row() const;
        [[nodiscard]] const bn::string<48>& status_text() const;

    private:
        void _handle_overlay_input();
        void _open_pause_menu();
        void _open_game_over_menu();
        void _open_mode_select(overlay_mode return_overlay);
        void _close_overlay();
        void _confirm_overlay_selection();
        void _update_resolution();
        [[nodiscard]] cell _generate_piece();
        [[nodiscard]] cell _generate_numbered_piece();
        [[nodiscard]] int _generate_value(bool prefer_high_values = false,
                                          bool prefer_low_values = false,
                                          bool unique_only = false,
                                          const std::array<bool, max_disc_value + 1>& used_values = {});
        [[nodiscard]] int _blank_spawn_chance() const;
        void _move_cursor(int delta);
        void _drop_piece();
        void _finish_resolution_step();
        void _raise_blank_row();
        void _set_status(const char* text);
        void _set_score_status(const char* prefix, int score);
        void _load_save();
        void _store_high_score_if_needed();
        void _reset_empty();
        void _reset_new_game();
        void _seed_opening_board();
        void _generate_rise_row();
        void _remember_generated_piece(const cell& generated_piece);
        void _play_crack_sound(int cracked_blank_cells) const;
        void _play_reveal_sound(int revealed_numbered_cells) const;
        void _play_clear_sound(int cleared_numbered_cells) const;
        void _play_chain_sound() const;
        void _play_rise_sound() const;
        void _play_game_over_sound() const;

        board _board;
        bn::random _random;
        cell _next_piece;
        clear_mask _pending_clear_mask;
        int _cursor_column = board_size / 2;
        int _score = 0;
        std::array<int, 2> _high_scores{};
        int _level = 1;
        int _turn = 0;
        int _highest_chain = 0;
        int _discs_cleared = 0;
        int _last_move_score = 0;
        int _score_popup_value = 0;
        int _score_popup_chain = 0;
        int _score_popup_timer = 0;
        int _last_clear_count = 0;
        int _last_chain_count = 0;
        int _blocks_remaining = 5;
        int _max_blocks = 5;
        int _phase_timer = 0;
        int _cascade_cleared_cells = 0;
        int _cascade_chain_count = 0;
        int _turns_until_rise = 5;
        int _cursor_repeat_frames = 0;
        int _cursor_repeat_direction = 0;
        int _menu_selection = 0;
        int _blank_effect_timer = 0;
        int _cracked_blank_count = 0;
        int _revealed_blank_count = 0;
        int _landing_timer = 0;
        int _last_drop_row = -1;
        int _last_drop_column = -1;
        int _all_clear_timer = 0;
        int _rise_impact_timer = 0;
        int _level_up_timer = 0;
        int _status_timer = 0;
        bool _game_over = false;
        bool _has_pending_rise_row = false;
        resolution_phase _phase = resolution_phase::idle;
        overlay_mode _overlay = overlay_mode::none;
        overlay_mode _mode_select_return_overlay = overlay_mode::pause_menu;
        game_mode _mode = game_mode::standard;
        std::array<cell, board_size> _pending_rise_row{};
        std::array<bool, board_size * board_size> _cracked_effect_mask{};
        std::array<bool, board_size * board_size> _revealed_effect_mask{};
        std::array<int, 4> _recent_piece_values{};
        std::array<cell_kind, 4> _recent_piece_kinds{};
        int _recent_piece_count = 0;
        bn::string<48> _status;
    };
}

#endif
