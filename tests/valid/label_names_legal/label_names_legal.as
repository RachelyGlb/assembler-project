; None of these names is reserved.
PSW:    .data 1
PC:     .data 2
r8:     .data 3
r20:    .data 4
R1:     .data 5
MOV:    inc PSW
Stop:   inc PC
Data:   inc r8
movx:   inc r20
x:      inc R1
hEllo:  jmp MOV
He78902: jmp Stop
        mov r8, r1
        mov R1, r20
        stop
