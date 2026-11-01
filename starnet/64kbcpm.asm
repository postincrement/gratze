; 64kbcpm — original Starnet 64K CP/M slave (CCP + BDOS + BIOS).
; Build: z80asm 64kbcpm.asm -o 64kbcpm.slv
;
; Image map: CCP $C000, BDOS $C800, BIOS $D600. DPB at $D706.

	org 0c000h

; BN ROM helpers used by BIOS
rom_conout:	equ 0xe00c
rom_net_io:	equ 0xe042
; BDOS still contains a word table misparsed as CALL Z,sub_dbd4h (CC D4 DB).
; Keep the absolute so those bytes round-trip until that table is DEFW'd.
sub_dbd4h:	equ 0dbd4h

	include '64kbcpm_ccp.inc'
	include '64kbcpm_bdos.inc'
	include '64kbcpm_bios.inc'
