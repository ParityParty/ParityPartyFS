from __future__ import annotations

import json
import subprocess
import tempfile
from pathlib import Path
import statistics
from dataclasses import dataclass

import matplotlib.pyplot as plt
import pandas as pd

# =========================
# CONFIG
# =========================

TOTAL_SIZE = 1024 * 1024 * 1024  # 1 GiB
BLOCK_SIZE = 256
AVERAGE_FILE_SIZE = 4096
USE_JOURNAL = False

FIO_RUNTIME = 10
FILE_SIZE = "50k"
BATCH_SIZE = "1k"

ECC_CONFIGS = [
    {
        "name": "None",
        "ecc_type": "none",
    },
    {
        "name": "CRC",
        "ecc_type": "crc",
        "crc_polynomial": "0x9960034c",
    },
    {
        "name": "Hamming",
        "ecc_type": "hamming",
    },
    {
        "name": "Parity check",
        "ecc_type": "parity",
    },
    {
        "name": "RS, strength 1",
        "ecc_type": "reed_solomon",
        "rs_correctable_bytes": "1",
    },
    {
        "name": "RS, strength 3",
        "ecc_type": "reed_solomon",
        "rs_correctable_bytes": "3",
    },
]

MAX_RUNS = 30
MIN_RUNS = 5
TARGET_CV = 0.05  

# =========================

@dataclass
class ECCBenchmarkResult:
    ecc: str
    read_bw_MBps: float
    write_bw_MBps: float
    read_lat_us: float
    write_lat_us: float

def write_ppfs_config(cfg: dict[str, str], path: Path) -> None:
    """Writes a PPFS configuration file based on the provided ECC configuration."""
    with open(path, "w") as f:
        f.write(
            f"""
total_size = {TOTAL_SIZE}
average_file_size = {AVERAGE_FILE_SIZE}
block_size = {BLOCK_SIZE}
use_journal = {USE_JOURNAL}
ecc_type = {cfg["ecc_type"]}
"""
        )
        if "crc_polynomial" in cfg:
            f.write(f"crc_polynomial = {cfg['crc_polynomial']}\n")
        if "rs_correctable_bytes" in cfg:
            f.write(f"rs_correctable_bytes = {cfg['rs_correctable_bytes']}\n")


def run(cmd: list[str]) -> None:
    """Runs a shell command."""
    subprocess.run(cmd, check=True)


def run_fio(mount_point: Path, ecc: str) -> ECCBenchmarkResult:
    """Runs fio on the given mount point and returns the parsed JSON output."""
    fio_cmd = [
        "fio",
        "--name=ppfs-test",
        f"--directory={mount_point}",
        "--rw=randrw",
        "--ioengine=sync",
        "--direct=1",
        "--numjobs=1",
        f"--size={FILE_SIZE}",
        f"--bs={BATCH_SIZE}",
        f"--runtime={FIO_RUNTIME}",
        "--time_based",
        "--output-format=json",
        "--group_reporting"
    ]

    result = subprocess.run(
        fio_cmd,
        capture_output=True,
        text=True,
        check=True,
    )
    result_dict = json.loads(result.stdout)
    job = result_dict["jobs"][0]
    
    return ECCBenchmarkResult(
        ecc=ecc,
        read_bw_MBps=job["read"]["bw"] / 1024,
        write_bw_MBps=job["write"]["bw"] / 1024,
        read_lat_us=job["read"]["lat_ns"]["mean"] / 1_000,
        write_lat_us=job["write"]["lat_ns"]["mean"] / 1_000,
    )


