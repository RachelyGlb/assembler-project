        .extern late
mcro mac
        inc r2
mcroend
MAIN:   inc r1
        mac
        .extern mac
mcro late
        inc r3
mcroend
        late
        dec UNDEF
        stop
