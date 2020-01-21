P      EQU       1
N      EQU       2
Z      EQU       0
LCOM   EQU       02H
CAR    EQU       01H     
SENS   EQU       80H
FLAG   EQU       40H
II     EQU       20H
IDC    EQU       20H
OVF    EQU       04H
UN     EQU       3
EQ     EQU       0
LT     EQU       2
GT     EQU       1
WC     EQU       08H
RS     EQU       10H
SPAC   EQU       20H
BMAX   EQU       1
DELE   EQU       7FH
CR     EQU       13
LF     EQU       10
BLEN   EQU       20
STAR   EQU       ':'
*
       ORG       0          ;Zero mark vector and 0
INIT   LODI,R3   63
       EORZ      R0
AINI   STRA,R0   COM,R3,-
       BRNR,R3   AINI
       LODI,R0   77H
       STRA,R0   XGOT
       LODI,R0   1BH
       STRA,R0   XGOT+2
       LODI,R0   80H
       STRA,R0   XGOT+3
       BCTR,UN   MBUG
VEC    DW        BK01      ; Breakpoint vector
       DW        BK02
*
* COMMAND HANDLER
EBUG   LODI,R0   '?'
       BSTA,UN   COUT
MBUG   CPSL      0FFH
       BSTA,UN   CRLF
       LODI,R0   '*'
       BSTA,UN   COUT
       BSTR,UN   LINE      ; Don't care if there is
       EORZ      R0
       STRA,R0   BPTR
       LODA,R0   BUFF
       COMI,R0   'A'
       BCTA,EQ   ALTE
       COMI,R0   'B'
       BCTA,EQ   BKPT
       COMI,R0   'C'
       BCTA,EQ   CLR
       COMI,R0   'D'
       BCTA,EQ   DUMP
       COMI,R0   'G'
       BCTA,EQ   GOTO
       COMI,R0   'L'
       BCTA,EQ   LOAD
       COMI,R0   'S'
       BCTA,EQ   SREG
       BCTA,UN   EBUG
* Input a CMD line into buffer
* Code is 1=CR  2=LF  3=MSG+CR  4=MSG+LF
LINE   LODI,R3   0FFH      ; -1
       STRA,R3   BPTR
LLIN   COMI,R3   BLEN
       BCTR,EQ   ELIN      ; On buffer overflow
       BSTA,UN   CHIN      ; Get char
       COMI,R0   DELE
       BCFR,EQ   ALIN
       COMI,R3   0FFH      ; -1 Echo and back ptr
       BCTR,EQ   LLIN
       LODA,R0   BUFF,R3
       BSTA,UN   COUT
       SUBI,R3   1
       BCTR,UN   LLIN
ALIN   COMI,R0   CR
       BCFR,EQ   BLIN
ELIN   LODI,R1   1
CLIN   LODZ      R3
       BCTR,N    DLIN
       ADDI,R1   2
DLIN   STRA,R1   CODE
       STRA,R3   CNT
CRLF   LODI,R0   CR
       BSTA,UN   COUT
       LODI,R0   LF
       BSTA,UN   COUT
       RETC,UN
BLIN   LODI,R1   2
       COMI,R0   LF
       BCTR,EQ   CLIN
       STRA,R0   BUFF,R3,+ ; Store char and echo
       BSTA,UN   COUT
       BCTA,UN   LLIN
*
* Subr that stores double precision into temp
STRT   STRA,R1   TEMP
       STRA,R2   TEMP+1
       RETC,UN
* Display and alter memory
ALTE   BSTA,UN   GNUM
LALT   BSTR,UN   STRT                
       BSTA,UN   BOUT
       LODA,R1   TEMP+1
       BSTA,UN   BOUT
       BSTA,UN   FORM
       LODA,R1   *TEMP     ; Display content
       BSTA,UN   BOUT
       BSTA,UN   FORM
       BSTA,UN   LINE
       LODA,R0   CODE
       COMI,R0   2
       BCTA,LT   MBUG
       BCTR,EQ   DALT
CALT   STRA,R0   TEMR
       BSTA,UN   GNUM
       STRA,R2   *TEMP     ; Update contents
       LODA,R0   TEMR
       COMI,R0   4
       BCFA,EQ   MBUG
DALT   LODI,R2   1
       ADDA,R2   TEMP+1
       LODI,R1   0
       PPSL      WC
       ADDA,R1   TEMP
       CPSL      WC
       BCTA,UN   LALT
