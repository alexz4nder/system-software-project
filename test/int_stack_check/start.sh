ASSEMBLER=../../build/asembler
LINKER=../../build/linker
EMULATOR=../../build/emulator

cd ../..
make emulator
make linker
make asembler
cd test/int_stack_check

${ASSEMBLER} -o main.o main.s

${LINKER} -hex \
  -place=test@0x40000000 -place=interrupt_sec@0x11223344 \
  -o program.hex \
  main.o

${EMULATOR} program.hex
