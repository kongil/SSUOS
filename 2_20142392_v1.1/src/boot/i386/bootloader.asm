org	0x7c00   

[BITS 16]

START:   
		jmp		BOOT1_LOAD ;BOOT1_LOAD로 점프

BOOT1_LOAD:
	mov     ax, 0x0900 
        mov     es, ax
        mov     bx, 0x0

        mov     ah, 2	
        mov     al, 0x4		
        mov     ch, 0	
        mov     cl, 2	
        mov     dh, 0		
        mov     dl, 0x80

        int     0x13	
        jc      BOOT1_LOAD
;출력
		
RESET:
		mov		ax, 0xb800
		mov 	es, ax
		mov		ax, 0x00
		mov		bx, 0
		mov		cx, 80*25*2
		
CLS:
		mov		[es:bx], ax
		add		bx, 1
		loop		CLS

MOVE_CURSOR1:
		mov bx, 0

PRINT_SSUOS1:
		mov		dl, byte[ssuos_1+si]	
		mov		byte[es:bx], dl
		add		bx, 1
		add		si, 1
		mov		byte[es:bx], 0x07
		add		bx, 1
		cmp		dl, 0
		jne		PRINT_SSUOS1


MOVE_CURSOR2:
		mov bx, 80*2
		xor	si, si
		
PRINT_SSUOS2:
		mov		dl, byte[ssuos_2+si]	
		mov		byte[es:bx], dl
		add		bx, 1
		add		si, 1
		mov		byte[es:bx], 0x07
		add		bx, 1
		cmp		dl, 0
		jne		PRINT_SSUOS2

MOVE_CURSOR3:
		mov bx, 80*4
		xor	si, si

PRINT_SSUOS3:
		mov		dl, byte[ssuos_3+si]	
		mov		byte[es:bx], dl
		add		bx, 1
		add		si, 1
		mov		byte[es:bx], 0x07
		add		bx, 1
		cmp		dl, 0
		jne		PRINT_SSUOS3

MOVE_CURSOR_SELECT:
		xor 	si, si
		mov		bx, 0	

PRINT_SELECT:	
		mov		dl, byte[select+si]	
		mov		byte[es:bx], dl
		add		bx, 1
		add		si, 1
		mov		byte[es:bx], 0x07	
		add		bx, 1
		cmp		dl, 0
		jne		PRINT_SELECT 
		
READ_INPUT:
		mov		ah, 0
		int 	0x16 
		cmp		ah, 0x50
		je 		MOVE_CURSOR_DOWN
		cmp		ah, 0x48
		je 		MOVE_CURSOR_UP
		cmp		ah, 0x1c
		je		CHOOSE_KIND
		jmp		READ_INPUT

MOVE_CURSOR_DOWN:
		cmp		bx, 80*4+4*2
		je		READ_INPUT
		xor 	si, si
		sub		bx, 3*2	
		mov		byte[es:bx], 0
		sub		bx, 1*2	
		add		bx, 80*2	
		jmp		PRINT_SELECT

MOVE_CURSOR_UP:
		cmp		bx, 4*2
		je		READ_INPUT
		xor 	si, si
		sub		bx, 3*2	
		mov		byte[es:bx], 0
		sub		bx, 1*2	
		sub		bx, 80*2	
		jmp		PRINT_SELECT

CHOOSE_KIND:
		cmp		bx, 4*2
		je		KERNEL_LOAD
		cmp		bx, 80*2+4*2
		je		KERNEL_LOAD2
		cmp		bx, 80*4+4*2
		je		KERNEL_LOAD3

KERNEL_LOAD:
	mov     ax, 0x1000	
       mov     es, ax		
        mov     bx, 0x0		

        mov     ah, 2		
        mov     al, 0x3f	
        mov     ch, 0		
        mov     cl, 0x6	
        mov     dh, 0     
        mov     dl, 0x80  

        int     0x13
        jc      KERNEL_LOAD

		jmp		END

KERNEL_LOAD2:
	mov     ax, 0x1000	
       mov     es, ax		
        mov     bx, 0x0		

        mov     ah, 2		
        mov     al, 0x3f	
        mov     ch, 9		
        mov     cl, 47	
        mov     dh, 14     
        mov     dl, 0x80  

        int     0x13
        jc      KERNEL_LOAD2

		jmp END

KERNEL_LOAD3:
	mov     ax, 0x1000	
       mov     es, ax		
        mov     bx, 0x0		

        mov     ah, 2		
        mov     al, 0x3f	
        mov     ch, 14		
        mov     cl, 7	
        mov     dh, 14
        mov     dl, 0x80  

        int     0x13
        jc      KERNEL_LOAD3

END:

jmp		0x0900:0x0000

select db "[O]",0
ssuos_1 db "[ ] SSUOS_1",0
ssuos_2 db "[ ] SSUOS_2",0
ssuos_3 db "[ ] SSUOS_3",0
ssuos_4 db "[ ] SSUOS_4",0
partition_num : resw 1

times   446-($-$$) db 0x00

PTE:
partition1 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition2 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition3 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x98, 0x3a, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition4 db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
times 	510-($-$$) db 0x00
dw	0xaa55
