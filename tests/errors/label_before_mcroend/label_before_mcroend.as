mcro prev
        inc r1
mcroend
MAIN:   clr r1
mcro second
        inc r2
        prev extra
L1:     mcroend
        prev extra
        second extra
mcro third
        inc r3
L2:     mcroend junk
        third extra
        stop
