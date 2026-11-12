"""Referensi ECDSA P-256 deterministik (RFC 6979) murni Python, untuk membandingkan keluaran firmware.

Hanya memakai hashlib/hmac + aritmetika kurva dari ds9481_comm/ds9481/ecdsa_verify.py.
Jalankan langsung untuk uji mandiri terhadap vektor resmi RFC 6979 Lampiran A.2.5:
    python tools/rfc6979_ref.py
"""
import hashlib
import hmac
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "ds9481_comm"))
from ds9481.ecdsa_verify import G, N, _mul  # noqa: E402


def _hmac(key, data):
    return hmac.new(key, data, hashlib.sha256).digest()


def nonce_k(x: int, h1: bytes) -> int:
    """RFC 6979 bagian 3.2 untuk P-256 + SHA-256 (qlen = hlen = 256, jadi bits2int tanpa geser)."""
    xb = x.to_bytes(32, "big")
    h2 = (int.from_bytes(h1, "big") % N).to_bytes(32, "big")        # bits2octets(h1)
    v, k = b"\x01" * 32, b"\x00" * 32
    k = _hmac(k, v + b"\x00" + xb + h2)
    v = _hmac(k, v)
    k = _hmac(k, v + b"\x01" + xb + h2)
    v = _hmac(k, v)
    while True:
        v = _hmac(k, v)
        cand = int.from_bytes(v, "big")
        if 1 <= cand < N:
            return cand
        k = _hmac(k, v + b"\x00")
        v = _hmac(k, v)


def sign(x: int, h1: bytes):
    """-> (r, s) sesuai RFC 6979; h1 = digest 32 byte yang langsung ditandatangani."""
    k = nonce_k(x, h1)
    r = _mul(k, G)[0] % N
    s = pow(k, -1, N) * (int.from_bytes(h1, "big") + r * x) % N
    return r, s


def selftest() -> bool:
    x = 0xC9AFA9D845BA75166B5C215767B1D6934E50C3DB36E89B127B8A622B120F6721
    h1 = hashlib.sha256(b"sample").digest()
    k_ok = nonce_k(x, h1) == 0xA6E3C57DD01ABE90086538398355DD4C3B17AA873382B0F24D6129493D8AAD60
    r, s = sign(x, h1)
    r_ok = r == 0xEFD48B2AACB6A8FD1140DD9CD45E81D69D2C877B56AAF991C34D0EA84EAF3716
    s_ok = s == 0xF7CB1C942D657C41D436C7A1B6E29F65F3E900DBB9AFF4064DC4AB2F843ACDA8
    print(f"vektor RFC 6979 A.2.5 (P-256, SHA-256, 'sample'): k={'OK' if k_ok else 'SALAH'} "
          f"r={'OK' if r_ok else 'SALAH'} s={'OK' if s_ok else 'SALAH'}")
    return k_ok and r_ok and s_ok


if __name__ == "__main__":
    sys.exit(0 if selftest() else 1)
