#!/bin/bash
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts
echo 1 > /proc/sys/net/ipv4/ip_forward

ifconfig eth0 down
ifconfig eth1 down
ifconfig eth2 down
ifconfig ath0 down
/etc/init.d/networking restart
ifconfig eth0 172.16.10.254/24
ifconfig eth1 172.16.20.253/24

route add default gateway 172.16.31.254
route add -net 172.16.30.0/24 gw 172.16.30.254
route add -net 172.16.31.0/24 gw 172.16.31.253

