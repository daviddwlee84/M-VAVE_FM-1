/* SPDX-License-Identifier: GPL-3.0-or-later */
/* stats: WCA singles and averages (stats.c). */
#pragma once
#include <stdint.h>

#define ST_DNF 0x7FFFFFFF
#define ST_NONE (-1)
#define ST_AVG_MAX 100
enum { ST_OK, ST_PLUS2, ST_DNF_PEN };

int32_t st_with_penalty(int32_t ms, int pen);
int32_t st_mean(const int32_t *t, int n);      /* mo3: a DNF makes it a DNF */
int32_t st_average(const int32_t *t, int n);   /* ao5 .. ao100 (n of them, newest or not, any order) */
int32_t st_best(const int32_t *t, int n);
void st_format(int32_t ms, char *out);         /* "9.87", "1:23.45", "DNF" (truncated to 0.01 s) */
int32_t st_round_cs(int32_t ms);               /* an average, rounded to 0.01 s before st_format */
