
ARMGNU ?= arm-none-eabi
#ARMGNU ?= arm-linux-gnueabi

AOPS = --warn --fatal-warnings -mcpu=cortex-m0
COPS = -Wall -Werror -O2 -nostdlib -nostartfiles -ffreestanding  -mcpu=cortex-m0

all : vectors.bin

clean:
	rm -f *.bin
	rm -f *.o
	rm -f *.elf
	rm -f *.list

vectors.bin : vectors.s
	$(ARMGNU)-as $(AOPS) vectors.s -o vectors.o
	$(ARMGNU)-ld -o vectors.elf -Ttext=0x00000000 vectors.o
	$(ARMGNU)-objdump -D vectors.elf > vectors.list
	$(ARMGNU)-objcopy --srec-forceS3 vectors.elf -O srec vectors.srec
	$(ARMGNU)-objcopy vectors.elf vectors.bin -O binary

