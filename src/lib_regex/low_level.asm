PUBLIC Call0Asm
PUBLIC Call1Asm
PUBLIC Call2Asm
PUBLIC Call3Asm

_TEXT SEGMENT
Call0Asm PROC FRAME
	sub rsp, 00a8h
	.ALLOCSTACK 00a8h
	mov QWORD PTR 20h[rsp], rbx
	mov QWORD PTR 28h[rsp], rsi
	mov QWORD PTR 30h[rsp], rdi
	mov QWORD PTR 38h[rsp], r12
	mov QWORD PTR 40h[rsp], r13
	mov QWORD PTR 48h[rsp], r14
	mov QWORD PTR 50h[rsp], r15
	movsd QWORD PTR 58h[rsp], xmm6
	movsd QWORD PTR 60h[rsp], xmm7
	movsd QWORD PTR 68h[rsp], xmm8
	movsd QWORD PTR 70h[rsp], xmm9
	movsd QWORD PTR 78h[rsp], xmm10
	movsd QWORD PTR 80h[rsp], xmm11
	movsd QWORD PTR 88h[rsp], xmm12
	movsd QWORD PTR 90h[rsp], xmm13
	movsd QWORD PTR 98h[rsp], xmm14
	movsd QWORD PTR 00a0h[rsp], xmm15
	.ENDPROLOG
	; begin
	call rcx
	; end
	mov rbx, QWORD PTR 20h[rsp]
	mov rsi, QWORD PTR 28h[rsp]
	mov rdi, QWORD PTR 30h[rsp]
	mov r12, QWORD PTR 38h[rsp]
	mov r13, QWORD PTR 40h[rsp]
	mov r14, QWORD PTR 48h[rsp]
	mov r15, QWORD PTR 50h[rsp]
	movsd xmm6, QWORD PTR 58h[rsp]
	movsd xmm7, QWORD PTR 60h[rsp]
	movsd xmm8, QWORD PTR 68h[rsp]
	movsd xmm9, QWORD PTR 70h[rsp]
	movsd xmm10, QWORD PTR 78h[rsp]
	movsd xmm11, QWORD PTR 80h[rsp]
	movsd xmm12, QWORD PTR 88h[rsp]
	movsd xmm13, QWORD PTR 90h[rsp]
	movsd xmm14, QWORD PTR 98h[rsp]
	movsd xmm15, QWORD PTR 00a0h[rsp]
	add rsp, 00a8h
	ret 0
Call0Asm ENDP
_TEXT ENDS

_TEXT SEGMENT
Call1Asm PROC FRAME
	sub rsp, 00a8h
	.ALLOCSTACK 00a8h
	mov QWORD PTR 20h[rsp], rbx
	mov QWORD PTR 28h[rsp], rsi
	mov QWORD PTR 30h[rsp], rdi
	mov QWORD PTR 38h[rsp], r12
	mov QWORD PTR 40h[rsp], r13
	mov QWORD PTR 48h[rsp], r14
	mov QWORD PTR 50h[rsp], r15
	movsd QWORD PTR 58h[rsp], xmm6
	movsd QWORD PTR 60h[rsp], xmm7
	movsd QWORD PTR 68h[rsp], xmm8
	movsd QWORD PTR 70h[rsp], xmm9
	movsd QWORD PTR 78h[rsp], xmm10
	movsd QWORD PTR 80h[rsp], xmm11
	movsd QWORD PTR 88h[rsp], xmm12
	movsd QWORD PTR 90h[rsp], xmm13
	movsd QWORD PTR 98h[rsp], xmm14
	movsd QWORD PTR 00a0h[rsp], xmm15
	.ENDPROLOG
	; begin
	mov QWORD PTR 00h[rsp], rcx
	call rdx
	; end
	mov rbx, QWORD PTR 20h[rsp]
	mov rsi, QWORD PTR 28h[rsp]
	mov rdi, QWORD PTR 30h[rsp]
	mov r12, QWORD PTR 38h[rsp]
	mov r13, QWORD PTR 40h[rsp]
	mov r14, QWORD PTR 48h[rsp]
	mov r15, QWORD PTR 50h[rsp]
	movsd xmm6, QWORD PTR 58h[rsp]
	movsd xmm7, QWORD PTR 60h[rsp]
	movsd xmm8, QWORD PTR 68h[rsp]
	movsd xmm9, QWORD PTR 70h[rsp]
	movsd xmm10, QWORD PTR 78h[rsp]
	movsd xmm11, QWORD PTR 80h[rsp]
	movsd xmm12, QWORD PTR 88h[rsp]
	movsd xmm13, QWORD PTR 90h[rsp]
	movsd xmm14, QWORD PTR 98h[rsp]
	movsd xmm15, QWORD PTR 00a0h[rsp]
	add rsp, 00a8h
	ret 0
Call1Asm ENDP
_TEXT ENDS

