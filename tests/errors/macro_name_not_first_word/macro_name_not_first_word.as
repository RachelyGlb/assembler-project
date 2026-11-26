mcro mac
        inc r1
mcroend
MAIN:   clr r1
        inc mac
        mac
        mov r1, mac
        cmp mac[r1][r2], #1
        stop
