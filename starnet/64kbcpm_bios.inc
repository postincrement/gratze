;; 64kbcpm original BIOS — $D600-$DFFF
; BLOCK 'bios_jt' (start 0xd600 end 0xd640)
bios_jt_start:
	jp bios_cold		;d600
bios_wboot:
	jp ld862h		;d603
bios_const:
	jp 0e04eh		;d606
bios_conin:
	jp ld99ah		;d609
bios_conout:
	jp rom_conout		;d60c
bios_list:
	jp ld8f3h		;d60f
	jp ld98ah		;d612
sub_d615h:
	jp ld98fh		;d615
bios_home:
	jp ld9b5h		;d618
bios_seldsk:
	jp ld9cah		;d61b
bios_settrk:
	jp ld9b8h		;d61e
bios_setsec:
	jp ld9bdh		;d621
bios_setdma:
	jp ld9c2h		;d624
bios_read:
	jp ld9ffh		;d627
bios_write:
	jp lda24h		;d62a
bios_listst:
	jp ld96ch		;d62d
bios_sectran:
	jp ld9c7h		;d630
	dec e			;d633
	jp ld86eh		;d634
	jp 0e03ch		;d637
	jp 0e03fh		;d63a
	jp rom_net_io		;d63d

; BLOCK 'bios_dpb0' (start 0xd640 end 0xd682)
bios_dpb0_start:
	defb 000h		;d640
ld641h:
	defb 000h		;d641
	defb 06bh		;d642
	defb 050h		;d643
	defb 057h		;d644
	defb 037h		;d645
	defb 01bh		;d646
	defb 005h		;d647
	defb 018h		;d648
	defb 01ah		;d649
	defb 048h		;d64a
	defb 00ah		;d64b
	defb 02ah		;d64c
	defb 00ah		;d64d
	defb 020h		;d64e
	defb 000h		;d64f
	defb 000h		;d650
ld651h:
	defb 000h		;d651
	defb 000h		;d652
	defb 000h		;d653
	defb 000h		;d654
	defb 000h		;d655
	defb 000h		;d656
	defb 000h		;d657
	defb 000h		;d658
	defb 000h		;d659
	defb 000h		;d65a
	defb 000h		;d65b
	defb 000h		;d65c
	defb 000h		;d65d
	defb 000h		;d65e
	defb 000h		;d65f
	defb 000h		;d660
	defb 000h		;d661
	defb 000h		;d662
	defb 000h		;d663
	defb 000h		;d664
	defb 000h		;d665
	defb 000h		;d666
	defb 000h		;d667
	defb 000h		;d668
	defb 000h		;d669
	defb 000h		;d66a
	defb 000h		;d66b
	defb 000h		;d66c
	defb 000h		;d66d
	defb 000h		;d66e
	defb 000h		;d66f
	defb 000h		;d670
	defb 000h		;d671
	defb 000h		;d672
	defb 000h		;d673
	defb 000h		;d674
	defb 000h		;d675
	defb 000h		;d676
	defb 000h		;d677
	defb 000h		;d678
	defb 000h		;d679
	defb 000h		;d67a
	defb 000h		;d67b
	defb 000h		;d67c
	defb 000h		;d67d
	defb 000h		;d67e
	defb 000h		;d67f
	defb 000h		;d680
	defb 000h		;d681
;CCP/BIOS shared command buffer — address fixed; CCP loads SP/$D684 here.
	org 0d682h
ccp_sp:

; BLOCK 'ccp_cmdbuf' (start 0xd682 end 0xd706)
ccp_cmdbuf_start:
	defb 086h		;d682
	defb 0d6h		;d683
ccp_cmdline:
	defb 07eh		;d684
ccp_cmdlen:
	defb 000h		;d685
ccp_cmdtext:
	defb 000h		;d686
	defb 000h		;d687
	defb 000h		;d688
	defb 000h		;d689
	defb 000h		;d68a
	defb 000h		;d68b
	defb 000h		;d68c
	defb 000h		;d68d
	defb 000h		;d68e
	defb 000h		;d68f
	defb 000h		;d690
	defb 000h		;d691
	defb 000h		;d692
	defb 000h		;d693
	defb 000h		;d694
	defb 000h		;d695
	defb 000h		;d696
	defb 000h		;d697
	defb 000h		;d698
	defb 000h		;d699
	defb 000h		;d69a
	defb 000h		;d69b
	defb 000h		;d69c
	defb 000h		;d69d
	defb 000h		;d69e
	defb 000h		;d69f
	defb 000h		;d6a0
	defb 000h		;d6a1
	defb 000h		;d6a2
	defb 000h		;d6a3
	defb 000h		;d6a4
	defb 000h		;d6a5
	defb 000h		;d6a6
	defb 000h		;d6a7
	defb 000h		;d6a8
	defb 000h		;d6a9
	defb 000h		;d6aa
	defb 000h		;d6ab
	defb 000h		;d6ac
	defb 000h		;d6ad
	defb 000h		;d6ae
	defb 000h		;d6af
	defb 000h		;d6b0
	defb 000h		;d6b1
	defb 000h		;d6b2
	defb 000h		;d6b3
	defb 000h		;d6b4
	defb 000h		;d6b5
	defb 000h		;d6b6
	defb 000h		;d6b7
	defb 000h		;d6b8
	defb 000h		;d6b9
	defb 000h		;d6ba
	defb 000h		;d6bb
	defb 000h		;d6bc
	defb 000h		;d6bd
	defb 000h		;d6be
	defb 000h		;d6bf
	defb 000h		;d6c0
	defb 000h		;d6c1
	defb 000h		;d6c2
	defb 000h		;d6c3
	defb 000h		;d6c4
	defb 000h		;d6c5
	defb 000h		;d6c6
	defb 000h		;d6c7
	defb 000h		;d6c8
	defb 000h		;d6c9
	defb 000h		;d6ca
	defb 000h		;d6cb
	defb 000h		;d6cc
	defb 000h		;d6cd
	defb 000h		;d6ce
	defb 000h		;d6cf
	defb 000h		;d6d0
	defb 000h		;d6d1
	defb 000h		;d6d2
	defb 000h		;d6d3
	defb 000h		;d6d4
	defb 000h		;d6d5
	defb 000h		;d6d6
	defb 000h		;d6d7
	defb 000h		;d6d8
	defb 000h		;d6d9
	defb 000h		;d6da
	defb 000h		;d6db
	defb 000h		;d6dc
	defb 000h		;d6dd
	defb 000h		;d6de
	defb 000h		;d6df
	defb 000h		;d6e0
	defb 000h		;d6e1
	defb 000h		;d6e2
	defb 000h		;d6e3
	defb 000h		;d6e4
	defb 000h		;d6e5
	defb 000h		;d6e6
	defb 000h		;d6e7
	defb 000h		;d6e8
	defb 000h		;d6e9
	defb 000h		;d6ea
	defb 000h		;d6eb
	defb 000h		;d6ec
	defb 000h		;d6ed
	defb 000h		;d6ee
	defb 000h		;d6ef
	defb 000h		;d6f0
	defb 000h		;d6f1
	defb 000h		;d6f2
	defb 000h		;d6f3
	defb 000h		;d6f4
	defb 000h		;d6f5
	defb 000h		;d6f6
	defb 000h		;d6f7
	defb 000h		;d6f8
	defb 000h		;d6f9
	defb 000h		;d6fa
	defb 000h		;d6fb
	defb 000h		;d6fc
	defb 000h		;d6fd
	defb 000h		;d6fe
	defb 000h		;d6ff
	defb 000h		;d700
	defb 000h		;d701
	defb 000h		;d702
	defb 000h		;d703
	defb 000h		;d704
	defb 000h		;d705
