"""Bangun varian ECDSA P-256 untuk emulator Cortex-M3 dengan flag kompilasi yang SAMA dengan PlatformIO.
Keluaran: out/<nama>.elf, out/<nama>.bin.   Pemakaian: python tools/emu_cycles/build.py [varian ...]
Prasyarat: `pio run -e softwaretes` pernah dijalankan (mengunduh micro-ecc); TinyCrypt di-clone otomatis
ke tools/emu_cycles/_ext/ pada pemakaian pertama."""
import os, subprocess, sys
from pathlib import Path

EMU = Path(__file__).resolve().parent
OUT = EMU / "out"; OUT.mkdir(exist_ok=True)
PROJ = EMU.parents[1]
TC = Path.home() / ".platformio/packages/toolchain-gccarmnoneeabi/bin"
GCC, OBJCOPY, SIZE = str(TC / "arm-none-eabi-gcc.exe"), str(TC / "arm-none-eabi-objcopy.exe"), str(TC / "arm-none-eabi-size.exe")

COMMON = ["-std=gnu17", "-mcpu=cortex-m3", "-mthumb", "-Os", "-ffunction-sections", "-fdata-sections",
          "--param", "max-inline-insns-single=500", "-DNDEBUG", f"-I{EMU}"]
UECC_DIR = PROJ / ".pio/libdeps/softwaretes/micro-ecc"
TINY_REPO = EMU / "_ext" / "tinycrypt"
if not TINY_REPO.exists():
    TINY_REPO.parent.mkdir(exist_ok=True)
    subprocess.run(["git", "clone", "--quiet", "--depth", "1", "--branch", "v0.2.8",
                    "https://github.com/intel/tinycrypt.git", str(TINY_REPO)], check=True)
TINY = TINY_REPO / "lib"
MBED = PROJ / "lib/mbedtls"
MBED_SRC = sorted((MBED / "library").glob("*.c"))

VARIANTS = {
    "uecc_ref": dict(  # persis konfigurasi env softwaretes (yang sudah diukur di board: 12,46 M siklus)
        src=[EMU / "test_uecc.c", UECC_DIR / "uECC.c"],
        flags=[f"-I{UECC_DIR}", "-DuECC_SUPPORTS_secp160r1=0", "-DuECC_SUPPORTS_secp192r1=0",
               "-DuECC_SUPPORTS_secp224r1=0", "-DuECC_SUPPORTS_secp256k1=0", "-DuECC_SUPPORT_COMPRESSED_POINT=0"]),
    "tinycrypt": dict(
        src=[EMU / "test_tinycrypt.c", TINY / "source/ecc.c", TINY / "source/ecc_dsa.c", TINY / "source/utils.c"],
        flags=[f"-I{TINY/'include'}"]),
    "mbedtls_default": dict(
        src=[EMU / "test_mbedtls.c", *MBED_SRC],
        flags=[f"-I{PROJ/'include'}", f"-I{MBED/'include'}", '-DMBEDTLS_CONFIG_FILE="mbedtls_heavy_config.h"',
               "-DMBEDTLS_PROFILE_DEFAULT"]),
    "mbedtls_heavy": dict(
        src=[EMU / "test_mbedtls.c", *MBED_SRC],
        flags=[f"-I{PROJ/'include'}", f"-I{MBED/'include'}", '-DMBEDTLS_CONFIG_FILE="mbedtls_heavy_config.h"']),
}

CFGDIR = EMU / "cfg"
for _n, _h in [("abl_noNIST", "cfg_noNIST.h"), ("abl_noComb_w2", "cfg_noComb.h"), ("abl_noAsm", "cfg_noAsm.h"), ("abl_noFixed_w4", "cfg_noFixed.h")]:
    VARIANTS[_n] = dict(src=[EMU / "test_mbedtls.c", *MBED_SRC],
                        flags=[f"-I{CFGDIR}", f"-I{MBED/'include'}", f'-DMBEDTLS_CONFIG_FILE="{_h}"'])

def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode:
        print(r.stdout, r.stderr); sys.exit(f"gagal: {' '.join(map(str, cmd[:3]))} ...")
    return r

names = sys.argv[1:] or list(VARIANTS)
for name in names:
    v = VARIANTS[name]; objs = []
    objdir = OUT / name; objdir.mkdir(exist_ok=True)
    for s in [EMU / "startup.c", *v["src"]]:
        o = objdir / (s.stem + ".o")
        run([GCC, *COMMON, *v["flags"], "-c", str(s), "-o", str(o)]); objs.append(str(o))
    elf = OUT / f"{name}.elf"
    run([GCC, "-mcpu=cortex-m3", "-mthumb", "-Os", "-nostartfiles", "--specs=nano.specs", "-Wl,--gc-sections",
         f"-Wl,-Map={OUT/(name+'.map')}", "-T", str(EMU / "link.ld"), *objs, "-lc", "-lgcc", "-o", str(elf)])
    run([OBJCOPY, "-O", "binary", str(elf), str(OUT / f"{name}.bin")])
    sz = run([SIZE, str(elf)]).stdout.strip().splitlines()[-1].split()
    print(f"{name:16s} text={sz[0]:>6} data={sz[1]:>4} bss={sz[2]:>5}  (flash ~ {int(sz[0])+int(sz[1])} B)")
