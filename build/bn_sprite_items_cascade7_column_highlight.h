#ifndef BN_SPRITE_ITEMS_CASCADE7_COLUMN_HIGHLIGHT_H
#define BN_SPRITE_ITEMS_CASCADE7_COLUMN_HIGHLIGHT_H

#include "bn_sprite_item.h"

//{{BLOCK(cascade7_column_highlight_bn_gfx)

//======================================================================
//
//	cascade7_column_highlight_bn_gfx, 16x32@4, 
//	+ palette 16 entries, not compressed
//	+ 8 tiles Metatiled by 2x2 not compressed
//	Total size: 32 + 256 = 288
//
//	Time-stamp: 2026-03-15, 15:46:56
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_CASCADE7_COLUMN_HIGHLIGHT_BN_GFX_H
#define GRIT_CASCADE7_COLUMN_HIGHLIGHT_BN_GFX_H

#define cascade7_column_highlight_bn_gfxTilesLen 256
extern const bn::tile cascade7_column_highlight_bn_gfxTiles[8];

#define cascade7_column_highlight_bn_gfxPalLen 32
extern const bn::color cascade7_column_highlight_bn_gfxPal[16];

#endif // GRIT_CASCADE7_COLUMN_HIGHLIGHT_BN_GFX_H

//}}BLOCK(cascade7_column_highlight_bn_gfx)

namespace bn::sprite_items
{
    constexpr inline sprite_item cascade7_column_highlight(sprite_shape_size(sprite_shape::SQUARE, sprite_size::NORMAL), 
            sprite_tiles_item(span<const tile>(cascade7_column_highlight_bn_gfxTiles, 8), bpp_mode::BPP_4, compression_type::NONE, 2), 
            sprite_palette_item(span<const color>(cascade7_column_highlight_bn_gfxPal, 16), bpp_mode::BPP_4, compression_type::NONE));
}

#endif

