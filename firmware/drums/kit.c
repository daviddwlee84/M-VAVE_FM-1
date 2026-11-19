/* SPDX-License-Identifier: GPL-3.0-or-later */
/* kit: DRUMS' synthesised drums (see kit.h). */
#include "kit.h"
#include "groove.h"
#include "fastmath.h"

enum { KT_TONE, KT_CLAP, KT_METAL, KT_NOISE, KT_BELL };

typedef struct {
    uint8_t type;
    float f0, f1;                        /* tone: start and end pitch (Hz); METAL / BELL: the base pitch(es) */
    float pdec;                          /* pitch fall, ms */
    float tdec, tlev;                    /* tone decay (ms) and level */
    float ndec, nlev;                    /* noise (or metal) decay (ms) and level */
    float fc, q;                         /* noise filter: band-pass at fc (damping q), or a high-pass above 4.5 kHz */
    float level;
} kit_param_t;

/*                     type      f0     f1   pdec  tdec  tlev   ndec  nlev    fc    q    level */
static const kit_param_t KITS[KIT_N][GV_LANES] = {
    { /* 808 */
      { KT_TONE, 150.0f, 47.0f, 35.0f, 480.0f, 1.0f, 4.0f, 0.10f, 6000.0f, 0.0f, 1.00f },     /* kick */
      { KT_TONE, 250.0f, 185.0f, 25.0f, 90.0f, 0.55f, 150.0f, 0.75f, 4200.0f, 0.9f, 0.70f },   /* snare */
      { KT_TONE, 1700.0f, 1650.0f, 5.0f, 14.0f, 0.8f, 8.0f, 0.25f, 3200.0f, 0.8f, 0.45f },     /* rim */
      { KT_CLAP, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 190.0f, 1.0f, 1150.0f, 0.55f, 0.75f },          /* clap */
      { KT_METAL, 205.3f, 0.0f, 0.0f, 0.0f, 0.0f, 45.0f, 1.0f, 7000.0f, 0.0f, 0.45f },         /* hat */
      { KT_METAL, 205.3f, 0.0f, 0.0f, 0.0f, 0.0f, 300.0f, 1.0f, 7000.0f, 0.0f, 0.40f },        /* open hat */
      { KT_METAL, 290.0f, 0.0f, 0.0f, 0.0f, 0.0f, 450.0f, 1.0f, 5000.0f, 0.0f, 0.30f },        /* ride */
      { KT_METAL, 330.0f, 0.0f, 0.0f, 0.0f, 0.0f, 650.0f, 1.0f, 4800.0f, 0.0f, 0.45f },       /* crash */
      { KT_TONE, 260.0f, 200.0f, 70.0f, 260.0f, 1.0f, 10.0f, 0.04f, 6000.0f, 0.0f, 0.70f },    /* tom hi */
      { KT_TONE, 190.0f, 140.0f, 70.0f, 300.0f, 1.0f, 10.0f, 0.04f, 6000.0f, 0.0f, 0.72f },    /* tom mid */
      { KT_TONE, 135.0f, 95.0f, 70.0f, 340.0f, 1.0f, 10.0f, 0.04f, 6000.0f, 0.0f, 0.75f },     /* tom lo */
      { KT_BELL, 540.0f, 800.0f, 0.0f, 160.0f, 1.0f, 0.0f, 0.0f, 2600.0f, 0.6f, 0.35f },       /* cowbell */
      { KT_NOISE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 55.0f, 1.0f, 6500.0f, 0.0f, 0.30f },           /* shaker */
    },
    { /* STUDIO: tighter, closer to a played kit */
      { KT_TONE, 115.0f, 52.0f, 14.0f, 260.0f, 1.0f, 7.0f, 0.35f, 5000.0f, 0.0f, 1.00f },      /* kick */
      { KT_TONE, 205.0f, 175.0f, 12.0f, 70.0f, 0.45f, 190.0f, 1.0f, 3300.0f, 1.2f, 0.80f },    /* snare */
      { KT_TONE, 900.0f, 840.0f, 6.0f, 30.0f, 0.7f, 18.0f, 0.45f, 2500.0f, 1.0f, 0.45f },      /* rim */
      { KT_CLAP, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 150.0f, 1.0f, 1500.0f, 0.7f, 0.70f },           /* clap */
      { KT_NOISE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 50.0f, 1.0f, 8000.0f, 0.0f, 0.30f },           /* hat */
      { KT_NOISE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 450.0f, 1.0f, 7000.0f, 0.0f, 0.26f },          /* open hat */
      { KT_METAL, 420.0f, 0.0f, 0.0f, 0.0f, 0.0f, 550.0f, 1.0f, 6000.0f, 0.0f, 0.28f },       /* ride */
      { KT_NOISE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 700.0f, 1.0f, 3500.0f, 0.0f, 0.30f },         /* crash */
      { KT_TONE, 220.0f, 185.0f, 50.0f, 230.0f, 1.0f, 40.0f, 0.12f, 2200.0f, 1.0f, 0.72f },    /* tom hi */
      { KT_TONE, 165.0f, 138.0f, 50.0f, 260.0f, 1.0f, 40.0f, 0.12f, 1800.0f, 1.0f, 0.74f },    /* tom mid */
      { KT_TONE, 118.0f, 98.0f, 50.0f, 300.0f, 1.0f, 40.0f, 0.12f, 1400.0f, 1.0f, 0.78f },     /* tom lo */
      { KT_BELL, 560.0f, 840.0f, 0.0f, 120.0f, 1.0f, 0.0f, 0.0f, 2800.0f, 0.5f, 0.32f },       /* cowbell */
      { KT_NOISE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 70.0f, 1.0f, 7000.0f, 0.0f, 0.28f },           /* shaker */
    },
};
static const char *const KIT_NAME[KIT_N] = { "808", "STUDIO" };
static const float METAL[6] = { 1.0f, 1.4826f, 1.8003f, 2.5460f, 2.6303f, 3.8967f };   /* the 808's six */

