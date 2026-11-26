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
MODES=${MODES:-"0 1 2"}
SEEDS=${SEEDS:-"0"}
TIMEOUT=${TIMEOUT:-60}
STRICT_EXPECT=${STRICT_EXPECT:-0}
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

echo "Exact completion-time check: $([ "$STRICT_EXPECT" = 1 ] && echo enabled || echo disabled)"

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
    local line
    line=$(grep -m1 '^RESULT ' "$out") || return
    if [ ! -f "$RESULTS" ]; then
        echo "$line" | awk '{ printf "input"; for (i = 2; i <= NF; i++) { split($i, kv, "="); printf ",%s", kv[1] } print "" }' > "$RESULTS"
    fi
    echo "$line" | awk -v input="$1" '{ printf "%s", input; for (i = 2; i <= NF; i++) { split($i, kv, "="); printf ",%s", kv[2] } print "" }' >> "$RESULTS"
}

get_field() {
    printf '%s\n' "$1" | awk -v key="$2" '{
        for (i = 2; i <= NF; i++) {
            n = index($i, "=");
            if (n && substr($i, 1, n - 1) == key) {
                print substr($i, n + 1); exit
            }
        }
    }'
}

check_run() {
    local file=$1 mode=$2 result got want expect modes bad
    result=$(grep -m1 '^RESULT ' "$out")
    [ -z "$result" ] && { echo "no RESULT line in output"; return; }
    [ "$(get_field "$result" status)" != ok ] && { echo "status is not ok"; return; }

    got=$(grep -c '^PROC ' "$out" || true)
    want=$(get_field "$result" processes)
    [ -z "$want" ] && { echo "missing processes= in RESULT"; return; }
    [ "$got" -ne "$want" ] && { echo "found $got PROC lines, expected $want"; return; }
    [ "$(get_field "$result" completed)" != "$want" ] && {
        echo "not all processes completed"; return;
    }

    bad=$(awk '
        /^PROC / {
            delete v
            for (i = 2; i <= NF; i++) {
                n = index($i, "=")
                if (n) v[substr($i, 1, n - 1)] = substr($i, n + 1)
            }
            if (!("pid" in v) || !("arrival" in v) || !("completion" in v) ||
                !("work" in v) || !("io_count" in v) || !("io_total" in v)) {
                malformed++
                next
            }
            if (v["completion"] + 0 < v["arrival"] + v["work"] + v["io_total"]) early++
        }
        END { print (malformed + 0) "," (early + 0) }
    ' "$out")
    case "$bad" in
        0,0) ;;
        *,*) echo "malformed/too-early PROC records: $bad"; return ;;
    esac

    if [ "$mode" = 1 ]; then
        got=$(get_field "$result" context_switches)
        want=$(awk '
            /^PROC / {
                for (i = 2; i <= NF; i++) {
                    n = index($i, "=")
                    if (n && substr($i, 1, n - 1) == "io_count") s += substr($i, n + 1)
                }
                p++
            }
            END { print p + s }
        ' "$out")
        [ "$got" != "$want" ] && {
            echo "cooperative context_switches=$got, expected $want"; return;
        }
    fi

    if [ "$STRICT_EXPECT" = 1 ]; then
        expect=$(grep -m1 '^# *expect:' "$file")
        if [ -n "$expect" ]; then
            modes=$(printf '%s\n' "$expect" | grep -o 'modes=[0-9,]*' | cut -d= -f2)
            if echo ",$modes," | grep -q ",$mode,"; then
                want=$(printf '%s\n' "$expect" | grep -o 'sorted_completions=[0-9,]*' | cut -d= -f2)
                got=$(grep '^PROC ' "$out" | grep -o 'completion=[0-9]*' | cut -d= -f2 | sort -n | paste -sd, -)
                [ "$got" != "$want" ] && {
                    echo "completions $got, expected $want (STRICT_EXPECT=1)"; return;
                }
            fi
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
                if [ "$rc" -eq 124 ]; then
                    report FAIL "$name" "timed out after ${TIMEOUT}s"
                    continue
                fi
                if [ "$rc" -ne 0 ]; then
                    report FAIL "$name" "exit status $rc: $(head -n1 "$err")"
                    continue
                fi
                reason=$(check_run "$file" "$mode")
                if [ -z "$reason" ]; then
                    save_result "$(basename "$file")"
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
if [ $((pass + fail)) -eq 0 ]; then
    echo "error: no test inputs found under tests/debug or tests/inputs"
    exit 1
fi
[ "$fail" -eq 0 ]
