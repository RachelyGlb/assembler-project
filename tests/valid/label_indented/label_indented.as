    MAIN:  mov r1, r2
	LOOP:	inc r1
  	 DAT:  .data 5, -5
                STR: .string "s"
        jmp LOOP
        prn DAT
        lea STR, r3
   .entry LOOP
        stop