typedef struct {
    uint8_t on, type, hp;
    float amp;
    float ph, f, f1, pe, pk, tlev, te, td;
    float ne, nd, nlev;
    float h1, h1x, h2, h2x, ha;          /* two one-pole high-passes */
    float lo, bd, bf, bq;                /* state-variable band-pass */
    float mph[6], minc[6];
    uint32_t t, burst;
} voice_t;

static voice_t vo[GV_LANES];
static float SR = 44100.0f;
static uint32_t rng = 0x1234567u;

void kit_init(float sr)
{
    uint32_t i;
    SR = sr;
    for (i = 0; i < GV_LANES; i++) vo[i].on = 0;
}

const char *kit_name(int kit) { return kit >= 0 && kit < KIT_N ? KIT_NAME[kit] : "?"; }

static float decay(float ms) { return ms > 0.0f ? fm_expf(-1000.0f / (ms * SR)) : 0.0f; }

void kit_trigger(int lane, int vel, int kit)
{
    const kit_param_t *p;
    voice_t *v;
    float g;
    int i;
    if (lane < 0 || lane >= GV_LANES || kit < 0 || kit >= KIT_N) return;
    p = &KITS[kit][lane];
    v = &vo[lane];
    if (lane == L_CHH) vo[L_OHH].nd = decay(6.0f);   /* the closed hat chokes the open one */
    g = (float)(vel < 1 ? 1 : vel > 9 ? 9 : vel) / 9.0f;
    v->type = p->type;
    v->amp = p->level * g * g;
    v->ph = 0.0f;
    v->f = p->f0;
    v->f1 = p->f1;
    v->pe = 1.0f;
    v->pk = decay(p->pdec);
    v->tlev = p->tlev;
    v->te = 1.0f;
    v->td = decay(p->tdec);
    v->ne = 1.0f;
    v->nd = decay(p->ndec);
    v->nlev = p->nlev;
    v->hp = p->fc > 4500.0f || p->type == KT_METAL || p->type == KT_NOISE;
    v->ha = 1.0f / (1.0f + 6.2831853f * p->fc / SR);
    v->h1 = v->h1x = v->h2 = v->h2x = 0.0f;
    v->lo = v->bd = 0.0f;
    v->bf = 2.0f * fm_sinf(3.1415927f * fm_minf(p->fc, 5000.0f) / SR);
    v->bq = p->q > 0.1f ? p->q : 0.1f;
    for (i = 0; i < 6; i++) {
        v->mph[i] = (float)i * 0.17f;
        v->minc[i] = p->f0 * METAL[i] / SR;
    }
    if (p->type == KT_BELL) {
        v->minc[0] = p->f0 / SR;
        v->minc[1] = p->f1 / SR;
    }
    v->t = 0;
    v->burst = (uint32_t)(0.011f * SR);
    v->on = 1;
}

