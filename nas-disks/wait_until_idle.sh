#!/bin/bash

while true;
do

echo "wait until idle"
#/usr/bin/wait_until_idle  95 32768 100 #Delay time until sleep 10x100=16.6mins
/usr/bin/wait_until_idle 95 2048 60 10 100
echo "idle"
systemctl stop smbd nmbd
/usr/bin/udev-disk-attach.sh --cmd poweroff  --dev all --keep no

ethtool -s enp2s0 wol g
systemctl hybrid-sleep
echo "enter sleep: " $(date)
sleep 2
sleep 2
echo "exit sleep: " $(date)

/usr/bin/udev-disk-attach.sh --cmd poweron  --dev all --keep no
systemctl restart smbd nmbd
echo "Mount OK Restart"
done