;DPB table (server cold-boot packet; m_dpbOffset 0x1e00)
dpb_table:

; BLOCK 'bios_dpb' (start 0xd706 end 0xd760)
bios_dpb_start:
	defb 000h		;d706
	defb 000h		;d707
	defb 000h		;d708
	defb 000h		;d709
	defb 000h		;d70a
	defb 000h		;d70b
	defb 000h		;d70c
	defb 000h		;d70d
	defb 000h		;d70e
	defb 000h		;d70f
	defb 000h		;d710
	defb 000h		;d711
	defb 000h		;d712
	defb 000h		;d713
	defb 000h		;d714
	defb 000h		;d715
	defb 000h		;d716
	defb 000h		;d717
	defb 000h		;d718
	defb 000h		;d719
	defb 000h		;d71a
	defb 000h		;d71b
	defb 000h		;d71c
	defb 000h		;d71d
	defb 000h		;d71e
	defb 000h		;d71f
	defb 000h		;d720
	defb 000h		;d721
	defb 000h		;d722
	defb 000h		;d723
	defb 000h		;d724
	defb 000h		;d725
	defb 000h		;d726
	defb 000h		;d727
	defb 000h		;d728
	defb 000h		;d729
	defb 000h		;d72a
	defb 000h		;d72b
	defb 000h		;d72c
	defb 000h		;d72d
	defb 000h		;d72e
	defb 000h		;d72f
	defb 000h		;d730
	defb 000h		;d731
	defb 000h		;d732
	defb 000h		;d733
	defb 000h		;d734
	defb 000h		;d735
	defb 000h		;d736
	defb 000h		;d737
	defb 000h		;d738
	defb 000h		;d739
	defb 000h		;d73a
	defb 000h		;d73b
	defb 000h		;d73c
	defb 000h		;d73d
	defb 000h		;d73e
	defb 000h		;d73f
	defb 000h		;d740
	defb 000h		;d741
	defb 000h		;d742
	defb 000h		;d743
	defb 000h		;d744
	defb 000h		;d745
	defb 000h		;d746
	defb 000h		;d747
	defb 000h		;d748
	defb 000h		;d749
	defb 000h		;d74a
	defb 000h		;d74b
	defb 000h		;d74c
	defb 000h		;d74d
	defb 000h		;d74e
	defb 000h		;d74f
	defb 000h		;d750
	defb 000h		;d751
	defb 000h		;d752
	defb 000h		;d753
	defb 000h		;d754
	defb 000h		;d755
	defb 000h		;d756
	defb 000h		;d757
	defb 000h		;d758
	defb 000h		;d759
	defb 000h		;d75a
	defb 000h		;d75b
	defb 000h		;d75c
	defb 000h		;d75d
	defb 000h		;d75e
	defb 000h		;d75f

; BLOCK 'bios_spare0' (start 0xd760 end 0xd78d)
bios_spare0_start:
	defb 000h		;d760
	defb 000h		;d761
	defb 000h		;d762
	defb 000h		;d763
	defb 000h		;d764
	defb 000h		;d765
	defb 000h		;d766
	defb 000h		;d767
	defb 000h		;d768
	defb 000h		;d769
	defb 000h		;d76a
	defb 000h		;d76b
	defb 000h		;d76c
	defb 000h		;d76d
	defb 000h		;d76e
	defb 000h		;d76f
	defb 001h		;d770
	defb 004h		;d771
	defb 00fh		;d772
	defb 000h		;d773
	defb 00fh		;d774
	defb 000h		;d775
	defb 0efh		;d776
	defb 000h		;d777
	defb 0f0h		;d778
	defb 000h		;d779
	defb 000h		;d77a
	defb 000h		;d77b
	defb 000h		;d77c
	defb 000h		;d77d
	defb 000h		;d77e
	defb 001h		;d77f
	defb 004h		;d780
	defb 00fh		;d781
	defb 000h		;d782
	defb 00fh		;d783
	defb 000h		;d784
	defb 0efh		;d785
	defb 000h		;d786
	defb 0f0h		;d787
	defb 000h		;d788
	defb 000h		;d789
	defb 000h		;d78a
	defb 000h		;d78b
	defb 000h		;d78c
; Private stack / bank-$14 helpers (former DPB hole)
;DPH table
dph_table:

; BLOCK 'bios_dph' (start 0xd78d end 0xd7ed)
bios_dph_start:
	defb 000h		;d78d
	defb 000h		;d78e
	defb 000h		;d78f
	defb 000h		;d790
	defb 000h		;d791
	defb 000h		;d792
	defb 000h		;d793
	defb 000h		;d794
	defb 0dch		;d795
	defb 0dch		;d796
	defb 006h		;d797
	defb 0d7h		;d798
	defb 000h		;d799
	defb 000h		;d79a
	defb 06eh		;d79b
	defb 0ddh		;d79c
	defb 000h		;d79d
	defb 000h		;d79e
	defb 000h		;d79f
	defb 000h		;d7a0
	defb 000h		;d7a1
	defb 000h		;d7a2
	defb 000h		;d7a3
	defb 000h		;d7a4
	defb 0dch		;d7a5
	defb 0dch		;d7a6
	defb 015h		;d7a7
	defb 0d7h		;d7a8
	defb 000h		;d7a9
	defb 000h		;d7aa
	defb 06eh		;d7ab
	defb 0ddh		;d7ac
	defb 000h		;d7ad
	defb 000h		;d7ae
	defb 000h		;d7af
	defb 000h		;d7b0
	defb 000h		;d7b1
	defb 000h		;d7b2
	defb 000h		;d7b3
	defb 000h		;d7b4
	defb 0dch		;d7b5
	defb 0dch		;d7b6
	defb 024h		;d7b7
	defb 0d7h		;d7b8
	defb 000h		;d7b9
	defb 000h		;d7ba
	defb 06eh		;d7bb
	defb 0ddh		;d7bc
	defb 000h		;d7bd
	defb 000h		;d7be
	defb 000h		;d7bf
	defb 000h		;d7c0
	defb 000h		;d7c1
	defb 000h		;d7c2
	defb 000h		;d7c3
	defb 000h		;d7c4
	defb 0dch		;d7c5
	defb 0dch		;d7c6
	defb 033h		;d7c7
	defb 0d7h		;d7c8
	defb 000h		;d7c9
	defb 000h		;d7ca
	defb 06eh		;d7cb
	defb 0ddh		;d7cc
	defb 000h		;d7cd
	defb 000h		;d7ce
	defb 000h		;d7cf
	defb 000h		;d7d0
	defb 000h		;d7d1
	defb 000h		;d7d2
	defb 000h		;d7d3
	defb 000h		;d7d4
	defb 0dch		;d7d5
	defb 0dch		;d7d6
	defb 042h		;d7d7
	defb 0d7h		;d7d8
	defb 000h		;d7d9
	defb 000h		;d7da
	defb 049h		;d7db
	defb 0dch		;d7dc
	defb 000h		;d7dd
	defb 000h		;d7de
	defb 000h		;d7df
	defb 000h		;d7e0
	defb 000h		;d7e1
	defb 000h		;d7e2
	defb 000h		;d7e3
	defb 000h		;d7e4
	defb 0dch		;d7e5
	defb 0dch		;d7e6
	defb 051h		;d7e7
	defb 0d7h		;d7e8
	defb 000h		;d7e9
	defb 000h		;d7ea
	defb 062h		;d7eb
	defb 0dch		;d7ec

