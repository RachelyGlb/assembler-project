MAIN:   lea X, r1
        lea #5, r1
        lea M[r1][r2], r3
        lea r1, r2
        lea #-1, X
        lea r7, M[r1][r2]
        stop
X:      .data 1
M:      .mat [2][2]
