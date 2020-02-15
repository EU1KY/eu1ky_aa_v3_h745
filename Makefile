ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
else
    DETECTED_OS := $(shell uname -s)
endif

ifeq ($(DETECTED_OS),Windows)
    mkdir = mkdir $(subst /,\,$(1)) > nul 2>&1 || (exit 0)
    rm = $(wordlist 2,65535,$(foreach FILE,$(subst /,\,$(1)),& del $(FILE) > nul 2>&1)) || (exit 0)
    rmdir = del /F /S /Q $(subst /,\,$(1)) > nul 2>&1 || (exit 0)
    gen_timestamp = gen_timestamp.bat
else
    mkdir = mkdir -p $(1)
    rm = rm $(1) > /dev/null 2>&1 || true
    rmdir = rm -rf $(1) > /dev/null 2>&1 || true
    gen_timestamp = ./gen_timestamp.sh
endif

# ----------------------------------------------------
ifeq ($(TARGET),Debug)
    TARGET_CFLAGS = -O0 -DDEBUG -DUSE_FULL_ASSERT
else
    TARGET = Release
    TARGET_CFLAGS = -Os -DRELEASE
endif

# ----------------------------------------------------
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
BINSCRIPT = hexmerge.py

BINDIR   = out/$(TARGET)/bin
OBJDIR   = out/$(TARGET)/obj
BINFILE  = $(BINDIR)/fw_image.bin

# ----------------------------------------------------
CFLAGSM7  = -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -fgcse -fexpensive-optimizations -fomit-frame-pointer \
          -fdata-sections -ffunction-sections $(TARGET_CFLAGS) -g -mfpu=fpv5-d16 -MMD -Wall

ASFLAGSM7 = -mcpu=cortex-m7 -mthumb -Wa,--gdwarf-2

LDFLAGSM7 = -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -fgcse -fexpensive-optimizations -fomit-frame-pointer \
          -fdata-sections -ffunction-sections $(TARGET_CFLAGS) -g -mthumb -mfpu=fpv5-d16 -Wl,-Map=$(BINDIR)/cm7/CM7.map \
          -u _printf_float -specs=nano.specs -Wl,--gc-sections -Tsrc/sys/STM32H745XIHx_FLASH_CM7.ld -lm

# ----------------------------------------------------
CFLAGSM4  = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -fgcse -fexpensive-optimizations -fomit-frame-pointer \
          -fdata-sections -ffunction-sections $(TARGET_CFLAGS) -g -mfpu=fpv5-sp-d16 -MMD -Wall

ASFLAGSM4 = -mcpu=cortex-m4 -mthumb -Wa,--gdwarf-2

LDFLAGSM4 = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -fgcse -fexpensive-optimizations -fomit-frame-pointer \
          -fdata-sections -ffunction-sections $(TARGET_CFLAGS) -g -mthumb -mfpu=fpv5-sp-d16 -Wl,-Map=$(BINDIR)/cm4/CM4.map \
          -u _printf_float -specs=nano.specs -Wl,--gc-sections -Tsrc/sys/STM32H745XIHx_FLASH_CM4.ld -lm

# ----------------------------------------------------
DEFINEM7 =  -DSTM32H745xx \
            -DUSE_STM32H745I_DISCO \
            -DCORE_CM7 \
            -DUSE_IOEXPANDER \
            -DSTM32 \
            -DUSE_HAL_DRIVER \
            -DARM_MATH_CM7 \
            -D__FPU_PRESENT \
            -DUSE_FULL_LL_DRIVER\
            -DFS_ENABLED \
            -DLODEPNG_NO_COMPILE_DISK \
            -DLODEPNG_NO_COMPILE_ERROR_TEXT \
            -DLODEPNG_NO_COMPILE_ALLOCATORS \
            -DLODEPNG_NO_COMPILE_ANCILLARY_CHUNKS \
            -DUSE_USB_FS \
            -DUSBD_ENABLED \
            -DRIGEXPERT_PROTOCOL_ENABLED