* Selectively display and alter registers
SREG   BSTA,UN   GNUM      ; Get index of reg
LSRE   COMI,R2   8         ; Check range
       BCTA,GT   EBUG
       STRA,R2   TEMR
       LODA,R0   COM,R2    ; Display contents
       STRZ      R1
       BSTA,UN   BOUT
       BSTA,UN   FORM
       BSTA,UN   LINE
       LODA,R0   CODE
       COMI,R0   2
       BCTA,LT   MBUG      ; CR
       BCTR,EQ   CSRE      ; LF
ASRE   STRA,R0   TEMQ      ; Upate contents, then
       BSTA,UN   GNUM
       LODZ      R2
       LODA,R2   TEMR
       STRA,R0   COM,R2
       COMI,R2   8         ; Must update PSW lower
       BCFR,EQ   BSRE
       STRA,R0   XGOT+1
BSRE   LODA,R0   TEMQ
       COMI,R0   3
       BCTA,EQ   MBUG
CSRE   LODA,R2   TEMR
       ADDI,R2   1
       BCTA,UN   LSRE
* Goto Address
GOTO   BSTA,UN   GNUM
       BSTA,UN   STRT      ; Put addr in RAM
       LODA,R0   COM+7
       LPSU
       LODA,R1   COM+1     ; Bank zero
       LODA,R2   COM+2
       LODA,R3   COM+3
       PPSL      RS        ; Bank one
       LODA,R1   COM+4
       LODA,R2   COM+5
       LODA,R3   COM+6
       LODA,R0   COM
       CPSL      0FFH
       BCTA,UN   XGOT      ; and BCTA,UN $TEMP
*
* Breakpoint Runtime Code
BK01   STRA,R0   COM       ; Entry for BKPT-1 VIA V
       SPSL
       STRA,R0   COM+8
       STRA,R0   XGOT+1    ; In RAM for reg restore
       LODI,R0   0         ; BKPT index
       BCTR,UN   BKEN
BK02   STRA,R0   COM       ; Entry for BKPT-2
       SPSL
       STRA,R0   COM+8
       STRA,R0   XGOT+1
       LODI,R0   1
BKEN   STRA,R0   TEMR
       SPSU
       STRA,R0   COM+7
       PPSL      RS
       STRA,R1   COM+4
       STRA,R2   COM+5
       STRA,R3   COM+6
       CPSL      RS        ; Force to bank zero
       STRA,R1   COM+1
       STRA,R2   COM+2
       STRA,R3   COM+3
       LODA,R2   TEMR
       BSTR,UN   CLBK
       LODA,R1   TEMP      ; Print BKPT addr
       BSTA,UN   BOUT
       LODA,R1   TEMP+1
       BSTA,UN   BOUT
       BCTA,UN   MBUG
* Subr to clear a BKPT  Like many subr has rel addr   
CLBK   EORZ      R0
       STRA,R0   MARK,R2
       LODA,R0   HADR,R2
       STRA,R0   TEMP
       LODA,R0   LADR,R2
       STRA,R0   TEMP+1
       LODA,R0   HDAT,R2
       STRA,R0   *TEMP
       LODA,R0   LDAT,R2
       LODI,R3   1
       STRA,R0   *TEMP,R3
       RETC,UN
* Break point  Mark indicates if set
* HADR +LADR is BKPT addr,  HDAT + LDAT is two byte
CLR    BSTR,UN   NOK
       LODA,R0   MARK,R2   ; Clear it if set
       BCTA,Z    EBUG
       BSTR,UN   CLBK
       BCTA,UN   MBUG
NOK    BSTA,UN   GNUM      ; Check range on BKPT number
       SUBI,R2   1
       BCTA,N    ABRT
       COMI,R2   BMAX
       BCTA,GT   ABRT
       RETC,UN
BKPT   BSTR,UN   NOK
       LODA,R0   MARK,R2
       BSFA,Z    CLBK      ; Clear existing
       STRA,R2   TEMR
       BSTA,UN   GNUM      ; Get BKPT addr
       BSTA,UN   STRT      ; Subr to store r1-r2 in
       LODA,R3   TEMR
       LODZ      R2
       STRA,R0   LADR,R3
       LODZ      R1
       STRA,R0   HADR,R3
       LODA,R0   *TEMP     ; Save contents
       STRA,R0   HDAT,R3
       LODI,R1   9BH       ; = ZBBR
       STRA,R1   *TEMP
       LODI,R2   1
       LODA,R0   *TEMP,R2
       STRA,R0   LDAT,R3
       LODA,R0   DISP,R3
       STRA,R0   *TEMP,R2
       LODI,R0   0FFH      ; -1
       STRA,R0   MARK, R3
       BCTA,UN   MBUG