; BLOCK 'bios_spare1' (start 0xd7ed end 0xd852)
bios_spare1_start:
	defb 000h		;d7ed
	defb 000h		;d7ee
	defb 000h		;d7ef
	defb 000h		;d7f0
	defb 000h		;d7f1
	defb 000h		;d7f2
	defb 000h		;d7f3
	defb 000h		;d7f4
	defb 0dch		;d7f5
	defb 0dch		;d7f6
	defb 060h		;d7f7
	defb 0d7h		;d7f8
	defb 000h		;d7f9
	defb 000h		;d7fa
	defb 07bh		;d7fb
	defb 0dch		;d7fc
	defb 000h		;d7fd
	defb 000h		;d7fe
	defb 000h		;d7ff
	defb 000h		;d800
	defb 000h		;d801
	defb 000h		;d802
	defb 000h		;d803
	defb 000h		;d804
	defb 0dch		;d805
	defb 0dch		;d806
	defb 07eh		;d807
	defb 0d7h		;d808
	defb 000h		;d809
	defb 000h		;d80a
	defb 094h		;d80b
	defb 0dch		;d80c
	defb 000h		;d80d
	defb 000h		;d80e
	defb 000h		;d80f
	defb 000h		;d810
	defb 000h		;d811
	defb 000h		;d812
	defb 000h		;d813
	defb 000h		;d814
	defb 0dch		;d815
	defb 0dch		;d816
	defb 06fh		;d817
	defb 0d7h		;d818
	defb 000h		;d819
	defb 000h		;d81a
	defb 0cfh		;d81b
	defb 0deh		;d81c
ld81dh:
	defb 024h		;d81d
	defb 024h		;d81e
	defb 001h		;d81f
	defb 000h		;d820
	defb 000h		;d821
	defb 000h		;d822
	defb 000h		;d823
	defb 000h		;d824
	defb 000h		;d825
	defb 000h		;d826
	defb 000h		;d827
	defb 000h		;d828
	defb 000h		;d829
	defb 000h		;d82a
	defb 000h		;d82b
	defb 000h		;d82c
ld82dh:
	defb 000h		;d82d
	defb 000h		;d82e
	defb 000h		;d82f
	defb 000h		;d830
	defb 000h		;d831
	defb 000h		;d832
	defb 000h		;d833
	defb 000h		;d834
	defb 000h		;d835
	defb 000h		;d836
	defb 000h		;d837
	defb 000h		;d838
	defb 000h		;d839
	defb 000h		;d83a
	defb 000h		;d83b
	defb 000h		;d83c
	defb 000h		;d83d
	defb 000h		;d83e
	defb 000h		;d83f
	defb 000h		;d840
	defb 000h		;d841
	defb 000h		;d842
	defb 000h		;d843
	defb 000h		;d844
	defb 000h		;d845
	defb 000h		;d846
	defb 000h		;d847
	defb 000h		;d848
	defb 000h		;d849
	defb 000h		;d84a
	defb 000h		;d84b
	defb 000h		;d84c
	defb 000h		;d84d
	defb 000h		;d84e
	defb 000h		;d84f
	defb 000h		;d850
	defb 000h		;d851

; BLOCK 'bios_code' (start 0xd852 end 0xdcac)
bios_code_start:
	ld (bc),a		;d852
ld853h:
	ld bc,04000h		;d853
ld856h:
	ex af,af'		;d856
	ld bc,02824h		;d857
	inc b			;d85a
	nop			;d85b
	nop			;d85c
	nop			;d85d
	nop			;d85e
	nop			;d85f
	nop			;d860
ld861h:
	nop			;d861
ld862h:
	ld sp,ccp_sp		;d862
	ei			;d865
	call bank_video_off	;d866
	ld a,002h		;d869
	jp 0e039h		;d86b
ld86eh:
	call bank_normal	;d86e
	ld hl,(ccp_sp)		;d871
	ld a,(hl)		;d874
	or a			;d875
	jr nz,ld887h		;d876
	ld hl,ld82dh		;d878
	ld de,ccp_cmdtext	;d87b
	ld (ccp_sp),de		;d87e
	ld bc,00012h		;d882
	ldir			;d885
ld887h:
	ld sp,ccp_sp		;d887
	ld hl,ld651h		;d88a
	call 0e024h		;d88d
	ld hl,0f800h		;d890
	ld a,040h		;d893
	ld (hl),a		;d895
	out (008h),a		;d896
	xor a			;d898
	ld (hl),a		;d899
	out (008h),a		;d89a
	ld a,(hl)		;d89c
	or a			;d89d
	jr z,ld8b1h		;d89e
	out (008h),a		;d8a0
	ld hl,0f800h		;d8a2
ld8a5h:
	ld a,(bios_code_start)	;d8a5
	ld (hl),a		;d8a8
	inc hl			;d8a9
	bit 7,h			;d8aa
	jr nz,ld8a5h		;d8ac
	xor a			;d8ae
	out (008h),a		;d8af
ld8b1h:
	call 0e02ah		;d8b1
	ld a,0c3h		;d8b4
	ld (00000h),a		;d8b6
	ld hl,bios_wboot	;d8b9
	ld (00001h),hl		;d8bc
	ld (00005h),a		;d8bf
	ld hl,lc806h		;d8c2
	ld (00006h),hl		;d8c5
	ld (00028h),a		;d8c8
	ld hl,ldafah		;d8cb
	ld (00029h),hl		;d8ce
	ld hl,ld856h		;d8d1
	ld de,ldf71h		;d8d4
	ld bc,00005h		;d8d7
	ldir			;d8da
	ld bc,00080h		;d8dc
	ld (ldcdah),bc		;d8df
	ld hl,ldb8bh		;d8e3
	ld (lc80dh),hl		;d8e6
	ei			;d8e9
	ld a,(00004h)		;d8ea
	and 00fh		;d8ed
	ld c,a			;d8ef
	jp ccp			;d8f0
ld8f3h:
	ld a,(ld853h)		;d8f3
	or a			;d8f6
	jr z,ld935h		;d8f7
	ld a,(ldc47h)		;d8f9
	push af			;d8fc
	call bank_video_low	;d8fd
	ld hl,ld861h		;d900
	ld a,(hl)		;d903
	inc (hl)		;d904
	ld hl,0fa00h		;d905
	add a,l			;d908
	ld l,a			;d909
	jr nc,ld90dh		;d90a
	inc h			;d90c
ld90dh:
	ld (hl),c		;d90d
	ld a,(ld861h)		;d90e
	cp 07eh			;d911
	jr nz,ld92ch		;d913
	ld hl,0fa00h		;d915
	ld de,0fb0fh		;d918
	ld bc,0007eh		;d91b
	ldir			;d91e
	ld de,0fb8fh		;d920
	ld a,007h		;d923
	call sub_da6dh		;d925
	xor a			;d928
	ld (ld861h),a		;d929
ld92ch:
	pop af			;d92c
	di			;d92d
	out (050h),a		;d92e
	ld (ldc47h),a		;d930
	ei			;d933
	ret			;d934
ld935h:
	ld hl,ld853h+2		;d935
	bit 7,(hl)		;d938
	jr z,ld940h		;d93a
	ld a,c			;d93c
	cp 00ah			;d93d
	ret z			;d93f
ld940h:
	call sub_d961h		;d940
	bit 6,(hl)		;d943
	jp z,0e04bh		;d945
ld948h:
	call sub_d955h		;d948
	ld a,(ldc48h)		;d94b
	or a			;d94e
	jr nz,ld948h		;d94f
ld951h:
	ld a,c			;d951
	jp 0e04bh		;d952
sub_d955h:
	call 0e030h		;d955
	ret nz			;d958
	and 07fh		;d959
	sub 011h		;d95b
	ld (ldc48h),a		;d95d
	ret			;d960
sub_d961h:
	bit 0,(hl)		;d961
	push af			;d963
	call nz,0e045h		;d964
	pop af			;d967
	call z,0e048h		;d968
	ret			;d96b
