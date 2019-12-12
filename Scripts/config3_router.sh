#!/bin/bash
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts
echo 1 > /proc/sys/net/ipv4/ip_forward

ifconfig eth0 down
ifconfig eth1 down
ifconfig eth2 down

route del default
ifconfig eth0 172.16.30.254/24
ifconfig eth2 172.16.31.253/24

route add default gateway 172.16.31.254

/etc/init.d/networking restart

ifconfig
route -n

