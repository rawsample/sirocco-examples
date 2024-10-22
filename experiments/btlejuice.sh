#!/bin/bash
#
# Evaluate the detection for a BTLEJuice attack launched by mirage.
# Warning: highly specific to my environment.
# As you may know, using sleep for synchronization increase the chance of portability (wrong)


set -x
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
LOG_FILE="${SCRIPT_DIR}/btlejuice.log"
DETECTION_MODE_FILE="/tmp/btlejuice_detection_mode.tmp"
TRUE_DETECTION_FILE="/tmp/btlejuice_true_detection.tmp"
FALSE_DETECTION_FILE="/tmp/btlejuice_false_detection.tmp"
SUCCESSFUL_LAUNCH_COUNT_FILE="/tmp/btlejuice_successful_launch_count.tmp"
DEVICE_SERIAL=/dev/ttyACM0
ADV_ADDR=DE:AD:BE:EF:AA:AA
#INTERFACE=hci0
#
# ****************************************************************************
#
total_runs=1000
attack_launch_count=0
successful_launches=0
benign_count=0
true_detection=0
false_detection=0
#
# ****************************************************************************

log() {
  echo "$(date +'%Y-%m-%d %H:%M:%S') [$$] - $1" | tee -a "$LOG_FILE"
}

reset_board() {
    log "Reset board"
    nrfjprog --reset
    sleep 1
}

# ****************************************************************************

launch_attack() {
    local mirage_output_file="/tmp/mirage_output_$$"
    local detection="true"

    # Note: stdbuf is used to disable buffering for both stdout and stderr
    stdbuf -o0 -e0 ${MIRAGE_DIR}/mirage_launcher ble_mitm CONNECTION_TYPE=public TARGET=${ADV_ADDR} INTERFACE1=hci1 INTERFACE2=hci2 > "$mirage_output_file" 2>&1 &

    local mirage_pid=$!

    sleep 10
    kill $mirage_pid 2>/dev/null || true
    if [ -n "$mirage_pid" ]; then
        waitpid "$mirage_pid" 2>/dev/null || true
    fi
    sleep 1
    log "Mirage output: "
    log "$(cat "$mirage_output_file")"

    # Mirage may sometimes fail to launched its attack,
    # and it should not be counted as a false positive.
    #if grep -q "Connecting to slave" "$mirage_output_file"; then
    #    log "Attack successfully launched"
    #    detection="true"
    #elif grep -q "[FAIL]" "$mirage_output_file"; then 
    #    log "Mirage encountered an error during the attack"
    #else 
    #    log "Unexpected Mirage error"
    #fi
    if grep -q "[FAIL]" "$mirage_output_file"; then 
        log "Mirage encountered an error during the attack"
        detection="false"
    fi

    # Count the number of successful attack launches
    if [[ "$detection" == true ]]
    then
        content=$(tr -d '[:space:]' < "$SUCCESSFUL_LAUNCH_COUNT_FILE")
        detection_count=$(cat $SUCCESSFUL_LAUNCH_COUNT_FILE)
        detection_count=$((detection_count + 1))
        echo "$detection_count" > "$SUCCESSFUL_LAUNCH_COUNT_FILE"
    fi

    rm -f "$mirage_output_file"
    return 0
}

benign_traffic() {
    local mirage_output_file="/tmp/mirage_output_$$"

    {
        stdbuf -oL -eL ${MIRAGE_DIR}/mirage_launcher "ble_connect|ble_master" ble_master2.SCENARIO=central_hr ble_connect1.TARGET=${ADV_ADDR}
    } > "$mirage_output_file" 2>&1 &

    local mirage_pid=$!

    sleep 20
    reset_board
    log "Mirage output: "
    log "$(cat "$mirage_output_file")"
    
    if grep -q "Connected on device" "$mirage_output_file"; then
        log "Benign traffic successfully launched"
    else 
        log "Unexpected Mirage error"
    fi

    rm -f "$mirage_output_file"
}

# ****************************************************************************
#
monitor_serial() {
    echo "0" > "$TRUE_DETECTION_FILE"
    echo "0" > "$FALSE_DETECTION_FILE"

    #stdbuf -oL dd if=${DEVICE_SERIAL} bs=1 2>/dev/null | while IFS= read -r n1 char; do
    stty -F "$DEVICE_SERIAL" 115200 cs8 -cstopb -parenb -echo
    #cat ${DEVICE_SERIAL} | while read -r line; do
    while true; do
        if read -r line; then
            if [[ "$line" == *"BTLEJuice"* ]]; then
                log $line

                # Check if the file exists
                if [ ! -f "$DETECTION_MODE_FILE" ]; then
                    echo "Error: File '$DETECTION_MODE_FILE' does not exist."
                    return 1
                fi

                # Read the current experiment mode
                content=$(tr -d '[:space:]' < "$DETECTION_MODE_FILE")

                if [ "$content" = "attack" ]; then
                    echo "1" > "$TRUE_DETECTION_FILE"

                elif [ "$content" = "benign" ]; then 
                    echo "1" > "$FALSE_DETECTION_FILE"

                else
                    log "Invalid content file. Expected 'attack' or 'benign'."
                fi
            fi
        fi

        # To stop the monitoring subshell using stop_monitor()
        if [[ -e "/tmp/stop_monitor" ]]; then
            break
        fi
    done < "$DEVICE_SERIAL"
}