ld96ch:
	ld a,(ld853h)		;d96c
	or a			;d96f
	ld a,0ffh		;d970
	ret nz			;d972
	ld hl,ld853h+2		;d973
	call sub_d961h		;d976
	bit 6,(hl)		;d979
	jp z,0e054h		;d97b
	call sub_d955h		;d97e
	ld a,(ldc48h)		;d981
	or a			;d984
	ld a,0ffh		;d985
	ret z			;d987
	inc a			;d988
	ret			;d989
ld98ah:
	call 0e048h		;d98a
	jr ld951h		;d98d
ld98fh:
	call 0e048h		;d98f
ld992h:
	call 0e030h		;d992
	jr nz,ld992h		;d995
	and 07fh		;d997
	ret			;d999
ld99ah:
	call 0e006h		;d99a
	and 07fh		;d99d
	cp 01eh			;d99f
	ret nz			;d9a1
	call bank_video_low	;d9a2
	ld a,(0f800h)		;d9a5
	cp 0aah			;d9a8
	call nz,0f800h		;d9aa
	call bank_normal	;d9ad
	jr nz,ld99ah		;d9b0
	ld a,01eh		;d9b2
	ret			;d9b4
ld9b5h:
	ld bc,00000h		;d9b5
ld9b8h:
	ld (ldcd6h),bc		;d9b8
	ret			;d9bc
ld9bdh:
	ld a,c			;d9bd
	ld (ldcd8h),a		;d9be
	ret			;d9c1
ld9c2h:
	ld (ldcdah),bc		;d9c2
	ret			;d9c6
ld9c7h:
	ld h,b			;d9c7
	ld l,c			;d9c8
	ret			;d9c9
ld9cah:
	ld a,c			;d9ca
	cp 00dh			;d9cb
	jr c,ld9e7h		;d9cd
	ld hl,ld641h		;d9cf
	ld c,007h		;d9d2
	cp 00eh			;d9d4
	jr z,ld9dfh		;d9d6
	bit 0,(hl)		;d9d8
	jr nz,ld9f0h		;d9da
	jp ldb96h		;d9dc
ld9dfh:
	inc c			;d9df
	bit 1,(hl)		;d9e0
	jr nz,ld9f0h		;d9e2
	jp ldb96h		;d9e4
ld9e7h:
	ld a,(bios_dpb0_start)	;d9e7
	dec a			;d9ea
	cp c			;d9eb
	ld a,c			;d9ec
	jp c,ldb9eh		;d9ed
ld9f0h:
	ld (ldcd5h),a		;d9f0
	ld h,000h		;d9f3
	ld l,c			;d9f5
	add hl,hl		;d9f6
	add hl,hl		;d9f7
	add hl,hl		;d9f8
	add hl,hl		;d9f9
	ld de,dph_table		;d9fa
	add hl,de		;d9fd
	ret			;d9fe
ld9ffh:
	call bank_video_off	;d9ff
	ld a,003h		;da02
	ld de,0fb0fh		;da04
	call sub_da6dh		;da07
	jr nz,lda21h		;da0a
	or a			;da0c
	ld a,0ffh		;da0d
	jr nz,lda21h		;da0f
	ld hl,(ldcdah)		;da11
	call bank_map_f000	;da14
	ex de,hl		;da17
	ld hl,0fa87h		;da18
	ld bc,00080h		;da1b
	ldir			;da1e
	xor a			;da20
lda21h:
	jp bank_normal		;da21
lda24h:
	call bank_video_off	;da24
	ld a,c			;da27
lda28h:
	push af			;da28
	ld (0fb0ah),a		;da29
	call sub_da57h		;da2c
	jr nz,lda52h		;da2f
	or a			;da31
	jr nz,lda38h		;da32
	pop af			;da34
	xor a			;da35
	jr lda21h		;da36
lda38h:
	cp 0e5h			;da38
	jp nz,ldb8bh		;da3a
	call sub_dab8h		;da3d
	ld a,009h		;da40
	ld de,0fb0fh		;da42
	call sub_da6dh		;da45
	jp nz,ldb8bh		;da48
	or a			;da4b
	jp nz,ldb8bh		;da4c
	pop af			;da4f
	jr lda28h		;da50
lda52h:
	pop af			;da52
	or 0ffh			;da53
	jr lda21h		;da55
sub_da57h:
	ld hl,(ldcdah)		;da57
	call bank_map_f000	;da5a
	ld de,0fb0fh		;da5d
	ld bc,00080h		;da60
	ldir			;da63
	call bank_video_off	;da65
	ld a,004h		;da68
	ld de,0fb8fh		;da6a
sub_da6dh:
	ld b,064h		;da6d
lda6fh:
	push af			;da6f
	push bc			;da70
	ld (0fb08h),a		;da71
	ld a,(ldcd5h)		;da74
	ld (0fb09h),a		;da77
	ld hl,(ldcd6h)		;da7a
	ld (0fb0bh),hl		;da7d
	ld hl,(ldcd8h)		;da80
	ld (0fb0dh),hl		;da83
	ex de,hl		;da86
	ld de,0fb08h		;da87
	call rom_net_io		;da8a
	ld hl,0fa80h		;da8d
	push hl			;da90
	call 0e03fh		;da91
	pop de			;da94
	call 0e03ch		;da95
	pop bc			;da98
	jr z,ldaabh		;da99
lda9bh:
	push bc			;da9b
	ld bc,00514h		;da9c
lda9fh:
	dec bc			;da9f
	ld a,b			;daa0
	or c			;daa1
	jr nz,lda9fh		;daa2
	pop bc			;daa4
	pop af			;daa5
	djnz lda6fh		;daa6
	or 0ffh			;daa8
	ret			;daaa
ldaabh:
	ld a,(0fa80h)		;daab
	cp 0ffh			;daae
	jr z,lda9bh		;dab0
	pop af			;dab2
	xor a			;dab3
	ld a,(0fa80h)		;dab4
	ret			;dab7
sub_dab8h:
	call bank_normal	;dab8
	ld c,007h		;dabb
	call rom_conout		;dabd
	ld hl,0f000h		;dac0
	push hl			;dac3
	ld de,0f780h		;dac4
	ld bc,00050h		;dac7
	ldir			;daca
	pop de			;dacc
	ld hl,ldc1ah		;dacd
ldad0h:
	ld a,(hl)		;dad0
	bit 7,a			;dad1
	set 7,a			;dad3
	ld (de),a		;dad5
	inc de			;dad6
	inc hl			;dad7
	jr z,ldad0h		;dad8
ldadah:
	call ld99ah		;dada
	res 5,a			;dadd
	cp 059h			;dadf
	jr z,ldae8h		;dae1
	cp 04eh			;dae3
	jr nz,ldadah		;dae5
	scf			;dae7
ldae8h:
	ld hl,0f780h		;dae8
	ld de,0f000h		;daeb
	ld bc,00050h		;daee
	ldir			;daf1
	call bank_video_off	;daf3
	jp c,ld862h		;daf6
	ret			;daf9
ldafah:
	cp 012h			;dafa
	ret nc			;dafc
	ld l,a			;dafd
	ld h,000h		;dafe
	add hl,hl		;db00
	ld de,ldb0ah		;db01
	add hl,de		;db04
	ld a,(hl)		;db05
	inc hl			;db06
	ld h,(hl)		;db07
	ld l,a			;db08
	jp (hl)			;db09
