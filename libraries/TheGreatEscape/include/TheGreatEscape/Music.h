#ifndef MUSIC_H
#define MUSIC_H

#include <stdint.h>

extern const uint8_t music_channel0_data[80 * 8 + 1];
extern const uint8_t music_channel1_data[80 * 8 + 1];

uint16_t get_tuning(uint8_t A, uint8_t *L);

#endif /* MUSIC_H */
