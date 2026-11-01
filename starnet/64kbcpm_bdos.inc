;; 64kbcpm network BDOS — $C800-$D5FF
bdos:

; BLOCK 'bdos' (start 0xc800 end 0xd600)
bdos_start:
	ret m			;c800
	ld d,001h		;c801
	nop			;c803
	daa			;c804
	nop			;c805
lc806h:
	jp lc811h		;c806
lc809h:
	sbc a,c			;c809
	ret z			;c80a
lc80bh:
	and l			;c80b
	ret z			;c80c
lc80dh:
	xor e			;c80d
	ret z			;c80e
lc80fh:
	or c			;c80f
	ret z			;c810
lc811h:
	ex de,hl		;c811
	ld (lcb43h),hl		;c812
	ex de,hl		;c815
	ld a,e			;c816
	ld (ld5dah),a		;c817
	ld hl,00000h		;c81a
	ld (lcb45h),hl		;c81d
	add hl,sp		;c820
	ld (lcb0fh),hl		;c821
	ld sp,lcb41h		;c824
	xor a			;c827
	ld (ld5e4h),a		;c828
	ld (ld5e2h),a		;c82b
	ld hl,ld578h		;c82e
	push hl			;c831
	ld a,c			;c832
	cp 029h			;c833
	ret nc			;c835
	ld c,e			;c836
	ld hl,lc847h		;c837
	ld e,a			;c83a
	ld d,000h		;c83b
	add hl,de		;c83d
	add hl,de		;c83e
	ld e,(hl)		;c83f
	inc hl			;c840
	ld d,(hl)		;c841
	ld hl,(lcb43h)		;c842
	ex de,hl		;c845
	jp (hl)			;c846
lc847h:
	inc bc			;c847
	sub 0c8h		;c848
	jp z,lc990h		;c84a
	adc a,0cah		;c84d
	ld (de),a		;c84f
	sub 00fh		;c850
	sub 0d4h		;c852
	jp z,lcaedh		;c854
	di			;c857
	jp z,lcaf8h		;c858
	pop hl			;c85b
	ret			;c85c
	cp 0cah			;c85d
	add a,d			;c85f
	call nc,sub_d487h	;c860
	ld c,c			;c863
	call nc,sub_d4a0h	;c864
	xor c			;c867
	call nc,sub_d4afh	;c868
	call z,sub_dbd4h	;c86b
	call nc,sub_d4e4h	;c86e
	jp pe,0f0d4h		;c871
	call nc,sub_d4f9h	;c874
	ld (bc),a		;c877
	push de			;c878
	ex af,af'		;c879
	push de			;c87a
	ld c,0d5h		;c87b
	dec d			;c87d
	push de			;c87e
	inc l			;c87f
	call sub_d51bh		;c880
	ld hl,02ad5h		;c883
	push de			;c886
	ld sp,045d5h		;c887
	push de			;c88a
	ld c,e			;c88b
	push de			;c88c
	ld d,c			;c88d
	push de			;c88e
	ld (de),a		;c88f
	call nc,sub_d557h	;c890
	inc b			;c893
	rlc h			;c894
	res 3,a			;c896
	push de			;c898
	ld hl,lc8cah		;c899
	call sub_c8e5h		;c89c
	cp 003h			;c89f
	jp z,00000h		;c8a1
	ret			;c8a4
	ld hl,lc8d5h		;c8a5
	jp lc8b4h		;c8a8
	ld hl,0c8e1h		;c8ab
	jp lc8b4h		;c8ae
	ld hl,lc8dch		;c8b1
lc8b4h:
	call sub_c8e5h		;c8b4
	jp 00000h		;c8b7
lc8bah:
	ld b,e			;c8ba
	ld d,b			;c8bb
	cpl			;c8bc
	ld c,l			;c8bd
	jr nz,$+71		;c8be
	ld (hl),d		;c8c0
	ld (hl),d		;c8c1
	jr nz,lc913h		;c8c2
	ld l,(hl)		;c8c4
	jr nz,$+34		;c8c5
	ld a,(02420h)		;c8c7
lc8cah:
	ld b,d			;c8ca
	ld h,c			;c8cb
	ld h,h			;c8cc
	jr nz,lc922h		;c8cd
	ld h,l			;c8cf
	ld h,e			;c8d0
	ld (hl),h		;c8d1
	ld l,a			;c8d2
	ld (hl),d		;c8d3
	inc h			;c8d4
lc8d5h:
	ld d,e			;c8d5
	ld h,l			;c8d6
	ld l,h			;c8d7
	ld h,l			;c8d8
	ld h,e			;c8d9
	ld (hl),h		;c8da
	inc h			;c8db
lc8dch:
	ld b,(hl)		;c8dc
	ld l,c			;c8dd
	ld l,h			;c8de
	ld h,l			;c8df
	jr nz,$+84		;c8e0
	cpl			;c8e2
	ld c,a			;c8e3
	inc h			;c8e4
sub_c8e5h:
	push hl			;c8e5
	call sub_c9c9h		;c8e6
	ld a,(lcb42h)		;c8e9
	add a,041h		;c8ec
	ld (0c8c6h),a		;c8ee
	ld bc,lc8bah		;c8f1
	call sub_c9d3h		;c8f4
	pop bc			;c8f7
	call sub_c9d3h		;c8f8
sub_c8fbh:
	ld hl,lcb0eh		;c8fb
	ld a,(hl)		;c8fe
	ld (hl),000h		;c8ff
	or a			;c901
	ret nz			;c902
	jp bios_conin		;c903
sub_c906h:
	call sub_c8fbh		;c906
	call sub_c914h		;c909
	ret c			;c90c
	push af			;c90d
	ld c,a			;c90e
	call lc990h		;c90f
	pop af			;c912
lc913h:
	ret			;c913
sub_c914h:
	cp 00dh			;c914
	ret z			;c916
	cp 00ah			;c917
	ret z			;c919
	cp 009h			;c91a
	ret z			;c91c
	cp 008h			;c91d
	ret z			;c91f
	cp 020h			;c920
lc922h:
	ret			;c922
sub_c923h:
	ld a,(lcb0eh)		;c923
	or a			;c926
	jp nz,lc945h		;c927
	call bios_const		;c92a
	and 001h		;c92d
	ret z			;c92f
	call bios_conin		;c930
	cp 013h			;c933
	jp nz,lc942h		;c935
	call bios_conin		;c938
	cp 003h			;c93b
	jp z,00000h		;c93d
	xor a			;c940
	ret			;c941
lc942h:
	ld (lcb0eh),a		;c942
lc945h:
	ld a,001h		;c945
	ret			;c947
sub_c948h:
	ld a,(lcb0ah)		;c948
	or a			;c94b
	jp nz,lc962h		;c94c
	push bc			;c94f
	call sub_c923h		;c950
	pop bc			;c953
	push bc			;c954
	call bios_conout	;c955
	pop bc			;c958
	push bc			;c959
	ld a,(lcb0dh)		;c95a
	or a			;c95d
	call nz,bios_list	;c95e
	pop bc			;c961
lc962h:
	ld a,c			;c962
	ld hl,lcb0ch		;c963
	cp 07fh			;c966
	ret z			;c968
	inc (hl)		;c969
	cp 020h			;c96a
	ret nc			;c96c
	dec (hl)		;c96d
	ld a,(hl)		;c96e
	or a			;c96f
	ret z			;c970
	ld a,c			;c971
	cp 008h			;c972
	jp nz,lc979h		;c974
	dec (hl)		;c977
	ret			;c978
lc979h:
	cp 00ah			;c979
	ret nz			;c97b
	ld (hl),000h		;c97c
	ret			;c97e
sub_c97fh:
	ld a,c			;c97f
	call sub_c914h		;c980
	jp nc,lc990h		;c983
	push af			;c986
	ld c,05eh		;c987
	call sub_c948h		;c989
	pop af			;c98c
	or 040h			;c98d
	ld c,a			;c98f
lc990h:
	ld a,c			;c990
	cp 009h			;c991
	jp nz,sub_c948h		;c993
lc996h:
	ld c,020h		;c996
	call sub_c948h		;c998
	ld a,(lcb0ch)		;c99b
	and 007h		;c99e
	jp nz,lc996h		;c9a0
	ret			;c9a3
sub_c9a4h:
	call sub_c9ach		;c9a4
	ld c,020h		;c9a7
	call bios_conout	;c9a9
sub_c9ach:
	ld c,008h		;c9ac
	jp bios_conout		;c9ae
sub_c9b1h:
	ld c,023h		;c9b1
	call sub_c948h		;c9b3
	call sub_c9c9h		;c9b6
lc9b9h:
	ld a,(lcb0ch)		;c9b9
	ld hl,lcb0bh		;c9bc
	cp (hl)			;c9bf
	ret nc			;c9c0
	ld c,020h		;c9c1
	call sub_c948h		;c9c3
	jp lc9b9h		;c9c6
