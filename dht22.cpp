#include "dht22.h"
#include <pigpiod_if2.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <cstdio>

#define WDOG_MS       200
#define MAX_HI        128
#define BIT_THRESH_US  50

typedef struct {
    volatile int done;
    int          last_level;
    uint32_t     last_tick;
    volatile int hi_n;
    uint32_t     hi_us[MAX_HI];
} ctx_t;

static ctx_t g_ctx;

static void cb(int pi, unsigned gpio,
               unsigned level, uint32_t tick, void *user)
{
    (void)pi; (void)gpio; (void)user;
    ctx_t *c = &g_ctx;
    if (level == PI_TIMEOUT) { c->done = 1; return; }
    if (c->last_tick == 0) {
        c->last_tick  = tick;
        c->last_level = (int)level;
        return;
    }
    uint32_t dt = tick - c->last_tick;
    c->last_tick = tick;
    if (c->last_level == 1 && level == 0) {
        if (c->hi_n < MAX_HI) c->hi_us[c->hi_n++] = dt;
        else c->done = 1;
    }
    c->last_level = (int)level;
}

static int decode(const uint32_t *hi, int n, float *t, float *h)
{
    if (n < 40) return -10;

    int start = (n >= 41) ? 1 : 0;

    uint8_t b[5] = {0};
    for (int i = 0; i < 40; i++) {
        b[i / 8] <<= 1;
        if (hi[start + i] > BIT_THRESH_US)
            b[i / 8] |= 1;
    }

    uint8_t sum = b[0] + b[1] + b[2] + b[3];
    fprintf(stderr, "b=%02X %02X %02X %02X %02X sum=%02X\n",
            b[0], b[1], b[2], b[3], b[4], sum);

    if (sum == b[4]) {
        *h = (float)b[0] + (float)b[1] / 10.0f;
        *t = (float)(b[2] & 0x7F) + (float)b[3] / 10.0f;
        if (b[2] & 0x80) *t = -*t;
        fprintf(stderr, "OK h=%.1f t=%.1f\n", *h, *t);
        return 0;
    }

    if (((sum ^ 0x80) & 0xFF) == b[4]) {
        b[0] = b[0] ^ 0x80;
        sum  = b[0] + b[1] + b[2] + b[3];
        fprintf(stderr, "MSB fixed b[0]=%02X sum=%02X\n", b[0], sum);
        if (sum == b[4]) {
            *h = (float)b[0] + (float)b[1] / 10.0f;
            *t = (float)(b[2] & 0x7F) + (float)b[3] / 10.0f;
            if (b[2] & 0x80) *t = -*t;
            fprintf(stderr, "OK h=%.1f t=%.1f\n", *h, *t);
            return 0;
        }
    }

    return -11;
}

int dht22_init(int gpio_bcm, int pi_handle)
{
    set_mode(pi_handle, gpio_bcm, PI_OUTPUT);
    gpio_write(pi_handle, gpio_bcm, 1);
    usleep(1000000);
    return 0;
}

int dht22_read(int pi_handle, int gpio_bcm,
               float *t_c, float *h)
{
    if (!t_c || !h) return -101;

    memset(&g_ctx, 0, sizeof(g_ctx));

    int cb_id = callback_ex(pi_handle, gpio_bcm,
                            EITHER_EDGE, cb, NULL);
    set_watchdog(pi_handle, gpio_bcm, WDOG_MS);

    set_mode(pi_handle, gpio_bcm, PI_OUTPUT);
    gpio_write(pi_handle, gpio_bcm, 1);
    usleep(100000);
    gpio_write(pi_handle, gpio_bcm, 0);
    usleep(20000);
    gpio_write(pi_handle, gpio_bcm, 1);
    usleep(40);
    set_mode(pi_handle, gpio_bcm, PI_INPUT);

    uint32_t t0 = get_current_tick(pi_handle);
    while (!g_ctx.done &&
           (get_current_tick(pi_handle) - t0) < 600000)
        usleep(500);

    set_watchdog(pi_handle, gpio_bcm, 0);
    callback_cancel(cb_id);

    fprintf(stderr, "hi_n=%d\n", g_ctx.hi_n);
    if (g_ctx.hi_n < 40) return -10;
    return decode(g_ctx.hi_us, g_ctx.hi_n, t_c, h);
}