_TEXT SEGMENT
Call2Asm PROC FRAME
	sub rsp, 00a8h
	.ALLOCSTACK 00a8h
	mov QWORD PTR 20h[rsp], rbx
	mov QWORD PTR 28h[rsp], rsi
	mov QWORD PTR 30h[rsp], rdi
	mov QWORD PTR 38h[rsp], r12
	mov QWORD PTR 40h[rsp], r13
	mov QWORD PTR 48h[rsp], r14
	mov QWORD PTR 50h[rsp], r15
	movsd QWORD PTR 58h[rsp], xmm6
	movsd QWORD PTR 60h[rsp], xmm7
	movsd QWORD PTR 68h[rsp], xmm8
	movsd QWORD PTR 70h[rsp], xmm9
	movsd QWORD PTR 78h[rsp], xmm10
	movsd QWORD PTR 80h[rsp], xmm11
	movsd QWORD PTR 88h[rsp], xmm12
	movsd QWORD PTR 90h[rsp], xmm13
	movsd QWORD PTR 98h[rsp], xmm14
	movsd QWORD PTR 00a0h[rsp], xmm15
	.ENDPROLOG
	; begin
	mov QWORD PTR 00h[rsp], rcx
	mov QWORD PTR 08h[rsp], rdx
	call r8
	; end
	mov rbx, QWORD PTR 20h[rsp]
	mov rsi, QWORD PTR 28h[rsp]
	mov rdi, QWORD PTR 30h[rsp]
	mov r12, QWORD PTR 38h[rsp]
	mov r13, QWORD PTR 40h[rsp]
	mov r14, QWORD PTR 48h[rsp]
	mov r15, QWORD PTR 50h[rsp]
	movsd xmm6, QWORD PTR 58h[rsp]
	movsd xmm7, QWORD PTR 60h[rsp]
	movsd xmm8, QWORD PTR 68h[rsp]
	movsd xmm9, QWORD PTR 70h[rsp]
	movsd xmm10, QWORD PTR 78h[rsp]
	movsd xmm11, QWORD PTR 80h[rsp]
	movsd xmm12, QWORD PTR 88h[rsp]
	movsd xmm13, QWORD PTR 90h[rsp]
	movsd xmm14, QWORD PTR 98h[rsp]
	movsd xmm15, QWORD PTR 00a0h[rsp]
	add rsp, 00a8h
	ret 0
Call2Asm ENDP
_TEXT ENDS

_TEXT SEGMENT
Call3Asm PROC FRAME
	sub rsp, 00a8h
	.ALLOCSTACK 00a8h
	mov QWORD PTR 20h[rsp], rbx
	mov QWORD PTR 28h[rsp], rsi
	mov QWORD PTR 30h[rsp], rdi
	mov QWORD PTR 38h[rsp], r12
	mov QWORD PTR 40h[rsp], r13
	mov QWORD PTR 48h[rsp], r14
	mov QWORD PTR 50h[rsp], r15
	movsd QWORD PTR 58h[rsp], xmm6
	movsd QWORD PTR 60h[rsp], xmm7
	movsd QWORD PTR 68h[rsp], xmm8
	movsd QWORD PTR 70h[rsp], xmm9
	movsd QWORD PTR 78h[rsp], xmm10
	movsd QWORD PTR 80h[rsp], xmm11
	movsd QWORD PTR 88h[rsp], xmm12
	movsd QWORD PTR 90h[rsp], xmm13
	movsd QWORD PTR 98h[rsp], xmm14
	movsd QWORD PTR 00a0h[rsp], xmm15
	.ENDPROLOG
	; begin
	mov QWORD PTR 00h[rsp], rcx
	mov QWORD PTR 08h[rsp], rdx
	mov QWORD PTR 10h[rsp], r8
	call r9
	; end
	mov rbx, QWORD PTR 20h[rsp]
	mov rsi, QWORD PTR 28h[rsp]
	mov rdi, QWORD PTR 30h[rsp]
	mov r12, QWORD PTR 38h[rsp]
	mov r13, QWORD PTR 40h[rsp]
	mov r14, QWORD PTR 48h[rsp]
	mov r15, QWORD PTR 50h[rsp]
	movsd xmm6, QWORD PTR 58h[rsp]
	movsd xmm7, QWORD PTR 60h[rsp]
	movsd xmm8, QWORD PTR 68h[rsp]
	movsd xmm9, QWORD PTR 70h[rsp]
	movsd xmm10, QWORD PTR 78h[rsp]
	movsd xmm11, QWORD PTR 80h[rsp]
	movsd xmm12, QWORD PTR 88h[rsp]
	movsd xmm13, QWORD PTR 90h[rsp]
	movsd xmm14, QWORD PTR 98h[rsp]
	movsd xmm15, QWORD PTR 00a0h[rsp]
	add rsp, 00a8h
	ret 0
Call3Asm ENDP
_TEXT ENDS

END
