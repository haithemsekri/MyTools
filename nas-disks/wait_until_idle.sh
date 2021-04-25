#!/bin/bash

while true;
do

echo "wait until idle"
/usr/bin/wait_until_idle  95 32768 100 #Delay time until sleep 10x100 seconds
echo "idle"
systemctl stop smbd nmbd
/usr/bin/udev-disk-attach.sh --cmd poweroff  --dev all --keep no

ethtool -s enp2s0 wol g
systemctl hybrid-sleep
echo "enter sleep: " $(date +%s)
sleep 2
echo "exit sleep: " $(date +%s)

/usr/bin/udev-disk-attach.sh --cmd poweron  --dev all --keep no
systemctl restart smbd nmbd
echo "Mount OK Restart"
done
