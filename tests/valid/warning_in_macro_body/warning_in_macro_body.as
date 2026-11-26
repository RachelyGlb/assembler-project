mcro decl
IGN:    .extern W
        jmp W
mcroend
MAIN:   inc r1
        decl
        decl
OTHER:  .entry MAIN
        stop
