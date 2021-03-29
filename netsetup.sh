#!/bin/bash

sudo /home/kaosv/Tutorials/RIOT/dist/tools/tapsetup/tapsetup
sudo ip a a fec0:affe::1/64 dev tapbr0