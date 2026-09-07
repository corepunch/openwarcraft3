#!/bin/sh

set -eu

usage() {
    cat <<'EOF'
Usage: tools/r2_ability.sh [options] Game.dll

Options:
  -a, --address ADDR  Analyze/decompile one address with pdg
  -c, --class NAME    Find RTTI strings matching NAME
  -f, --fourcc CODE   Find the packed FourCC in the binary
  -o, --output FILE   Write the generated r2 command transcript to FILE
  -h, --help          Show this help

The wrapper uses r2ghidra's `pdg` command when it is installed. Without the
plugin it still performs the string/FourCC analysis and prints the exact
missing setup step.
EOF
}

addr=
class_name=
fourcc=
output=
binary=

while [ "$#" -gt 0 ]; do
    case "$1" in
        -a|--address) addr=$2; shift 2 ;;
        -c|--class) class_name=$2; shift 2 ;;
        -f|--fourcc) fourcc=$2; shift 2 ;;
        -o|--output) output=$2; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        -*) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
        *) binary=$1; shift ;;
    esac
done

if [ -z "$binary" ]; then
    usage >&2
    exit 2
fi
if [ ! -f "$binary" ]; then
    echo "ability-r2: binary not found: $binary" >&2
    exit 1
fi
if ! command -v r2 >/dev/null 2>&1; then
    echo "ability-r2: r2 is not installed; install radare2 first" >&2
    exit 1
fi

commands="e bin.cache=true; aaa"
[ -n "$class_name" ] && commands="$commands; izz~$class_name"
if [ -n "$fourcc" ]; then
    if [ "${#fourcc}" -ne 4 ]; then
        echo "ability-r2: --fourcc requires exactly four characters" >&2
        exit 2
    fi
    commands="$commands; / ${fourcc}"
fi
if [ -n "$addr" ]; then
    commands="$commands; s $addr; pdf"
    if r2 -q -c 'pdg?' "$binary" 2>/dev/null | grep -q 'Unknown command'; then
        echo "ability-r2: r2ghidra is unavailable; skipped pdg for $addr" >&2
    else
        commands="$commands; pdg"
    fi
fi
commands="$commands; q"

if [ -n "$output" ]; then
    r2 -q -e bin.cache=true -c "$commands" "$binary" >"$output"
else
    r2 -q -e bin.cache=true -c "$commands" "$binary"
fi
