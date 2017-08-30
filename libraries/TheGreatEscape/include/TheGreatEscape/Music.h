#ifndef MUSIC_H
#define MUSIC_H

#include <stdint.h>

extern const uint8_t music_channel0_data[80 * 8 + 1];
extern const uint8_t music_channel1_data[80 * 8 + 1];

uint16_t frequency_for_semitone(uint8_t semitone, uint8_t *beep);

#endif /* MUSIC_H */
