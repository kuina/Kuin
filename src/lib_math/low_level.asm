PUBLIC AddAsm
PUBLIC SubAsm
PUBLIC MulAsm

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
SubAsm PROC
	; begin
	sub QWORD PTR 00h[rcx], rdx
	jno SHORT $lbl1@SubAsm
	mov rax, 01h
	jmp SHORT $lbl2@SubAsm
$lbl1@SubAsm:
	xor eax, eax
$lbl2@SubAsm:
	; end
	ret 0
SubAsm ENDP
_TEXT ENDS

_TEXT SEGMENT
MulAsm PROC
	; begin
	mov r8, QWORD PTR 00h[rcx]
	imul r8, rdx
	jno SHORT $lbl1@MulAsm
	mov rax, 01h
	jmp SHORT $lbl2@MulAsm
$lbl1@MulAsm:
	xor eax, eax
$lbl2@MulAsm:
	mov QWORD PTR 00h[rcx], r8
	; end
	ret 0
MulAsm ENDP
_TEXT ENDS

END
