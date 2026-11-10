#include "p256.h"

#include <string.h>

/* 256-bit numbers: 8 x 32-bit limbs, least significant limb first.
 * Modular multiplication uses Montgomery arithmetic (CIOS), one context for the
 * field prime p and one for the group order n. Points are Jacobian, a = -3. */

typedef uint32_t bn[8];

typedef struct {
    bn m;        /* modulus (odd) */
    uint32_t n0; /* -m^-1 mod 2^32 */
    bn rr;       /* R^2 mod m, R = 2^256 */
    bn one;      /* R mod m (the number 1 in Montgomery form) */
} mont_t;

static const char *HEX_P = "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF";
static const char *HEX_N = "FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551";
static const char *HEX_B = "5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B";
static const char *HEX_GX = "6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296";
static const char *HEX_GY = "4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5";

/* ---------------------------------------------------------------- plain bn */
static void bn_from_be(bn r, const uint8_t b[32])
{
    int i;
    for (i = 0; i < 8; i++) {
        const uint8_t *p = b + 28 - 4 * i;
        r[i] = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
    }
}

static int hexval(char c)
{
    return c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10;
}

static void bn_from_hex(bn r, const char *hex)
{
    uint8_t b[32];
    int i;
    for (i = 0; i < 32; i++)
        b[i] = (uint8_t)(hexval(hex[2 * i]) * 16 + hexval(hex[2 * i + 1]));
    bn_from_be(r, b);
}

static int bn_cmp(const bn a, const bn b)
{
    int i;
    for (i = 7; i >= 0; i--) {
        if (a[i] != b[i])
            return a[i] > b[i] ? 1 : -1;
    }
    return 0;
}

static int bn_is_zero(const bn a)
{
    int i;
    for (i = 0; i < 8; i++)
        if (a[i])
            return 0;
    return 1;
}

/* r = a + b, returns carry */
static uint32_t bn_add(bn r, const bn a, const bn b)
{
    uint64_t c = 0;
    int i;
    for (i = 0; i < 8; i++) {
        c += (uint64_t)a[i] + b[i];
        r[i] = (uint32_t)c;
        c >>= 32;
    }
    return (uint32_t)c;
}

/* r = a - b, returns borrow */
static uint32_t bn_sub(bn r, const bn a, const bn b)
{
    int64_t c = 0;
    int i;
    for (i = 0; i < 8; i++) {
        c += (int64_t)a[i] - b[i];
        r[i] = (uint32_t)c;
        c >>= 32;
    }
    return (uint32_t)(c & 1);
}

static int bn_bit(const bn a, int i)
{
    return (int)((a[i >> 5] >> (i & 31)) & 1);
}

/* ------------------------------------------------------------ mod arithmetic */
static void mod_add(const bn m, bn r, const bn a, const bn b)
{
    uint32_t carry = bn_add(r, a, b);
    if (carry || bn_cmp(r, m) >= 0)
        bn_sub(r, r, m);
}

static void mod_sub(const bn m, bn r, const bn a, const bn b)
{
    if (bn_sub(r, a, b))
        bn_add(r, r, m);
}

static void mont_mul(const mont_t *c, bn out, const bn a, const bn b)
{
    uint32_t t[10] = {0};
    uint64_t cur, carry;
    uint32_t mm;
    int i, j;

    for (i = 0; i < 8; i++) {
        carry = 0;
        for (j = 0; j < 8; j++) {
            cur = (uint64_t)t[j] + (uint64_t)a[j] * b[i] + carry;
            t[j] = (uint32_t)cur;
            carry = cur >> 32;
        }
        cur = (uint64_t)t[8] + carry;
        t[8] = (uint32_t)cur;
        t[9] = (uint32_t)(cur >> 32);

        mm = t[0] * c->n0;
        cur = (uint64_t)t[0] + (uint64_t)mm * c->m[0];
        carry = cur >> 32;
        for (j = 1; j < 8; j++) {
            cur = (uint64_t)t[j] + (uint64_t)mm * c->m[j] + carry;
            t[j - 1] = (uint32_t)cur;
            carry = cur >> 32;
        }
        cur = (uint64_t)t[8] + carry;
        t[7] = (uint32_t)cur;
        t[8] = t[9] + (uint32_t)(cur >> 32);
    }
    for (i = 0; i < 8; i++)
        out[i] = t[i];
    if (t[8] || bn_cmp(out, c->m) >= 0)
        bn_sub(out, out, c->m);
}

static void mont_init(mont_t *c, const bn m)
{
    uint32_t inv = m[0];
    int i;
    bn x, one = {1, 0, 0, 0, 0, 0, 0, 0};

    memcpy(c->m, m, sizeof(bn));
    for (i = 0; i < 5; i++)  /* Newton iteration for m0^-1 mod 2^32 */
        inv *= 2 - m[0] * inv;
    c->n0 = (uint32_t)(0u - inv);

    /* R^2 mod m = 2^512 mod m: start at 1 and double 512 times */
    memcpy(x, one, sizeof(bn));
    for (i = 0; i < 512; i++)
        mod_add(m, x, x, x);
    memcpy(c->rr, x, sizeof(bn));
    mont_mul(c, c->one, one, c->rr);  /* R mod m */
}