sub_c9c9h:
	ld c,00dh		;c9c9
	call sub_c948h		;c9cb
	ld c,00ah		;c9ce
	jp sub_c948h		;c9d0
sub_c9d3h:
	ld a,(bc)		;c9d3
	cp 024h			;c9d4
	ret z			;c9d6
	inc bc			;c9d7
	push bc			;c9d8
	ld c,a			;c9d9
	call lc990h		;c9da
	pop bc			;c9dd
	jp sub_c9d3h		;c9de
lc9e1h:
	ld a,(lcb0ch)		;c9e1
	ld (lcb0bh),a		;c9e4
	ld hl,(lcb43h)		;c9e7
	ld c,(hl)		;c9ea
	inc hl			;c9eb
	push hl			;c9ec
	ld b,000h		;c9ed
lc9efh:
	push bc			;c9ef
	push hl			;c9f0
lc9f1h:
	call sub_c8fbh		;c9f1
	and 07fh		;c9f4
	pop hl			;c9f6
	pop bc			;c9f7
	cp 00dh			;c9f8
	jp z,lcac1h		;c9fa
	cp 00ah			;c9fd
	jp z,lcac1h		;c9ff
	cp 008h			;ca02
	jp nz,lca16h		;ca04
	ld a,b			;ca07
	or a			;ca08
	jp z,lc9efh		;ca09
	dec b			;ca0c
	ld a,(lcb0ch)		;ca0d
	ld (lcb0ah),a		;ca10
	jp lca70h		;ca13
lca16h:
	cp 07fh			;ca16
	jp nz,lca26h		;ca18
	ld a,b			;ca1b
	or a			;ca1c
	jp z,lc9efh		;ca1d
	ld a,(hl)		;ca20
	dec b			;ca21
	dec hl			;ca22
	jp lcaa9h		;ca23
lca26h:
	cp 005h			;ca26
	jp nz,lca37h		;ca28
	push bc			;ca2b
	push hl			;ca2c
	call sub_c9c9h		;ca2d
	xor a			;ca30
	ld (lcb0bh),a		;ca31
	jp lc9f1h		;ca34
lca37h:
	cp 010h			;ca37
	jp nz,lca48h		;ca39
	push hl			;ca3c
	ld hl,lcb0dh		;ca3d
	ld a,001h		;ca40
	sub (hl)		;ca42
	ld (hl),a		;ca43
	pop hl			;ca44
	jp lc9efh		;ca45
lca48h:
	cp 018h			;ca48
	jp nz,lca5fh		;ca4a
	pop hl			;ca4d
lca4eh:
	ld a,(lcb0bh)		;ca4e
	ld hl,lcb0ch		;ca51
	cp (hl)			;ca54
	jp nc,lc9e1h		;ca55
	dec (hl)		;ca58
	call sub_c9a4h		;ca59
	jp lca4eh		;ca5c
lca5fh:
	cp 015h			;ca5f
	jp nz,lca6bh		;ca61
	call sub_c9b1h		;ca64
	pop hl			;ca67
	jp lc9e1h		;ca68
lca6bh:
	cp 012h			;ca6b
	jp nz,lcaa6h		;ca6d
lca70h:
	push bc			;ca70
	call sub_c9b1h		;ca71
	pop bc			;ca74
	pop hl			;ca75
	push hl			;ca76
	push bc			;ca77
lca78h:
	ld a,b			;ca78
	or a			;ca79
	jp z,lca8ah		;ca7a
	inc hl			;ca7d
	ld c,(hl)		;ca7e
	dec b			;ca7f
	push bc			;ca80
	push hl			;ca81
	call sub_c97fh		;ca82
	pop hl			;ca85
	pop bc			;ca86
	jp lca78h		;ca87
lca8ah:
	push hl			;ca8a
	ld a,(lcb0ah)		;ca8b
	or a			;ca8e
	jp z,lc9f1h		;ca8f
	ld hl,lcb0ch		;ca92
	sub (hl)		;ca95
	ld (lcb0ah),a		;ca96
lca99h:
	call sub_c9a4h		;ca99
	ld hl,lcb0ah		;ca9c
	dec (hl)		;ca9f
	jp nz,lca99h		;caa0
	jp lc9f1h		;caa3
lcaa6h:
	inc hl			;caa6
	ld (hl),a		;caa7
	inc b			;caa8
lcaa9h:
	push bc			;caa9
	push hl			;caaa
	ld c,a			;caab
	call sub_c97fh		;caac
	pop hl			;caaf
	pop bc			;cab0
	ld a,(hl)		;cab1
	cp 003h			;cab2
	ld a,b			;cab4
	jp nz,lcabdh		;cab5
	cp 001h			;cab8
	jp z,00000h		;caba
lcabdh:
	cp c			;cabd
	jp c,lc9efh		;cabe
lcac1h:
	pop hl			;cac1
	ld (hl),b		;cac2
	ld c,00dh		;cac3
	jp sub_c948h		;cac5
	call sub_c906h		;cac8
	jp lcb01h		;cacb
	call sub_d615h		;cace
	jp lcb01h		;cad1
	ld a,c			;cad4
	inc a			;cad5
	jp z,lcae0h		;cad6
	inc a			;cad9
	jp z,bios_const		;cada
	jp bios_conout		;cadd
lcae0h:
	call bios_const		;cae0
	or a			;cae3
	jp z,ld595h		;cae4
	call bios_conin		;cae7
	jp lcb01h		;caea
lcaedh:
	ld a,(00003h)		;caed
	jp lcb01h		;caf0
	ld hl,00003h		;caf3
	ld (hl),c		;caf6
	ret			;caf7
lcaf8h:
	ex de,hl		;caf8
	ld c,l			;caf9
	ld b,h			;cafa
	jp sub_c9d3h		;cafb
	call sub_c923h		;cafe
lcb01h:
	ld (lcb45h),a		;cb01
	ret			;cb04
sub_cb05h:
	ld a,001h		;cb05
	jp lcb01h		;cb07
lcb0ah:
	nop			;cb0a
lcb0bh:
	nop			;cb0b
lcb0ch:
	nop			;cb0c
lcb0dh:
	nop			;cb0d
lcb0eh:
	nop			;cb0e
lcb0fh:
	nop			;cb0f
	nop			;cb10
	nop			;cb11
	nop			;cb12
	nop			;cb13
	nop			;cb14
	nop			;cb15
	nop			;cb16
	nop			;cb17
	nop			;cb18
	nop			;cb19
	nop			;cb1a
	nop			;cb1b
	nop			;cb1c
	nop			;cb1d
	nop			;cb1e
	nop			;cb1f
	nop			;cb20
	nop			;cb21
	nop			;cb22
	nop			;cb23
	nop			;cb24
	nop			;cb25
	nop			;cb26
	nop			;cb27
	nop			;cb28
	nop			;cb29
	nop			;cb2a
	nop			;cb2b
	nop			;cb2c
	nop			;cb2d
	nop			;cb2e
	nop			;cb2f
	nop			;cb30
	nop			;cb31
	nop			;cb32
	nop			;cb33
	nop			;cb34
	nop			;cb35
	nop			;cb36
	nop			;cb37
	nop			;cb38
	nop			;cb39
	nop			;cb3a
	nop			;cb3b
	nop			;cb3c
	nop			;cb3d
	nop			;cb3e
	nop			;cb3f
	nop			;cb40
lcb41h:
	nop			;cb41
lcb42h:
	nop			;cb42
lcb43h:
	nop			;cb43
	nop			;cb44
lcb45h:
	nop			;cb45
	nop			;cb46
sub_cb47h:
	ld hl,lc80bh		;cb47
lcb4ah:
	ld e,(hl)		;cb4a
	inc hl			;cb4b
	ld d,(hl)		;cb4c
	ex de,hl		;cb4d
	jp (hl)			;cb4e
sub_cb4fh:
	inc c			;cb4f
lcb50h:
	dec c			;cb50
	ret z			;cb51
	ld a,(de)		;cb52
	ld (hl),a		;cb53
	inc de			;cb54
	inc hl			;cb55
	jp lcb50h		;cb56
sub_cb59h:
	ld a,(lcb42h)		;cb59
	ld c,a			;cb5c
	call bios_seldsk	;cb5d
	ld a,h			;cb60
	or l			;cb61
	ret z			;cb62
	ld e,(hl)		;cb63
	inc hl			;cb64
	ld d,(hl)		;cb65
	inc hl			;cb66
	ld (ld5b7h),hl		;cb67
	inc hl			;cb6a
	inc hl			;cb6b
	ld (ld5b9h),hl		;cb6c
	inc hl			;cb6f
	inc hl			;cb70
	ld (ld5bbh),hl		;cb71
	inc hl			;cb74
	inc hl			;cb75
	ex de,hl		;cb76
	ld (ld5d4h),hl		;cb77
	ld hl,ld5bdh		;cb7a
	ld c,008h		;cb7d
	call sub_cb4fh		;cb7f
	ld hl,(ld5bfh)		;cb82
	ex de,hl		;cb85
	ld hl,ld5c5h		;cb86
	ld c,00fh		;cb89
	call sub_cb4fh		;cb8b
	ld hl,(ld5cah)		;cb8e
	ld a,h			;cb91
	ld hl,ld5e1h		;cb92
	ld (hl),0ffh		;cb95
	or a			;cb97
	jp z,lcb9dh		;cb98
	ld (hl),000h		;cb9b