ldb0ah:
	scf			;db0a
	in a,(037h)		;db0b
	in a,(02ch)		;db0d
	in a,(038h)		;db0f
	in a,(03ah)		;db11
	in a,(03eh)		;db13
	in a,(037h)		;db15
	in a,(037h)		;db17
	in a,(043h)		;db19
	in a,(037h)		;db1b
	in a,(037h)		;db1d
	in a,(037h)		;db1f
	in a,(037h)		;db21
	in a,(037h)		;db23
	in a,(037h)		;db25
	in a,(047h)		;db27
	in a,(048h)		;db29
	in a,(03ah)		;db2b
	ld h,b			;db2d
	ret c			;db2e
	add a,007h		;db2f
	cp 007h			;db31
	ret nz			;db33
	ld a,(bios_dpb0_start)	;db34
	ret			;db37
	xor a			;db38
	ret			;db39
	ld hl,ldf48h		;db3a
	ret			;db3d
	ld hl,ccp_cmdtext	;db3e
	xor a			;db41
	ret			;db42
	ld a,(bios_dpb0_start)	;db43
	ret			;db46
	ret			;db47
	ret			;db48
;Translate F000 window then fall into bank_video_low
bank_map_f000:
	push hl			;db49
	ld de,0f000h		;db4a
	or a			;db4d
	sbc hl,de		;db4e
	pop hl			;db50
	ret c			;db51
	ld de,09000h		;db52
	add hl,de		;db55
;Bank select helpers
bank_video_low:
	ld a,014h		;db56
	di			;db58
	out (050h),a		;db59
	ld (ldc47h),a		;db5b
	ei			;db5e
	ret			;db5f
bank_video_off:
	push af			;db60
	ld a,00ch		;db61
	di			;db63
	out (050h),a		;db64
	ld (ldc47h),a		;db66
	ei			;db69
	pop af			;db6a
	ret			;db6b
bank_normal:
	push af			;db6c
	ld a,004h		;db6d
	di			;db6f
	out (050h),a		;db70
	ld (ldc47h),a		;db72
	ei			;db75
	pop af			;db76
	ret			;db77
sub_db78h:
	push de			;db78
	ld de,ldbb8h		;db79
	call sub_db80h		;db7c
	pop de			;db7f
sub_db80h:
	ld a,(de)		;db80
	cp 024h			;db81
	ret z			;db83
	inc de			;db84
	ld c,a			;db85
	call rom_conout		;db86
	jr sub_db80h		;db89
ldb8bh:
	ld a,(lcb42h)		;db8b
	ld hl,msg_err_write_x		;db8e
	ld de,ldbc7h		;db91
	jr ldba4h		;db94
ldb96h:
	ld hl,msg_err_nolog_x		;db96
	ld de,ldc01h		;db99
	jr ldba4h		;db9c
ldb9eh:
	ld hl,msg_err_nodrive_x		;db9e
	ld de,ldbe7h		;dba1
ldba4h:
	call bank_normal	;dba4
	add a,041h		;dba7
	ld (hl),a		;dba9
	call sub_db78h		;dbaa
	ld a,(00004h)		;dbad
	and 0f0h		;dbb0
	ld (00004h),a		;dbb2
	jp ld862h		;dbb5
; CP/M error messages ($ -terminated). 'x' sites are patched with the drive letter.
ldbb8h:
msg_cpm_error:
	defb 0dh, 0ah
	defm 'CP/M error: '
	defb '$'
ldbc7h:
msg_write_denied:
	defm 'Write permission to '
msg_err_write_x:
	defb 'x'
	defm ' denied.'
	defb 0dh, 0ah, '$'
ldbe7h:
msg_no_drive:
	defm 'Drive '
msg_err_nodrive_x:
	defb 'x'
	defm ' does not exist.'
	defb 0dh, 0ah, '$'
ldc01h:
msg_not_logged:
	defm 'Drive '
msg_err_nolog_x:
	defb 'x'
	defm ' not logged in.'
	defb 0dh, 0ah, '$'
ldc1ah:
msg_write_prompt:
	defm ' Write permission not held - Request (YorN)?'
	defb 0a0h			; soft space before latch bytes
; Current port-50 bank latch (byte) + pad — must remain at $DC47/$DC48
ldc47h:
	defb 000h
ldc48h:
	defb 000h
;Cold boot entry (BIOS $D600 target)
bios_cold:
	ld sp,ccp_sp		;dc49
	call bank_video_off	;dc4c
	ld a,0aah		;dc4f
	ld (0f800h),a		;dc51
ldc54h:
	call bank_normal	;dc54
	ld de,ldc7ah		;dc57
	call sub_db80h		;dc5a
	xor a			;dc5d
ldc5eh:
	ld (00004h),a		;dc5e
	ld (ld861h),a		;dc61
	ld (00003h),a		;dc64
	ld hl,ld81dh		;dc67
	ld de,00040h		;dc6a
	ld bc,00009h		;dc6d
	ldir			;dc70
	ld a,005h		;dc72
	ld (0e0e9h),a		;dc74
	jp ld887h		;dc77
; Cold-boot banner (printed until '$'). BSS is pinned at $DCAC below.
ldc7ah:
msg_cold_banner:
	defb 01ah, 0ah
	defm 'MicroBee Star Network v2.3'
	defb 0dh, 0ah
	defm 'CP/M Version 2.20'
	defb 0dh, 0ah, '$'
	defs 0dcach - $

	org 0dcach

; BLOCK 'bios_bss' (start 0xdcac end 0xe000)
bios_bss_start:
	defb 000h		;dcac
	defb 000h		;dcad
	defb 000h		;dcae
	defb 000h		;dcaf
	defb 000h		;dcb0
	defb 000h		;dcb1
	defb 000h		;dcb2
	defb 000h		;dcb3
	defb 000h		;dcb4
	defb 000h		;dcb5
	defb 000h		;dcb6
ldcb7h:
	defb 000h		;dcb7
	defb 000h		;dcb8
	defb 000h		;dcb9
	defb 000h		;dcba
	defb 000h		;dcbb
	defb 000h		;dcbc
	defb 000h		;dcbd
	defb 000h		;dcbe
	defb 000h		;dcbf
	defb 000h		;dcc0
	defb 000h		;dcc1
	defb 000h		;dcc2
	defb 000h		;dcc3
	defb 000h		;dcc4
	defb 000h		;dcc5
	defb 000h		;dcc6
	defb 000h		;dcc7
	defb 000h		;dcc8
	defb 000h		;dcc9
	defb 000h		;dcca
	defb 000h		;dccb
	defb 000h		;dccc
	defb 000h		;dccd
	defb 000h		;dcce
	defb 000h		;dccf
	defb 000h		;dcd0
	defb 000h		;dcd1
	defb 000h		;dcd2
	defb 000h		;dcd3
	defb 000h		;dcd4
ldcd5h:
	defb 000h		;dcd5
ldcd6h:
	defb 000h		;dcd6
	defb 000h		;dcd7
ldcd8h:
	defb 000h		;dcd8
ldcd9h:
	defb 000h		;dcd9
ldcdah:
	defb 000h		;dcda
	defb 000h		;dcdb
;DIRBUF
	org 0dcdch
dirbuf:
	defb 000h		;dcdc
	defb 000h		;dcdd
	defb 000h		;dcde
	defb 000h		;dcdf
	defb 000h		;dce0
	defb 000h		;dce1
	defb 000h		;dce2
	defb 000h		;dce3
	defb 000h		;dce4
	defb 000h		;dce5
	defb 000h		;dce6
	defb 000h		;dce7
	defb 000h		;dce8
	defb 000h		;dce9
	defb 000h		;dcea
	defb 000h		;dceb
	defb 000h		;dcec
	defb 000h		;dced
	defb 000h		;dcee
	defb 000h		;dcef
	defb 000h		;dcf0
	defb 000h		;dcf1
	defb 000h		;dcf2
	defb 000h		;dcf3
