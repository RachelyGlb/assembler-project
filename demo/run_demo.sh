#!/bin/sh
#
# run_demo.sh: a guided run of the assembler in a terminal, step by step.
#
# The script builds the assembler, assembles two programs of the test suite
# and shows every file the runs produce, shows a program with errors and the
# messages it gets, and ends with the summary line of the test suite. It is
# meant for a screen recording and for a first look at the project.
#
# Everything is real: the commands shown are the commands that run. The
# script works on copies in a temporary directory, removes that directory
# when it ends and writes nothing into the repository apart from the build
# products of make, which git ignores.
#
# Usage: sh demo/run_demo.sh
#
# Environment:
#   DEMO_PAUSE  not set: wait for Enter between the steps.
#               a number of seconds: sleep that long instead.
#               0: run straight through (for unattended checks).
#
# Exit status: 0 if every step behaved as expected, non-zero otherwise.

set -eu

MACRO_CASE=tests/valid/macro_single
FULL_CASE=tests/valid/ps
ERROR_CASE=tests/errors/line_numbers_in_macro_body

root=$(cd "$(dirname "$0")/.." && pwd)
work=$(mktemp -d "${TMPDIR:-/tmp}/assembler-demo.XXXXXX")
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM

# Prints the heading of a step.
step() {
    printf '\n==== %s ====\n\n' "$1"
}

# Waits between two steps, as DEMO_PAUSE asks.
pause() {
    if [ -z "${DEMO_PAUSE+set}" ]; then
        printf '\n[Press Enter to continue] '
        read -r _answer || true
    elif [ "$DEMO_PAUSE" != 0 ]; then
        sleep "$DEMO_PAUSE"
    fi
}

# Prints a command the way it would be typed.
show_command() {
    printf '$ %s\n' "$1"
}

# Prints a file of the scratch directory with line numbers, or says that the
# assembler did not write it.
show_file() {
    if [ -f "$work/$1" ]; then
        show_command "nl -ba $1"
        nl -ba "$work/$1"
    else
        printf '(%s was not written)\n' "$1"
    fi
    printf '\n'
}

# Copies the source of a case into the scratch directory and prints it.
show_source() {
    cp "$root/$1/$(basename "$1").as" "$work/"
    show_file "$(basename "$1").as"
}

# Runs the assembler on one name in the scratch directory, prints its exit
# status and fails the script if the status is not the expected one.
assemble() {
    show_command "./assembler $1"
    status=0
    (cd "$work" && "$root/assembler" "$1") || status=$?
    printf '(exit status %s)\n' "$status"
    if [ "$status" -ne "$2" ]; then
        echo "run_demo: expected exit status $2" >&2
        exit 1
    fi
}

macro_name=$(basename "$MACRO_CASE")
full_name=$(basename "$FULL_CASE")
error_name=$(basename "$ERROR_CASE")

step "1. Build: gcc -Wall -ansi -pedantic, no warnings"
show_command "make"
(cd "$root" && make)
pause

step "2. A small program with a macro"
show_source "$MACRO_CASE"
pause

step "3. Assemble it: a clean run prints nothing"
assemble "$macro_name" 0
pause

step "4. The pre-assembler expanded the macro (.am)"
show_file "$macro_name.am"
pause

step "5. The object file (.ob): lengths, then address and word, in base 4"
show_file "$macro_name.ob"
pause

step "6. A program with entry and external symbols"
show_source "$FULL_CASE"
pause

step "7. Assemble it"
assemble "$full_name" 0
show_file "$full_name.ob"
pause

step "8. Entry symbols (.ent) and external references (.ext)"
show_file "$full_name.ent"
show_file "$full_name.ext"
pause

step "9. A program with errors inside macro bodies"
show_source "$ERROR_CASE"
pause

step "10. Every error is reported, with the line of the original source"
assemble "$error_name" 1
printf '\n'
show_file "$error_name.am"
show_file "$error_name.ob"
pause

step "11. The test suite"
show_command "make test"
if ! (cd "$root" && make test) > "$work/test.log" 2>&1; then
    tail -n 20 "$work/test.log"
    echo "run_demo: the test suite failed" >&2
    exit 1
fi
tail -n 1 "$work/test.log"
