; Every one of the 16 operations appears exactly once.
MAIN: mov r1, r2
      cmp #3, X
      add X, r3
      sub r4, X
      lea X, r5
      clr r6
      not r7
      inc X
      dec X
      jmp MAIN
      bne MAIN
      jsr FUNC
      red r0
      prn #65
      stop
FUNC: rts
X:    .data 7
