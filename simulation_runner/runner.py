from __future__ import annotations

import matplotlib.dates as mdates
import subprocess
import sys
import tempfile
from pathlib import Path
import shutil
from typing import Iterable
from joblib import Parallel, delayed

import matplotlib.pyplot as plt
import pandas as pd

SIMULATION_STEPS = 100000

COMMON_CONFIG = {
    "use_journal": "false",
    "krad_per_year": "5.0",
    "bit_flip_seed": "67",
    "num_users": "10",
    "max_write_size": "65536",
    "max_read_size": "65536",
    "avg_steps_between_ops": "90",
    "create_weight": "3",
    "write_weight": "10",
    "read_weight": "9",
    "delete_weight": "2",
    "simulation_seconds": str(SIMULATION_STEPS),
    "seconds_per_step": "1"
}

CONFIGS = [
    {
        "name": "Crc256",
        "block_size": "256",
        "ecc_type": "Crc",
        "rs_correctable_bytes": "2",
    } | COMMON_CONFIG,
    {
        "name": "None256",
        "block_size": "256",
        "ecc_type": "None",
        "rs_correctable_bytes": "2",
    } | COMMON_CONFIG,
    {
        "name": "Hamming256",
        "block_size": "256",
        "ecc_type": "Hamming",
        "rs_correctable_bytes": "2",
    } | COMMON_CONFIG,
    {
        "name": "Crc1024",
        "block_size": "1024",
        "ecc_type": "Crc",
        "rs_correctable_bytes": "2",
    } | COMMON_CONFIG,
    {
        "name": "None1024",
        "block_size": "1024",
        "ecc_type": "None",
        "rs_correctable_bytes": "2",
    } | COMMON_CONFIG,
    {
        "name": "Hamming1024",
        "block_size": "1024",
        "ecc_type": "Hamming",
        "rs_correctable_bytes": "2",
    } | COMMON_CONFIG,
    {
        "name": "ReedSolomon256_2correctable",
        "block_size": "256",
        "ecc_type": "ReedSolomon",
        "rs_correctable_bytes": "2",
    } | COMMON_CONFIG,
    {
        "name": "ReedSolomon256_1correctable",
        "block_size": "256",
        "ecc_type": "ReedSolomon",
        "rs_correctable_bytes": "1",
    } | COMMON_CONFIG,
    {
        "name": "ReedSolomon256_3correctable",
        "block_size": "256",
        "ecc_type": "ReedSolomon",
        "rs_correctable_bytes": "3",
    } | COMMON_CONFIG
]


def candidate_binaries() -> Iterable[Path]:
    """Generate candidate paths for the usage_simulator binary."""
    for preset in ["release", "debug"]:
        yield Path("build") / preset / "usage_simulator" / "usage_simulator"
        yield Path("build") / preset / "bin" / "usage_simulator"
    yield Path("cmake-build-debug") / "usage_simulator" / "usage_simulator"
    yield Path("cmake-build-release") / "usage_simulator" / "usage_simulator"


def find_binary() -> Path:
    """Find the usage_simulator binary in common build locations."""
    for candidate in candidate_binaries():
        if candidate.is_file():
            print(f"Found binary: {candidate}")
            return candidate
    raise FileNotFoundError(
        "usage_simulator binary not found. Build the project first."
    )


def create_config_file(config: dict[str, str], temp_dir: Path) -> Path:
    """Create a temporary config file from the config dictionary."""
    config_path = temp_dir / "config.txt"
    with open(config_path, "w") as f:
        for key, value in config.items():
            if key != "name":
                f.write(f"{key}={value}\n")
    return config_path


def run_simulator(binary: Path, config_path: Path, logs_dir: Path) -> None:
    """Run the simulator with the given config and log directory."""
    cmd = [str(binary), str(config_path), str(logs_dir)]
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, check=False, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Simulator stderr: {result.stderr}", file=sys.stderr)


def _read_csv(path: Path) -> pd.DataFrame:
    """Read a CSV file or return empty DataFrame if it doesn't exist."""
    if not path.exists():
        return pd.DataFrame()
    # Strict parse: rely on _validate_read_write_csv to catch malformed rows beforehand.
    return pd.read_csv(path)


def load_logs(logs_dir: Path) -> dict[str, pd.DataFrame]:
    """Load all log CSVs from the logs directory."""
    # Validate structure of read/write CSVs before pandas parsing to catch real data issues.
    _validate_read_write_csv(logs_dir)
    return {
        "read": _read_csv(logs_dir / "read.csv"),
        "write": _read_csv(logs_dir / "write.csv"),
        "flip": _read_csv(logs_dir / "flip.csv"),
        "correction": _read_csv(logs_dir / "correction.csv"),
        "detection": _read_csv(logs_dir / "detection.csv"),
        "error": _read_csv(logs_dir / "error.csv"),
    }


