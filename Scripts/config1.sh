#!/bin/bash
ifconfig eth0 down
ifconfig eth1 down
/etc/init.d/networking restart
ifconfig eth0 172.16.10.1/24
ifconfig
route -n

