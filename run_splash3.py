import system
import multiprocessing
import subprocess
import shlex
import tqdm

from multiprocessing.pool import Pool


def generate_splash3_command(program, protocol, ncore, llc_size, l1d_size="16kB",  l1i_size="16kB", l1_assoc=8, llc_assoc=16, mem_size="1GB"):
    gem5_home = "/gem5/pecc"
    splash3_dir = "/gem5/pecc/splash-3-static-link/codes"
    config = f"{program}_{protocol}_{ncore}_{l1d_size}_{llc_size}"
    binary = ""
    stdin = ""
    options = ""
    wkdir = ""
    outdir = f"/gem5/pecc/splash-3-out/{config}"

    if program == 'cholesky':
        binary=f"{splash3_dir}/kernels/cholesky/CHOLESKY"
        options=f"-p{ncore} -B32 -C65536"
        stdin=f"{splash3_dir}/kernels/cholesky/inputs/tk16.O"
        wkdir=f"{splash3_dir}/kernels/cholesky"
    elif program == 'radix':
        binary=f"{splash3_dir}/kernels/radix/RADIX"
        options=f"-p{ncore} -n2097152"
        stdin=""
        wkdir=f"{splash3_dir}/kernels/radix"
    elif program == 'fft':
        binary=f"{splash3_dir}/kernels/fft/FFT"
        options=f"-p{ncore} -m20 -l6 -n16384"
        stdin=""
        wkdir=f"{splash3_dir}/kernels/fft"
    elif program == 'lu_contiguous':
        binary=f"{splash3_dir}/kernels/lu/contiguous_blocks/LU"
        options=f"-p{ncore} -n768"
        stdin=""
        wkdir=f"{splash3_dir}/kernels/lu/contiguous_blocks"
    elif program == 'lu_non_contig':
        binary=f"{splash3_dir}/kernels/lu/non_contiguous_blocks/LU"
        options=f"-p{ncore} -n768"
        stdin=""
        wkdir=f"{splash3_dir}/kernels/lu/non_contiguous_blocks"
    elif program == 'ocean_contiguous':
        binary=f"{splash3_dir}/apps/ocean/contiguous_partitions/OCEAN"
        options=f"-p{ncore} -n258"
        stdin=""
        wkdir=f"{splash3_dir}/apps/ocean/contiguous_partitions"
    elif program == 'ocean_non_contig':
        binary=f"{splash3_dir}/apps/ocean/non_contiguous_partitions/OCEAN"
        options=f"-p{ncore} -n258"
        stdin=""
        wkdir=f"{splash3_dir}/apps/ocean/non_contiguous_partitions"
    elif program == 'raytrace':
        binary=f"{splash3_dir}/apps/raytrace/RAYTRACE"
        options=f"-p{ncore} {splash3_dir}/apps/raytrace/inputs/teapot.env"
        stdin=""
        wkdir=f"{splash3_dir}/apps/raytrace"
    elif program == 'barnes':
        binary=f"{splash3_dir}/apps/barnes/BARNES"
        options=""
        stdin=f"{splash3_dir}/apps/barnes/inputs/n16384-p{ncore}"
        wkdir=f"{splash3_dir}/apps/barnes"
    elif program == 'water-nsquared':
        binary=f"{splash3_dir}/apps/water-nsquared/WATER-NSQUARED"
        options=""
        stdin=f"{splash3_dir}/apps/water-nsquared/inputs/n3375-p{ncore}"
        wkdir=f"{splash3_dir}/apps/water-nsquared"
    elif program == 'water-spatial':
        binary=f"{splash3_dir}/apps/water-spatial/WATER-SPATIAL"
        options=""
        stdin=f"{splash3_dir}/apps/water-spatial/inputs/n8000-p{ncore}"
        wkdir=f"{splash3_dir}/apps/water-spatial"
    elif program == 'fmm':
        binary=f"{splash3_dir}/apps/fmm/FMM"
        options=""
        stdin=f"{splash3_dir}/apps/fmm/inputs/input.{ncore}.16384"
        wkdir=f"{splash3_dir}/apps/fmm"
    elif program == 'raytrace':
        binary=f"{splash3_dir}/apps/raytrace/RAYTRACE"
        options=f"-p{ncore} {splash3_dir}/apps/raytrace/inputs/teapot.env"
        stdin=""
        wkdir=f"{splash3_dir}/apps/raytrace"
    elif program == 'radiosity':
        binary=f"{splash3_dir}/apps/{program}/{program.upper()}"
        options=f"-p {ncore} -ae 5000 -bf 0.1 -en 0.1 -room -batch"
        stdin=""
        wkdir=f"{splash3_dir}/apps/{program}"
    elif program == 'volrend':
        binary = f"{splash3_dir}/apps/volrend/VOLREND"
        options = f"{ncore} {splash3_dir}/apps/volrend/inputs/head-scaleddown2 4"
        stdin = ""
        wkdir = f"{splash3_dir}/apps/volrend"
    else:
        print("Unknown program")
        system.exit(-1)
    
    command = (
        f"{gem5_home}/build/X86_{protocol}/gem5.opt "
        # f"--debug-flags=FlexLLC --debug-file=flexllc.log --debug-start=0 "
        f"-d {outdir} {gem5_home}/configs/example/se.py --ruby --num-cpus {ncore} "
        f"--cpu-type TimingSimpleCPU --l1d_size {l1d_size} --l1i_size {l1i_size} "
        f"--l2_size {llc_size} --mem-type SimpleMemory --mem-size {mem_size} "
        f"--l1d_assoc {l1_assoc} --l1i_assoc {l1_assoc} --l2_assoc {llc_assoc} "
        f'-c {binary} --options="{options}"'
    )
    if stdin:
        command += f" < {stdin}"
    
    return (command, wkdir, outdir, config)



