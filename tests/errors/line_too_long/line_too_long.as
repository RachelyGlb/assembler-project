; a comment line of exactly eighty characters ----------------------------------
; a comment line of eighty-one characters ---------------------------------------
MAIN:   inc r1
X:      .data 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 10
Y:      .data 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 1
mcro m1
        .data 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
        inc r2
mcroend
        m1
S:      .string "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
        stop
