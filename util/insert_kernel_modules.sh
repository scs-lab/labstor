#!/bin/bash

sudo dmesg --clear
sudo insmod ${LABSTOR_ROOT}/modules/kernel/kpkg_devkit/kernel/labstor_kpkg_devkit.ko
sudo insmod ${LABSTOR_ROOT}/modules/kernel/secure_shmem/kernel/secure_shmem.ko
sudo dmesg
sudo chmod a+rwX /dev/labstor_shared_shmem0