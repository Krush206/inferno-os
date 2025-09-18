load std

echo 1 > /dev/jit
echo ps2 > /dev/pointerctl
9660srv /dev/sdD0/data /n/cd
tarfs /n/cd/dis.tar /dis
tarfs /n/cd/lib.tar /lib
tarfs /n/cd/fonts.tar /fonts
tarfs /n/cd/icons.tar /icons
tarfs /n/cd/man.tar /man
tarfs /n/cd/usr.tar /tmp
memfs /chan
memfs /usr
cp -r /tmp/* /usr/
unmount /tmp
memfs
