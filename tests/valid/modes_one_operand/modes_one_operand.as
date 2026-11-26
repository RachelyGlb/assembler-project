; One-operand operations: the operand is a destination operand.
TOP:   clr X
       clr M[r1][r2]
       clr r3
       not X
       not M[r3][r4]
       not r5
       inc X
       inc M[r5][r6]
       inc r7
       dec X
       dec M[r7][r0]
       dec r1
       jmp TOP
       jmp M[r0][r1]
       jmp r2
       bne TOP
       bne M[r2][r3]
       bne r4
       jsr TOP
       jsr M[r4][r5]
       jsr r6
       red X
       red M[r6][r7]
       red r0
       prn #-1
       prn X
       prn M[r1][r1]
       prn r7
       stop
X:     .data 5
M:     .mat [2][2]