def call_proc(args):
    cmd, wkdir, outdir, config = args
    subprocess.run(f'mkdir -p {outdir}', shell=True)
    with open(f'{outdir}/run_log.txt', 'w') as fp:
        p = subprocess.Popen(cmd, shell=True, executable='/bin/bash', stdout=fp, stderr=subprocess.STDOUT, cwd=wkdir)
        p.communicate(timeout=3600*12*7)
        return (config, p.returncode)


if __name__ == '__main__':
    # Generate the commands to run Splash-3 benchmarks
    programs = ['cholesky', 'radix',  'ocean_contiguous', 'radiosity', 'volrend', 'fft', 'lu_contiguous', 'fmm', 'barnes', 'water-spatial']

    mem_size = '1GB'
    core_count= 4
    cmds = []
    for program in programs:
        for protocol in ['INCL','EXCL']:
            l1_size = '16kB'
            llc_size = '2048kB'
            cmds.append(generate_splash3_command(program, protocol, core_count, llc_size,l1i_size=l1_size, l1d_size=l1_size, mem_size=mem_size))
            # l1_size = '8kB'
            # llc_size = '2048kB'
            # cmds.append(generate_splash3_command(program, protocol, core_count, llc_size,l1i_size=l1_size, l1d_size=l1_size, mem_size=mem_size))
            # l1_size = '32kB'
            # llc_size = '2048kB'
            # cmds.append(generate_splash3_command(program, protocol, core_count, llc_size,l1i_size=l1_size, l1d_size=l1_size, mem_size=mem_size))
            # l1_size = '16kB'
            # llc_size = '1024kB'
            # cmds.append(generate_splash3_command(program, protocol, core_count, llc_size,l1i_size=l1_size, l1d_size=l1_size, mem_size=mem_size))
            # l1_size = '16kB'
            # llc_size = '4096kB'
            # cmds.append(generate_splash3_command(program, protocol, core_count, llc_size,l1i_size=l1_size, l1d_size=l1_size, mem_size=mem_size))

    pool = Pool(31)  # Modify here to configure the number of cores to run the simulation
    results = list(tqdm.tqdm(pool.imap_unordered(call_proc, cmds), total=len(cmds)))
    pool.close()
    pool.join()
    
    failure = 0
    failed_configs = []
    for config, retcode in results:
        if retcode != 0:
            failed_configs.append((config, retcode))
    print(f"Splash3 run completes: {len(cmds) - len(failed_configs)} out of {len(cmds)} passes")
    if failed_configs:
        print("Failed experiments:")
        for config, retcode in failed_configs:
            print(f"{config}: retcode {retcode}")
