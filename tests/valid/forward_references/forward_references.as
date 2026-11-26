; Every operand label is defined after its use.
        jmp SKIP
        mov VAL, r1
        lea TXT, r2
        mov TAB[r1][r2], r3
SKIP:   jsr SUB
        stop
SUB:    rts
VAL:    .data 42
TXT:    .string "hi"
TAB:    .mat [2][2] 1,2,3,4
