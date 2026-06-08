import subprocess
import sys

import tqdm

from multiprocessing.pool import Pool

GAPBS_WORKLOADS = {
    "bfs": "-u 18 -k 18 -n 1",
    "bc": "-u 18 -k 10 -n 1 -i 2",
    "cc": "-u 18 -k 18 -n 1",
    "cc_sv": "-u 18 -k 18 -n 1",
    "pr": "-u 18 -k 18 -n 1 -i 10 -t 1e-4",
    "pr_spmv": "-u 18 -k 18 -n 1 -i 10 -t 1e-4",
    "sssp": "-u 18 -k 10 -n 1 -d 2",
    "tc": "-u 18 -k 10 -n 1",
}

def generate_gapbs_command(
    program,
    protocol,
    ncore,
    llc_size,
    l1d_size="16kB",
    l1i_size="16kB",
    l1_assoc=8,
    llc_assoc=16,
    mem_size="1GB",
):
    gem5_home = "/gem5/pecc"
    gapbs_dir = "/gem5/gem5-resources/src/gapbs/gapbs-with-roi-annotations/gapbs"
    config = f"{program}_{protocol}_{ncore}_{l1d_size}_{llc_size}"
    outdir = f"{gem5_home}/gapbs-out/{config}"
    binary = f"{gapbs_dir}/bin/{program}"
    wkdir = gapbs_dir
    env_file = f"{outdir}/env"

    if program not in GAPBS_WORKLOADS:
        print("Unknown program")
        sys.exit(-1)
    options = GAPBS_WORKLOADS[program]

    command = (
        f"{gem5_home}/build/X86_{protocol}/gem5.opt "
        f"-d {outdir} {gem5_home}/configs/example/se.py --ruby --num-cpus {ncore} "
        f"--cpu-type TimingSimpleCPU --l1d_size {l1d_size} --l1i_size {l1i_size} "
        f"--l2_size {llc_size} --mem-type SimpleMemory --mem-size {mem_size} "
        f"--l1d_assoc {l1_assoc} --l1i_assoc {l1_assoc} --l2_assoc {llc_assoc} "
        f"--env {env_file} "
        f'-c {binary} --options="{options}"'
    )

    env_lines = [f"OMP_NUM_THREADS={ncore}"]
    return (command, wkdir, outdir, config, env_file, env_lines)


def call_proc(args):
    cmd, wkdir, outdir, config, env_file, env_lines = args
    subprocess.run(f"mkdir -p {outdir}", shell=True)
    with open(env_file, "w") as fp:
        fp.write("\n".join(env_lines) + "\n")

    with open(f"{outdir}/run_log.txt", "w") as fp:
        p = subprocess.Popen(
            cmd,
            shell=True,
            executable="/bin/bash",
            stdout=fp,
            stderr=subprocess.STDOUT,
            cwd=wkdir,
        )
        p.communicate(timeout=3600 * 12 * 7)
        return (config, p.returncode)


if __name__ == "__main__":
    programs = ["bfs", "bc", "cc", "cc_sv", "pr", "pr_spmv", "sssp", "tc"]

    mem_size = "1GB"
    core_count = 4
    cmds = []
    for program in programs:
        for protocol in ['INCL','EXCL']:
            l1_size = "16kB"
            llc_size = "2048kB"
            cmds.append(
                generate_gapbs_command(
                    program,
                    protocol,
                    core_count,
                    llc_size,
                    l1i_size=l1_size,
                    l1d_size=l1_size,
                    mem_size=mem_size,
                )
            )

    pool = Pool(31)  # Modify here to configure the number of cores to run the simulation
    results = list(tqdm.tqdm(pool.imap_unordered(call_proc, cmds), total=len(cmds)))
    pool.close()
    pool.join()

    failed_configs = []
    for config, retcode in results:
        if retcode != 0:
            failed_configs.append((config, retcode))
    print(f"GAPBS run completes: {len(cmds) - len(failed_configs)} out of {len(cmds)} passes")
    if failed_configs:
        print("Failed experiments:")
        for config, retcode in failed_configs:
            print(f"{config}: retcode {retcode}")
