;; 64kbcpm CCP (ZCPR-style) — $C000-$C7FF
;CCP entry (JP $C082)
ccp:

; BLOCK 'ccp' (start 0xc000 end 0xc800)
ccp_start:
	jp 0c082h		;c000
	jp 0c082h		;c003
lc006h:
	ld b,e			;c006
	ld c,a			;c007
	ld c,l			;c008
lc009h:
	nop			;c009
	inc h			;c00a
	inc h			;c00b
	inc h			;c00c
	jr nz,lc02fh		;c00d
	jr nz,lc031h		;c00f
	jr nz,lc066h		;c011
	ld d,l			;c013
	ld b,d			;c014
	nop			;c015
	nop			;c016
lc017h:
	nop			;c017
lc018h:
	nop			;c018
	nop			;c019
	nop			;c01a
	nop			;c01b
	nop			;c01c
	nop			;c01d
	nop			;c01e
	nop			;c01f
	nop			;c020
	nop			;c021
	nop			;c022
	nop			;c023
	nop			;c024
	nop			;c025
	nop			;c026
	nop			;c027
	nop			;c028
lc029h:
	nop			;c029
lc02ah:
	nop			;c02a
lc02bh:
	nop			;c02b
	nop			;c02c
	nop			;c02d
	nop			;c02e
lc02fh:
	nop			;c02f
	nop			;c030
lc031h:
	nop			;c031
	nop			;c032
lc033h:
	nop			;c033
	nop			;c034
lc035h:
	nop			;c035
	nop			;c036
	nop			;c037
	nop			;c038
	nop			;c039
lc03ah:
	nop			;c03a
	nop			;c03b
	nop			;c03c
	nop			;c03d
	nop			;c03e
	nop			;c03f
	nop			;c040
	nop			;c041
	nop			;c042
	nop			;c043
	nop			;c044
	nop			;c045
	nop			;c046
	nop			;c047
	nop			;c048
	nop			;c049
lc04ah:
	nop			;c04a
lc04bh:
	ld d,044h		;c04b
	ld c,c			;c04d
	ld d,d			;c04e
	jr nz,$-116		;c04f
	call nz,0494ch		;c051
	ld d,e			;c054
	ld d,h			;c055
	adc a,l			;c056
	push bc			;c057
	ld d,h			;c058
	ld e,c			;c059
	ld d,b			;c05a
	ld b,l			;c05b
	sub c			;c05c
	push bc			;c05d
	ld b,a			;c05e
	ld c,a			;c05f
	jr nz,$+34		;c060
	adc a,0c6h		;c062
	ld b,l			;c064
	ld d,d			;c065
lc066h:
	ld b,c			;c066
	jr nz,lc089h		;c067
	push bc			;c069
	ld d,e			;c06a
	ld b,c			;c06b
	ld d,(hl)		;c06c
	ld b,l			;c06d
	inc bc			;c06e
	add a,052h		;c06f
	ld b,l			;c071
	ld c,(hl)		;c072
	jr nz,$-120		;c073
	add a,047h		;c075
	ld b,l			;c077
	ld d,h			;c078
	jr nz,lc0f6h		;c079
	rst 0			;c07b
	ld c,d			;c07c
	ld d,l			;c07d
	ld c,l			;c07e
	ld d,b			;c07f
	ret			;c080
	add a,031h		;c081
	add a,d			;c083
	sub 0c5h		;c084
	ld a,c			;c086
	rra			;c087
	rra			;c088
lc089h:
	rra			;c089
	rra			;c08a
	and 00fh		;c08b
	ld (0c39dh),a		;c08d
	call sub_c21eh		;c090
	call sub_c1e6h		;c093
	ld a,0ffh		;c096
	ld (0c0afh),a		;c098
	pop bc			;c09b
	ld a,c			;c09c
	and 00fh		;c09d
	ld (sub_c397h+1),a	;c09f
	call nz,sub_c1ebh	;c0a2
	call sub_c223h		;c0a5
	call sub_c1dfh		;c0a8
	ld de,lc009h		;c0ab
	ld a,000h		;c0ae
	or a			;c0b0
	call nz,sub_c205h	;c0b1
	ld (0c0afh),a		;c0b4
lc0b7h:
	ld hl,(ccp_sp)		;c0b7
	ld a,(hl)		;c0ba
	cp 003h			;c0bb
	jr z,lc0c2h		;c0bd
	or a			;c0bf
	jr nz,lc0fdh		;c0c0
lc0c2h:
	ld sp,ccp_sp		;c0c2
	call sub_c16eh		;c0c5
	ld a,(sub_c397h+1)	;c0c8
	add a,041h		;c0cb
	call sub_c17fh		;c0cd
	ld a,(0c39dh)		;c0d0
	or a			;c0d3
	jr z,lc0e8h		;c0d4
	cp 00ah			;c0d6
	jr c,lc0e3h		;c0d8
	sub 00ah		;c0da
	push af			;c0dc
	ld a,031h		;c0dd
	call sub_c17fh		;c0df
	pop af			;c0e2
