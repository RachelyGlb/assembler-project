MAIN:   clr r1
mcro twice
        inc r1
        foo r1
        inc r2
mcroend
mcro once
        mov r1
        dec r1
mcroend
        twice
        once
        twice
        stop
