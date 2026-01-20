from __future__ import annotations

import subprocess
import sys
import tempfile
import time
from pathlib import Path
import shutil
from queue import Queue, Empty
from typing import Iterable
from concurrent.futures import ThreadPoolExecutor

import enlighten
import matplotlib.pyplot as plt
import pandas as pd

COMMON_CONFIG = {
    "use_journal": "false",
    "krad_per_year": 90.0,
    "bit_flip_seed": 67,
    "num_users": 3,
    "max_write_size": 256,
    "max_read_size": 1024,
    "avg_steps_between_ops": 2,
    "create_weight": 2,
    "write_weight": 10,
    "read_weight": 4,
    "delete_weight": 1,
    "simulation_seconds": 3 * 365 * 24 * 60 * 60,
    "seconds_per_step": 300,
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
    raise FileNotFoundError("usage_simulator binary not found. Build the project first.")


def create_config_file(config: dict[str, str], temp_dir: Path) -> Path:
    config_path = temp_dir / "config.txt"
    with open(config_path, "w") as f:
        for key, value in config.items():
            if key != "name":
                f.write(f"{key}={value}\n")
    return config_path


def _read_csv(path: Path) -> pd.DataFrame:
    if not path.exists():
        return pd.DataFrame()
    return pd.read_csv(path)


def load_logs(logs_dir: Path) -> dict[str, pd.DataFrame]:
    return {
        "read": _read_csv(logs_dir / "read.csv"),
        "write": _read_csv(logs_dir / "write.csv"),
        "flip": _read_csv(logs_dir / "flip.csv"),
        "correction": _read_csv(logs_dir / "correction.csv"),
        "detection": _read_csv(logs_dir / "detection.csv"),
        "error": _read_csv(logs_dir / "error.csv"),
    }


def plot_read_status_trend(read_df: pd.DataFrame, error_df: pd.DataFrame,
                           detection_df: pd.DataFrame, correction_df: pd.DataFrame,
                           config_name: str, out: Path) -> None:
    if read_df.empty: return

    read_steps = read_df['step']
    results = read_df['result']
    steps = range(int(COMMON_CONFIG["simulation_seconds"]) // int(COMMON_CONFIG["seconds_per_step"]) + 1)

    successes = pd.Series([0 for _ in steps])
    explicit_errors = pd.Series([0 for _ in steps])
    false_successes = pd.Series([0 for _ in steps])
    corrections = pd.Series([0 for _ in steps])

    for i in range(len(read_steps)):
        idx = read_steps[i]
        if idx >= len(successes): continue
        if results[i] == 'success':
            successes[idx] += 1
        elif results[i] == 'explicit_error':
            explicit_errors[idx] += 1
        elif results[i] == 'false_success':
            false_successes[idx] += 1

    if not correction_df.empty:
        for correction_step in correction_df['step']:
            if correction_step < len(corrections):
                corrections[correction_step] += 1

    fig, ax = plt.subplots(figsize=(10, 5))
    times_in_seconds = [step * COMMON_CONFIG["seconds_per_step"] for step in steps]

    total_seconds = COMMON_CONFIG["simulation_seconds"]
    total_years = total_seconds / (365 * 24 * 60 * 60)

    if total_years >= 1:
        x_data = [t / (24 * 60 * 60) / 365 for t in times_in_seconds]
        time_unit = "Years"
    else:
        x_data = [t / (24 * 60 * 60) for t in times_in_seconds]
        time_unit = "Days"

    ax.plot(x_data, successes.cumsum(), linewidth=2, color="green", alpha=0.8, label="Successful Reads")
    ax.plot(x_data, explicit_errors.cumsum(), linewidth=2, color="orange", alpha=0.8, label="Unsuccessful Reads")
    ax.plot(x_data, false_successes.cumsum(), linewidth=2, color="red", alpha=0.8, label="False successes")
    ax.plot(x_data, corrections.cumsum(), linewidth=2, color="lightblue", alpha=0.8, label="Corrections")

    ax.set_xlabel(f"Time ({time_unit})")
    ax.set_ylabel("Count")
    ax.set_title(f"Read Operations and Corrections Over Time ({config_name})")
    ax.grid(True, alpha=0.3)
    ax.legend()

    total_reads = len(read_steps)
    total_successful = successes.sum()
    total_unsuccessful = explicit_errors.sum() + false_successes.sum()
    success_rate = 100 * total_successful / total_reads if total_reads > 0 else 0
    total_corrections = corrections.sum()

    stats_text = f"Total Reads: {total_reads:,}\nSuccessful: {total_successful:,}\nUnsuccessful: {total_unsuccessful:,}\nTotal corrections: {total_corrections:,}\nSuccess Rate: {success_rate:.2f}%"
    ax.text(0.02, 0.98, stats_text, transform=ax.transAxes,
            ha="left", va="top", bbox=dict(boxstyle="round", facecolor="lightblue", alpha=0.8))

    fig.tight_layout()
    fig.savefig(out, dpi=100)
    plt.close(fig)


def generate_plots(logs_dir: Path, config_name: str, plots_dir: Path) -> tuple[float | None, float | None]:
    data = load_logs(logs_dir)
    plot_read_status_trend(data["read"], data["error"], data["detection"], data["correction"],
                           config_name, plots_dir / f"{config_name}_read_status_trend.png")
    avg_read_time = calculate_avg_time(data["read"])
    avg_write_time = calculate_avg_time(data["write"])
    return avg_read_time, avg_write_time


def calculate_avg_time(df: pd.DataFrame) -> float | None:
    if df.empty or "time" not in df or "size" not in df:
        return None
    filtered = df
    if "result" in df:
        filtered = df[df["result"] == "success"]
    if len(filtered) == 0:
        return None
    time_per_byte = filtered["time"] / filtered["size"]
    return float(time_per_byte.mean())


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


def run_simulation_process(binary: Path, config_path: Path, logs_dir: Path,
                           name: str, progress_queue: Queue) -> None:
    """Run the C++ binary and push progress updates to the queue."""
    cmd = [str(binary), str(config_path), str(logs_dir)]

    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )

    total = 100
    for line in iter(process.stdout.readline, ''):
        line = line.strip()
        if not line: continue
        if line.startswith("PROGRESS:"):
            try:
                # Expected format: "PROGRESS:15/100"
                parts = line.split(':')[1].split('/')
                current = int(parts[0])
                total = int(parts[1]) if len(parts) > 1 else 100

                progress_queue.put((name, current, total))
            except (ValueError, IndexError):
                pass
        else:
            print(f"[{name}] OUT: {line}")

    process.wait()

    if process.returncode != 0:
        print(f"[{name}] Failed with return code {process.returncode}")
        raise RuntimeError(f"Simulation {name} failed")

    progress_queue.put((name, total, total))


def run_simulation_task(config: dict[str, str], binary: Path, plots_dir: Path,
                        progress_queue: Queue) -> tuple[str, float | None, float | None]:
    """Orchestrates config creation, simulation run, and plotting for one config."""
    config_name = config["name"]

    with tempfile.TemporaryDirectory() as temp_dir:
        temp_path = Path(temp_dir)
        config_file = create_config_file(config, temp_path)
        logs_dir = temp_path / "logs"
        logs_dir.mkdir()

        run_simulation_process(binary, config_file, logs_dir, config_name, progress_queue)

        try:
            avg_read_time, avg_write_time = generate_plots(logs_dir, config_name, plots_dir)
        except Exception as e:
            dest_root = Path("logs_failed")
            dest_root.mkdir(exist_ok=True)
            dest = dest_root / f"failed_{config_name}"
            if dest.exists(): shutil.rmtree(dest)
            shutil.copytree(logs_dir, dest)

            print(f"[{config_name}] Plotting failed: {e}")
            print(f"[{config_name}] Logs saved to {dest}")
            raise

    return config_name, avg_read_time, avg_write_time


def main() -> int:
    print("ParityPartyFS Simulation Runner")
    print("=" * 60)

    try:
        binary = find_binary()
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    plots_dir = Path("plots")
    plots_dir.mkdir(exist_ok=True)
    print(f"Plots will be saved to: {plots_dir.absolute()}\n")

    manager = enlighten.get_manager()
    progress_queue = Queue()

    counters = {}
    for config in CONFIGS:
        name = config["name"]
        counters[name] = manager.counter(
            total=100, desc=name, unit='steps', leave=False, color='green'
        )

    with ThreadPoolExecutor(max_workers=8) as executor:
        futures = {
            executor.submit(
                run_simulation_task, config, binary, plots_dir, progress_queue
            ): config["name"]
            for config in CONFIGS
        }

        while True:
            while True:
                try:
                    name, current, total = progress_queue.get_nowait()
                    if name in counters:
                        ctr = counters[name]
                        if ctr.total != total:
                            ctr.total = total
                        ctr.count = current
                        ctr.refresh()
                except Empty:
                    break

            incomplete = [f for f in futures if not f.done()]
            if not incomplete and progress_queue.empty():
                break

            time.sleep(0.1)

        results = []
        for future in futures:
            try:
                res = future.result()
                if res: results.append(res)
            except Exception as e:
                print(f"Task failed: {e}")

    manager.stop()

    read_avg_times: dict[str, float] = {}
    write_avg_times: dict[str, float] = {}

    for name, avg_read, avg_write in results:
        if avg_read is not None: read_avg_times[name] = avg_read
        if avg_write is not None: write_avg_times[name] = avg_write

    plot_avg_times(read_avg_times, plots_dir / "avg_read_times.png", "Average read operation time")
    plot_avg_times(write_avg_times, plots_dir / "avg_write_times.png", "Average write operation time")

    print(f"\n{'=' * 60}")
    print(f"All done! Plots saved to {plots_dir.absolute()}")
    print(f"{'=' * 60}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
