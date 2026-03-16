#ifndef BN_SPRITE_ITEMS_CASCADE7_LABELS_H
#define BN_SPRITE_ITEMS_CASCADE7_LABELS_H

#include "bn_sprite_item.h"

//{{BLOCK(cascade7_labels_bn_gfx)

//======================================================================
//
//	cascade7_labels_bn_gfx, 32x32@4, 
//	+ palette 16 entries, not compressed
//	+ 16 tiles Metatiled by 4x1 not compressed
//	Total size: 32 + 512 = 544
//
//	Time-stamp: 2026-03-15, 15:46:56
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_CASCADE7_LABELS_BN_GFX_H
#define GRIT_CASCADE7_LABELS_BN_GFX_H

#define cascade7_labels_bn_gfxTilesLen 512
extern const bn::tile cascade7_labels_bn_gfxTiles[16];

#define cascade7_labels_bn_gfxPalLen 32
extern const bn::color cascade7_labels_bn_gfxPal[16];

#endif // GRIT_CASCADE7_LABELS_BN_GFX_H

//}}BLOCK(cascade7_labels_bn_gfx)

namespace bn::sprite_items
{
    constexpr inline sprite_item cascade7_labels(sprite_shape_size(sprite_shape::WIDE, sprite_size::NORMAL), 
            sprite_tiles_item(span<const tile>(cascade7_labels_bn_gfxTiles, 16), bpp_mode::BPP_4, compression_type::NONE, 4), 
            sprite_palette_item(span<const color>(cascade7_labels_bn_gfxPal, 16), bpp_mode::BPP_4, compression_type::NONE));
}

#endif

