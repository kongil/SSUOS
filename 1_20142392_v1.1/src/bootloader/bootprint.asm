org 0x7c00
[BITS 16]

START:   
mov		ax, 0xb800;비디오 메모리
mov		es, ax
;clear
mov		ax, 0x00
mov		bx, 0
mov		cx, 80*25*2;가로 80 세로 25 글자 2바이트

CLS:
mov		[es:bx], ax
add		bx, 1
loop 	CLS

RESET:
mov		bx, 0

PRINT:
mov		dl,	byte[msg+si]
mov		byte[es:bx], dl;es에 바로 값대입 불가능
add		bx, 1
add		si, 1
mov		byte[es:bx], 0x07
add		bx, 1
cmp		dl, 0
jne		PRINT


msg db "Hello, Kongil's World",0