lc0e3h:
	add a,030h		;c0e3
	call sub_c17fh		;c0e5
lc0e8h:
	ld hl,ccp_cmdtext	;c0e8
	ld (ccp_sp),hl		;c0eb
	ld (hl),000h		;c0ee
	push hl			;c0f0
	call sub_c23fh		;c0f1
	pop hl			;c0f4
	ld a,(hl)		;c0f5
lc0f6h:
	cp 03bh			;c0f6
	jr z,lc0c2h		;c0f8
	or a			;c0fa
	jr z,lc0c2h		;c0fb
lc0fdh:
	ld sp,ccp_sp		;c0fd
	ld a,(hl)		;c100
	cp 03bh			;c101
	jr nz,lc109h		;c103
	inc hl			;c105
	ld (ccp_sp),hl		;c106
lc109h:
	ld (0c73ch),hl		;c109
lc10ch:
	ld a,(hl)		;c10c
	call sub_c234h		;c10d
	ld (hl),a		;c110
	inc hl			;c111
	or a			;c112
	jr nz,lc10ch		;c113
	call sub_c3aah		;c115
	ld de,(ccp_sp)		;c118
	ld (0c73ch),de		;c11c
	jr nz,lc145h		;c120
	ld de,lc137h		;c122
	push de			;c125
	ld a,000h		;c126
	or a			;c128
	jp nz,lc6d3h		;c129
	call sub_c464h		;c12c
	jp nz,lc6d3h		;c12f
	ld a,(hl)		;c132
	inc hl			;c133
	ld h,(hl)		;c134
	ld l,a			;c135
	jp (hl)			;c136
lc137h:
	call sub_c397h		;c137
lc13ah:
	call sub_c3aah		;c13a
	ld a,(lc02bh)		;c13d
	cp 020h			;c140
	jp z,lc0b7h		;c142
lc145h:
	call sub_c16eh		;c145
	ld hl,00000h		;c148
lc14bh:
	ld a,(hl)		;c14b
	cp 021h			;c14c
	jr c,lc156h		;c14e
	call sub_c17fh		;c150
	inc hl			;c153
	jr lc14bh		;c154
lc156h:
	call sub_c1c7h		;c156
	cp a			;c159
lc15ah:
	call sub_c397h		;c15a
	call sub_c20dh		;c15d
	jp lc0c2h		;c160
sub_c163h:
	call sub_c1c4h		;c163
	ld c,(hl)		;c166
	ld l,a			;c167
	jr nz,lc1b0h		;c168
	ld l,c			;c16a
	ld l,h			;c16b
	push hl			;c16c
	ret			;c16d
sub_c16eh:
	ld a,00dh		;c16e
	call sub_c17fh		;c170
	ld a,00ah		;c173
	jr sub_c17fh		;c175
sub_c177h:
	ld c,001h		;c177
	call 00005h		;c179
	jp sub_c234h		;c17c
sub_c17fh:
	exx			;c17f
	ld c,002h		;c180
lc182h:
	ld e,a			;c182
	call 00005h		;c183
	exx			;c186
	ret			;c187
sub_c188h:
	push af			;c188
	ld a,000h		;c189
	or a			;c18b
	jr z,lc194h		;c18c
	pop af			;c18e
	exx			;c18f
	ld c,005h		;c190
	jr lc182h		;c192
lc194h:
	pop af			;c194
	push af			;c195
	call sub_c17fh		;c196
	pop af			;c199
	cp 00ah			;c19a
	ret nz			;c19c
	push hl			;c19d
	ld hl,lc04bh		;c19e
	dec (hl)		;c1a1
	jr nz,lc1b6h		;c1a2
	ld (hl),016h		;c1a4
	ld a,000h		;c1a6
	cp 050h			;c1a8
	jr z,lc1b6h		;c1aa
	push bc			;c1ac
	call bios_conin		;c1ad
lc1b0h:
	pop bc			;c1b0
	cp 003h			;c1b1
	jp z,lc137h		;c1b3
lc1b6h:
	pop hl			;c1b6
	ret			;c1b7
sub_c1b8h:
	ld de,lc02ah		;c1b8
sub_c1bbh:
	ld c,014h		;c1bb
sub_c1bdh:
	push bc			;c1bd
	call 00005h		;c1be
	pop bc			;c1c1
	or a			;c1c2
	ret			;c1c3
sub_c1c4h:
	call sub_c16eh		;c1c4
sub_c1c7h:
	ex (sp),hl		;c1c7
	call sub_c1cdh		;c1c8
	ex (sp),hl		;c1cb
	ret			;c1cc
sub_c1cdh:
	ld a,(hl)		;c1cd
	inc hl			;c1ce
	or a			;c1cf
	ret z			;c1d0
	push af			;c1d1
	and 07fh		;c1d2
	call sub_c17fh		;c1d4
	pop af			;c1d7
	ret m			;c1d8
	jr sub_c1cdh		;c1d9
	ld c,019h		;c1db
	jr lc1e8h		;c1dd
