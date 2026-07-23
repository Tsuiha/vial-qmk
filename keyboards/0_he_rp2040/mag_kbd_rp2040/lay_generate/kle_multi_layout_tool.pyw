import json
import os
import re
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

script_dir = os.path.dirname(os.path.abspath(__file__))
custom_keycodes_path = os.path.join(script_dir, "customKeycodes.json")


CANVAS_UNIT = 42
CANVAS_MARGIN = 28
KEY_GAP = 4
ISO_LEFT_OFFSET = -0.25
OPTION_COLORS = {
    None: "#f7fbfa",
    0: "#eaf4ff",
    1: "#fff3dc",
    2: "#f1eaff",
    3: "#eaffef",
    4: "#ffeef3",
    5: "#eff1ff",
    6: "#f7f0e8",
    7: "#e8fbff",
}
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
LAYOUT_OPTION_COMMON = "0"


def load_json(path):
    with open(path, "r", encoding="utf-8") as file:
        return json.load(file)


def load_json_fragment(path):
    with open(path, "r", encoding="utf-8") as file:
        text = file.read().strip()
    if not text:
        return {}
    if not text.startswith("{"):
        text = "{\n" + text + "\n}"
    return json.loads(text)


def write_text(path, text):
    with open(path, "w", encoding="utf-8", newline="\n") as file:
        file.write(text)


def normalize_kle_data(data):
    if not isinstance(data, list):
        raise ValueError("Select a KLE array JSON file.")

    metadata = {}
    keymap = []
    for item in data:
        if isinstance(item, dict):
            metadata.update(item)
        elif isinstance(item, list):
            keymap.append(item)
        else:
            raise ValueError("KLE top level must contain row arrays and optional metadata objects.")

    if not keymap:
        raise ValueError("No KLE key rows found.")
    return metadata, keymap


def matrix_size_from_json(matrix, entries):
    rows = int(matrix.get("rows", 0) or 0)
    cols = int(matrix.get("cols", 0) or 0)
    if rows and cols:
        return rows, cols
    max_row = max((entry["row"] for entry in entries), default=-1)
    max_col = max((entry["col"] for entry in entries), default=-1)
    return max_row + 1, max_col + 1


def is_iso(prop):
    return (
        prop.get("h") == 2
        and prop.get("h2") == 1
        and prop.get("x2") == -0.25
    )


def width_to_macro(width, iso):
    if iso:
        return "ISO"
    rounded = round(float(width), 4)
    if rounded in WIDTH_MAP:
        return WIDTH_MAP[rounded]
    scaled = round(rounded * 4)
    if abs(rounded * 4 - scaled) < 0.0001:
        return str(int(scaled))
    raise ValueError(f"Unsupported key width: {width}")


def parse_matrix_label(label):
    first = str(label).split("\n", 1)[0].strip()
    match = re.match(r"^(\d+)\s*,\s*(\d+)$", first)
    if not match:
        return None
    return int(match.group(1)), int(match.group(2))


def parse_option_label(label):
    parts = str(label).split("\n")
    if len(parts) < 4:
        return None, 0
    option_text = parts[3].strip()
    if not option_text:
        return None, 0
    match = re.match(r"^(\d+)\s*,\s*(\d+)$", option_text)
    if not match:
        return None, 0
    return int(match.group(1)), int(match.group(2))


def parse_layout_label(label):
    parts = str(label).split("\n")
    option, choice = parse_option_label(label)
    option_name = parts[4].strip() if len(parts) > 4 else ""
    choice_name = parts[5].strip() if len(parts) > 5 else ""
    return option, choice, option_name, choice_name


