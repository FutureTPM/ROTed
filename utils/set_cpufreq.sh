#!/bin/bash
currentDir=$( dirname "$( command -v "$0" )" )
scriptsCommonUtilities="$currentDir/../thirdparty/scripts-common/utilities.sh"
[ ! -f "$scriptsCommonUtilities" ] && echo -e "ERROR: scripts-common utilities not found, you must initialize the repository before running this script:\n./init-repo.sh" >&2 && exit 1
# shellcheck disable=1090
. "$scriptsCommonUtilities"

if isRootUser; then
    errorMessage 'Please do not execute this script as sudo. You will be prompted sudo when needed.'
fi

if [ $# -eq 0 ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    errorMessage "No arguments supplied.\n You need to supply an argument. E.g.:\n ./set_cpufreq.sh 2200000"
fi

checkBin git || errorMessage "This tool requires git. Install it please, and then run this tool again."

if [ ! -d .git ]; then
    if ! git rev-parse > /dev/null 2>&1; then
        echo "Couldn't find a git repo. Initializing..."
        git init
    fi
fi

CMD=$(grep -c processor < /proc/cpuinfo)
NUM_CPUS=$((CMD - 1))

writeMessage "Changing frequency to $1"

PATH_TO_ROOT=$(git rev-parse --show-toplevel)
BACKUP_FOLDER=$PATH_TO_ROOT/backup

writeMessage "Creating backup folder at $BACKUP_FOLDER"
mkdir -p "$BACKUP_FOLDER"

if [ -f "$BACKUP_FOLDER"/governors ]; then
    writeMessage "Previous governors file exists. Backing up..."
    DATE=$(date +"%d-%m-%y@%T")
    mv "$BACKUP_FOLDER"/governors "$BACKUP_FOLDER"/governors-"$DATE"
fi

echo "- BEFORE -"
for i in $(seq 0 $NUM_CPUS)
do
        get_freq=$(sudo cat /sys/devices/system/cpu/cpu"$i"/cpufreq/cpuinfo_cur_freq)
        get_governor=$(sudo cat /sys/devices/system/cpu/cpu"$i"/cpufreq/scaling_governor)
        writeMessage -n "CPU$i: $get_freq"
        writeMessage "($get_governor)"
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

writeMessage "Done!"
warning "You can restore your previous configuration by running:"
warning "./restore_cpufreq.sh"
