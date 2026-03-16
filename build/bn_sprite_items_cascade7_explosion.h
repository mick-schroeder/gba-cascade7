#ifndef BN_SPRITE_ITEMS_CASCADE7_EXPLOSION_H
#define BN_SPRITE_ITEMS_CASCADE7_EXPLOSION_H

#include "bn_sprite_item.h"

//{{BLOCK(cascade7_explosion_bn_gfx)

//======================================================================
//
//	cascade7_explosion_bn_gfx, 16x48@4, 
//	+ palette 16 entries, not compressed
//	+ 12 tiles Metatiled by 2x2 not compressed
//	Total size: 32 + 384 = 416
//
//	Time-stamp: 2026-03-15, 15:46:56
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_CASCADE7_EXPLOSION_BN_GFX_H
#define GRIT_CASCADE7_EXPLOSION_BN_GFX_H

#define cascade7_explosion_bn_gfxTilesLen 384
extern const bn::tile cascade7_explosion_bn_gfxTiles[12];

#define cascade7_explosion_bn_gfxPalLen 32
extern const bn::color cascade7_explosion_bn_gfxPal[16];

#endif // GRIT_CASCADE7_EXPLOSION_BN_GFX_H

//}}BLOCK(cascade7_explosion_bn_gfx)

namespace bn::sprite_items
{
    constexpr inline sprite_item cascade7_explosion(sprite_shape_size(sprite_shape::SQUARE, sprite_size::NORMAL), 
            sprite_tiles_item(span<const tile>(cascade7_explosion_bn_gfxTiles, 12), bpp_mode::BPP_4, compression_type::NONE, 3), 
            sprite_palette_item(span<const color>(cascade7_explosion_bn_gfxPal, 16), bpp_mode::BPP_4, compression_type::NONE));
}

#endif

