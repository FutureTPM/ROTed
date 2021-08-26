#!/bin/bash
currentDir=$( dirname "$( command -v "$0" )" )
scriptsCommonUtilities="$currentDir/../thirdparty/scripts-common/utilities.sh"
[ ! -f "$scriptsCommonUtilities" ] && echo -e "ERROR: scripts-common utilities not found, you must initialize the repository before running this script:\n./init-repo.sh" >&2 && exit 1
# shellcheck disable=1090
. "$scriptsCommonUtilities"

if isRootUser; then
    errorMessage 'Please do not execute this script as sudo. You will be prompted sudo when needed.'
fi

if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    writeMessage "./restore_cpufreq.sh PATH_TO_GOVERNOR"
    exit 1
fi

checkBin git || errorMessage "This tool requires git. Install it please, and then run this tool again."

if [ ! -d .git ]; then
    if ! git rev-parse > /dev/null 2>&1; then
        writeMessage "Couldn't find a git repo. Initializing..."
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
    writeMessage "No arguments supplied."
fi

writeMessage "Using $RESTORE_FILE as a restore point"

if [ ! -f "$RESTORE_FILE" ]; then
    errorMessage "$RESTORE_FILE does not exist. Exiting..."
fi

for i in $(seq 0 $NUM_CPUS)
do
    head -n $((i + 1)) "$RESTORE_FILE" | sudo tee /sys/devices/system/cpu/cpu"$i"/cpufreq/scaling_governor
done

writeMessage "Done!"
