#!/usr/bin/env python3
"""Run a reproducible thesis-level correctness validation campaign."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import platform
import shlex
import subprocess
import sys
from dataclasses import dataclass, asdict
from pathlib import Path


@dataclass
class CommandResult:
    name: str
    command: list[str]
    command_shell: str
    started_utc: str
    elapsed_seconds: float
    exit_code: int
    log_file: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, default=Path("build"))
    parser.add_argument("--output-dir", type=Path, default=Path("results/validation"))
    parser.add_argument("--profile", choices=("quick", "full", "thesis"), default="quick")
    parser.add_argument("--seeds", default="19676,27182,31415,424242")
    parser.add_argument("--random-count", type=int)
    parser.add_argument("--random-max-n", type=int)
    return parser.parse_args()


def executable(build_dir: Path, name: str) -> Path:
    candidates = [
        build_dir / name,
        build_dir / f"{name}.exe",
        build_dir / "Release" / f"{name}.exe",
        build_dir / "Debug" / f"{name}.exe",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise FileNotFoundError(f"Cannot find executable {name} below {build_dir}")


def run_logged(name: str, command: list[str], log_dir: Path) -> CommandResult:
    log_file = log_dir / f"{name}.log"
    started = dt.datetime.now(dt.timezone.utc)
    completed = subprocess.run(command, text=True, capture_output=True, check=False)
    finished = dt.datetime.now(dt.timezone.utc)
    log_file.write_text(
        "$ " + " ".join(shlex.quote(part) for part in command) + "\n\n"
        + "--- stdout ---\n" + completed.stdout
        + "\n--- stderr ---\n" + completed.stderr,
        encoding="utf-8",
    )
    return CommandResult(
        name=name,
        command=command,
        command_shell=" ".join(shlex.quote(part) for part in command),
        started_utc=started.isoformat(),
        elapsed_seconds=(finished - started).total_seconds(),
        exit_code=completed.returncode,
        log_file=str(log_file),
    )


def profile_values(args: argparse.Namespace) -> tuple[list[int], int, int, str]:
    seeds = [int(value) for value in args.seeds.split(",") if value]
    if args.profile == "quick":
        return seeds[:1], args.random_count or 500, args.random_max_n or 30, "quick"
    if args.profile == "full":
        return seeds[:1], args.random_count or 5000, args.random_max_n or 30, "full"
    return seeds, args.random_count or 10000, args.random_max_n or 100, "full"


def git_value(*arguments: str) -> str | None:
    completed = subprocess.run(["git", *arguments], text=True, capture_output=True, check=False)
    return completed.stdout.strip() if completed.returncode == 0 else None


def main() -> int:
    args = parse_args()
    seeds, random_count, random_max_n, script_profile = profile_values(args)
    timestamp = dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    run_dir = args.output_dir / f"{timestamp}-{args.profile}"
    log_dir = run_dir / "logs"
    log_dir.mkdir(parents=True, exist_ok=False)

    decision_cli = executable(args.build_dir, "bm_planarity_decision_cli")
    embedding_cli = executable(args.build_dir, "bm_planarity_embedding_cli")
    verifier_cli = executable(args.build_dir, "bm_kuratowski_verifier_cli")
    classifier_cli = executable(args.build_dir, "bm_kuratowski_classifier_cli")
    certificate_cli = executable(args.build_dir, "bm_kuratowski_certificate_cli")

    python = sys.executable
    tools = Path(__file__).parent
    commands: list[tuple[str, list[str]]] = []

    commands.append(("ctest", ["ctest", "--test-dir", str(args.build_dir), "--output-on-failure"]))

    for seed in seeds:
        common = ["--profile", script_profile, "--random-count", str(random_count), "--random-max-n", str(random_max_n), "--seed", str(seed)]
        commands.append((f"decision-seed-{seed}", [python, str(tools / "differential_regression.py"), "--cli", str(decision_cli), *common]))
        commands.append((f"embedding-seed-{seed}", [python, str(tools / "embedding_regression.py"), "--cli", str(embedding_cli), *common]))
        commands.append((f"extractor-seed-{seed}", [python, str(tools / "kuratowski_extractor_regression.py"), "--cli", str(certificate_cli), *common]))
        commands.append((f"verifier-seed-{seed}", [python, str(tools / "kuratowski_verifier_regression.py"), "--cli", str(verifier_cli), "--profile", script_profile, "--random-count", str(max(100, random_count // 10)), "--random-max-n", str(max(30, random_max_n)), "--seed", str(seed)]))

    commands.append(("classifier", [python, str(tools / "kuratowski_classifier_regression.py"), "--cli", str(classifier_cli), "--profile", script_profile]))

    results: list[CommandResult] = []
    failed = False
    for name, command in commands:
        print(f"Running {name}...", flush=True)
        result = run_logged(name, command, log_dir)
        results.append(result)
        if result.exit_code != 0:
            failed = True
            print(f"FAILED: {name}. See {result.log_file}", file=sys.stderr)
            break

    metadata = {
        "timestamp_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "profile": args.profile,
        "seeds": seeds,
        "random_count": random_count,
        "random_max_n": random_max_n,
        "git_commit": git_value("rev-parse", "HEAD"),
        "git_status_porcelain": git_value("status", "--porcelain"),
        "platform": platform.platform(),
        "python_version": platform.python_version(),
        "results": [asdict(result) for result in results],
        "passed": not failed,
    }
    (run_dir / "validation_report.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")

    markdown = [
        "# Validation campaign report",
        "",
        f"- Profile: `{args.profile}`",
        f"- Git commit: `{metadata['git_commit']}`",
        f"- Passed: `{metadata['passed']}`",
        f"- Seeds: `{', '.join(str(seed) for seed in seeds)}`",
        f"- Random cases per regression script and seed: `{random_count}`",
        f"- Maximum random vertex count: `{random_max_n}`",
        "",
        "| Command | Exit code | Time (s) | Log |",
        "|---|---:|---:|---|",
    ]
    for result in results:
        markdown.append(f"| `{result.name}` | {result.exit_code} | {result.elapsed_seconds:.2f} | `{result.log_file}` |")
    (run_dir / "README.md").write_text("\n".join(markdown) + "\n", encoding="utf-8")

    print(f"Validation campaign saved to: {run_dir}")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
