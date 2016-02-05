    ORG 0FB00H
	include "spc-1000_all.inc"	
CPM_RUN EQU 0CB00h
L0FC94h EQU 0fc94h
SDINIT		EQU 0
SDWRITE		EQU 1
SDREAD		EQU 2
SDSEND  	EQU 3
SDCOPY		EQU 4
SDFORMAT	EQU 5
SDSTATUS	EQU 6
SDDRVSTS	EQU 7
SDRAMTST	EQU 8
SDTRANS2	EQU 9
SDNOACT		EQU 0Ah
SDTRANS1	EQU 0Bh
SDRCVE		EQU 0Ch
SDGO		EQU 0Dh
SDLOAD		EQU 0Eh
SDSAVE		EQU 0Fh
SDLDNGO		EQU 010h

   	LD	HL,$FB00      	;!
   	LD	DE,0FB00h      	;
   	LD	BC,AHEXIN      	;
   	LDIR              	;
  	LD	HL,00000h      	;!
   	LD	DE,00000h      	;
  	LD	BC,07B00h      	;{
  	LDIR              	;
   	LD	BC,0A000h      	;
  	IN	A,(C)          	;x
   	LD	A,080h         	;>
   	IN	A,(009h)       	;
   	AND	001h          	;
   	JR  Z, CPMCHECK      	;(
BOOTBAS:	
   	LD	HL,MAIN        	;!J
   	LD	(00001h),HL    	;"
 	JP	ROMPATCH        	;
	
CPMCHECK:
   	CALL	L0FE49h      	;I
   	LD	DE,TITLEMSG      	;N
  	CALL	DEPRINT      	;
KEYCHECK:   	LD	B,080h         	;
  	LD	C,001h         	;
   	IN	A,(C)          	;x
   	AND	010h          	;
   	JR	Z,FDDLOAD      	;(
   	LD	C,003h         	;
  	IN	A,(C)          	;x
   	AND	010h          	;
  	JR	NZ,KEYCHECK     	; 
  	JR	BOOTBAS           	;
FDDLOAD:   	LD	DE,WAITMSG      	;m
  	CALL	DEPRINT      	;
CHECKFDD:   	LD	B,0C0h         	;
  	LD	C,002h         	;
   	XOR	A             	;
  	OUT	(C),A         	;y
   	LD	C,000h         	;
   	OUT	(C),A         	;y
L0FB5Ch:
   	LD	D,SDINIT         	;
   	CALL	SENDCOM      	;
   	LD	D,SDDRVSTS         	;
   	CALL	SENDCOM      	;
   	CALL	GETDATA      	;
   	LD	A,D            	;z
  	AND	010h          	;
   	JR	Z,CHECKFDD      ; drive A check
   	JP	LOADCPM        	;
   	RET	PE            	;
   	EI                	;
   	LD	D,07Fh         	;
   	CALL	SENDDAT      	;
   	LD	D,01Ch         	;
  	CALL	SENDDAT      	;
   	LD	D,000h         	;
   	CALL	SENDDAT      	;
   	LD	D,002h         	;
    CALL	SENDDAT      	;
   	LD	D,00Fh         	;
   	CALL	SENDDAT      	;
   	CALL	SENDDAT      	;
   	LD	D,SDTRANS1         	;
  	CALL	SENDCOM      	;
   	LD	D,07Fh         	;
   	CALL	SENDDAT      	;
  	LD	D,01Ch         	;
   	CALL	SENDDAT      	;
  	LD	D,000h         	;
  	CALL	SENDDAT      	;
  	LD	D,003h         	;
  	CALL	SENDDAT      	;
  	CALL	GETDATA      	;
   	LD	A,D            	;z
   	CP	00Fh           	;
   	JP	NZ,L0FB5Ch     	;
   	CALL	GETDATA      	;
   	LD	A,D            	;z
   	CP	00Fh           	;
   	JP	NZ,L0FB5Ch     	;
LOADCPM:	
   	LD	D,SDREAD         	;
   	CALL	SENDCOM      	;
   	LD	D,001h         	;
   	CALL	SENDDAT      	;
   	LD	D,000h         	;
   	CALL	SENDDAT      	;
   	LD	D,000h         	;
   	CALL	SENDDAT      	;
   	LD	D,001h         	;
   	CALL	SENDDAT      	;
   	LD	D,SDSEND         	;
   	CALL	SENDCOM      	;
   	LD	E,000h         	;
   	LD	HL,CPM_RUN    	;!
SYM1:   
	CALL	GETDATA      	;
    LD	(HL),D         	;r
    INC	HL            	;#
    DEC	E             	;
    JR	NZ,SYM1     	; 
	LD  BC, 0100h
CLS3:	
	LD  A, ' '
	OUT (C), A
	DEC BC
	LD  A,B
	OR  C
	JR  NZ, CLS3
    JP	CPM_RUN        	;
SENDCOM:
    LD	B,0C0h         	;
    LD	C,002h         	;
    LD	A,080h         	;>
    OUT	(C),A         	;y
SENDDAT:	
    LD	B,0C0h         	;
    LD	C,002h         	;
CHKRFD1:   	IN	A,(C)          	;x
    AND	002h          	;
    JR	Z,CHKRFD1      	;(
    LD	C,002h         	;
    XOR	A             	;
    OUT	(C),A         	; ATN=0
    LD	C,000h         	;
    OUT	(C),D         	;
    LD	C,002h         	;
    LD	A,010h         	;
    OUT	(C),A         	;
    LD	C,002h         	;
CHKDAC2:   	IN	A,(C)   ;x
    AND	004h          	;
    JR	Z,CHKDAC2      	;(
    LD	C,002h         	;
    XOR	A             	;
    OUT	(C),A         	;y
    LD	C,002h         	;
CHKDAC3:   	IN	A,(C)          	;x
    AND	004h          	;
    JR	NZ,CHKDAC3     	; 
    RET               	;
GETDATA:
    PUSH	BC           	;
    LD	C,002h         	;
    LD	B,0C0h         	;
    LD	A,020h         	;> 
    OUT	(C),A         	;y
    LD	C,002h         	;
CHKDAV0:   	IN	A,(C)          	;x
    AND	001h          	;
    JR	Z,CHKDAV0      	;(
    LD	C,002h         	;
    XOR	A             	;
    OUT	(C),A         	;y
    LD	C,001h         	;
    IN	D,(C)          	;P
    LD	C,002h         	;
    LD	A,040h         	;>@
    OUT	(C),A         	;y
    LD	C,002h         	;
CHKDAV1:   	IN	A,(C)          	;x
    AND	001h          	;
    JR	NZ,CHKDAV1     	; 
    LD	C,002h         	;
    XOR	A             	;
    OUT	(C),A         	;y
    POP	BC            	;
    RET               	;
TITLEMSG:
    DEFM 'SELECT BASIC (B) OR CP/M (C) ?'
    DEFB 00h 
WAITMSG:	
	DEFB 43h, 0dh, 0ah
    DEFM 'BOOTING FROM FDD-----'
    DEFB 0DH
    DEFM ' Wait for a moment!'
    DEFB 0H
    
    JP  NZ, L0FC9Fh
    SCF
    RET
L0FC9Fh:	
    LD	A,040h         	;>@
    IN	A,(001h)       	;
    AND	080h          	;
    JP	NZ,0FC94h     	;
L0FCA8h:	
    LD	A,080h         	;>
    IN	A,(000h)       	;
    AND	012h          	;
    JP	NZ,L0FCB3h     	;
    SCF               	;7
    RET               	;
L0FCB3h:
    LD	A,040h         	;>@
    IN	A,(001h)       	;
    AND	080h          	;
    JP	Z,L0FCA8h      	;
    RET               	;

    PUSH	BC           	;
    PUSH	DE           	;
L0FCBFh:   	PUSH	HL           	;
    LD	HL,00800h      	;!
    LD	BC,04000h      	;@
    LD	A,00Eh         	;>
    OUT	(C),A         	;y
L0FCCAh:
    CALL	L0FC94h      	;
    JP	C,L0FCF1h      	;
    CALL	L0FDF6h      	;
    LD	A,040h         	;>@
    IN	A,(001h)       	;
    AND	080h          	;
    JP	Z,L0FCE6h      	;
    PUSH	HL           	;
    LD	HL,(0FEF9h)    	;*
    INC	HL            	;#
    LD	(0FEF9h),HL    	;"
    POP	HL            	;
    SCF               	;7
L0FCE6h:	
    LD	A,L            	;}
    RLA               	;
    LD	L,A            	;o
    DEC	H             	;%
    JP	NZ,L0FCCAh     	;
    CALL	L0FC94h      	;
    LD	A,L            	;}
L0FCF1h:	
    POP	HL            	;
L0FCF2h:   	POP	DE            	;
L0FCF3h:   	POP	BC            	;
    RET               	;

    PUSH	BC           	;
    PUSH	DE           	;
    PUSH	HL           	;
    LD	HL,INKEY    	;!((
    LD	A,E            	;{
    CP	0CCh           	;
    JP	Z,L0FD04h      	;
    LD	HL,01414h      	;!
L0FD04h:	
    LD	(0FEFDh),HL    	;"
    LD	BC,04000h      	;@
    LD	A,00Eh         	;>
    OUT	(C),A         	;y
L0FD0Eh:	
    LD	HL,(0FEFDh)    	;*
L0FD11h:	
    CALL	L0FC94h      	;
    JP	C,L0FD40h      	;@
    CALL	L0FDF6h      	;
    LD	A,040h         	;>@
    IN	A,(001h)       	;
    AND	080h          	;
    JP	Z,L0FD0Eh      	;
    DEC	H             	;%
    JP	NZ,L0FD11h     	;
L0FD27h:	
    CALL	L0FC94h      	;
    JP	C,L0FD40h      	;@
    CALL	L0FDF6h      	;
    LD	A,040h         	;>@
    IN	A,(001h)       	;
    AND	080h          	;
    JP	NZ,L0FD0Eh     	;
    DEC	L             	;-
    JP	NZ,L0FD27h     	;'
    CALL	L0FC94h      	;
L0FD40h:	
    POP	HL            	;
    POP	DE            	;
    POP	BC            	;
    RET               	;

    PUSH	BC           	;
    PUSH	DE           	;
    PUSH	HL           	;
    LD	BC,04000h      	;@
    LD	A,007h         	;>
    OUT	(C),A         	;y
    INC	C             	;
    LD	A,0BFh         	;>
    OUT	(C),A         	;y
    DEC	C             	;
    LD	A,00Eh         	;>
    OUT	(C),A         	;y
    LD	L,00Ah         	;.
    LD	BC,06000h      	;`
L0FD5Dh:	
    LD	A,040h         	;>@
    IN	A,(001h)       	;
    AND	040h          	;@
    JP	NZ,L0FD6Bh     	;k
L0FD66h:	
    XOR	A             	;
L0FD67h:	
    POP	HL            	;
    POP	DE            	;
    POP	BC            	;
    RET               	;
L0FD6Bh:
    LD	A,(0FEFBh)     	;:
    RES	1,A           	;
    OUT	(C),A         	;y
    SET	1,A           	;
    OUT	(C),A         	;y
    LD	(0FEFBh),A     	;2
    DEC	L             	;-
    JP	NZ,L0FD5Dh     	;]
    CALL	L0FE0Dh      	;
    LD	DE,0FDCDh      	;
    CALL	DEPRINT      	;
L0FD86h:	
    LD	A,040h         	;>@
    IN	A,(001h)       	;
    AND	040h          	;@
    JP	Z,L0FD66h      	;f
    LD	A,080h         	;>
    IN	A,(000h)       	;
    AND	012h          	;
    JP	NZ,L0FD86h     	;
    SCF               	;7
    JP	L0FD67h        	;g
    PUSH	AF           	;
    PUSH	BC           	;
    PUSH	DE           	;
    LD	D,00Ah         	;
    LD	BC,04000h      	;@
    LD	A,00Eh         	;>
    OUT	(C),A         	;y
    LD	BC,06000h      	;`
L0FDABh:	
    LD	A,040h         	;>@
    IN	A,(001h)       	;
    AND	040h          	;@
    JR	Z,L0FDB7h      	;(
    POP	DE            	;
    POP	BC            	;
    POP	AF            	;
    RET               	;

L0FDB7h:   	LD	A,(0FEFBh)     	;:
    RES	1,A           	;
    OUT	(C),A         	;y
    SET	1,A           	;
    OUT	(C),A         	;y
    LD	(0FEFBh),A     	;2
    DEC	D             	;
    JP	NZ,L0FDABh     	;
    POP	DE            	;
    POP	BC            	;
    POP	AF            	;
    RET               	;

    LD	D,B            	;P
    LD	C,H            	;L
    LD	B,C            	;A
    LD	E,C            	;Y
    DEC	C             	;
    NOP               	;
    PUSH	BC           	;
    PUSH	DE           	;
    PUSH	HL           	;
    LD	DE,00000h      	;
L0FDD9h:	
    LD	A,B            	;x
    OR	C              	;
    JR	NZ,L0FDE5h     	; 
    EX	DE,HL          	;
    LD	(0FEF9h),HL    	;"
    POP	HL            	;
    POP	DE            	;
    POP	BC            	;
    RET               	;

L0FDE5h:   	LD	A,(HL)         	;~
    PUSH	HL           	;
    LD	H,008h         	;&
L0FDE9h:   	RLCA              	;
    JR	NC,L0FDEDh     	;0
    INC	DE            	;
L0FDEDh:   	DEC	H             	;%
    JR	NZ,L0FDE9h     	; 
    POP	HL            	;
    INC	HL            	;#
    DEC	BC            	;
    JP	L0FDD9h        	;
L0FDF6h:	
    LD	A,05Ah         	;>Z
L0FDF8h:	
    DEC	A             	;=
    JP	NZ,L0FDF8h     	;
    RET               	;
DEPRINT:
    PUSH	AF           	;
    PUSH	DE           	;
PRTCH:   	LD	A,(DE)         	;
    CP	000h           	;
    JR	Z,ENDPRT      	;(
    CALL	ACCPRT      	;1
    INC	DE            	;
    JR	PRTCH        	;
ENDPRT:   	POP	DE            	;
    POP	AF            	;
    RET               	;
L0FE0Dh:
    LD	A,(CURXY)     	;:
    AND	01Fh          	;
    OR	A              	;
    RET	Z             	;
PRTCR:   	PUSH	HL           	;
    PUSH	BC           	;
    LD	HL,(CURXY)    	;*
    LD	BC,00020h      	; 
    ADD	HL,BC         	;
    LD	A,L            	;}
    AND	0E0h          	;
    LD	L,A            	;o
    LD	(CURXY),HL    	;"
    POP	BC            	;
    POP	HL            	;
    RET               	;

L0FE27h:   	PUSH	BC           	;
    PUSH	DE           	;
    PUSH	HL           	;
    CALL	CGINIT      	;Y
    POP	HL            	;
    POP	DE            	;
    POP	BC            	;
    RET               	;
ACCPRT0:
    CP	00Dh           	;
    JR	Z,PRTCR      	;(
    CP	00Ch           	;
    JR	Z,L0FE27h      	;(
    OR	A              	;
    RET	Z             	;
    PUSH	BC           	;
    LD	BC,(CURXY)    	;K
    OUT	(C),A         	;y
    INC	BC            	;
    LD	(CURXY),BC    	;C
    POP	BC            	;
    RET               	;
L0FE49h:
	CALL 	INITSB
	CALL	GSAVES
    LD	BC,02000h      	; 
    XOR	A             	;
    OUT	(C),A         	;y
    LD	BC,06000h      	;`
    OUT	(C),A         	;y
    LD	(0FEFBh),A     	;2
CGINIT:	
    LD	D,020h         	; 
    LD	BC,00800h      	;
L0FE5Eh:   	DEC	BC            	;
    OUT	(C),D         	;Q
    LD	A,B            	;x
    OR	C              	;
    JR	NZ,L0FE5Eh     	; 
    LD	D,000h         	;
    LD	HL,00800h      	;!
    LD	BC,01000h      	;
L0FE6Dh:   	DEC	BC            	;
    DEC	HL            	;+
    OUT	(C),D         	;Q
    LD	A,H            	;|
    OR	L              	;
    JR	NZ,L0FE6Dh     	; 
    LD	(CURXY),HL    	;"
    RET               	;

    DEFB 0
    DEFM '                ' 
    NOP               	;
    NOP               	;
    NOP               	;
    NOP               	;
    NOP               	;
    NOP               	;
    NOP               	;
RVRTFONT:	
    LD	HL,000FFh      	;
    LD	(055D3h),HL    	; CHR$(&H8A)
    RET               	;

PATCOLOR:	
;    LD	A,020h         	;
;    LD	HL,00C45h      	;
;    LD	(HL),A         	;;
;	LD	HL,00B22h      	;
;    LD	(HL),A         	;
;   	LD	HL,00ADCh      	;
;    LD	(HL),A         	; 
;   	LD	HL,00AA2h      	;
;    LD	(HL),A         	;
;   	LD	HL,0090Ch      	;
;    LD	(HL),A         	;
    EI                	;
    JP	BOOT           	;
    DEFS    572-497
    INC	D             	;
    INC	D             	;
    NOP               	;
    NOP               	;
ROMPATCH:	
    DI                	;
    LD	SP,00000h      	;
    LD	B,09Dh         	; 1. replace 7c4e --> 7c9d from address 04300h to 01500h  
    LD	HL,04300h      	;
L0FF0Ah:   	LD	A,(HL)  ;
    CP	07Ch           	;
    JR	NZ,L0FF16h     	; 
    DEC	HL            	;
    LD	A,(HL)         	;
    CP	04Eh           	;
    JR	NZ,L0FF16h     	; 
    LD	(HL),B         	;
L0FF16h:   	DEC	HL      ;
    LD	A,H            	;
    CP	015h           	;
    JR	NC,L0FF0Ah     	;
    LD	HL,VSTART      	; 2. put data 09dh at address 7a3bh
    LD	(HL),B         	;
    LD	HL,CGCHR      	; 3. font height modification
L0FF23h:   	LD	BC,0000Bh      	
    LD	D,H            	; 
    LD	E,L            	;
    ADD	HL,BC         	;
    LD	A,H            	;
    CP	05Bh           	;
    JR	C,L0FF33h      	;
    LD	A,L            	;
    CP	04Ah           	;
    JR	NC,L0FF44h     	;
L0FF33h:   	LD	A,(DE)  ;
    OR	(HL)           	;
    JR	Z,L0FF3Ah      	;
    INC	HL            	;
    JR	L0FF23h        	;
L0FF3Ah:   	PUSH	HL  ;
    LD	D,H            	;
    LD	E,L            	;
    DEC	HL            	;
    LDDR              	;
    POP	HL            	;
    INC	HL            	;
    JR	L0FF23h        	;
L0FF44h:   	
    CALL	RVRTFONT    ; 4. revert font '-' 
    LD	DE,07C4Eh      	; 5. 7c4eh <- ff98h (size = 04fh)
    LD	HL,PATCODE     	;
    LD	BC,PATCODEE - PATCODE      	;
    LDIR              	;
    LD	HL,PATCH1      	; 
    LD	DE,07C75h      	;
    CALL	PATCHCD     ; 6. CALL 07c75h @ 01425h
    LD	HL,PATCH2      	; 
    LD	DE,07C4Eh      	;
    CALL	PATCHCD     ; 7. CALL 07c4eh @ 04860h
    LD	HL,PATCH3      	; 
    LD	DE,07C5Bh      	;
    CALL	PATCHCD     ; 8. CALL 07c5bh @ 048a5h
    LD	HL,PATCH4      	; 
    LD	DE,07C6Ch      	;
    CALL	PATCHCD     ; 9. CALL 07c6ch @ 005efh
    LD	HL,PATCH5+1      	;
    LD	DE,07C90h      	;
    CALL	PATCHDE     ; 10. put 07c90h @ 01b89h CALL GRAPH -> 7c90h
    LD	HL,PATCH6      	;
    LD	DE,07C89h      	;
    CALL	PATCHCD     ; 11. CALL 07c89h @ 01485h 
    CALL	INITSB      ;
    CALL	BEEP      	;
    JP	PATCOLOR       	; 12. color value patch.
PATCHCD:	
	LD	(HL),0CDh
	INC	HL    
PATCHDE:	
	LD	(HL),E
    INC	HL            	;#
    LD	(HL),D         	;r
    RET               	;
PATCODE:
    LD	A,E            	;{	
    AND	003h          	;
    SRL	E             	;;
    SRL	E             	;;
    SLA	A             	;'
    SLA	A             	;'
    JR	L0FFB0h        	;
    LD	A,E            	;{
    AND	00Ch          	;
    RRC	E             	;
    RRC	E             	;
    RRC	E             	;
    RRC	E             	;
L0FFB0h:   	OR	E              	;
    LD	E,A            	;_
    LD	A,(GCOLOR)     	;:
    RET               	;

    BIT	7,A           	;
    JR	Z,L0FFBBh      	;(
    CPL               	;/
L0FFBBh:   	RLCA              	;
    RLCA              	;
    RLCA              	;
    RET               	;

    PUSH	BC           	;
    LD	BC,001FFh      	;
    LD	HL,KEYBUF      	;!Oz
L0FFC6h:   	XOR	A             	;
    LD	(HL),A         	;w
    INC	HL            	;#
    DEC	BC            	;
    LD	A,B            	;x
    OR	C              	;
    JR	NZ,L0FFC6h     	; 
    LD	HL,07C9Dh      	;!|
    POP	BC            	;
    RET               	;

    RET	C             	;
    LD	A,020h         	;> 
    LD	L,000h         	;.
    JR	L0FFDCh        	;
    LD	A,000h         	;>
L0FFDCh:   	PUSH	HL           	;
    LD	HL,CLS1LP-1   	;!P
    LD	(HL),A         	;w
    POP	HL            	;
    CALL	GRAPH      	;
    RET               	;
CURXY:
	DEFW	0
PATCODEE:
	DEFS 10000h-$, 0ffh