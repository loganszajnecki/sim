#!/usr/bin/env python3
"""
plot_telemetry.py

Simple post-processing script for the missile simulation.
Reads telemetry.csv and plots missile + target 3D trajectories,
and their distance vs time.

Usage:
    python3 plot_telemetry.py
"""

import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D  # noqa: F401
import numpy as np

# -------------------------------------------------------------------
# Load telemetry data
# -------------------------------------------------------------------
csv_file = "../build/telemetry.csv"

try:
    df = pd.read_csv(csv_file)
except FileNotFoundError:
    print(f"Error: '{csv_file}' not found.")
    exit(1)

required = ["t", "px", "py", "pz", "tx", "ty", "tz"]
for col in required:
    if col not in df.columns:
        print(f"Error: '{col}' column missing from telemetry.csv.")
        exit(1)

t = df["t"].to_numpy()
missile = df[["px", "py", "pz"]].to_numpy()
target  = df[["tx", "ty", "tz"]].to_numpy()

# -------------------------------------------------------------------
# Compute distance between missile and target
# -------------------------------------------------------------------
distance = np.linalg.norm(missile - target, axis=1)
min_idx = np.argmin(distance)
min_dist = distance[min_idx]
t_min = t[min_idx]

# -------------------------------------------------------------------
# Plot trajectories
# -------------------------------------------------------------------
fig = plt.figure(figsize=(10, 5))

# 3D Trajectory plot
ax1 = fig.add_subplot(1, 2, 1, projection="3d")
ax1.plot(target[:,0], target[:,1], target[:,2], "r--", label="Target")
ax1.plot(missile[:,0], missile[:,1], missile[:,2], "b-", label="Missile")

ax1.scatter(target[0,0], target[0,1], target[0,2], color="r", marker="o", label="Target start")
ax1.scatter(missile[0,0], missile[0,1], missile[0,2], color="b", marker="o", label="Missile start")

ax1.set_xlabel("X [m]")
ax1.set_ylabel("Y [m]")
ax1.set_zlabel("Z [m]")
ax1.set_title("3D Trajectories")
ax1.legend()
ax1.grid(True)

# Distance vs Time plot
ax2 = fig.add_subplot(1, 2, 2)
ax2.plot(t, distance, label="Missile-Target Distance [m]")
ax2.axvline(t_min, color="red", linestyle="--", label=f"Min dist = {min_dist:.2f} m @ {t_min:.2f} s")
ax2.set_xlabel("Time [s]")
ax2.set_ylabel("Distance [m]")
ax2.set_title("Separation vs Time")
ax2.legend()
ax2.grid(True)

plt.tight_layout()
plt.show()