sub_c1dfh:
	ld de,00080h		;c1df
sub_c1e2h:
	ld c,01ah		;c1e2
	jr lc1e8h		;c1e4
sub_c1e6h:
	ld c,00dh		;c1e6
lc1e8h:
	jp 00005h		;c1e8
sub_c1ebh:
	ld e,a			;c1eb
	ld c,00eh		;c1ec
	jr lc1e8h		;c1ee
sub_c1f0h:
	xor a			;c1f0
	ld (lc04ah),a		;c1f1
	ld de,lc02ah		;c1f4
sub_c1f7h:
	ld c,00fh		;c1f7
lc1f9h:
	call 00005h		;c1f9
	inc a			;c1fc
	ret			;c1fd
sub_c1feh:
	ld c,010h		;c1fe
	jr lc1f9h		;c200
sub_c202h:
	ld de,lc02ah		;c202
sub_c205h:
	ld c,011h		;c205
	jr lc1f9h		;c207
sub_c209h:
	ld c,012h		;c209
	jr lc1f9h		;c20b
sub_c20dh:
	ld hl,0c0afh		;c20d
	ld a,(hl)		;c210
	or a			;c211
	ret z			;c212
	ld (hl),000h		;c213
	ld de,lc009h		;c215
sub_c218h:
	ld c,013h		;c218
	jr lc1e8h		;c21a
sub_c21ch:
	ld a,0ffh		;c21c
sub_c21eh:
	ld e,a			;c21e
	ld c,020h		;c21f
	jr lc1e8h		;c221
sub_c223h:
	call sub_c21ch		;c223
	and 00fh		;c226
	add a,a			;c228
	add a,a			;c229
	add a,a			;c22a
	add a,a			;c22b
	ld hl,sub_c397h+1	;c22c
	or (hl)			;c22f
	ld (00004h),a		;c230
	ret			;c233
sub_c234h:
	and 07fh		;c234
	cp 061h			;c236
	ret c			;c238
	cp 07bh			;c239
	ret nc			;c23b
	and 05fh		;c23c
	ret			;c23e
sub_c23fh:
	ld a,(0c0afh)		;c23f
	or a			;c242
	jr z,lc28ah		;c243
	ld de,lc009h		;c245
	push de			;c248
	call sub_c1f7h		;c249
	pop de			;c24c
	jr z,lc28ah		;c24d
	ld a,(lc018h)		;c24f
	dec a			;c252
	ld (lc029h),a		;c253
	call sub_c1bbh		;c256
	jr nz,lc28ah		;c259
	ld de,ccp_cmdlen	;c25b
	ld hl,00080h		;c25e
	ld bc,00080h		;c261
	ldir			;c264
	ld hl,lc017h		;c266
	ld (hl),000h		;c269
	inc hl			;c26b
	dec (hl)		;c26c
	ld de,lc009h		;c26d
	call sub_c1feh		;c270
	jr z,lc28ah		;c273
	ld a,024h		;c275
	call sub_c17fh		;c277
	ld hl,ccp_cmdtext	;c27a
	call sub_c1cdh		;c27d
	call sub_c2a5h		;c280
	ret nz			;c283
	call sub_c20dh		;c284
	jp lc0c2h		;c287
lc28ah:
	call sub_c20dh		;c28a
	ld a,03eh		;c28d
	call sub_c17fh		;c28f
	ld c,00ah		;c292
	ld de,ccp_cmdline	;c294
	call 00005h		;c297
	ld hl,ccp_cmdlen	;c29a
	ld a,(hl)		;c29d
	inc hl			;c29e
	call sub_c2ebh		;c29f
	ld (hl),000h		;c2a2
	ret			;c2a4
sub_c2a5h:
	exx			;c2a5
	call bios_const		;c2a6
	or a			;c2a9
	call nz,bios_conin	;c2aa
	cp 013h			;c2ad
	call z,bios_conin	;c2af
	exx			;c2b2
	cp 003h			;c2b3
	ret			;c2b5
sub_c2b6h:
	ld a,(de)		;c2b6
	or a			;c2b7
	ret z			;c2b8
	cp 021h			;c2b9
	jr c,lc2d8h		;c2bb
	cp 03dh			;c2bd
	ret z			;c2bf
	cp 05fh			;c2c0
	ret z			;c2c2
lc2c3h:
	cp 02eh			;c2c3
	ret z			;c2c5
	cp 03ah			;c2c6
	ret z			;c2c8
	cp 02ch			;c2c9
	ret z			;c2cb
	cp 03bh			;c2cc
	ret z			;c2ce
	cp 03ch			;c2cf
	ret z			;c2d1
	cp 03eh			;c2d2
	ret z			;c2d4
	cp 03bh			;c2d5
	ret			;c2d7
lc2d8h:
	xor a			;c2d8
	ret			;c2d9