def _validate_read_write_csv(logs_dir: Path) -> None:
    """Detect malformed lines in read.csv and write.csv (should have 4 fields).

    Raises a ValueError with details on the first few offending lines so the root cause is visible.
    """

    def _find_bad_lines(path: Path, expected_fields: int = 4, max_report: int = 5):
        bad = []
        if not path.exists():
            return bad
        with path.open("r", encoding="utf-8", errors="replace") as f:
            for i, line in enumerate(f, start=1):
                if i == 1:
                    # header line: trust it defines expected fields
                    continue
                # Fast check: split on ',' (no quoting is expected in these files)
                fields = line.rstrip("\n\r").split(",")
                if len(fields) != expected_fields:
                    bad.append((i, len(fields), line.rstrip("\n\r")))
                    if len(bad) >= max_report:
                        break
        return bad

    issues = {}
    for name in ("read.csv", "write.csv"):
        path = logs_dir / name
        bad = _find_bad_lines(path)
        if bad:
            issues[name] = bad

    if issues:
        details = []
        for name, bad in issues.items():
            details.append(f"File: {name}")
            for line_no, seen, content in bad:
                details.append(f"  Line {line_no}: expected 4 fields, saw {seen} -> {content}")
        raise ValueError(
            "Malformed CSV rows detected before parsing.\n" + "\n".join(details)
        )


def plot_read_status_trend(read_df: pd.DataFrame, error_df: pd.DataFrame,
                           detection_df: pd.DataFrame, correction_df: pd.DataFrame,
                           config_name: str, out: Path) -> None:
    # steps with read operation
    read_steps = read_df['step']
    # result of corresponding read operation
    results = read_df['result']

    steps = range(SIMULATION_STEPS + 1)

    # number of operations indexed by step number
    successes = pd.Series([0 for _ in steps])
    explicit_errors = pd.Series([0 for _ in steps])
    false_successes = pd.Series([0 for _ in steps])
    corrections = pd.Series([0 for _ in steps])

    for i in range(len(read_steps)):
        if results[i] == 'success':
            successes[read_steps[i]] += 1
        elif results[i] == 'explicit_error':
            explicit_errors[read_steps[i]] += 1
        elif results[i] == 'false_success':
            false_successes[read_steps[i]] += 1

    for correction_step in correction_df['step']:
        corrections[correction_step] += 1

    fig, ax = plt.subplots(figsize=(10, 5))

    times = pd.to_datetime(steps, unit='s')

    ax.plot(times, successes.cumsum(),
            linewidth=2, color="green", alpha=0.8, label="Successful Reads")

    ax.plot(times, explicit_errors.cumsum(),
            linewidth=2, color="orange", alpha=0.8, label="Unsuccessful Reads")

    ax.plot(times, false_successes.cumsum(),
            linewidth=2, color="red", alpha=0.8, label="False successes")

    ax.plot(times, corrections.cumsum(),
            linewidth=2, color="lightblue", alpha=0.8, label="Corrections")

    ax.set_xlabel("Time")
    ax.set_ylabel("Count")
    ax.set_title(f"Read Operations and Corrections Over Time ({config_name})")

    my_fmt = mdates.DateFormatter('%H:%M:%S')
    plt.gca().xaxis.set_major_formatter(my_fmt)
    ax.grid(True, alpha=0.3)
    ax.legend()

    # Add statistics annotation
    total_reads = len(read_steps)
    total_successful = successes.sum()
    total_unsuccessful = explicit_errors.sum() + false_successes.sum()
    success_rate = 100 * total_successful / total_reads if total_reads > 0 else 0
    total_corrections = corrections.sum()

    stats_text = f"Total Reads: {total_reads:,}\nSuccessful: {total_successful:,}\nUnsuccessful: {total_unsuccessful:,}\nTotal corrections: {total_corrections:,}\nSuccess Rate: {success_rate:.2f}%"
    ax.text(0.02, 0.98, stats_text, transform=ax.transAxes,
            ha="left", va="top",
            bbox=dict(boxstyle="round", facecolor="lightblue", alpha=0.8))

    fig.tight_layout()
    fig.savefig(out, dpi=100)
    plt.close(fig)


