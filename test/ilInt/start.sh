ASSEMBLER=../../build/asembler
LINKER=../../build/linker
EMULATOR=../../build/emulator

cd ../..
make emulator
make linker
make asembler
cd test/ilInt

${ASSEMBLER} -o ilInt.o ilInt.s

${LINKER} -hex \
  -place=test@0x40000000 \
  -o program.hex \
  ilInt.o

${EMULATOR} program.hex
