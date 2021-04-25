#!/bin/bash

exec 2>&1

ROOT="/media"

mount_mount() 
{
  local label
  label="$1"
  label_path="/dev/disk/by-label/$label"
  mnt_path="$ROOT/$label"
  char_path="$(realpath $label_path)"
  
  [ ! -b $char_path ] && { echo "ERROR: device not found: $char_path"; return 1;}
  [ ! -z "$(cat /etc/mtab | egrep "$char_path $mnt_path")" ] && { echo "WARNING: device already mounted: $label_path on $mnt_path"; return 0;}
  
  echo "mount mounting $label_path $mnt_path "
  mount -o rw,nosuid,nodev,nofail,noauto,noatime,grpid,acl,x-gvfs-show $label_path $mnt_path || { echo "mount failed: $label_path"; return 1;}
  sync
  [ ! -d "$mnt_path/.mounted" ] && echo "mount.mounted failed: $label_path" && return 1
  
  unlink /media/storage/$label
  ln -sf /media/$label /media/storage/$label
  echo "mount ok: $label_path"
  return 0
}

udisksctl_mount() 
{
  local label
  label="$1"
  label_path="/dev/disk/by-label/$label"
  mnt_path="$ROOT/$label"
  char_path="$(realpath $label_path)"
  
  [ ! -b $char_path ] && { echo "ERROR: device not found: $char_path"; return 1;}
  [ ! -z "$(cat /etc/mtab | egrep "$char_path $mnt_path")" ] && { echo "WARNING: device already mounted: $label_path on $mnt_path"; return 0;}
  
  echo "udisk mounting"
  udisksctl-2.8.4 mount -o rw,nosuid,nodev,noatime --no-user-interaction --block-device $label_path || { echo "udisksctl failed: $label_path"; return 1;}
  sync
  [ ! -d "$mnt_path/.mounted" ] && echo "udisksctl.mounted failed: $label_path" && return 1
  
  unlink /media/storage/$label
  ln -sf /media/$label /media/storage/$label
  echo "udisksctl ok: $label_path"
  return 0
}

mount_all_disks()
{
  [ ! -d /dev/disk/by-label ] && return 0
  for _dev in $(ls /dev/disk/by-label) ; do mount_mount $_dev; done
  sync
  sleep 2
  for _dev in $(ls /dev/disk/by-label) ; do udisksctl_mount $_dev; done
}

umount_lsof() 
{
  local dev dir
  mnt_path="$1"
  echo "umount $mnt_path"
  umount $mnt_path && return  0
  echo "umount failed: $mnt_path"
  lsof $(realpath $mnt_path) 2>/dev/null | grep $(realpath $mnt_path)
  return  1
}

if_umount_dir() 
{
  local mnt
  mnt="$1"
  [ ! -d $mnt ] && return 0
  mount | grep "$mnt" | awk '{ print $3 }' | while read line ; do [[ -d $line && "$line" != "$mnt" ]] && umount_lsof $line; done
  umount_lsof $mnt 
}

