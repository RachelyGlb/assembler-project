mcro consts
        .data 1, 2
        .string "m"
mcroend
mcro table
TAB:    .mat [1][2] 7
.entry TAB
mcroend
MAIN:   mov TAB[r0][r1], r2
        consts
        stop
        table
        consts
