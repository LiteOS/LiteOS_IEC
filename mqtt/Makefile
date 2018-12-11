# ------------------------------------------------
# Generic Makefile (based on gcc)
# ------------------------------------------------

######################################
# target
######################################
TARGET = Atiny
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
TOP_DIR = $(PROJECTBASE)
SRC = $(TOP_DIR)/src
INC = $(TOP_DIR)/include


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
        -D ATINY_DEBUG


ifeq ($(WITH_DTLS), yes)
C_DEFS +=  \
        -D DEBUG \
        -D USE_MBED_TLS \
        -D WITH_DTLS  \
        -D MBEDTLS_CONFIG_FILE=\"atiny_mbedtls_config_x509.h\" \
        -D LWIP_TIMEVAL_PRIVATE=0

        #-D MBEDTLS_DEBUG_C \

endif


ifeq ($(WITH_DTLS), yes)
MBEDTLS_SRC = \
        ${wildcard $(SRC)/mbedtls/mbedtls-2.6.0/library/*.c}
        C_SOURCES += $(MBEDTLS_SRC)

MBEDTLS_PORT_SRC = \
        ${wildcard $(SRC)/atiny_mbed_ssl/*.c}
        C_SOURCES += $(MBEDTLS_PORT_SRC)
endif


ATINY_LOG_SRC = \
        ${wildcard $(SRC)/atiny_log/atiny_log.c}
        C_SOURCES += $(ATINY_LOG_SRC)

MQTT_PACKET_SRC = \
        ${wildcard $(SRC)/mqtt_packet/*.c}
        C_SOURCES += $(MQTT_PACKET_SRC)

MQTT_SRC = \
        ${wildcard $(SRC)/atiny_mqtt/*.c}
        C_SOURCES += $(MQTT_SRC)

ATINY_TINY_SRC = \
        ${wildcard $(SRC)/atiny/*.c}
        C_SOURCES += $(ATINY_TINY_SRC)

#AGENT_DEMO_SRC = \
#        ${wildcard $(TOP_DIR)/agent_tiny/examples/mqtt_demo/*.c}
#        C_SOURCES += $(AGENT_DEMO_SRC)

#CJSON_SRC = \
#        ${wildcard $(TOP_DIR)/cJSON/cJSON*.c}
#        C_SOURCES += $(CJSON_SRC)

#USER_SRC =  \
#        $(TOP_DIR)/main.c
#        C_SOURCES += $(USER_SRC)


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

# C includes
ifeq ($(WITH_DTLS), yes)
MBEDTLS_INC = \
        -I $(SRC)/mbedtls/mbedtls-2.6.0/include \
        -I $(SRC)/mbedtls/mbedtls-2.6.0/include/mbedtls
        C_INCLUDES += $(MBEDTLS_INC)

MBEDTLS_PORT_INC = \
        -I $(SRC)/atiny_mbed_ssl
        C_INCLUDES += $(MBEDTLS_PORT_INC)
endif

MQTT_PACKET_INC = \
        -I $(SRC)/mqtt_packet
        C_INCLUDES += $(MQTT_PACKET_INC)


MQTT_INC = \
        -I $(SRC)/atiny_mqtt
        C_INCLUDES += $(MQTT_INC)

ATINY_LOG_INC = \
        -I $(SRC)/atiny_log
        C_INCLUDES += $(ATINY_LOG_INC)

ATINY_INC = \
        -I $(SRC)/atiny
        C_INCLUDES += $(ATINY_INC)

ATINY_DBG_INC = \
        -I $(INC)
        C_INCLUDES += $(ATINY_DBG_INC)

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
