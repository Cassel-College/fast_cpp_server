
#!/bin/bash

# This script is used to upload the code to the server.
# Usage: ./uploadcode.sh <server_ip> <username> <password>

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <server_ip> <username> <password>"
    exit 1
fi

SERVER_IP=$1
USERNAME=$2
PASSWORD=$3

target_dir="/home/nvidia/liupeng/fast_cpp_server"
target_dir="/root/liupeng/fast_cpp_server"

log() {
    echo "[$(date '+%F %T')] $*"
}

run_scp() {
    local src_path="$1"
    local dest_sub="$2"
    log "Uploading $src_path -> $USERNAME@$SERVER_IP:$target_dir/$dest_sub"
    log "Command: sshpass -p \"\$PASSWORD\" scp -r \"$src_path\" \"$USERNAME@$SERVER_IP:$target_dir/\""
    sshpass -p "$PASSWORD" scp -r "$src_path" "$USERNAME@$SERVER_IP:$target_dir/"
    local rc=$?
    if [ $rc -eq 0 ]; then
        log "Success: $src_path"
    else
        log "Error($rc): failed to upload $src_path"
        exit $rc
    fi
}

log "Start upload to $SERVER_IP as $USERNAME"
log "Target directory: $target_dir"

# Use scp to upload the code to the server with logging

run_scp ./src src
run_scp ./test test
run_scp ./cmake cmake
# run_scp ./config config
run_scp ./build.sh build.sh
# run_scp ./source source
# run_scp ./other other
run_scp ./CMakeLists.txt CMakeLists.txt

log "Upload finished successfully"




# How to used to upload code to server:
# bash ./uploadcode.sh <server_ip> <username> <password>
# ./uploadcode.sh 172.20.1.23 root qwer1234