INCLUDEM7 = \
    -Isrc/cm7/inc \
    -Isrc/cm7/src/uartcomm \
    -Isrc/cm7/src/analyzer/lcd \
    -Isrc/cm7/src/analyzer/lcd/bitmaps \
    -Isrc/cm7/src/analyzer/config \
    -Isrc/cm7/src/analyzer/dsp \
    -Isrc/cm7/src/analyzer/gen \
    -Isrc/cm7/src/analyzer/osl \
    -Isrc/cm7/src/analyzer/window \
    -Isrc/common \
    -Isrc/drivers \
    -Isrc/drivers/BSP/Components/ampire480272 \
    -Isrc/drivers/BSP/Components/Common \
    -Isrc/drivers/BSP/Components/ft5336 \
    -Isrc/drivers/BSP/Components/rk043fn48h \
    -Isrc/drivers/BSP/Components/wm8994 \
    -Isrc/drivers/BSP/STM32H745I-Discovery \
    -Isrc/drivers/CMSIS/Include \
    -Isrc/drivers/CMSIS/Core/Include \
    -Isrc/drivers/CMSIS/Device/ST/STM32H7xx/Include \
    -Isrc/drivers/CMSIS/Device/ST/STM32H7xx/Include \
    -Isrc/drivers/CMSIS/DSP/Include \
    -Isrc/drivers/STM32H7xx_HAL_Driver/inc \
    -Isrc/drivers/STM32H7xx_HAL_Driver/inc/Legacy \
    -Isrc/cm7/src/FatFs/src \
    -Isrc/cm7/src/USBD/Core/Inc \
    -Isrc/cm7/src/USBD/Class/MSC/Inc

SRCM7 = \
    src/cm7/src/main.c \
    src/cm7/src/sdram_heap.c \
    src/cm7/src/uartcomm/aauart.c \
    src/cm7/src/uartcomm/fifo.c \
    src/cm7/src/uartcomm/shell.c \
    src/cm7/src/uartcomm/nanovna.c \
    src/cm7/src/uartcomm/rigexpert.c \
    src/cm7/src/stm32h7xx_hal_msp.c \
    src/cm7/src/stm32h7xx_it.c \
    src/cm7/src/analyzer/lcd/LCD.c \
    src/cm7/src/analyzer/lcd/font.c \
    src/cm7/src/analyzer/lcd/fran.c \
    src/cm7/src/analyzer/lcd/franbig.c \
    src/cm7/src/analyzer/lcd/consbig.c \
    src/cm7/src/analyzer/lcd/sdigits.c \
    src/cm7/src/analyzer/lcd/hit.c \
    src/cm7/src/analyzer/lcd/libnsbmp.c \
    src/cm7/src/analyzer/lcd/lodepng.c \
    src/cm7/src/analyzer/lcd/smith.c \
    src/cm7/src/analyzer/lcd/textbox.c \
    src/cm7/src/analyzer/lcd/touch.c \
    src/cm7/src/analyzer/config/config.c \
    src/cm7/src/analyzer/dsp/dsp.c \
    src/cm7/src/analyzer/osl/oslfile.c \
    src/cm7/src/analyzer/osl/match.c \
    src/cm7/src/analyzer/gen/gen.c \
    src/cm7/src/analyzer/gen/rational.c \
    src/cm7/src/analyzer/gen/adf4350.c \
    src/cm7/src/analyzer/gen/adf4351.c \
    src/cm7/src/analyzer/gen/si5338a.c \
    src/cm7/src/analyzer/gen/si5351.c \
    src/cm7/src/analyzer/window/mainwnd.c \
    src/cm7/src/analyzer/window/tdr.c \
    src/cm7/src/analyzer/window/keyboard.c \
    src/cm7/src/analyzer/window/fftwnd.c \
    src/cm7/src/analyzer/window/generator.c \
    src/cm7/src/analyzer/window/measurement.c \
    src/cm7/src/analyzer/window/num_keypad.c \
    src/cm7/src/analyzer/window/oslcal.c \
    src/cm7/src/analyzer/window/panfreq.c \
    src/cm7/src/analyzer/window/panvswr2.c \
    src/cm7/src/analyzer/window/screenshot.c \
    src/cm7/src/FatFs/src/ff.c \
    src/cm7/src/FatFs/src/ff_gen_drv.c \
    src/cm7/src/FatFs/src/diskio.c \
    src/cm7/src/FatFs/src/option/ccsbcs.c \
    src/cm7/src/FatFs/src/mmc_diskio.c \
    src/cm7/src/USBD/Core/Src/usbd_storage.c \
    src/cm7/src/USBD/Core/Src/usbd_ioreq.c \
    src/cm7/src/USBD/Core/Src/usbd_desc.c \
    src/cm7/src/USBD/Core/Src/usbd_conf.c \
    src/cm7/src/USBD/Core/Src/usbd_core.c \
    src/cm7/src/USBD/Core/Src/usbd_ctlreq.c \
    src/cm7/src/USBD/Class/MSC/Src/usbd_msc.c \
    src/cm7/src/USBD/Class/MSC/Src/usbd_msc_bot.c \
    src/cm7/src/USBD/Class/MSC/Src/usbd_msc_data.c \
    src/cm7/src/USBD/Class/MSC/Src/usbd_msc_scsi.c \
    src/common/system_stm32h7xx.c \
    src/sys/syscalls.c \
    src/common/build_timestamp.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_rcc.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_ltdc.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_ltdc_ex.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_pwr.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_pwr_ex.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_rcc_ex.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_gpio.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_cortex.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_hsem.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_sdram.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_ll_fmc.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_dma.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_mdma.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_uart.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_uart_ex.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_tim.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_dma2d.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_i2c.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_sai.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_spi.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_mmc.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_ll_sdmmc.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_pcd.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_pcd_ex.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_ll_usb.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_rtc.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_rtc_ex.c \
    src/drivers/BSP/STM32H745I-Discovery/stm32h745i_discovery.c \
    src/drivers/BSP/STM32H745I-Discovery/stm32h745i_discovery_sdram.c \
    src/drivers/BSP/STM32H745I-Discovery/stm32h745i_discovery_lcd.c \
    src/drivers/BSP/STM32H745I-Discovery/stm32h745i_discovery_ts.c \
    src/drivers/BSP/STM32H745I-Discovery/stm32h745i_discovery_audio.c \
    src/drivers/BSP/STM32H745I-Discovery/stm32h745i_discovery_mmc.c \
    src/drivers/BSP/STM32H745I-Discovery/custom_spi2.c \
    src/drivers/BSP/Components/ft5336/ft5336.c \
    src/drivers/BSP/Components/wm8994/wm8994.c \
    src/drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_init_f32.c \
    src/drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_f32.c \
    src/drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_f32.c \
    src/drivers/CMSIS/DSP/Source/TransformFunctions/arm_cfft_radix8_f32.c \
    src/drivers/CMSIS/DSP/Source/CommonTables/arm_common_tables.c

