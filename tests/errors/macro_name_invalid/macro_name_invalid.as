MAIN:   inc r1
mcro 1abc
        inc r2
mcroend
mcro _abc
        inc r3
mcroend
mcro good_1
        inc r4
mcroend
mcro ab$c
        inc r5
mcroend
mcro ab-c
        inc r6
mcroend
        good_1
mcro a.b
        inc r7
mcroend
        stop
