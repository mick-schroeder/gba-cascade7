#ifndef BN_SPRITE_ITEMS_CASCADE7_LOGO_H
#define BN_SPRITE_ITEMS_CASCADE7_LOGO_H

#include "bn_sprite_item.h"

//{{BLOCK(cascade7_logo_bn_gfx)

//======================================================================
//
//	cascade7_logo_bn_gfx, 32x32@4, 
//	+ palette 16 entries, not compressed
//	+ 16 tiles Metatiled by 4x2 not compressed
//	Total size: 32 + 512 = 544
//
//	Time-stamp: 2026-03-15, 17:50:18
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_CASCADE7_LOGO_BN_GFX_H
#define GRIT_CASCADE7_LOGO_BN_GFX_H

#define cascade7_logo_bn_gfxTilesLen 512
extern const bn::tile cascade7_logo_bn_gfxTiles[16];

#define cascade7_logo_bn_gfxPalLen 32
extern const bn::color cascade7_logo_bn_gfxPal[16];

#endif // GRIT_CASCADE7_LOGO_BN_GFX_H

//}}BLOCK(cascade7_logo_bn_gfx)

namespace bn::sprite_items
{
    constexpr inline sprite_item cascade7_logo(sprite_shape_size(sprite_shape::WIDE, sprite_size::BIG), 
            sprite_tiles_item(span<const tile>(cascade7_logo_bn_gfxTiles, 16), bpp_mode::BPP_4, compression_type::NONE, 2), 
            sprite_palette_item(span<const color>(cascade7_logo_bn_gfxPal, 16), bpp_mode::BPP_4, compression_type::NONE));
}

#endif

