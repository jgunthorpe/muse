; 2.0 to xx mixer
IF DIRECTION EQ FORWARD
proc MixAsm_20_xx syscall
ELSE
proc MixAsm_20_xx_R syscall
ENDIF
ARG @@MCur:PTR DWORD,@@End:PTR DWORD,@@Incr:DWORD,@@CurIncr:DWORD, \
    @@Add:DWORD,@@PlayPos:PTR BYTE,@@VolMap:PTR DWORD

   ; Save volitile registers
   pushad
   push ebp

   ; Save esp and store MEnd
   mov [StackSave],esp                    ; Stash esp in the dgroup
   mov eax,[@@VolMap]
   mov [VolTable],eax                     ; Stash VolMap too
   mov eax,[@@Add]
   mov [WholeIncr],eax                    ; Stash Add too

   ; Register map:
   mov eax,[@@CurIncr]                    ; eax - Current Incr
   mov ebx,[@@Incr]                       ; ebx - Incr
   mov ecx,[@@MCur]                       ; esp - MCur
   mov edi,[@@PlayPos]                    ; edi - PlayPos
   mov esi,[@@End]                        ; esi - Volume map

   mov esp,ecx                            ; Put MCur into esp

   mov ebp,[VolTable]

   mov ecx, esi
   sub ecx, esp
   shr ecx, 3
   and ecx, 3
   mov ecx, [@@JumpTable+ecx*4]
   mov [JumpTarget], ecx
   
   ; Preload the current #
   mov dl,[edi]                           ; Store *PlayPos
   lea edx, [ebp+edx*8]
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right

   jmp [JumpTarget]
   ; Start of main loop
@@Top:

   add [esp+4],edx                        ; Add right channel to buffer

   add eax,ebx                            ; Add the lower portion of the fraction
IF DIRECTION EQ FORWARD
   adc edi,[WholeIncr]                    ; Move play pos up 1
ELSE
   sbb edi,[WholeIncr]                    ; Move play pos up 1
ENDIF
   xor edx, edx                           ; Zero our temp var
   add [esp],ecx                          ; Add Left channel to buffer
   mov dl,[edi]                           ; Store *PlayPos
   add esp,8                              ; Move Buffer pointer
   lea edx, [ebp+edx*8]
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right

@@In3:
   add [esp+4],edx                        ; Add right channel to buffer

   add eax,ebx                            ; Add the lower portion of the fraction
IF DIRECTION EQ FORWARD
   adc edi,[WholeIncr]                    ; Move play pos up 1
ELSE
   sbb edi,[WholeIncr]                    ; Move play pos up 1
ENDIF
   xor edx, edx                           ; Zero our temp var
   add [esp],ecx                          ; Add Left channel to buffer
   mov dl,[edi]                           ; Store *PlayPos
   add esp,8                              ; Move Buffer pointer
   lea edx, [ebp+edx*8]
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right

@@In2:
   add [esp+4],edx                        ; Add right channel to buffer

   add eax,ebx                            ; Add the lower portion of the fraction
IF DIRECTION EQ FORWARD
   adc edi,[WholeIncr]                    ; Move play pos up 1
ELSE
   sbb edi,[WholeIncr]                    ; Move play pos up 1
ENDIF
   xor edx, edx                           ; Zero our temp var
   add [esp],ecx                          ; Add Left channel to buffer
   mov dl,[edi]                           ; Store *PlayPos
   add esp,8                              ; Move Buffer pointer
   lea edx, [ebp+edx*8]
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right

@@In1:
   add [esp+4],edx                        ; Add right channel to buffer

   add eax,ebx                            ; Add the lower portion of the fraction
IF DIRECTION EQ FORWARD
   adc edi,[WholeIncr]                    ; Move play pos up 1
ELSE
   sbb edi,[WholeIncr]                    ; Move play pos up 1
ENDIF
   xor edx, edx                           ; Zero our temp var
   add [esp],ecx                          ; Add Left channel to buffer
   mov dl,[edi]                           ; Store *PlayPos
   add esp,8                              ; Move Buffer pointer
   lea edx, [ebp+edx*8]
   cmp esp,esi                            ; Check to see if the end was reached
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right
   jl @@Top                               ; Boing back up

@@NoTop:

   ; Restore the stack
   mov esp,[StackSave]

   ; Restore the flags
   pop ebp
   popad
   cld

   ret

   @@JumpTable DD OFFSET @@Top, OFFSET @@In1, OFFSET @@In2, OFFSET @@In3
   
endp