def parse_keymap(keymap, include_decals=False):
    entries = []
    current_y = 0.0
    for row in keymap:
        current_x = 0.0
        prop = {}
        row_y_adjusted = False
        for item in row:
            if isinstance(item, dict):
                current_x += float(item.get("x", 0) or 0)
                if not row_y_adjusted and "y" in item:
                    current_y += float(item.get("y", 0) or 0)
                    row_y_adjusted = True
                prop.update(item)
                continue

            matrix = parse_matrix_label(item)
            width = float(prop.get("w", 1) or 1)
            if matrix is not None and (include_decals or not prop.get("d", False)):
                iso = is_iso(prop)
                option, choice = parse_option_label(item)
                row_idx, col_idx = matrix
                entries.append({
                    "row": row_idx,
                    "col": col_idx,
                    "idx": None,
                    "x": int(round(current_x * 4)),
                    "y": int(round(current_y * 4)),
                    "w": width_to_macro(width, iso),
                    "option": option,
                    "choice": choice,
                })
            current_x += width
            prop = {}
        current_y += 1.0
    return entries


def assign_indices(entries, rows, cols):
    for entry in entries:
        if entry["row"] >= rows or entry["col"] >= cols:
            raise ValueError(f"Matrix position out of range: {entry['row']},{entry['col']} for {rows}x{cols}")
        entry["idx"] = entry["row"] * cols + entry["col"]


def c_string(value, max_bytes=15):
    encoded = str(value or "").encode("utf-8")[:max_bytes]
    while True:
        try:
            text = encoded.decode("utf-8")
            break
        except UnicodeDecodeError:
            encoded = encoded[:-1]
    return text.replace("\\", "\\\\").replace('"', '\\"')


def layout_labels_from_detected(layout_options):
    labels = []
    for option, choices in layout_options["choices"].items():
        option_name = layout_options["option_names"].get(option, f"Option {option + 1}")
        for choice in choices:
            choice_name = layout_options["choice_names"].get((option, choice), "")
            if not choice_name:
                if len(choices) == 2:
                    choice_name = "Off" if choice == min(choices) else "On"
                else:
                    choice_name = f"Choice {choice}"
            labels.append({
                "option": option + 1,
                "choice": choice,
                "option_name": option_name,
                "choice_name": choice_name,
            })
    return labels


def format_c(entries, option_count, layout_options):
    lines = []
    lines.append("#include <stdint.h>")
    lines.append('#include "config.h"')
    lines.append('#include "src/include/key_layout.h"')
    lines.append("")
    lines.append("const layout_option_state_t key_layout_default_state = {")
    lines.append(f"    .option_count = {option_count},")
    lines.append("    .choices = {0}")
    lines.append("};")
    lines.append("")
    labels = layout_labels_from_detected(layout_options)
    lines.append("const key_layout_label_t key_layout_labels[] = {")
    for label in labels:
        option_name = c_string(label["option_name"])
        choice_name = c_string(label["choice_name"])
        lines.append(f"    {{ {label['option']}, {label['choice']}, \"{option_name}\", \"{choice_name}\" }},")
    if not labels:
        lines.append('    { 0, 0, "", "" },')
    lines.append("};")
    lines.append("")
    lines.append(f"const uint16_t key_layout_label_count = {len(labels)};")
    lines.append("")
    lines.append("const key_layout_entry_t key_layout_entries[] = {")
    for entry in entries:
        option = LAYOUT_OPTION_COMMON if entry["option"] is None else str(entry["option"] + 1)
        lines.append(
            f"    {{ {entry['idx']:3d}, {entry['x']:3d}, {entry['y']:3d}, {entry['w']:<6}, {option:>20}, {entry['choice']} }},"
        )
    lines.append("};")
    lines.append("")
    lines.append("const uint16_t key_layout_entry_count = sizeof(key_layout_entries) / sizeof(key_layout_entries[0]);")
    lines.append("")
    return "\n".join(lines)


def load_custom_keycodes():
    try:
        data = load_json_fragment(custom_keycodes_path)
    except FileNotFoundError:
        return []
    custom_keycodes = data.get("customKeycodes", [])
    if not isinstance(custom_keycodes, list):
        raise ValueError("customKeycodes.json must contain an array named customKeycodes.")
    return custom_keycodes


