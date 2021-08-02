MCU      = atmega328p
FREQ	 = 20000000
TARGET   = opl2
OBJECTS  = main.o opl2.o

LDFLAGS  = -flto -Wl,--gc-sections -Wl,--print-gc-sections -Wl,-Map,${TARGET}.map
CPPFLAGS = -std=c++17 -Wall -O3 -Wextra -fdata-sections -ffunction-sections -DF_CPU=${FREQ}

.PHONY: all flash clean

all: flash size

size: ${TARGET}.elf
	avr-size --format=avr --mcu=${MCU} ${TARGET}.elf

flash: ${TARGET}.hex
	avrdude -c usbasp -p ${MCU} -U flash:w:${TARGET}.hex

clean:
	rm -vf ${OBJECTS} ${TARGET}.elf ${TARGET}.hex ${TARGET}.map

${TARGET}.hex: ${TARGET}.elf
	avr-objcopy -R .eeprom -R .fuse -R .lock -R .signature -O ihex ${TARGET}.elf ${TARGET}.hex

${TARGET}.elf: ${OBJECTS}
	avr-g++ -mmcu=${MCU} ${LDFLAGS} -o ${TARGET}.elf ${OBJECTS}

%.o: %.cpp
	avr-g++ -mmcu=${MCU} ${CPPFLAGS} -c -o $@ $<
