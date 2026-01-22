#!/bin/bash

# 1. 参数校验
TARGET_DIR=$1
if [ -z "$TARGET_DIR" ] || [ ! -d "$TARGET_DIR" ]; then
    echo "使用方法: $0 <文件夹路径>" >&2
    exit 1
fi

# 2. 扫描并输出
# 使用 find 查找所有文件（排除 .git 等隐藏目录）
find "$TARGET_DIR" -type f -not -path '*/.*' | while read -r file; do
    # 打印分隔符和文件名信息
    echo "================================================================================"
    echo "FILE: $(basename "$file")"
    echo "PATH: $(realpath "$file")"
    echo "--------------------------------------------------------------------------------"
    
    # 打印文件实际内容
    cat "$file"
    
    echo -e "\n"
done

# How to use:
# bash ./collect_files.sh /home/cs/DockerRoot/fast_cpp_server/src >> all.txt 