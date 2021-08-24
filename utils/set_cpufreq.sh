#!/bin/bash

if [ $# -eq 0 ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "No arguments supplied."
    echo "You need to supply an argument. E.g.:"
    echo "./set_cpufreq.sh 2200000"
    exit 1
fi

if [[ $(id -u) == 0 ]]; then
    echo 'Please do not execute this script as sudo. You will be prompted sudo when needed.'
    exit 1
fi

if ! which git > /dev/null; then
    echo "git command not found. Is it installed?"
    echo "Exiting..."
    exit 1
fi

if [ ! -d .git ]; then
    if ! git rev-parse > /dev/null 2>&1; then
        echo "Couldn't find a git repo. Initializing..."
        git init
    fi
fi

CMD=$(grep -c processor < /proc/cpuinfo)
NUM_CPUS=$((CMD - 1))

echo "Changing frequency to $1"

PATH_TO_ROOT=$(git rev-parse --show-toplevel)
BACKUP_FOLDER=$PATH_TO_ROOT/backup

echo "Creating backup folder at $BACKUP_FOLDER"
mkdir -p "$BACKUP_FOLDER"

if [ -f "$BACKUP_FOLDER"/governors ]; then
    echo "Previous governors file exists. Backing up..."
    DATE=$(date +"%d-%m-%y@%T")
    mv "$BACKUP_FOLDER"/governors "$BACKUP_FOLDER"/governors-"$DATE"
fi

echo "- BEFORE -"
for i in $(seq 0 $NUM_CPUS)
do
        get_freq=$(sudo cat /sys/devices/system/cpu/cpu"$i"/cpufreq/cpuinfo_cur_freq)
        get_governor=$(sudo cat /sys/devices/system/cpu/cpu"$i"/cpufreq/scaling_governor)
        echo -n "CPU$i: $get_freq"
        echo "($get_governor)"
        echo "$get_governor" >> "$BACKUP_FOLDER"/governors
done

for i in $(seq 0 $NUM_CPUS)
do
        echo "userspace" | sudo tee /sys/devices/system/cpu/cpu"$i"/cpufreq/scaling_governor
        echo "$1" | sudo tee /sys/devices/system/cpu/cpu"$i"/cpufreq/scaling_setspeed
done

echo "- AFTER -"
for i in $(seq 0 $NUM_CPUS)
do
        get_freq=$(sudo cat /sys/devices/system/cpu/cpu"$i"/cpufreq/cpuinfo_cur_freq)
        get_governor=$(sudo cat /sys/devices/system/cpu/cpu"$i"/cpufreq/scaling_governor)
        echo -n "CPU$i: $get_freq"
        echo "($get_governor)"
done

echo "0" | sudo tee /proc/sys/kernel/nmi_watchdog

echo "Done!"
echo "You can restore your previous configuration by running:"
echo -e "\t ./restore_cpufreq.sh"
