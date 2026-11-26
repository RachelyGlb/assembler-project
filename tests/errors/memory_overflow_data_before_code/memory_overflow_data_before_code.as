; 153 data words first, then code: the instruction line crosses the limit.
M:      .mat [10][15]
D:      .data 1, 2, 3
MAIN:   inc r1
        rts
        mov M[r1][r2], D
        stop
