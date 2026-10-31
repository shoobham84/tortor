#!/usr/bin/env bash

if [ "$#" -lt 1 ]; then
	echo "Usage: toralize <command> [args....]"
	exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_PRELOAD="${SCRIPT_DIR}/toralize.so"

exec "$@"
