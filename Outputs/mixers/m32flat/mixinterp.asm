;
; ASM mixing code
;

ideal                                     ; TASM Ideal ode
p386                                      ; protected mode 386 code
model os2 flat, syscall                   ; os/2 flat model, system calling
nosmart

public syscall MixIntAsm
public syscall MixIntAsmR
public syscall MixIntAsm_Si
public syscall MixIntAsmR_Si
public syscall MixIntAsm_16
public syscall MixIntAsmR_16

dataseg
   StackSave    DD ?
   VolTable     DD ?
   MulIncr      DD ?
   WholeIncr    DD ?
   Fraction     DD ?
   FirstPos     DD ?
   PrevPos      DD ?
   LastPos      DD ?
   LastVal      DD ?

   FromL        DD ?
   FromR        DD ?
   StoreEnd     DD ?
   
codeseg                                   ; Start the code segment

MACRO MIXCODE

; Main mixer
IF DIRECTION EQ FORWARD
 IF MODE EQ UNSIGNED
proc MixIntAsm syscall
 ELSEIF MODE EQ SIGNED
proc MixIntAsm_Si syscall
 ELSE
proc MixIntAsm_16 syscall
 ENDIF
ELSE
 IF MODE EQ UNSIGNED
proc MixIntAsmR syscall
 ELSEIF MODE EQ SIGNED
proc MixIntAsmR_Si syscall
 ELSE         
proc MixIntAsmR_16 syscall
 ENDIF
ENDIF
ARG @@MCur:PTR DWORD,@@End:PTR DWORD,@@Incr:DWORD,@@CurIncr:DWORD, \
    @@Add:DWORD,@@PlayPos:PTR BYTE,@@VolMap:PTR DWORD, \
    @@FirstPos:DWORD,@@PrevPos:DWORD,@@LastPos:PTR DWORD

   ; Save volatile registers
   pushad
   push ebp

   ; Save esp and store MEnd
   mov [StackSave],esp                    ; Stash esp in the dgroup
   mov eax,[@@VolMap]
   mov [VolTable], eax
   mov eax,[@@Add]
IF MODE EQ _16BIT
   add eax, eax
ENDIF
   mov [WholeIncr],eax                    ; Stash Add too

   ; Register map:
   mov ecx,[@@CurIncr]                    ; Current Incr
   mov ebx,[@@Incr]                       ; Incr
   mov [Fraction], ebx
   mov edx,[@@MCur]                       ; MCur
   mov edi,[@@PlayPos]                    ; edi - PlayPos
   mov esp,[@@End]                        ; esi - Volume map
   mov eax,[@@FirstPos]
   inc eax
   shl eax, 3
   mov [FirstPos], eax
   mov eax,[@@PrevPos]
   inc eax
   shl eax, 3
   mov [PrevPos], eax
   sub esp, eax
   mov eax,[@@LastPos]
   mov [LastPos], eax
IF MODE EQ UNSIGNED
   movzx eax, [BYTE eax]
   sub eax, 80h
ELSEIF MODE EQ SIGNED
   movsx eax, [BYTE eax]
ELSE
   movsx eax, [WORD eax]
ENDIF
   mov [LastVal], eax
   
   mov esi,edx                            ; Put MCur into esp
   mov eax, [VolTable]
   mov ebp, eax

   mov [StoreEnd], esp

   mov eax, [FirstPos]
   add eax, esi
   cmp esp, eax
   jbe @@Top2

;;int 3
   
;;Pass 1, everything from the beginning of mixing to the first source change
   mov esp, [FirstPos]
   add esp, esi
   
@@Top1:
   cmp esi, esp
   jae @@End1
IF MODE EQ UNSIGNED
   movzx ebx, [BYTE edi]
   sub ebx, 80h
ELSEIF MODE EQ SIGNED
   movsx ebx, [BYTE edi]
ELSE
   movsx ebx, [WORD edi]
