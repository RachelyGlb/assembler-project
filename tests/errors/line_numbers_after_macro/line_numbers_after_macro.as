; line 1
mcro two
        inc r1
        inc r2
mcroend

MAIN:   clr r1
        foo r1
        two
        mov r1
mcro three
        dec r1
        dec r2
        dec r3
mcroend
        three
        two
        bar
        stop
        clr
