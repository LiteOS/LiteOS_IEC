# ------------------------------------------------
# Generic Makefile (based on gcc)
# ------------------------------------------------

######################################
# target
######################################
TARGET = IEC
######################################
# building variables
######################################
# debug build?
DEBUG = 1
# optimization
OPT = -Og


#######################################
# binaries
#######################################
PREFIX    =
CC        = $(PREFIX)gcc
AS        = $(PREFIX)gcc -x assembler-with-cpp
OBJCOPY   = $(PREFIX)objcopy
OBJDUMP   = $(PREFIX)objdump
AR        = $(PREFIX)ar
SZ        = $(PREFIX)size
LD        = $(PREFIX)ld
HEX       = $(OBJCOPY) -O ihex
BIN       = $(OBJCOPY) -O binary -S


PROJECTBASE = $(PWD)
override PROJECTBASE    := $(abspath $(PROJECTBASE))
TOP_DIR = $(PROJECTBASE)/../..


#######################################
# paths
#######################################
# firmware library path
PERIFLIB_PATH =

# Build path
BUILD_DIR = build

######################################
# source
######################################
# C sources

WITH_DTLS := yes

# macros for gcc
# AS defines
AS_DEFS =

# C defines
C_DEFS :=  \
        -D IEC_DEBUG


ifeq ($(WITH_DTLS), yes)
C_DEFS +=  \
        -D DEBUG \
        -D USE_MBED_TLS \
        -D WITH_DTLS  \
        -D MBEDTLS_CONFIG_FILE=\"iec_mbedtls_config_x509.h\" \
        -D LWIP_TIMEVAL_PRIVATE=0
endif


ifeq ($(WITH_DTLS), yes)
MBEDTLS_SRC = \
        ${wildcard $(TOP_DIR)/components/security/mbedtls/mbedtls-2.6.0/library/*.c}
        C_SOURCES += $(MBEDTLS_SRC)

MBEDTLS_PORT_SRC = \
        ${wildcard $(TOP_DIR)/components/security/mbedtls/mbedtls_port/entropy*.c} \
        ${wildcard $(TOP_DIR)/components/security/mbedtls/mbedtls_port/iec*.c}
        C_SOURCES += $(MBEDTLS_PORT_SRC)
endif

SOCKET_ADAPTER_SRC = \
        ${wildcard $(TOP_DIR)/components/net/sal/*.c}
        C_SOURCES += $(SOCKET_ADAPTER_SRC)

IEC_LOG_SRC = \
        ${wildcard $(TOP_DIR)/components/log/*.c}
        C_SOURCES += $(IEC_LOG_SRC)

IEC_SRC = \
        ${wildcard $(TOP_DIR)/components/connectivity/iec/*.c}
        C_SOURCES += $(IEC_SRC)
MQTT_PACKET_SRC = \
        ${wildcard $(TOP_DIR)/components/connectivity/mqtt/mqtt_packet/*.c}
        C_SOURCES += $(MQTT_PACKET_SRC)
IEC_MQTT_SRC = \
        ${wildcard $(TOP_DIR)/components/connectivity/mqtt/iec_mqtt/*.c}
        C_SOURCES += $(IEC_MQTT_SRC)
IEC_DEMO_MQTT_SRC = \
        ${wildcard $(TOP_DIR)/demos/iec_mqtt/*.c}
        C_SOURCES += $(IEC_DEMO_MQTT_SRC)


# ASM sources



######################################
# firmware library
######################################
PERIFLIB_SOURCES =


#######################################
# CFLAGS
#######################################
# cpu
# fpu
# float-abi
# mcu


# AS includes
AS_INCLUDES =

INCLUDE_INC = \
        -I $(TOP_DIR)/include
        C_INCLUDES += $(INCLUDE_INC)

# C includes
ifeq ($(WITH_DTLS), yes)
MBEDTLS_INC = \
        -I $(TOP_DIR)/components/security/mbedtls/mbedtls-2.6.0/include
        C_INCLUDES += $(MBEDTLS_INC)

MBEDTLS_PORT_INC = \
        -I $(TOP_DIR)/components/security/mbedtls/mbedtls_port
        C_INCLUDES += $(MBEDTLS_PORT_INC)
endif

MQTT_PACKET_INC = \
        -I $(TOP_DIR)/components/connectivity/mqtt/mqtt_packet
        C_INCLUDES += $(MQTT_PACKET_INC)


MQTT_INC = \
        -I $(TOP_DIR)/components/connectivity/mqtt/iec_mqtt
        C_INCLUDES += $(MQTT_INC)

SOCKET_ADAPTER_INC = \
        -I $(TOP_DIR)/include/sal
        C_INCLUDES += $(SOCKET_ADAPTER_INC)

IEC_LOG_INC = \
        -I $(TOP_DIR)/include/log
        C_INCLUDES += $(IEC_LOG_INC)

IEC_INC = \
        -I $(TOP_DIR)/components/connectivity/iec \
        -I $(TOP_DIR)/include/iec_mqtt
        C_INCLUDES += $(IEC_INC)

IEC_DBG_INC = \
        -I $(INC)
        C_INCLUDES += $(IEC_DBG_INC)

# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif


# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"


#######################################
# LDFLAGS
#######################################
# link script

# libraries
LIBS = -lc -lm -lpthread
LIBDIR =
LDFLAGS = $(MCU) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf


#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES_s:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES_s)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES_S:.S=.o)))
vpath %.S $(sort $(dir $(ASM_SOURCES_S)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.S Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir $@

#######################################
# clean up
#######################################
clean:
	-rm -fR .dep $(BUILD_DIR)

#######################################
# dependencies
#######################################
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

# *** EOF ***
