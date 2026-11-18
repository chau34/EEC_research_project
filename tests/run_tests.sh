#!/usr/bin/env bash
# Runs every input in tests/debug and tests/inputs under each mode and seed.
# Called by "make test"; see tests/README.md for details.
#
# Checks for every run:
#   - exits with status 0 within TIMEOUT seconds
#   - every process completes no earlier than arrival + work + I/O time
#   - cooperative only: context switches == processes + I/O events
#   - debug files: completions match the "# expect:" line in the file
# Every run's RESULT line is saved to tests/results.csv.

cd "$(dirname "$0")/.." || exit 1

SIM=./sim
MODES=${MODES:-"0 1"}
SEEDS=${SEEDS:-"0"}
TIMEOUT=${TIMEOUT:-60}
RESULTS=tests/results.csv

if [ ! -x "$SIM" ]; then
    echo "error: $SIM not found; build with 'make sim' first"
    exit 1
fi

pass=0
fail=0
rm -f "$RESULTS"
out=$(mktemp)
err=$(mktemp)
trap 'rm -f "$out" "$err"' EXIT

report() {
    if [ "$1" = PASS ]; then
        pass=$((pass + 1))
        printf "  PASS  %s\n" "$2"
    else
        fail=$((fail + 1))
        printf "  FAIL  %s: %s\n" "$2" "$3"
    fi
}

save_result() {
    line=$(grep -m1 '^RESULT ' "$out") || return
    if [ ! -f "$RESULTS" ]; then
        echo "$line" | awk '{ printf "input"; for (i = 2; i <= NF; i++) { split($i, kv, "="); printf ",%s", kv[1] } print "" }' > "$RESULTS"
    fi
    echo "$line" | awk -v input="$1" '{ printf "%s", input; for (i = 2; i <= NF; i++) { split($i, kv, "="); printf ",%s", kv[2] } print "" }' >> "$RESULTS"
}

check_run() {
    local file=$1 mode=$2
    local result
    result=$(grep -m1 '^RESULT ' "$out")
    [ -z "$result" ] && { echo "no RESULT line in output"; return; }
    echo "$result" | grep -q 'status=ok' || { echo "status is not ok"; return; }

    local bad
    bad=$(awk '/^PROC /{ for (i = 2; i <= NF; i++) { split($i, kv, "="); v[kv[1]] = kv[2] }
                if (v["completion"] < v["arrival"] + v["work"] + v["io_total"]) n++ }
               END { print n + 0 }' "$out")
    [ "$bad" -ne 0 ] && { echo "$bad process(es) finished faster than arrival + work + I/O"; return; }

    if [ "$mode" = 1 ]; then
        local got want
        got=$(echo "$result" | grep -o 'context_switches=[0-9]*' | cut -d= -f2)
        want=$(awk '/^PROC /{ split($7, kv, "="); s += kv[2] + 1 } END { print s + 0 }' "$out")
        [ "$got" != "$want" ] && { echo "cooperative context_switches=$got, expected $want"; return; }
    fi

    local expect
    expect=$(grep -m1 '^# *expect:' "$file")
    if [ -n "$expect" ]; then
        local modes want got
        modes=$(echo "$expect" | grep -o 'modes=[0-9,]*' | cut -d= -f2)
        if echo ",$modes," | grep -q ",$mode,"; then
            want=$(echo "$expect" | grep -o 'sorted_completions=[0-9,]*' | cut -d= -f2)
            got=$(grep '^PROC ' "$out" | grep -o 'completion=[0-9]*' | cut -d= -f2 | sort -n | paste -sd, -)
            [ "$got" != "$want" ] && { echo "completions $got, expected $want"; return; }
        fi
    fi
    echo ""
}

for dir in tests/debug tests/inputs; do
    echo "$dir:"
    for file in "$dir"/*; do
        [ -f "$file" ] || continue
        case "$file" in *.md|*.csv|*.sh) continue ;; esac
        for mode in $MODES; do
            for seed in $SEEDS; do
                name="$(basename "$file") mode=$mode seed=$seed"
                timeout "$TIMEOUT" "$SIM" "$file" "$mode" "$seed" --dump > "$out" 2> "$err"
                rc=$?
                if [ $rc -eq 124 ]; then
                    report FAIL "$name" "timed out after ${TIMEOUT}s"
                    continue
                fi
                save_result "$(basename "$file")" "$seed"
                if [ $rc -ne 0 ]; then
                    report FAIL "$name" "exit status $rc: $(head -n1 "$err")"
                    continue
                fi
                reason=$(check_run "$file" "$mode")
                if [ -z "$reason" ]; then
                    report PASS "$name"
                else
                    report FAIL "$name" "$reason"
                fi
            done
        done
    done
done

echo
echo "$pass passed, $fail failed. Results saved to $RESULTS"
[ $fail -eq 0 ]