def generate_plots(logs_dir: Path, config_name: str, plots_dir: Path) -> tuple[float | None, float | None]:
    """Load logs and generate all plots for a configuration."""
    data = load_logs(logs_dir)

    # Generate read status plot with corrections
    plot_read_status_trend(data["read"], data["error"], data["detection"], data["correction"],
                           config_name, plots_dir / f"{config_name}_read_status_trend.png")
    avg_read_time = calculate_avg_time(data["read"])
    avg_write_time = calculate_avg_time(data["write"])

    print(f"  Generated plots for {config_name}")
    return avg_read_time, avg_write_time


def calculate_avg_time(df: pd.DataFrame) -> float | None:
    if "time" not in df or "size" not in df:
        return None
    filtered = df
    if "result" in df:
        filtered = df[df["result"] == "success"]

    # Filter out rows with invalid time or size
    filtered = filtered[(filtered["time"] > 0) & (filtered["size"] > 0)]

    if len(filtered) == 0:
        return None

    # Calculate time per byte for each operation, then average
    time_per_byte = filtered["time"] / filtered["size"]
    return float(time_per_byte.mean())


def run_simulation(config: dict[str, str], binary: Path, plots_dir: Path) -> tuple[str, float | None, float | None]:
    """Run simulator with config, generate plots, clean up logs."""
    config_name = config["name"]
    print(f"\n{'=' * 60}")
    print(f"Running simulation: {config_name}")
    print(f"{'=' * 60}")

    with tempfile.TemporaryDirectory() as temp_dir:
        temp_path = Path(temp_dir)
        config_file = create_config_file(config, temp_path)
        logs_dir = temp_path / "logs"
        logs_dir.mkdir()

        run_simulator(binary, config_file, logs_dir)
        try:
            avg_read_time, avg_write_time = generate_plots(logs_dir, config_name, plots_dir)
        except Exception:
            # Preserve logs for debugging on failure
            dest_root = Path("logs")
            dest_root.mkdir(exist_ok=True)
            dest = dest_root / f"failed_{config_name}"
            # Ensure unique destination
            idx = 1
            tmp_dest = dest
            while tmp_dest.exists():
                idx += 1
                tmp_dest = Path(f"{dest}_{idx}")
            shutil.copytree(logs_dir, tmp_dest)
            print(f"  Preserved failing logs at: {tmp_dest}", file=sys.stderr)
            raise
        # logs_dir is automatically deleted when temp_dir context exits

    print(f"  Cleaned up temporary logs")
    return config_name, avg_read_time, avg_write_time


def run_single_config(config, binary, plots_dir):
    try:
        return run_simulation(config, binary, plots_dir)
    except Exception as e:
        print(f"Error running {config['name']}: {e}", file=sys.stderr)
        return None


def plot_avg_times(avg_times: dict[str, float], out: Path, title: str) -> None:
    if not avg_times:
        print(f"No data for {title}; skipping plot.")
        return

    labels = list(avg_times.keys())
    values = list(avg_times.values())

    fig, ax = plt.subplots(figsize=(10, 5))
    ax.bar(labels, values)
    ax.set_xlabel("Error correction")
    ax.set_ylabel("Average time per byte (microseconds)")
    ax.set_title(title)

    for tick in ax.get_xticklabels():
        tick.set_rotation(25)
        tick.set_ha("right")

    fig.tight_layout()
    fig.savefig(out, dpi=100)
    plt.close(fig)


def main() -> int:
    """Main entry point: find binary, run configs, generate plots."""
    print("ParityPartyFS Simulation Runner")
    print("=" * 60)

    try:
        binary = find_binary()
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    plots_dir = Path("simulator_plots")
    plots_dir.mkdir(exist_ok=True)
    print(f"Plots will be saved to: {plots_dir.absolute()}\n")

    results = Parallel(n_jobs=-1)(
        delayed(run_single_config)(config, binary, plots_dir)
        for config in CONFIGS
    )

    read_avg_times: dict[str, float] = {}
    write_avg_times: dict[str, float] = {}

    for result in results:
        if result is None:
            continue
        name, avg_read, avg_write = result
        if avg_read is not None:
            read_avg_times[name] = avg_read
        if avg_write is not None:
            write_avg_times[name] = avg_write

    plot_avg_times(read_avg_times, plots_dir / "avg_read_times.png", "Average read operation time")
    plot_avg_times(write_avg_times, plots_dir / "avg_write_times.png", "Average write operation time")

    print(f"\n{'=' * 60}")
    print(f"All done! Plots saved to {plots_dir.absolute()}")
    print(f"{'=' * 60}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
