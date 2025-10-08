#!/usr/bin/env python3
"""
plot_via_spectrum.py
--------------------------------------------------
Reads the VIA spectrometer's generated CSV file
(`/spectrum.csv`) and plots Pixel vs Intensity.

Usage:
    python3 plot_via_spectrum.py               # Uses default /spectrum.csv
    python3 plot_via_spectrum.py path/to/file  # Specify custom path
"""

import sys
import csv
import matplotlib.pyplot as plt
from pathlib import Path

# -----------------------------------------------------------------------------
# CONFIGURATION
# -----------------------------------------------------------------------------
DEFAULT_PATH = Path("spectrum.csv")

# -----------------------------------------------------------------------------
def load_csv(file_path: Path):
    """Load CSV data (Pixel,Intensity) from file."""
    pixels, intensity = [], []

    with open(file_path, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                pixels.append(int(row["Pixel"]))
                intensity.append(float(row["Intensity"]))
            except (ValueError, KeyError):
                continue  # skip malformed lines

    if not pixels:
        raise ValueError("No valid data found in CSV file.")

    return pixels, intensity


def plot_spectrum(pixels, intensity, title="VIA Spectrometer Output"):
    """Plot intensity vs. pixel index."""
    plt.figure(figsize=(10, 5))
    plt.plot(pixels, intensity, color="royalblue", linewidth=1.2)
    plt.title(title)
    plt.xlabel("Pixel Index")
    plt.ylabel("Intensity (a.u.)")
    plt.grid(True, linestyle="--", alpha=0.6)
    plt.tight_layout()
    plt.show()


# -----------------------------------------------------------------------------
if __name__ == "__main__":
    # Determine which file to read
    if len(sys.argv) > 1:
        path = Path(sys.argv[1])
    else:
        path = DEFAULT_PATH

    if not path.exists():
        print(f"❌ File not found: {path.resolve()}")
        sys.exit(1)

    print(f"📄 Loading: {path.resolve()}")

    try:
        pixels, intensity = load_csv(path)
        print(f"✅ Loaded {len(pixels)} data points.")
        plot_spectrum(pixels, intensity)
    except Exception as e:
        print(f"⚠️ Error reading {path.name}: {e}")