ENDIF
   mov edx, ebx
   imul ebx, [ebp-8-8*2]
   imul edx, [ebp-4-8*2]
   mov [FromL], ebx
   mov [FromR], edx

;   cmp edi, [LastPos]
;   jne @@GetByte
;   mov ebx, [LastVal]
;   jmp @@GotByte
;@@GetByte:
IF DIRECTION EQ FORWARD
 IF MODE EQ UNSIGNED
   movzx ebx, [BYTE edi+1]
   sub ebx, 80h
 ELSEIF MODE EQ SIGNED
   movsx ebx, [BYTE edi+1]
 ELSE
   movsx ebx, [WORD edi+2]
 ENDIF
ELSE
 IF MODE EQ UNSIGNED
   movzx ebx, [BYTE edi-1]
   sub ebx, 80h
 ELSEIF MODE EQ SIGNED
   movsx ebx, [BYTE edi-1]
 ELSE
   movsx ebx, [WORD edi-2]
 ENDIF
ENDIF
;@@GotByte:

   mov eax, ebx
   mov edx, ecx
   imul eax, [ebp-8]
   shr edx, 17
   imul ebx, [ebp-4]
   
   sub eax, [FromL]
   imul edx
   shrd eax, edx, 15
   mov edx, ecx
   add [FromL], eax
   shr edx, 17
   mov eax, ebx
   sub eax, [FromR]
   imul edx
   shrd eax, edx, 15
   add eax, [FromR]
   mov ebx, [FromL]

   add [esi+4],eax                        ; Add right channel to buffer
   add [esi],ebx                          ; Add Left channel to buffer
   add esi, 8

IF MODE EQ _16BIT
   xor eax, eax
   add ecx,[Fraction]                     ; Add the lower portion of the fraction
   setc al
   mov ebx,[WholeIncr]
   lea eax, [ebx+eax*2]
 IF DIRECTION EQ FORWARD
   add edi,eax                            ; Move play pos up 1
 ELSE
   sub edi,eax                            ; Move play pos up 1
 ENDIF
ELSE
   add ecx,[Fraction]                     ; Add the lower portion of the fraction
 IF DIRECTION EQ FORWARD
   adc edi,[WholeIncr]                    ; Move play pos up 1
 ELSE
   sbb edi,[WholeIncr]                    ; Move play pos up 1
 ENDIF
ENDIF

   jmp @@Top1

@@End1:
  
   
   mov esp, [StoreEnd]
;;Pass 2, everything up to mixing from the last byte
@@Top2:
   cmp esi,esp                            ; Check to see if the end was reached
   jae @@End2

   mov edx, ecx
IF MODE EQ UNSIGNED
   movzx ebx, [BYTE edi]
ELSEIF MODE EQ SIGNED
   movsx ebx, [BYTE edi]
ELSE
   movsx ebx, [WORD edi]
ENDIF
IF DIRECTION EQ FORWARD
 IF MODE EQ UNSIGNED
   movzx eax, [BYTE edi+1]
 ELSEIF MODE EQ SIGNED
   movsx eax, [BYTE edi+1]
 ELSE
   movsx eax, [WORD edi+2]
 ENDIF
ELSE
 IF MODE EQ UNSIGNED
   movzx eax, [BYTE edi-1]
 ELSEIF MODE EQ SIGNED
   movsx eax, [BYTE edi-1]
 ELSE
   movsx eax, [WORD edi-2]
 ENDIF
ENDIF
   shr edx, 17
   sub eax, ebx
   imul edx
IF MODE EQ _16BIT
   sar eax, 15
   add ebx, eax
ELSE
   sar eax, 15-8
   add bl, ah
ENDIF
IF MODE EQ SIGNED
   and EBX, 000000FFh
ENDIF
IF MODE EQ _16BIT
   mov edx, ebx
   imul ebx, [ebp-8]
   imul edx, [ebp-4]