OBJSM7 = $(SRCM7:%.c=$(OBJDIR)/cm7/%.c.o) $(OBJDIR)/cm7/sys/startup_stm32h745xx.o \
    $(OBJDIR)/src/drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal2.o

DEPSM7 := $(SRCM7:%.c=$(OBJDIR)/cm7/%.c.d)

# ----------------------------------------------------
DEFINEM4 =  -DSTM32H745xx \
            -DUSE_STM32H745I_DISCO \
            -DCORE_CM4 \
            -DUSE_IOEXPANDER \
            -DSTM32 \
            -DUSE_HAL_DRIVER \
            -DARM_MATH_CM4 \
            -D__FPU_PRESENT

INCLUDEM4 = \
    -Isrc/cm4/inc \
    -Isrc/drivers/CMSIS/Device/ST/STM32H7xx/Include \
    -Isrc/drivers/BSP/Components/Common \
    -Isrc/drivers/BSP/STM32H745I-Discovery \
    -Isrc/drivers/CMSIS/Include \
    -Isrc/drivers/CMSIS/Core/Include \
    -Isrc/drivers/STM32H7xx_HAL_Driver/inc \
    -Isrc/drivers/STM32H7xx_HAL_Driver/inc/Legacy

SRCM4 = \
    src/cm4/src/main.c \
    src/cm4/src/stm32h7xx_hal_msp.c \
    src/cm4/src/stm32h7xx_it.c \
    src/common/system_stm32h7xx.c \
    src/sys/syscalls.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_rcc.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_pwr_ex.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_rcc_ex.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_gpio.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_cortex.c \
    src/drivers/STM32H7xx_HAL_Driver/src/stm32h7xx_hal_hsem.c \
    src/drivers/BSP/STM32H745I-Discovery/stm32h745i_discovery.c

