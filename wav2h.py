import sys
from pathlib import Path

arquivo = Path(sys.argv[1])

with open(arquivo, "rb") as f:
    data = f.read()

nome = arquivo.stem.replace(" ", "_")

print(f"unsigned char {nome}_wav[] = {{")

for i, b in enumerate(data):
    if i % 12 == 0:
        print("\n    ", end="")

    print(f"0x{b:02X}, ", end="")

print("\n};")
print(f"\nunsigned int {nome}_wav_len = {len(data)};")