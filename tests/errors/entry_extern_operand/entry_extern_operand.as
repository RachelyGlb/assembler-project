MAIN:   inc r1
.extern GOOD
.entry MAIN
.entry
.extern
        jmp GOOD
.entry MAIN MAIN
.extern A, B
.extern 1abc
.entry a_b
.extern C D
.entry #5
        stop
