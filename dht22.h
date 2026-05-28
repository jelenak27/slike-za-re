#ifndef DHT22_H
#define DHT22_H

int dht22_init(int gpio_bcm, int pi_handle);
int dht22_read(int pi_handle, int gpio_bcm,
               float *t_c, float *h);

#endif
