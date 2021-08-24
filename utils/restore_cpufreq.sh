#!/bin/bash

if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "./restore_cpufreq.sh PATH_TO_GOVERNOR"
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

PATH_TO_ROOT=$(git rev-parse --show-toplevel)
BACKUP_FOLDER=$PATH_TO_ROOT/backup

RESTORE_FILE="$BACKUP_FOLDER"/governors
if [ $# -ne 0 ]; then
    RESTORE_FILE="$1"
else
    echo "No arguments supplied."
fi

echo "Using $RESTORE_FILE as a restore point"

if [ ! -f "$RESTORE_FILE" ]; then
    echo "$RESTORE_FILE does not exist. Exiting..."
    exit 1
fi

for i in $(seq 0 $NUM_CPUS)
do
    head -n $((i + 1)) "$RESTORE_FILE" | sudo tee /sys/devices/system/cpu/cpu"$i"/cpufreq/scaling_governor
done

echo "Done!"