set_detection_mode() {
    if [ "$1" = "attack" ]; then
        echo "attack" > "$DETECTION_MODE_FILE"
    elif [ "$1" = "benign" ]; then 
        echo "benign" > "$DETECTION_MODE_FILE"
    else
        log "Invalid argument. Please use 'attack' or 'benign'."
    fi
}

start_monitor() {
    rm -f "/tmp/stop_monitor"
    monitor_serial &
    monitor_pid=$!
    log "Monitor serial started with PID $monitor_pid"
}

stop_monitor() {
    touch "/tmp/stop_monitor"
    reset_board
    if [ -n "$monitor_pid" ]; then
        wait "$monitor_pid" 2>/dev/null
    fi

    content=$(tr -d '[:space:]' < "$TRUE_DETECTION_FILE")
    if [ "$content" = "1" ]; then
        echo "1" > "$TRUE_DETECTION_FILE"
        true_detection=$((true_detection + 1))
    fi

    content=$(tr -d '[:space:]' < "$FALSE_DETECTION_FILE")
    if [ "$content" = "1" ]; then
        echo "1" > "$FALSE_DETECTION_FILE"
        false_detection=$((false_detection + 1))
    fi

    rm -f "/tmp/stop_monitor"
    return 0
}

# ****************************************************************************

init_experiment() {
    log "************************************"
    log "Initialization"
    reset_board
    touch $DETECTION_MODE_FILE
    echo "0" > "$TRUE_DETECTION_FILE"
    echo "0" > "$FALSE_DETECTION_FILE"
    echo "0" > "$SUCCESSFUL_LAUNCH_COUNT_FILE"
}

clean_experiment() {
    log "Clean up"
    #kill $monitor_pid 2>/dev/null || true

    #true_detection=$(cat $TRUE_DETECTION_FILE)
    #false_detection=$(cat $FALSE_DETECTION_FILE)
    successful_launches=$(cat $SUCCESSFUL_LAUNCH_COUNT_FILE)
    rm -f $DETECTION_MODE_FILE $TRUE_DETECTION_FILE $FALSE_DETECTION_FILE $DETECTION_MODE_FILE $SUCCESSFUL_LAUNCH_COUNT_FILE

    log "======================="
    log "Test campaign completed for evaluating BTLEJuice detection"
    log "Total runs: $total_runs"
    log "Benign traffic runs: $benign_count"
    log "Attack launched runs: $attack_launch_count"
    log "Successful launches: $successful_launches"
    log "True Detections: $true_detection"
    log "False Detections: $false_detection"

    true_positive=$true_detection
    false_positive=$false_detection
    true_negative=$((benign_count - false_detection))
    false_negative=$((successful_launches - true_detection))

    if ((true_positive + false_positive > 0)); then
        precision=$(bc <<< "scale=2; $true_positive / ($true_positive + $false_positive)")
    else
        precision="N/A (division by zero)"
    fi

    if ((true_positive + false_negative > 0)); then
        recall=$(bc <<< "scale=2; $true_positive / ($true_positive + $false_negative)")
    else
        recall="N/A (division by zero)"
    fi

    log "----------------------"
    log "True Positive: $true_positive"
    log "False Positive: $false_positive"
    log "True Negative: $true_negative"
    log "False Negative: $false_negative"
    log "Precision: $precision"
    log "Recall: $recall"
}

# ****************************************************************************

init_experiment

log "Launch a sequence of $total_runs random BTLEJuice attacks or benign traffic"
log "===================================="
for i in $(seq 1 $total_runs)
do
    log "Run $i"

    start_monitor
    sleep 1

    # Chose randomly between launching an attack or benign traffic
    if [ $((RANDOM % 2)) -eq 0 ]; then

        log "Try to launch an attack..."
        # Reset bluetooth dongle
        ${USER_HOME}/tools/ykush-master/bin/ykushcmd -d 1
        ${USER_HOME}/tools/ykush-master/bin/ykushcmd -d 2
        sleep 1
        ${USER_HOME}/tools/ykush-master/bin/ykushcmd -u 1
        ${USER_HOME}/tools/ykush-master/bin/ykushcmd -u 2
        sleep 3
        set_detection_mode attack
        attack_launch_count=$((attack_launch_count + 1))

        launch_attack $i

    else
        log "Generate benign traffic"
        set_detection_mode benign

        benign_count=$((benign_count + 1))
        benign_traffic $i
    fi

    stop_monitor
    sleep 1
done

clean_experiment
