; mov: source modes 0,1,2,3 with destination modes 1,2,3.
START: mov #1, X
       mov #-2, M[r1][r2]
       mov #3, r4
       mov X, Y
       mov X, M[r3][r4]
       mov Y, r5
       mov M[r0][r1], X
       mov M[r2][r3], M[r4][r5]
       mov M[r6][r7], r0
       mov r1, X
       mov r2, M[r5][r6]
       mov r3, r7
       stop
X:     .data 10
Y:     .data 20
M:     .mat [2][2] 1, 2, 3, 4
