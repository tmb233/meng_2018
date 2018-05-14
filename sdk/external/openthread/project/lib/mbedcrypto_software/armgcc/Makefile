PROJECT_NAME     := openthread_lib_libmbedcrypto
TARGETS          := libmbedcrypto
OUTPUT_DIRECTORY := _build

SDK_ROOT := ../../../../../..
PROJ_DIR := ../../..


# Source files common to all targets
SRC_FILES += \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/aes.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/bignum.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/ccm.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/cipher.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/cipher_wrap.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/cmac.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/ctr_drbg.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/debug.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/ecjpake.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/ecp.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/ecp_curves.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/entropy.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/entropy_poll.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/hardware_entropy.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/md.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/md_wrap.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/memory_buffer_alloc.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/platform.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/sha256.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/ssl_ciphersuites.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/ssl_cli.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/ssl_cookie.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/ssl_srv.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/ssl_ticket.c \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/library/ssl_tls.c \

# Include folders common to all targets
INC_FOLDERS += \
  $(PROJ_DIR)/openthread/include \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/include \
  $(PROJ_DIR)/openthread/src/core \
  $(PROJ_DIR) \
  $(PROJ_DIR)/openthread/examples/platforms/nrf52840 \
  $(PROJ_DIR)/openthread/examples/platforms/nrf52840/crypto \
  $(PROJ_DIR)/openthread/third_party/mbedtls/repo/include/mbedtls \
  $(PROJ_DIR)/openthread/third_party/mbedtls \

# Libraries common to all targets
LIB_FILES += \

# C flags common to all targets
CFLAGS += -DHAVE_CONFIG_H
CFLAGS += -DMBEDTLS_CONFIG_FILE="\"mbedtls-config.h\""
CFLAGS += -DMBEDTLS_USER_CONFIG_FILE="\"nrf52840-mbedtls-config.h\""
CFLAGS += -DOPENTHREAD_PROJECT_CORE_CONFIG_FILE="\"openthread-core-nrf52840-config.h\""
CFLAGS += -D_BSD_SOURCE=1
CFLAGS += -D_DEFAULT_SOURCE=1
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs
CFLAGS +=  -Wall -Werror -Os -g3
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# keep every function in separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin --short-enums 

# C++ flags common to all targets
CXXFLAGS += \

# Assembler flags common to all targets
ASMFLAGS += -x assembler-with-cpp
ASMFLAGS += -DHAVE_CONFIG_H
ASMFLAGS += -DMBEDTLS_CONFIG_FILE="\"mbedtls-config.h\""
ASMFLAGS += -DMBEDTLS_USER_CONFIG_FILE="\"nrf52840-mbedtls-config.h\""
ASMFLAGS += -DOPENTHREAD_PROJECT_CORE_CONFIG_FILE="\"openthread-core-nrf52840-config.h\""
ASMFLAGS += -D_BSD_SOURCE=1
ASMFLAGS += -D_DEFAULT_SOURCE=1



.PHONY: $(TARGETS) default all clean help flash 

# Default target - first one defined
default: libmbedcrypto

# Print all targets that can be built
help:
	@echo following targets are available:
	@echo 	libmbedcrypto

TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc

include $(TEMPLATE_PATH)/Makefile.common

$(call define_library, $(TARGETS), $(PROJ_DIR)/lib/mbedcrypto_software/armgcc/libmbedcrypto.a)

define create_library
@echo Creating library: $($@)
$(NO_ECHO)$(AR) $($@) $^
@echo Done
endef
libmbedcrypto:
	$(create_library)
