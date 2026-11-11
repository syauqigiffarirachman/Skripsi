"""Jalankan biner ARM (Cortex-M3) tiap varian di emulator Unicorn; hitung instruksi & ESTIMASI siklus
antara marker_begin() dan marker_end() (hanya pemanggilan sign), lalu verifikasi tanda tangan di PC.
Pemakaian: python tools/emu_cycles/run.py [varian ...]    (butuh: pip install unicorn capstone)

Model siklus: Cortex-M3 TRM (UMULL 4, UMLAL 5, MUL 1, LDR/STR 2 tetapi 1 bila beruntun, LDM/STM/PUSH/POP 1+N,
UDIV 7, cabang diambil +2) lalu dikalibrasi terhadap micro-ecc yang SUDAH TERUKUR di board (12.46 M siklus)."""
import json, os, statistics, subprocess, sys, time
from pathlib import Path
from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB, CS_MODE_MCLASS
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_BLOCK
from unicorn.arm_const import UC_ARM_REG_SP, UC_CPU_ARM_CORTEX_M3

HERE = Path(__file__).resolve().parent
OUT = HERE / "out"
PROJ = HERE.parents[1]
NM = Path.home() / ".platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-nm.exe"
sys.path.insert(0, str(PROJ / "tools"))
import p256

def _measured_uecc():     # rata-rata siklus uECC_sign terukur di board (data/k1_software_normal_48mhz.csv, N=30)
    rows = (PROJ / "data/k1_software_normal_48mhz.csv").read_text().splitlines()[1:]
    return statistics.mean(int(r.split(",")[1]) for r in rows if r)
MEASURED_UECC_CYCLES = _measured_uecc()
FLASH, FLASH_SZ, RAM, RAM_SZ = 0x08000000, 128 * 1024, 0x20000000, 20 * 1024
DIGEST = bytes.fromhex("D72C103978443C67EAD7542910F0EBF1C8BAF63182C07E062BCC6FC2E9A8A50E")
PRIV = int("643EC908DC6AF993A3287913132" "41E693FAAD7A2334208F12C99021AD3E0223D", 16)
PUB = p256.mul(PRIV, p256.G)

md = Cs(CS_ARCH_ARM, CS_MODE_THUMB | CS_MODE_MCLASS); md.detail = True
LOAD = {"ldr", "ldrb", "ldrh", "ldrsb", "ldrsh", "ldrt"}
STORE = {"str", "strb", "strh", "strt"}
MULTI = ("push", "pop", "ldm", "stm", "ldmia", "stmia", "ldmdb", "stmdb", "ldmfd", "stmfd")

def cost_block(code, addr):
    n = cyc = 0; prev_mem = False
    for ins in md.disasm(code, addr):
        m = ins.mnemonic.split(".")[0]
        base = m
        for cc in ("eq", "ne", "cs", "hs", "cc", "lo", "mi", "pl", "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le"):
            if m.endswith(cc) and m[:-2] in (LOAD | STORE | {"umull", "umlal", "smull", "smlal", "mul", "mla", "mls", "udiv", "sdiv"}):
                base = m[:-2]
        is_mem = base in LOAD or base in STORE
        if base in ("umull", "smull"): c = 4
        elif base in ("umlal", "smlal"): c = 5
        elif base == "mul": c = 1
        elif base in ("mla", "mls"): c = 2
        elif base in ("udiv", "sdiv"): c = 7
        elif is_mem: c = 1 if prev_mem else 2            # LDR/STR beruntun saling tumpang (pipelining M3)
        elif base.startswith(MULTI): c = 1 + max(1, ins.op_str.count(",") + 1 - (1 if "{" not in ins.op_str else 0))
        elif base in ("ldrd", "strd"): c = 3
        elif base in ("tbb", "tbh"): c = 4
        else: c = 1
        n += 1; cyc += c; prev_mem = is_mem
    return n, cyc

