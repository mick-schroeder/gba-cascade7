#include "cascade7/renderer.h"

#include "bn_blending.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_string.h"

#include "bn_sprite_items_cascade7_column_highlight.h"
#include "bn_sprite_items_cascade7_discs.h"
#include "bn_sprite_items_cascade7_explosion.h"
#include "bn_sprite_items_cascade7_grid.h"

#include "cascade7/scoring.h"

#include "common_variable_8x8_sprite_font.h"

namespace cascade7
{
    namespace
    {
        constexpr int board_left = -4;
        constexpr int board_top = -36;
        constexpr int cell_size = 16;
        constexpr int sidebar_x = -112;
        constexpr int sidebar_value_x = -54;
        constexpr int preview_y = board_top - 18;
        constexpr int rise_frames = 18;
    }

    renderer::renderer() :
        _preview_sprite(bn::sprite_items::cascade7_discs.create_sprite(0, preview_y, 0)),
        _game_over_window(bn::rect_window::internal()),
        _outside_window(bn::window::outside()),
        _text_generator(common::variable_8x8_sprite_font)
    {
        _text_generator.set_left_alignment();
        bn::bg_palettes::set_transparent_color(bn::color(0, 0, 0));
        _game_over_window.set_boundaries(0, 0, 0, 0);
        _game_over_window.set_show_blending(false);
        _outside_window.set_show_blending(false);

        const bn::fixed_point grid_positions[4] = {
            bn::fixed_point(board_left + 32, board_top + 32),
            bn::fixed_point(board_left + 80, board_top + 32),
            bn::fixed_point(board_left + 32, board_top + 80),
            bn::fixed_point(board_left + 80, board_top + 80),
        };

        for(int index = 0; index < 4; ++index)
        {
            _grid_sprites.push_back(bn::sprite_items::cascade7_grid.create_sprite(grid_positions[index], index));
        }

        for(int row = 0; row < board_size; ++row)
        {
            bn::sprite_ptr sprite = bn::sprite_items::cascade7_column_highlight.create_sprite(_cell_position(row, 0), 0);
            sprite.set_z_order(1);
            _column_highlight_sprites.push_back(sprite);
        }

        for(int row = 0; row < board_size; ++row)
        {
            for(int column = 0; column < board_size; ++column)
            {
                bn::sprite_ptr sprite = bn::sprite_items::cascade7_discs.create_sprite(_cell_position(row, column), 0);
                sprite.set_z_order(0);
                sprite.set_visible(false);
                _disc_sprites.push_back(sprite);
            }
        }

        for(int column = 0; column < board_size; ++column)
        {
            bn::sprite_ptr sprite = bn::sprite_items::cascade7_discs.create_sprite(_cell_position(board_size, column), 0);
            sprite.set_z_order(0);
            sprite.set_visible(false);
            _rise_sprites.push_back(sprite);
        }

        for(int index = 0; index < 8; ++index)
        {
            bn::sprite_ptr explosion = bn::sprite_items::cascade7_explosion.create_sprite(0, 0, 0);
            explosion.set_z_order(-2);
            explosion.set_visible(false);
            _explosion_sprites.push_back(explosion);
        }

    }

