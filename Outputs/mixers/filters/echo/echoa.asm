ideal                                     ; TASM Ideal mode
p386                                      ; protected mode 386 code
model os2 flat, syscall                   ; os/2 flat model, system calling
nosmart

public syscall EchoMix1m
public syscall EchoMix2m
public syscall EchoMix3m
public syscall EchoMix4m
public syscall EchoMix5m
public syscall EchoMix6m
public syscall EchoMix7m
public syscall EchoMix8m
public syscall EchoMix1s
public syscall EchoMix2s
public syscall EchoMix3s
public syscall EchoMix4s
public syscall EchoMix5s
public syscall EchoMix6s
public syscall EchoMix7s
public syscall EchoMix8s
public syscall FeedbackMixS
public syscall FeedbackMixM

dataseg
StackSave			DD ?

codeseg                                      ; Start the code segment

MONO       = 1
STEREO     = 2

MACRO MIX1CODE

   mov EAX, [ESI]
   mov EBP, [EDI]
   imul EBX
   mov [ESP], EBP
   shrd EAX, EDX, 16
   add ESI, MOVE
   add [EDI], EAX
   add ESP, MOVE
   add EDI, MOVE

@@MadeMono:
ENDM

MACRO MIX2CODE

   mov ESP, [ESI+4]
   mov EBP, [EDI]
   mov EDX, [EBX+4]
   mov EAX, [ESP]
   add [DWORD ESI+4], MOVE
   imul EDX
   mov ESP, [ESI]
   shrd EAX, EDX, 16
   mov EDX, [EBX]
   add [EDI], EAX

   mov EAX, [ESP]
   add [DWORD ESI], MOVE
   imul EDX
   mov ESP, [ESI+4*8]
   shrd EAX, EDX, 16
   mov [ESP], EBP
   add [EDI], EAX
   add [DWORD ESI+4*8], MOVE
   add EDI, MOVE
   
ENDM

MACRO MIXNCODE NUMBER

   mov ESP, [ESI+(NUMBER-1)*4]
   mov EBP, [EDI]
   mov EDX, [EBX+(NUMBER-1)*4]
   mov EAX, [ESP]
   add [DWORD ESI+(NUMBER-1)*4], MOVE
   imul EDX
   mov ESP, [ESI+(NUMBER-2)*4]
   shrd EAX, EDX, 16
   mov EDX, [EBX+(NUMBER-2)*4]
   add [EDI], EAX

CURNUM = (NUMBER-2)
REPT (NUMBER-2)
LOCAL @@NoMakeMonox
   mov EAX, [ESP]
   add [DWORD ESI+CURNUM*4], MOVE
   imul EDX
   mov ESP, [ESI+(CURNUM-1)*4]
   shrd EAX, EDX, 16
   mov EDX, [EBX+(CURNUM-1)*4]
   add [EDI], EAX
CURNUM=CURNUM-1
ENDM

   mov EAX, [ESP]
   add [DWORD ESI], MOVE
   imul EDX
   mov ESP, [ESI+4*8]
   shrd EAX, EDX, 16
   mov [ESP], EBP
   add [EDI], EAX
   add [DWORD ESI+4*8], MOVE
   add EDI, MOVE
   
ENDM


MIXTYPE=MONO
MIXNUM=1
REPT 8
INCLUDE "echoa.inc"
MIXNUM=MIXNUM+1
ENDM

MIXTYPE=STEREO
MIXNUM=1
REPT 8
INCLUDE "echoa.inc"
MIXNUM=MIXNUM+1
ENDM



proc FeedbackMixS syscall
ARG @@ToPos:PTR DWORD,@@FromPos:PTR DWORD,@@Length:DWORD,@@Scale:DWORD

   ; Save volatile registers
   pushad
   push ebp

   ; Save esp and store MEnd
   mov [StackSave],esp                    ; Stash esp in the dgroup

   mov ESI, [@@FromPos]
   mov EDI, [@@ToPos]
   mov ECX, [@@Length]
   mov EBX, [@@Scale]
   shr ECX, 1
   jz @@End
   
@@Top:
   mov EAX, [ESI]
   sub ESI, 8
   imul EBX
   shrd EAX, EDX, 16
   add [EDI], EAX
   sub EDI, 8
   dec ECX
   jnz @@Top

@@End:

   ; Restore the stack
   mov esp,[StackSave]

   ; Restore the flags
   pop ebp
   popad
   cld

   ret

endp

proc FeedbackMixM syscall
ARG @@ToPos:PTR DWORD,@@FromPos:PTR DWORD,@@Length:DWORD,@@Scale:DWORD

   ; Save volatile registers
   pushad
   push ebp

   ; Save esp and store MEnd
   mov [StackSave],esp                    ; Stash esp in the dgroup

   mov ESI, [@@FromPos]
   mov EDI, [@@ToPos]
   mov ECX, [@@Length]
   mov EBX, [@@Scale]
   test ECX, ECX
   jz @@End
   
@@Top:
   mov EAX, [ESI]
   sub ESI, 4
   imul EBX
   shrd EAX, EDX, 16
   add [EDI], EAX
   sub EDI, 4
   dec ECX
   jnz @@Top

@@End:

   ; Restore the stack
   mov esp,[StackSave]

   ; Restore the flags
   pop ebp
   popad
   cld

   ret

endp

end
