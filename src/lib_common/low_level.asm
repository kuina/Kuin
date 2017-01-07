PUBLIC ToBinClassAsm
PUBLIC FromBinClassAsm
PUBLIC CmpClassAsm
PUBLIC DtorClassAsm
PUBLIC CopyClassAsm
PUBLIC AddAsm
PUBLIC MulAsm

_TEXT SEGMENT
ToBinClassAsm PROC
	sub rsp, 00a8h
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
	; begin
	inc QWORD PTR 00h[rcx]
	mov QWORD PTR 00h[rsp], rcx
	call QWORD PTR 30h[rcx]
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
ToBinClassAsm ENDP
_TEXT ENDS

_TEXT SEGMENT
FromBinClassAsm PROC
	sub rsp, 00a8h
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
	; begin
	inc QWORD PTR 00h[rcx]
	mov QWORD PTR 00h[rsp], rcx
	inc QWORD PTR 00h[rdx]
	mov QWORD PTR 08h[rsp], rdx
	mov QWORD PTR 10h[rsp], r8
	call QWORD PTR 38h[rcx]
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
FromBinClassAsm ENDP
_TEXT ENDS

_TEXT SEGMENT
CmpClassAsm PROC
	sub rsp, 00a8h
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
	; begin
	inc QWORD PTR 00h[rcx]
	mov QWORD PTR 00h[rsp], rcx
	inc QWORD PTR 00h[rdx]
	mov QWORD PTR 08h[rsp], rdx
	call QWORD PTR 20h[rcx]
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
CmpClassAsm ENDP
_TEXT ENDS

_TEXT SEGMENT
DtorClassAsm PROC
	sub rsp, 00a8h
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
	; begin
	mov QWORD PTR 00h[rcx], 02h
	mov QWORD PTR 00h[rsp], rcx
	call QWORD PTR 18h[rcx]
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
DtorClassAsm ENDP
_TEXT ENDS

_TEXT SEGMENT
CopyClassAsm PROC
	sub rsp, 00a8h
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
	; begin
	inc QWORD PTR 00h[rcx]
	mov QWORD PTR 00h[rsp], rcx
	call QWORD PTR 28h[rcx]
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
CopyClassAsm ENDP
_TEXT ENDS

_TEXT SEGMENT
AddAsm PROC
	; begin
	add QWORD PTR 00h[rcx], rdx
	jno SHORT $lbl1@AddAsm
	mov rax, 01h
	jmp SHORT $lbl2@AddAsm
$lbl1@AddAsm:
	xor eax, eax
$lbl2@AddAsm:
	; end
	ret 0
AddAsm ENDP
_TEXT ENDS

_TEXT SEGMENT
MulAsm PROC
	; begin
	mov r8, QWORD PTR 00h[rcx]
	imul r8, rdx
	jno SHORT $lbl1@AddAsm
	mov rax, 01h
	jmp SHORT $lbl2@AddAsm
$lbl1@AddAsm:
	xor eax, eax
$lbl2@AddAsm:
	mov QWORD PTR 00h[rcx], r8
	; end
	ret 0
MulAsm ENDP
_TEXT ENDS

END