sub_c2dah:
	ld de,(ccp_sp)		;c2da
lc2deh:
	ld a,(de)		;c2de
	or a			;c2df
	ret z			;c2e0
	cp 03bh			;c2e1
	ret z			;c2e3
	call sub_c2b6h		;c2e4
	ret nz			;c2e7
	inc de			;c2e8
	jr lc2deh		;c2e9
sub_c2ebh:
	add a,l			;c2eb
	ld l,a			;c2ec
	ret nc			;c2ed
	inc hl			;c2ee
	ret			;c2ef
sub_c2f0h:
	call sub_c3aah		;c2f0
	ld hl,lc035h		;c2f3
	ld b,00bh		;c2f6
lc2f8h:
	ld a,(hl)		;c2f8
	dec hl			;c2f9
	cp 020h			;c2fa
	jr nz,lc302h		;c2fc
	djnz lc2f8h		;c2fe
	jr lc306h		;c300
lc302h:
	cp 048h			;c302
	jr z,lc33bh		;c304
lc306h:
	ld hl,lc02bh		;c306
sub_c309h:
	ld bc,01100h		;c309
lc30ch:
	ld a,(hl)		;c30c
	cp 020h			;c30d
	jr z,lc333h		;c30f
	cp 03ah			;c311
	jr z,lc333h		;c313
	inc hl			;c315
	sub 030h		;c316
	cp 00ah			;c318
	jr nc,lc335h		;c31a
	ld d,a			;c31c
	ld a,c			;c31d
	rlca			;c31e
	jr c,lc335h		;c31f
	rlca			;c321
	jr c,lc335h		;c322
	rlca			;c324
	jr c,lc335h		;c325
	add a,c			;c327
	jr c,lc335h		;c328
	add a,c			;c32a
	jr c,lc335h		;c32b
	add a,d			;c32d
	jr c,lc335h		;c32e
	ld c,a			;c330
	djnz lc30ch		;c331
lc333h:
	ld a,c			;c333
	ret			;c334
lc335h:
	jp lc145h		;c335
sub_c338h:
	call sub_c3aah		;c338
lc33bh:
	ld hl,lc02bh		;c33b
	ld de,00000h		;c33e
	ld b,00bh		;c341
lc343h:
	ld a,(hl)		;c343
	cp 020h			;c344
	jr z,lc375h		;c346
	cp 048h			;c348
	jr z,lc375h		;c34a
	sub 030h		;c34c
	jr c,lc335h		;c34e
	cp 00ah			;c350
	jr c,lc35ah		;c352
	sub 007h		;c354
	cp 010h			;c356
	jr nc,lc335h		;c358
lc35ah:
	inc hl			;c35a
	ld c,a			;c35b
	ld a,d			;c35c
	rlca			;c35d
	rlca			;c35e
	rlca			;c35f
	rlca			;c360
	and 0f0h		;c361
	ld d,a			;c363
	ld a,e			;c364
	rlca			;c365
	rlca			;c366
	rlca			;c367
	rlca			;c368
	ld e,a			;c369
	and 00fh		;c36a
	or d			;c36c
	ld d,a			;c36d
	ld a,e			;c36e
	and 0f0h		;c36f
	or c			;c371
	ld e,a			;c372
	djnz lc343h		;c373
lc375h:
	ex de,hl		;c375
	ld a,l			;c376
	ret			;c377
sub_c378h:
	ld hl,00080h		;c378
	add a,c			;c37b
	call sub_c2ebh		;c37c
	ld a,(hl)		;c37f
	ret			;c380
sub_c381h:
	xor a			;c381
	ld (lc02ah),a		;c382
	ld a,000h		;c385
	or a			;c387
	jr nz,lc38eh		;c388
	ld a,(sub_c397h+1)	;c38a
	inc a			;c38d
lc38eh:
	dec a			;c38e
	call sub_c1ebh		;c38f
	ld a,000h		;c392
	jp sub_c21eh		;c394
sub_c397h:
	ld a,000h		;c397
	call sub_c1ebh		;c399
	ld a,000h		;c39c
	jp sub_c21eh		;c39e
sub_c3a1h:
	call sub_c3aah		;c3a1
	push af			;c3a4
	call sub_c381h		;c3a5
	pop af			;c3a8
	ret			;c3a9
sub_c3aah:
	ld hl,lc02ah		;c3aa
sub_c3adh:
	xor a			;c3ad
	ld (0c386h),a		;c3ae
	ld (hl),a		;c3b1
	ld (0c127h),a		;c3b2
	ld a,(0c39dh)		;c3b5
	ld (0c393h),a		;c3b8
	call sub_c2dah		;c3bb
	ld (0c149h),de		;c3be
	ld b,00bh		;c3c2
	jr z,lc424h		;c3c4
	push de			;c3c6
	call sub_c2b6h		;c3c7
	cp 041h			;c3ca
	jr c,lc3d2h		;c3cc
	cp 051h			;c3ce
	jr c,lc3dah		;c3d0