lcb9dh:
	ld a,0ffh		;cb9d
	or a			;cb9f
	ret			;cba0
sub_cba1h:
	call bios_home		;cba1
	xor a			;cba4
	ld hl,(ld5b9h)		;cba5
	ld (hl),a		;cba8
	inc hl			;cba9
	ld (hl),a		;cbaa
	ld hl,(ld5bbh)		;cbab
	ld (hl),a		;cbae
	inc hl			;cbaf
	ld (hl),a		;cbb0
	ret			;cbb1
sub_cbb2h:
	call bios_read		;cbb2
	jp lcbbbh		;cbb5
sub_cbb8h:
	call bios_write		;cbb8
lcbbbh:
	or a			;cbbb
	ret z			;cbbc
	ld hl,lc809h		;cbbd
	jp lcb4ah		;cbc0
sub_cbc3h:
	ld hl,(ld5eeh)		;cbc3
	ld c,002h		;cbc6
	call sub_cceah		;cbc8
	ld (ld5e9h),hl		;cbcb
	ld (ld5f0h),hl		;cbce
sub_cbd1h:
	ld hl,ld5e9h		;cbd1
	ld c,(hl)		;cbd4
	inc hl			;cbd5
	ld b,(hl)		;cbd6
	ld hl,(ld5bbh)		;cbd7
	ld e,(hl)		;cbda
	inc hl			;cbdb
	ld d,(hl)		;cbdc
	ld hl,(ld5b9h)		;cbdd
	ld a,(hl)		;cbe0
	inc hl			;cbe1
	ld h,(hl)		;cbe2
	ld l,a			;cbe3
lcbe4h:
	ld a,c			;cbe4
	sub e			;cbe5
	ld a,b			;cbe6
	sbc a,d			;cbe7
	jp nc,lcbfah		;cbe8
	push hl			;cbeb
	ld hl,(ld5c5h)		;cbec
	ld a,e			;cbef
	sub l			;cbf0
	ld e,a			;cbf1
	ld a,d			;cbf2
	sbc a,h			;cbf3
	ld d,a			;cbf4
	pop hl			;cbf5
	dec hl			;cbf6
	jp lcbe4h		;cbf7
lcbfah:
	push hl			;cbfa
	ld hl,(ld5c5h)		;cbfb
	add hl,de		;cbfe
	jp c,lcc0fh		;cbff
	ld a,c			;cc02
	sub l			;cc03
	ld a,b			;cc04
	sbc a,h			;cc05
	jp c,lcc0fh		;cc06
	ex de,hl		;cc09
	pop hl			;cc0a
	inc hl			;cc0b
	jp lcbfah		;cc0c
lcc0fh:
	pop hl			;cc0f
	push bc			;cc10
	push de			;cc11
	push hl			;cc12
	ex de,hl		;cc13
	ld hl,(ld5d2h)		;cc14
	add hl,de		;cc17
	ld b,h			;cc18
	ld c,l			;cc19
	call bios_settrk	;cc1a
	pop de			;cc1d
	ld hl,(ld5b9h)		;cc1e
	ld (hl),e		;cc21
	inc hl			;cc22
	ld (hl),d		;cc23
	pop de			;cc24
	ld hl,(ld5bbh)		;cc25
	ld (hl),e		;cc28
	inc hl			;cc29
	ld (hl),d		;cc2a
	pop bc			;cc2b
	ld a,c			;cc2c
	sub e			;cc2d
	ld c,a			;cc2e
	ld a,b			;cc2f
	sbc a,d			;cc30
	ld b,a			;cc31
	ld hl,(ld5d4h)		;cc32
	ex de,hl		;cc35
	call bios_sectran	;cc36
	ld c,l			;cc39
	ld b,h			;cc3a
	jp bios_setsec		;cc3b
sub_cc3eh:
	ld hl,ld5c7h		;cc3e
	ld c,(hl)		;cc41
	ld a,(ld5e7h)		;cc42
lcc45h:
	or a			;cc45
	rra			;cc46
	dec c			;cc47
	jp nz,lcc45h		;cc48
	ld b,a			;cc4b
	ld a,008h		;cc4c
	sub (hl)		;cc4e
	ld c,a			;cc4f
	ld a,(ld5e6h)		;cc50
lcc53h:
	dec c			;cc53
	jp z,lcc5ch		;cc54
	or a			;cc57
	rla			;cc58
	jp lcc53h		;cc59
lcc5ch:
	add a,b			;cc5c
	ret			;cc5d
sub_cc5eh:
	ld hl,(lcb43h)		;cc5e
	ld de,00010h		;cc61
	add hl,de		;cc64
	add hl,bc		;cc65
	ld a,(ld5e1h)		;cc66
	or a			;cc69
	jp z,lcc71h		;cc6a
	ld l,(hl)		;cc6d
	ld h,000h		;cc6e
	ret			;cc70
lcc71h:
	add hl,bc		;cc71
	ld e,(hl)		;cc72
	inc hl			;cc73
	ld d,(hl)		;cc74
	ex de,hl		;cc75
	ret			;cc76
sub_cc77h:
	call sub_cc3eh		;cc77
	ld c,a			;cc7a
	ld b,000h		;cc7b
	call sub_cc5eh		;cc7d
	ld (ld5e9h),hl		;cc80
	ret			;cc83
sub_cc84h:
	ld hl,(ld5e9h)		;cc84
	ld a,l			;cc87
	or h			;cc88
	ret			;cc89
sub_cc8ah:
	ld a,(ld5c7h)		;cc8a
	ld hl,(ld5e9h)		;cc8d
lcc90h:
	add hl,hl		;cc90
	dec a			;cc91
	jp nz,lcc90h		;cc92
	ld (ld5ebh),hl		;cc95
	ld a,(ld5c8h)		;cc98
	ld c,a			;cc9b
	ld a,(ld5e7h)		;cc9c
	and c			;cc9f
	or l			;cca0
	ld l,a			;cca1
	ld (ld5e9h),hl		;cca2
	ret			;cca5
sub_cca6h:
	ld hl,(lcb43h)		;cca6
	ld de,0000ch		;cca9
	add hl,de		;ccac
	ret			;ccad
sub_ccaeh:
	ld hl,(lcb43h)		;ccae
	ld de,0000fh		;ccb1
	add hl,de		;ccb4
	ex de,hl		;ccb5
	ld hl,00011h		;ccb6
	add hl,de		;ccb9
	ret			;ccba
sub_ccbbh:
	call sub_ccaeh		;ccbb
	ld a,(hl)		;ccbe
	ld (ld5e7h),a		;ccbf
	ex de,hl		;ccc2
	ld a,(hl)		;ccc3
	ld (ld5e5h),a		;ccc4
	call sub_cca6h		;ccc7
	ld a,(ld5c9h)		;ccca
	and (hl)		;cccd
	ld (ld5e6h),a		;ccce
	ret			;ccd1
lccd2h:
	call sub_ccaeh		;ccd2
	ld a,(ld5d9h)		;ccd5
	cp 002h			;ccd8
	jp nz,lccdeh		;ccda
	xor a			;ccdd
lccdeh:
	ld c,a			;ccde
	ld a,(ld5e7h)		;ccdf
	add a,c			;cce2
	ld (hl),a		;cce3
	ex de,hl		;cce4
	ld a,(ld5e5h)		;cce5
	ld (hl),a		;cce8
	ret			;cce9
sub_cceah:
	inc c			;ccea
lccebh:
	dec c			;cceb
	ret z			;ccec
	ld a,h			;cced
	or a			;ccee
	rra			;ccef
	ld h,a			;ccf0
	ld a,l			;ccf1
	rra			;ccf2
	ld l,a			;ccf3
	jp lccebh		;ccf4
sub_ccf7h:
	ld c,080h		;ccf7
	ld hl,(ld5bdh)		;ccf9
	xor a			;ccfc
lccfdh:
	add a,(hl)		;ccfd
	inc hl			;ccfe
	dec c			;ccff
	jp nz,lccfdh		;cd00
	ret			;cd03
sub_cd04h:
	inc c			;cd04
lcd05h:
	dec c			;cd05
	ret z			;cd06
	add hl,hl		;cd07
	jp lcd05h		;cd08
