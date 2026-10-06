

.section test
ld $1000,%sp
ld $int_handler,%r1
csrwr %r1,%handler
ld $0x55667788,%r1
csrwr %r1,%status
int

.section interrupt_sec
int_handler:
pop %r1
pop %r2
halt

.end
