/* --------------------------------------------------------------------------
 *    Name: SSBuffer
 * Purpose: SharedSoundBuffer interface
 *  Author: David Thomas
 * ----------------------------------------------------------------------- */

#ifndef SSBUFFER_H
#define SSBUFFER_H

typedef unsigned int ssndbuf_t;

#define SharedSoundBuffer_OpenStream            (0x55FC0)
#define SharedSoundBuffer_CloseStream           (0x55FC1)
#define SharedSoundBuffer_AddBlock              (0x55FC2)
#define SharedSoundBuffer_PollWord              (0x55FC3)
#define SharedSoundBuffer_Volume                (0x55FC4)
#define SharedSoundBuffer_SampleRate            (0x55FC5)
#define SharedSoundBuffer_ReturnSSHandle        (0x55FC6)
#define SharedSoundBuffer_SetBuffer             (0x55FC7)
#define SharedSoundBuffer_BufferStats           (0x55FC8)
#define SharedSoundBuffer_Pause                 (0x55FC9)
#define SharedSoundBuffer_Mark                  (0x55FCA)
#define SharedSoundBuffer_StreamEnd             (0x55FCB)
#define SharedSoundBuffer_Flush                 (0x55FCC)
#define SharedSoundBuffer_BlockFilled           (0x55FCD)
#define SharedSoundBuffer_ReturnStreamHandle    (0x55FCE)

#endif /* SSBUFFER_H */
