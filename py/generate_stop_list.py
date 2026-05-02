#!/usr/bin/env python3
"""Generate bkk_stop_list.h from data/stops.txt"""

import csv
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
STOPS_FILE = os.path.join(SCRIPT_DIR, "../data/stops.txt")
OUTPUT_FILE = os.path.join(SCRIPT_DIR, "../cpp/bkk_uds/bkk_stop_list.h")

STOP_ID_MAX_LEN = 16
STOP_NAME_MAX_LEN = 96

def escape_c_string(s):
    return s.replace("\\", "\\\\").replace('"', '\\"')

def main():
    stops = []
    with open(STOPS_FILE, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            stops.append({
                "stop_id":   row["stop_id"].strip(),
                "stop_name": row["stop_name"].strip(),
                "stop_lat":  row["stop_lat"].strip(),
                "stop_lon":  row["stop_lon"].strip(),
            })

    lines = []
    lines.append("#ifndef BKK_STOP_LIST_H")
    lines.append("#define BKK_STOP_LIST_H")
    lines.append("")
    lines.append("#ifdef __cplusplus")
    lines.append('extern "C" {')
    lines.append("#endif")
    lines.append("")
    lines.append(f"#define STOP_ID_MAX_LEN   {STOP_ID_MAX_LEN}")
    lines.append(f"#define STOP_NAME_MAX_LEN {STOP_NAME_MAX_LEN}")
    lines.append("")
    lines.append("typedef struct {")
    lines.append(f"  char stop_id[STOP_ID_MAX_LEN];")
    lines.append(f"  char stop_name[STOP_NAME_MAX_LEN];")
    lines.append("  double stop_lat;")
    lines.append("  double stop_lon;")
    lines.append("} bkk_stop_t;")
    lines.append("")
    lines.append(f"#define BKK_STOP_COUNT {len(stops)}")
    lines.append("")
    lines.append("static const bkk_stop_t bkk_stop_list[BKK_STOP_COUNT] = {")

    for stop in stops:
        sid  = escape_c_string(stop["stop_id"])[:STOP_ID_MAX_LEN - 1]
        name = escape_c_string(stop["stop_name"])[:STOP_NAME_MAX_LEN - 1]
        lat  = stop["stop_lat"] if stop["stop_lat"] else "0.0"
        lon  = stop["stop_lon"] if stop["stop_lon"] else "0.0"
        lines.append(f'  {{ "{sid}", "{name}", {lat}, {lon} }},')

    lines.append("};")
    lines.append("")
    lines.append("#ifdef __cplusplus")
    lines.append("} // extern \"C\"")
    lines.append("#endif")
    lines.append("")
    lines.append("#endif // BKK_STOP_LIST_H")
    lines.append("")

    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

    print(f"Generated {OUTPUT_FILE} with {len(stops)} stops.")

if __name__ == "__main__":
    main()