ELSE
   lea eax, [ebx*8+ebp]
   mov ebx,[eax]                          ; Index volume table, Left
   mov edx,[eax+4]                        ; Index volume table, Right
ENDIF

   add [esi],ebx                          ; Add Left channel to buffer
   add [esi+4],edx                        ; Add right channel to buffer
   add esi, 8
   
IF MODE EQ _16BIT
   xor eax, eax
   add ecx,[Fraction]                     ; Add the lower portion of the fraction
   setc al
   mov ebx,[WholeIncr]
   lea eax, [ebx+eax*2]
 IF DIRECTION EQ FORWARD
   add edi,eax                            ; Move play pos up 1
 ELSE
   sub edi,eax                            ; Move play pos up 1
 ENDIF
ELSE
   add ecx,[Fraction]                     ; Add the lower portion of the fraction
 IF DIRECTION EQ FORWARD
   adc edi,[WholeIncr]                    ; Move play pos up 1
 ELSE
   sbb edi,[WholeIncr]                    ; Move play pos up 1
 ENDIF
ENDIF
   jmp @@Top2

@@End2:


;;Pass 3, everything from mixing from the last byte to the end of mixing
   add esp, [PrevPos]

@@Top3:
   cmp esi, esp
   jae @@End3
IF MODE EQ UNSIGNED
   movzx ebx, [BYTE edi]
ELSEIF MODE EQ SIGNED
   movzx ebx, [BYTE edi]
ELSE
   movsx ebx, [WORD edi]
ENDIF
IF MODE EQ _16BIT
   mov edx, ebx
   imul ebx, [ebp-8]
   imul edx, [ebp-4]
ELSE
   lea eax, [ebx*8+ebp]
   mov ebx,[eax]                          ; Index volume table, Left
   mov edx,[eax+4]                        ; Index volume table, Right
ENDIF
   mov [FromL], ebx
   mov [FromR], edx

   mov ebx, [LastVal]
   mov eax, ebx
   mov edx, ecx
   imul eax, [ebp-8-8]
   shr edx, 17
   imul ebx, [ebp-4-8]
   
   sub eax, [FromL]
   imul edx
   shrd eax, edx, 15
   mov edx, ecx
   add [FromL], eax
   shr edx, 17
   mov eax, ebx
   sub eax, [FromR]
   imul edx
   shrd eax, edx, 15
   add eax, [FromR]
   mov ebx, [FromL]

   add [esi+4],eax                        ; Add right channel to buffer
   add [esi],ebx                          ; Add Left channel to buffer
   add esi, 8

IF MODE EQ _16BIT
   xor eax, eax
   add ecx,[Fraction]                     ; Add the lower portion of the fraction
   setc al
   mov ebx,[WholeIncr]
   lea eax, [ebx+eax*2]
 IF DIRECTION EQ FORWARD
   add edi,eax                            ; Move play pos up 1
 ELSE
   sub edi,eax                            ; Move play pos up 1
 ENDIF
ELSE
   add ecx,[Fraction]                     ; Add the lower portion of the fraction
 IF DIRECTION EQ FORWARD
   adc edi,[WholeIncr]                    ; Move play pos up 1
 ELSE
   sbb edi,[WholeIncr]                    ; Move play pos up 1
 ENDIF
ENDIF

   jmp @@Top3

@@End3:

   ; Restore the stack
   mov esp,[StackSave]

   ; Restore the flags
   pop ebp
   popad
   cld

   ret

endp

ENDM



FORWARD  = 1
BACKWARD = 2

UNSIGNED = 1
SIGNED   = 2
_16BIT   = 3


MODE = UNSIGNED
DIRECTION = FORWARD
MIXCODE
DIRECTION = BACKWARD
MIXCODE

MODE = SIGNED
DIRECTION = FORWARD
MIXCODE
DIRECTION = BACKWARD
MIXCODE

MODE = _16BIT
DIRECTION = FORWARD
MIXCODE
DIRECTION = BACKWARD
MIXCODE


end
