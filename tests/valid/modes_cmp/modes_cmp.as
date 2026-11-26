; cmp: every source mode with every destination mode.
       cmp #1, #2
       cmp #-3, X
       cmp #4, M[r1][r2]
       cmp #5, r6
       cmp X, #-6
       cmp X, Y
       cmp Y, M[r3][r4]
       cmp X, r7
       cmp M[r0][r0], #7
       cmp M[r7][r7], Y
       cmp M[r1][r2], M[r3][r4]
       cmp M[r5][r6], r7
       cmp r0, #-8
       cmp r1, X
       cmp r2, M[r3][r4]
       cmp r5, r6
       stop
X:     .data 1
Y:     .data 2
M:     .mat [1][1] 3
