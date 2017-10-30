
.thumb
.globl _start
_start:
.word 0x20001000
.word reset
.word three
.word four
.thumb_func
reset:
    b three
three:
    b three
.thumb_func
four:
    b three
