mcro mac
        inc r1
mcroend
MAIN:   mov DUP, r1
        mac
DUP:    .extern EXT
DUP:    .data 5
LATER:  .entry MAIN
LATER:  dec r2
MAIN:   .entry DUP
mov:    .entry LATER
1bad$:  .extern EXT
mac:    .extern EXT2
r3:     .entry MAIN
ThisLabelIsLongerThanThirtyCharacters: .extern EXT
:       .extern EXT
        jmp EXT
        stop
