; CR LF line endings

mcro twice
        inc r1
        dec r2
mcroend
.entry MAIN
MAIN:   mov #-3, r1
        twice
;0000000000000000000000000000000000000000000000000000000000000000000000000000000
STR:    .string "ab"                                                            
        twice
NUM:    .data 7, -1
        stop