OBJSM4 = $(SRCM4:%.c=$(OBJDIR)/cm4/%.c.o) $(OBJDIR)/cm4/sys/startup_stm32h745xx.o

DEPSM4 := $(SRCM4:%.c=$(OBJDIR)/cm4/%.c.d)

# ----------------------------------------------------
.PHONY: all clean debug flash flashdbg gen

all:
	@$(MAKE) -s gen
	@$(MAKE) $(BINFILE)

debug:
	@$(MAKE) TARGET=Debug

clean:
	@$(call rm,src/common/build_timestamp.h)
	@$(call rmdir,out)

flash:
	STM32_Programmer_CLI -c port=SWD --skipErase -e [0 7] -w $(BINDIR)/cm7/CM7.hex
	STM32_Programmer_CLI -c port=SWD --skipErase -w $(BINDIR)/cm4/CM4.hex -rst

#	STM32_Programmer_CLI -c port=SWD --skipErase -e [0 1] -w $(BINDIR)/fw_image.bin 0x08000000 -rst

flashdbg:
	@$(MAKE) TARGET=Debug flash

src/common/build_timestamp.h:
gen:
	@$(call gen_timestamp)

# ----------------------------------------------------------------------
$(BINFILE): $(BINDIR)/cm4/CM4.hex $(BINDIR)/cm7/CM7.hex
	@python $(BINSCRIPT) $(BINFILE) $^

$(BINDIR)/cm7/CM7.hex : $(BINDIR)/cm7/CM7.elf
	@$(OBJCOPY) -O ihex $< $@
	@$(SIZE) $<

$(BINDIR)/cm7/CM7.elf : $(OBJSM7) Makefile
	@$(call mkdir,$(BINDIR)/cm7)
	@echo Linking...
	@$(CC) $(OBJSM7) -o $@ $(LDFLAGSM7)

$(OBJDIR)/cm7/src/common/build_timestamp.c.o : src/common/build_timestamp.h
	@echo src/common/build_timestamp.c
	@$(call mkdir,"$(@D)")
	@$(CC) $(CFLAGSM7) $(INCLUDEM7) $(DEFINEM7) -c src/common/build_timestamp.c -o $@

$(OBJDIR)/cm7/%.c.o : %.c
	@$(call mkdir,"$(@D)")
	@echo $<
	@$(CC) $(CFLAGSM7) $(INCLUDEM7) $(DEFINEM7) -c $< -o $@

$(OBJDIR)/cm7/sys/startup_stm32h745xx.o : src/sys/startup_stm32h745xx.s
	@$(call mkdir,"$(@D)")
	@echo $<
	@$(CC) $(ASFLAGSM7) -MMD -c $< -o $@

$(OBJDIR)/src/drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal2.o: src/drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal2.S
	@$(call mkdir,"$(@D)")
	@echo $<
	@$(CC) $(ASFLAGSM7) -MMD -c $< -o $@

# ----------------------------------------------------------------------
$(BINDIR)/cm4/CM4.hex : $(BINDIR)/cm4/CM4.elf Makefile
	@$(OBJCOPY) -O ihex $< $@
	@$(SIZE) $<

$(BINDIR)/cm4/CM4.elf : $(OBJSM4) Makefile
	@$(call mkdir,$(BINDIR)/cm4)
	@echo Linking...
	@$(CC) $(OBJSM4) -o $@ $(LDFLAGSM4)

$(OBJDIR)/cm4/%.c.o : %.c
	@$(call mkdir,"$(@D)")
	@echo $<
	@$(CC) $(CFLAGSM4) $(INCLUDEM4) $(DEFINEM4) -c $< -o $@

$(OBJDIR)/cm4/sys/startup_stm32h745xx.o : src/sys/startup_stm32h745xx.s
	@$(call mkdir,"$(@D)")
	@echo $<
	@$(CC) $(ASFLAGSM4) -MMD -c $< -o $@

-include $(DEPSM7)
-include $(DEPSM4)
