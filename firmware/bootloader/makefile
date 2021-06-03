#
#             LUFA Library
#     Copyright (C) Dean Camera, 2017.
#
#  dean [at] fourwalledcubicle [dot] com
#           www.lufa-lib.org
#
# --------------------------------------
#         LUFA Project Makefile.
# --------------------------------------

# Run "make help" for target help.

MCU          = atmega32u4
ARCH         = AVR8
BOARD        = LEONARDO
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = 3
TARGET       = Caterina
SRC          = $(TARGET).c Descriptors.c $(LUFA_SRC_USB)
LUFA_PATH    = ../lufa/LUFA

PID = 0x0036
VID = 0x2341

FLASH_SIZE_KB        = 32
BOOT_SECTION_SIZE_KB = 4
BOOT_START           = 0x$(shell echo "obase=16; ($(FLASH_SIZE_KB) - $(BOOT_SECTION_SIZE_KB)) * 1024" | bc)

# LUFA library compile-time options and predefined tokens
LUFA_OPTS  = -D USB_DEVICE_ONLY
LUFA_OPTS += -D DEVICE_STATE_AS_GPIOR=0
LUFA_OPTS += -D ORDERED_EP_CONFIG
LUFA_OPTS += -D FIXED_CONTROL_ENDPOINT_SIZE=8
LUFA_OPTS += -D FIXED_NUM_CONFIGURATIONS=1
LUFA_OPTS += -D USE_RAM_DESCRIPTORS
LUFA_OPTS += -D USE_STATIC_OPTIONS="(USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)"
LUFA_OPTS += -D NO_INTERNAL_SERIAL
LUFA_OPTS += -D NO_DEVICE_SELF_POWER
LUFA_OPTS += -D NO_DEVICE_REMOTE_WAKEUP
LUFA_OPTS += -D NO_SOF_EVENTS

#LUFA_OPTS += -D NO_BLOCK_SUPPORT
#LUFA_OPTS += -D NO_EEPROM_BYTE_SUPPORT
#LUFA_OPTS += -D NO_FLASH_BYTE_SUPPORT
LUFA_OPTS += -D NO_LOCK_BYTE_WRITE_SUPPORT

CDEFS  = -DF_CPU=$(F_CPU)UL
CDEFS += -DF_USB=$(F_USB)UL
CDEFS += -DBOARD=BOARD_$(BOARD) -DARCH=ARCH_$(ARCH)
CDEFS += -DBOOT_START_ADDR=$(BOOT_START)UL
CDEFS += -DDEVICE_VID=$(VID)UL
CDEFS += -DDEVICE_PID=$(PID)UL
CDEFS += $(LUFA_OPTS)

CC_FLAGS     = $(CDEFS)
LD_FLAGS     =

# Default target
all:

# Include LUFA-specific DMBS extension modules
DMBS_LUFA_PATH ?= $(LUFA_PATH)/Build/LUFA
include $(DMBS_LUFA_PATH)/lufa-sources.mk
include $(DMBS_LUFA_PATH)/lufa-gcc.mk

# Include common DMBS build system modules
DMBS_PATH      ?= $(LUFA_PATH)/Build/DMBS/DMBS
include $(DMBS_PATH)/core.mk
include $(DMBS_PATH)/cppcheck.mk
include $(DMBS_PATH)/doxygen.mk
include $(DMBS_PATH)/dfu.mk
include $(DMBS_PATH)/gcc.mk
include $(DMBS_PATH)/hid.mk
include $(DMBS_PATH)/avrdude.mk
include $(DMBS_PATH)/atprogram.mk

install:
	teensy_loader_cli --mcu=atmega32u4 -w ./AnalogDancePad.hex
