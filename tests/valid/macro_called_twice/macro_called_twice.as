mcro step
        inc r1
        prn r1
mcroend
mcro unused
        dec r7
mcroend
MAIN:   clr r1
        step
        step
        cmp r1, #3
        step
        stop
