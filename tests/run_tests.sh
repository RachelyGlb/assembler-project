#!/bin/sh
#
# run_tests.sh - black-box test runner for the assembler.
#
# Shell: POSIX sh (no bash features are used).
#
# Usage:
#   sh tests/run_tests.sh                 run every case
#   sh tests/run_tests.sh <case> ...      run only the named cases; a name is
#                                         either <case> or <group>/<case>
#
# Environment:
#   ASSEMBLER   path of the binary under test (default: ./assembler). A
#               relative path is resolved against the current directory.
#
# For every case the inputs are copied into a fresh scratch directory under
# tests/out/<group>/<case>/, the binary is run there, and the result is
# compared with the expected files stored next to the input. The scratch
# directory of a passing case is removed; the scratch directory of a failing
# case is kept, together with diff.txt (what differed), stdout.txt and
# stderr.txt.
#
# Output: one line per case, "PASS <group>/<case>" or
# "FAIL <group>/<case>: <reason>", then "passed N, failed M, total T".
# Exit status: 0 if nothing failed, 1 if a case failed, 2 if the runner could
# not start (no binary).
#
# See tests/README.md for the layout of the cases and for what is compared.

set -u

TESTS_DIR=$(cd "$(dirname "$0")" && pwd)
OUT_DIR=$TESTS_DIR/out
GROUPS_ALL="valid errors cli"
OUTPUT_EXTS="am ob ent ext"
TIME_LIMIT=5
STATUS_TIMEOUT=124
STATUS_SIGNAL_BASE=128

