#!/bin/sh
#
# build_demo.sh: builds the demo page, demo/index.html.
#
# The script builds the assembler with make, runs the compiled ./assembler on
# the examples listed in demo/examples.txt (each in a scratch directory, never
# inside tests/ or the repository root) and writes one self-contained HTML
# file that shows, for every example, the source, the files the run produced,
# every word of the object file decoded, the messages and the exit status.
# The running, decoding and HTML are done by demo/make_page.py (Python 3,
# standard library only).
#
# Usage: sh demo/build_demo.sh
#
# Environment:
#   DEMO_REPO_URL      address of the repository used for the links; overrides
#                      everything else.
#   GITHUB_REPOSITORY, GITHUB_SERVER_URL, GITHUB_SHA
#                      set by GitHub Actions; used when present.
#   DEMO_OUTPUT        the file to write (default: demo/index.html).
#
# Exit status: 0 if the page was written. Non-zero if the build fails, if a
# valid example does not assemble, if an error example prints no error, or
# if the decoder meets a word it cannot explain; no page is left in that case.

set -eu

DEFAULT_REPO_URL='https://github.com/RachelyGlb/assembler-project'

demo_dir=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$demo_dir/.." && pwd)
output=${DEMO_OUTPUT:-$demo_dir/index.html}

if [ -n "${DEMO_REPO_URL:-}" ]; then
    repo_url=$DEMO_REPO_URL
elif [ -n "${GITHUB_REPOSITORY:-}" ]; then
    repo_url=${GITHUB_SERVER_URL:-https://github.com}/$GITHUB_REPOSITORY
else
    repo_url=$DEFAULT_REPO_URL
fi

if [ -n "${GITHUB_SHA:-}" ]; then
    commit=$GITHUB_SHA
else
    commit=$(git -C "$root" rev-parse HEAD 2>/dev/null || true)
fi

build_date=$(date -u '+%Y-%m-%d %H:%M')

echo "build_demo: building the assembler"
(cd "$root" && make) >/dev/null

work=$(mktemp -d "${TMPDIR:-/tmp}/assembler-demo.XXXXXX")
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM

echo "build_demo: running the examples of demo/examples.txt"
mkdir "$work/runs"
if ! python3 "$demo_dir/make_page.py" \
        --root "$root" \
        --work "$work/runs" \
        --examples "$demo_dir/examples.txt" \
        --output "$work/index.html" \
        --repo-url "$repo_url" \
        --commit "$commit" \
        --date "$build_date"; then
    echo "build_demo: FAILED, no page was written" >&2
    exit 1
fi

mv "$work/index.html" "$output"
echo "build_demo: wrote $output ($(wc -c < "$output" | tr -d ' ') bytes)"
