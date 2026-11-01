; n64kbcpm — 64kbcpm with private BIOS stack while video bank $14 is mapped.
; Build: z80asm n64kbcpm.asm -o n64kbcpm.slv
;
; Same CCP/BDOS as 64kbcpm. BIOS keeps the $D600 jump table; DPB moves to
; $DE00 (server m_dpbOffset 0x1e00). Bank helpers switch SP to $DCD4 for
; the bank-$14 window so callers with SP near $9000 are not smashed by VDU.

	org 0c000h

; BN ROM helpers used by BIOS
rom_conout:	equ 0xe00c
rom_net_io:	equ 0xe042
; BDOS still contains a word table misparsed as CALL Z,sub_dbd4h (CC D4 DB).
; Keep the absolute so those bytes round-trip until that table is DEFW'd.
sub_dbd4h:	equ 0dbd4h

	include '64kbcpm_ccp.inc'
	include '64kbcpm_bdos.inc'
	include 'n64kbcpm_bios.inc'
