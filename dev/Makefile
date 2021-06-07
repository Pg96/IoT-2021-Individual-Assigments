# name of your application
APPLICATION = assignment3

# If no BOARD is found in the environment, use this default:
BOARD ?= b-l072z-lrwan1

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= /senslab/users/priamo/RIOT

# Modules to include:
USEMODULE += xtimer
USEMODULE += hts221

#
# The application needs LoRaWAN related modules and variables:
#

# The Semtech LoRa radio device (SX1276)
USEMODULE += sx1276
USEMODULE += semtech_loramac_rx

# The Semtech Loramac package
USEPKG += semtech-loramac

# Default region is Europe and default band is 868MHz
LORA_REGION ?= EU868

# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += ps

TTN_DEV_ID ?= 01
CFLAGS += -DTTN_DEV_ID='"$(TTN_DEV_ID)"'

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1


#include $(CURDIR)/Makefile.net.conf
include $(RIOTBASE)/Makefile.include