; 1.0 to 2.0 mixer
IF DIRECTION EQ FORWARD
proc MixAsm_10_20 syscall
ELSE
proc MixAsm_10_20_R syscall
ENDIF
ARG @@MCur:PTR DWORD,@@End:PTR DWORD,@@Incr:DWORD,@@CurIncr:DWORD, \
    @@Add:DWORD,@@PlayPos:PTR BYTE,@@VolMap:PTR DWORD

   ; Save volitile registers
   pushad
   push ebp

   ; Save esp and store MEnd
   mov [StackSave],esp                    ; Stash esp in the dgroup
   mov eax,[@@VolMap]
   mov [VolTable],eax                     ; Stash VolMap too
   mov eax,[@@Add]
   mov [WholeIncr],eax                    ; Stash Add too

   ; Register map:
   mov eax,[@@CurIncr]                    ; eax - Current Incr
   mov ebx,[@@Incr]                       ; ebx - Incr
   mov ecx,[@@MCur]                       ; esp - MCur
   mov edi,[@@PlayPos]                    ; edi - PlayPos
   mov esi,[@@End]                        ; esi - Volume map

   mov esp,ecx                            ; Put MCur into esp

   mov ebp,[VolTable]

   mov ecx, esi
   sub ecx, esp
   shr ecx, 3
   and ecx, 3
   mov ecx, [@@JumpTable+ecx*4]
   mov [JumpTarget], ecx
   
   ; Preload the current #
   mov dl,[edi]                           ; Store *PlayPos
   lea edx, [ebp+edx*8]
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right

   jmp [JumpTarget]
   ; Start of main loop
@@Top:

   add [esp+4],edx                        ; Add right channel to buffer

   add eax,ebx                            ; Add the lower portion of the fraction
IF DIRECTION EQ FORWARD
   adc edi,1                              ; Move play pos up 1
ELSE
   sbb edi,1                              ; Move play pos up 1
ENDIF
   xor edx, edx                           ; Zero our temp var
   add [esp],ecx                          ; Add Left channel to buffer
   mov dl,[edi]                           ; Store *PlayPos
   add esp,8                              ; Move Buffer pointer
   lea edx, [ebp+edx*8]
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right

@@In3:
   add [esp+4],edx                        ; Add right channel to buffer

   add eax,ebx                            ; Add the lower portion of the fraction
IF DIRECTION EQ FORWARD
   adc edi,1                              ; Move play pos up 1
ELSE
   sbb edi,1                              ; Move play pos up 1
ENDIF
   xor edx, edx                           ; Zero our temp var
   add [esp],ecx                          ; Add Left channel to buffer
   mov dl,[edi]                           ; Store *PlayPos
   add esp,8                              ; Move Buffer pointer
   lea edx, [ebp+edx*8]
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right

@@In2:
   add [esp+4],edx                        ; Add right channel to buffer

   add eax,ebx                            ; Add the lower portion of the fraction
IF DIRECTION EQ FORWARD
   adc edi,1                              ; Move play pos up 1
ELSE
   sbb edi,1                              ; Move play pos up 1
ENDIF
   xor edx, edx                           ; Zero our temp var
   add [esp],ecx                          ; Add Left channel to buffer
   mov dl,[edi]                           ; Store *PlayPos
   add esp,8                              ; Move Buffer pointer
   lea edx, [ebp+edx*8]
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right

@@In1:
   add [esp+4],edx                        ; Add right channel to buffer

   add eax,ebx                            ; Add the lower portion of the fraction
IF DIRECTION EQ FORWARD
   adc edi,1                              ; Move play pos up 1
ELSE
   sbb edi,1                              ; Move play pos up 1
ENDIF
   xor edx, edx                           ; Zero our temp var
   add [esp],ecx                          ; Add Left channel to buffer
   mov dl,[edi]                           ; Store *PlayPos
   add esp,8                              ; Move Buffer pointer
   lea edx, [ebp+edx*8]
   cmp esp,esi                            ; Check to see if the end was reached
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right
   jl @@Top                               ; Boing back up

@@NoTop:

   ; Restore the stack
   mov esp,[StackSave]

   ; Restore the flags
   pop ebp
   popad
   cld

   ret

   @@JumpTable DD OFFSET @@Top, OFFSET @@In1, OFFSET @@In2, OFFSET @@In3
   
endp