sub_cd0bh:
	push bc			;cd0b
	ld a,(lcb42h)		;cd0c
	ld c,a			;cd0f
	ld hl,00001h		;cd10
	call sub_cd04h		;cd13
	pop bc			;cd16
	ld a,c			;cd17
	or l			;cd18
	ld l,a			;cd19
	ld a,b			;cd1a
	or h			;cd1b
	ld h,a			;cd1c
	ret			;cd1d
sub_cd1eh:
	ld hl,(ld5b1h)		;cd1e
	ld a,(lcb42h)		;cd21
	ld c,a			;cd24
	call sub_cceah		;cd25
	ld a,l			;cd28
	and 001h		;cd29
	ret			;cd2b
sub_cd2ch:
	ld hl,ld5b1h		;cd2c
	ld c,(hl)		;cd2f
	inc hl			;cd30
	ld b,(hl)		;cd31
	call sub_cd0bh		;cd32
	ld (ld5b1h),hl		;cd35
	ld hl,(ld5cch)		;cd38
	inc hl			;cd3b
	ex de,hl		;cd3c
	ld hl,(ld5b7h)		;cd3d
	ld (hl),e		;cd40
	inc hl			;cd41
	ld (hl),d		;cd42
	ret			;cd43
sub_cd44h:
	call sub_cd5eh		;cd44
sub_cd47h:
	ld de,00009h		;cd47
	add hl,de		;cd4a
	ld a,(hl)		;cd4b
	rla			;cd4c
	ret nc			;cd4d
	ld hl,lc80fh		;cd4e
	jp lcb4ah		;cd51
sub_cd54h:
	call sub_cd1eh		;cd54
	ret z			;cd57
	ld hl,lc80dh		;cd58
	jp lcb4ah		;cd5b
sub_cd5eh:
	ld hl,(ld5bdh)		;cd5e
	ld a,(ld5edh)		;cd61
sub_cd64h:
	add a,l			;cd64
	ld l,a			;cd65
	ret nc			;cd66
	inc h			;cd67
	ret			;cd68
sub_cd69h:
	ld hl,(lcb43h)		;cd69
	ld de,0000eh		;cd6c
	add hl,de		;cd6f
	ld a,(hl)		;cd70
	ret			;cd71
sub_cd72h:
	call sub_cd69h		;cd72
	ld (hl),000h		;cd75
	ret			;cd77
sub_cd78h:
	call sub_cd69h		;cd78
	or 080h			;cd7b
	ld (hl),a		;cd7d
	ret			;cd7e
sub_cd7fh:
	ld hl,(ld5eeh)		;cd7f
	ex de,hl		;cd82
	ld hl,(ld5b7h)		;cd83
	ld a,e			;cd86
	sub (hl)		;cd87
	inc hl			;cd88
	ld a,d			;cd89
	sbc a,(hl)		;cd8a
	ret			;cd8b
sub_cd8ch:
	call sub_cd7fh		;cd8c
	ret c			;cd8f
	inc de			;cd90
	ld (hl),d		;cd91
	dec hl			;cd92
	ld (hl),e		;cd93
	ret			;cd94
sub_cd95h:
	ld a,e			;cd95
	sub l			;cd96
	ld l,a			;cd97
	ld a,d			;cd98
	sbc a,h			;cd99
	ld h,a			;cd9a
	ret			;cd9b
sub_cd9ch:
	ld c,0ffh		;cd9c
lcd9eh:
	ld hl,(ld5f0h)		;cd9e
	ex de,hl		;cda1
	ld hl,(ld5d0h)		;cda2
	call sub_cd95h		;cda5
	ret nc			;cda8
	push bc			;cda9
	call sub_ccf7h		;cdaa
	ld hl,(ld5c1h)		;cdad
	ex de,hl		;cdb0
	ld hl,(ld5f0h)		;cdb1
	add hl,de		;cdb4
	pop bc			;cdb5
	inc c			;cdb6
	jp z,lcdc4h		;cdb7
	cp (hl)			;cdba
	ret z			;cdbb
	call sub_cd7fh		;cdbc
	ret nc			;cdbf
	call sub_cd2ch		;cdc0
	ret			;cdc3
lcdc4h:
	ld (hl),a		;cdc4
	ret			;cdc5
sub_cdc6h:
	call sub_cd9ch		;cdc6
	call sub_cde0h		;cdc9
	ld c,001h		;cdcc
	call sub_cbb8h		;cdce
	jp lcddah		;cdd1
sub_cdd4h:
	call sub_cde0h		;cdd4
	call sub_cbb2h		;cdd7
lcddah:
	ld hl,ld5b5h		;cdda
	jp lcde3h		;cddd
sub_cde0h:
	ld hl,ld5bdh		;cde0
lcde3h:
	ld c,(hl)		;cde3
	inc hl			;cde4
	ld b,(hl)		;cde5
	jp bios_setdma		;cde6
lcde9h:
	ld hl,(ld5bdh)		;cde9
	ex de,hl		;cdec
	ld hl,(ld5b5h)		;cded
	ld c,080h		;cdf0
	jp sub_cb4fh		;cdf2
sub_cdf5h:
	ld hl,ld5eeh		;cdf5
	ld a,(hl)		;cdf8
	inc hl			;cdf9
	cp (hl)			;cdfa
	ret nz			;cdfb
	inc a			;cdfc
	ret			;cdfd
lcdfeh:
	ld hl,0ffffh		;cdfe
	ld (ld5eeh),hl		;ce01
	ret			;ce04
sub_ce05h:
	ld hl,(ld5cch)		;ce05
	ex de,hl		;ce08
	ld hl,(ld5eeh)		;ce09
	inc hl			;ce0c
	ld (ld5eeh),hl		;ce0d
	call sub_cd95h		;ce10
	jp nc,lce19h		;ce13
	jp lcdfeh		;ce16
lce19h:
	ld a,(ld5eeh)		;ce19
	and 003h		;ce1c
	ld b,005h		;ce1e
lce20h:
	add a,a			;ce20
	dec b			;ce21
	jp nz,lce20h		;ce22
	ld (ld5edh),a		;ce25
	or a			;ce28
	ret nz			;ce29
	push bc			;ce2a
	call sub_cbc3h		;ce2b
	call sub_cdd4h		;ce2e
	pop bc			;ce31
	jp lcd9eh		;ce32
sub_ce35h:
	ld a,c			;ce35
	and 007h		;ce36
	inc a			;ce38
	ld e,a			;ce39
	ld d,a			;ce3a
	ld a,c			;ce3b
	rrca			;ce3c
	rrca			;ce3d
	rrca			;ce3e
	and 01fh		;ce3f
	ld c,a			;ce41
	ld a,b			;ce42
	add a,a			;ce43
	add a,a			;ce44
	add a,a			;ce45
	add a,a			;ce46
	add a,a			;ce47
	or c			;ce48
	ld c,a			;ce49
	ld a,b			;ce4a
	rrca			;ce4b
	rrca			;ce4c
	rrca			;ce4d
	and 01fh		;ce4e
	ld b,a			;ce50
	ld hl,(ld5c3h)		;ce51
	add hl,bc		;ce54
	ld a,(hl)		;ce55
lce56h:
	rlca			;ce56
	dec e			;ce57
	jp nz,lce56h		;ce58
	ret			;ce5b
sub_ce5ch:
	push de			;ce5c
	call sub_ce35h		;ce5d
	and 0feh		;ce60
	pop bc			;ce62
	or c			;ce63
lce64h:
	rrca			;ce64
	dec d			;ce65
	jp nz,lce64h		;ce66
	ld (hl),a		;ce69
	ret			;ce6a
sub_ce6bh:
	call sub_cd5eh		;ce6b
	ld de,00010h		;ce6e
	add hl,de		;ce71
	push bc			;ce72
	ld c,011h		;ce73
lce75h:
	pop de			;ce75
	dec c			;ce76
	ret z			;ce77
	push de			;ce78
	ld a,(ld5e1h)		;ce79
	or a			;ce7c
	jp z,lce88h		;ce7d
	push bc			;ce80
	push hl			;ce81
	ld c,(hl)		;ce82
	ld b,000h		;ce83
	jp lce8eh		;ce85
lce88h:
	dec c			;ce88
	push bc			;ce89
	ld c,(hl)		;ce8a
	inc hl			;ce8b
	ld b,(hl)		;ce8c
	push hl			;ce8d
lce8eh:
	ld a,c			;ce8e
	or b			;ce8f
	jp z,lce9dh		;ce90
	ld hl,(ld5cah)		;ce93
	ld a,l			;ce96
	sub c			;ce97
	ld a,h			;ce98
	sbc a,b			;ce99
	call nc,sub_ce5ch	;ce9a
lce9dh:
	pop hl			;ce9d
	inc hl			;ce9e
	pop bc			;ce9f
	jp lce75h		;cea0
lcea3h:
	ld hl,(ld5cah)		;cea3
	ld c,003h		;cea6
	call sub_cceah		;cea8
	inc hl			;ceab
	ld b,h			;ceac
	ld c,l			;cead
	ld hl,(ld5c3h)		;ceae
