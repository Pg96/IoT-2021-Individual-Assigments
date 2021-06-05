# name of your application
APPLICATION = assignment3

# If no BOARD is found in the environment, use this default:
BOARD ?= b-l072z-lrwan1

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= /senslab/users/priamo/RIOT
RIOTMAKE ?= /senslab/users/priamo/RIOT/makefiles

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

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

TTN_DEV_ID ?= 01
CFLAGS += -DTTN_DEV_ID='"$(TTN_DEV_ID)"'

#USEMODULE += lps331ap
#USEMODULE += isl29020

SERVER_ADDR = 2001:660:3207:400::61
SERVER_PORT = 1885

# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
#USEMODULE += netstats_l2
#USEMODULE += netstats_ipv6
#USEMODULE += netstats_rpl
MQTT_TOPIC = localgateway_to_awsiot
MQTT_TOPIC_IN = awsiot_to_localgateway
CFLAGS += -DSERVER_ADDR='"$(SERVER_ADDR)"'
CFLAGS += -DSERVER_PORT=$(SERVER_PORT)
CFLAGS += -DMQTT_TOPIC='"$(MQTT_TOPIC)"'
CFLAGS += -DMQTT_TOPIC_IN='"$(MQTT_TOPIC_IN)"'

ifneq (,$(EMCUTE_ID))
  CFLAGS += -DEMCUTE_ID=\"$(EMCUTE_ID)\"
endif

# if NUCLEO = 1 => an stm32 nucleo board is being used, else => an M3 node is being used
NUCLEO = 0
CFLAGS += -DNUCLEO=$(NUCLEO)

#include $(CURDIR)/Makefile.net.conf
include $(RIOTBASE)/Makefile.include