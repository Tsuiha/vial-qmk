import math

# ===== 設定 =====
FILENAME = "cal_sqrt_lut.h"
start = 100      # ← ここ変更
size = 3801    # ← ここ変更 maxはstart+size-1
SCALE = 1024

# ===== 入力チェック =====
if start <= 0:
    raise ValueError("start must be > 0 (sqrt(0) or negative is invalid)")

if size <= 0:
    raise ValueError("size must be > 0")

# ===== ファイル生成 =====
with open(FILENAME, "w") as f:
    f.write("#ifndef CAL_SQRT_LUT_H\n")
    f.write("#define CAL_SQRT_LUT_H\n\n")
    f.write("#include <stdint.h>\n\n")
    f.write(f"//start:{start}, end:{start+size-1}\n\n")
    f.write(f"#define CAL_SQRT_START {start}\n")
    f.write(f"#define CAL_SQRT_SIZE {size}\n\n")
    f.write(f"static const uint32_t cal_sqrt_lut[CAL_SQRT_SIZE] = {{\n")
    for i in range(size):
        x = start + i
        value = int(SCALE * math.sqrt(x))

        f.write(f"    {value}, // 2^10 * sqrt({x})\n")

    f.write("};\n\n")
    f.write("#endif // CAL_SQRT_LUT_H\n")

print(f"{FILENAME} generated. start={start}, size={size}")