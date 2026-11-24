#!/bin/bash
# Bangun env jejak S2, flash, kirim beberapa frame, dan baca salinan frame bad pertama (g_badcopy).
cd "$(dirname "$0")/../../.."
$HOME/.platformio/penv/Scripts/pio.exe run -e host_s2_trace 2>&1 | grep -i "error\|SUCCESS" | head -3
python tools/1_akuisisi/flash_stop.py .pio/build/host_s2_trace/firmware.elf | tail -1
sleep 13
python - <<'PY'
import sys,time
sys.path.insert(0,"tools")
from run_bench import Attacker
at=Attacker("COM22"); at.stop(); at.send("a"); time.sleep(4.5); at.send("x"); time.sleep(0.3); print(at.status()); at.close()
PY
python tools/1_akuisisi/flash_stop.py --sym .pio/build/host_s2_trace/firmware.elf --baca g_bad_n:1 --baca g_bad_sync:1 --baca g_bad_ndtr:1 --baca g_bad_done:1 --baca g_badcopy:9 --coba 40 2>&1 | tail -6
