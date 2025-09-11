load std

(first second third) = $*
src = $first
dest = $second

if {~ $first -m} {
	src = $second
	dest = $third
	echo 'Installing MBR'
	disk/mbr -m /Inferno/386/mbr $dest^/data
}

echo 'Partitioning ' $dest
disk/fdisk $dest^/data
echo 'Creating the subpartitions'
disk/prep -bw -a^(9fat fs) $dest^/plan9
echo 'Mounting the CD'
9660srv $src^/data /n/cd
echo 'Formatting the 9fat subpartition'
disk/format -b /n/cd/pbslba -d -r 2 $dest^/9fat /n/cd/9load /n/cd/ipc.gz
dossrv -f $dest^/9fat
echo 'bootfile'^$dest^'!9fat!ipc.gz' | sed -e 's/\/dev\///' /n/dos/plan9.ini
unmount /n/dos
echo 'Mounting and initializing the kfs file system'
cmd = '{disk/kfs -r '^$dest^' /fs}'
mount -c $cmd /n/kfs
echo 'Binding the CDs dis directory'
bind -b /n/cd/dis /dis
echo 'Installing Inferno'
echo 1 > /dev/jit
/n/cd/dis/install/inst -v -r /n/kfs /n/cd/install/inferno/*
cp /n/cd/kinit.sh /n/kfs
cp /n/cd/shutdown /n/kfs/dis
cp /n/cd/kfs.b /n/kfs/appl/cmd/disk/
echo 'Installation done'
unmount /n/kfs
unmount /n/cd
echo 'Remove the CD and reboot'
