#!/bin/bash

if [[ $(id -u) == 0 ]]; then
    echo 'Please do not execute this script as sudo. You will be prompted sudo when needed.'
    exit 1
fi

#call it with watch -n 1 (for 1 sec refresh)

#EXPLORE
echo -n "Available PStates: "
more /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies
echo -n "Available Governors: "
more /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors

#echo "- CURR FREQ CPUINFO -"
#more /proc/cpuinfo | grep MHz

CMD=$(grep -c processor < /proc/cpuinfo)
NUM_CPUS=$((CMD - 1))

echo "- FREQ (GOVERNOR) -"
for i in $(seq 0 $NUM_CPUS)
do
        get_freq=$(sudo cat /sys/devices/system/cpu/cpu"$i"/cpufreq/cpuinfo_cur_freq)
        get_governor=$(sudo cat /sys/devices/system/cpu/cpu"$i"/cpufreq/scaling_governor)
        echo -n "CPU$i: $get_freq"
        echo "($get_governor)"
done
