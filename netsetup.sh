#!/bin/bash

sudo /home/kaosv/Tutorials/RIOT/dist/tools/tapsetup/tapsetup -d
sudo /home/kaosv/Tutorials/RIOT/dist/tools/tapsetup/tapsetup
#sudo ip a a fec0:affe::1/64 dev tapbr0
ip -6 addr add 2001:0db8:0:f101::1/64 dev tapbr0
