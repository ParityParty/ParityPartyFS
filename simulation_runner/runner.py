from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Iterable

import matplotlib.pyplot as plt
import pandas as pd

CONFIGS = [
    {
        "name": "Crc1024",
        "block_size": "1024",
        "ecc_type": "Crc",
        "rs_correctable_bytes": "2",
        "use_journal": "false",
        "bit_flip_probability": "0.02",
        "bit_flip_seed": "1",
        "num_users": "10",
        "max_write_size": "1000000",
        "max_read_size": "1000000",
        "avg_steps_between_ops": "90",
        "create_weight": "3",
        "write_weight": "10",
        "read_weight": "9",
        "delete_weight": "2",
        "max_iterations": "10000",
    },
    {
        "name": "None1024",
        "block_size": "1024",
        "ecc_type": "None",
        "rs_correctable_bytes": "2",
        "use_journal": "false",
        "bit_flip_probability": "0.02",
        "bit_flip_seed": "1",
        "num_users": "10",
        "max_write_size": "1000000",
        "max_read_size": "1000000",
        "avg_steps_between_ops": "90",
        "create_weight": "3",
        "write_weight": "10",
        "read_weight": "9",
        "delete_weight": "2",
        "max_iterations": "10000",
    },
    {
        "name": "Hamming1024",
        "block_size": "1024",
        "ecc_type": "Hamming",
        "rs_correctable_bytes": "2",
        "use_journal": "false",
        "bit_flip_probability": "0.02",
        "bit_flip_seed": "1",
        "num_users": "10",
        "max_write_size": "1000000",
        "max_read_size": "1000000",
        "avg_steps_between_ops": "90",
        "create_weight": "3",
        "write_weight": "10",
        "read_weight": "9",
        "delete_weight": "2",
        "max_iterations": "10000",
    },
    {
        "name": "ReedSolomon256_2correctable",
        "block_size": "256",
        "ecc_type": "ReedSolomon",
        "rs_correctable_bytes": "2",
        "use_journal": "false",
        "bit_flip_probability": "0.02",
        "bit_flip_seed": "2",
        "num_users": "10",
        "max_write_size": "1000000",
        "max_read_size": "1000000",
        "avg_steps_between_ops": "90",
        "create_weight": "3",
        "write_weight": "10",
        "read_weight": "9",
        "delete_weight": "2",
        "max_iterations": "10000",
    },
    {
        "name": "ReedSolomon256_1correctable",
        "block_size": "256",
        "ecc_type": "ReedSolomon",
        "rs_correctable_bytes": "1",
        "use_journal": "false",
        "bit_flip_probability": "0.02",
        "bit_flip_seed": "2",
        "num_users": "10",
        "max_write_size": "1000000",
        "max_read_size": "1000000",
        "avg_steps_between_ops": "90",
        "create_weight": "3",
        "write_weight": "10",
        "read_weight": "9",
        "delete_weight": "2",
        "max_iterations": "10000",
    },
    {
        "name": "ReedSolomon256_3correctable",
        "block_size": "256",
        "ecc_type": "ReedSolomon",
        "rs_correctable_bytes": "3",
        "use_journal": "false",
        "bit_flip_probability": "0.02",
        "bit_flip_seed": "2",
        "num_users": "10",
        "max_write_size": "1000000",
        "max_read_size": "1000000",
        "avg_steps_between_ops": "90",
        "create_weight": "3",
        "write_weight": "10",
        "read_weight": "9",
        "delete_weight": "2",
        "max_iterations": "10000",
    }
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
    return pd.read_csv(path)


def load_logs(logs_dir: Path) -> dict[str, pd.DataFrame]:
    """Load all log CSVs from the logs directory."""
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
    """Plot successful reads, unsuccessful reads, and corrections over time.
    
    All reads: read.csv + detection.csv
    Unsuccessful reads: detection.csv + errors with "Data contains an error"
    Successful reads: all reads - unsuccessful reads
    """
    if read_df.empty and detection_df.empty:
        print("  Warning: No read data available")
        return

    # Find reads with errors
    read_steps_with_errors = set()
    if not error_df.empty:
        errors = error_df[
            error_df["message"].str.contains("Data contains an error", na=False)]
        read_steps_with_errors = set(errors["step"].unique())

    # All reads: read.csv + detection.csv
    all_read_steps = set()
    if not read_df.empty:
        all_read_steps.update(read_df["step"].unique())
    if not detection_df.empty:
        all_read_steps.update(detection_df["step"].unique())

    # Unsuccessful reads: detection.csv + errors with "Data contains an error"
    unsuccessful_steps = set()
    if not detection_df.empty:
        unsuccessful_steps.update(detection_df["step"].unique())
    unsuccessful_steps.update(read_steps_with_errors)

    # Successful reads: all reads - unsuccessful reads
    successful_steps = all_read_steps - unsuccessful_steps

    # Create sorted dataframes
    if successful_steps:
        successful_sorted = pd.DataFrame({"step": sorted(successful_steps)})
        successful_sorted["cumulative_success"] = range(1, len(successful_sorted) + 1)
    else:
        successful_sorted = pd.DataFrame(columns=["step", "cumulative_success"])

    if unsuccessful_steps:
        unsuccessful_sorted = pd.DataFrame({"step": sorted(unsuccessful_steps)})
        unsuccessful_sorted["cumulative_unsuccessful"] = range(1, len(unsuccessful_sorted) + 1)
    else:
        unsuccessful_sorted = pd.DataFrame(columns=["step", "cumulative_unsuccessful"])

    # Calculate cumulative corrections
    if not correction_df.empty:
        correction_sorted = correction_df.sort_values("step").reset_index(drop=True)
        correction_sorted["cumulative_corrections"] = range(1, len(correction_sorted) + 1)
    else:
        correction_sorted = pd.DataFrame(columns=["step", "cumulative_corrections"])

    fig, ax = plt.subplots(figsize=(10, 5))

    if not successful_sorted.empty:
        ax.plot(successful_sorted["step"], successful_sorted["cumulative_success"],
                linewidth=2, color="green", alpha=0.8, label="Successful Reads")

    if not unsuccessful_sorted.empty:
        ax.plot(unsuccessful_sorted["step"], unsuccessful_sorted["cumulative_unsuccessful"],
                linewidth=2, color="red", alpha=0.8, label="Unsuccessful Reads")

    if not correction_sorted.empty:
        ax.plot(correction_sorted["step"], correction_sorted["cumulative_corrections"],
                linewidth=2, color="blue", alpha=0.8, label="Corrections")

    ax.set_xlabel("Simulation Step")
    ax.set_ylabel("Count")
    ax.set_title(f"Read Operations and Corrections Over Time ({config_name})")
    ax.grid(True, alpha=0.3)
    ax.legend()

    # Add statistics annotation
    total_reads = len(all_read_steps)
    total_successful = len(successful_steps)
    total_unsuccessful = len(unsuccessful_steps)
    total_corrections = len(correction_sorted) if not correction_sorted.empty else 0
    success_rate = 100 * total_successful / total_reads if total_reads > 0 else 0

    stats_text = f"Total Reads: {total_reads:,}\nSuccessful: {total_successful:,}\nUnsuccessful: {total_unsuccessful:,}\nCorrections: {total_corrections:,}\nSuccess Rate: {success_rate:.2f}%"
    ax.text(0.02, 0.98, stats_text, transform=ax.transAxes,
            ha="left", va="top",
            bbox=dict(boxstyle="round", facecolor="lightblue", alpha=0.8))

    fig.tight_layout()
    fig.savefig(out, dpi=100)
    plt.close(fig)


def generate_plots(logs_dir: Path, config_name: str, plots_dir: Path) -> None:
    """Load logs and generate all plots for a configuration."""
    data = load_logs(logs_dir)

    # Generate read status plot with corrections
    plot_read_status_trend(data["read"], data["error"], data["detection"], data["correction"],
                           config_name, plots_dir / f"{config_name}_read_status_trend.png")

    print(f"  Generated plots for {config_name}")


def run_simulation(config: dict[str, str], binary: Path, plots_dir: Path) -> None:
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
        generate_plots(logs_dir, config_name, plots_dir)
        # logs_dir is automatically deleted when temp_dir context exits

    print(f"  Cleaned up temporary logs")


def main() -> int:
    """Main entry point: find binary, run configs, generate plots."""
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

    for config in CONFIGS:
        try:
            run_simulation(config, binary, plots_dir)
        except Exception as e:
            print(f"Error running {config['name']}: {e}", file=sys.stderr)
            continue

    print(f"\n{'=' * 60}")
    print(f"All done! Plots saved to {plots_dir.absolute()}")
    print(f"{'=' * 60}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