static float noise(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (float)(int32_t)rng * (1.0f / 2147483648.0f);
}

static float hp2(voice_t *v, float x)
{
    v->h1 = v->ha * (v->h1 + x - v->h1x);
    v->h1x = x;
    v->h2 = v->ha * (v->h2 + v->h1 - v->h2x);
    v->h2x = v->h1;
    return v->h2;
}

static float bp(voice_t *v, float x)
{
    float hi;
    v->lo += v->bf * v->bd;
    hi = x - v->lo - v->bq * v->bd;
    v->bd += v->bf * hi;
    return v->bd;
}

static float sq(float *ph, float inc)
{
    *ph += inc;
    if (*ph >= 1.0f) *ph -= 1.0f;
    return *ph < 0.5f ? 1.0f : -1.0f;
}

void kit_render(float *out, uint32_t n)
{
    uint32_t i, k;
    for (i = 0; i < n; i++) out[i] = 0.0f;
    for (k = 0; k < GV_LANES; k++) {
        voice_t *v = &vo[k];
        if (!v->on) continue;
        for (i = 0; i < n; i++) {
            float s = 0.0f;
            switch (v->type) {
            case KT_TONE: {
                float f = v->f1 + (v->f - v->f1) * v->pe;
                v->pe *= v->pk;
                s = fm_sin_turns(v->ph) * v->te * v->tlev;
                v->ph += f / SR;
                if (v->ph >= 1.0f) v->ph -= 1.0f;
                v->te *= v->td;
                if (v->nlev > 0.0f) {
                    float x = noise();
                    s += (v->hp ? hp2(v, x) : bp(v, x)) * v->ne * v->nlev;
                    v->ne *= v->nd;
                }
                break;
            }
            case KT_CLAP: {
                float e;
                if (v->t < 3u * v->burst)
                    e = 1.0f - 0.85f * (float)(v->t % v->burst) / (float)v->burst;
                else {
                    e = v->ne;
                    v->ne *= v->nd;
                }
                s = bp(v, noise()) * e * 2.0f;
                break;
            }
            case KT_METAL: {
                float m = 0.0f;
                int j;
                for (j = 0; j < 6; j++) m += sq(&v->mph[j], v->minc[j]);
                s = hp2(v, m * 0.6f) * v->ne;
                v->ne *= v->nd;
                break;
            }
            case KT_NOISE: {
                float a = v->t < 64u ? (float)v->t / 64.0f : 1.0f;   /* a soft start (the shaker) */
                s = hp2(v, noise()) * v->ne * a;
                v->ne *= v->nd;
                break;
            }
            case KT_BELL:
                s = bp(v, (sq(&v->mph[0], v->minc[0]) + sq(&v->mph[1], v->minc[1])) * 0.5f) * v->te * 2.0f;
                v->te *= v->td;
                break;
            }
            out[i] += s * v->amp;
            v->t++;
        }
        /* done when every envelope is down */
        if ((v->type == KT_TONE && v->te < 0.001f && v->ne < 0.001f)
            || (v->type != KT_TONE && v->type != KT_BELL && v->ne < 0.001f && v->t > 3u * v->burst)
            || (v->type == KT_BELL && v->te < 0.001f))
            v->on = 0;
    }
}

void kit_silence(void)
{
    uint32_t k;
    for (k = 0; k < GV_LANES; k++) vo[k].on = 0;
}

int kit_active(void)
{
    int k, n = 0;
    for (k = 0; k < GV_LANES; k++) n += vo[k].on;
    return n;
}
