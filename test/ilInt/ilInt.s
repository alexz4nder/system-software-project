#STAVLJA VREDNOST 1 u r1 kada dodje do ilegalne instrukcije

.section test
ld $handler_start,%r1
csrwr %r1,%handler
ld $0x1000,%sp
ld $0,%r1
jmp bad_instrction

bad_instrction:
.word 1
halt

.section handler_sec
handler_start:
ld $1,%r2
csrrd %cause,%r3
bne %r2,%r3,wrong_int
ld $1,%r1
halt

wrong_int:
iret

.end

