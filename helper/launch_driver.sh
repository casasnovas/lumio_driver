#! /bin/sh

./helper/detach
sleep 2
insmod lumio_driver.ko
sleep 2
rmmod lumio_driver
./helper/detach
sleep 2
insmod lumio_driver.ko