umount_all_disks() 
{
  echo "Umount All."
  [ ! -d /dev/disk/by-label ] && return 0  
  sync
  for _mnt in $(ls /dev/disk/by-label) ; do if_umount_dir $ROOT/$_mnt; done
  [ ! -d /dev/disk/by-label ] && return 0  
  
  umount /dev/disk/by-label/* &>/dev/null
  [ ! -d /dev/disk/by-label ] && return 0  
  
  umount -f -l /dev/disk/by-label/* &>/dev/null
  echo 3 | tee /proc/sys/vm/drop_caches
  sleep 1
}


usb_bind()
{
  USB_VID="$2"
  USB_PID="$3"
  for i in /sys/bus/usb/devices/*; do
    if [ -e $i/idProduct ] && [ -e $i/idVendor ] && [ $(cat $i/idVendor):$(cat $i/idProduct) = $USB_VID:$USB_PID ]; then
      BUSNUM="$(cat $i/busnum)"
      DEVNUM="$(cat $i/devnum)"
      echo $(basename $i) > /sys/bus/usb/drivers/usb/$1
    fi
  done
}

usb_detach()
{
  USB_VID="$1"
  USB_PID="$2"
  for i in /sys/bus/usb/devices/*; do
    if [ -e $i/idProduct ] && [ -e $i/idVendor ] && [ $(cat $i/idVendor):$(cat $i/idProduct) = $USB_VID:$USB_PID ]; then
      BUSNUM="$(cat $i/busnum)"
      DEVNUM="$(cat $i/devnum)"
      usb_modeswitch -v 0x$USB_VID -p 0x$USB_PID -b $BUSNUM -g $DEVNUM -d -Q
    fi
  done
}

poweroff_all_disks() 
{
  echo "Poweroff All."
  
  usb_detach  152d 0578
  usb_detach  174c 55aa
  sleep 0.5
  usb_bind unbind  152d 0578
  usb_bind unbind   174c 55aa
  sleep 0.5
  
  usb_detach  05e3 0620
  usb_detach  05e3 0610
  sleep 0.5
  usb_bind unbind  05e3 0620
  usb_bind unbind  05e3 0610
  sleep 0.5
}

poweron_all_disks() 
{
  usb_bind bind  05e3 0610
  sleep 1
  usb_bind bind  05e3 0620
}

chardev_block_label() 
{
  device="$1"
  label=""
  
  [ -b /dev/disk/by-label/$device ] && label="$device" && return 0
  
  [ ! -b /dev/$device ] && echo "not a chardev" && return 1
  
  blkid /dev/$device  | grep "PTUUID=" | grep "PTTYPE=" && { echo "not a partition"; return 1;} ||  echo "partition: yes"

  local linkpath="$(ls -l /dev/disk/by-label/* | grep $device | awk {'print $11'})"
  [ -z "$linkpath" ] && { echo "linkpath not found"; return 1;}

  local link="$(ls -l /dev/disk/by-label/* | grep $linkpath | awk {'print $9'})"
  [ -z "$link" ] && { echo "link not found"; return 1;}
  
  label="$(basename $link)"
}

usage() {
  cat <<EOF
Udev block devices handler

Special arguments:

  [ -h | --help ]: Print this help message and exit.

Script internal arguments:

  [ --cmd <cmd> ]: add / remove / update / poweron / poweroff
  [ --dev <blockname> ]: device / all
  [ --keep <keep>  ]: keep running

Script example:
  $0 --cmd bind   --path all
  
EOF
  return 0
}

if ! options=$(getopt -o hp:n: -l help,cmd:,dev:,keep: -- "$@"); then
  usage
  exit 1
fi
eval set -- "$options"

keep="no"
while true
do
  case "$1" in
    -h|--help)    usage && exit 0;;
    -c|--cmd)     cmd=$2; shift 2;;
    -d|--dev)     dev=$2; shift 2;;
    -k|--keep)    keep=$2; shift 2;;
    --)           shift 1; break ;;
    *)            break ;;
  esac
done

if [ -z "$cmd" ] || [ -z "$dev" ]; then
    echo "ERROR: Please pass the cmd and dev" 1>&2
    exit 1
fi

echo "dev:$$: <$dev>"

daemonize()
{
  while [ ! -d /dev/disk/by-label ]; do sleep 1; done
  lockfile -r 0 /tmp/udev-disk-attach.lock &>/dev/null || { sleep 1; return 0;}
  sleep 4
  mount_all_disks
  unlink /tmp/udev-disk-attach.lock 
  local count="$(ls /dev/disk/by-label/ | wc -l)"
  while [[ -d /dev/disk/by-label && "$(ls /dev/disk/by-label/ | wc -l)" == "$count" ]]; do sleep 1; done
}
  
[ "$keep" == "yes" ] &&  while true; do daemonize; done

if [ "$dev" == "all" ] ; then
  sleep 1.5
  [ -f /tmp/udev-disk-attach.lock ] && { exit 0;} 
  lockfile -r 0 /tmp/udev-disk-attach.lock &>/dev/null || { exit 0;} 
  if [ "$cmd" == "add" ]; then
    sleep 4
    mount_all_disks
  elif [ "$cmd" == "remove" ]; then
    umount_all_disks
  elif [ "$cmd" == "poweron" ]; then
    poweron_all_disks
  elif [ "$cmd" == "poweroff" ]; then
    umount_all_disks
    sleep 1
    poweroff_all_disks
  else
    echo "unknown command <$cmd>"
  fi
  unlink /tmp/udev-disk-attach.lock 
  exit 0
fi


if [ "$cmd" == "add" ]; then
  chardev_block_label "$dev"
  [ -z "$label" ] && { echo "label not found for $dev"; exit 0;}
  mount_mount "$label"
elif [ "$cmd" == "remove" ]; then
  chardev_block_label "$dev"
  [ -z "$label" ] && { exit 0;}
  if_umount_dir " $ROOT/$label"
  umount /dev/disk/by-label/$label &>/dev/null
  umount -f -l /dev/disk/by-label/$label &>/dev/null
else
  echo "unknown command <$cmd>"
fi
exit 0

