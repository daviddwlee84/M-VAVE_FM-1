/* SPDX-License-Identifier: MIT */
/* hostlink: an app's own SysEx conversation with a program on the computer, over fm1-base.
 *
 * Frames are F0 7D <id0> <id1> <ver> <type> <payload...> F7. 7D is the MIDI non-commercial ID; pick
 * id0/id1 so you stay apart from fm1-base's own 7D 4C .. frames (Conductor uses 43 4E, "CN").
 * The device says HELLO once a second until the host answers, then a HEARTBEAT once a second; the host
 * counts as present while frames keep coming (HOSTLINK_TIMEOUT_MS).
 *
 * Use (unity build: app.c includes hostlink.c; fm1.toml include = ["../common"]):
 *   static hostlink_t hl;
 *   hl_init(&hl, 0x43, 0x4E, 1);
 *   int app_sysex(const uint8_t *p, uint32_t n) { uint8_t t; uint32_t len; const uint8_t *d =
 *       hl_match(&hl, p, n, &t, &len); if (!d) return 0; hl_seen(&hl, plat_ms()); ...; return 1; }
 *   each frame: hl_tick(&hl, plat_ms(), T_HELLO, T_HEARTBEAT, hello_payload, 3);
 */
#pragma once
#include <stdint.h>

#define HOSTLINK_TIMEOUT_MS 10000u
#define HOSTLINK_EVERY_MS 1000u
#define HOSTLINK_MAX 64u                 /* the longest frame an app sends through hl_send */

typedef struct {
    uint8_t id0, id1, ver;
    uint8_t acked;                       /* the host answered our HELLO since it was last lost */
    uint32_t seen;                       /* plat_ms of the last frame from the host (0: never) */
    uint32_t sent;                       /* plat_ms of the last HELLO / HEARTBEAT */
    uint32_t session;                    /* plat_usb_session when acked: a new cable means a new HELLO */
} hostlink_t;

void hl_init(hostlink_t *h, uint8_t id0, uint8_t id1, uint8_t ver);
/* one of ours (p: the bytes between F0 and F7): its type and payload; 0 when it is someone else's */
const uint8_t *hl_match(const hostlink_t *h, const uint8_t *p, uint32_t n, uint8_t *type, uint32_t *len);
void hl_seen(hostlink_t *h, uint32_t now);
void hl_acked(hostlink_t *h, uint32_t now);
int hl_host(const hostlink_t *h, uint32_t now);   /* 1: the host is there */
int hl_send(hostlink_t *h, uint8_t type, const uint8_t *payload, uint32_t n);   /* 0 = queued */
void hl_tick(hostlink_t *h, uint32_t now, uint8_t hello, uint8_t heartbeat, const uint8_t *hello_p, uint32_t hello_n);
