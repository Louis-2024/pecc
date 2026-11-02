#!/usr/bin/env bash

B=/gem5/benchmarks/coremark-pro/builds/linux64/gcc64/bin
ROOT=/gem5/pecc
OUT_ROOT="$ROOT/eembc-out"

# CMDS="$B/core.exe;$B/radix2-big-64k.exe;$B/sha-test.exe;$B/cjpeg-rose7-preset.exe;$B/linear_alg-mid-100x100-sp.exe;$B/zip-test.exe;$B/parser-125k.exe;$B/nnet_test.exe"
# OPTS="-v0 -i1 -c1;-v0 -i1 -c1;-v0 -i1 -c1;-v0 -i1 -c1;-v0 -i1 -c1;-v0 -i1 -c1;-v0 -i1 -c1;-v0 -i1 -c1"

CMDS="$B/parser-125k.exe;$B/sha-test.exe;$B/radix2-big-64k.exe;$B/zip-test.exe"
OPTS="-v0 -i1 -c1;-v0 -i1 -c1;-v0 -i1 -c1;-v0 -i1 -c1"

# for BINARY in \
#     "$B/core.exe" \
#     "$B/radix2-big-64k.exe" \
#     "$B/sha-test.exe" \
#     "$B/cjpeg-rose7-preset.exe" \
#     "$B/linear_alg-mid-100x100-sp.exe" \
#     "$B/zip-test.exe" \
#     "$B/parser-125k.exe" \
#     "$B/nnet_test.exe"

for BINARY in \
    "$B/parser-125k.exe" \
    "$B/sha-test.exe" \
    "$B/radix2-big-64k.exe" \
    "$B/zip-test.exe"
do
    if [ ! -x "$BINARY" ]; then
        echo "Error: workload not found: $BINARY"
        exit 1
    fi
done

if [ "$#" -eq 0 ]; then
    echo "Usage: $0 INCL [EXCL]"
    echo "Examples:"
    echo "  $0 INCL"
    echo "  $0 EXCL"
    echo "  $0 INCL EXCL"
    exit 1
fi

PROTOCOLS=()

# Validate and normalize arguments before starting simulations.
for ARG in "$@"; do
    PROTOCOL="${ARG^^}"

    case "$PROTOCOL" in
        INCL|EXCL)
            ;;
        *)
            echo "Error: unsupported protocol '$ARG'. Use INCL or EXCL."
            exit 1
            ;;
    esac

    # Reject duplicate protocol arguments.
    for EXISTING in "${PROTOCOLS[@]}"; do
        if [ "$EXISTING" = "$PROTOCOL" ]; then
            echo "Error: protocol '$PROTOCOL' was specified more than once."
            exit 1
        fi
    done

    GEM5="$ROOT/build/X86_${PROTOCOL}/gem5.opt"
    if [ ! -x "$GEM5" ]; then
        echo "Error: gem5 executable not found: $GEM5"
        exit 1
    fi

    PROTOCOLS+=("$PROTOCOL")
done

run_protocol() {
    local PROTOCOL="$1"
    local OUT="$OUT_ROOT/$PROTOCOL"
    local GEM5="$ROOT/build/X86_${PROTOCOL}/gem5.opt"
    local OUTPUTS="$OUT/parser.out;$OUT/sha.out;$OUT/radix2.out;$OUT/zip.out" # local OUTPUTS="$OUT/core.out;$OUT/radix2.out;$OUT/sha.out;$OUT/cjpeg.out;$OUT/linear_alg.out;$OUT/zip.out;$OUT/parser.out;$OUT/nnet.out"

    mkdir -p "$OUT"

    echo "Starting $PROTOCOL; log: $OUT/run_log.txt"

    "$GEM5" \
        -d "$OUT" \
        "$ROOT/configs/example/se.py" \
        --ruby \
        --num-cpus 4 \
        --cpu-type TimingSimpleCPU \
        --l1d_size 16kB \
        --l1i_size 16kB \
        --l2_size 2048kB \
        --l1d_assoc 8 \
        --l1i_assoc 8 \
        --l2_assoc 16 \
        --num-l2-banks 8 \
        --mem-type SimpleMemory \
        --mem-size 1GB \
        -c "$CMDS" \
        --options "$OPTS" \
        --output "$OUTPUTS" \
        >"$OUT/run_log.txt" 2>&1
}

cd /gem5/benchmarks/coremark-pro || exit 1

PIDS=()

# All requested protocols start in parallel.
for PROTOCOL in "${PROTOCOLS[@]}"; do
    run_protocol "$PROTOCOL" &
    PIDS+=("$!")
    echo "$PROTOCOL PID: $!"
done

FAILED=0

for INDEX in "${!PIDS[@]}"; do
    PID="${PIDS[$INDEX]}"
    PROTOCOL="${PROTOCOLS[$INDEX]}"

    if wait "$PID"; then
        echo "$PROTOCOL completed successfully."
    else
        STATUS=$?
        echo "$PROTOCOL failed with exit status $STATUS."
        FAILED=1
    fi
done

if [ "$FAILED" -ne 0 ]; then
    exit 1
fi

echo "All requested simulations completed successfully."