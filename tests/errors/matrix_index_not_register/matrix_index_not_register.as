MAIN:   mov M[r1][r2], r3
        mov M[r8][r1], r2
        inc M[r0][r7]
        inc M[r1][r9]
        mov r1, M[r10][r2]
        inc M[5][r1]
        inc M[r1][X]
        inc M[#1][r2]
        inc M[R1][r2]
        stop
X:      .data 1
M:      .mat [2][2]
