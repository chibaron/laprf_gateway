#ifndef __LAPRF_H__
#define __LAPRF_H__

void laprf_init(void);
int laprf_protocol(uint8_t *buf, char *message);
int laprf_data(uint8_t ch, char *message);

#endif

