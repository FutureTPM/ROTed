#!/bin/bash

#call it with watch -n 1 (for 1 sec refresh)

#EXPLORE
echo -n "Available PStates: "
more /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies
echo -n "Available Governors: "
more /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors

#echo "- CURR FREQ CPUINFO -"
#more /proc/cpuinfo | grep MHz

CMD=$(cat /proc/cpuinfo | grep processor | wc -l)
NUM_CPUS=$(expr $CMD - 1)

echo "- FREQ (GOVERNOR) -"
for i in $(seq 0 $NUM_CPUS)
do
        get_freq=$(cat /sys/devices/system/cpu/cpu$i/cpufreq/cpuinfo_cur_freq)
        get_governor=$(cat /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor)
        echo -n "CPU"$i":" $get_freq
        echo " ("$get_governor")"
done