lc3d2h:
	cp 030h			;c3d2
	jr c,lc3e0h		;c3d4
	cp 03ah			;c3d6
	jr nc,lc3e0h		;c3d8
lc3dah:
	inc de			;c3da
	call sub_c2b6h		;c3db
	jr lc3d2h		;c3de
lc3e0h:
	pop de			;c3e0
	cp 03ah			;c3e1
	jr nz,lc40eh		;c3e3
	ld (0c127h),a		;c3e5
	ld a,(de)		;c3e8
	cp 041h			;c3e9
	jr c,lc3feh		;c3eb
	sub 040h		;c3ed
	cp 011h			;c3ef
	jp nc,lc145h		;c3f1
	ld (0c386h),a		;c3f4
	ld (hl),a		;c3f7
	inc de			;c3f8
	ld a,(de)		;c3f9
	cp 03ah			;c3fa
	jr z,lc40dh		;c3fc
lc3feh:
	push hl			;c3fe
	ex de,hl		;c3ff
	call sub_c309h		;c400
	ex de,hl		;c403
	pop hl			;c404
	cp 010h			;c405
	jp nc,lc145h		;c407
	ld (0c393h),a		;c40a
lc40dh:
	inc de			;c40d
lc40eh:
	xor a			;c40e
	ld (0c432h),a		;c40f
	ld b,008h		;c412
	call sub_c435h		;c414
	ld b,003h		;c417
	ld a,(de)		;c419
	cp 02eh			;c41a
	jr nz,lc424h		;c41c
	inc de			;c41e
	call sub_c435h		;c41f
	jr lc427h		;c422
lc424h:
	call sub_c456h		;c424
lc427h:
	ld b,004h		;c427
	xor a			;c429
	call sub_c458h		;c42a
	ld (ccp_sp),de		;c42d
	ld a,000h		;c431
	or a			;c433
	ret			;c434
sub_c435h:
	call sub_c2b6h		;c435
	jr z,sub_c456h		;c438
	inc hl			;c43a
	cp 02ah			;c43b
	jr nz,lc446h		;c43d
	ld (hl),03fh		;c43f
	call sub_c45dh		;c441
	jr lc44dh		;c444
lc446h:
	ld (hl),a		;c446
	inc de			;c447
	cp 03fh			;c448
	call z,sub_c45dh	;c44a
lc44dh:
	djnz sub_c435h		;c44d
lc44fh:
	call sub_c2b6h		;c44f
	ret z			;c452
	inc de			;c453
	jr lc44fh		;c454
sub_c456h:
	ld a,020h		;c456
sub_c458h:
	inc hl			;c458
	ld (hl),a		;c459
	djnz sub_c458h		;c45a
	ret			;c45c
sub_c45dh:
	push hl			;c45d
	ld hl,0c432h		;c45e
	inc (hl)		;c461
	pop hl			;c462
	ret			;c463
sub_c464h:
	ld hl,lc04bh+1		;c464
	ld c,009h		;c467
	ld a,c			;c469
	or a			;c46a
	jr z,lc488h		;c46b
lc46dh:
	ld de,lc02bh		;c46d
	ld b,004h		;c470
lc472h:
	ld a,(de)		;c472
	cp (hl)			;c473
	jr nz,lc480h		;c474
	inc de			;c476
	inc hl			;c477
	djnz lc472h		;c478
	ld a,(de)		;c47a
	cp 020h			;c47b
	jr nz,lc483h		;c47d
	ret			;c47f
lc480h:
	inc hl			;c480
	djnz lc480h		;c481
lc483h:
	inc hl			;c483
	inc hl			;c484
	dec c			;c485
	jr nz,lc46dh		;c486
lc488h:
	inc c			;c488
	ret			;c489
	call sub_c3a1h		;c48a
	ld hl,lc02bh		;c48d
	ld a,(hl)		;c490
	cp 020h			;c491
	call z,sub_c518h	;c493
	call sub_c2dah		;c496
	ld b,080h		;c499
	jr z,lc4a9h		;c49b
	ld b,001h		;c49d
	cp 041h			;c49f
	jr z,lc4a8h		;c4a1
	cp 053h			;c4a3
	jr nz,lc4a9h		;c4a5
	dec b			;c4a7
lc4a8h:
	inc de			;c4a8
lc4a9h:
	ld (ccp_sp),de		;c4a9
	ld a,b			;c4ad
	ld (0c516h),a		;c4ae
	ld e,000h		;c4b1
	push de			;c4b3
	call sub_c202h		;c4b4
	jr nz,lc4bfh		;c4b7
	call sub_c163h		;c4b9
	xor a			;c4bc
	pop de			;c4bd
	ret			;c4be
lc4bfh:
	call sub_c507h		;c4bf
	jr z,lc4e2h		;c4c2
	pop de			;c4c4
	ld a,e			;c4c5
	inc e			;c4c6
	push de			;c4c7
	and 003h		;c4c8
	jr nz,lc4d1h		;c4ca
	call sub_c16eh		;c4cc
	jr lc4d9h		;c4cf
