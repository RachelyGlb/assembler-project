; A partly initialised matrix: remaining cells are 0.
        mov M[r1][r2], r3
        prn AFTER
        stop
M:      .mat [2][3] 7, 8
AFTER:  .data 9
