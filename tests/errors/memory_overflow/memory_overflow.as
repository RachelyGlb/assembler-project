; 6 code words, 150 data words, and one data word too many.
        mov M[r1][r2], M[r3][r4]
        stop
M:      .mat [10][15] 1
LAST:   .data 1
