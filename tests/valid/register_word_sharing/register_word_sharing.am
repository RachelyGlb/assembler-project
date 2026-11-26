; Two register operands share one extra word; nothing else is shared.
        mov r1, r2
        cmp r7, r0
        add r3, r3
        sub r0, r7
        mov r1, X
        mov X, r1
        mov #7, r1
        cmp r1, #7
        mov M[r1][r2], r3
        mov r3, M[r1][r2]
        mov M[r3][r3], r3
        cmp M[r1][r2], M[r3][r4]
        clr r5
        prn r6
        stop
X:      .data 1
M:      .mat [2][2]
