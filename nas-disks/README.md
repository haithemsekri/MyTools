#### fstab:
systemctl stop udisks2.service 
cat /etc/fstab | grep -v "LABEL=" > /tmp/fstab

for _dev in $(ls /dev/disk/by-label) ; do
  echo "LABEL=$_dev /media/$_dev auto nosuid,nodev,nofail,noauto,x-gvfs-hide 0 0" >> /tmp/fstab
done

mv /tmp/fstab /etc/fstab



#### disk attach script
sudo apt install procmail
#Adjust udev-disk-attach.sh if needed for usb pid&vid
cp -r udev-disk-attach.sh udisks-2.8.4/* /usr/bin/
/usr/bin/udev-disk-attach.sh --cmd poweroff  --dev all --keep no

/usr/bin/udev-disk-attach.sh --cmd poweron  --dev all --keep no
# a failure is expected here du to the missing of .mounted dir
for _dev in $(ls /dev/disk/by-label) ; do
  mkdir /media/$_dev/.mounted
  chmod 700 /media/$_dev/.mounted
done

# poweron must work now
/usr/bin/udev-disk-attach.sh --cmd poweroff  --dev all --keep no
/usr/bin/udev-disk-attach.sh --cmd poweron  --dev all --keep no



#### disk attach service
cp 99-udisks2.rules /etc/udev/rules.d/
udevadm control --reload && udevadm trigger

cp udev-disk-attach.service /lib/systemd/system/
systemctl start  udev-disk-attach.service
systemctl status udev-disk-attach.service
systemctl enable udev-disk-attach.service

#service testing
journactl -efu udev-disk-attach.service&
/usr/bin/udev-disk-attach.sh --cmd poweroff  --dev all --keep no
/usr/bin/udev-disk-attach.sh --cmd poweron  --dev all --keep no



#### Samba storage
[storage]
    comment = smb-storage
    path = /media/storage
    writable = yes
    guest ok = yes
    browseable = yes
    force user = storage
    follow symlinks = yes
    wide links = yes

[global]
    allow insecure wide links = yes



#### Sleep service
cp wait_until_idle  wait_until_idle.sh /usr/bin/
systemctl start  udev-disk-attach.service
systemctl status udev-disk-attach.service
systemctl enable udev-disk-attach.service
cp autosleep.service /lib/systemd/system/autosleep.service

h
