; Every reference to an external symbol gets its own .ext line.
.extern W
.extern F
        mov W, r1
        mov r1, W
        cmp W, W
        jsr F
        lea W[r1][r2], r3
        mov #1, W[r3][r4]
        bne F
        stop
