MAIN:   mov M[r2][r7], r1
        mov [r2][r7], r1
        dec r1
        inc [r1][r2]
        mov r1, [r3][r4]
        stop
M:      .mat [2][2]
