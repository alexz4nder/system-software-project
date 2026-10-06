.equ nth_number,7 #nalazi n-ti fibonacijev broj i stavlja ga u registar 1

.section program_start

ld $0x10000,%sp
ld $nth_number,%r1
call fib
halt

fib:
ld $1,%r2
beq %r1,%r2,fib_skip
beq %r1,%r0,fib_skip

push %r1
sub %r2,%r1
call fib

pop %r3
push %r1
ld $2,%r2
sub %r2,%r3

ld %r3,%r1
call fib
pop %r2
add %r2,%r1

fib_skip:
ret
  
.end