lc4d1h:
	call sub_c1c7h		;c4d1
	jr nz,lc4f6h		;c4d4
	ld a,h			;c4d6
	jr nz,$-94		;c4d7
lc4d9h:
	ld b,001h		;c4d9
	ld a,b			;c4db
	call sub_c378h		;c4dc
	call sub_c4f1h		;c4df
lc4e2h:
	call sub_c2a5h		;c4e2
	jr z,lc4ech		;c4e5
	call sub_c209h		;c4e7
	jr nz,lc4bfh		;c4ea
lc4ech:
	pop de			;c4ec
	ld a,0ffh		;c4ed
	or a			;c4ef
	ret			;c4f0
sub_c4f1h:
	ld b,008h		;c4f1
	call sub_c4fdh		;c4f3
lc4f6h:
	ld a,02eh		;c4f6
	call sub_c17fh		;c4f8
	ld b,003h		;c4fb
sub_c4fdh:
	ld a,(hl)		;c4fd
	and 07fh		;c4fe
	inc hl			;c500
	call sub_c17fh		;c501
	djnz sub_c4fdh		;c504
	ret			;c506
sub_c507h:
	dec a			;c507
	rrca			;c508
	rrca			;c509
	rrca			;c50a
	and 060h		;c50b
	ld c,a			;c50d
	ld a,00ah		;c50e
	call sub_c378h		;c510
	and 080h		;c513
	xor 000h		;c515
	ret			;c517
sub_c518h:
	ld b,00bh		;c518
lc51ah:
	ld (hl),03fh		;c51a
	inc hl			;c51c
	djnz lc51ah		;c51d
	ret			;c51f
	call sub_c3a1h		;c520
	call sub_c2dah		;c523
	ld (0c551h),a		;c526
	jr z,lc52ch		;c529
	inc de			;c52b
lc52ch:
	ex de,hl		;c52c
	ld (ccp_sp),hl		;c52d
	ld a,001h		;c530
	ld (0c57dh),a		;c532
	call sub_c202h		;c535
	jr nz,lc53dh		;c538
	jp sub_c163h		;c53a
lc53dh:
	add a,003h		;c53d
	rrca			;c53f
	rrca			;c540
	rrca			;c541
	ld l,a			;c542
	ld h,000h		;c543
	ld (hl),000h		;c545
	push hl			;c547
	inc hl			;c548
	call sub_c16eh		;c549
	call sub_c4f1h		;c54c
	pop de			;c54f
	ld a,000h		;c550
	cp 046h			;c552
	jr z,lc575h		;c554
	push de			;c556
	call sub_c1c7h		;c557
	jr nz,lc5a1h		;c55a
	ld (hl),d		;c55c
	ld h,c			;c55d
	ld (hl),e		;c55e
	ld h,l			;c55f
	jr nz,lc58ah		;c560
	ld a,c			;c562
	cpl			;c563
	ld l,(hl)		;c564
	add hl,hl		;c565
	cp a			;c566
	call sub_c177h		;c567
	pop de			;c56a
	cp 059h			;c56b
	jr z,lc575h		;c56d
	ld hl,0c57dh		;c56f
	inc (hl)		;c572
	jr lc578h		;c573
lc575h:
	call sub_c218h		;c575
lc578h:
	call sub_c202h		;c578
	ret z			;c57b
	ld b,001h		;c57c
lc57eh:
	dec b			;c57e
	jr z,lc53dh		;c57f
	push bc			;c581
	ld de,lc02ah		;c582
	call sub_c209h		;c585
	pop bc			;c588
	ret z			;c589
lc58ah:
	jr lc57eh		;c58a
	ret			;c58c
	ld a,0ffh		;c58d
	jr lc592h		;c58f
	xor a			;c591
lc592h:
	ld (0c18ah),a		;c592
	call sub_c3a1h		;c595
	jp nz,lc145h		;c598
	call sub_c2dah		;c59b
	ld (0c1a7h),a		;c59e
lc5a1h:
	jr z,lc5a4h		;c5a1
	inc de			;c5a3
lc5a4h:
	ld (ccp_sp),de		;c5a4
	call sub_c1f0h		;c5a8
	jp z,lc145h		;c5ab
	call sub_c16eh		;c5ae
	ld a,017h		;c5b1
	ld (lc04bh),a		;c5b3
	ld bc,00080h		;c5b6
lc5b9h:
	ld a,c			;c5b9
	cp 080h			;c5ba
	jr c,lc5cch		;c5bc
	push hl			;c5be
	push bc			;c5bf
	call sub_c1b8h		;c5c0
	pop bc			;c5c3
	pop hl			;c5c4
	jr nz,lc5feh		;c5c5
	ld c,000h		;c5c7
	ld hl,00080h		;c5c9