; 0.5 to 1.0 mixer
IF DIRECTION EQ FORWARD
proc MixAsm_05_10 syscall
ELSE
proc MixAsm_05_10_R syscall
ENDIF
ARG @@MCur:PTR DWORD,@@End:PTR DWORD,@@Incr:DWORD,@@CurIncr:DWORD, \
    @@Add:DWORD,@@PlayPos:PTR BYTE,@@VolMap:PTR DWORD

   ; Save volitile registers
   pushad
   push ebp

   ; Save esp and store MEnd
   mov [StackSave],esp                    ; Stash esp in the dgroup
   mov eax,[@@VolMap]
   mov [VolTable],eax                     ; Stash VolMap too

   ; Register map:
   mov eax,[@@CurIncr]                    ; eax - Current Incr
   mov ebx,[@@Incr]                       ; ebx - Incr
   mov ecx,[@@MCur]                       ; esp - MCur
   mov edi,[@@PlayPos]                    ; edi - PlayPos
   mov esi,[@@End]                        ; esi - Volume map

   mov esp,ecx                            ; Put MCur into esp

   mov ebp, [VolTable]
   xor edx, edx

   ; Preload the current #
   mov dl,[edi]                           ; Store *PlayPos
   lea edx, [ebp+edx*8]
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right

   ; Determine entry point
   add [esp],ecx                          ; Add Left channel to buffer
   add [esp+4],edx                        ; Add right channel to buffer
   add esp,8                              ; Move Buffer pointer
   add eax,ebx                            ; Add the lower portion of the fraction
   jc @@Carry                             ; Did we have motion of *PlayPos?

   ; Determine entry point (2)
   add [esp],ecx                          ; Add Left channel to buffer
   add [esp+4],edx                        ; Add right channel to buffer
   add esp,8                              ; Move Buffer pointer
   add eax,ebx                            ; Add the lower portion of the fraction
   jc @@Carry                             ; Did we have motion of *PlayPos?

   ; Start of main loop
@@Top:
   ; Add 1 sets
   add [esp],ecx                          ; Add Left channel to buffer
   add [esp+4],edx                        ; Add right channel to buffer
   add esp,8                              ; Move Buffer pointer
   add eax,ebx
   jc @@Carry                             ; Did we have motion of *PlayPos?

   ; This loop is used if a third loop is needed
@@Inner:
   add [esp],ecx                          ; Add Left channel to buffer
   add [esp+4],edx                        ; Add right channel to buffer
   add esp,8                              ; Move Buffer pointer

   add eax,ebx                            ; Add the lower portion of the fraction
@@Carry:
   xor edx, edx                           ; Zero our temp var
IF DIRECTION EQ FORWARD
   inc edi                                ; Move play pos up 1
ELSE
   dec edi
ENDIF
   mov dl,[edi]                           ; Store *PlayPos
   lea edx, [ebp+edx*8]
   cmp esp,esi                            ; Check to see if the end was reached
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right
   jl @@Top                               ; Boing back up

   ; Restore the stack
   mov esp,[StackSave]

   ; Restore the flags
   pop ebp
   popad
   cld

   ret
endp

; 0.33 to 0.5 mixer
IF DIRECTION EQ FORWARD
proc MixAsm_03_05 syscall
ELSE
proc MixAsm_03_05_R syscall
ENDIF
ARG @@MCur:PTR DWORD,@@End:PTR DWORD,@@Incr:DWORD,@@CurIncr:DWORD, \
    @@Add:DWORD,@@PlayPos:PTR BYTE,@@VolMap:PTR DWORD

   ; Save volitile registers
   pushad
   push ebp

   ; Save esp and store MEnd
   mov [StackSave],esp                    ; Stash esp in the dgroup
   mov eax,[@@VolMap]
   mov [VolTable],eax                     ; Stash VolMap too

   ; Register map:
   mov eax,[@@CurIncr]                    ; eax - Current Incr
   mov ebx,[@@Incr]                       ; ebx - Incr
   mov [MulIncr],ebx                      ; Make a 2x incr in the dseg
   add [MulIncr],ebx
   mov ecx,[@@MCur]                       ; esp - MCur
   mov edi,[@@PlayPos]                    ; edi - PlayPos
   mov esi,[@@End]                        ; esi - Volume map

   mov esp,ecx                            ; Put MCur into esp

   mov ebp, [VolTable]
   xor edx, edx
   
   ; Preload the current #
   mov dl,[edi]                           ; Store *PlayPos
   lea edx, [ebp+edx*8]
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right

   ; Determine entry point
   add [esp],ecx                          ; Add Left channel to buffer
   add [esp+4],edx                        ; Add right channel to buffer
   add esp,8                              ; Move Buffer pointer
   add eax,ebx                            ; Add the lower portion of the fraction
   jc @@Carry                             ; Did we have motion of *PlayPos?

   ; Determine entry point (2)
   add [esp],ecx                          ; Add Left channel to buffer
   add [esp+4],edx                        ; Add right channel to buffer
   add esp,8                              ; Move Buffer pointer
   add eax,ebx                            ; Add the lower portion of the fraction
   jc @@Carry                             ; Did we have motion of *PlayPos?

   ; Determine entry point (3)
   add [esp],ecx                          ; Add Left channel to buffer
   add [esp+4],edx                        ; Add right channel to buffer
   add esp,8                              ; Move Buffer pointer
   add eax,ebx                            ; Add the lower portion of the fraction
   jc @@Carry                             ; Did we have motion of *PlayPos?

   ; Start of main loop