static void to_mont(const mont_t *c, bn out, const bn a)
{
    mont_mul(c, out, a, c->rr);
}

static void from_mont(const mont_t *c, bn out, const bn a)
{
    bn one = {1, 0, 0, 0, 0, 0, 0, 0};
    mont_mul(c, out, a, one);
}

/* out = a^(m-2) in Montgomery form (modular inverse, m prime) */
static void mont_inv(const mont_t *c, bn out, const bn a)
{
    bn e, two = {2, 0, 0, 0, 0, 0, 0, 0}, acc;
    int i;
    bn_sub(e, c->m, two);
    memcpy(acc, c->one, sizeof(bn));
    for (i = 255; i >= 0; i--) {
        mont_mul(c, acc, acc, acc);
        if (bn_bit(e, i))
            mont_mul(c, acc, acc, a);
    }
    memcpy(out, acc, sizeof(bn));
}

/* ------------------------------------------------------------ curve points */
typedef struct {
    bn x, y, z;  /* Jacobian, Montgomery form; z == 0 means the point at infinity */
} pt;

static mont_t FP, FN;      /* field prime and group order contexts */
static bn B_M, GX_M, GY_M; /* curve constants in Montgomery form (field p) */
static int ready;

static void setup(void)
{
    bn p, n, b, gx, gy;
    if (ready)
        return;
    bn_from_hex(p, HEX_P);
    bn_from_hex(n, HEX_N);
    bn_from_hex(b, HEX_B);
    bn_from_hex(gx, HEX_GX);
    bn_from_hex(gy, HEX_GY);
    mont_init(&FP, p);
    mont_init(&FN, n);
    to_mont(&FP, B_M, b);
    to_mont(&FP, GX_M, gx);
    to_mont(&FP, GY_M, gy);
    ready = 1;
}

#define FMUL(o, a, b) mont_mul(&FP, o, a, b)
#define FADD(o, a, b) mod_add(FP.m, o, a, b)
#define FSUB(o, a, b) mod_sub(FP.m, o, a, b)

static int pt_is_inf(const pt *p)
{
    return bn_is_zero(p->z);
}

static void pt_set_inf(pt *p)
{
    memset(p, 0, sizeof *p);
}

static void pt_double(pt *r, const pt *p)
{
    bn delta, gamma, beta, t1, t2, alpha, b4, b8, gg, tmp;
    pt out;

    if (pt_is_inf(p) || bn_is_zero(p->y)) {
        pt_set_inf(r);
        return;
    }
    FMUL(delta, p->z, p->z);
    FMUL(gamma, p->y, p->y);
    FMUL(beta, p->x, gamma);
    FSUB(t1, p->x, delta);
    FADD(t2, p->x, delta);
    FMUL(alpha, t1, t2);                 /* alpha = 3*(x-delta)*(x+delta) */
    FADD(tmp, alpha, alpha);
    FADD(alpha, tmp, alpha);
    FADD(b4, beta, beta);                /* 4*beta */
    FADD(b4, b4, b4);
    FADD(b8, b4, b4);                    /* 8*beta */
    FMUL(out.x, alpha, alpha);
    FSUB(out.x, out.x, b8);              /* x3 = alpha^2 - 8 beta */
    FADD(tmp, p->y, p->z);
    FMUL(out.z, tmp, tmp);
    FSUB(out.z, out.z, gamma);
    FSUB(out.z, out.z, delta);           /* z3 = (y+z)^2 - gamma - delta */
    FSUB(tmp, b4, out.x);
    FMUL(out.y, alpha, tmp);
    FMUL(gg, gamma, gamma);
    FADD(gg, gg, gg);
    FADD(gg, gg, gg);
    FADD(gg, gg, gg);                    /* 8*gamma^2 */
    FSUB(out.y, out.y, gg);              /* y3 = alpha (4 beta - x3) - 8 gamma^2 */
    *r = out;
}

