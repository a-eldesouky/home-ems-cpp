#!/usr/bin/env python3
"""Writes a synthetic one-day PV and load profile as CSV.

The numbers are made up: a smooth PV curve that peaks at midday and a
household load with a morning and an evening peak, plus small seeded noise.
The same seed always gives the same file.

Usage: generate_profile.py [--seed N] [--step-minutes M] [OUTPUT]
"""

import argparse
import math
import random
import sys

PV_PEAK_W = 6500.0  # Above the default 4000 W inverter limit on purpose.
SUNRISE_H = 6.0
SUNSET_H = 20.0
BASE_LOAD_W = 350.0


def pv_power(hour: float) -> float:
    if hour <= SUNRISE_H or hour >= SUNSET_H:
        return 0.0
    phase = (hour - SUNRISE_H) / (SUNSET_H - SUNRISE_H)
    return PV_PEAK_W * math.sin(math.pi * phase) ** 2


def load_power(hour: float) -> float:
    morning = 1800.0 * math.exp(-((hour - 7.5) ** 2) / 0.8)
    evening = 5200.0 * math.exp(-((hour - 19.5) ** 2) / 1.2)
    return BASE_LOAD_W + morning + evening


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("output", nargs="?", default="-")
    parser.add_argument("--seed", type=int, default=7)
    parser.add_argument("--step-minutes", type=int, default=60)
    args = parser.parse_args()
    if args.step_minutes <= 0 or (24 * 60) % args.step_minutes != 0:
        parser.error("--step-minutes must divide 1440")

    rng = random.Random(args.seed)
    rows = ["time,pv_w,load_w"]
    for minute in range(0, 24 * 60, args.step_minutes):
        hour = minute / 60.0
        pv = pv_power(hour)
        if pv > 0.0:
            pv = max(0.0, pv * rng.uniform(0.9, 1.0))
        load = load_power(hour) * rng.uniform(0.95, 1.05)
        rows.append(f"{minute // 60:02d}:{minute % 60:02d},{pv:.0f},{load:.0f}")

    text = "\n".join(rows) + "\n"
    if args.output == "-":
        sys.stdout.write(text)
    else:
        with open(args.output, "w", encoding="ascii", newline="\n") as f:
            f.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
