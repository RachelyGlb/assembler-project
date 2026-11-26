; The kind of statement a label was defined on is not checked.
M1:     .data 1,2,3,4
MAIN:   mov M1[r3][r3], r3
        mov MAIN[r1][r2], r4
        lea MAIN, r5
        inc S
        inc S[r0][r0]
        jmp M1
        stop
S:      .string "ab"
