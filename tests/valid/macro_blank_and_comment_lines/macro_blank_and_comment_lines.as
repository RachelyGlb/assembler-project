; a comment before the macro

mcro body
; a comment inside the body
        inc r1

        dec r2
mcroend

MAIN:   clr r1
        body
; a comment between the calls
        body
        stop
