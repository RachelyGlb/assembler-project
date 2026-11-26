#!/usr/bin/env python3
"""
make_page.py: the helper of demo/build_demo.sh.

It reads demo/examples.txt, runs the compiled assembler on every example in a
scratch directory, decodes each object file word by word, and writes one
self-contained HTML page.

The decoder is deliberately independent of the assembler: it knows only the
layout of a machine word and the instruction table, walks the code section
from address 100 using the two lengths of the .ob header, and derives the
extra words of every instruction from that instruction's first word. It never
reads the assembly source or the expected files of the test suite. A word it
cannot explain stops the build with an error instead of being shown as a
guess.

Only the Python 3 standard library is used.
"""

import argparse
import html
import os
import re
import shutil
import subprocess
import sys

LOAD_ADDRESS = 100          # address of the first word of a program
MEMORY_SIZE = 256           # words of memory in the machine
REGISTER_COUNT = 8          # r0 to r7
RUN_TIMEOUT_SECONDS = 20    # limit for one run of the assembler

MODE_NAMES = ("immediate", "direct", "matrix", "register")
MODE_EXAMPLES = ("#-5", "COUNT", "TABLE[r2][r7]", "r3")
ARE_NAMES = {0: "A, absolute", 1: "E, external", 2: "R, relocatable"}

# name, number of operands, legal source modes, legal destination modes.
# The index in the table is the opcode.
INSTRUCTIONS = (
    ("mov", 2, (0, 1, 2, 3), (1, 2, 3)),
    ("cmp", 2, (0, 1, 2, 3), (0, 1, 2, 3)),
    ("add", 2, (0, 1, 2, 3), (1, 2, 3)),
    ("sub", 2, (0, 1, 2, 3), (1, 2, 3)),
    ("lea", 2, (1, 2), (1, 2, 3)),
    ("clr", 1, (), (1, 2, 3)),
    ("not", 1, (), (1, 2, 3)),
    ("inc", 1, (), (1, 2, 3)),
    ("dec", 1, (), (1, 2, 3)),
    ("jmp", 1, (), (1, 2, 3)),
    ("bne", 1, (), (1, 2, 3)),
    ("jsr", 1, (), (1, 2, 3)),
    ("red", 1, (), (1, 2, 3)),
    ("prn", 1, (), (0, 1, 2, 3)),
    ("rts", 0, (), ()),
    ("stop", 0, (), ()),
)

OUTPUT_SUFFIXES = (".am", ".ob", ".ent", ".ext")

# Source files linked from the page, by the part of the program they implement.
STAGE_SOURCES = {
    "driver": ("assembler.c",),
    "pre": ("preassembler.c", "macro_table.c", "line_map.c"),
    "first": ("first_pass.c", "syntax.c", "statement.c", "instruction_table.c",
              "symbol_table.c", "memory_image.c"),
    "second": ("second_pass.c", "encoder.c", "reference_list.c"),
    "output": ("output_writer.c", "base4.c"),
    "errors": ("errors.c",),
}

DIAGNOSTIC = re.compile(r"^(.*\.as):(?:(\d+):)? (error|warning): ")


class DemoError(Exception):
    """A condition that must stop the build of the page."""


def esc(text):
    """Escape text for use in HTML content or in an attribute value."""
    return html.escape(str(text), quote=True)


# --------------------------------------------------------------------------
# Reading the output files
# --------------------------------------------------------------------------

def base4_value(digits, where):
    """Return the number written with the base 4 digits a, b, c, d."""
    value = 0
    if not digits:
        raise DemoError("%s: empty base 4 number" % where)
    for digit in digits:
        if digit not in "abcd":
            raise DemoError("%s: '%s' is not a base 4 number" % (where, digits))
        value = value * 4 + "abcd".index(digit)
    return value


def two_fields(line, where):
    """Split a line of an output file into its two TAB-separated fields."""
    fields = line.split("\t")
    if len(fields) != 2 or not fields[0] or not fields[1]:
        raise DemoError("%s: expected two fields separated by one TAB: %r"
                        % (where, line))
    return fields


def read_lines(path):
    """Return the lines of a text file; every line must end with a newline."""
    with open(path, "r", encoding="utf-8", errors="replace", newline="") as f:
        text = f.read()
    if text and not text.endswith("\n"):
        raise DemoError("%s: the last line has no newline" % path)
    return text.split("\n")[:-1]


def read_object_file(path):
    """
    Parse an .ob file. Return (code length, data length, list of words), where
    each word is a tuple (address, base 4 text, value). The layout of every
    line and the sequence of the addresses are checked.
    """
    lines = read_lines(path)
    if not lines:
        raise DemoError("%s: the file is empty" % path)
    header = two_fields(lines[0], path + ":1")
    for field in header:
        if len(field) > 1 and field.startswith("a"):
            raise DemoError("%s:1: a length has a leading 'a'" % path)
    code_length = base4_value(header[0], path + ":1")
    data_length = base4_value(header[1], path + ":1")
    if len(lines) - 1 != code_length + data_length:
        raise DemoError("%s: the header announces %d words, the file has %d"
                        % (path, code_length + data_length, len(lines) - 1))
    if LOAD_ADDRESS + code_length + data_length > MEMORY_SIZE:
        raise DemoError("%s: the image does not fit in memory" % path)
    words = []
    for number, line in enumerate(lines[1:], 2):
        where = "%s:%d" % (path, number)
        address_text, word_text = two_fields(line, where)
        if len(address_text) != 4 or len(word_text) != 5:
            raise DemoError("%s: wrong field width: %r" % (where, line))
        address = base4_value(address_text, where)
        if address != LOAD_ADDRESS + len(words):
            raise DemoError("%s: address %d is out of sequence"
                            % (where, address))
        words.append((address, word_text, base4_value(word_text, where)))
    return code_length, data_length, words


