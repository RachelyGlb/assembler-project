;
;;;
; mov r1, r2
;.data 5
; LABEL: stop
;LABEL: this is not a label
MAIN:   inc r1
; mcro x would not start a macro here
; , , # [ ] " :
        stop
; last line is a comment
