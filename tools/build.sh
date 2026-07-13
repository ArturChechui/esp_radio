#!/usr/bin/env bash

set -euo pipefail

# --------- config ----------
TARGET="${TARGET:-esp32s3}"
PORT="${PORT:-/dev/ttyACM0}"
UNIT_BUILD_DIR="${UNIT_BUILD_DIR:-tests/unit/build}"
USE_IDF_LOADER="${USE_IDF_LOADER:-1}"
# ----------------------------------------------

# Action flags
DO_IDF_BUILD=0
DO_UNIT_BUILD=0
DO_UNIT_RUN=0
DO_FLASH=0
DO_MONITOR=0
DO_FLASH_MONITOR=0
DO_FULLCLEAN=0
DO_GEN_DOCS=0

# TODO: fix the tabs!
usage() {
	cat <<'EOF'
Usage:
  ./build.sh              -> idf.py build
  ./build.sh -u           -> build unit tests (host)
  ./build.sh -r           -> run unit tests (builds them if needed)
  ./build.sh -f           -> flash
  ./build.sh -m           -> monitor
  ./build.sh -F           -> flash + monitor
  ./build.sh -c           -> idf.py fullclean
  ./build.sh -d           -> generate documentation (doxygen)
  ./build.sh -h           -> help

Env overrides (optional):
  TARGET=esp32s3
  PORT=/dev/ttyACM0
  UNIT_BUILD_DIR=tests/unit/build
  USE_IDF_LOADER=1|0
EOF
}

load_idf_env() {
	. $HOME/git/esp/esp-idf/export.sh
}

ensure_target() {
	if [[ -f sdkconfig ]] && grep -q 'CONFIG_IDF_TARGET="'"${TARGET}"'"' sdkconfig; then
		return 0
	fi
	idf.py set-target "${TARGET}"
}

idf_build() {
	load_idf_env
	ensure_target
	# TODO: Try to reuse the cache and speed up the compilation
	# idf.py -j ${nproc} build
	# export IDF_CCACHE_ENABLE=1
	# idf.py --ccache build
	idf.py build
}

idf_fullclean() {
	load_idf_env
	ensure_target
	idf.py fullclean
}

idf_flash() {
	load_idf_env
	ensure_target
	idf.py -p "${PORT}" flash
}

idf_monitor() {
	load_idf_env
	ensure_target
	idf.py -p "${PORT}" monitor
}

idf_flash_monitor() {
	load_idf_env
	ensure_target
	idf.py -p "${PORT}" flash monitor
}

unit_build() {
	mkdir -p "${UNIT_BUILD_DIR}"
	cmake -S tests/unit -B "${UNIT_BUILD_DIR}"
	cmake --build "${UNIT_BUILD_DIR}" -j"$(nproc)"
}

unit_run() {
	unit_build
	ctest --test-dir "${UNIT_BUILD_DIR}" --output-on-failure --verbose

	# TODO: use only src in the report? or both?
	# --filter '.*components/.*/src/.*' \
	gcovr -r . "${UNIT_BUILD_DIR}" \
		--exclude '.*(lib|tests|mock).*' \
		--html --html-details -o "${UNIT_BUILD_DIR}/coverage.html" \
		--print-summary
}

gen_docs() {
	doxygen Doxyfile
}

parse_args() {
	while getopts ":urfmFch" opt; do
		case "${opt}" in
		u) DO_UNIT_BUILD=1 ;;
		r) DO_UNIT_RUN=1 ;;
		f) DO_FLASH=1 ;;
		m) DO_MONITOR=1 ;;
		F) DO_FLASH_MONITOR=1 ;;
		c) DO_FULLCLEAN=1 ;;
		d) DO_GEN_DOCS=1 ;;
		h)
			usage
			exit 0
			;;
		\?)
			echo "ERROR: Unknown option -${OPTARG}" >&2
			usage
			exit 2
			;;
		esac
	done
	shift $((OPTIND - 1))

	# No extra args expected
	if [[ $# -ne 0 ]]; then
		echo "ERROR: Unexpected arguments: $*" >&2
		usage
		exit 2
	fi

	# Default behavior: no flags => idf build
	if [[ $DO_UNIT_BUILD -eq 0 && $DO_UNIT_RUN -eq 0 && $DO_FLASH -eq 0 && $DO_MONITOR -eq 0 && $DO_FLASH_MONITOR -eq 0 && $DO_FULLCLEAN -eq 0 && $DO_GEN_DOCS -eq 0 ]]; then
		DO_IDF_BUILD=1
	fi

	# If -F is used, it supersedes -f/-m
	if [[ $DO_FLASH_MONITOR -eq 1 ]]; then
		DO_FLASH=0
		DO_MONITOR=0
	fi
}

main() {
	parse_args "$@"

	if ((DO_FULLCLEAN)); then idf_fullclean; fi
	if ((DO_IDF_BUILD)); then idf_build; fi
	if ((DO_GEN_DOCS)); then gen_docs; fi

	if ((DO_UNIT_RUN)); then unit_run; fi
	if ((DO_UNIT_BUILD)); then unit_build; fi

	if ((DO_FLASH_MONITOR)); then idf_flash_monitor; fi
	if ((DO_FLASH)); then idf_flash; fi
	if ((DO_MONITOR)); then idf_monitor; fi
}

main "$@"
