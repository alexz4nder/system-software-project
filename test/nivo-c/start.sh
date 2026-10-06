ASSEMBLER=../../build/asembler
LINKER=../../build/linker
EMULATOR=../../build/emulator

cd ../..
make emulator
make linker
make asembler
cd test/nivo-c

${ASSEMBLER} -o main.o main.s
${ASSEMBLER} -o handler.o handler.s
${ASSEMBLER} -o isr_terminal.o isr_terminal.s
${ASSEMBLER} -o isr_timer.o isr_timer.s
${LINKER} -hex -place=code@0x40000000 -o program.hex main.o isr_terminal.o isr_timer.o handler.o
${EMULATOR} program.hex
