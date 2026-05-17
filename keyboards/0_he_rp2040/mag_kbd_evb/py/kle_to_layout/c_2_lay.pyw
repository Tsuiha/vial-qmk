import tkinter as tk
from tkinter import filedialog
import re

UNIT = 60
PADDING = 20

SIZE_MAP = {
    "U_1": 1,
    "U_1_25": 1.25,
    "U_1_5": 1.5,
    "U_1_75": 1.75,
    "U_2": 2,
    "U_2_25": 2.25,
    "U_2_75": 2.75,
    "U_3": 3,
    "U_4_5": 4.5,
    "U_6": 6,
    "U_6_25": 6.25,
    "U_7": 7,
}

keys = []
rows = 0
cols = 0


# --- C読み込み ---
def load_c():
    global rows, cols

    path = filedialog.askopenfilename(
        filetypes=[("C files", "*.c"), ("All files", "*.*")]
    )
    if not path:
        return

    with open(path, "r", encoding="utf-8") as f:
        text = f.read()

    # rows / cols取得
    m1 = re.search(r'rows\s*=\s*(\d+)', text)
    m2 = re.search(r'cols\s*=\s*(\d+)', text)

    rows = int(m1.group(1)) if m1 else 0
    cols = int(m2.group(1)) if m2 else 0

    parse_c(text)
    draw_layout()


# --- C解析 ---
def parse_c(text):
    global keys
    keys = []

    matches = re.findall(
        r'\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*([A-Za-z0-9_]+)\s*\}',
        text
    )

    for idx, m in enumerate(matches):
        valid = int(m[0])
        x = int(m[1])
        y = int(m[2])
        size_str = m[3]

        if size_str == "ISO":
            w = 1
            iso = True
        elif size_str in SIZE_MAP:
            w = SIZE_MAP[size_str]
            iso = False
        else:
            w = int(size_str) / 4
            iso = False

        row = idx // cols if cols else 0
        col = idx % cols if cols else 0

        keys.append({
            "valid": valid,
            "x": x,
            "y": y,
            "w": w,
            "iso": iso,
            "row": row,
            "col": col
        })


# --- 描画 ---
def draw_layout():
    canvas.delete("all")

    invalid_list = []

    for k in keys:
        if not k["valid"]:
            invalid_list.append(f"{k['row']},{k['col']}")
            continue

        px = PADDING + (k["x"] / 4) * UNIT
        py = PADDING + (k["y"] / 4) * UNIT
        pw = k["w"] * UNIT
        ph = UNIT

        canvas.create_rectangle(px, py, px + pw, py + ph,
                                fill="#aaffaa", outline="#333")

        label = f"{k['row']},{k['col']}"
        if k["iso"]:
            label += "\n<ISO>"

        canvas.create_text(px + pw/2, py + ph/2, text=label)

    # --- invalid一覧表示 ---
    if invalid_list:
        text = "INVALID KEYS:\n" + ", ".join(invalid_list)
    else:
        text = "INVALID KEYS: none"

    canvas.create_text(
        PADDING,
        canvas.winfo_height() - 10,
        text=text,
        anchor="sw",
        fill="red",
        font=("Arial", 10)
    )


# --- GUI ---
root = tk.Tk()
root.title("C → Layout Viewer (invalid list)")

tk.Button(root, text="Cファイル読み込み", command=load_c).pack()

canvas = tk.Canvas(root, bg="#ddd", width=1200, height=500)
canvas.pack()

root.mainloop()