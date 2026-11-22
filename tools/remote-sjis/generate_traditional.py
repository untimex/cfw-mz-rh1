"""Generate 35ELK display aliases from OpenCC and selected lookalikes."""
import hashlib
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "src/extension-code/src"
EXPECTED_SHA256 = "ed1d268e0ad028511dcf5b0089faed0a980ad332449ec11d481ceefde6879f41"


def jisbmp(text):
    if len(text) == 1 and ord(text) <= 0xFFFF:
        try:
            return text.encode("shift_jis")
        except UnicodeEncodeError:
            pass
    return b""


source = Path(sys.argv[1]).read_bytes()
if hashlib.sha256(source).hexdigest() != EXPECTED_SHA256:
    raise SystemExit("Expected OpenCC 1.1.9 STCharacters.txt")
aliases = {}
for line in source.decode("utf-8").splitlines():
    if not line or line.startswith("#"):
        continue
    original, targets = line.split("\t")
    if len(original) != 1 or not 0x3400 <= ord(original) <= 0x9fff or jisbmp(original):
        continue
    for target in targets.split():
        if len(target) == 1 and 0x3400 <= ord(target) <= 0x9fff:
            encoded = jisbmp(target)
            if len(encoded) == 2:
                aliases[ord(original)] = int.from_bytes(encoded, "big")
                break

# These display substitutions use characters available in the 35ELK's encoding.
for original, target in {"你": "祢", "说": "説"}.items():
    aliases[ord(original)] = int.from_bytes(target.encode("shift_jis"), "big")

rows = [
    f"    {{0x{source:04x}, 0x{target:04x}}}," for source, target in sorted(aliases.items())
]
(DATA / "remote_traditional.inc").write_text(
    "/* Generated from OpenCC 1.1.9 STCharacters.txt (Apache-2.0) and two 35ELK aliases. */\n"
    'static const uint16_t traditional_sjis[][2] '
    '__attribute__((section(".sjis_traditional"))) = {\n'
    + "\n".join(rows) + "\n};\n"
)
print(f"{len(aliases)} simplified Han characters have a 35ELK Shift-JIS display alias.")