lceb1h:
	ld (hl),000h		;ceb1
	inc hl			;ceb3
	dec bc			;ceb4
	ld a,b			;ceb5
	or c			;ceb6
	jp nz,lceb1h		;ceb7
	ld hl,(ld5ceh)		;ceba
	ex de,hl		;cebd
	ld hl,(ld5c3h)		;cebe
	ld (hl),e		;cec1
	inc hl			;cec2
	ld (hl),d		;cec3
	call sub_cba1h		;cec4
	ld hl,(ld5b7h)		;cec7
	ld (hl),003h		;ceca
	inc hl			;cecc
	ld (hl),000h		;cecd
	call lcdfeh		;cecf
lced2h:
	ld c,0ffh		;ced2
	call sub_ce05h		;ced4
	call sub_cdf5h		;ced7
	ret z			;ceda
	call sub_cd5eh		;cedb
	ld a,0e5h		;cede
	cp (hl)			;cee0
	jr z,lcf00h		;cee1
	ld a,(lcb41h)		;cee3
	cp (hl)			;cee6
	jp nz,lcef5h		;cee7
	inc hl			;ceea
	ld a,(hl)		;ceeb
	sub 024h		;ceec
	jp nz,lcef5h		;ceee
	dec a			;cef1
	ld (lcb45h),a		;cef2
lcef5h:
	ld c,001h		;cef5
	call sub_ce6bh		;cef7
	call sub_cd8ch		;cefa
	jp lced2h		;cefd
lcf00h:
	inc hl			;cf00
	cp (hl)			;cf01
	ret z			;cf02
	jr lced2h		;cf03
lcf05h:
	ld a,(ld5d8h)		;cf05
	jp lcb01h		;cf08
sub_cf0bh:
	push bc			;cf0b
	push af			;cf0c
	ld a,(ld5c9h)		;cf0d
	cpl			;cf10
	ld b,a			;cf11
	ld a,c			;cf12
	and b			;cf13
	ld c,a			;cf14
	pop af			;cf15
	and b			;cf16
	sub c			;cf17
	and 01fh		;cf18
	pop bc			;cf1a
	ret			;cf1b
sub_cf1ch:
	ld a,0ffh		;cf1c
	ld (ld5d8h),a		;cf1e
	ld hl,ld5dch		;cf21
	ld (hl),c		;cf24
	ld hl,(lcb43h)		;cf25
	ld (ld5ddh),hl		;cf28
	call lcdfeh		;cf2b
	call sub_cba1h		;cf2e
lcf31h:
	ld c,000h		;cf31
	call sub_ce05h		;cf33
	call sub_cdf5h		;cf36
	jp z,lcf98h		;cf39
	ld hl,(ld5ddh)		;cf3c
	ex de,hl		;cf3f
	ld a,(de)		;cf40
	cp 0e5h			;cf41
	jp z,lcf4eh		;cf43
	push de			;cf46
	call sub_cd7fh		;cf47
	pop de			;cf4a
	jp nc,lcf98h		;cf4b
lcf4eh:
	call sub_cd5eh		;cf4e
	ld a,(ld5dch)		;cf51
	ld c,a			;cf54
	ld b,000h		;cf55
lcf57h:
	ld a,c			;cf57
	or a			;cf58
	jp z,lcf87h		;cf59
	ld a,(de)		;cf5c
	cp 03fh			;cf5d
	jp z,lcf80h		;cf5f
	ld a,b			;cf62
	cp 00dh			;cf63
	jp z,lcf80h		;cf65
	cp 00ch			;cf68
	ld a,(de)		;cf6a
	jp z,lcf77h		;cf6b
	sub (hl)		;cf6e
	and 07fh		;cf6f
	jp nz,lcf31h		;cf71
	jp lcf80h		;cf74
lcf77h:
	push bc			;cf77
	ld c,(hl)		;cf78
	call sub_cf0bh		;cf79
	pop bc			;cf7c
	jp nz,lcf31h		;cf7d
lcf80h:
	inc de			;cf80
	inc hl			;cf81
	inc b			;cf82
	dec c			;cf83
	jp lcf57h		;cf84
lcf87h:
	ld a,(ld5eeh)		;cf87
	and 003h		;cf8a
	ld (lcb45h),a		;cf8c
	ld hl,ld5d8h		;cf8f
	ld a,(hl)		;cf92
	rla			;cf93
	ret nc			;cf94
	xor a			;cf95
	ld (hl),a		;cf96
	ret			;cf97
lcf98h:
	call lcdfeh		;cf98
	ld a,0ffh		;cf9b
	jp lcb01h		;cf9d
sub_cfa0h:
	call sub_cd54h		;cfa0
	ld c,00ch		;cfa3
	call sub_cf1ch		;cfa5
lcfa8h:
	call sub_cdf5h		;cfa8
	ret z			;cfab
	call sub_cd44h		;cfac
	call sub_cd5eh		;cfaf
	ld (hl),0e5h		;cfb2
	ld c,000h		;cfb4
	call sub_ce6bh		;cfb6
	call sub_cdc6h		;cfb9
	call lcf31h		;cfbc
	jp lcfa8h		;cfbf
sub_cfc2h:
	ld d,b			;cfc2
	ld e,c			;cfc3
lcfc4h:
	ld a,c			;cfc4
	or b			;cfc5
	jp z,lcfd5h		;cfc6
	dec bc			;cfc9
	push de			;cfca
	push bc			;cfcb
	call sub_ce35h		;cfcc
	rra			;cfcf
	jp nc,lcff0h		;cfd0
	pop bc			;cfd3
	pop de			;cfd4
lcfd5h:
	ld hl,(ld5cah)		;cfd5
	ld a,e			;cfd8
	sub l			;cfd9
	ld a,d			;cfda
	sbc a,h			;cfdb
	jp nc,lcff8h		;cfdc
	inc de			;cfdf
	push bc			;cfe0
	push de			;cfe1
	ld b,d			;cfe2
	ld c,e			;cfe3
	call sub_ce35h		;cfe4
	rra			;cfe7
	jp nc,lcff0h		;cfe8
	pop de			;cfeb
	pop bc			;cfec
	jp lcfc4h		;cfed
lcff0h:
	rla			;cff0
	inc a			;cff1
	call lce64h		;cff2
	pop hl			;cff5
	pop de			;cff6
	ret			;cff7
lcff8h:
	ld a,c			;cff8
	or b			;cff9
	jp nz,lcfc4h		;cffa
	ld hl,00000h		;cffd
	ret			;d000
sub_d001h:
	ld c,000h		;d001
	ld e,020h		;d003
sub_d005h:
	push de			;d005
	ld b,000h		;d006
	ld hl,(lcb43h)		;d008
	add hl,bc		;d00b
	ex de,hl		;d00c
	call sub_cd5eh		;d00d
	pop bc			;d010
	call sub_cb4fh		;d011
ld014h:
	call sub_cbc3h		;d014
	jp sub_cdc6h		;d017
sub_d01ah:
	call sub_cd54h		;d01a
	ld c,00ch		;d01d
	call sub_cf1ch		;d01f
	ld hl,(lcb43h)		;d022
	ld a,(hl)		;d025
	ld de,00010h		;d026
	add hl,de		;d029
	ld (hl),a		;d02a
ld02bh:
	call sub_cdf5h		;d02b
	ret z			;d02e
	call sub_cd44h		;d02f
	ld c,010h		;d032
	ld e,00ch		;d034
	call sub_d005h		;d036
	call lcf31h		;d039
	jp ld02bh		;d03c
sub_d03fh:
	ld c,00ch		;d03f
	call sub_cf1ch		;d041
ld044h:
	call sub_cdf5h		;d044
	ret z			;d047
	ld c,000h		;d048
	ld e,00ch		;d04a
	call sub_d005h		;d04c
	call lcf31h		;d04f
	jp ld044h		;d052
sub_d055h:
	ld c,00fh		;d055
	call sub_cf1ch		;d057
	call sub_cdf5h		;d05a
	ret z			;d05d
sub_d05eh:
	call sub_cca6h		;d05e
	ld a,(hl)		;d061
	push af			;d062
	push hl			;d063
	call sub_cd5eh		;d064
	ex de,hl		;d067
	ld hl,(lcb43h)		;d068
	ld c,020h		;d06b
	push de			;d06d
	call sub_cb4fh		;d06e
	call sub_cd78h		;d071
	pop de			;d074
	ld hl,0000ch		;d075
	add hl,de		;d078
	ld c,(hl)		;d079
	ld hl,0000fh		;d07a
	add hl,de		;d07d
	ld b,(hl)		;d07e
	pop hl			;d07f
	pop af			;d080
	ld (hl),a		;d081
	ld a,c			;d082
	cp (hl)			;d083
	ld a,b			;d084
	jp z,ld08fh		;d085
	ld a,000h		;d088
	jp c,ld08fh		;d08a
	ld a,080h		;d08d
