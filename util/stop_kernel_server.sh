#!/bin/bash

sudo dmesg --clear
sudo rmmod secure_shmem
sudo rmmod labstor_kernel_server
sudo rmmod labstor_kpkg_devkit
sudo dmesg