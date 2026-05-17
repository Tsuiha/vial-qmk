import tkinter as tk
from tkinter import filedialog
import json
import re
import os

UNIT = 60
PADDING = 20

WIDTH_MAP = {
    1: "U_1",
    1.25: "U_1_25",
    1.5: "U_1_5",
    1.75: "U_1_75",
    2: "U_2",
    2.25: "U_2_25",
    2.75: "U_2_75",
    3: "U_3",
    4.5: "U_4_5",
    6: "U_6",
    6.25: "U_6_25",
    7: "U_7",
}

layout_data = None
parsed_keys = []

# --- KLE補正 ---
def kle_fix(text):
    return re.sub(r'([{,]\s*)([a-zA-Z_][a-zA-Z0-9_]*)\s*:', r'\1"\2":', text)

# --- ISO判定 ---
def is_iso(prop):
    return (
        prop.get("w") == 1.25 and
        prop.get("h") == 2 and
        prop.get("w2") == 1.5 and
        prop.get("h2") == 1 and
        prop.get("x2") == -0.25
    )

# --- サイズ変換 ---
def width_to_macro(w, iso):
    if iso:
        return "ISO"
    return WIDTH_MAP.get(w, str(int(w * 4)))

# --- JSON読み込み ---
def load_layout():
    global layout_data

    path = filedialog.askopenfilename(
        filetypes=[("JSON", "*.json"), ("All files", "*.*")]
    )
    if not path:
        return

    with open(path, "r", encoding="utf-8") as f:
        raw = f.read()

    try:
        data = json.loads(raw)
    except:
        data = json.loads(kle_fix(raw))

    if isinstance(data, dict):
        data = data.get("keys", data)

    layout_data = data
    draw_layout()

# --- パース ---
def parse_layout():
    global parsed_keys
    parsed_keys = []

    for y, row in enumerate(layout_data):
        x = 0
        w = 1
        prop = {}

        for item in row:
            if isinstance(item, dict):
                x += item.get("x", 0)
                w = item.get("w", 1)
                prop = item
                continue

            try:
                r, c = map(int, item.split(","))
            except:
                continue

            iso = is_iso(prop)

            parsed_keys.append({
                "row": r,
                "col": c,
                "x": int(x * 4),   # ★ 座標4倍
                "y": int(y * 4),
                "w": w,
                "iso": iso
            })

            x += w
            w = 1
            prop = {}

# --- 描画 ---
def draw_layout():
    canvas.delete("all")

    parse_layout()

    for k in parsed_keys:
        px = PADDING + (k["x"] / 4) * UNIT
        py = PADDING + (k["y"] / 4) * UNIT

        pw = k["w"] * UNIT
        ph = UNIT

        label = f"{k['row']},{k['col']}"
        if k["iso"]:
            label += "\n<ISO>"

        canvas.create_rectangle(px, py, px + pw, py + ph,
                                fill="#eee", outline="#888")
        canvas.create_text(px + pw/2, py + ph/2, text=label)

# --- C生成 ---
def export_c():
    if not parsed_keys:
        return

    max_row = max(k["row"] for k in parsed_keys)
    max_col = max(k["col"] for k in parsed_keys)

    matrix = [[None for _ in range(max_col+1)] for _ in range(max_row+1)]

    for k in parsed_keys:
        matrix[k["row"]][k["col"]] = k

    lines = []

    # --- ヘッダ ---
    lines.append("#include <stdint.h>")
    lines.append('#include "config.h"')
    lines.append('#include "src/include/key_layout.h"')
    lines.append("")

    # --- サイズ情報 ---
    lines.append(f"// rows = {max_row + 1}")
    lines.append(f"// cols = {max_col + 1}")
    lines.append("")

    # --- 配列 ---
    lines.append("const key_layout_t key_layout[MATRIX_SIZE] = {")
    lines.append("//  {valid, x, y, size}")

    idx = 0

    for r in range(max_row+1):
        lines.append(f"\t// row {r}")
        for c in range(max_col+1):
            k = matrix[r][c]

            if k:
                size = width_to_macro(k["w"], k["iso"])
                lines.append(
                    f"\t{{  1, {k['x']}, {k['y']}, {size} }}, // col {c} idx{idx}"
                )
            else:
                lines.append(
                    f"\t{{  0, 0, 0, U_1 }}, // col {c} idx{idx}"
                )

            idx += 1

    lines.append("};")
    lines.append("")

    # --- フッタ ---
    lines.append("/*")
    lines.append("/if you want to use not listed width, enter 4x wanted size[U] integer.")
    lines.append("")
    lines.append("#define ISO        0")
    lines.append("#define U_1        4")
    lines.append("#define U_1_25     5")
    lines.append("#define U_1_5      6")
    lines.append("#define U_1_75     7")
    lines.append("#define U_2        8")
    lines.append("#define U_2_25     9")
    lines.append("#define U_2_75     11")
    lines.append("#define U_3        12")
    lines.append("#define U_4_5      18")
    lines.append("#define U_6        24")
    lines.append("#define U_6_25     25")
    lines.append("#define U_7        28")
    lines.append("")
    lines.append("typedef struct {")
    lines.append("    uint8_t valid;")
    lines.append("    uint8_t x;")
    lines.append("    uint8_t y;")
    lines.append("    uint8_t w;")
    lines.append("} key_layout_t;")
    lines.append("*/")

    # --- 保存先（pywと同じフォルダ） ---
    script_dir = os.path.dirname(os.path.abspath(__file__))
    path = os.path.join(script_dir, "key_layout.c")

    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

def export_json():

    if not parsed_keys:
        return

    # ★ レイアウト順（上→下、左→右）
    sorted_keys = sorted(parsed_keys, key=lambda k: (k["y"], k["x"]))

    lines = []

    for k in sorted_keys:
        x = k["x"] / 4
        y = k["y"] / 4

        line = f'{{"matrix": [{k["row"]}, {k["col"]}], "x": {x}, "y": {y}}}'
        lines.append(line)

    script_dir = os.path.dirname(os.path.abspath(__file__))
    path = os.path.join(script_dir, "kbd_lay.json")

    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

def export_vial_json():

    if not layout_data:
        return

    lines = []
    lines.append("[")

    for i, row in enumerate(layout_data):
        # 行を1行JSONにする
        row_text = json.dumps(row, separators=(',', ':'))

        # 最後以外はカンマ
        if i != len(layout_data) - 1:
            row_text += ","

        lines.append(row_text)

    lines.append("]")

    script_dir = os.path.dirname(os.path.abspath(__file__))
    path = os.path.join(script_dir, "vial_lay.json")

    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))


# --- GUI ---
root = tk.Tk()
root.title("kle_2_lay")

tk.Button(root, text="kle.json読み込み", command=load_layout).pack()
tk.Button(root, text="key_layout.cファイル出力", command=export_c).pack()
tk.Button(root, text="keyboard.jsonのレイアウト部分出力", command=export_json).pack()
tk.Button(root, text="vial.jsonのレイアウト部分出力", command=export_vial_json).pack()

canvas = tk.Canvas(root, bg="#ddd", width=1200, height=500)
canvas.pack()

root.mainloop()