; add and sub: source modes 0,1,2,3 with destination modes 1,2,3.
       add #1, X
       add #2, M[r1][r2]
       add #3, r3
       add X, X
       add X, M[r1][r2]
       add X, r4
       add M[r1][r2], X
       add M[r1][r2], M[r3][r4]
       add M[r1][r2], r5
       add r6, X
       add r7, M[r1][r2]
       add r0, r1
       sub #-1, X
       sub #-2, M[r7][r6]
       sub #-3, r3
       sub X, X
       sub X, M[r5][r4]
       sub X, r4
       sub M[r3][r2], X
       sub M[r1][r0], M[r3][r4]
       sub M[r1][r2], r5
       sub r6, X
       sub r7, M[r1][r2]
       sub r0, r1
       stop
X:     .data 0
M:     .mat [2][2]