ldcf4h:
	defb 000h		;dcf4
	defb 000h		;dcf5
	defb 000h		;dcf6
	defb 000h		;dcf7
	defb 000h		;dcf8
	defb 000h		;dcf9
	defb 000h		;dcfa
	defb 000h		;dcfb
	defb 000h		;dcfc
	defb 000h		;dcfd
	defb 000h		;dcfe
	defb 000h		;dcff
	defb 000h		;dd00
	defb 000h		;dd01
	defb 000h		;dd02
	defb 000h		;dd03
	defb 000h		;dd04
	defb 000h		;dd05
	defb 000h		;dd06
	defb 000h		;dd07
	defb 000h		;dd08
ldd09h:
	defb 000h		;dd09
	defb 000h		;dd0a
	defb 000h		;dd0b
	defb 000h		;dd0c
	defb 000h		;dd0d
	defb 000h		;dd0e
	defb 000h		;dd0f
	defb 000h		;dd10
	defb 000h		;dd11
	defb 000h		;dd12
	defb 000h		;dd13
	defb 000h		;dd14
	defb 000h		;dd15
	defb 000h		;dd16
	defb 000h		;dd17
	defb 000h		;dd18
	defb 000h		;dd19
	defb 000h		;dd1a
	defb 000h		;dd1b
	defb 000h		;dd1c
	defb 000h		;dd1d
	defb 000h		;dd1e
	defb 000h		;dd1f
	defb 000h		;dd20
	defb 000h		;dd21
	defb 000h		;dd22
	defb 000h		;dd23
	defb 000h		;dd24
	defb 000h		;dd25
	defb 000h		;dd26
	defb 000h		;dd27
	defb 000h		;dd28
	defb 000h		;dd29
	defb 000h		;dd2a
	defb 000h		;dd2b
	defb 000h		;dd2c
	defb 000h		;dd2d
	defb 000h		;dd2e
	defb 000h		;dd2f
	defb 000h		;dd30
	defb 000h		;dd31
	defb 000h		;dd32
	defb 000h		;dd33
	defb 000h		;dd34
	defb 000h		;dd35
	defb 000h		;dd36
	defb 000h		;dd37
	defb 000h		;dd38
	defb 000h		;dd39
	defb 000h		;dd3a
	defb 000h		;dd3b
	defb 000h		;dd3c
	defb 000h		;dd3d
	defb 000h		;dd3e
	defb 000h		;dd3f
	defb 000h		;dd40
	defb 000h		;dd41
	defb 000h		;dd42
	defb 000h		;dd43
	defb 000h		;dd44
	defb 000h		;dd45
	defb 000h		;dd46
	defb 000h		;dd47
	defb 000h		;dd48
	defb 000h		;dd49
	defb 000h		;dd4a
	defb 000h		;dd4b
	defb 000h		;dd4c
	defb 000h		;dd4d
	defb 000h		;dd4e
	defb 000h		;dd4f
	defb 000h		;dd50
	defb 000h		;dd51
	defb 000h		;dd52
	defb 000h		;dd53
	defb 000h		;dd54
	defb 000h		;dd55
	defb 000h		;dd56
	defb 000h		;dd57
	defb 000h		;dd58
	defb 000h		;dd59
	defb 000h		;dd5a
	defb 000h		;dd5b
	defb 000h		;dd5c
	defb 000h		;dd5d
	defb 000h		;dd5e
	defb 000h		;dd5f
	defb 000h		;dd60
	defb 000h		;dd61
	defb 000h		;dd62
	defb 000h		;dd63
	defb 000h		;dd64
	defb 000h		;dd65
	defb 000h		;dd66
	defb 000h		;dd67
	defb 000h		;dd68
	defb 000h		;dd69
	defb 000h		;dd6a
	defb 000h		;dd6b
	defb 000h		;dd6c
	defb 000h		;dd6d
;Shared ALV for drives A-D
	org 0dd6eh
