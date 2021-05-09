# name of your application
APPLICATION = assignment2

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
USEMODULE += lps331ap
USEMODULE += isl29020

# For the light sensor
#USEMODULE += analog_util
# Temp & Humidity 
#USEMODULE += dht
#USEMODULE += fmt
# To enable-disable pins
#USEMODULE += periph_gpio
# USEMODULE += pm

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
# Activate ICMPv6 error messages
USEMODULE += gnrc_icmpv6_error
# Specify the mandatory networking modules for IPv6 and UDP
USEMODULE += gnrc_ipv6_router_default
USEMODULE += gnrc_udp
# Add a routing protocol
USEMODULE += gnrc_rpl
USEMODULE += auto_init_gnrc_rpl
# This application dumps received packets to STDIO using the pktdump module
USEMODULE += gnrc_pktdump
# Additional networking modules that can be dropped if not needed
USEMODULE += gnrc_icmpv6_echo
# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += netstats_l2
USEMODULE += netstats_ipv6
USEMODULE += netstats_rpl

# Set a custom channel if needed
include $(RIOTMAKE)/default-radio-settings.inc.mk

#FEATURES_OPTIONAL += periph_rtc
#FEATURES_OPTIONAL += periph_adc

# if NUCLEO = 1 => an stm32 nucleo board is being used, else => an M3 node is being used
NUCLEO = 0
CFLAGS += -DNUCLEO=$(NUCLEO)

#include $(CURDIR)/Makefile.net.conf
include $(RIOTBASE)/Makefile.include



# Set a custom channel if needed
include $(RIOTMAKE)/default-radio-settings.inc.mk