def read_symbol_file(path):
    """Parse an .ent or .ext file into a list of (symbol, address) pairs."""
    pairs = []
    for number, line in enumerate(read_lines(path), 1):
        where = "%s:%d" % (path, number)
        symbol, address_text = two_fields(line, where)
        if len(address_text) != 4:
            raise DemoError("%s: wrong field width: %r" % (where, line))
        pairs.append((symbol, base4_value(address_text, where)))
    if not pairs:
        raise DemoError("%s: the file is empty" % path)
    return pairs


# --------------------------------------------------------------------------
# The decoder
# --------------------------------------------------------------------------

class Decoder:
    """
    Decodes the words of one object file. `rows` receives one dictionary per
    word with the keys address, base4, bits, role, meaning (HTML) and first
    (true for the first word of an instruction).
    """

    def __init__(self, name, code_length, data_length, words, entries,
                 externals):
        self.name = name
        self.code_length = code_length
        self.data_length = data_length
        self.words = words
        self.entry_names = {}
        for symbol, address in entries:
            self.entry_names.setdefault(address, []).append(symbol)
        self.external_names = {}
        for symbol, address in externals:
            if address in self.external_names:
                self.fail(address, "two .ext lines name this address")
            self.external_names[address] = symbol
        self.used_externals = set()
        self.rows = []

    def fail(self, address, reason):
        """Stop the build: the word at `address` cannot be explained."""
        raise DemoError("%s.ob: cannot decode the word at address %d: %s"
                        % (self.name, address, reason))

    def image_end(self):
        """Return the address after the last word of the image."""
        return LOAD_ADDRESS + self.code_length + self.data_length

    def add_row(self, index, groups, role, meaning, first=False):
        """Record one decoded word; `groups` are the widths of its fields."""
        address, text, value = self.words[index]
        bits = format(value, "010b")
        parts = []
        position = 0
        for width in groups:
            parts.append(bits[position:position + width])
            position += width
        self.rows.append({"address": address, "base4": text,
                          "bits": " ".join(parts), "role": role,
                          "meaning": meaning, "first": first})

    def check_are(self, index, expected):
        """Require the A,R,E field of a word to have the expected value."""
        address, _, value = self.words[index]
        if value & 3 != expected:
            self.fail(address, "the A,R,E field is %s, expected %s"
                      % (format(value & 3, "02b"), format(expected, "02b")))

    def register(self, index, number):
        """Return the name of a register, or fail if there is none."""
        if number >= REGISTER_COUNT:
            self.fail(self.words[index][0],
                      "%d is not a register number" % number)
        return "r%d" % number

    def describe_address(self, address):
        """Return HTML that says what lies at an address of the image."""
        if address < LOAD_ADDRESS + self.code_length:
            text = "address %d, in the code" % address
        else:
            text = "address %d, in the data" % address
        names = self.entry_names.get(address)
        if names:
            text += ", the entry symbol " + " / ".join(
                "<code>%s</code>" % esc(n) for n in names)
        return text

    def decode_immediate(self, index, role):
        """Decode the extra word of an immediate operand."""
        value = self.words[index][2]
        self.check_are(index, 0)
        number = value >> 2
        if number >= 128:
            number -= 256
        self.add_row(index, (8, 2), role,
                     "immediate value <b>%d</b>; A,R,E 00 (A, absolute)"
                     % number)

    def decode_address(self, index, role, what):
        """Decode a word that holds the address of a symbol."""
        address, _, value = self.words[index]
        are = value & 3
        target = value >> 2
        if are == 2:
            if address in self.external_names:
                self.fail(address, "relocatable, but listed in the .ext file")
            if not LOAD_ADDRESS <= target < self.image_end():
                self.fail(address, "address %d is outside the image" % target)
            meaning = ("%s: %s; A,R,E 10 (R, relocatable)"
                       % (what, self.describe_address(target)))
        elif are == 1:
            if target != 0:
                self.fail(address, "external, but the address bits are not 0")
            if address not in self.external_names:
                self.fail(address, "external, but not listed in the .ext file")
            self.used_externals.add(address)
            meaning = ("%s: external symbol <code>%s</code>, address bits 0 "
                       "until linking; A,R,E 01 (E, external); this word is "
                       "listed in the <code>.ext</code> file"
                       % (what, esc(self.external_names[address])))
        else:
            self.fail(address, "an address word cannot have A,R,E %s"
                      % format(are, "02b"))
        self.add_row(index, (8, 2), role, meaning)

    def decode_register(self, index, role, is_source):
        """Decode the extra word of a single register operand."""
        address, _, value = self.words[index]
        self.check_are(index, 0)
        high = (value >> 6) & 15
        low = (value >> 2) & 15
        if is_source:
            if low != 0:
                self.fail(address, "unused bits 5-2 are not zero")
            meaning = "source register <b>%s</b> in bits 9-6" \
                % self.register(index, high)
        else:
            if high != 0:
                self.fail(address, "unused bits 9-6 are not zero")
            meaning = "destination register <b>%s</b> in bits 5-2" \
                % self.register(index, low)
        self.add_row(index, (4, 4, 2), role,
                     meaning + "; A,R,E 00 (A, absolute)")

    def decode_register_pair(self, index):
        """Decode the word shared by two register operands."""
        value = self.words[index][2]
        self.check_are(index, 0)
        source = self.register(index, (value >> 6) & 15)
        destination = self.register(index, (value >> 2) & 15)
        self.add_row(index, (4, 4, 2), "both operands",
                     "source register <b>%s</b> in bits 9-6 and destination "
                     "register <b>%s</b> in bits 5-2, sharing one word; "
                     "A,R,E 00 (A, absolute)" % (source, destination))

    def decode_matrix_indices(self, index, role):
        """Decode the second extra word of a matrix operand."""
        value = self.words[index][2]
        self.check_are(index, 0)
        row = self.register(index, (value >> 6) & 15)
        column = self.register(index, (value >> 2) & 15)
        self.add_row(index, (4, 4, 2), role,
                     "matrix indices: row register <b>%s</b> in bits 9-6, "
                     "column register <b>%s</b> in bits 5-2; "
                     "A,R,E 00 (A, absolute)" % (row, column))

    def decode_operand(self, index, mode, is_source, limit, first_address):
        """
        Decode the extra words of one operand, starting at `index`. Return the
        index after them.
        """
        role = "source operand" if is_source else "destination operand"
        needed = 2 if mode == 2 else 1
        if index + needed > limit:
            self.fail(first_address,
                      "the instruction runs past the end of the code")
        if mode == 0:
            self.decode_immediate(index, role)
        elif mode == 1:
            self.decode_address(index, role, "direct operand")
        elif mode == 2:
            self.decode_address(index, role, "start of the matrix")
            self.decode_matrix_indices(index + 1, role)
        else:
            self.decode_register(index, role, is_source)
        return index + needed

    def decode_instruction(self, index):
        """Decode the instruction that starts at `index`; return the next."""
        address, _, value = self.words[index]
        opcode = value >> 6
        source = (value >> 4) & 3
        destination = (value >> 2) & 3
        name, operands, legal_source, legal_destination = INSTRUCTIONS[opcode]
        self.check_are(index, 0)
        if operands == 2 and source not in legal_source:
            self.fail(address, "%s does not allow source mode %d"
                      % (name, source))
        if operands >= 1 and destination not in legal_destination:
            self.fail(address, "%s does not allow destination mode %d"
                      % (name, destination))
        if operands < 2 and source != 0:
            self.fail(address, "%s has no source operand, but the source "
                      "mode field is %d" % (name, source))
        if operands == 0 and destination != 0:
            self.fail(address, "%s has no operands, but the destination "
                      "mode field is %d" % (name, destination))
        parts = ["<b>%s</b>" % name, "opcode %d" % opcode]
        parts.append("source mode %d (%s)" % (source, MODE_NAMES[source])
                     if operands == 2 else "source mode field unused (00)")
        parts.append("destination mode %d (%s)"
                     % (destination, MODE_NAMES[destination])
                     if operands >= 1 else "destination mode field unused (00)")
        parts.append("A,R,E 00 (A, absolute)")
        row_index = len(self.rows)
        self.add_row(index, (4, 2, 2, 2), "instruction", "", first=True)
        limit = self.code_length
        index += 1
        if operands == 2 and source == 3 and destination == 3:
            if index + 1 > limit:
                self.fail(address, "the instruction runs past the end of "
                          "the code")
            self.decode_register_pair(index)
            index += 1
        else:
            if operands == 2:
                index = self.decode_operand(index, source, True, limit,
                                            address)
            if operands >= 1:
                index = self.decode_operand(index, destination, False, limit,
                                            address)
        length = len(self.rows) - row_index
        parts.append("%d word%s in all" % (length, "" if length == 1 else "s"))
        self.rows[row_index]["meaning"] = "; ".join(parts)
        return index

    def decode_data(self, index):
        """Decode one word of the data section."""
        value = self.words[index][2]
        number = value - 1024 if value >= 512 else value
        meaning = "value <b>%d</b>" % number
        if 32 <= number <= 126:
            meaning += ", the character <code>%s</code>" % esc(
                "space" if number == 32 else chr(number))
        self.add_row(index, (10,), "data", meaning)

    def decode(self):
        """Decode the whole image and return the rows."""
        index = 0
        while index < self.code_length:
            index = self.decode_instruction(index)
        for index in range(self.code_length,
                           self.code_length + self.data_length):
            self.decode_data(index)
        for address in sorted(self.external_names):
            if address not in self.used_externals:
                self.fail(address, "listed in the .ext file, but the word is "
                          "not an external reference")
        for address in self.entry_names:
            if not LOAD_ADDRESS <= address < self.image_end():
                raise DemoError("%s.ent: address %d is outside the image"
                                % (self.name, address))
        return self.rows