ld08fh:
	ld hl,(lcb43h)		;d08f
	ld de,0000fh		;d092
	add hl,de		;d095
	ld (hl),a		;d096
	ret			;d097
sub_d098h:
	ld a,(hl)		;d098
	inc hl			;d099
	or (hl)			;d09a
	dec hl			;d09b
	ret nz			;d09c
	ld a,(de)		;d09d
	ld (hl),a		;d09e
	inc de			;d09f
	inc hl			;d0a0
	ld a,(de)		;d0a1
	ld (hl),a		;d0a2
	dec de			;d0a3
	dec hl			;d0a4
	ret			;d0a5
sub_d0a6h:
	xor a			;d0a6
	ld (lcb45h),a		;d0a7
	ld (ld5eeh),a		;d0aa
	ld (ld5efh),a		;d0ad
	call sub_cd1eh		;d0b0
	ret nz			;d0b3
	call sub_cd69h		;d0b4
	and 080h		;d0b7
	ret nz			;d0b9
	ld c,00fh		;d0ba
	call sub_cf1ch		;d0bc
	call sub_cdf5h		;d0bf
	ret z			;d0c2
	ld bc,00010h		;d0c3
	call sub_cd5eh		;d0c6
	add hl,bc		;d0c9
	ex de,hl		;d0ca
	ld hl,(lcb43h)		;d0cb
	add hl,bc		;d0ce
	ld c,010h		;d0cf
ld0d1h:
	ld a,(ld5e1h)		;d0d1
	or a			;d0d4
	jp z,ld0ech		;d0d5
	ld a,(hl)		;d0d8
	or a			;d0d9
	ld a,(de)		;d0da
	jp nz,ld0dfh		;d0db
	ld (hl),a		;d0de
ld0dfh:
	or a			;d0df
	jp nz,ld0e5h		;d0e0
	ld a,(hl)		;d0e3
	ld (de),a		;d0e4
ld0e5h:
	cp (hl)			;d0e5
	jp nz,ld123h		;d0e6
	jp ld101h		;d0e9
ld0ech:
	call sub_d098h		;d0ec
	ex de,hl		;d0ef
	call sub_d098h		;d0f0
	ex de,hl		;d0f3
	ld a,(de)		;d0f4
	cp (hl)			;d0f5
	jp nz,ld123h		;d0f6
	inc de			;d0f9
	inc hl			;d0fa
	ld a,(de)		;d0fb
	cp (hl)			;d0fc
	jp nz,ld123h		;d0fd
	dec c			;d100
ld101h:
	inc de			;d101
	inc hl			;d102
	dec c			;d103
	jp nz,ld0d1h		;d104
	ld bc,0ffech		;d107
	add hl,bc		;d10a
	ex de,hl		;d10b
	add hl,bc		;d10c
	ld a,(de)		;d10d
	cp (hl)			;d10e
	jp c,ld11bh		;d10f
	ld (hl),a		;d112
	ld bc,00003h		;d113
	add hl,bc		;d116
	ex de,hl		;d117
	add hl,bc		;d118
	ld a,(hl)		;d119
	ld (de),a		;d11a
ld11bh:
	ld a,0ffh		;d11b
	ld (ld5d6h),a		;d11d
	jp ld014h		;d120
ld123h:
	ld hl,lcb45h		;d123
	dec (hl)		;d126
	ret			;d127
sub_d128h:
	call sub_cd54h		;d128
	ld hl,(lcb43h)		;d12b
	push hl			;d12e
	ld hl,ld5b0h		;d12f
	ld (lcb43h),hl		;d132
	ld c,001h		;d135
	call sub_cf1ch		;d137
	call sub_cdf5h		;d13a
	pop hl			;d13d
	ld (lcb43h),hl		;d13e
	ret z			;d141
	ex de,hl		;d142
	ld hl,0000fh		;d143
	add hl,de		;d146
	ld c,011h		;d147
	xor a			;d149
ld14ah:
	ld (hl),a		;d14a
	inc hl			;d14b
	dec c			;d14c
	jp nz,ld14ah		;d14d
	ld hl,0000dh		;d150
	add hl,de		;d153
	ld (hl),a		;d154
	call sub_cd8ch		;d155
	call sub_d001h		;d158
	jp sub_cd78h		;d15b
sub_d15eh:
	xor a			;d15e
	ld (ld5d6h),a		;d15f
	call sub_d0a6h		;d162
	call sub_cdf5h		;d165
	ret z			;d168
	ld hl,(lcb43h)		;d169
	ld bc,0000ch		;d16c
	add hl,bc		;d16f
	ld a,(hl)		;d170
	inc a			;d171
	and 01fh		;d172
	ld (hl),a		;d174
	jp z,ld187h		;d175
	ld b,a			;d178
	ld a,(ld5c9h)		;d179
	and b			;d17c
	ld hl,ld5d6h		;d17d
	and (hl)		;d180
	jp z,ld192h		;d181
	jp ld1b0h		;d184
ld187h:
	ld bc,00002h		;d187
	add hl,bc		;d18a
	inc (hl)		;d18b
	ld a,(hl)		;d18c
	and 00fh		;d18d
	jp z,ld1bah		;d18f
ld192h:
	ld c,00fh		;d192
	call sub_cf1ch		;d194
	call sub_cdf5h		;d197
	jp nz,ld1b0h		;d19a
	ld a,(ld5d7h)		;d19d
	inc a			;d1a0
	jp z,ld1bah		;d1a1
	call sub_d128h		;d1a4
	call sub_cdf5h		;d1a7
	jp z,ld1bah		;d1aa
	jp ld1b3h		;d1ad
ld1b0h:
	call sub_d05eh		;d1b0
ld1b3h:
	call sub_ccbbh		;d1b3
	xor a			;d1b6
	jp lcb01h		;d1b7
ld1bah:
	call sub_cb05h		;d1ba
	jp sub_cd78h		;d1bd
ld1c0h:
	ld a,001h		;d1c0
	ld (ld5d9h),a		;d1c2
sub_d1c5h:
	ld a,0ffh		;d1c5
	ld (ld5d7h),a		;d1c7
	call sub_ccbbh		;d1ca
	ld a,(ld5e7h)		;d1cd
	ld hl,ld5e5h		;d1d0
	cp (hl)			;d1d3
	jp c,ld1eah		;d1d4
	cp 080h			;d1d7
	jp nz,ld1ffh		;d1d9
	call sub_d15eh		;d1dc
	xor a			;d1df
	ld (ld5e7h),a		;d1e0
	ld a,(lcb45h)		;d1e3
	or a			;d1e6
	jp nz,ld1ffh		;d1e7
ld1eah:
	call sub_cc77h		;d1ea
	call sub_cc84h		;d1ed
	jp z,ld1ffh		;d1f0
	call sub_cc8ah		;d1f3
	call sub_cbd1h		;d1f6
	call sub_cbb2h		;d1f9
	jp lccd2h		;d1fc
ld1ffh:
	jp sub_cb05h		;d1ff
ld202h:
	ld a,001h		;d202
	ld (ld5d9h),a		;d204
sub_d207h:
	ld a,000h		;d207
	ld (ld5d7h),a		;d209
	call sub_cd54h		;d20c
	ld hl,(lcb43h)		;d20f
	call sub_cd47h		;d212
	call sub_ccbbh		;d215
	ld a,(ld5e7h)		;d218
	cp 080h			;d21b
	jp nc,sub_cb05h		;d21d
	call sub_cc77h		;d220
	call sub_cc84h		;d223
	ld c,000h		;d226
	jp nz,ld272h		;d228
	call sub_cc3eh		;d22b
	ld (ld5dbh),a		;d22e
	ld bc,00000h		;d231
	or a			;d234
	jp z,ld23fh		;d235
	ld c,a			;d238
	dec bc			;d239
	call sub_cc5eh		;d23a
	ld b,h			;d23d
	ld c,l			;d23e
ld23fh:
	call sub_cfc2h		;d23f
	ld a,l			;d242
	or h			;d243
	jp nz,ld24ch		;d244
	ld a,002h		;d247
	jp lcb01h		;d249
ld24ch:
	ld (ld5e9h),hl		;d24c
	ex de,hl		;d24f
	ld hl,(lcb43h)		;d250
	ld bc,00010h		;d253
	add hl,bc		;d256
	ld a,(ld5e1h)		;d257
	or a			;d25a
	ld a,(ld5dbh)		;d25b
	jp z,ld268h		;d25e
	call sub_cd64h		;d261
	ld (hl),e		;d264
	jp ld270h		;d265
ld268h:
	ld c,a			;d268
	ld b,000h		;d269
	add hl,bc		;d26b
	add hl,bc		;d26c
	ld (hl),e		;d26d
	inc hl			;d26e
	ld (hl),d		;d26f
ld270h:
	ld c,002h		;d270
