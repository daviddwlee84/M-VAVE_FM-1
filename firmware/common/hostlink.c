/* SPDX-License-Identifier: MIT */
/* hostlink: see hostlink.h. Transport: USB SysEx through fm1-base (plat_sysex_send, APP_SYSEX). A Bluetooth
 * path would replace hl_send's plat_sysex_send and feed hl_match from reassembled BLE packets; the frames
 * stay the same. */
#include "hostlink.h"

void hl_init(hostlink_t *h, uint8_t id0, uint8_t id1, uint8_t ver)
{
    h->id0 = id0;
    h->id1 = id1;
    h->ver = ver;
    h->acked = 0;
    h->seen = h->sent = h->session = 0;
}

const uint8_t *hl_match(const hostlink_t *h, const uint8_t *p, uint32_t n, uint8_t *type, uint32_t *len)
{
    if (n < 5u || p[0] != 0x7Du || p[1] != h->id0 || p[2] != h->id1 || p[3] != h->ver)
        return 0;
    *type = p[4];
    *len = n - 5u;
    return p + 5;
}

void hl_seen(hostlink_t *h, uint32_t now) { h->seen = now ? now : 1u; }

void hl_acked(hostlink_t *h, uint32_t now)
{
    hl_seen(h, now);
    h->acked = 1;
    h->session = plat_usb_session();
}

int hl_host(const hostlink_t *h, uint32_t now) { return h->acked && h->seen && now - h->seen < HOSTLINK_TIMEOUT_MS; }

int hl_send(hostlink_t *h, uint8_t type, const uint8_t *payload, uint32_t n)
{
    uint8_t f[HOSTLINK_MAX + 7u];
    uint32_t i, k = 0;
    if (n > HOSTLINK_MAX)
        return -1;
    f[k++] = 0xF0;
    f[k++] = 0x7D;
    f[k++] = h->id0;
    f[k++] = h->id1;
    f[k++] = h->ver;
    f[k++] = type & 0x7Fu;
    for (i = 0; i < n; i++)
        f[k++] = payload[i] & 0x7Fu;
    f[k++] = 0xF7;
    return plat_sysex_send(f, k);
}

void hl_tick(hostlink_t *h, uint32_t now, uint8_t hello, uint8_t heartbeat, const uint8_t *hello_p, uint32_t hello_n)
{
    if (h->acked && (!hl_host(h, now) || plat_usb_session() != h->session))
        h->acked = 0;                    /* lost, or a new cable: say HELLO again */
    if (h->sent && now - h->sent < HOSTLINK_EVERY_MS)
        return;
    h->sent = now ? now : 1u;
    if (h->acked)
        hl_send(h, heartbeat, &h->ver, 1);
    else
        hl_send(h, hello, hello_p, hello_n);
}
