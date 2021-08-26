#!/bin/bash
currentDir=$( dirname "$( command -v "$0" )" )
scriptsCommonUtilities="$currentDir/../thirdparty/scripts-common/utilities.sh"
[ ! -f "$scriptsCommonUtilities" ] && echo -e "ERROR: scripts-common utilities not found, you must initialize the repository before running this script:\n./init-repo.sh" >&2 && exit 1
# shellcheck disable=1090
. "$scriptsCommonUtilities"

if isRootUser; then
    errorMessage 'Please do not execute this script as sudo. You will be prompted sudo when needed.'
fi

#call it with watch -n 1 (for 1 sec refresh)

#EXPLORE
writeMessage "Available PStates: "
more /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies
writeMessage "Available Governors: "
more /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors

#echo "- CURR FREQ CPUINFO -"
#more /proc/cpuinfo | grep MHz

CMD=$(grep -c processor < /proc/cpuinfo)
NUM_CPUS=$((CMD - 1))

writeMessage "- FREQ (GOVERNOR) -"
for i in $(seq 0 $NUM_CPUS)
do
        get_freq=$(sudo cat /sys/devices/system/cpu/cpu"$i"/cpufreq/cpuinfo_cur_freq)
        get_governor=$(sudo cat /sys/devices/system/cpu/cpu"$i"/cpufreq/scaling_governor)
        writeMessage "CPU$i: $get_freq"
        writeMessage "($get_governor)"
done