def detect_layout_options(keymap):
    options = {}
    option_names = {}
    choice_names = {}

    for row in keymap:
        for item in row:
            if not isinstance(item, str):
                continue
            option, choice, option_name, choice_name = parse_layout_label(item)
            if option is None:
                continue
            options.setdefault(option, set()).add(choice)
            if option_name:
                option_names.setdefault(option, option_name)
            if choice_name:
                choice_names.setdefault((option, choice), choice_name)

    return {
        "choices": {option: sorted(choices) for option, choices in sorted(options.items())},
        "option_names": option_names,
        "choice_names": choice_names,
    }


def matrix_size_from_kle(keymap, entries):
    rows, cols = matrix_size_from_json({}, entries)
    for row in keymap:
        for item in row:
            if isinstance(item, str):
                matrix = parse_matrix_label(item)
                if matrix is not None:
                    rows = max(rows, matrix[0] + 1)
                    cols = max(cols, matrix[1] + 1)
    return rows, cols


def parse_preview_keys(keymap):
    keys = []
    current_y = 0.0
    for row in keymap:
        current_x = 0.0
        prop = {}
        row_y_adjusted = False
        for item in row:
            if isinstance(item, dict):
                current_x += float(item.get("x", 0) or 0)
                if not row_y_adjusted and "y" in item:
                    current_y += float(item.get("y", 0) or 0)
                    row_y_adjusted = True
                prop.update(item)
                continue

            width = float(prop.get("w", 1) or 1)
            height = float(prop.get("h", 1) or 1)
            matrix = parse_matrix_label(item)
            option, choice, _, _ = parse_layout_label(item)
            if matrix is not None and not prop.get("d", False):
                keys.append({
                    "x": current_x,
                    "y": current_y,
                    "w": width,
                    "h": height,
                    "matrix": matrix,
                    "option": option,
                    "choice": choice,
                    "iso": is_iso(prop),
                })
            current_x += width
            prop = {}
        current_y += 1.0
    return keys


def strip_extra_layout_label_lines(label):
    parts = str(label).split("\n")
    if len(parts) <= 4:
        return str(label)
    return "\n".join(parts[:4])


def keymap_for_vial_json(keymap):
    cleaned = []
    for row in keymap:
        cleaned_row = []
        pending_prop = None
        for item in row:
            if isinstance(item, dict):
                pending_prop = dict(item)
                continue

            if not isinstance(item, str):
                continue

            prop = pending_prop or {}
            pending_prop = None
            if prop.get("d", False):
                spacer = float(prop.get("x", 0) or 0) + float(prop.get("w", 1) or 1)
                if spacer:
                    cleaned_row.append({"x": spacer})
                continue

            prop.pop("d", None)
            if prop:
                cleaned_row.append(prop)
            cleaned_row.append(strip_extra_layout_label_lines(item))

        if pending_prop:
            pending_prop.pop("d", None)
            if pending_prop:
                cleaned_row.append(pending_prop)
        cleaned.append(cleaned_row)
    return cleaned


def option_labels_from_detected(layout_options):
    labels = []
    choices_by_option = layout_options["choices"]
    option_names = layout_options["option_names"]
    choice_names = layout_options["choice_names"]

    if not choices_by_option:
        return labels

    for option in range(max(choices_by_option) + 1):
        choices = choices_by_option.get(option, [])
        max_choice = max(choices, default=-1)
        option_name = option_names.get(option, f"Option {option}")
        if max_choice <= 1:
            labels.append(option_name)
            continue

        row = [option_name]
        for choice in range(max_choice + 1):
            row.append(choice_names.get((option, choice), f"Choice {choice}"))
        labels.append(row)
    return labels


def clean_float(value):
    rounded = round(float(value), 4)
    if rounded == int(rounded):
        return int(rounded)
    return rounded