def symbols(elf):
    out = subprocess.run([str(NM), str(elf)], capture_output=True, text=True, check=True).stdout
    return {l.split()[2]: int(l.split()[0], 16) for l in out.splitlines() if len(l.split()) == 3}

def run_variant(name):
    elf, binf = OUT / f"{name}.elf", OUT / f"{name}.bin"
    sym = symbols(elf)
    img = binf.read_bytes()
    uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    uc.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_M3)
    uc.mem_map(FLASH, FLASH_SZ); uc.mem_map(RAM, RAM_SZ)
    uc.mem_write(FLASH, img)
    uc.reg_write(UC_ARM_REG_SP, RAM + RAM_SZ)
    begin, end, finish = sym["marker_begin"] & ~1, sym["marker_end"] & ~1, sym["finish"] & ~1
    st = dict(on=False, done=False, instr=0, cyc=0, blocks=0, prev_end=None)
    cache = {}
    def hook(uc_, addr, size, _):
        a = addr & ~1
        if a == end: st["on"] = False; st["done"] = True
        if st["on"]:
            k = (a, size)
            v = cache.get(k)
            if v is None:
                v = cache[k] = cost_block(bytes(uc_.mem_read(a, size)), a)
            st["instr"] += v[0]; st["cyc"] += v[1]; st["blocks"] += 1
            if st["prev_end"] is not None and st["prev_end"] != a: st["cyc"] += 2   # cabang diambil
            st["prev_end"] = a + size
        if a == begin: st["on"] = True; st["prev_end"] = None
    uc.hook_add(UC_HOOK_BLOCK, hook)
    t0 = time.time()
    uc.emu_start(sym["Reset_Handler"] | 1, finish)
    dt = time.time() - t0
    rd = lambda s, n: bytes(uc.mem_read(sym[s], n))
    sig = rd("g_sig", 64); ret = int.from_bytes(rd("g_ret", 4), "little", signed=True)
    r, s = int.from_bytes(sig[:32], "big"), int.from_bytes(sig[32:], "big")
    ok = ret == 0 and p256.verify(PUB, DIGEST, r, s) and not p256.verify(PUB, DIGEST, r, s ^ 1)
    u32 = lambda s_: int.from_bytes(rd(s_, 4), "little")
    return dict(name=name, ret=ret, sig_valid=bool(ok), instr=st["instr"], model_cycles=st["cyc"],
                stack=u32("g_stack"), heap_sbrk=u32("g_heap_sbrk"), heap_peak=u32("g_heap_peak"),
                flash_text=len(img), wall_s=round(dt, 1), sig=sig.hex().upper())

if __name__ == "__main__":
    names = sys.argv[1:] or ["uecc_ref", "tinycrypt", "mbedtls_default", "mbedtls_heavy"]
    res = {}
    for n in names:
        print(f"[{n}] menjalankan...", flush=True)
        res[n] = run_variant(n)
        r = res[n]
        print(f"   ret={r['ret']} tanda tangan valid (verifikasi PC)={r['sig_valid']}  instr={r['instr']:,}  model={r['model_cycles']:,} siklus  "
              f"stack={r['stack']} B heap_peak={r['heap_peak']} B heap_sbrk={r['heap_sbrk']} B  ({r['wall_s']} s)", flush=True)
    (OUT / "results.json").write_text(json.dumps(res, indent=1))
    if "uecc_ref" in res and res["uecc_ref"]["model_cycles"]:
        k = MEASURED_UECC_CYCLES / res["uecc_ref"]["model_cycles"]
        print(f"\nFaktor kalibrasi k = terukur/model (micro-ecc) = {k:.3f}")
        print(f"{'varian':16s} {'instr (M)':>10s} {'siklus est (M)':>15s} {'ms @48MHz':>10s} {'x micro-ecc':>11s}")
        for n, r in res.items():
            est = r["model_cycles"] * k
            print(f"{n:16s} {r['instr']/1e6:10.2f} {est/1e6:15.2f} {est/48e3:10.1f} {est/MEASURED_UECC_CYCLES:11.2f}")
