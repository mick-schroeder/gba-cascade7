
@{{BLOCK(cascade7_dot_bn_gfx)

@=======================================================================
@
@	cascade7_dot_bn_gfx, 8x8@4, 
@	+ palette 16 entries, not compressed
@	+ 1 tiles not compressed
@	Total size: 32 + 32 = 64
@
@	Time-stamp: 2026-03-15, 15:46:56
@	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
@	( http://www.coranac.com/projects/#grit )
@
@=======================================================================

	.section .rodata
	.align	2
	.global cascade7_dot_bn_gfxTiles		@ 32 unsigned chars
	.hidden cascade7_dot_bn_gfxTiles
cascade7_dot_bn_gfxTiles:
	.word 0x00000000,0x00022000,0x00222200,0x02222220,0x02222220,0x00222200,0x00022000,0x00000000

	.section .rodata
	.align	2
	.global cascade7_dot_bn_gfxPal		@ 32 unsigned chars
	.hidden cascade7_dot_bn_gfxPal
cascade7_dot_bn_gfxPal:
	.hword 0x7C1F,0x0000,0x73BE,0x6318,0x5252,0x55F1,0x5E54,0x2744
	.hword 0x7FFF,0x212A,0x7A85,0x5524,0x347F,0x2456,0x12BF,0x05B9

@}}BLOCK(cascade7_dot_bn_gfx)
