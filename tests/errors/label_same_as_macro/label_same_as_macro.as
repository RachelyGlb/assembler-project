late:   inc r1
mcro early
        inc r2
mcroend
MAIN:   clr r1
        early
early:  inc r3
mcro late
        inc r4
mcroend
        late
mcro third
        inc r5
mcroend
third:  .data 5
        stop