DISP   DB        VEC+80H       
       DB        VEC+80H+2
*
* Input two hex chars and form as byte in R1
BIN    BSTA,UN   CHIN
       BSTR,UN   LKUP
       RRL,R3
       RRL,R3
       RRL,R3
       RRL,R3
       STRA,R3   TEMS
       BSTA,UN   CHIN
       BSTR,UN   LKUP
       IORA,R3   TEMS
       LODZ      R3
       STRZ      R1
       BSTR,UN   CBCC
       RETC,UN
* Calculate the BCC char, EOR and then rotate left
CBCC   LODZ      R1
       EORA,R0   BCC
       RRL,R0
       STRA,R0   BCC
       RETC,UN
* Lookup ASCII char in hex value table
LKUP   LODI,R3   16
ALKU   COMA,R0   ANSI,R3,-
       RETC,EQ
       COMI,R3   1
       BCFR,LT   ALKU
* Abort exit from any level of subr
* Use RAS ptr since possible BKPT prog using it
ABRT   LODA,R0   COM+7
       IORI,R0   40H
       SPSU
       BCTA,UN   EBUG
ANSI   DB        '0123'
       DB        '4567'
       DB        '89AB'
       DB        'CDEF'
* Byte in R1 output in hex
BOUT   STRA,R1   TEMS
       BSTR,UN   CBCC
       RRR,R1
       RRR,R1
       RRR,R1
       RRR,R1
       ANDI,R1   0FH
       LODA,R0   ANSI,R1
       BSTA,UN   COUT
       LODA,R1   TEMS
       ANDI,R1   0FH
       LODA,R0   ANSI,R1
       BSTA,UN   COUT
       RETC,UN
* 110 baud input for papertape and char  1Mhz clock
CHIN   PPSL      RS
       LODI,R0   80H
       WRTC,R0
       LODI,R1   0
       LODI,R2   8
ACHI   SPSU
       BCTR,LT   CHIN
       EORZ      R0
       WRTC,R0
       BSTR,UN   DLY
BCHI   BSTR,UN   DLAY
       SPSU
       ANDI,R0   80H
       RRR,R1
       IORZ      R1
       STRZ      R1
       BDRR,R2   BCHI
       BSTR,UN   DLAY
       ANDI,R1   7FH       ; Delete parity bit
       LODZ      R1
       CPSL      RS+WC
       RETC,UN
* Delay for one bit time
DLAY   EORZ      R0
       BDRR,R0   $
       BDRR,R0   $
DLY    BDRR,R0   $
       LODI,R0   0E5H
       BDRR,R0   $
       RETC,UN
*
COUT   PPSL      RS
       PPSU      FLAG
       STRZ      R2
       LODI,R1   8
       BSTR,UN   DLAY
       BSTR,UN   DLAY
       CPSU      FLAG
ACOU   BSTR,UN   DLAY
       RRR,R2
       BCTR,LT   ONE
       CPSU      FLAG
       BCTR,UN   ZERO
ONE    PPSU      FLAG
ZERO   BDRR,R1   ACOU
       BSTR,UN   DLAY
       PPSU      FLAG
       CPSL      RS
       RETC,UN
*
* Get a number from the buffer into R1 - R2
DNUM   LODA,R0   CODE
       BCTR,Z    LNUM      ; Skip spaces until EOB
       RETC,UN             ; or space ending number
GNUM   EORZ      R0
       STRZ      R1
       STRZ      R2
       STRA,R0   CODE
LNUM   LODA,R3   BPTR
       COMA,R3   CNT       ; Check for EOB
       RETC,EQ
       LODA,R0   BUFF,R3,+ ; Get char
       STRA,R3   BPTR
       COMI,R0   SPAC
       BCTR,EQ   DNUM
BNUM   BSTA,UN   LKUP       
CNUM   LODI,R0   0FH       ; R1=AB R2=DD
       RRL,R2
       RRL,R2
       RRL,R2
       RRL,R2
       ANDZ      R2
       RRL,R1
       RRL,R1
       RRL,R1
       RRL,R1
       ANDI,R1   0F0H;
       ANDI,R2   0F0H      ; R0=C R1=B0 R2=D0 R3=V
       IORZ      R1
       STRZ      R1
       LODZ      R3
       IORZ      R2
       STRZ      R2        ; R1=BC R2=DV
       LODI,R0   1
       STRA,R0   CODE
       BCTR,UN   LNUM
