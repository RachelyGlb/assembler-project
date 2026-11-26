mcro prev
        inc r1
mcroend
MAIN:   clr r1
L1:     mcro newm
        prev extra
        dec r1
mcroend
        prev extra
        newm extra
mcro newm
        inc r2
mcroend
1x$:    mcro other
        inc r3
mcroend
        other extra
        stop
