# A3: pull-up 1-Wire (PA15 - 3V3), 21 Nov 2026

Alat: SANWA CD800a, mode Ω. Semua tanpa daya (host, logger, sumber 5 V dicabut; ST-Link dicabut).
Pembacaan dilaporkan user sebagai "4,66" (diinterpretasikan 4,66 kOhm, rentang 40 kOhm; ketidakpastian katalog +-(1,5 % + 5 digit) = +-(0,07 + 0,05) = +-0,12 kOhm).

| Keadaan | Resistansi PA15 - 3V3 |
|---|---|
| Papan DS9121 dicabut (pull-up papan host saja) | 4,66 kOhm |
| Papan DS9121 terpasang | 4,66 kOhm |

Kesimpulan: pull-up = 4,66 +- 0,12 kOhm (nominal 4,7 kOhm +-5 % terpenuhi); papan DS9121 tidak menambah jalur paralel yang terukur (tanpa daya).

Belum: foto/catatan posisi jumper VCC/CEXT pada papan DS9121.

Satuan dikonfirmasi user: 4,66 kOhm.
Papan DS9121: tulisan "VCC CEXT" adalah SATU pin/pad (bukan jumper pemilih dua posisi), sesuai catatan 17 Nov bahwa DS28E38 hanya memakai IO + GND (selalu parasit) dan pad itu untuk chip lain. Belum dicatat apakah pad itu terhubung ke kabel/3V3 atau dibiarkan.