static void pt_add(pt *r, const pt *p, const pt *q)
{
    bn z1z1, z2z2, u1, u2, s1, s2, h, i, j, rr, v, tmp;
    pt out;

    if (pt_is_inf(p)) { *r = *q; return; }
    if (pt_is_inf(q)) { *r = *p; return; }

    FMUL(z1z1, p->z, p->z);
    FMUL(z2z2, q->z, q->z);
    FMUL(u1, p->x, z2z2);
    FMUL(u2, q->x, z1z1);
    FMUL(tmp, q->z, z2z2);
    FMUL(s1, p->y, tmp);
    FMUL(tmp, p->z, z1z1);
    FMUL(s2, q->y, tmp);
    FSUB(h, u2, u1);
    FSUB(rr, s2, s1);
    if (bn_is_zero(h)) {
        if (bn_is_zero(rr))
            pt_double(r, p);   /* same point */
        else
            pt_set_inf(r);     /* p == -q */
        return;
    }
    FADD(tmp, h, h);
    FMUL(i, tmp, tmp);                    /* I = (2H)^2 */
    FMUL(j, h, i);                        /* J = H*I */
    FADD(rr, rr, rr);                     /* r = 2 (S2 - S1) */
    FMUL(v, u1, i);                       /* V = U1*I */
    FMUL(out.x, rr, rr);
    FSUB(out.x, out.x, j);
    FSUB(out.x, out.x, v);
    FSUB(out.x, out.x, v);                /* X3 = r^2 - J - 2V */
    FSUB(tmp, v, out.x);
    FMUL(out.y, rr, tmp);
    FMUL(tmp, s1, j);
    FADD(tmp, tmp, tmp);
    FSUB(out.y, out.y, tmp);              /* Y3 = r (V - X3) - 2 S1 J */
    FADD(tmp, p->z, q->z);
    FMUL(tmp, tmp, tmp);
    FSUB(tmp, tmp, z1z1);
    FSUB(tmp, tmp, z2z2);
    FMUL(out.z, tmp, h);                  /* Z3 = ((Z1+Z2)^2 - Z1Z1 - Z2Z2) H */
    *r = out;
}

/* r = k * base (k plain 256-bit integer) */
static void pt_mul(pt *r, const bn k, const pt *base)
{
    pt acc;
    int i;
    pt_set_inf(&acc);
    for (i = 255; i >= 0; i--) {
        pt_double(&acc, &acc);
        if (bn_bit(k, i))
            pt_add(&acc, &acc, base);
    }
    *r = acc;
}

static int on_curve(const bn x, const bn y)  /* x, y in Montgomery form */
{
    bn lhs, rhs, x2, t;
    FMUL(lhs, y, y);
    FMUL(x2, x, x);
    FMUL(rhs, x2, x);
    FADD(t, x, x);
    FADD(t, t, x);                 /* 3x */
    FSUB(rhs, rhs, t);
    FADD(rhs, rhs, B_M);           /* x^3 - 3x + b */
    return bn_cmp(lhs, rhs) == 0;
}

int p256_ecdsa_verify(const uint8_t pub_x[32], const uint8_t pub_y[32],
                      const uint8_t digest[32], const uint8_t r_b[32], const uint8_t s_b[32])
{
    bn r, s, e, qx, qy, w, u1, u2, tmp, x_aff, p_mod, n_mod;
    bn rm, sm, em;
    pt g, q, a, b, sum;
    bn zinv, zinv2;

    setup();
    bn_from_be(r, r_b);
    bn_from_be(s, s_b);
    bn_from_be(e, digest);
    bn_from_be(qx, pub_x);
    bn_from_be(qy, pub_y);
    memcpy(p_mod, FP.m, sizeof(bn));
    memcpy(n_mod, FN.m, sizeof(bn));

    if (bn_is_zero(r) || bn_cmp(r, n_mod) >= 0 || bn_is_zero(s) || bn_cmp(s, n_mod) >= 0)
        return 0;
    if (bn_cmp(qx, p_mod) >= 0 || bn_cmp(qy, p_mod) >= 0)
        return 0;

    memset(&q, 0, sizeof q);
    to_mont(&FP, q.x, qx);
    to_mont(&FP, q.y, qy);
    memcpy(q.z, FP.one, sizeof(bn));
    if (!on_curve(q.x, q.y))
        return 0;

    /* w = s^-1 mod n; u1 = e*w mod n; u2 = r*w mod n */
    if (bn_cmp(e, n_mod) >= 0)
        bn_sub(e, e, n_mod);
    to_mont(&FN, sm, s);
    mont_inv(&FN, w, sm);
    to_mont(&FN, em, e);
    to_mont(&FN, rm, r);
    mont_mul(&FN, tmp, em, w);
    from_mont(&FN, u1, tmp);
    mont_mul(&FN, tmp, rm, w);
    from_mont(&FN, u2, tmp);

    memset(&g, 0, sizeof g);
    memcpy(g.x, GX_M, sizeof(bn));
    memcpy(g.y, GY_M, sizeof(bn));
    memcpy(g.z, FP.one, sizeof(bn));

    pt_mul(&a, u1, &g);
    pt_mul(&b, u2, &q);
    pt_add(&sum, &a, &b);
    if (pt_is_inf(&sum))
        return 0;

    /* affine x = X / Z^2, then x mod n == r ? */
    mont_inv(&FP, zinv, sum.z);
    FMUL(zinv2, zinv, zinv);
    FMUL(tmp, sum.x, zinv2);
    from_mont(&FP, x_aff, tmp);
    if (bn_cmp(x_aff, n_mod) >= 0)
        bn_sub(x_aff, x_aff, n_mod);
    return bn_cmp(x_aff, r) == 0;
}