ASSEMBLER=${ASSEMBLER:-./assembler}
case $ASSEMBLER in
    /*) BIN=$ASSEMBLER ;;
    *)  BIN=$(pwd)/$ASSEMBLER ;;
esac

if [ ! -f "$BIN" ] || [ ! -x "$BIN" ]; then
    echo "run_tests.sh: assembler binary not found or not executable: $BIN" >&2
    echo "run_tests.sh: build it first, or set ASSEMBLER to its path" >&2
    exit 2
fi

if command -v timeout >/dev/null 2>&1; then
    TIMEOUT="timeout $TIME_LIMIT"
else
    TIMEOUT=""
fi

passed=0
failed=0

# State of the case that is running.
scratch=""
reason=""
extra=0
status=0

# Records one failed check: the first reason is shown on the FAIL line, every
# reason is appended to diff.txt in the scratch directory.
#   $1  a short reason
fail() {
    if [ -z "$reason" ]; then
        reason=$1
    else
        extra=$((extra + 1))
    fi
    printf '== %s\n' "$1" >> "$scratch/diff.txt"
}

# Creates the empty scratch directory of a case and copies its inputs.
#   $1  group, $2  case name, $3  case directory
prepare() {
    scratch=$OUT_DIR/$1/$2
    reason=""
    extra=0
    rm -rf "$scratch"
    mkdir -p "$scratch"
    for src in "$3"/*.as; do
        [ -f "$src" ] && cp "$src" "$scratch/"
    done
}

# Runs the binary inside the scratch directory with the given arguments.
# Standard output and standard error go to stdout.txt and stderr.txt. Sets
# $status. Returns 1 (after recording the failure) on a timeout or a crash.
run_binary() {
    # The explicit "exit" keeps the subshell alive after the binary ends, so
    # that a "Segmentation fault" notice of the shell lands in /dev/null.
    (
        cd "$scratch" || exit 2
        $TIMEOUT "$BIN" "$@" > stdout.txt 2> stderr.txt < /dev/null
        exit $?
    ) 2>/dev/null
    status=$?
    if [ "$status" -eq "$STATUS_TIMEOUT" ]; then
        fail "timeout after $TIME_LIMIT seconds"
        return 1
    fi
    if [ "$status" -gt "$STATUS_SIGNAL_BASE" ]; then
        fail "crash (exit status $status, signal $((status - STATUS_SIGNAL_BASE)))"
        return 1
    fi
    return 0
}

# Checks the exit status of the last run.
#   $1  expected status
check_status() {
    if [ "$status" -ne "$1" ]; then
        fail "exit status $status, expected $1"
    fi
}

# Checks that a file ends with a newline character (an empty file passes).
#   $1  path
ends_with_newline() {
    [ -z "$(tail -c 1 "$1")" ]
}

# Compares one generated file with its expected file. If no expected file
# exists, the generated file must not exist either.
#   $1  directory holding the expected file
#   $2  file name
#   $3  "exact" (byte-identical) or "anyorder" (same lines in any order)
check_file() {
    if [ ! -f "$1/$2" ]; then
        if [ -e "$scratch/$2" ]; then
            fail "$2 was created but must not exist"
        fi
        return
    fi
    if [ ! -f "$scratch/$2" ]; then
        fail "$2 was not created"
        return
    fi
    if [ "$3" = "anyorder" ]; then
        LC_ALL=C sort "$1/$2" > "$scratch/$2.expected.sorted"
        LC_ALL=C sort "$scratch/$2" > "$scratch/$2.actual.sorted"
        if ! diff "$scratch/$2.expected.sorted" "$scratch/$2.actual.sorted" \
                > "$scratch/$2.diff" 2>&1; then
            fail "$2 differs (lines compared in any order)"
            cat "$scratch/$2.diff" >> "$scratch/diff.txt"
        elif ! ends_with_newline "$scratch/$2"; then
            fail "$2 does not end with a newline"
        fi
    elif ! diff "$1/$2" "$scratch/$2" > "$scratch/$2.diff" 2>&1; then
        fail "$2 differs"
        cat "$scratch/$2.diff" >> "$scratch/diff.txt"
    fi
}

# Checks all four output files of one source file name.
#   $1  directory holding the expected files, $2  base name
check_outputs() {
    check_file "$1" "$2.am" exact
    check_file "$1" "$2.ob" exact
    check_file "$1" "$2.ent" anyorder
    check_file "$1" "$2.ext" anyorder
}

# Writes the diagnostics found in stdout.txt to messages.actual, one per line
# as "<file> <line> <severity>", sorted; <line> is "-" for a message without
# a line number. Only lines in the format of SPEC ERR-R13 and ERR-R14 count;
# the text of a message is never looked at.
extract_messages() {
    sed -n \
        -e 's/^\(.*\.as\):\([1-9][0-9]*\): error: ..*$/\1 \2 error/p' \
        -e 's/^\(.*\.as\):\([1-9][0-9]*\): warning: ..*$/\1 \2 warning/p' \
        -e 's/^\(.*\.as\): error: ..*$/\1 - error/p' \
        -e 's/^\(.*\.as\): warning: ..*$/\1 - warning/p' \
        "$scratch/stdout.txt" | LC_ALL=C sort > "$scratch/messages.actual"
}

# Compares messages.actual with messages.expected.
compare_messages() {
    if ! diff "$scratch/messages.expected" "$scratch/messages.actual" \
            > "$scratch/messages.diff" 2>&1; then
        fail "diagnostics differ (expected < > actual)"
        cat "$scratch/messages.diff" >> "$scratch/diff.txt"
    fi
}

# Rewrites messages.actual for a single-file case: the file name is dropped
# when it is the expected one ("<line> <severity>"), and kept otherwise, so
# that a message naming another file can never match.
#   $1  expected file name in messages (<case>.as)
reduce_messages() {
    awk -v want="$1" '{
        if ($1 == want) print $2, $3
        else print $1 ":" $2, $3
    }' "$scratch/messages.actual" | LC_ALL=C sort > "$scratch/messages.reduced"
    mv "$scratch/messages.reduced" "$scratch/messages.actual"
}

# A valid case: exit status 0, every expected file identical, no other
# output file, no error message.
#   $1  case name, $2  case directory
run_valid() {
    prepare valid "$1" "$2"
    run_binary "$1" || return
    check_status 0
    check_outputs "$2" "$1"
    extract_messages
    if grep -q ' error$' "$scratch/messages.actual"; then
        fail "error message printed for a valid file"
        cat "$scratch/messages.actual" >> "$scratch/diff.txt"
    fi
}

# An error case: exit status 1, the diagnostics of <case>.expected, no .ob,
# .ent or .ext, and an .am file only if an expected one exists. A line
# "* <severity>" in <case>.expected means that only the severity of every
# message is compared (the line number is undecided in SPEC 6.1).
#   $1  case name, $2  case directory
run_errors() {
    prepare errors "$1" "$2"
    run_binary "$1" || return
    check_status 1
    check_outputs "$2" "$1"
    extract_messages
    reduce_messages "$1.as"
    LC_ALL=C sort "$2/$1.expected" > "$scratch/messages.expected"
    if grep -q '^\*' "$scratch/messages.expected"; then
        awk '{ print "*", $NF }' "$scratch/messages.actual" | LC_ALL=C sort \
            > "$scratch/messages.reduced"
        mv "$scratch/messages.reduced" "$scratch/messages.actual"
    fi
    compare_messages
}

# A command-line case: the arguments come from the file "args", the exit
# status from "expected_status", the diagnostics from "expected_messages"
# (no such file: none expected). Every expected output file must be
# identical and no other output file may exist.
#   $1  case name, $2  case directory
run_cli() {
    prepare cli "$1" "$2"
    set -f
    # The arguments are split on whitespace on purpose.
    # shellcheck disable=SC2046
    run_binary $(cat "$2/args")
    binary_ok=$?
    set +f
    [ "$binary_ok" -eq 0 ] || return
    check_status "$(cat "$2/expected_status")"
    for ext in $OUTPUT_EXTS; do
        mode=exact
        case $ext in ent|ext) mode=anyorder ;; esac
        for path in "$2"/*."$ext"; do
            [ -e "$path" ] || continue
            check_file "$2" "$(basename "$path")" "$mode"
        done
        for path in "$scratch"/*."$ext"; do
            [ -e "$path" ] || continue
            name=$(basename "$path")
            # Files that have an expected counterpart were checked above.
            [ -f "$2/$name" ] || check_file "$2" "$name" "$mode"
        done
    done
    extract_messages
    if [ -f "$2/expected_messages" ]; then
        LC_ALL=C sort "$2/expected_messages" > "$scratch/messages.expected"
    else
        : > "$scratch/messages.expected"
    fi
    compare_messages
}

# Runs one case and prints its PASS or FAIL line.
#   $1  group, $2  case name
run_case() {
    case_dir=$TESTS_DIR/$1/$2
    "run_$1" "$2" "$case_dir"
    if [ -z "$reason" ]; then
        passed=$((passed + 1))
        rm -rf "$scratch"
        echo "PASS $1/$2"
    else
        failed=$((failed + 1))
        if [ "$extra" -gt 0 ]; then
            reason="$reason (+$extra more, see diff.txt)"
        fi
        echo "FAIL $1/$2: $reason"
    fi
}

# Runs every case of every group.
run_all() {
    rm -rf "$OUT_DIR"
    for group in $GROUPS_ALL; do
        for dir in "$TESTS_DIR/$group"/*/; do
            [ -d "$dir" ] || continue
            run_case "$group" "$(basename "$dir")"
        done
    done
}

# Runs the cases named on the command line. An unknown name is a failure.
#   $1  <case> or <group>/<case>
run_named() {
    found=0
    for group in $GROUPS_ALL; do
        case $1 in
            */*) [ "${1%%/*}" = "$group" ] || continue; name=${1#*/} ;;
            *)   name=$1 ;;
        esac
        if [ -d "$TESTS_DIR/$group/$name" ]; then
            found=1
            run_case "$group" "$name"
        fi
    done
    if [ "$found" -eq 0 ]; then
        failed=$((failed + 1))
        echo "FAIL $1: no such case"
    fi
}

if [ "$#" -eq 0 ]; then
    run_all
else
    for arg in "$@"; do
        run_named "${arg%/}"
    done
fi

echo "passed $passed, failed $failed, total $((passed + failed))"
[ "$failed" -eq 0 ]
