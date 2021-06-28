#!/bin/bash

CMD=$(cat /proc/cpuinfo | grep processor | wc -l)
NUM_CPUS=$(expr $CMD - 1)

echo "- BEFORE -"
for i in $(seq 0 $NUM_CPUS)
do
        get_freq=$(cat /sys/devices/system/cpu/cpu$i/cpufreq/cpuinfo_cur_freq)
        get_governor=$(cat /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor)
        echo -n "CPU"$i":" $get_freq
        echo " ("$get_governor")"
done

for i in $(seq 0 $NUM_CPUS)
do
        echo "userspace" > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor
        echo $1 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_setspeed
done

echo "- AFTER -"
for i in $(seq 0 $NUM_CPUS)
do
        get_freq=$(cat /sys/devices/system/cpu/cpu$i/cpufreq/cpuinfo_cur_freq)
        get_governor=$(cat /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor)
        echo -n "CPU"$i":" $get_freq
        echo " ("$get_governor")"
done

echo "0" > /proc/sys/kernel/nmi_watchdog