    void renderer::draw(const game& game)
    {
        ++_animation_frame;

        bn::blending::set_fade_alpha(0);
        _game_over_window.set_boundaries(0, 0, 0, 0);
        _outside_window.set_show_blending(false);

        if(game.game_over())
        {
            bn::blending::set_black_fade_color();
            bn::blending::set_fade_alpha(0.4);
            _game_over_window.set_boundaries(-44, -116, 48, 12);
            _game_over_window.set_show_blending(false);
            _outside_window.set_show_blending(true);
        }
        else if(game.phase() == resolution_phase::flashing)
        {
            bn::blending::set_white_fade_color();
            bn::blending::set_fade_alpha(((_animation_frame / 4) & 1) ? 0.12 : 0.04);
        }
        else if(game.phase() == resolution_phase::clearing)
        {
            bn::blending::set_white_fade_color();
            bn::blending::set_fade_alpha(0.08);
        }

        const int pulse_graphics_index = (_animation_frame / 12) % 2;
        const int preview_bob_offsets[] = { 0, -1, -2, -1, 0, 1, 0, -1 };
        const int preview_bob = preview_bob_offsets[(_animation_frame / 5) % 8];
        const bool show_play_cursor = ! game.resolving() && ! game.game_over();
        const bn::fixed_point board_offset = _board_offset(game);

        _preview_sprite.set_x(board_left + 8 + (game.cursor_column() * cell_size) + board_offset.x());
        _preview_sprite.set_y(preview_y + preview_bob + board_offset.y());
        _preview_sprite.set_item(bn::sprite_items::cascade7_discs, _disc_graphics_index(game.next_piece()));
        _preview_sprite.set_visible(show_play_cursor);

        for(int row = 0; row < board_size; ++row)
        {
            bn::sprite_ptr& highlight_sprite = _column_highlight_sprites[row];
            highlight_sprite.set_x(board_left + 8 + (game.cursor_column() * cell_size) + board_offset.x());
            highlight_sprite.set_y(board_top + 8 + (row * cell_size) + board_offset.y());
            highlight_sprite.set_item(bn::sprite_items::cascade7_column_highlight, pulse_graphics_index);
            highlight_sprite.set_visible(show_play_cursor);
        }

        for(int row = 0; row < board_size; ++row)
        {
            for(int column = 0; column < board_size; ++column)
            {
                const int sprite_index = (row * board_size) + column;
                bn::sprite_ptr& sprite = _disc_sprites[sprite_index];
                const cell& board_cell = game.board_state().at(row, column);

                if(board_cell.occupied())
                {
                    sprite.set_item(bn::sprite_items::cascade7_discs, _disc_graphics_index(board_cell));
                    sprite.set_position(_cell_position(row, column) + board_offset);
                    sprite.set_visible(true);
                }
                else
                {
                    sprite.set_visible(false);
                }
            }
        }

        _update_cascade_effects(game);

        const bool showing_rise = game.phase() == resolution_phase::rising && game.has_pending_rise_row();

        if(showing_rise)
        {
            const int progress = rise_frames - game.phase_timer();
            const int rise_offset = cell_size - ((progress * cell_size) / rise_frames);

            for(int column = 0; column < board_size; ++column)
            {
                bn::sprite_ptr& rise_sprite = _rise_sprites[column];
                const cell& rise_cell = game.pending_rise_row()[column];
                rise_sprite.set_item(bn::sprite_items::cascade7_discs, _disc_graphics_index(rise_cell));
                rise_sprite.set_position(_cell_position(board_size - 1, column).x() + board_offset.x(),
                                         _cell_position(board_size - 1, column).y() + rise_offset + board_offset.y());
                rise_sprite.set_visible(true);
            }
        }
        else
        {
            for(bn::sprite_ptr& rise_sprite : _rise_sprites)
            {
                rise_sprite.set_visible(false);
            }
        }

        _draw_hud_text(game);
    }

    bn::fixed_point renderer::_board_offset(const game& game) const
    {
        if(game.phase() == resolution_phase::clearing)
        {
            return bn::fixed_point(((_animation_frame / 2) & 1) ? 1 : -1, 0);
        }

        if(game.phase() == resolution_phase::flashing)
        {
            return bn::fixed_point(0, ((_animation_frame / 4) & 1) ? -1 : 0);
        }

        if(game.phase() == resolution_phase::rising)
        {
            return bn::fixed_point(0, -1);
        }

        return bn::fixed_point();
    }

    bn::fixed_point renderer::_cell_position(int row, int column)
    {
        return bn::fixed_point(board_left + 8 + (column * cell_size), board_top + 8 + (row * cell_size));
    }

    int renderer::_disc_graphics_index(const cell& cell)
    {
        switch(cell.kind)
        {
        case cell_kind::numbered:
            return cell.value - 1;
        case cell_kind::blank:
            return 7;
        case cell_kind::cracked_blank:
            return 8;
        case cell_kind::empty:
        default:
            return 0;
        }
    }