@@Top:
   ; Add 2 sets
   add [esp],ecx                          ; Add Left channel to buffer
   add [esp + 4],edx                      ; Add right channel to buffer
   add [esp + 8],ecx                      ; Add Left channel to buffer
   add [esp + 12],edx                     ; Add right channel to buffer
   add esp,16                             ; Move Buffer pointer
   add eax,[MulIncr]
   jc @@Carry                             ; Did we have motion of *PlayPos?

   ; This loop is used if a third loop is needed
@@Inner:
   add [esp],ecx                          ; Add Left channel to buffer
   add [esp+4],edx                        ; Add right channel to buffer
   add esp,8                              ; Move Buffer pointer

   add eax,ebx                            ; Add the lower portion of the fraction
@@Carry:
   xor edx, edx                           ; Zero our temp var
IF DIRECTION EQ FORWARD
   inc edi                                ; Move play pos up 1
ELSE
   dec edi
ENDIF
   mov dl,[edi]                           ; Store *PlayPos
   lea edx, [ebp+edx*8]
   cmp esp,esi                            ; Check to see if the end was reached
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right

   jl @@Top                               ; Boing back up

   ; Restore the stack
   mov esp,[StackSave]

   ; Restore the flags
   pop ebp
   popad
   cld

   ret
endp

; 0 to 0.33 mixer
IF DIRECTION EQ FORWARD
proc MixAsm_00_03 syscall
ELSE
proc MixAsm_00_03_R syscall
ENDIF
ARG @@MCur:PTR DWORD,@@End:PTR DWORD,@@Incr:DWORD,@@CurIncr:DWORD, \
    @@Add:DWORD,@@PlayPos:PTR BYTE,@@VolMap:PTR DWORD

   ; Save volitile registers
   pushad
   push ebp

   ; Save esp and store MEnd
   mov [StackSave],esp                    ; Stash esp in the dgroup
   mov eax,[@@VolMap]
   mov [VolTable],eax                     ; Stash VolMap too

   ; Register map:
   mov eax,[@@CurIncr]                    ; eax - Current Incr
   mov ebx,[@@Incr]                       ; ebx - Incr
   mov [MulIncr],ebx                      ; Make a 3x incr in the dseg
   add [MulIncr],ebx
   add [MulIncr],ebx
   mov ecx,[@@MCur]                       ; esp - MCur
   mov edi,[@@PlayPos]                    ; edi - PlayPos
   mov esi,[@@End]                        ; esi - Volume map

   mov esp,ecx                            ; Put MCur into esp

   ; Temp regs
   mov ebp, [VolTable]
   xor edx, edx

   ; Preload the current #
   mov dl,[edi]                           ; Store *PlayPos
   lea edx, [ebp+edx*8]
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right
   jmp @@Inner;                           ; Cannot assum CurIncr is 0!!

   ; Start of main loop
@@Top:
   ; Perform 3 loops now
   add [esp],ecx                          ; Add Left channel to buffer
   add [esp + 4],edx                      ; Add right channel to buffer
   add [esp + 8],ecx                      ; Add Left channel to buffer
   add [esp + 12],edx                     ; Add right channel to buffer
   add [esp + 16],ecx                     ; Add Left channel to buffer
   add [esp + 20],edx                     ; Add right channel to buffer
   add esp,24                             ; Move Buffer pointer
   add eax,[MulIncr]
   jmp @@Check

   ; This loop is used if a 4th loop is needed
@@Inner:
   add [esp],ecx                          ; Add Left channel to buffer
   add [esp+4],edx                        ; Add right channel to buffer
   add esp,8                              ; Move Buffer pointer

   add eax,ebx                            ; Add the lower portion of the fraction
@@Check:
   jnc @@NoCarry                          ; Did we have motion of *PlayPos?
   xor edx, edx                           ; Zero our temp var
IF DIRECTION EQ FORWARD
   inc edi                                ; Move play pos up 1
ELSE
   dec edi
ENDIF
   mov dl,[edi]                           ; Store *PlayPos
   lea edx, [ebp+edx*8]
   cmp esp,esi                            ; Check to see if the end was reached
   mov ecx,[edx]                          ; Index volume table, Left
   mov edx,[edx+4]                        ; Index volume table, Right

   jl @@Top                               ; Boing back up

@@NoCarry:
   cmp esp,esi                            ; Check to see if the end was reached
   jl @@Inner                             ; Boing back up

   ; Restore the stack
   mov esp,[StackSave]

   ; Restore the flags
   pop ebp
   popad
   cld

   ret
endp
