"""Python wrapper around src/actukit-mack.

Usage:
    python py/mack.py examples/mack_triangle.csv
"""
from pathlib import Path
import sqlite3
import subprocess
import sys

import pandas as pd

ROOT = Path(__file__).resolve().parent.parent
BIN  = ROOT / "src" / "actukit-mack"
DB   = ROOT / "actukit.db"


def load_triangle(csv_path: str) -> pd.DataFrame:
    """Read CSV, load into SQLite using the managed schema."""
    df = (pd.read_csv(csv_path).rename(columns=str.lower)
            .rename(columns={"origin": "origin_year", "dev": "dev_year"}))
    with sqlite3.connect(DB) as conn:
        conn.executescript((ROOT / "sql" / "schema.sql").read_text())
        conn.execute("DELETE FROM claims_triangle")
        df.to_sql("claims_triangle", conn, if_exists="append", index=False)
    return df


def run(csv_path: str) -> str:
    """Run the Mack binary and return its stdout."""
    return subprocess.check_output([str(BIN), csv_path], text=True)


def plot_factors(csv_path: str, out_png: str = "dev_factors.png") -> str:
    """Plot f_j — eyeball whether Mack's proportionality assumption holds."""
    import matplotlib.pyplot as plt

    out = run(csv_path)
    js, fs = [], []
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("f[") and "=" in line:
            left, right = line.split("=", 1)
            j = int(left.strip()[2:left.index("]")])
            f = float(right.strip().split()[0])
            js.append(j)
            fs.append(f)

    plt.figure(figsize=(7, 3.5))
    plt.plot(js, fs, marker="o", color="#0b7285")
    plt.xlabel("development period j")
    plt.ylabel("f_j")
    plt.title("Chain-ladder development factors")
    plt.grid(alpha=0.3)
    plt.tight_layout()
    plt.savefig(out_png, dpi=120)
    return out_png


def main() -> None:
    csv_path = sys.argv[1] if len(sys.argv) > 1 else "examples/mack_triangle.csv"
    print(f"→ loading {csv_path} into {DB.name}")
    load_triangle(csv_path)
    print(f"→ running {BIN.name}")
    print(run(csv_path))
    png = plot_factors(csv_path)
    print(f"→ saved {png}")


if __name__ == "__main__":
    main()