lc5cch:
	ld a,(hl)		;c5cc
	and 07fh		;c5cd
	cp 01ah			;c5cf
	ret z			;c5d1
	cp 00dh			;c5d2
	jr z,lc5e4h		;c5d4
	cp 00ah			;c5d6
	jr z,lc5e4h		;c5d8
	cp 009h			;c5da
	jr z,lc5ebh		;c5dc
	call sub_c188h		;c5de
	inc b			;c5e1
	jr lc5f6h		;c5e2
lc5e4h:
	call sub_c188h		;c5e4
	ld b,000h		;c5e7
	jr lc5f6h		;c5e9
lc5ebh:
	ld a,020h		;c5eb
	call sub_c188h		;c5ed
	inc b			;c5f0
	ld a,b			;c5f1
	and 007h		;c5f2
	jr nz,lc5ebh		;c5f4
lc5f6h:
	inc c			;c5f6
	inc hl			;c5f7
	call sub_c2a5h		;c5f8
	ret z			;c5fb
	jr lc5b9h		;c5fc
lc5feh:
	dec a			;c5fe
	ret z			;c5ff
	jp lc145h		;c600
	call sub_c2f0h		;c603
	ld l,a			;c606
	ld h,000h		;c607
	push hl			;c609
	call sub_c655h		;c60a
	ld c,016h		;c60d
	call lc1f9h		;c60f
	pop hl			;c612
	jr z,lc64fh		;c613
	xor a			;c615
	ld (lc04ah),a		;c616
	call sub_c2dah		;c619
	inc de			;c61c
	cp 053h			;c61d
	jr z,lc623h		;c61f
	dec de			;c621
	add hl,hl		;c622
lc623h:
	ld (ccp_sp),de		;c623
	ld de,00100h		;c627
lc62ah:
	ld a,h			;c62a
	or l			;c62b
	jr z,lc646h		;c62c
	dec hl			;c62e
	push hl			;c62f
	ld hl,00080h		;c630
	add hl,de		;c633
	push hl			;c634
	call sub_c1e2h		;c635
	ld de,lc02ah		;c638
	ld c,015h		;c63b
	call sub_c1bdh		;c63d
	pop de			;c640
	pop hl			;c641
	jr nz,lc64fh		;c642
	jr lc62ah		;c644
lc646h:
	ld de,lc02ah		;c646
	call sub_c1feh		;c649
	inc a			;c64c
	jr nz,lc652h		;c64d
lc64fh:
	call sub_c7edh		;c64f
lc652h:
	jp sub_c1dfh		;c652
sub_c655h:
	call sub_c3a1h		;c655
	jp nz,lc145h		;c658
	call sub_c202h		;c65b
	ld de,lc02ah		;c65e
	ret z			;c661
	push de			;c662
	call sub_c1c4h		;c663
	ld b,l			;c666
	ld (hl),d		;c667
	ld h,c			;c668
	ld (hl),e		;c669
	ld h,l			;c66a
	and b			;c66b
	ld hl,lc02bh		;c66c
	call sub_c4f1h		;c66f
	ld a,03fh		;c672
	call sub_c17fh		;c674
	call sub_c177h		;c677
	pop de			;c67a
	cp 059h			;c67b
	jp nz,lc15ah		;c67d
	push de			;c680
	call sub_c218h		;c681
	pop de			;c684
	ret			;c685
	call sub_c655h		;c686
	ld a,(0c386h)		;c689
	push af			;c68c
	ld hl,lc02ah		;c68d
	ld de,lc03ah		;c690
	ld bc,00010h		;c693
	ldir			;c696
	call sub_c2dah		;c698
	jr z,lc6c3h		;c69b
	ld (ccp_sp),de		;c69d
	call sub_c3aah		;c6a1
	jr nz,lc6c3h		;c6a4
	pop af			;c6a6
	ld b,a			;c6a7
	ld hl,0c386h		;c6a8
	ld a,(hl)		;c6ab
	or a			;c6ac
	jr z,lc6b2h		;c6ad
	cp b			;c6af
	jr nz,lc6c3h		;c6b0
lc6b2h:
	ld (hl),b		;c6b2
	xor a			;c6b3
	ld (lc02ah),a		;c6b4
	ld de,lc02ah		;c6b7
	ld c,017h		;c6ba
	call lc1f9h		;c6bc
	ret nz			;c6bf
	call sub_c163h		;c6c0
lc6c3h:
	jp lc145h		;c6c3
	jp lc13ah		;c6c6
	call sub_c338h		;c6c9
	jr lc713h		;c6cc
	ld hl,00100h		;c6ce
	jr lc713h		;c6d1
lc6d3h:
	ld a,(lc02bh)		;c6d3
	cp 020h			;c6d6
	jr nz,lc6fah		;c6d8
	ld a,(0c127h)		;c6da
	or a			;c6dd
	ret z			;c6de
	ld a,(0c393h)		;c6df
	cp 010h			;c6e2
	jp nc,lc145h		;c6e4
	ld (0c39dh),a		;c6e7
	call sub_c381h		;c6ea
	ld a,(0c386h)		;c6ed
	or a			;c6f0
	jr z,lc6f7h		;c6f1
	dec a			;c6f3
	ld (sub_c397h+1),a	;c6f4
