#!/usr/bin/env bash

set -euo pipefail

DEFAULT_VERSION="v1.17.0"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_ROOT="${SCRIPT_DIR}/../source/soft/mediamtx"

log() {
	printf '[download_mediamtx] %s\n' "$*"
}

fail() {
	printf '[download_mediamtx] ERROR: %s\n' "$*" >&2
	exit 1
}

command -v tar >/dev/null 2>&1 || fail "tar 未安装"

download_file() {
	local url="$1"
	local output="$2"

	if command -v curl >/dev/null 2>&1; then
		curl -L --fail --retry 3 -o "$output" "$url"
		return
	fi

	if command -v wget >/dev/null 2>&1; then
		wget -O "$output" "$url"
		return
	fi

	fail "未找到 curl 或 wget，无法下载 ${url}"
}

resolve_platform() {
	local input_arch="${1:-}"
	local machine_arch
	machine_arch="${input_arch:-$(uname -m)}"

	case "$machine_arch" in
		x86_64|amd64)
			RELEASE_ARCH="amd64"
			TARGET_SUBDIR="ubuntu_x86"
			;;
		aarch64|arm64)
			RELEASE_ARCH="arm64"
			TARGET_SUBDIR="kylin_arm_RK3588"
			;;
		*)
			fail "不支持的架构: ${machine_arch}，可手动传入 x86_64/amd64/aarch64/arm64"
			;;
	esac
}

VERSION="${1:-$DEFAULT_VERSION}"
ARCH_INPUT="${2:-}"

resolve_platform "$ARCH_INPUT"

ARCHIVE_NAME="mediamtx_${VERSION}_linux_${RELEASE_ARCH}.tar.gz"
DOWNLOAD_URL="https://github.com/bluenviron/mediamtx/releases/download/${VERSION}/${ARCHIVE_NAME}"
INSTALL_DIR="${PACKAGE_ROOT}/${TARGET_SUBDIR}"
INSTALL_BIN="${INSTALL_DIR}/mediamtx"

TMP_DIR="$(mktemp -d)"
ARCHIVE_PATH="${TMP_DIR}/${ARCHIVE_NAME}"
EXTRACT_DIR="${TMP_DIR}/extract"

cleanup() {
	rm -rf "$TMP_DIR"
}
trap cleanup EXIT

log "版本: ${VERSION}"
log "架构: ${RELEASE_ARCH}"
log "下载地址: ${DOWNLOAD_URL}"
log "安装目录: ${INSTALL_DIR}"

if [[ -x "$INSTALL_BIN" ]]; then
	log "检测到已安装文件，跳过下载: ${INSTALL_BIN}"
	exit 0
fi

mkdir -p "$PACKAGE_ROOT"
mkdir -p "$INSTALL_DIR"
mkdir -p "$EXTRACT_DIR"

log "开始下载软件包"
download_file "$DOWNLOAD_URL" "$ARCHIVE_PATH"

log "开始解压软件包"
tar -xzf "$ARCHIVE_PATH" -C "$EXTRACT_DIR"

if [[ ! -e "${EXTRACT_DIR}/mediamtx" ]]; then
	fail "解压后未找到 mediamtx 可执行文件"
fi

log "清理旧版本文件"
find "$INSTALL_DIR" -mindepth 1 -maxdepth 1 -exec rm -rf {} +

log "转移新文件到目标目录"
shopt -s dotglob nullglob
mv "$EXTRACT_DIR"/* "$INSTALL_DIR"/
shopt -u dotglob nullglob

chmod +x "$INSTALL_BIN"

log "安装完成: ${INSTALL_BIN}"
log "可执行文件版本检查建议: ${INSTALL_BIN} --version"