def qmk_layout_item_from_key(key):
    item = {
        "matrix": [key["matrix"][0], key["matrix"][1]],
        "x": clean_float(key["x"]),
        "y": clean_float(key["y"]),
    }
    if key["w"] != 1:
        item["w"] = clean_float(key["w"])
    if key["h"] != 1:
        item["h"] = clean_float(key["h"])
    return item


def format_qmk_layout_fragment(keys):
    lines = [
        "{",
        '    "layouts": {',
        '        "LAYOUT": {',
        '            "layout": [',
    ]
    items = [qmk_layout_item_from_key(key) for key in keys]
    for index, item in enumerate(items):
        suffix = "," if index < len(items) - 1 else ""
        matrix = item["matrix"]
        parts = [
            f'"matrix": [{matrix[0]}, {matrix[1]}]',
            f'"x": {json.dumps(item["x"])}',
            f'"y": {json.dumps(item["y"])}',
        ]
        if "w" in item:
            parts.append(f'"w": {json.dumps(item["w"])}')
        if "h" in item:
            parts.append(f'"h": {json.dumps(item["h"])}')
        lines.append("                {" + ", ".join(parts) + "}" + suffix)
    lines.extend([
        "            ]",
        "        }",
        "    }",
        "}",
        "",
    ])
    return "\n".join(lines)


def group_keys_by_y(keys):
    groups = []
    for key in keys:
        if not groups or key["y"] != groups[-1][0]:
            groups.append((key["y"], []))
        groups[-1][1].append(key)
    return [group_keys for _y, group_keys in groups]


def format_keymap_layer_args(keys, indent="        ", unit_cols=9):
    lines = []
    total = len(keys)
    index = 0
    for group in group_keys_by_y(keys):
        line = indent
        cursor = 0
        for _key in group:
            index += 1
            suffix = "," if index < total else ""
            target = int(round(_key["x"] * unit_cols))
            if len(line) - len(indent) < target:
                line += " " * (target - (len(line) - len(indent)))
            token = f"_______{suffix}"
            line += token
            cursor = max(target + int(round(_key["w"] * unit_cols)), len(line) - len(indent) + 1)
            if len(line) - len(indent) < cursor:
                line += " " * (cursor - (len(line) - len(indent)))
        lines.append(line.rstrip())
    return "\n".join(lines)


def format_keymap_c(keys, layer_count=4):
    lines = [
        "#include QMK_KEYBOARD_H",
        "",
        "enum layer_names {",
    ]
    for layer in range(layer_count):
        suffix = "," if layer < layer_count - 1 else ""
        lines.append(f"    keymap_{layer}{suffix}")
    lines.extend([
        "};",
        "",
        "const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {",
    ])
    for layer in range(layer_count):
        suffix = "," if layer < layer_count - 1 else ""
        lines.append(f"[keymap_{layer}] = LAYOUT(")
        lines.append(format_keymap_layer_args(keys))
        lines.append(f"    ){suffix}")
    lines.extend([
        "};",
        "",
    ])
    return "\n".join(lines)