lc6f7h:
	jp sub_c223h		;c6f7
lc6fah:
	ld de,lc033h		;c6fa
	ld a,(de)		;c6fd
	cp 020h			;c6fe
	jp nz,lc145h		;c700
	ld hl,lc006h		;c703
	ld bc,00003h		;c706
	ldir			;c709
	ld hl,00100h		;c70b
	push hl			;c70e
	call sub_c786h		;c70f
	pop hl			;c712
lc713h:
	ld (0c770h),hl		;c713
	call sub_c3aah		;c716
	ld hl,0c386h		;c719
	push hl			;c71c
	ld a,(hl)		;c71d
	ld (lc02ah),a		;c71e
	ld hl,lc03ah		;c721
	call sub_c3adh		;c724
	pop hl			;c727
	ld a,(hl)		;c728
	ld (lc03ah),a		;c729
	xor a			;c72c
	ld (lc04ah),a		;c72d
	ld de,0005ch		;c730
	ld hl,lc02ah		;c733
	ld bc,00021h		;c736
	ldir			;c739
	ld hl,ccp_cmdtext	;c73b
lc73eh:
	ld a,(hl)		;c73e
	or a			;c73f
	jr z,lc74dh		;c740
	cp 03bh			;c742
	jr z,lc74dh		;c744
	cp 020h			;c746
	jr z,lc74dh		;c748
	inc hl			;c74a
	jr lc73eh		;c74b
lc74dh:
	ld b,000h		;c74d
	ld de,00081h		;c74f
lc752h:
	ld a,(hl)		;c752
	ld (de),a		;c753
	or a			;c754
	jr z,lc762h		;c755
	cp 03bh			;c757
	jr z,lc760h		;c759
	inc b			;c75b
	inc hl			;c75c
	inc de			;c75d
	jr lc752h		;c75e
lc760h:
	xor a			;c760
	ld (de),a		;c761
lc762h:
	ld (ccp_sp),hl		;c762
	ld a,b			;c765
	ld (00080h),a		;c766
	call sub_c16eh		;c769
	call sub_c1dfh		;c76c
	call 00100h		;c76f
	call sub_c1dfh		;c772
	call sub_c397h		;c775
	jp lc0b7h		;c778
	call sub_c338h		;c77b
	push hl			;c77e
	call sub_c3aah		;c77f
	pop hl			;c782
	jp nz,lc145h		;c783
sub_c786h:
	ld (0c7ceh),hl		;c786
	call sub_c381h		;c789
	call sub_c202h		;c78c
	ld hl,00040h		;c78f
	jr nz,lc7cah		;c792
lc794h:
	ld a,(hl)		;c794
	or a			;c795
	jp z,lc145h		;c796
	cp 024h			;c799
	jr nz,lc7a1h		;c79b
	ld a,(sub_c397h+1)	;c79d
	inc a			;c7a0
lc7a1h:
	ld (0c386h),a		;c7a1
	ld a,001h		;c7a4
	ld (0c516h),a		;c7a6
	inc hl			;c7a9
	ld a,(hl)		;c7aa
	inc hl			;c7ab
	push hl			;c7ac
	and 07fh		;c7ad
	cp 024h			;c7af
	jr nz,lc7b6h		;c7b1
	ld a,(0c39dh)		;c7b3
lc7b6h:
	ld (0c393h),a		;c7b6
	cpl			;c7b9
	and 080h		;c7ba
	jr nz,lc7c1h		;c7bc
	ld (0c516h),a		;c7be
lc7c1h:
	call sub_c381h		;c7c1
	call sub_c202h		;c7c4
	pop hl			;c7c7
	jr z,lc794h		;c7c8
lc7cah:
	call sub_c1f0h		;c7ca
	ld hl,00100h		;c7cd
lc7d0h:
	ld a,0bfh		;c7d0
	cp h			;c7d2
	jr c,sub_c7edh		;c7d3
	push hl			;c7d5
	ex de,hl		;c7d6
	call sub_c1e2h		;c7d7
	ld de,lc02ah		;c7da
	call sub_c1bbh		;c7dd
	pop hl			;c7e0
	jr nz,lc7e9h		;c7e1
	ld de,00080h		;c7e3
	add hl,de		;c7e6
	jr lc7d0h		;c7e7
lc7e9h:
	dec a			;c7e9
	jp z,sub_c397h		;c7ea
sub_c7edh:
	call sub_c1c4h		;c7ed
	ld b,(hl)		;c7f0
	ld (hl),l		;c7f1
	ld l,h			;c7f2
	call pe,097cdh		;c7f3
	jp lc2c3h		;c7f6
	ret nz			;c7f9
	ret nz			;c7fa
	call sub_c338h		;c7fb
	push hl			;c7fe
	defb 0cdh		;c7ff
;BDOS entry
