import os
import sys

fn_fw = ".pio\\build\\d1_mini\\firmware.bin"
fn_fs = ".pio\\build\\d1_mini\\littlefs.bin"

for fn in [fn_fs, fn_fw]:
    if not os.path.exists(fn):
        print(f"No file {fn}")
        sys.exit(1)

fw = open(fn_fw, "rb").read()
fs = open(fn_fs, "rb").read()

# total flash size 4 Mbyte
# align fw part to 2M
# append fs
fw_align = 0x200000
b = bytearray('\x00'*fw_align, encoding="utf-8")

f = open("full_flash.bin", "wb")
f.write(fw)
f.write(b[:fw_align-len(fw)])
f.write(fs)
f.close()

print("Have a noice day!")
