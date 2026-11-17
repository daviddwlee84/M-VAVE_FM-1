/* SPDX-License-Identifier: MIT */
/* Conductor: the FM-1 as a desk console for coding agents. The keys are agents, their lights say how urgent
 * each one is, the screen says what it is, a chime says it without looking. The host daemon
 * (host/conductor) does the talking to hc or Herdr; the device sends intents and the host checks them.
 * Built on fm1-base (MPL-2.0, its headers); this app and firmware/common are MIT. */
#pragma once
#include <stdint.h>
#include "plat.h"

#define APP_NAME "CONDUCTOR"
#define APP_SPLASH_RGB RGB(255, 176, 0)
#define APP_BLOCK 256u                   /* frames per audio block: 5.8 ms at 44.1 kHz */
#define APP_SYSEX                        /* our frames from the host (host/conductor/PROTOCOL.md) */

/* nothing of ours is saved: the slots live on the host. The platform's panel calibration is the only object;
 * Bluetooth's record is kept by the app (APP_BLE_STORE), in RAM only (app.c app_ble_load / app_ble_save) */
enum { OBJ_PANEL, OBJ_NOBJ };
#define APP_STORE_MAP {0xDC000u}
#define APP_BLE_STORE

void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);
int app_sysex(const uint8_t *p, uint32_t n);

/* host simulator checks: host, page, sel, total, stateN (slot N's state), chimes (figures played) */
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
