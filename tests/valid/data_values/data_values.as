; .data: signs, zero and the limits of a 10-bit word.
        mov A, r1
        mov D, r2
        stop
A:      .data 7, -57, +17, 9
B:      .data 0
C:      .data -512, 511
D:      .data -1, +0, -0, 1
E:      .data -256, 255, 256, -511