ld272h:
	ld a,(lcb45h)		;d272
	or a			;d275
	ret nz			;d276
	push bc			;d277
	call sub_cc8ah		;d278
	ld a,(ld5d9h)		;d27b
	dec a			;d27e
	dec a			;d27f
	jp nz,ld2bfh		;d280
	pop bc			;d283
	push bc			;d284
	ld a,c			;d285
	dec a			;d286
	dec a			;d287
	jp nz,ld2bfh		;d288
	push hl			;d28b
	ld hl,(ld5bdh)		;d28c
	ld d,a			;d28f
ld290h:
	ld (hl),a		;d290
	inc hl			;d291
	inc d			;d292
	jp p,ld290h		;d293
	call sub_cde0h		;d296
	ld hl,(ld5ebh)		;d299
	ld c,002h		;d29c
ld29eh:
	ld (ld5e9h),hl		;d29e
	push bc			;d2a1
	call sub_cbd1h		;d2a2
	pop bc			;d2a5
	call sub_cbb8h		;d2a6
	ld hl,(ld5e9h)		;d2a9
	ld c,000h		;d2ac
	ld a,(ld5c8h)		;d2ae
	ld b,a			;d2b1
	and l			;d2b2
	cp b			;d2b3
	inc hl			;d2b4
	jp nz,ld29eh		;d2b5
	pop hl			;d2b8
	ld (ld5e9h),hl		;d2b9
	call lcddah		;d2bc
ld2bfh:
	call sub_cbd1h		;d2bf
	pop bc			;d2c2
	push bc			;d2c3
	call sub_cbb8h		;d2c4
	pop bc			;d2c7
	ld a,(ld5e7h)		;d2c8
	ld hl,ld5e5h		;d2cb
	cp (hl)			;d2ce
	jp c,ld2d6h		;d2cf
	ld (hl),a		;d2d2
	inc (hl)		;d2d3
	ld c,002h		;d2d4
ld2d6h:
	nop			;d2d6
	nop			;d2d7
	nop			;d2d8
	nop			;d2d9
	nop			;d2da
	push af			;d2db
	call sub_cd69h		;d2dc
	and 07fh		;d2df
	ld (hl),a		;d2e1
	pop af			;d2e2
	cp 07fh			;d2e3
	jp nz,ld304h		;d2e5
	ld a,(ld5d9h)		;d2e8
	cp 001h			;d2eb
	jp nz,ld304h		;d2ed
	call lccd2h		;d2f0
	call sub_d15eh		;d2f3
	ld hl,lcb45h		;d2f6
	ld a,(hl)		;d2f9
	or a			;d2fa
	jp nz,ld302h		;d2fb
	dec a			;d2fe
	ld (ld5e7h),a		;d2ff
ld302h:
	ld (hl),000h		;d302
ld304h:
	jp lccd2h		;d304
sub_d307h:
	xor a			;d307
	ld (ld5d9h),a		;d308
sub_d30bh:
	push bc			;d30b
	ld hl,(lcb43h)		;d30c
	ex de,hl		;d30f
	ld hl,00021h		;d310
	add hl,de		;d313
	ld a,(hl)		;d314
	and 07fh		;d315
	push af			;d317
	ld a,(hl)		;d318
	rla			;d319
	inc hl			;d31a
	ld a,(hl)		;d31b
	rla			;d31c
	and 01fh		;d31d
	ld c,a			;d31f
	ld a,(hl)		;d320
	rra			;d321
	rra			;d322
	rra			;d323
	rra			;d324
	and 00fh		;d325
	ld b,a			;d327
	pop af			;d328
	inc hl			;d329
	ld l,(hl)		;d32a
	inc l			;d32b
	dec l			;d32c
	ld l,006h		;d32d
	jp nz,ld38fh		;d32f
	ld hl,00020h		;d332
	add hl,de		;d335
	ld (hl),a		;d336
	ld hl,0000ch		;d337
	add hl,de		;d33a
	ld a,c			;d33b
	sub (hl)		;d33c
	jp nz,ld34bh		;d33d
	ld hl,0000eh		;d340
	add hl,de		;d343
	ld a,b			;d344
	sub (hl)		;d345
	and 07fh		;d346
	jp z,ld383h		;d348
ld34bh:
	push bc			;d34b
	push de			;d34c
	call sub_d0a6h		;d34d
	pop de			;d350
	pop bc			;d351
	ld l,003h		;d352
	ld a,(lcb45h)		;d354
	inc a			;d357
	jp z,ld388h		;d358
	ld hl,0000ch		;d35b
	add hl,de		;d35e
	ld (hl),c		;d35f
	ld hl,0000eh		;d360
	add hl,de		;d363
	ld (hl),b		;d364
	call sub_d055h		;d365
	ld a,(lcb45h)		;d368
	inc a			;d36b
	jp nz,ld383h		;d36c
	pop bc			;d36f
	push bc			;d370
	ld l,004h		;d371
	inc c			;d373
	jp z,ld388h		;d374
	call sub_d128h		;d377
	ld l,005h		;d37a
	ld a,(lcb45h)		;d37c
	inc a			;d37f
	jp z,ld388h		;d380
ld383h:
	pop bc			;d383
	xor a			;d384
	jp lcb01h		;d385
ld388h:
	push hl			;d388
	call sub_cd69h		;d389
	ld (hl),0c0h		;d38c
	pop hl			;d38e
ld38fh:
	pop bc			;d38f
	ld a,l			;d390
	ld (lcb45h),a		;d391
	jp sub_cd78h		;d394
ld397h:
	ld c,0ffh		;d397
	call sub_d307h		;d399
	call z,sub_d1c5h	;d39c
	ret			;d39f
ld3a0h:
	ld c,000h		;d3a0
	call sub_d307h		;d3a2
	call z,sub_d207h	;d3a5
	ret			;d3a8
sub_d3a9h:
	ex de,hl		;d3a9
	add hl,de		;d3aa
	ld c,(hl)		;d3ab
	ld b,000h		;d3ac
	ld hl,0000ch		;d3ae
	add hl,de		;d3b1
	ld a,(hl)		;d3b2
	rrca			;d3b3
	and 080h		;d3b4
	add a,c			;d3b6
	ld c,a			;d3b7
	ld a,000h		;d3b8
	adc a,b			;d3ba
	ld b,a			;d3bb
	ld a,(hl)		;d3bc
	rrca			;d3bd
	and 00fh		;d3be
	add a,b			;d3c0
	ld b,a			;d3c1
	ld hl,0000eh		;d3c2
	add hl,de		;d3c5
	ld a,(hl)		;d3c6
	add a,a			;d3c7
	add a,a			;d3c8
	add a,a			;d3c9
	add a,a			;d3ca
	push af			;d3cb
	add a,b			;d3cc
	ld b,a			;d3cd
	push af			;d3ce
	pop hl			;d3cf
	ld a,l			;d3d0
	pop hl			;d3d1
	or l			;d3d2
	and 001h		;d3d3
	ret			;d3d5
ld3d6h:
	ld c,00ch		;d3d6
	call sub_cf1ch		;d3d8
	ld hl,(lcb43h)		;d3db
	ld de,00021h		;d3de
	add hl,de		;d3e1
	push hl			;d3e2
	ld (hl),d		;d3e3
	inc hl			;d3e4
	ld (hl),d		;d3e5
	inc hl			;d3e6
	ld (hl),d		;d3e7
ld3e8h:
	call sub_cdf5h		;d3e8
	jp z,ld410h		;d3eb
	call sub_cd5eh		;d3ee
	ld de,0000fh		;d3f1
	call sub_d3a9h		;d3f4
	pop hl			;d3f7
	push hl			;d3f8
	ld e,a			;d3f9
	ld a,c			;d3fa
	sub (hl)		;d3fb
	inc hl			;d3fc
	ld a,b			;d3fd
	sbc a,(hl)		;d3fe
	inc hl			;d3ff
	ld a,e			;d400
	sbc a,(hl)		;d401
	jp c,ld40ah		;d402
	ld (hl),e		;d405
	dec hl			;d406
	ld (hl),b		;d407
	dec hl			;d408
	ld (hl),c		;d409
ld40ah:
	call lcf31h		;d40a
	jp ld3e8h		;d40d
ld410h:
	pop hl			;d410
	ret			;d411
	ld hl,(lcb43h)		;d412
	ld de,00020h		;d415
	call sub_d3a9h		;d418
	ld hl,00021h		;d41b
	add hl,de		;d41e
	ld (hl),c		;d41f
	inc hl			;d420
	ld (hl),b		;d421
	inc hl			;d422
	ld (hl),a		;d423
	ret			;d424
ld425h:
	ld hl,(ld5b3h)		;d425
	ld a,(lcb42h)		;d428
	ld c,a			;d42b
	call sub_cceah		;d42c
	push hl			;d42f
	ex de,hl		;d430
	call sub_cb59h		;d431
	pop hl			;d434
	call z,sub_cb47h	;d435
	ld a,l			;d438
	rra			;d439
	ret c			;d43a
	ld hl,(ld5b3h)		;d43b
	ld c,l			;d43e
	ld b,h			;d43f
	call sub_cd0bh		;d440
	ld (ld5b3h),hl		;d443
	jp lcea3h		;d446