alv:
	defb 000h		;dd6e
	defb 000h		;dd6f
	defb 000h		;dd70
	defb 000h		;dd71
	defb 000h		;dd72
	defb 000h		;dd73
	defb 000h		;dd74
	defb 000h		;dd75
	defb 000h		;dd76
	defb 000h		;dd77
	defb 000h		;dd78
	defb 000h		;dd79
	defb 000h		;dd7a
	defb 000h		;dd7b
	defb 000h		;dd7c
	defb 000h		;dd7d
	defb 000h		;dd7e
	defb 000h		;dd7f
	defb 000h		;dd80
	defb 000h		;dd81
	defb 000h		;dd82
	defb 000h		;dd83
	defb 000h		;dd84
	defb 000h		;dd85
	defb 000h		;dd86
	defb 000h		;dd87
	defb 000h		;dd88
	defb 000h		;dd89
	defb 000h		;dd8a
	defb 000h		;dd8b
	defb 000h		;dd8c
	defb 000h		;dd8d
	defb 000h		;dd8e
	defb 000h		;dd8f
	defb 000h		;dd90
	defb 000h		;dd91
	defb 000h		;dd92
	defb 000h		;dd93
	defb 000h		;dd94
	defb 000h		;dd95
	defb 000h		;dd96
	defb 000h		;dd97
	defb 000h		;dd98
	defb 000h		;dd99
	defb 000h		;dd9a
	defb 000h		;dd9b
	defb 000h		;dd9c
	defb 000h		;dd9d
	defb 000h		;dd9e
	defb 000h		;dd9f
	defb 000h		;dda0
	defb 000h		;dda1
	defb 000h		;dda2
	defb 000h		;dda3
	defb 000h		;dda4
	defb 000h		;dda5
	defb 000h		;dda6
	defb 000h		;dda7
	defb 000h		;dda8
	defb 000h		;dda9
	defb 000h		;ddaa
	defb 000h		;ddab
	defb 000h		;ddac
	defb 000h		;ddad
	defb 000h		;ddae
	defb 000h		;ddaf
	defb 000h		;ddb0
	defb 000h		;ddb1
	defb 000h		;ddb2
	defb 000h		;ddb3
	defb 000h		;ddb4
	defb 000h		;ddb5
	defb 000h		;ddb6
	defb 000h		;ddb7
	defb 000h		;ddb8
	defb 000h		;ddb9
	defb 000h		;ddba
	defb 000h		;ddbb
	defb 000h		;ddbc
	defb 000h		;ddbd
	defb 000h		;ddbe
	defb 000h		;ddbf
	defb 000h		;ddc0
	defb 000h		;ddc1
	defb 000h		;ddc2
	defb 000h		;ddc3
	defb 000h		;ddc4
	defb 000h		;ddc5
	defb 000h		;ddc6
	defb 000h		;ddc7
	defb 000h		;ddc8
	defb 000h		;ddc9
	defb 000h		;ddca
	defb 000h		;ddcb
	defb 000h		;ddcc
	defb 000h		;ddcd
	defb 000h		;ddce
	defb 000h		;ddcf
	defb 000h		;ddd0
	defb 000h		;ddd1
	defb 000h		;ddd2
	defb 000h		;ddd3
	defb 000h		;ddd4
	defb 000h		;ddd5
	defb 000h		;ddd6
	defb 000h		;ddd7
	defb 000h		;ddd8
	defb 000h		;ddd9
	defb 000h		;ddda
	defb 000h		;dddb
	defb 000h		;dddc
	defb 000h		;dddd
	defb 000h		;ddde
	defb 000h		;dddf
	defb 000h		;dde0
	defb 000h		;dde1
	defb 000h		;dde2
	defb 000h		;dde3
	defb 000h		;dde4
	defb 000h		;dde5
	defb 000h		;dde6
	defb 000h		;dde7
	defb 000h		;dde8
	defb 000h		;dde9
	defb 000h		;ddea
	defb 000h		;ddeb
	defb 000h		;ddec
	defb 000h		;dded
	defb 000h		;ddee
	defb 000h		;ddef
	defb 000h		;ddf0
	defb 000h		;ddf1
	defb 000h		;ddf2
	defb 000h		;ddf3
	defb 000h		;ddf4
	defb 000h		;ddf5
	defb 000h		;ddf6
	defb 000h		;ddf7
	defb 000h		;ddf8
	defb 000h		;ddf9
	defb 000h		;ddfa
	defb 000h		;ddfb
	defb 000h		;ddfc
	defb 000h		;ddfd
	defb 000h		;ddfe
	defb 000h		;ddff
	defb 000h		;de00
	defb 000h		;de01
	defb 000h		;de02
	defb 000h		;de03
	defb 000h		;de04
	defb 000h		;de05
	defb 000h		;de06
	defb 000h		;de07
	defb 000h		;de08
	defb 000h		;de09
	defb 000h		;de0a
	defb 000h		;de0b
	defb 000h		;de0c
	defb 000h		;de0d
	defb 000h		;de0e
	defb 000h		;de0f
	defb 000h		;de10
	defb 000h		;de11
	defb 000h		;de12
	defb 000h		;de13
	defb 000h		;de14
	defb 000h		;de15
	defb 000h		;de16
	defb 000h		;de17
	defb 000h		;de18
	defb 000h		;de19
	defb 000h		;de1a
	defb 000h		;de1b
	defb 000h		;de1c
	defb 000h		;de1d
	defb 000h		;de1e
	defb 000h		;de1f
	defb 000h		;de20
	defb 000h		;de21
	defb 000h		;de22
	defb 000h		;de23
	defb 000h		;de24
	defb 000h		;de25
	defb 000h		;de26
	defb 000h		;de27
	defb 000h		;de28
	defb 000h		;de29
	defb 000h		;de2a
	defb 000h		;de2b
	defb 000h		;de2c
	defb 000h		;de2d
	defb 000h		;de2e
	defb 000h		;de2f
	defb 000h		;de30
	defb 000h		;de31
	defb 000h		;de32
	defb 000h		;de33
	defb 000h		;de34
	defb 000h		;de35
	defb 000h		;de36
	defb 000h		;de37
	defb 000h		;de38
	defb 000h		;de39
	defb 000h		;de3a
	defb 000h		;de3b
	defb 000h		;de3c
	defb 000h		;de3d
	defb 000h		;de3e
	defb 000h		;de3f
	defb 000h		;de40
	defb 000h		;de41
	defb 000h		;de42
	defb 000h		;de43
	defb 000h		;de44
	defb 000h		;de45
	defb 000h		;de46
	defb 000h		;de47
	defb 000h		;de48
	defb 000h		;de49
	defb 000h		;de4a
	defb 000h		;de4b
	defb 000h		;de4c
	defb 000h		;de4d
	defb 000h		;de4e
	defb 000h		;de4f
	defb 000h		;de50
	defb 000h		;de51
	defb 000h		;de52
	defb 000h		;de53
	defb 000h		;de54
	defb 000h		;de55
	defb 000h		;de56
	defb 000h		;de57
	defb 000h		;de58
	defb 000h		;de59
	defb 000h		;de5a
	defb 000h		;de5b
	defb 000h		;de5c
	defb 000h		;de5d
	defb 000h		;de5e
	defb 000h		;de5f
	defb 000h		;de60
	defb 000h		;de61
	defb 000h		;de62
	defb 000h		;de63
	defb 000h		;de64
	defb 000h		;de65
	defb 000h		;de66
	defb 000h		;de67
	defb 000h		;de68
	defb 000h		;de69
	defb 000h		;de6a
	defb 000h		;de6b
	defb 000h		;de6c
	defb 000h		;de6d
	defb 000h		;de6e
	defb 000h		;de6f
	defb 000h		;de70
	defb 000h		;de71
	defb 000h		;de72
	defb 000h		;de73
	defb 000h		;de74
	defb 000h		;de75
	defb 000h		;de76
	defb 000h		;de77
	defb 000h		;de78
	defb 000h		;de79
	defb 000h		;de7a
	defb 000h		;de7b
	defb 000h		;de7c
	defb 000h		;de7d
	defb 000h		;de7e
	defb 000h		;de7f
	defb 000h		;de80
	defb 000h		;de81
	defb 000h		;de82
	defb 000h		;de83
	defb 000h		;de84
	defb 000h		;de85
	defb 000h		;de86
	defb 000h		;de87
	defb 000h		;de88
	defb 000h		;de89
	defb 000h		;de8a
	defb 000h		;de8b
	defb 000h		;de8c
	defb 000h		;de8d
	defb 000h		;de8e
	defb 000h		;de8f
	defb 000h		;de90
	defb 000h		;de91
	defb 000h		;de92
	defb 000h		;de93
	defb 000h		;de94
	defb 000h		;de95
	defb 000h		;de96
	defb 000h		;de97
	defb 000h		;de98
	defb 000h		;de99
	defb 000h		;de9a
	defb 000h		;de9b
	defb 000h		;de9c
	defb 000h		;de9d
	defb 000h		;de9e
	defb 000h		;de9f
	defb 000h		;dea0
	defb 000h		;dea1
	defb 000h		;dea2
	defb 000h		;dea3
	defb 000h		;dea4
	defb 000h		;dea5
	defb 000h		;dea6
	defb 000h		;dea7
	defb 000h		;dea8
	defb 000h		;dea9
	defb 000h		;deaa
	defb 000h		;deab
	defb 000h		;deac
	defb 000h		;dead
	defb 000h		;deae
	defb 000h		;deaf
	defb 000h		;deb0
	defb 000h		;deb1
	defb 000h		;deb2
	defb 000h		;deb3
	defb 000h		;deb4
	defb 000h		;deb5
	defb 000h		;deb6
	defb 000h		;deb7
	defb 000h		;deb8
	defb 000h		;deb9
	defb 000h		;deba
	defb 000h		;debb
	defb 000h		;debc
	defb 000h		;debd
	defb 000h		;debe
	defb 000h		;debf
	defb 000h		;dec0
	defb 000h		;dec1
	defb 000h		;dec2
	defb 000h		;dec3
	defb 000h		;dec4
	defb 000h		;dec5
	defb 000h		;dec6
	defb 000h		;dec7
	defb 000h		;dec8
	defb 000h		;dec9
	defb 000h		;deca
	defb 000h		;decb
	defb 000h		;decc
	defb 000h		;decd
	defb 000h		;dece
	defb 000h		;decf
	defb 000h		;ded0
	defb 000h		;ded1
	defb 000h		;ded2
	defb 000h		;ded3
	defb 000h		;ded4
	defb 000h		;ded5
	defb 000h		;ded6
	defb 000h		;ded7
	defb 000h		;ded8
	defb 000h		;ded9
	defb 000h		;deda
	defb 000h		;dedb
	defb 000h		;dedc
	defb 000h		;dedd
	defb 000h		;dede
	defb 000h		;dedf
	defb 000h		;dee0
	defb 000h		;dee1
	defb 000h		;dee2
	defb 000h		;dee3
	defb 000h		;dee4
	defb 000h		;dee5
	defb 000h		;dee6
	defb 000h		;dee7
	defb 000h		;dee8
	defb 000h		;dee9
	defb 000h		;deea
	defb 000h		;deeb
	defb 000h		;deec
	defb 000h		;deed
	defb 000h		;deee
	defb 000h		;deef
	defb 000h		;def0
	defb 000h		;def1
	defb 000h		;def2
	defb 000h		;def3
	defb 000h		;def4
	defb 000h		;def5
	defb 000h		;def6
	defb 000h		;def7
	defb 000h		;def8
	defb 000h		;def9
	defb 000h		;defa
	defb 000h		;defb
	defb 000h		;defc
	defb 000h		;defd
	defb 000h		;defe
	defb 000h		;deff
	defb 000h		;df00
	defb 000h		;df01
	defb 000h		;df02
	defb 000h		;df03
	defb 000h		;df04
	defb 000h		;df05
	defb 000h		;df06
	defb 000h		;df07
	defb 000h		;df08
	defb 000h		;df09
	defb 000h		;df0a
	defb 000h		;df0b
	defb 000h		;df0c
	defb 000h		;df0d
	defb 000h		;df0e
	defb 000h		;df0f
	defb 000h		;df10
	defb 000h		;df11
	defb 000h		;df12
	defb 000h		;df13
	defb 000h		;df14
	defb 000h		;df15
	defb 000h		;df16
	defb 000h		;df17
	defb 000h		;df18
	defb 000h		;df19
	defb 000h		;df1a
	defb 000h		;df1b
	defb 000h		;df1c
	defb 000h		;df1d
	defb 000h		;df1e
	defb 000h		;df1f
	defb 000h		;df20
	defb 000h		;df21
	defb 000h		;df22
	defb 000h		;df23
	defb 000h		;df24
	defb 000h		;df25
	defb 000h		;df26
	defb 000h		;df27
	defb 000h		;df28
	defb 000h		;df29
	defb 000h		;df2a
	defb 000h		;df2b
	defb 000h		;df2c
	defb 000h		;df2d
	defb 000h		;df2e
	defb 000h		;df2f
	defb 000h		;df30
	defb 000h		;df31
	defb 000h		;df32
	defb 000h		;df33
	defb 000h		;df34
	defb 000h		;df35
	defb 000h		;df36
	defb 000h		;df37
	defb 000h		;df38
	defb 000h		;df39
	defb 000h		;df3a
	defb 000h		;df3b
	defb 000h		;df3c
	defb 000h		;df3d
	defb 000h		;df3e
	defb 000h		;df3f
	defb 000h		;df40
	defb 000h		;df41
	defb 000h		;df42
	defb 000h		;df43
	defb 000h		;df44
	defb 000h		;df45
	defb 000h		;df46
	defb 000h		;df47
