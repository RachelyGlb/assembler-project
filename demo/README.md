# Demo

This directory holds the public demo of the assembler. Nothing here is part
of the assembler itself, and nothing here changes it: both scripts build the
program with `make` and then run the real `./assembler` binary on cases of
the test suite, in a temporary directory.

## `build_demo.sh`: the demo page

```
sh demo/build_demo.sh
```

The script writes `demo/index.html`, one self-contained page (inline CSS, no
script, no external request) that works when it is opened from the disk. For
every example the page shows the source with line numbers, the `.am` file,
every word of the `.ob` file decoded into its fields, the `.ent` and `.ext`
files, the messages the assembler printed and its exit status.

The page is generated, so it is ignored by git and is never committed.

The work is done by `make_page.py` (Python 3, standard library only). Its
decoder does not share any code or data with the assembler. It walks the code
section from address 100 with the two lengths of the `.ob` header, derives
the extra words of each instruction from the first word of that instruction,
and uses the `.ext` file only to name external references. It never reads
the assembly source or the expected files of the test suite, which makes
every table an independent check of the encoder.

The script exits with a non-zero status, and leaves no page, when:

- `make` fails;
- an example below `tests/valid` exits with a non-zero status or prints an
  error;
- an example below `tests/errors` prints no error message, or an example
  below `tests/cli` prints no diagnostic;
- an output file is malformed, or the decoder meets a word it cannot explain
  (an illegal addressing mode, a non-zero unused bit, an external word that
  the `.ext` file does not list, an instruction that runs past the end of
  the code, and so on).

Environment variables:

| Variable | Meaning |
|---|---|
| `DEMO_REPO_URL` | The address of the repository that the links point to. It overrides everything else. |
| `GITHUB_REPOSITORY`, `GITHUB_SERVER_URL`, `GITHUB_SHA` | Set by GitHub Actions and used when present, for the repository address and the commit. |
| `DEMO_OUTPUT` | The file to write. The default is `demo/index.html`. |

Without these variables the commit comes from `git rev-parse HEAD` and the
links point to the default repository address written in the script.

## `examples.txt`: the selection

The examples of the page, in the order in which they are shown. Each line has
three fields separated by `|`:

```
tests/valid/mat_full | A matrix | One sentence that describes the example.
```

The first field is a case directory of the test suite. Its group decides how
the case is run: for `tests/valid` and `tests/errors` the assembler gets the
name of the case; for `tests/cli` it gets the words of the file `args` of the
case, with every `.as` file of the case present. The title and the
description are the only hand-written text of an example. Empty lines and
lines that start with `#` are ignored.

To add an example, add a line and run the script again.

## `run_demo.sh`: the same in a terminal

```
sh demo/run_demo.sh
```

A guided run for a terminal or a screen recording: it builds the assembler,
shows a source file, assembles it, shows the `.am`, `.ob`, `.ent` and `.ext`
files, shows a program with errors and its messages, and ends with the
summary line of `make test`.

The script waits for Enter between the steps. With `DEMO_PAUSE=3` it sleeps
three seconds instead, and with `DEMO_PAUSE=0` it runs straight through:

```
DEMO_PAUSE=0 sh demo/run_demo.sh
```

It works on copies in a temporary directory and removes it at the end.
