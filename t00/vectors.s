
.thumb
.globl _start
_start:
.word 0x20001000
.word reset
.word done
.thumb_func
reset:
    b done
done:
    b done
