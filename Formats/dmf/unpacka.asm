
;
; ASM Huffman unpacking code
;

ideal                                     ; TASM Ideal mode
p386                                      ; protected mode 386 code
model os2 flat, syscall                   ; os/2 flat model, system calling
nosmart

dataseg
StackSave			DD ?
DestEnd                         DD ?
Result                          DD ?
BitTable                        DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                DB 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h, 01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
                                
IndexTable                      DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
                                DD 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1

struc Tree
Left        DW ?
Right       DW ?
Children    DW ?
Value       DB ?
Discard     DB ?
ends

codeseg                                      ; Start the code segment

public syscall HuffmanUnpackAsm


proc HuffmanUnpackAsm syscall
ARG @@Packed:PTR DWORD, @@SourceEnd:PTR DWORD, @@Sample:PTR DWORD, @@DestEnd:PTR DWORD, @@Tree:PTR DWORD, @@StartBit:BYTE

   ; Save volatile registers
   pushad
   push ebp

   ; Save esp and store MEnd
   mov [StackSave],esp                    ; Stash esp in the dgroup

   mov ESI, [@@Packed]
   mov EDI, [@@Sample]
   mov EAX, [@@DestEnd]
   mov [DestEnd], EAX
   mov EAX, [@@SourceEnd]
   xor EDX, EDX
   mov DL, [@@StartBit]
   mov EBP, [@@Tree]
   mov ESP, EAX
   xor EBX, EBX

@@TopOfUnpack:
   mov AH, [ESI]
   add ESI, [IndexTable+EDX*4]
   and AH, [BitTable+EDX]
   jz @@NoSign
   mov AH, 0FFh
@@NoSign:

   inc DL
   xor ECX, ECX
@@TopOfTree:
   mov AL, [ESI]
   add ESI, [IndexTable+EDX*4]
   and AL, [BitTable+EDX]
   jnz @@RightBranch
   ;;Take the left branch
   mov CX, [EBP+ECX*8+0]
   inc DL
   cmp [WORD EBP+ECX*8+4], 0
   jne @@TopOfTree
   jmp @@BottomOfTree

@@RightBranch:
   mov CX, [EBP+ECX*8+2]
   inc DL
   cmp [WORD EBP+ECX*8+4], 0
   jne @@TopOfTree
@@BottomOfTree:
   mov AL, [EBP+ECX*8+6]
   xor AL, AH
   add BL, AL
   mov [EDI], BL
   inc EDI
   cmp ESI, ESP
   jae @@NoMoreData
   cmp EDI, [DestEnd]
   jb @@TopOfUnpack
@@NoMoreData:

   cmp EDI, [DestEnd]
   jae @@NoEndFill
   mov ECX, [DestEnd]
   sub ECX, EDI
   cmp ECX, 16
   ja @@NoEndFill
   mov AL, [EDI-1]
@@TopOfEndFill:
   mov [EDI], AL
   inc EDI
   dec ECX
   jnz @@TopOfEndFill
   
@@NoEndFill:

   mov [Result], 0
   cmp ESI, ESP
   jb @@OKEnd
   ja @@BadEnd
   and DL, 7
   jz @@OKEnd
@@BadEnd:
   cmp EDI, [DestEnd]
   jae @@OKEnd
   mov [Result], 1
@@OKEnd:
   
   ; Restore the stack
   mov esp,[StackSave]

   ; Restore the flags
   pop ebp
   popad
   mov EAX, [Result]
   cld

   ret

endp

end
