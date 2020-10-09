setenv ipaddr 192.168.0.14
setenv serverip 192.168.0.4
setenv netmask 255.255.255.0
setenv gatewayip 192.168.0.4
setenv board nano
setenv hostname myhost
setenv mountpath /home/lmi/workspace/$board/rootfs
setenv tftppath /$board/buildroot/output/images
setenv bootargs console=ttyS0,115200 earlyprintk rootdelay=1 root=/dev/nfs rw nfsroot=$serverip:$mountpath,nfsvers=4,tcp ip=$ipaddr:$serverip:$gatewayip:$netmask:$hostname:eth0:off

usb start
ping $serverip

tftp $kernel_addr_r $serverip:$tftppath/Image
tftp $fdt_addr_r $serverip:$tftppath/nanopi-neo-plus2.dtb

booti $kernel_addr_r - $fdt_addr_r
