MAIN:   clr r1
        foo
mcro first
        bar r1
mcroend
        inc
mcro second
        baz
IGN:    .extern W
mcroend
        second
        first
        dec
        second
        stop
