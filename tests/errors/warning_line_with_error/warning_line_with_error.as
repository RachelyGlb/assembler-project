MAIN:   inc r1
GOOD:   .extern W
L1:     .extern
L2:     .entry MAIN W
L3:     .extern MAIN
L4:.extern V
L5 :    .extern V
L6:     .entry 1bad
        stop
