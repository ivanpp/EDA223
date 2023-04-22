# Compilers
C=      arm-none-eabi
CC=     $C-gcc
AS=     $C-as
LINKER= $C-g++
POST=   $C-objcopy

# Flags
CCFLAGS=     -g \
             -O0 \
             -Wall \
             -mthumb \
             -mcpu=cortex-m4 \
             -mfloat-abi=hard \
             -mfpu=fpv4-sp-d16 \
             -fverbose-asm \
             -DSTM32F40_41xxx \
             -I ./device/inc \
             -I ./driver/inc \
             -DDEBUG

ASFLAGS=     -IIFLAGS

LINKERFLAGS= --specs=nano.specs \
             -mthumb \
             -mfloat-abi=hard \
             -mfpu=fpv4-sp-d16 \
             -mcpu=cortex-m4 \
             -nostartfiles \
             -T ./md407-ram.x \
             -Wl,-Map=./Debug/RTS-Lab.map,--cref

POSTFLAGS=   -S -O srec

# Directories
DEBUGDIR=  ./Debug/
DRIVERDIR= ./driver/src/
MKDIR=     test -d $(DEBUGDIR) || mkdir -p $(DEBUGDIR)

# Objects
OBJECTS= $(DEBUGDIR)dispatch.o \
         $(DEBUGDIR)TinyTimber.o \
         $(DEBUGDIR)canTinyTimber.o \
         $(DEBUGDIR)sciTinyTimber.o \
         $(DEBUGDIR)sioTinyTimber.o \
         $(DEBUGDIR)stm32f4xx_can.o \
         $(DEBUGDIR)stm32f4xx_dac.o \
         $(DEBUGDIR)stm32f4xx_exti.o \
         $(DEBUGDIR)stm32f4xx_gpio.o \
         $(DEBUGDIR)stm32f4xx_rcc.o \
         $(DEBUGDIR)stm32f4xx_syscfg.o \
         $(DEBUGDIR)stm32f4xx_tim.o \
         $(DEBUGDIR)stm32f4xx_usart.o \
         $(DEBUGDIR)stm32f4xx_rng.o \
         $(DEBUGDIR)startup.o \
         $(DEBUGDIR)toneGenerator.o \
         $(DEBUGDIR)musicPlayer.o \
         $(DEBUGDIR)userButton.o \
         $(DEBUGDIR)heartbeat.o \
         $(DEBUGDIR)failureMonitor.o \
         $(DEBUGDIR)application.o
         #$(DEBUGDIR)playList.o 

# MusicPlayer for different ranks
ELFS= $(DEBUGDIR)rank1.elf \
      $(DEBUGDIR)rank2.elf \
      $(DEBUGDIR)rank3.elf

HEXS= $(DEBUGDIR)rank1.s19 \
      $(DEBUGDIR)rank2.s19 \
      $(DEBUGDIR)rank3.s19

.PHONY: all
all: $(DEBUGDIR) $(ELFS) $(HEXS)

###
### Intermediate targets
###

# System-defined targets
$(DEBUGDIR):
	$(MKDIR)
# musicplyaer for RANK 0, 1, 2
$(DEBUGDIR)rank1.elf: $(OBJECTS) $(DEBUGDIR)networkR1.o
	$(LINKER) -o $@ $(LINKERFLAGS) $^
$(DEBUGDIR)rank2.elf: $(OBJECTS) $(DEBUGDIR)networkR2.o
	$(LINKER) -o $@ $(LINKERFLAGS) $^
$(DEBUGDIR)rank3.elf: $(OBJECTS) $(DEBUGDIR)networkR3.o
	$(LINKER) -o $@ $(LINKERFLAGS) $^
$(DEBUGDIR)rank1.s19: $(DEBUGDIR)rank1.elf
	$(POST) $(POSTFLAGS) $< $@
$(DEBUGDIR)rank2.s19: $(DEBUGDIR)rank2.elf
	$(POST) $(POSTFLAGS) $< $@
$(DEBUGDIR)rank3.s19: $(DEBUGDIR)rank3.elf
	$(POST) $(POSTFLAGS) $< $@

$(DEBUGDIR)dispatch.o: dispatch.s
	$(AS) $< -o $@ $(ASFLAGS)
# drivers
$(DEBUGDIR)stm32f4xx_can.o: $(DRIVERDIR)stm32f4xx_can.c
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)stm32f4xx_dac.o: $(DRIVERDIR)stm32f4xx_dac.c
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)stm32f4xx_exti.o: $(DRIVERDIR)stm32f4xx_exti.c
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)stm32f4xx_gpio.o: $(DRIVERDIR)stm32f4xx_gpio.c
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)stm32f4xx_rcc.o: $(DRIVERDIR)stm32f4xx_rcc.c
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)stm32f4xx_syscfg.o: $(DRIVERDIR)stm32f4xx_syscfg.c
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)stm32f4xx_tim.o: $(DRIVERDIR)stm32f4xx_tim.c
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)stm32f4xx_usart.o: $(DRIVERDIR)stm32f4xx_usart.c
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)stm32f4xx_rng.o: $(DRIVERDIR)stm32f4xx_rng.c
	$(CC) -c $< -o $@ $(CCFLAGS)

$(DEBUGDIR)startup.o: startup.c
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)TinyTimber.o: TinyTimber.c TinyTimber.h
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)canTinyTimber.o: canTinyTimber.c canTinyTimber.h
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)sciTinyTimber.o: sciTinyTimber.c sciTinyTimber.h
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)sioTinyTimber.o: sioTinyTimber.c sioTinyTimber.h
	$(CC) -c $< -o $@ $(CCFLAGS)

# User-defined targets
$(DEBUGDIR)playList.o: playList.c playList.h
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)toneGenerator.o: toneGenerator.c toneGenerator.h
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)musicPlayer.o: musicPlayer.c musicPlayer.h brotherJohn.h
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)userButton.o: userButton.c userButton.h
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)heartbeat.o: heartbeat.c heartbeat.h
	$(CC) -c $< -o $@ $(CCFLAGS)
$(DEBUGDIR)failureMonitor.o: failureMonitor.c failureMonitor.h
	$(CC) -c $< -o $@ $(CCFLAGS)
# network for different RANKS
$(DEBUGDIR)networkR1.o: network.c network.h
	$(CC) -c $< -o $@ $(CCFLAGS) -DRANK=1
$(DEBUGDIR)networkR2.o: network.c network.h
	$(CC) -c $< -o $@ $(CCFLAGS) -DRANK=2
$(DEBUGDIR)networkR3.o: network.c network.h
	$(CC) -c $< -o $@ $(CCFLAGS) -DRANK=3
# application
DEPS=application.c TinyTimber.h sciTinyTimber.h canTinyTimber.h sioTinyTimber.h \
     application.h systemPorts.h toneGenerator.h musicPlayer.h brotherJohn.h \
     userButton.h network.h heartbeat.h failureMonitor.h
$(DEBUGDIR)application.o: $(DEPS)
	$(CC) -c $< -o $@ $(CCFLAGS)

###
### Clean
###

.PHONY: clean
clean:
	rm -vrf $(DEBUGDIR)

