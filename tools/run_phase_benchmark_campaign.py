#!/usr/bin/env python3
"""Run targeted phase-level BM diagnostics and preserve reproducibility metadata."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import platform
import shlex
import subprocess
import sys
from pathlib import Path
from typing import Any


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", required=True, type=Path)
    parser.add_argument("--profile", choices=("quick", "full"), default="quick")
    parser.add_argument("--output-dir", type=Path, default=Path("results/phase-benchmarks"))
    parser.add_argument("--seed", type=int, default=19676)
    parser.add_argument("--repetitions", type=int)
    parser.add_argument("--warmups", type=int)
    parser.add_argument("--families")
    parser.add_argument("--sizes")
    parser.add_argument("--plot", action="store_true")
    return parser.parse_args()


def git_value(*arguments: str) -> str | None:
    completed = subprocess.run(
        ["git", *arguments],
        text=True,
        capture_output=True,
        check=False,
    )
    return completed.stdout.strip() if completed.returncode == 0 else None


def system_metadata(command: list[str]) -> dict[str, Any]:
    return {
        "timestamp_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "command": command,
        "command_shell": " ".join(shlex.quote(part) for part in command),
        "cwd": str(Path.cwd()),
        "git_commit": git_value("rev-parse", "HEAD"),
        "git_branch": git_value("rev-parse", "--abbrev-ref", "HEAD"),
        "git_status_porcelain": git_value("status", "--porcelain"),
        "platform": platform.platform(),
        "system": platform.system(),
        "release": platform.release(),
        "machine": platform.machine(),
        "processor": platform.processor(),
        "python_version": platform.python_version(),
        "python_executable": sys.executable,
        "environment": {
            "NUMBER_OF_PROCESSORS": os.environ.get("NUMBER_OF_PROCESSORS"),
            "PROCESSOR_IDENTIFIER": os.environ.get("PROCESSOR_IDENTIFIER"),
        },
    }


def main() -> int:
    args = parse_args()
    if not args.cli.exists():
        raise SystemExit(f"Phase benchmark executable does not exist: {args.cli}")

    timestamp = dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    run_dir = args.output_dir / f"{timestamp}-{args.profile}"
    run_dir.mkdir(parents=True, exist_ok=False)

    raw_csv = run_dir / "raw_phase_results.csv"
    command = [
        str(args.cli),
        "--profile",
        args.profile,
        "--output",
        str(raw_csv),
        "--seed",
        str(args.seed),
    ]

    optional_arguments = (
        ("--repetitions", args.repetitions),
        ("--warmups", args.warmups),
        ("--families", args.families),
        ("--sizes", args.sizes),
    )
    for option, value in optional_arguments:
        if value is not None:
            command.extend([option, str(value)])

    metadata = system_metadata(command)
    metadata["profile"] = args.profile
    metadata["seed"] = args.seed

    print("Running:", metadata["command_shell"], flush=True)
    started = dt.datetime.now(dt.timezone.utc)
    completed = subprocess.run(command, text=True, check=False)
    finished = dt.datetime.now(dt.timezone.utc)

    metadata["elapsed_seconds"] = (finished - started).total_seconds()
    metadata["exit_code"] = completed.returncode
    (run_dir / "metadata.json").write_text(
        json.dumps(metadata, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )

    if completed.returncode != 0:
        raise SystemExit(completed.returncode)

    if args.plot:
        plot_script = Path(__file__).with_name("plot_phase_benchmarks.py")
        subprocess.run(
            [
                sys.executable,
                str(plot_script),
                "--input",
                str(raw_csv),
                "--output-dir",
                str(run_dir / "figures"),
            ],
            check=True,
        )

    print(f"Phase benchmark campaign saved to: {run_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