# --------------------------------------------------------------------------
# Running the examples
# --------------------------------------------------------------------------

def read_examples(path):
    """Return the examples of the data file as a list of dictionaries."""
    examples = []
    with open(path, "r", encoding="utf-8") as f:
        for number, line in enumerate(f, 1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            fields = [field.strip() for field in line.split("|")]
            if len(fields) != 3 or not all(fields):
                raise DemoError("%s:%d: expected 'path | title | description'"
                                % (path, number))
            parts = fields[0].strip("/").split("/")
            if len(parts) != 3 or parts[0] != "tests" or \
                    parts[1] not in ("valid", "errors", "cli"):
                raise DemoError("%s:%d: '%s' is not a case below tests/valid, "
                                "tests/errors or tests/cli"
                                % (path, number, fields[0]))
            examples.append({"path": "/".join(parts), "group": parts[1],
                             "name": parts[2], "title": fields[1],
                             "description": fields[2]})
    if not examples:
        raise DemoError("%s: no examples" % path)
    return examples


def read_text(path):
    """Return the content of a file as text, with LF line endings kept."""
    with open(path, "r", encoding="utf-8", errors="replace", newline="") as f:
        return f.read()


def collect_file(directory, name):
    """
    Gather what one command-line argument left in the scratch directory: the
    source, the outputs and, when there is an object file, its decoded rows.
    """
    result = {"name": name, "source": None, "outputs": {}, "rows": None}
    source = os.path.join(directory, name + ".as")
    if os.path.isfile(source):
        result["source"] = read_text(source)
    for suffix in OUTPUT_SUFFIXES:
        path = os.path.join(directory, name + suffix)
        if os.path.isfile(path):
            result["outputs"][suffix] = read_text(path)
    if ".ob" in result["outputs"]:
        base = os.path.join(directory, name)
        code_length, data_length, words = read_object_file(base + ".ob")
        entries = read_symbol_file(base + ".ent") \
            if ".ent" in result["outputs"] else []
        externals = read_symbol_file(base + ".ext") \
            if ".ext" in result["outputs"] else []
        decoder = Decoder(name, code_length, data_length, words, entries,
                          externals)
        result["rows"] = decoder.decode()
        result["lengths"] = (code_length, data_length)
    return result


def run_example(example, root, work, assembler):
    """Run the assembler on one example and attach the results to it."""
    case = os.path.join(root, example["path"])
    if not os.path.isdir(case):
        raise DemoError("%s: no such case directory" % example["path"])
    directory = os.path.join(work, example["group"] + "_" + example["name"])
    os.makedirs(directory)
    sources = sorted(n for n in os.listdir(case) if n.endswith(".as"))
    if not sources:
        raise DemoError("%s: no .as file" % example["path"])
    for source in sources:
        shutil.copyfile(os.path.join(case, source),
                        os.path.join(directory, source))
    if example["group"] == "cli":
        with open(os.path.join(case, "args"), "r", encoding="utf-8") as f:
            arguments = f.read().split()
    else:
        arguments = [example["name"]]
    run = subprocess.run([assembler] + arguments, cwd=directory,
                         stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, timeout=RUN_TIMEOUT_SECONDS)
    example["arguments"] = arguments
    example["status"] = run.returncode
    example["stdout"] = run.stdout.decode("utf-8", errors="replace")
    example["stderr"] = run.stderr.decode("utf-8", errors="replace")
    check_run(example)
    seen = []
    for argument in arguments:
        if argument not in seen:
            seen.append(argument)
    example["files"] = [collect_file(directory, name) for name in seen]


def check_run(example):
    """Fail the build when a run did not behave as its group requires."""
    label = example["path"]
    status = example["status"]
    lines = example["stdout"].splitlines()
    diagnostics = [DIAGNOSTIC.match(line) for line in lines]
    errors = [m for m in diagnostics if m and m.group(3) == "error"]
    if status < 0:
        raise DemoError("%s: the assembler was killed by signal %d"
                        % (label, -status))
    if example["group"] == "valid":
        if status != 0:
            raise DemoError("%s: a valid example exited with status %d:\n%s"
                            % (label, status, example["stdout"]))
        if errors:
            raise DemoError("%s: a valid example printed an error" % label)
    elif example["group"] == "errors":
        if not errors:
            raise DemoError("%s: an error example printed no error message"
                            % label)
        if status == 0:
            raise DemoError("%s: an error example exited with status 0"
                            % label)
    elif not any(diagnostics):
        raise DemoError("%s: the command-line example printed no diagnostic"
                        % label)


# --------------------------------------------------------------------------
# HTML
# --------------------------------------------------------------------------

STYLE = """
:root{--bg:#fbfaf8;--panel:#f1efea;--ink:#1f2328;--muted:#646a73;
--line:#d9d5cc;--accent:#0b5cad;--mark:#fff0b8;--ok:#1a7f37;--bad:#b3261e;
--mono:ui-monospace,SFMono-Regular,Menlo,Consolas,"Liberation Mono",monospace}
@media (prefers-color-scheme:dark){:root{--bg:#15171a;--panel:#1e2125;
--ink:#e3e5e8;--muted:#9aa1ab;--line:#33383f;--accent:#7cb7f5;
--mark:#4a3f12;--ok:#56c271;--bad:#f2867d}}
*{box-sizing:border-box}
html{-webkit-text-size-adjust:100%}
body{margin:0;background:var(--bg);color:var(--ink);line-height:1.55;
overflow-wrap:break-word;
font:16px/1.55 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
main,header,footer{max-width:68rem;margin:0 auto;padding:0 1rem}
header{padding-top:2.5rem}
h1{font-size:1.7rem;line-height:1.2;margin:0 0 .75rem}
h2{font-size:1.3rem;margin:3rem 0 .75rem;padding-top:.75rem;
border-top:1px solid var(--line)}
h3{font-size:1.1rem;margin:2.5rem 0 .4rem}
h4{font-size:.8rem;margin:1.1rem 0 .35rem;color:var(--muted);
text-transform:uppercase;letter-spacing:.05em;font-weight:600}
h4 code{text-transform:none;letter-spacing:0;font-size:.85rem}
p,ul,ol{margin:.6rem 0}
p,li{max-width:46rem}
a{color:var(--accent)}
code,pre,.mono{font-family:var(--mono);font-size:.875rem}
pre{margin:0;padding:.7rem .8rem;background:var(--panel);
border:1px solid var(--line);border-radius:6px;overflow-x:auto;
line-height:1.45;tab-size:8}
.ln{display:inline-block;min-width:2.5ch;margin-right:1.5ch;
text-align:right;color:var(--muted);-webkit-user-select:none;
user-select:none}
.hit{background:var(--mark)}
.scroll{overflow-x:auto;border:1px solid var(--line);border-radius:6px}
table{border-collapse:collapse;width:100%;font-size:.875rem}
th,td{padding:.3rem .6rem;text-align:left;vertical-align:top;
border-bottom:1px solid var(--line)}
tr:last-child>td{border-bottom:0}
th{background:var(--panel);font-weight:600;white-space:nowrap}
td.mono,td.num{white-space:nowrap}
td.num{text-align:right;font-variant-numeric:tabular-nums}
table.words td:last-child{min-width:18rem}
tr.first>td{border-top:2px solid var(--line)}
tr.data>td{color:var(--ink)}
.role{color:var(--muted);white-space:nowrap}
.pair{display:grid;gap:0 1rem;grid-template-columns:minmax(0,1fr)}
.pair>div{min-width:0}
@media (min-width:56rem){.pair{grid-template-columns:repeat(2,minmax(0,1fr))}}
.meta{color:var(--muted);font-size:.9rem}
.note{color:var(--muted);font-size:.875rem;margin:.35rem 0 0}
.chips{display:flex;flex-wrap:wrap;gap:.4rem;margin:.5rem 0;padding:0;
list-style:none}
.chips li{font-family:var(--mono);font-size:.8rem;padding:.1rem .5rem;
border:1px solid var(--line);border-radius:999px}
.chips .yes{color:var(--ok)}
.chips .no{color:var(--bad);text-decoration:line-through}
.status{font-family:var(--mono);font-weight:600}
.status.zero{color:var(--ok)}
.status.nonzero{color:var(--bad)}
.bits{display:grid;grid-template-columns:repeat(10,minmax(0,1fr));
max-width:30rem;margin:.5rem 0;font-family:var(--mono);font-size:.75rem;
text-align:center}
.bits div{border:1px solid var(--line);padding:.2rem 0;background:var(--panel);
margin-left:-1px;overflow:hidden}
.bits .n{background:none;border-color:transparent;color:var(--muted)}
nav ol{padding-left:1.3rem}
nav li{margin:.15rem 0}
nav .cols{columns:22rem;column-gap:2rem}
nav .cols>div{break-inside:avoid}
.file{margin:1rem 0 0;padding-left:.9rem;border-left:3px solid var(--line)}
footer{margin-top:3rem;padding-top:1rem;padding-bottom:3rem;
border-top:1px solid var(--line);color:var(--muted);font-size:.875rem}
"""


class Page:
    """Builds the HTML of the page from the examples that were run."""

    def __init__(self, examples, repo_url, commit, date):
        self.examples = examples
        self.repo_url = repo_url.rstrip("/")
        self.commit = commit
        self.date = date
        self.out = []

    def add(self, text):
        """Append a piece of HTML."""
        self.out.append(text)

    def blob(self, path):
        """Return the URL of a file of the repository at the built commit."""
        ref = self.commit if self.commit else "HEAD"
        return "%s/blob/%s/%s" % (self.repo_url, ref, path)

    def link(self, path):
        """Return an HTML link to a file of the repository."""
        return '<a href="%s"><code>%s</code></a>' % (
            esc(self.blob(path)), esc(path))

    def links(self, key):
        """Return the links to the source files of one part of the program."""
        return ", ".join(self.link(path) for path in STAGE_SOURCES[key])

    def listing(self, text, marked=()):
        """Return a <pre> with numbered lines; `marked` lines are shaded."""
        if text == "":
            return '<p class="note">The file is empty.</p>'
        lines = text.split("\n")
        if lines[-1] == "":
            lines.pop()
        parts = []
        for number, line in enumerate(lines, 1):
            shown = '<span class="ln">%d</span>%s' % (number,
                                                      esc(line.rstrip("\r")))
            if number in marked:
                shown = '<span class="hit">%s</span>' % shown
            parts.append(shown)
        return "<pre>%s</pre>" % "\n".join(parts)

    def word_table(self, entry):
        """Return the table of the decoded words of one object file."""
        code_length, data_length = entry["lengths"]
        out = ['<p class="note">Header of the file: %d code word%s and %d '
               'data word%s, loaded from address %d.</p>'
               % (code_length, "" if code_length == 1 else "s", data_length,
                  "" if data_length == 1 else "s", LOAD_ADDRESS)]
        if not entry["rows"]:
            return out[0]
        out.append('<div class="scroll"><table class="words"><thead><tr>'
                   "<th>Address</th><th>Base 4</th><th>Binary</th>"
                   "<th>Word</th><th>Meaning of the fields</th>"
                   "</tr></thead><tbody>")
        for row in entry["rows"]:
            classes = "first" if row["first"] else ""
            if row["role"] == "data":
                classes = "data"
            out.append('<tr class="%s"><td class="num">%d</td>'
                       '<td class="mono">%s</td><td class="mono">%s</td>'
                       '<td class="role">%s</td><td>%s</td></tr>'
                       % (classes, row["address"], esc(row["base4"]),
                          esc(row["bits"]), esc(row["role"]), row["meaning"]))
        out.append("</tbody></table></div>")
        return "".join(out)

    def chips(self, entry):
        """Return the list of the output files that were and were not made."""
        items = []
        for suffix in OUTPUT_SUFFIXES:
            made = suffix in entry["outputs"]
            items.append('<li class="%s" title="%s">%s%s</li>' % (
                "yes" if made else "no",
                "written" if made else "not written",
                esc(entry["name"]), suffix))
        return '<ul class="chips">%s</ul>' % "".join(items)

    def file_note(self, entry):
        """Return a sentence about which output files exist, and why."""
        outputs = entry["outputs"]
        if entry["source"] is None:
            return ("There is no file <code>%s.as</code>, so nothing was "
                    "written for this name." % esc(entry["name"]))
        if ".ob" in outputs:
            missing = [s for s in (".ent", ".ext") if s not in outputs]
            if not missing:
                return "All four output files were written."
            return ("Written: <code>.am</code>, <code>.ob</code>%s. Not "
                    "written: %s, because the file would be empty."
                    % ("".join(", <code>%s</code>" % s
                               for s in (".ent", ".ext") if s in outputs),
                       " and ".join("<code>%s</code>" % s for s in missing)))
        if ".am" in outputs:
            return ("The pre-assembler succeeded and wrote <code>.am</code>; "
                    "one of the two passes found errors, so no "
                    "<code>.ob</code>, <code>.ent</code> or <code>.ext</code> "
                    "file was written.")
        return ("The pre-assembler found errors, so not even the "
                "<code>.am</code> file was written and the two passes did "
                "not run.")

    def file_block(self, entry, marked):
        """Add everything known about one source file of an example."""
        name = esc(entry["name"])
        outputs = entry["outputs"]
        self.add(self.chips(entry))
        self.add('<p class="note">%s</p>' % self.file_note(entry))
        if entry["source"] is None:
            return
        self.add('<div class="pair"><div><h4>Source, <code>%s.as</code></h4>'
                 % name)
        self.add(self.listing(entry["source"], marked))
        self.add("</div>")
        if ".am" in outputs:
            self.add("<div><h4>After the pre-assembler, <code>%s.am</code>"
                     "</h4>" % name)
            self.add(self.listing(outputs[".am"]))
            if outputs[".am"] == entry["source"]:
                self.add('<p class="note">Identical to the source: there '
                         "was no macro to expand.</p>")
            self.add("</div>")
        self.add("</div>")
        if entry["rows"] is not None:
            self.add("<h4>Object file, <code>%s.ob</code>, decoded word by "
                     "word</h4>" % name)
            self.add(self.word_table(entry))
            self.add('<div class="pair">')
            for suffix, what in ((".ent", "Entry symbols"),
                                 (".ext", "External references")):
                if suffix in outputs:
                    self.add("<div><h4>%s, <code>%s%s</code></h4>"
                             % (what, name, suffix))
                    self.add(self.symbol_table(outputs[suffix]))
                    self.add("</div>")
            self.add("</div>")

    def symbol_table(self, text):
        """Return a table for an .ent or .ext file, with decimal addresses."""
        out = ['<div class="scroll"><table><thead><tr><th>Symbol</th>'
               "<th>Base 4, as in the file</th><th>Address</th></tr></thead>"
               "<tbody>"]
        for line in text.split("\n")[:-1]:
            symbol, digits = line.split("\t")
            out.append('<tr><td class="mono">%s</td><td class="mono">%s</td>'
                       '<td class="num">%d</td></tr>'
                       % (esc(symbol), esc(digits),
                          base4_value(digits, "symbol file")))
        out.append("</tbody></table></div>")
        return "".join(out)

    def marked_lines(self, example, name):
        """Return the source lines of `name` that a diagnostic points at."""
        marked = set()
        for line in example["stdout"].splitlines():
            match = DIAGNOSTIC.match(line)
            if match and match.group(2) and match.group(1) == name + ".as":
                marked.add(int(match.group(2)))
        return marked

    def example(self, example):
        """Add the section of one example."""
        self.add('<section id="%s"><h3>%s</h3>'
                 % (esc(example["anchor"]), esc(example["title"])))
        self.add("<p>%s</p>" % esc(example["description"]))
        command = "./assembler " + " ".join(example["arguments"])
        status = example["status"]
        self.add('<p class="meta">Case %s. Command: <code>%s</code>. '
                 'Exit status: <span class="status %s">%d</span>.</p>'
                 % (self.link(example["path"]), esc(command),
                    "zero" if status == 0 else "nonzero", status))
        if example["stdout"]:
            self.add("<h4>Messages printed by the assembler</h4>")
            self.add("<pre>%s</pre>" % esc(example["stdout"].rstrip("\n")))
        else:
            self.add('<p class="note">The assembler printed nothing: '
                     "a clean run is silent.</p>")
        if example["stderr"]:
            self.add("<h4>Standard error</h4><pre>%s</pre>"
                     % esc(example["stderr"].rstrip("\n")))
        several = len(example["files"]) > 1
        for entry in example["files"]:
            if several:
                self.add('<div class="file"><h4>File <code>%s</code></h4>'
                         % esc(entry["name"]))
            self.file_block(entry, self.marked_lines(example, entry["name"]))
            if several:
                self.add("</div>")
        self.add("</section>")

    def bit_row(self, cells):
        """Return one row of a word-layout diagram; cells are (span, text)."""
        return "".join('<div style="grid-column:span %d">%s</div>'
                       % (span, esc(text)) for span, text in cells)

    def layout(self, cells):
        """Return a diagram of a 10-bit word with numbered bits."""
        numbers = "".join('<div class="n">%d</div>' % n
                          for n in range(9, -1, -1))
        return '<div class="bits">%s%s</div>' % (numbers, self.bit_row(cells))

    def instruction_table(self):
        """Return the table of the operations, from the decoder's own data."""
        def modes(values):
            return ", ".join(str(v) for v in values) if values else "none"
        out = ['<div class="scroll"><table><thead><tr><th>Operation</th>'
               "<th>Opcode</th><th>Operands</th><th>Source modes</th>"
               "<th>Destination modes</th></tr></thead><tbody>"]
        for opcode, (name, count, source, destination) in \
                enumerate(INSTRUCTIONS):
            out.append('<tr><td class="mono">%s</td><td class="num">%d</td>'
                       '<td class="num">%d</td><td>%s</td><td>%s</td></tr>'
                       % (name, opcode, count, modes(source),
                          modes(destination)))
        out.append("</tbody></table></div>")
        return "".join(out)

    def mode_table(self):
        """Return the table of the four addressing modes."""
        extra = ("1: the value in bits 9-2",
                 "1: the address of the label in bits 9-2",
                 "2: the address of the matrix, then the row register in "
                 "bits 9-6 and the column register in bits 5-2",
                 "1: the register number, in bits 9-6 for a source and in "
                 "bits 5-2 for a destination")
        out = ['<div class="scroll"><table><thead><tr><th>Mode</th>'
               "<th>Name</th><th>Written as</th><th>Extra words</th></tr>"
               "</thead><tbody>"]
        for mode in range(4):
            out.append('<tr><td class="num">%d</td><td>%s</td>'
                       '<td class="mono">%s</td><td>%s</td></tr>'
                       % (mode, MODE_NAMES[mode], esc(MODE_EXAMPLES[mode]),
                          extra[mode]))
        out.append("</tbody></table></div>")
        return "".join(out)

    def header(self):
        """Add the title and the introduction."""
        if self.commit:
            commit = '<a href="%s/commit/%s"><code>%s</code></a>' % (
                esc(self.repo_url), esc(self.commit), esc(self.commit[:7]))
        else:
            commit = "an unknown commit"
        self.add("<header><h1>A two-pass assembler for an imaginary 10-bit "
                 "machine, written in C90</h1>")
        self.add(
            "<p>This project is an assembler: a program that translates "
            "assembly language, the readable form of machine instructions, "
            "into the numeric words a processor executes. The processor here "
            "is an imaginary one with 10-bit words, which keeps every word "
            "small enough to read by eye. The assembler is written in strict "
            "C90, split into small modules with hidden data structures, and "
            "reports every error of a source file with its line number "
            "instead of stopping at the first one.</p>"
            "<p>Nothing in the examples below was typed by hand except the "
            "titles and the one-sentence descriptions. Each source file was "
            "given to the compiled program, and the page shows what that run "
            "printed and wrote. The tables that explain the object files "
            "come from a separate decoder (%s) that knows only the layout of "
            "a machine word, so every table is also an independent check of "
            "the encoder: a word the decoder could not explain would have "
            "stopped the build of this page.</p>"
            % self.link("demo/make_page.py"))
        self.add('<p class="meta">Built on %s (UTC) from commit %s of '
                 '<a href="%s">%s</a>. The rules the assembler implements '
                 "are written down in %s.</p></header>"
                 % (esc(self.date), commit, esc(self.repo_url),
                    esc(self.repo_url.split("://")[-1]),
                    self.link("docs/SPEC.md")))

    def contents(self, groups):
        """Add the table of contents."""
        self.add('<nav aria-label="Contents"><h2 id="contents">Contents</h2>'
                 '<div class="cols"><div><ol>'
                 '<li><a href="#machine">The machine</a></li>'
                 '<li><a href="#stages">The three stages</a></li>'
                 '<li><a href="#outputs">The output files</a></li>'
                 '<li><a href="#reading">How to read the tables</a></li>'
                 "</ol></div>")
        for anchor, title, members in groups:
            self.add('<div><p><a href="#%s">%s</a></p><ol>'
                     % (anchor, esc(title)))
            for example in members:
                self.add('<li><a href="#%s">%s</a></li>'
                         % (esc(example["anchor"]), esc(example["title"])))
            self.add("</ol></div>")
        self.add("</div></nav>")

    def machine(self):
        """Add the description of the machine and of the assembler."""
        self.add('<h2 id="machine">The machine</h2>')
        self.add(
            "<ul><li>A <b>word</b> is 10 bits; bit 9 is the most significant. "
            "Memory has %d words, addresses 0 to %d. A program is loaded "
            "from address %d, so the code and the data of one program share "
            "%d words.</li>"
            "<li>There are eight registers, <code>r0</code> to "
            "<code>r7</code>.</li>"
            "<li>Numbers are integers in two's complement: a data word holds "
            "-512 to 511, and an immediate operand, which has 8 bits, holds "
            "-128 to 127. Characters are stored as ASCII codes.</li>"
            "<li>An instruction takes one to five words: a first word, then "
            "the extra words of its source operand, then those of its "
            "destination operand.</li></ul>"
            % (MEMORY_SIZE, MEMORY_SIZE - 1, LOAD_ADDRESS,
               MEMORY_SIZE - LOAD_ADDRESS))
        self.add("<h4>The first word of an instruction</h4>")
        self.add(self.layout(((4, "opcode"), (2, "source"), (2, "dest."),
                              (2, "A,R,E"))))
        self.add("<h4>An extra word: immediate value or address</h4>")
        self.add(self.layout(((8, "value or address"), (2, "A,R,E"))))
        self.add("<h4>An extra word: registers</h4>")
        self.add(self.layout(((4, "source / row"), (4, "dest. / column"),
                              (2, "A,R,E"))))
        self.add("<h4>A data word</h4>")
        self.add(self.layout(((10, "signed value"),)))
        self.add(
            "<p>The two lowest bits of every code word, called "
            "<b>A,R,E</b>, tell a linker what to do with the word. "
            "<code>00</code> (A, absolute): the word does not depend on "
            "where the program is loaded. <code>10</code> (R, relocatable): "
            "the word holds the address of a label of this file. "
            "<code>01</code> (E, external): the word refers to a symbol of "
            "another file; its address bits are zero and the word is listed "
            "in the <code>.ext</code> file. Data words have no such "
            "field.</p>")
        self.add("<h4>The four addressing modes</h4>")
        self.add(self.mode_table())
        self.add('<p class="note">Two register operands of one instruction '
                 "share a single extra word.</p>")
        self.add("<h4>The sixteen operations</h4>")
        self.add(self.instruction_table())
        self.add("<h4>Base 4 with letters</h4>")
        self.add(
            "<p>The output files write every number in base 4 with the "
            "digits <code>a</code> = 0, <code>b</code> = 1, <code>c</code> = "
            "2, <code>d</code> = 3. One base 4 digit is two bits, so a word "
            "is exactly five letters and an address four: the word "
            "<code>0000100100</code> is <code>aacba</code>, and address 100 "
            "is <code>bcba</code>.</p>")
        self.add('<h2 id="stages">The three stages</h2>')
        self.add(
            "<p>The command <code>./assembler x y</code> processes "
            "<code>x.as</code> and <code>y.as</code> one after the other, "
            "each through three stages (driver: %s).</p><ol>"
            "<li><b>Pre-assembler.</b> Checks the lines, collects the macro "
            "definitions (<code>mcro name</code> &hellip; "
            "<code>mcroend</code>) and writes the <code>.am</code> file, in "
            "which every macro call is replaced by the body of the macro. "
            "If it finds an error it writes no file and the passes do not "
            "run. Sources: %s.</li>"
            "<li><b>First pass.</b> Parses every line of the "
            "<code>.am</code> file, reports syntax errors, builds the symbol "
            "table, stores the data and encodes everything that does not "
            "need the address of a label. Sources: %s.</li>"
            "<li><b>Second pass.</b> Fills in the addresses of the labels, "
            "which may be defined after their use, handles "
            "<code>.entry</code> and records every reference to an external "
            "symbol. It runs only if the first pass found no error. "
            "Sources: %s.</li></ol>"
            "<p>Diagnostics have the form <code>file.as:line: error: "
            "text</code> (or <code>warning:</code>) and always name the line "
            "of the original <code>.as</code> file, also for a line that "
            "came out of a macro. They all pass through one function "
            "(%s). The exit status is 0 only if every file was assembled "
            "without an error.</p>"
            % (self.links("driver"), self.links("pre"), self.links("first"),
               self.links("second"), self.links("errors")))
        self.add('<h2 id="outputs">The output files</h2>')
        self.add(
            "<ul><li><code>.am</code>: the source after macro expansion. It "
            "is kept even when a pass fails.</li>"
            "<li><code>.ob</code>: the object file. Its first line holds the "
            "number of code words and the number of data words; every other "
            "line holds an address and the word stored there. The code "
            "comes first, the data directly after it.</li>"
            "<li><code>.ent</code>: one line for every symbol declared "
            "<code>.entry</code>, with its address, for use by other "
            "files.</li>"
            "<li><code>.ext</code>: one line for every word that refers to "
            "a symbol declared <code>.extern</code>, with the address of "
            "that word.</li></ul>"
            "<p>The last three are written only when the file has no error, "
            "and <code>.ent</code> or <code>.ext</code> only when it would "
            "not be empty. Sources: %s.</p>" % self.links("output"))
        self.add('<h2 id="reading">How to read the tables</h2>')
        self.add(
            "<p>Each row of an object-file table is one word of memory. "
            "<b>Base 4</b> is the word exactly as it stands in the "
            "<code>.ob</code> file; <b>Binary</b> is the same word with a "
            "space between its fields. A thicker line starts a new "
            "instruction, and the rows below it are its extra words. In the "
            "source listings of the error examples, the lines that a "
            "message points at are shaded.</p>")

    def render(self):
        """Return the whole page as a string."""
        titles = (("valid", "valid", "Programs that assemble"),
                  ("errors", "errors", "Programs with errors"),
                  ("cli", "command-line", "Several files on the command "
                                          "line"))
        groups = []
        for key, anchor, title in titles:
            members = [e for e in self.examples if e["group"] == key]
            for example in members:
                example["anchor"] = "%s-%s" % (anchor, example["name"])
            if members:
                groups.append((anchor, title, members))
        self.add('<!DOCTYPE html>\n<html lang="en"><head><meta charset="utf-8">'
                 '<meta name="viewport" content="width=device-width, '
                 'initial-scale=1"><meta name="color-scheme" '
                 'content="light dark">'
                 "<title>A two-pass assembler for an imaginary 10-bit "
                 "machine</title><style>%s</style></head><body>" % STYLE)
        self.header()
        self.add("<main>")
        self.contents(groups)
        self.machine()
        for anchor, title, members in groups:
            self.add('<h2 id="%s">%s</h2>' % (anchor, esc(title)))
            for example in members:
                self.example(example)
        self.add("</main>")
        self.add('<footer>Generated by %s and %s from %s. '
                 '<a href="#contents">Back to the contents</a>.</footer>'
                 "</body></html>\n"
                 % (self.link("demo/build_demo.sh"),
                    self.link("demo/make_page.py"),
                    self.link("demo/examples.txt")))
        return "".join(self.out)


def main():
    """Parse the command line, run the examples and write the page."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", required=True,
                        help="the root directory of the repository")
    parser.add_argument("--work", required=True,
                        help="an existing scratch directory")
    parser.add_argument("--examples", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--repo-url", required=True)
    parser.add_argument("--commit", default="")
    parser.add_argument("--date", required=True)
    args = parser.parse_args()
    try:
        root = os.path.abspath(args.root)
        assembler = os.path.join(root, "assembler")
        if not os.access(assembler, os.X_OK):
            raise DemoError("%s: no such program; run make first" % assembler)
        for paths in STAGE_SOURCES.values():
            for path in paths:
                if not os.path.isfile(os.path.join(root, path)):
                    raise DemoError("%s: a source file linked from the page "
                                    "does not exist" % path)
        examples = read_examples(args.examples)
        for example in examples:
            run_example(example, root, os.path.abspath(args.work), assembler)
        page = Page(examples, args.repo_url, args.commit, args.date).render()
        with open(args.output, "w", encoding="utf-8", newline="\n") as f:
            f.write(page)
    except (DemoError, OSError, subprocess.SubprocessError) as error:
        sys.stderr.write("make_page.py: error: %s\n" % error)
        return 1
    sys.stdout.write("make_page.py: %d examples, %d object files decoded\n"
                     % (len(examples),
                        sum(1 for e in examples for f in e["files"]
                            if f["rows"] is not None)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