    void renderer::_update_cascade_effects(const game& game)
    {
        const bn::fixed_point board_offset = _board_offset(game);
        bool highlighted_rows[board_size] = {};
        bool highlighted_columns[board_size] = {};
        const clear_mask& clear = game.pending_clear_mask();
        const int chain_depth = bn::max(game.current_chain_depth(), 1);
        const bool flashing = game.phase() == resolution_phase::flashing;
        const bool clearing = game.phase() == resolution_phase::clearing;
        const int flash_period = chain_depth >= 4 ? 2 : 3;
        const int flash_frame = (_animation_frame / flash_period) % 2;
        const int explosion_frame = (_animation_frame / 3) % 3;
        const int chain_nudge = bn::min(chain_depth - 1, 2);
        int explosion_index = 0;

        for(int row = 0; row < board_size; ++row)
        {
            for(int column = 0; column < board_size; ++column)
            {
                const int index = (row * board_size) + column;

                if(! clear.cells[index])
                {
                    continue;
                }

                highlighted_rows[row] = true;
                highlighted_columns[column] = true;
            }
        }

        for(int row = 0; row < board_size; ++row)
        {
            for(int column = 0; column < board_size; ++column)
            {
                const int index = (row * board_size) + column;
                bn::sprite_ptr& disc_sprite = _disc_sprites[index];
                const cell& board_cell = game.board_state().at(row, column);

                if(! board_cell.occupied())
                {
                    continue;
                }

                if(clear.cells[index])
                {
                    const int pop_offset = clearing ? (((row + column + _animation_frame) & 1) + chain_nudge) : 0;
                    disc_sprite.set_position(_cell_position(row, column).x() + board_offset.x(),
                                             _cell_position(row, column).y() + board_offset.y() - pop_offset);

                    if(flashing)
                    {
                        disc_sprite.set_visible(flash_frame == 0);
                    }

                    if(clearing && explosion_index < _explosion_sprites.size())
                    {
                        bn::sprite_ptr& explosion_sprite = _explosion_sprites[explosion_index];
                        explosion_sprite.set_position(_cell_position(row, column) + board_offset);
                        explosion_sprite.set_item(bn::sprite_items::cascade7_explosion, explosion_frame);
                        explosion_sprite.set_visible(true);
                        disc_sprite.set_visible(flash_frame == 0);
                        ++explosion_index;
                    }
                }
                else if(highlighted_rows[row] || highlighted_columns[column])
                {
                    const int nudge = (((_animation_frame / (chain_depth >= 4 ? 4 : 6)) + row + column) & 1) &&
                                      (flashing || clearing) ? 1 + chain_nudge : 0;
                    disc_sprite.set_position(_cell_position(row, column).x() + board_offset.x(),
                                             _cell_position(row, column).y() + board_offset.y() - nudge);
                    disc_sprite.set_visible(true);
                }
                else
                {
                    disc_sprite.set_position(_cell_position(row, column) + board_offset);
                    disc_sprite.set_visible(true);
                }
            }
        }

        while(explosion_index < _explosion_sprites.size())
        {
            _explosion_sprites[explosion_index].set_visible(false);
            ++explosion_index;
        }
    }

    void renderer::_draw_hud_text(const game& game)
    {
        _text_sprites.clear();

        if(game.game_over())
        {
            const int blink = (_animation_frame / 20) & 1;

            _text_generator.generate(-108, -12, "GAME OVER", _text_sprites);

            bn::string<24> final_score_text;
            final_score_text += "SCORE ";
            final_score_text += bn::to_string<10>(game.score());
            _text_generator.generate(-112, 8, final_score_text, _text_sprites);

            bn::string<24> high_score_text;
            high_score_text += "HIGH ";
            high_score_text += bn::to_string<10>(game.high_score());
            _text_generator.generate(-112, 20, high_score_text, _text_sprites);

            bn::string<24> final_level_text;
            final_level_text += "LEVEL ";
            final_level_text += bn::to_string<4>(game.level());
            _text_generator.generate(-112, 32, final_level_text, _text_sprites);

            bn::string<24> best_chain_text;
            best_chain_text += "BEST ";
            best_chain_text += bn::to_string<4>(game.highest_chain());
            _text_generator.generate(-112, 44, best_chain_text, _text_sprites);

            bn::string<24> reason_text = game.status_text();
            _text_generator.generate(-112, 58, reason_text, _text_sprites);

            if(blink == 0)
            {
                _text_generator.generate(-112, 74, "START RESET", _text_sprites);
                _text_generator.generate(-112, 84, "SELECT NEW", _text_sprites);
            }
        }
        else
        {
            _text_generator.generate(sidebar_x, 8, "BY MICK", _text_sprites);
            _text_generator.generate(sidebar_x, 16, "SCHROEDER", _text_sprites);
            _draw_stat_line(30, "SCORE", game.score());
            _draw_stat_line(42, "HIGH", game.high_score());
            _draw_stat_line(54, "LEVEL", game.level());
            _draw_stat_line(66, "RISE", game.blocks_remaining());

            if(game.score_popup_timer() > 0)
            {
                const int popup_y = board_top - 20 - ((scoring::popup_frames - game.score_popup_timer()) / 4);
                bn::string<24> popup_text;
                popup_text += '+';
                popup_text += bn::to_string<10>(game.score_popup_value());
                _text_generator.generate(board_left + 24, popup_y, popup_text, _text_sprites);

                if(game.score_popup_chain() >= scoring::large_chain_threshold)
                {
                    bn::string<24> chain_popup_text;
                    chain_popup_text += "CASCADE x";
                    chain_popup_text += bn::to_string<4>(game.score_popup_chain());
                    _text_generator.generate(board_left + 14, popup_y - 10, chain_popup_text, _text_sprites);
                }
            }
        }
    }

    void renderer::_draw_stat_line(int y, const char* label, int value)
    {
        if(label[0])
        {
            _text_generator.generate(sidebar_x, y, label, _text_sprites);
        }

        bn::string<16> value_text = bn::to_string<10>(value);
        _text_generator.generate(sidebar_value_x, y, value_text, _text_sprites);
    }
}
