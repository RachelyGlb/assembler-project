.extern E
.entry MAIN
mcro m
        inc r1
mcroend
MAIN:   jsr E
        m
        stop
X:      .data 1
