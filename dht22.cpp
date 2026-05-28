#include "dht22.h"
#include <pigpiod_if2.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define WDOG_MS        200
#define MAX_HI         120
#define BIT_THRESH_US   50

typedef struct {
    volatile int done;
    int          last_level;
    uint32_t     last_tick;
    volatile int hi_n;
    uint32_t     hi_us[MAX_HI];
} ctx_t;

static void cb(int pi, unsigned gpio,
               unsigned level, uint32_t tick, void *user)
{
    (void)pi; (void)gpio;
    ctx_t *c = (ctx_t *)user;

    if (level == PI_TIMEOUT) { c->done = 1; return; }

    if (c->last_tick == 0) {
        c->last_tick  = tick;
        c->last_level = (int)level;
        return;
    }

    uint32_t dt  = tick - c->last_tick;
    c->last_tick  = tick;

    if (c->last_level == 1 && level == 0) {
        if (c->hi_n < MAX_HI)
            c->hi_us[c->hi_n++] = dt;
        else
            c->done = 1;
    }
    c->last_level = (int)level;
}

static int decode(const uint32_t *hi, int n,
                  float *t, float *h)
{
    if (n < 40) return -10;

    int     start = n - 40;
    uint8_t b[5]  = {0};

    for (int i = 0; i < 40; i++) {
        b[i / 8] <<= 1;
        if (hi[start + i] > BIT_THRESH_US)
            b[i / 8] |= 1;
    }

    if ((uint8_t)(b[0] + b[1] + b[2] + b[3]) != b[4])
        return -11;

    *h = (float)((b[0] << 8) | b[1]) / 10.0f;
    *t = (float)(((b[2] & 0x7F) << 8) | b[3]) / 10.0f;
    if (b[2] & 0x80) *t = -*t;

    return 0;
}

int dht22_init(int gpio_bcm, int pi_handle)
{
    set_mode(pi_handle, gpio_bcm, PI_OUTPUT);
    gpio_write(pi_handle, gpio_bcm, 1);
    usleep(50000);
    return 0;
}

int dht22_read(int pi_handle, int gpio_bcm,
               float *t_c, float *h)
{
    if (!t_c || !h) return -101;

    ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    int cb_id = callback_ex(pi_handle, gpio_bcm,
                            EITHER_EDGE, cb, &ctx);
    set_watchdog(pi_handle, gpio_bcm, WDOG_MS);

    /* start signal */
    set_mode(pi_handle, gpio_bcm, PI_OUTPUT);
    gpio_write(pi_handle, gpio_bcm, 1);
    usleep(50000);
    gpio_write(pi_handle, gpio_bcm, 0);
    usleep(18000);
    gpio_write(pi_handle, gpio_bcm, 1);
    usleep(40);
    set_mode(pi_handle, gpio_bcm, PI_INPUT);

    /* cekaj odgovor max 500 ms */
    uint32_t t0 = get_current_tick(pi_handle);
    while (!ctx.done &&
           (get_current_tick(pi_handle) - t0) < 500000)
        usleep(1000);

    set_watchdog(pi_handle, gpio_bcm, 0);
    callback_cancel(cb_id);

    return decode(ctx.hi_us, ctx.hi_n, t_c, h);
}
