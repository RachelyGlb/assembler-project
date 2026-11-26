; 5 code words and 150 data words; then two erroneous lines, then two words.
MAIN:   mov M[r1][r2], M[r3][r4]
M:      .mat [10][15]
        foo r1
X:      .data 1, 2, 3, 4x
A:      .data 1
B:      .data 2
        stop
