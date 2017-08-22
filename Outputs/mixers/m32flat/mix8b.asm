; ASM mixing code
;
; MixAsm_20_xx - For 2.0 <= x incrs
;    Shuffles the code around to optimize for the constant motion using a
;    variable whole number.
;
; MixAsm_10_20 - For 1.0 <= x < 2.0 incrs
;    Shuffles the code around to optimize for the constant motion, uses a
;    whole number portion of 1.
;
; MixAsm_05_10 - For 0.5 <= x < 1.0 incrs
;    Takes advantage of the fact that a secondary addition does not need to be
;    done due to the 0 whole number portion.
;
; MixAsm_03_05 - For 0.3 <= x < 0.5 incrs
;    Takes advantage of the fact that in this number range a pattern of
;    2 loops and 3 loops, ie 2,3,2,2,3,2,2,3. varies with the counter.
;    Since it never goes below 2 loops it is safe to combine two loops into
;    one and do the math that way.
;
; MixAsm_00_03 - For 0.0 <= x < 0.3 incrs
;    Takes advantage of the fact that in this number range a pattern of
;    3 loops and 4 loops, ie 3,4,3,3,4,3,3,4. varies with the counter.
;    Since it never goes below 3 loops it is safe to combine three loops into
;    one and do the math that way.

ideal                                     ; TASM Ideal ode
p386                                      ; protected mode 386 code
model os2 flat, syscall                   ; os/2 flat model, system calling
nosmart

public syscall MixAsm_20_xx               ; Export the symbol
public syscall MixAsm_10_20               ; Export the symbol
public syscall MixAsm_05_10               ; Export the symbol
public syscall MixAsm_03_05               ; Export the symbol
public syscall MixAsm_00_03               ; Export the symbol
public syscall MixAsm_20_xx_R             ; Export the symbol
public syscall MixAsm_10_20_R             ; Export the symbol
public syscall MixAsm_05_10_R             ; Export the symbol
public syscall MixAsm_03_05_R             ; Export the symbol
public syscall MixAsm_00_03_R             ; Export the symbol

dataseg
   StackSave DD ?                         ; Place to store esp
   VolTable DD ?                          ; Place to store VolMap
   MulIncr DD ?                           ; Place to store x*Incr
   WholeIncr DD ?                         ; Place to store Add
   JumpTarget DD ?

codeseg                                   ; Start the code segment

FORWARD  = 1
BACKWARD = 2


DIRECTION = FORWARD
include "Mix8b.inc"

DIRECTION = BACKWARD
include "Mix8b.inc"


end