ldf48h:
	defb 000h		;df48
	defb 000h		;df49
	defb 000h		;df4a
	defb 000h		;df4b
	defb 000h		;df4c
	defb 000h		;df4d
	defb 000h		;df4e
	defb 000h		;df4f
	defb 000h		;df50
	defb 000h		;df51
	defb 000h		;df52
	defb 000h		;df53
	defb 000h		;df54
	defb 000h		;df55
	defb 000h		;df56
	defb 000h		;df57
	defb 000h		;df58
	defb 000h		;df59
	defb 000h		;df5a
	defb 000h		;df5b
	defb 000h		;df5c
	defb 000h		;df5d
	defb 000h		;df5e
	defb 000h		;df5f
	defb 000h		;df60
	defb 000h		;df61
	defb 000h		;df62
	defb 000h		;df63
	defb 000h		;df64
	defb 000h		;df65
	defb 000h		;df66
	defb 000h		;df67
	defb 000h		;df68
	defb 000h		;df69
	defb 000h		;df6a
	defb 000h		;df6b
	defb 000h		;df6c
	defb 000h		;df6d
	defb 000h		;df6e
	defb 000h		;df6f
	defb 000h		;df70
ldf71h:
	defb 000h		;df71
	defb 000h		;df72
	defb 000h		;df73
	defb 000h		;df74
	defb 000h		;df75
	defb 000h		;df76
	defb 000h		;df77
	defb 000h		;df78
	defb 000h		;df79
	defb 000h		;df7a
	defb 000h		;df7b
	defb 000h		;df7c
	defb 000h		;df7d
	defb 000h		;df7e
	defb 000h		;df7f
	defb 000h		;df80
	defb 000h		;df81
	defb 000h		;df82
	defb 000h		;df83
	defb 000h		;df84
	defb 000h		;df85
	defb 000h		;df86
	defb 000h		;df87
	defb 000h		;df88
	defb 000h		;df89
	defb 000h		;df8a
	defb 000h		;df8b
	defb 000h		;df8c
	defb 000h		;df8d
	defb 000h		;df8e
	defb 000h		;df8f
	defb 000h		;df90
	defb 000h		;df91
	defb 000h		;df92
	defb 000h		;df93
	defb 000h		;df94
	defb 000h		;df95
	defb 000h		;df96
	defb 000h		;df97
	defb 000h		;df98
	defb 000h		;df99
	defb 000h		;df9a
	defb 000h		;df9b
	defb 000h		;df9c
	defb 000h		;df9d
	defb 000h		;df9e
	defb 000h		;df9f
	defb 000h		;dfa0
	defb 000h		;dfa1
	defb 000h		;dfa2
	defb 000h		;dfa3
	defb 000h		;dfa4
	defb 000h		;dfa5
	defb 000h		;dfa6
	defb 000h		;dfa7
	defb 000h		;dfa8
	defb 000h		;dfa9
	defb 000h		;dfaa
	defb 000h		;dfab
	defb 000h		;dfac
	defb 000h		;dfad
	defb 000h		;dfae
	defb 000h		;dfaf
	defb 000h		;dfb0
	defb 000h		;dfb1
	defb 000h		;dfb2
	defb 000h		;dfb3
	defb 000h		;dfb4
	defb 000h		;dfb5
	defb 000h		;dfb6
	defb 000h		;dfb7
	defb 000h		;dfb8
	defb 000h		;dfb9
	defb 000h		;dfba
	defb 000h		;dfbb
	defb 000h		;dfbc
	defb 000h		;dfbd
	defb 000h		;dfbe
	defb 000h		;dfbf
	defb 000h		;dfc0
	defb 000h		;dfc1
	defb 000h		;dfc2
	defb 000h		;dfc3
	defb 000h		;dfc4
	defb 000h		;dfc5
	defb 000h		;dfc6
	defb 000h		;dfc7
	defb 000h		;dfc8
	defb 000h		;dfc9
	defb 000h		;dfca
	defb 000h		;dfcb
	defb 000h		;dfcc
	defb 000h		;dfcd
	defb 000h		;dfce
	defb 000h		;dfcf
	defb 000h		;dfd0
	defb 000h		;dfd1
	defb 000h		;dfd2
	defb 000h		;dfd3
	defb 000h		;dfd4
	defb 000h		;dfd5
	defb 000h		;dfd6
	defb 000h		;dfd7
	defb 000h		;dfd8
	defb 000h		;dfd9
	defb 000h		;dfda
	defb 000h		;dfdb
	defb 000h		;dfdc
	defb 000h		;dfdd
	defb 000h		;dfde
	defb 000h		;dfdf
	defb 000h		;dfe0
	defb 000h		;dfe1
	defb 000h		;dfe2
	defb 000h		;dfe3
	defb 000h		;dfe4
	defb 000h		;dfe5
	defb 000h		;dfe6
	defb 000h		;dfe7
	defb 000h		;dfe8
	defb 000h		;dfe9
	defb 000h		;dfea
	defb 000h		;dfeb
	defb 000h		;dfec
	defb 000h		;dfed
	defb 000h		;dfee
	defb 000h		;dfef
	defb 000h		;dff0
	defb 000h		;dff1
	defb 000h		;dff2
	defb 000h		;dff3
	defb 000h		;dff4
	defb 000h		;dff5
	defb 000h		;dff6
	defb 000h		;dff7
	defb 000h		;dff8
	defb 000h		;dff9
	defb 000h		;dffa
	defb 000h		;dffb
	defb 000h		;dffc
	defb 000h		;dffd
	defb 000h		;dffe
	defb 000h		;dfff
