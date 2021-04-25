#!/bin/bash
#/usr/bin/usb_reset.sh

if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

USB_VID="$1"
USB_PID="$2"

for i in /sys/bus/usb/devices/*; do
  if [ -e $i/idProduct ] && [ -e $i/idVendor ] && [ $(cat $i/idVendor):$(cat $i/idProduct) = $USB_VID:$USB_PID ]; then
    BUSNUM="$(cat $i/busnum)"
    DEVNUM="$(cat $i/devnum)"
    echo "Resetting $i $BUSNUM/$DEVNUM"
    usb_modeswitch -v 0x$USB_VID -p 0x$USB_PID -b $BUSNUM -g $DEVNUM -R -W
  fi
done
