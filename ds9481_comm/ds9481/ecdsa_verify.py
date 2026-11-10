"""NIST P-256 ECDSA verification in pure Python (no external library).

Used to check, on the PC, a signature produced by the DS28E38. Slow compared to
a C library (about a second) but fine for a single check.
"""

P = 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF
A = P - 3
B = 0x5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B
N = 0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551
G = (0x6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296,
     0x4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5)


def on_curve(pt):
    x, y = pt
    return (y * y - (x * x * x + A * x + B)) % P == 0


def _add(p1, p2):
    if p1 is None:
        return p2
    if p2 is None:
        return p1
    if p1[0] == p2[0] and (p1[1] + p2[1]) % P == 0:
        return None
    if p1 == p2:
        lam = (3 * p1[0] * p1[0] + A) * pow(2 * p1[1], -1, P) % P
    else:
        lam = (p2[1] - p1[1]) * pow(p2[0] - p1[0], -1, P) % P
    x = (lam * lam - p1[0] - p2[0]) % P
    return x, (lam * (p1[0] - x) - p1[1]) % P


def _mul(k, pt):
    r = None
    while k:
        if k & 1:
            r = _add(r, pt)
        pt = _add(pt, pt)
        k >>= 1
    return r


def verify(pub_x, pub_y, digest, r, s):
    """True if (r, s) is a valid ECDSA-P256 signature of `digest` (32-byte hash)
    for the public key (pub_x, pub_y). All numbers are ints except `digest`."""
    pub = (pub_x, pub_y)
    if not (0 < r < N and 0 < s < N) or not on_curve(pub):
        return False
    w = pow(s, -1, N)
    pt = _add(_mul(int.from_bytes(digest, "big") * w % N, G), _mul(r * w % N, pub))
    return pt is not None and pt[0] % N == r
