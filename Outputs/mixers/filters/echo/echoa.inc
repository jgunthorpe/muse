
IF MIXTYPE EQ MONO
 IF MIXNUM EQ 1
proc EchoMix1m syscall
 ELSEIF MIXNUM EQ 2
proc EchoMix2m syscall
 ELSEIF MIXNUM EQ 3
proc EchoMix3m syscall
 ELSEIF MIXNUM EQ 4
proc EchoMix4m syscall
 ELSEIF MIXNUM EQ 5
proc EchoMix5m syscall
 ELSEIF MIXNUM EQ 6
proc EchoMix6m syscall
 ELSEIF MIXNUM EQ 7
proc EchoMix7m syscall
 ELSEIF MIXNUM EQ 8
proc EchoMix8m syscall
 ENDIF
ELSE
 IF MIXNUM EQ 1
proc EchoMix1s syscall
 ELSEIF MIXNUM EQ 2
proc EchoMix2s syscall
 ELSEIF MIXNUM EQ 3
proc EchoMix3s syscall
 ELSEIF MIXNUM EQ 4
proc EchoMix4s syscall
 ELSEIF MIXNUM EQ 5
proc EchoMix5s syscall
 ELSEIF MIXNUM EQ 6
proc EchoMix6s syscall
 ELSEIF MIXNUM EQ 7
proc EchoMix7s syscall
 ELSEIF MIXNUM EQ 8
proc EchoMix8s syscall
 ENDIF
ENDIF
ARG @@Pos:PTR DWORD,@@MixSize:DWORD,@@PosBuf:PTR DWORD,@@Scale:PTR DWORD
   ; Save volatile registers
   pushad
   push ebp

   ; Save esp and store MEnd
   mov [StackSave],esp                    ; Stash esp in the dgroup

   mov EDI, [@@Pos]
   mov ECX, [@@MixSize]
   mov ESI, [@@PosBuf]
IF MIXTYPE EQ STEREO
   shr ECX, 1
ENDIF
   mov EBX, [@@Scale]
IF MIXNUM EQ 1
   mov ESP, [ESI+4*8]
   mov ESI, [ESI]
   mov EBX, [EBX]
ENDIF

@@Top:

IF MIXTYPE EQ MONO
   MOVE=4
ELSE
   MOVE=8
ENDIF

IF MIXNUM EQ 1
   MIX1CODE
ELSEIF MIXNUM EQ 2
   MIX2CODE
ELSE
   MIXNCODE MIXNUM
ENDIF

   dec ECX
   jnz @@Top
   
   ; Restore the stack
   mov esp,[StackSave]

   ; Restore the flags
   pop ebp
   popad
   cld

   ret

endp