class KleMultiLayoutTool(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("KLE multi-layout converter")
        self.geometry("1180x760")
        self.minsize(920, 600)

        self.input_path = None
        self.metadata = {}
        self.keymap = None
        self.layout_options = {"choices": {}, "option_names": {}, "choice_names": {}}
        self.preview_keys = []
        self.preview_choice_offsets = {}
        self.selected_choices = {}
        self.choice_vars = {}
        self.keyboard_name_var = tk.StringVar(value="magnetic_keyboard")
        self.status_var = tk.StringVar(value="Open a KLE JSON file.")

        self._build_ui()

    def _build_ui(self):
        toolbar = ttk.Frame(self, padding=8)
        toolbar.pack(side=tk.TOP, fill=tk.X)

        ttk.Button(toolbar, text="Open KLE JSON", command=self.open_json).pack(side=tk.LEFT)
        ttk.Button(toolbar, text="Save all", command=self.save_both).pack(side=tk.RIGHT)

        body = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        body.pack(fill=tk.BOTH, expand=True, padx=8, pady=(0, 8))

        left = ttk.Frame(body)
        right = ttk.Frame(body, width=340)
        body.add(left, weight=4)
        body.add(right, weight=1)

        canvas_frame = ttk.Frame(left)
        canvas_frame.pack(fill=tk.BOTH, expand=True)
        self.canvas = tk.Canvas(canvas_frame, bg="#eef5f2", highlightthickness=0)
        xbar = ttk.Scrollbar(canvas_frame, orient=tk.HORIZONTAL, command=self.canvas.xview)
        ybar = ttk.Scrollbar(canvas_frame, orient=tk.VERTICAL, command=self.canvas.yview)
        self.canvas.configure(xscrollcommand=xbar.set, yscrollcommand=ybar.set)
        self.canvas.grid(row=0, column=0, sticky="nsew")
        ybar.grid(row=0, column=1, sticky="ns")
        xbar.grid(row=1, column=0, sticky="ew")
        canvas_frame.rowconfigure(0, weight=1)
        canvas_frame.columnconfigure(0, weight=1)

        info = ttk.LabelFrame(right, text="Info", padding=8)
        info.pack(fill=tk.X)
        self._add_labeled_entry(info, "name", self.keyboard_name_var)

        option_frame_outer = ttk.LabelFrame(right, text="Preview multi layout", padding=0)
        option_frame_outer.pack(fill=tk.BOTH, expand=True, pady=(8, 0))
        self.option_canvas = tk.Canvas(option_frame_outer, highlightthickness=0)
        option_scroll = ttk.Scrollbar(option_frame_outer, orient=tk.VERTICAL, command=self.option_canvas.yview)
        self.option_canvas.configure(yscrollcommand=option_scroll.set)
        self.option_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        option_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.option_frame = ttk.Frame(self.option_canvas, padding=8)
        self.option_window = self.option_canvas.create_window((0, 0), window=self.option_frame, anchor="nw")
        self.option_frame.bind("<Configure>", self._update_option_scroll)
        self.option_canvas.bind("<Configure>", self._resize_option_window)

        status = ttk.Label(self, textvariable=self.status_var, anchor=tk.W, padding=(8, 0, 8, 8))
        status.pack(side=tk.BOTTOM, fill=tk.X)

    def _add_labeled_entry(self, parent, label, variable):
        row = ttk.Frame(parent)
        row.pack(fill=tk.X, pady=2)
        ttk.Label(row, text=label, width=9).pack(side=tk.LEFT)
        ttk.Entry(row, textvariable=variable).pack(side=tk.LEFT, fill=tk.X, expand=True)

    def _update_option_scroll(self, _event=None):
        self.option_canvas.configure(scrollregion=self.option_canvas.bbox("all"))

    def _resize_option_window(self, event):
        self.option_canvas.itemconfigure(self.option_window, width=event.width)

    def open_json(self):
        path = filedialog.askopenfilename(
            title="Select KLE JSON",
            initialdir=script_dir,
            filetypes=[("JSON", "*.json"), ("All files", "*.*")],
        )
        if not path:
            return

        try:
            metadata, keymap = normalize_kle_data(load_json(path))
            self.input_path = path
            self.metadata = metadata
            self.keymap = keymap
            self.layout_options = detect_layout_options(keymap)
            self.preview_keys = parse_preview_keys(keymap)
            self.preview_choice_offsets = self.build_preview_choice_offsets()
            self.selected_choices = {
                option: min(choices)
                for option, choices in self.layout_options["choices"].items()
                if choices
            }
            default_name = metadata.get("name") or os.path.splitext(os.path.basename(path))[0]
            self.keyboard_name_var.set(str(default_name).replace("-", "_").replace(" ", "_"))
            self._build_choice_selectors()
            self.draw_preview()
            rows, cols = self.current_matrix_size()
            self.status_var.set(
                f"Loaded: rows={rows} cols={cols} options={len(self.layout_options['choices'])} keys={len(self.preview_keys)}"
            )
        except Exception as exc:
            messagebox.showerror("Open error", str(exc))

    def _build_choice_selectors(self):
        for child in self.option_frame.winfo_children():
            child.destroy()
        self.choice_vars = {}

        choices_by_option = self.layout_options["choices"]
        if not choices_by_option:
            ttk.Label(self.option_frame, text="No multi-layout options found.").pack(anchor=tk.W)
            return

        for option, choices in choices_by_option.items():
            option_name = self.layout_options["option_names"].get(option, f"Option {option}")
            frame = ttk.LabelFrame(self.option_frame, text=f"{option}: {option_name}", padding=8)
            frame.pack(fill=tk.X, pady=(0, 8))

            display_items = [self.choice_display_name(option, choice) for choice in choices]
            var = tk.StringVar(value=display_items[0])
            self.choice_vars[option] = var
            combo = ttk.Combobox(frame, textvariable=var, values=display_items, state="readonly")
            combo.pack(fill=tk.X)
            combo.bind("<<ComboboxSelected>>", lambda _event, opt=option: self.on_choice_selected(opt))

    def choice_display_name(self, option, choice):
        choices = self.layout_options["choices"].get(option, [])
        if len(choices) == 2 and choice in (0, 1):
            return "オフ" if choice == 0 else "オン"
        choice_names = self.layout_options["choice_names"]
        if (option, choice) in choice_names:
            return choice_names[(option, choice)]
        return f"choice {choice}"

    def on_choice_selected(self, option):
        value = self.choice_vars[option].get()
        for choice in self.layout_options["choices"].get(option, []):
            if self.choice_display_name(option, choice) == value:
                self.selected_choices[option] = choice
                break
        self.draw_preview()

    def current_matrix_size(self):
        entries = parse_keymap(self.keymap or [], include_decals=False)
        return matrix_size_from_kle(self.keymap or [], entries)

    def is_key_visible(self, key):
        option = key["option"]
        if option is None:
            return True
        return self.selected_choices.get(option) == key["choice"]

    def qmk_layout_keys(self):
        if self.keymap is None:
            raise ValueError("No KLE JSON loaded.")

        used_matrices = set()
        keys = []
        for key in self.preview_keys:
            matrix = key["matrix"]
            if matrix in used_matrices:
                continue
            used_matrices.add(matrix)
            keys.append(key)
        return keys

    def choice_bounds(self, option, choice):
        keys = [
            key for key in self.preview_keys
            if key["option"] == option and key["choice"] == choice
        ]
        if not keys:
            return None
        return {
            "left": min(key["x"] for key in keys),
            "top": min(key["y"] for key in keys),
            "right": max(key["x"] + (1.5 if key["iso"] else key["w"]) for key in keys),
            "bottom": max(key["y"] + (2 if key["iso"] else key["h"]) for key in keys),
        }

    def build_preview_choice_offsets(self):
        offsets = {}
        for option, choices in self.layout_options["choices"].items():
            if not choices:
                continue
            base_choice = min(choices)
            base_bounds = self.choice_bounds(option, base_choice)
            if base_bounds is None:
                continue
            for choice in choices:
                bounds = self.choice_bounds(option, choice)
                if bounds is None:
                    continue
                offsets[(option, choice)] = {
                    "x": base_bounds["left"] - bounds["left"],
                    "y": base_bounds["top"] - bounds["top"],
                }
        return offsets

    def preview_geometry_for_key(self, key):
        offset = self.preview_choice_offsets.get((key["option"], key["choice"]), {"x": 0, "y": 0})
        return {
            "x": key["x"] + offset["x"],
            "y": key["y"] + offset["y"],
            "w": key["w"],
            "h": key["h"],
            "iso": key["iso"],
        }

    def draw_preview(self):
        self.canvas.delete("all")
        if not self.preview_keys:
            self.canvas.create_text(CANVAS_MARGIN, CANVAS_MARGIN, text="No layout", anchor=tk.NW, fill="#54615e")
            return

        max_x = 0
        max_y = 0
        for key in self.preview_keys:
            if not self.is_key_visible(key):
                continue

            geometry = self.preview_geometry_for_key(key)
            draw_x_u = geometry["x"] + (ISO_LEFT_OFFSET if geometry["iso"] else 0)
            x = CANVAS_MARGIN + draw_x_u * CANVAS_UNIT
            y = CANVAS_MARGIN + geometry["y"] * CANVAS_UNIT
            width = (1.5 if geometry["iso"] else geometry["w"]) * CANVAS_UNIT
            height = (2 if geometry["iso"] else geometry["h"]) * CANVAS_UNIT
            fill = OPTION_COLORS.get(key["option"], OPTION_COLORS[None])
            outline = "#93aaa5"

            if geometry["iso"]:
                self._draw_iso_key(x, y, fill, outline)
            else:
                self.canvas.create_rectangle(x, y, x + width - KEY_GAP, y + height - KEY_GAP, fill=fill, outline=outline, width=2)

            label = f"{key['matrix'][0]},{key['matrix'][1]}"
            if key["option"] is not None:
                label += f"\no{key['option']} c{key['choice']}"
            self.canvas.create_text(x + 7, y + 7, text=label, anchor=tk.NW, fill="#1e3733", font=("Consolas", 9, "bold"))
            max_x = max(max_x, x + width)
            max_y = max(max_y, y + height)

        self.canvas.configure(scrollregion=(0, 0, max_x + CANVAS_MARGIN, max_y + CANVAS_MARGIN))

    def _draw_iso_key(self, x, y, fill, outline):
        unit = CANVAS_UNIT
        gap = KEY_GAP
        points = [
            x, y,
            x + 1.5 * unit - gap, y,
            x + 1.5 * unit - gap, y + 2 * unit - gap,
            x + 0.25 * unit, y + 2 * unit - gap,
            x + 0.25 * unit, y + unit - gap,
            x, y + unit - gap,
        ]
        self.canvas.create_polygon(points, fill=fill, outline=outline, width=2)

    def build_vial_json(self):
        if self.keymap is None:
            raise ValueError("No KLE JSON loaded.")
        entries = parse_keymap(self.keymap, include_decals=True)
        rows, cols = matrix_size_from_kle(self.keymap, entries)
        custom_keycodes = load_custom_keycodes()
        return {
            "customKeycodes": custom_keycodes,
            "name": self.keyboard_name_var.get().strip() or "magnetic_keyboard",
            "matrix": {
                "rows": rows,
                "cols": cols,
            },
            "layouts": {
                "labels": option_labels_from_detected(self.layout_options),
                "keymap": keymap_for_vial_json(self.keymap),
            },
        }

    def build_key_layout_c(self):
        if self.keymap is None:
            raise ValueError("No KLE JSON loaded.")
        if self.layout_options["choices"] and max(self.layout_options["choices"]) >= 8:
            raise ValueError("KLE option numbers must be 0..7 for LAYOUT_OPTION_MAX=8.")
        entries = parse_keymap(self.keymap, include_decals=False)
        rows, cols = matrix_size_from_kle(self.keymap, entries)
        assign_indices(entries, rows, cols)
        option_count = min(len(option_labels_from_detected(self.layout_options)), 8)
        return format_c(entries, option_count, self.layout_options)

    def build_qmk_layout_fragment(self):
        return format_qmk_layout_fragment(self.qmk_layout_keys())

    def build_keymap_c(self):
        return format_keymap_c(self.qmk_layout_keys())

    def save_vial_json(self):
        if self.keymap is None:
            messagebox.showwarning("Not loaded", "Open a KLE JSON file first.")
            return False
        path = filedialog.asksaveasfilename(
            title="Save vial.json",
            defaultextension=".json",
            initialdir=os.path.dirname(self.input_path),
            initialfile="vial.json",
            filetypes=[("JSON", "*.json"), ("All files", "*.*")],
        )
        if not path:
            return False
        try:
            write_text(path, json.dumps(self.build_vial_json(), ensure_ascii=False, indent=2) + "\n")
            self.status_var.set(f"Saved: {path}")
            return True
        except Exception as exc:
            messagebox.showerror("Save error", str(exc))
            return False

    def save_key_layout(self):
        if self.keymap is None:
            messagebox.showwarning("Not loaded", "Open a KLE JSON file first.")
            return False
        path = filedialog.asksaveasfilename(
            title="Save key_layout.c",
            defaultextension=".c",
            initialdir=os.path.dirname(self.input_path),
            initialfile="key_layout.c",
            filetypes=[("C source", "*.c"), ("All files", "*.*")],
        )
        if not path:
            return False
        try:
            write_text(path, self.build_key_layout_c())
            self.status_var.set(f"Saved: {path}")
            return True
        except Exception as exc:
            messagebox.showerror("Save error", str(exc))
            return False

    def save_qmk_layout(self):
        if self.keymap is None:
            messagebox.showwarning("Not loaded", "Open a KLE JSON file first.")
            return False
        path = filedialog.asksaveasfilename(
            title="Save keyboard layout fragment",
            defaultextension=".json",
            initialdir=os.path.dirname(self.input_path),
            initialfile="keyboard_layout.json",
            filetypes=[("JSON", "*.json"), ("All files", "*.*")],
        )
        if not path:
            return False
        try:
            write_text(path, self.build_qmk_layout_fragment())
            self.status_var.set(f"Saved: {path}")
            return True
        except Exception as exc:
            messagebox.showerror("Save error", str(exc))
            return False

    def save_keymap_c(self):
        if self.keymap is None:
            messagebox.showwarning("Not loaded", "Open a KLE JSON file first.")
            return False
        path = filedialog.asksaveasfilename(
            title="Save keymap.c",
            defaultextension=".c",
            initialdir=os.path.dirname(self.input_path),
            initialfile="keymap.c",
            filetypes=[("C source", "*.c"), ("All files", "*.*")],
        )
        if not path:
            return False
        try:
            write_text(path, self.build_keymap_c())
            self.status_var.set(f"Saved: {path}")
            return True
        except Exception as exc:
            messagebox.showerror("Save error", str(exc))
            return False

    def save_both(self):
        if self.keymap is None:
            messagebox.showwarning("Not loaded", "Open a KLE JSON file first.")
            return
        directory = filedialog.askdirectory(
            title="Select output folder",
            initialdir=os.path.dirname(self.input_path),
        )
        if not directory:
            return
        try:
            vial_path = os.path.join(directory, "vial.json")
            key_layout_path = os.path.join(directory, "key_layout.c")
            qmk_layout_path = os.path.join(directory, "keyboard_layout.json")
            keymap_path = os.path.join(directory, "keymap.c")
            write_text(vial_path, json.dumps(self.build_vial_json(), ensure_ascii=False, indent=2) + "\n")
            write_text(key_layout_path, self.build_key_layout_c())
            write_text(qmk_layout_path, self.build_qmk_layout_fragment())
            write_text(keymap_path, self.build_keymap_c())
            self.status_var.set(f"Saved all files to: {directory}")
            messagebox.showinfo("Saved", f"Saved vial.json, key_layout.c, keyboard_layout.json, and keymap.c.\n{directory}")
        except Exception as exc:
            messagebox.showerror("Save error", str(exc))


if __name__ == "__main__":
    app = KleMultiLayoutTool()
    app.mainloop()