* Dump to paper tape in object format
DUMP   BSTR,UN   GNUM      ; Start address
       BSTA,UN   STRT
       BSTR,UN   GNUM
       ADDI,R2   1
       PPSL      WC
       ADDI,R1   0
       CPSL      WC        ; Make end addr not incl
       STRA,R1   TEMQ
       STRA,R2   TEMQ+1
FDUM   BSTR,UN   GAP
       LODI,R0   0FFH      ; -1
       STRA,R0   CNT
       BSTA,UN   CRLF      ; Punch for CR/LF and star
       LODI,R0   STAR
       BSTA,UN   COUT
       EORZ      R0
       STRA,R0   BCC
       LODA,R1   TEMQ
       LODA,R2   TEMQ+1
       SUBA,R2   TEMP+1    ; Get byte count
       PPSL      WC
       SUBA,R1   TEMP
       CPSL      WC
       BCTA,N    EBUG      ; Start > end addr
       BCTR,P    ADUM      ; Cnt > normal block size
       BRNR,R2   BDUM      ; This is short block
       LODI,R3   4         ; EOF. Punch zero blk
CDUM   BSTA,UN   BOUT
       BDRR,R3   CDUM
       BSTR,UN   GAP
       BCTA,UN   MBUG
* Subrs for outputting blanks
FORM   LODI,R3   3
       BCTR,UN   AGAP
GAP    LODI,R3   50
AGAP   LODI,R0   SPAC
       BSTA,UN   COUT
       BDRR,R3   AGAP
       RETC,UN
ADUM   LODI,R2   255
BDUM   STRA,R2   MCNT
       LODA,R1   TEMP      ; Starting address
       BSTA,UN   BOUT
       LODA,R1   TEMP+1
       BSTA,UN   BOUT
       LODA,R1   MCNT      ; Count of data bytes in
       BSTA,UN   BOUT
       LODA,R1   BCC
       BSTA,UN   BOUT
DDUM   LODA,R3   CNT
       LODA,R0   *TEMP,R3,+
       COMA,R3   MCNT
       BCTR,EQ   EDUM      ; Output BCC
       STRA,R3   CNT
       STRZ      R1
       BSTA,UN   BOUT
       BCTR,UN   DDUM
EDUM   LODA,R1   BCC
       BSTA,UN   BOUT
       LODA,R2   TEMP+1
       ADDA,R2   MCNT
       LODI,R1   0
       PPSL      WC
       ADDA,R1   TEMP
       CPSL      WC
       BSTA,UN   STRT
       BCTA,UN   FDUM
* Load from papertape in object format
LOAD   BSTA,UN   CHIN      ; Look for start char
       COMI,R0   STAR
       BCFR,EQ   LOAD
       EORZ      R0
       STRA,R0   BCC
       BSTA,UN   BIN       ; Read addr and count in
       STRA,R1   TEMP
       BSTA,UN   BIN
       STRA,R1   TEMP+1
       BSTA,UN   BIN
       BRNR,R1   ALOA
       BCTA,UN   *TEMP
ALOA   STRA,R1   MCNT
       BSTA,UN   BIN       ; Check BCC on information
       LODA,R0   BCC
       BCFA,Z    EBUG
       STRZ      R3        ; Read data
BLOA   STRA,R3   CNT
       BSTA,UN   BIN
       LODA,R3   CNT
       COMA,R3   MCNT
       BCTR,EQ   CLOA      ; Have read BCC
       LODZ      R1
       STRA,R0   *TEMP,R3  ; Store data
       BIRR,R3   BLOA
CLOA   LODA,R0   BCC
       BCFA,Z    EBUG
       BCTA,UN   LOAD
*
       ORG       0400H
*******     RAM Definitions
COM    DS        9
XGOT   DS        2         ; PPSL      0
       DS        2         ; BCTR,UN   *$+2      
                           ; Must precede the TEMP
TEMP   DS        2
TEMQ   DS        2
TEMR   DS        1
TEMS   DS        1
BUFF   DS        BLEN
BPTR   DS        1
MCNT   DS        1
CNT    DS        1
CODE   DS        1
OKGO   DS        1
BCC    DS        1
MARK   DS        BMAX+1
HDAT   DS        BMAX+1
LDAT   DS        BMAX+1
HADR   DS        BMAX+1
LADR   DS        BMAX+1
END
