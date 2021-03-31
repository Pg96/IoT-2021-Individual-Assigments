# name of your application
APPLICATION = assignment1

# If no BOARD is found in the environment, use this default:
BOARD ?= nucleo-f401re

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= /home/kaosv/Tutorials/RIOT/

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
FEATURES_REQUIRED += periph_adc

include $(CURDIR)/Makefile.ethos.conf
include $(RIOTBASE)/Makefile.include