def calculate_mean_max_cv(data: list[ECCBenchmarkResult]) -> tuple[ECCBenchmarkResult, float]:
    """ Calculates the mean and maximum coefficient of variation from the given data."""
    mean = ECCBenchmarkResult(
        ecc=data[0].ecc,
        read_bw_MBps=statistics.mean([d.read_bw_MBps for d in data]),
        write_bw_MBps=statistics.mean([d.write_bw_MBps for d in data]),
        read_lat_us=statistics.mean([d.read_lat_us for d in data]),
        write_lat_us=statistics.mean([d.write_lat_us for d in data]),
    )
    cv_values = ECCBenchmarkResult(
        ecc=data[0].ecc,
        read_bw_MBps=statistics.stdev([d.read_bw_MBps for d in data]) / mean.read_bw_MBps,
        write_bw_MBps=statistics.stdev([d.write_bw_MBps for d in data]) / mean.write_bw_MBps,
        read_lat_us=statistics.stdev([d.read_lat_us for d in data]) / mean.read_lat_us,
        write_lat_us=statistics.stdev([d.write_lat_us for d in data]) / mean.write_lat_us,
    )
    max_cv = max(
        cv_values.read_bw_MBps,
        cv_values.write_bw_MBps,
        cv_values.read_lat_us,
        cv_values.write_lat_us,
    )
    return mean, max_cv


def measure_fio(mount_point: Path, ecc: str) -> ECCBenchmarkResult:
    results: list[ECCBenchmarkResult] = []
    mean, max_cv = None, None
    while len(results) < MAX_RUNS:
        results.append(run_fio(mount_point, ecc))
        if len(results) < MIN_RUNS:
            continue

        mean, max_cv = calculate_mean_max_cv(results)
        if max_cv <= TARGET_CV:
            break


    mean.ecc = ecc
    print(f"Completed {len(results)} runs for ECC: {ecc} with max CV: {max_cv:.4f}")
    return mean

def run_single_ecc(cfg: dict[str, str], out_dir: Path) -> ECCBenchmarkResult:
    """Runs the benchmark for a single ECC configuration."""
    name = cfg["name"]
    print(f"\n=== Running ECC: {name} ===")

    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        img = tmp / "ppfs.img"
        cfg_file = tmp / "ppfs.cfg"
        mnt = tmp / "mnt"
        mnt.mkdir()

        write_ppfs_config(cfg, cfg_file)

        run(["./build/release/fuse_exec/mkfs_ppfs", str(cfg_file), str(img)])
        proc = subprocess.Popen(
            ["./build/release/fuse_exec/mount_ppfs", str(img), str(mnt), "--", "-f"]
        )

        try:
            mean_result = measure_fio(mnt, name)
        finally:
            run(["fusermount3", "-u", str(mnt)])
            proc.terminate()

    mean_result.ecc = name
    return mean_result


def plot(df: pd.DataFrame, out: Path) -> None:
    """Generates and saves plots from the benchmark results."""
    fig, axes = plt.subplots(2, 2, figsize=(12, 8))

    df.plot(x="ecc", y="read_bw_MBps", kind="bar", ax=axes[0][0], title="Read BW [MB/s]")
    df.plot(x="ecc", y="write_bw_MBps", kind="bar", ax=axes[0][1], title="Write BW [MB/s]")
    df.plot(x="ecc", y="read_lat_us", kind="bar", ax=axes[1][0], title="Read latency [µs]")
    df.plot(x="ecc", y="write_lat_us", kind="bar", ax=axes[1][1], title="Write latency [µs]")

    for ax in axes.flat:
        ax.set_xlabel("")
        ax.grid(True, alpha=0.3)

    fig.tight_layout()
    fig.savefig(out, dpi=120)
    plt.close(fig)


def main() -> None:
    out_dir = Path("fio_plots")
    out_dir.mkdir(exist_ok=True)

    results = []
    for cfg in ECC_CONFIGS:
        results.append(run_single_ecc(cfg, out_dir))

    df = pd.DataFrame(results)
    df.to_csv(out_dir / "results.csv", index=False)

    plot(df, out_dir / "ecc_comparison.png")
    print("All done! Plots saved to", out_dir)


if __name__ == "__main__":
    main()
