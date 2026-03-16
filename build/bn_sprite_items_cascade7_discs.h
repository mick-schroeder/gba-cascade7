#ifndef BN_SPRITE_ITEMS_CASCADE7_DISCS_H
#define BN_SPRITE_ITEMS_CASCADE7_DISCS_H

#include "bn_sprite_item.h"

//{{BLOCK(cascade7_discs_bn_gfx)

//======================================================================
//
//	cascade7_discs_bn_gfx, 16x144@8, 
//	+ palette 64 entries, not compressed
//	+ 36 tiles Metatiled by 2x2 not compressed
//	Total size: 128 + 2304 = 2432
//
//	Time-stamp: 2026-03-15, 15:46:56
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_CASCADE7_DISCS_BN_GFX_H
#define GRIT_CASCADE7_DISCS_BN_GFX_H

#define cascade7_discs_bn_gfxTilesLen 2304
extern const bn::tile cascade7_discs_bn_gfxTiles[72];

#define cascade7_discs_bn_gfxPalLen 128
extern const bn::color cascade7_discs_bn_gfxPal[64];

#endif // GRIT_CASCADE7_DISCS_BN_GFX_H

//}}BLOCK(cascade7_discs_bn_gfx)

namespace bn::sprite_items
{
    constexpr inline sprite_item cascade7_discs(sprite_shape_size(sprite_shape::SQUARE, sprite_size::NORMAL), 
            sprite_tiles_item(span<const tile>(cascade7_discs_bn_gfxTiles, 72), bpp_mode::BPP_8, compression_type::NONE, 9), 
            sprite_palette_item(span<const color>(cascade7_discs_bn_gfxPal, 64), bpp_mode::BPP_8, compression_type::NONE));
}

#endif

