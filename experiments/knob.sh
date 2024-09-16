#!/bin/bash
#
# Evaluate the detection for a KNOB attack launched by mirage.
# Warning: highly specific to my environment.
# As you may know, using sleep for synchronization increase the chance of portability (wrong)


set -euo pipefail
IFS=$'\n\t'

if [ "$EUID" -ne 0 ]; then
  echo "Please run this script as root or with sudo."
  exit 1
fi

if [ -n "${SUDO_USER:-}" ]; then
    USER_HOME=$(getent passwd "$SUDO_USER" | cut -d: -f6)
else
    USER_HOME=$HOME
fi

# ****************************************************************************
#
# Environment variables
#
WD=$(pwd)
SCRIPT_DIR=$(dirname "$(realpath "$0")")
MIRAGE_DIR=${USER_HOME}/projects/superviz/mirage-flo
LOG_FILE="${SCRIPT_DIR}/knob.log"
DETECTION_COUNT_FILE="/tmp/detection_count.tmp"
DEVICE_SERIAL=/dev/ttyACM0
ADV_ADDR=DE:AD:BE:EF:AA:AA
INTERFACE=hci0
#
# ****************************************************************************
#
total_runs=2
successful_launches=0
detections=0
#
# ****************************************************************************

log() {
  echo "$(date +'%Y-%m-%d %H:%M:%S') - $1" | tee -a "$LOG_FILE"
}

reset_board() {
    log "Reset board"
    nrfjprog --reset
}

init_experiment() {
    reset_board
}

launch_attack() {
    local mirage_output_file="/tmp/mirage_output_$$"

    # Note: stdbuf is used to disable buffering for both stdout and stderr
    stdbuf -oL -eL ${MIRAGE_DIR}/mirage_launcher "ble_connect|ble_master" ble_master2.SCENARIO=knob ble_connect1.TARGET=${ADV_ADDR} 2>&1 | tee "$mirage_output_file" &

    local mirage_pid=$!

    sleep 5
    reset_board
    #pkill -P $mirage_pid   # works only when mirage fails
    #wait 

    sleep 1
    log "$(cat "$mirage_output_file")"

    # Mirage may sometimes fail to launched its attack,
    # and it should not be counted as a false positive.
    if grep -q "Connected on device" "$mirage_output_file"; then
        log "Attack successfully launched"
        rm -f "$mirage_output_file"
        return 0
    elif grep -q "[FAIL]" "$mirage_output_file"; then 
        log "Mirage encountered an error"
        #log "$(cat "$mirage_output_file")"
    else 
        log "Unexpected Mirage error"
        #log "$(cat "$mirage_output_file")"
    fi

    rm -f "$mirage_output_file"
    return 1
}

# ****************************************************************************
#
monitor_serial() {
    local detections=0

    echo "0" > "$DETECTION_COUNT_FILE"

    stty -F "$DEVICE_SERIAL" 115200 cs8 -cstopb -parenb -echo
    cat ${DEVICE_SERIAL} | while read -r line; do
    #stdbuf -oL dd if=${DEVICE_SERIAL} bs=1 2>/dev/null | while IFS= read -r n1 char; do
        if [[ "$line" == *"KNOB"* ]]; then
            log $line
            detections=$((detections + 1))
            echo "$detections" > "$DETECTION_COUNT_FILE"
        fi

        if [[ -e "/tmp/stop_monitor" ]]; then
            break
        fi
    done
}

start_monitor() {
    rm -f "/tmp/stop_monitor"

    monitor_serial &
    monitor_pid=$!
    echo "Monitor serial started with PID $monitor_pid"
}

stop_monitor() {
    local count=0

    touch "/tmp/stop_monitor"

    wait "$monitor_pid" 2>/dev/null

    # Read and return the detection count
    if [[ -f "$DETECTION_COUNT_FILE" ]]; then
        count=$(cat "$DETECTION_COUNT_FILE")
        echo "Monitor stopped. Total detections: $count"
    else
        log "Couldn't read "$DETECTION_COUNT_FILE""
    fi

    rm -f "/tmp/stop_monitor" "$DETECTION_COUNT_FILE"
    echo "$count"
}

# ****************************************************************************

log "Initialization"
init_experiment
start_monitor
sleep 1

# Run attack campaign
#
log "************************************"
log "Launch $total_runs KNOB attacks in loop"
for i in $(seq 1 $total_runs)
do
    log "Run $i"
    if launch_attack; then # bug here
        successful_launches=$((successful_launches + 1))
    else
        log "Attack $i not launched correctly"
    fi
    sleep 1
done

log "Clean up"
#kill $monitor_pid 2>/dev/null || true
detections=$(stop_monitor)


log "======================="
log "Test campaign completed"
log "Total runs: $total_runs"
log "Successful launches: $successful_launches"
log "Detections: $detections"
if [ $successful_launches -ne "0" ]; then
    log "Detection rate: $(bc <<< "scale=2; $detections / $successful_launches * 100")%"
else
    log "No successful launches, cannot calculate detection rate"
fi

