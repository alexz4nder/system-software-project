ASSEMBLER=../../build/asembler
LINKER=../../build/linker
EMULATOR=../../build/emulator

cd ../..
make emulator
make linker
make asembler
cd test/fibbonachi

${ASSEMBLER} -o fibbonachi.o fibbonachi.s

${LINKER} -hex \
  -place=program_start@0x40000000 \
  -o program.hex \
  fibbonachi.o

${EMULATOR} program.hex
