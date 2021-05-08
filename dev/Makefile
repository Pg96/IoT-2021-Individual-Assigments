# name of your application
APPLICATION = assignment1

# If no BOARD is found in the environment, use this default:
BOARD ?= iotlab-m3

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= /senslab/users/priamo/RIOT

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Modules to include:
USEMODULE += xtimer
# For the light sensor
USEMODULE += analog_util
# Temp & Humidity 
USEMODULE += dht
USEMODULE += fmt
# To enable-disable pins
USEMODULE += periph_gpio
# USEMODULE += pm

# Shell support
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps


FEATURES_OPTIONAL += periph_rtc
FEATURES_OPTIONAL += periph_adc

# if NUCLEO = 1 => an stm32 nucleo board is being used, else => an M3 node is being used
NUCLEO = 0
CFLAGS += -DNUCLEO=$(NUCLEO)

include $(CURDIR)/Makefile.net.conf
include $(RIOTBASE)/Makefile.include
include $(RIOTMAKE)/default-radio-settings.inc.mk