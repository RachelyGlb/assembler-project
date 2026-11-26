MAIN:   mov NOA, NOB
        cmp NOC[r1][r2], NOD[r3][r4]
        inc r1
        lea NOA, NOA
        stop