sub_d449h:
	ld a,(ld5dah)		;d449
	ld hl,lcb42h		;d44c
	cp (hl)			;d44f
	ret z			;d450
	ld (hl),a		;d451
	jp ld425h		;d452
sub_d455h:
	ld a,0ffh		;d455
	ld (ld5e2h),a		;d457
	ld hl,(lcb43h)		;d45a
	ld a,(hl)		;d45d
	and 01fh		;d45e
	dec a			;d460
	ld (ld5dah),a		;d461
	cp 01eh			;d464
	jp nc,ld479h		;d466
	ld a,(lcb42h)		;d469
	ld (ld5e3h),a		;d46c
	ld a,(hl)		;d46f
	ld (ld5e4h),a		;d470
	and 0e0h		;d473
	ld (hl),a		;d475
	call sub_d449h		;d476
ld479h:
	ld a,(lcb41h)		;d479
	ld hl,(lcb43h)		;d47c
	or (hl)			;d47f
	ld (hl),a		;d480
	ret			;d481
	ld a,0c0h		;d482
	jp lcb01h		;d484
sub_d487h:
	ld hl,00000h		;d487
	ld (ld5b1h),hl		;d48a
	ld (ld5b3h),hl		;d48d
	xor a			;d490
	ld (lcb42h),a		;d491
	ld hl,00080h		;d494
	ld (ld5b5h),hl		;d497
	call lcddah		;d49a
	jp ld425h		;d49d
sub_d4a0h:
	call sub_cd72h		;d4a0
	call sub_d455h		;d4a3
	jp sub_d055h		;d4a6
	call sub_d455h		;d4a9
	jp sub_d0a6h		;d4ac
sub_d4afh:
	ld c,000h		;d4af
	ex de,hl		;d4b1
	ld a,(hl)		;d4b2
	cp 03fh			;d4b3
	jp z,ld4c6h		;d4b5
	call sub_cca6h		;d4b8
	ld a,(hl)		;d4bb
	cp 03fh			;d4bc
	call nz,sub_cd72h	;d4be
	call sub_d455h		;d4c1
	ld c,00fh		;d4c4
ld4c6h:
	call sub_cf1ch		;d4c6
	jp lcde9h		;d4c9
	ld hl,(ld5ddh)		;d4cc
	ld (lcb43h),hl		;d4cf
	call sub_d455h		;d4d2
	call lcf31h		;d4d5
	jp lcde9h		;d4d8
	call sub_d455h		;d4db
	call sub_cfa0h		;d4de
	jp lcf05h		;d4e1
sub_d4e4h:
	call sub_d455h		;d4e4
	jp ld1c0h		;d4e7
	call sub_d455h		;d4ea
	jp ld202h		;d4ed
	call sub_cd72h		;d4f0
	call sub_d455h		;d4f3
	jp sub_d128h		;d4f6
sub_d4f9h:
	call sub_d455h		;d4f9
	call sub_d01ah		;d4fc
	jp lcf05h		;d4ff
	ld hl,(ld5b3h)		;d502
	jp ld52dh		;d505
	ld a,(lcb42h)		;d508
	jp lcb01h		;d50b
	ex de,hl		;d50e
	ld (ld5b5h),hl		;d50f
	jp lcddah		;d512
	ld hl,(ld5c3h)		;d515
	jp ld52dh		;d518
sub_d51bh:
	ld hl,(ld5b1h)		;d51b
	jp ld52dh		;d51e
	call sub_d455h		;d521
	call sub_d03fh		;d524
	jp lcf05h		;d527
	ld hl,(ld5bfh)		;d52a
ld52dh:
	ld (lcb45h),hl		;d52d
	ret			;d530
	ld a,(ld5dah)		;d531
	cp 0ffh			;d534
	jp nz,ld53fh		;d536
	ld a,(lcb41h)		;d539
	jp lcb01h		;d53c
ld53fh:
	and 01fh		;d53f
	ld (lcb41h),a		;d541
	ret			;d544
	call sub_d455h		;d545
	jp ld397h		;d548
	call sub_d455h		;d54b
	jp ld3a0h		;d54e
	call sub_d455h		;d551
	jp ld3d6h		;d554
sub_d557h:
	ld hl,(lcb43h)		;d557
	ld a,l			;d55a
	cpl			;d55b
	ld e,a			;d55c
	ld a,h			;d55d
	cpl			;d55e
	ld hl,(ld5b3h)		;d55f
	and h			;d562
	ld d,a			;d563
	ld a,l			;d564
	and e			;d565
	ld e,a			;d566
	ld hl,(ld5b1h)		;d567
	ex de,hl		;d56a
	ld (ld5b3h),hl		;d56b
	ld a,l			;d56e
	and e			;d56f
	ld l,a			;d570
	ld a,h			;d571
	and d			;d572
	ld h,a			;d573
	ld (ld5b1h),hl		;d574
	ret			;d577
ld578h:
	ld a,(ld5e2h)		;d578
	or a			;d57b
	jp z,ld595h		;d57c
	ld hl,(lcb43h)		;d57f
	ld (hl),000h		;d582
	ld a,(ld5e4h)		;d584
	or a			;d587
	jp z,ld595h		;d588
	ld (hl),a		;d58b
	ld a,(ld5e3h)		;d58c
	ld (ld5dah),a		;d58f
	call sub_d449h		;d592
ld595h:
	ld hl,(lcb0fh)		;d595
	ld sp,hl		;d598
	ld hl,(lcb45h)		;d599
	ld a,l			;d59c
	ld b,h			;d59d
	ret			;d59e
	call sub_d455h		;d59f
	ld a,002h		;d5a2
	ld (ld5d9h),a		;d5a4
	ld c,000h		;d5a7
	call sub_d30bh		;d5a9
	call z,sub_d207h	;d5ac
	ret			;d5af
ld5b0h:
	push hl			;d5b0
ld5b1h:
	nop			;d5b1
	nop			;d5b2
ld5b3h:
	nop			;d5b3
	nop			;d5b4
ld5b5h:
	add a,b			;d5b5
	nop			;d5b6
ld5b7h:
	nop			;d5b7
	nop			;d5b8
ld5b9h:
	nop			;d5b9
	nop			;d5ba
ld5bbh:
	nop			;d5bb
	nop			;d5bc
ld5bdh:
	nop			;d5bd
	nop			;d5be
ld5bfh:
	nop			;d5bf
	nop			;d5c0
ld5c1h:
	nop			;d5c1
	nop			;d5c2
ld5c3h:
	nop			;d5c3
	nop			;d5c4
ld5c5h:
	nop			;d5c5
	nop			;d5c6
ld5c7h:
	nop			;d5c7
ld5c8h:
	nop			;d5c8
ld5c9h:
	nop			;d5c9
ld5cah:
	nop			;d5ca
	nop			;d5cb
ld5cch:
	nop			;d5cc
	nop			;d5cd
ld5ceh:
	nop			;d5ce
	nop			;d5cf
ld5d0h:
	nop			;d5d0
	nop			;d5d1
ld5d2h:
	nop			;d5d2
	nop			;d5d3
ld5d4h:
	nop			;d5d4
	nop			;d5d5
ld5d6h:
	nop			;d5d6
ld5d7h:
	nop			;d5d7
ld5d8h:
	nop			;d5d8
ld5d9h:
	nop			;d5d9
ld5dah:
	nop			;d5da
ld5dbh:
	nop			;d5db
ld5dch:
	nop			;d5dc
ld5ddh:
	nop			;d5dd
	nop			;d5de
	nop			;d5df
	nop			;d5e0
ld5e1h:
	nop			;d5e1
ld5e2h:
	nop			;d5e2
ld5e3h:
	nop			;d5e3
ld5e4h:
	nop			;d5e4
ld5e5h:
	nop			;d5e5
ld5e6h:
	nop			;d5e6
ld5e7h:
	nop			;d5e7
	nop			;d5e8
ld5e9h:
	nop			;d5e9
	nop			;d5ea
ld5ebh:
	nop			;d5eb
	nop			;d5ec
ld5edh:
	nop			;d5ed
ld5eeh:
	nop			;d5ee
ld5efh:
	nop			;d5ef
ld5f0h:
	nop			;d5f0
	nop			;d5f1
	nop			;d5f2
	nop			;d5f3
	nop			;d5f4
	nop			;d5f5
	nop			;d5f6
	nop			;d5f7
	nop			;d5f8
	nop			;d5f9
	nop			;d5fa
	nop			;d5fb
	nop			;d5fc
	nop			;d5fd
	nop			;d5fe
	nop			;d5ff
;BIOS jump table (must stay fixed)
bios:

