mcro first
        inc r1
mcroend
MAIN:   clr r1
        first
mcro second
        dec r2
        dec r3
mcroend
        second
        mcro third
        prn r1
        mcroend
        third
        stop
