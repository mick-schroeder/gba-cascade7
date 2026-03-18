#ifndef CASCADE7_RENDERER_H
#define CASCADE7_RENDERER_H

#include "bn_fixed_point.h"
#include "bn_rect_window.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "bn_window.h"

#include "cascade7/game.h"
#include "cascade7/types.h"

namespace cascade7
{
    class renderer
    {
    public:
        renderer();

        void draw(const game& game);

    private:
        [[nodiscard]] static bn::fixed_point _cell_position(int row, int column);
        [[nodiscard]] bn::fixed_point _board_offset(const game& game) const;
        [[nodiscard]] static int _disc_graphics_index(const cell& cell);
        void _update_cascade_effects(const game& game);
        void _draw_hud_text(const game& game);
        void _draw_stat_line(int y, const char* label, int value);

        bn::regular_bg_ptr _logo_bg;
        bn::sprite_ptr _preview_sprite;
        bn::rect_window _game_over_window;
        bn::window _outside_window;
        bn::vector<bn::sprite_ptr, 4> _grid_sprites;
        bn::vector<bn::sprite_ptr, 7> _column_highlight_sprites;
        bn::vector<bn::sprite_ptr, 49> _disc_sprites;
        bn::vector<bn::sprite_ptr, 7> _rise_sprites;
        bn::vector<bn::sprite_ptr, 8> _explosion_sprites;
        bn::sprite_text_generator _text_generator;
        bn::vector<bn::sprite_ptr, 160> _text_sprites;
        int _animation_frame = 0;
    };
}

#endif
