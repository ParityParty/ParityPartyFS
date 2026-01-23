from __future__ import annotations

import json
import subprocess
import tempfile
from pathlib import Path
from typing import Iterable

import matplotlib.pyplot as plt
import pandas as pd

# =========================
# CONFIG
# =========================

FIO_RUNTIME = 20
FILE_SIZE = "1024"
BLOCK_SIZE = "1024"

ECC_CONFIGS = [
    {
        "name": "none",
        "ecc_type": "none",
    },
    {
        "name": "crc",
        "ecc_type": "crc",
        "crc_polynomial": "0x9960034c",
    },
    {
        "name": "hamming",
        "ecc_type": "hamming",
    },
    {
        "name": "rs_1",
        "ecc_type": "reed_solomon",
        "rs_correctable_bytes": "1",
    },
    {
        "name": "rs_3",
        "ecc_type": "reed_solomon",
        "rs_correctable_bytes": "3",
    },
]

# =========================
# HELPERS
# =========================

def write_ppfs_config(cfg: dict[str, str], path: Path) -> None:
    with open(path, "w") as f:
        f.write(
            f"""
total_size = 1073741824
average_file_size = 4096
block_size = 256
use_journal = false
ecc_type = {cfg["ecc_type"]}
"""
        )
        if "crc_polynomial" in cfg:
            f.write(f"crc_polynomial = {cfg['crc_polynomial']}\n")
        if "rs_correctable_bytes" in cfg:
            f.write(f"rs_correctable_bytes = {cfg['rs_correctable_bytes']}\n")


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, check=True)


def run_fio(mount_point: Path) -> dict:
    fio_cmd = [
        "fio",
        "--name=ppfs-test",
        f"--directory={mount_point}",
        "--rw=randrw",
        "--ioengine=sync",
        "--direct=1",
        "--numjobs=1",
        f"--size={FILE_SIZE}",
        f"--bs={BLOCK_SIZE}",
        f"--runtime={FIO_RUNTIME}",
        "--time_based",
        "--group_reporting",
        "--output-format=json",
    ]

    result = subprocess.run(
        fio_cmd,
        capture_output=True,
        text=True,
        check=True,
    )
    return json.loads(result.stdout)


# =========================
# MAIN RUNNER
# =========================

def run_single_ecc(cfg: dict[str, str], out_dir: Path) -> dict:
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
            fio_out = run_fio(mnt)
        finally:
            run(["fusermount3", "-u", str(mnt)])
            proc.terminate()

    job = fio_out["jobs"][0]
    return {
        "ecc": name,
        "read_bw_MBps": job["read"]["bw"] / 1024,
        "write_bw_MBps": job["write"]["bw"] / 1024,
        "read_lat_us": job["read"]["lat_ns"]["mean"] / 1000,
        "write_lat_us": job["write"]["lat_ns"]["mean"] / 1000,
    }


def plot(df: pd.DataFrame, out: Path) -> None:
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
    print("\n✨ DONE ✨ plots in fio_plots/")


if __name__ == "__main__":
    main()
