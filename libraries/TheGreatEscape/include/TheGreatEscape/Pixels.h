/**
 * Pixels.h
 *
 * This file is part of "The Great Escape in C".
 *
 * This project recreates the 48K ZX Spectrum version of the prison escape
 * game "The Great Escape" in portable C code. It is free software provided
 * without warranty in the interests of education and software preservation.
 *
 * "The Great Escape" was created by Denton Designs and published in 1986 by
 * Ocean Software Limited.
 *
 * The original game is copyright (c) 1986 Ocean Software Ltd.
 * The original game design is copyright (c) 1986 Denton Designs Ltd.
 * The recreated version is copyright (c) 2012-2019 David Thomas
 */

#ifndef PIXELS_H
#define PIXELS_H

/* ----------------------------------------------------------------------- */

/* Macros to use when defining graphics in code. */

#define ________ 0
#define _______X 1
#define ______X_ 2
#define ______XX 3
#define _____X__ 4
#define _____X_X 5
#define _____XX_ 6
#define _____XXX 7
#define ____X___ 8
#define ____X__X 9
#define ____X_X_ 10
#define ____X_XX 11
#define ____XX__ 12
#define ____XX_X 13
#define ____XXX_ 14
#define ____XXXX 15
#define ___X____ 16
#define ___X___X 17
#define ___X__X_ 18
#define ___X__XX 19
#define ___X_X__ 20
#define ___X_X_X 21
#define ___X_XX_ 22
#define ___X_XXX 23
#define ___XX___ 24
#define ___XX__X 25
#define ___XX_X_ 26
#define ___XX_XX 27
#define ___XXX__ 28
#define ___XXX_X 29
#define ___XXXX_ 30
#define ___XXXXX 31
#define __X_____ 32
#define __X____X 33
#define __X___X_ 34
#define __X___XX 35
#define __X__X__ 36
#define __X__X_X 37
#define __X__XX_ 38
#define __X__XXX 39
#define __X_X___ 40
#define __X_X__X 41
#define __X_X_X_ 42
#define __X_X_XX 43
#define __X_XX__ 44
#define __X_XX_X 45
#define __X_XXX_ 46
#define __X_XXXX 47
#define __XX____ 48
#define __XX___X 49
#define __XX__X_ 50
#define __XX__XX 51
#define __XX_X__ 52
#define __XX_X_X 53
#define __XX_XX_ 54
#define __XX_XXX 55
#define __XXX___ 56
#define __XXX__X 57
#define __XXX_X_ 58
#define __XXX_XX 59
#define __XXXX__ 60
#define __XXXX_X 61
#define __XXXXX_ 62
#define __XXXXXX 63
#define _X______ 64
#define _X_____X 65
#define _X____X_ 66
#define _X____XX 67
#define _X___X__ 68
#define _X___X_X 69
#define _X___XX_ 70
#define _X___XXX 71
#define _X__X___ 72
#define _X__X__X 73
#define _X__X_X_ 74
#define _X__X_XX 75
#define _X__XX__ 76
#define _X__XX_X 77
#define _X__XXX_ 78
#define _X__XXXX 79
#define _X_X____ 80
#define _X_X___X 81
#define _X_X__X_ 82
#define _X_X__XX 83
#define _X_X_X__ 84
#define _X_X_X_X 85
#define _X_X_XX_ 86
#define _X_X_XXX 87
#define _X_XX___ 88
#define _X_XX__X 89
#define _X_XX_X_ 90
#define _X_XX_XX 91
#define _X_XXX__ 92
#define _X_XXX_X 93
#define _X_XXXX_ 94
#define _X_XXXXX 95
#define _XX_____ 96
#define _XX____X 97
#define _XX___X_ 98
#define _XX___XX 99
#define _XX__X__ 100
#define _XX__X_X 101
#define _XX__XX_ 102
#define _XX__XXX 103
#define _XX_X___ 104
#define _XX_X__X 105
#define _XX_X_X_ 106
#define _XX_X_XX 107
#define _XX_XX__ 108
#define _XX_XX_X 109
#define _XX_XXX_ 110
#define _XX_XXXX 111
#define _XXX____ 112
#define _XXX___X 113
#define _XXX__X_ 114
#define _XXX__XX 115
#define _XXX_X__ 116
#define _XXX_X_X 117
#define _XXX_XX_ 118
#define _XXX_XXX 119
#define _XXXX___ 120
#define _XXXX__X 121
#define _XXXX_X_ 122
#define _XXXX_XX 123
#define _XXXXX__ 124
#define _XXXXX_X 125
#define _XXXXXX_ 126
#define _XXXXXXX 127
#define X_______ 128
#define X______X 129
#define X_____X_ 130
#define X_____XX 131
#define X____X__ 132
#define X____X_X 133
#define X____XX_ 134
#define X____XXX 135
#define X___X___ 136
#define X___X__X 137
#define X___X_X_ 138
#define X___X_XX 139
#define X___XX__ 140
#define X___XX_X 141
#define X___XXX_ 142
#define X___XXXX 143
#define X__X____ 144
#define X__X___X 145
#define X__X__X_ 146
#define X__X__XX 147
#define X__X_X__ 148
#define X__X_X_X 149
#define X__X_XX_ 150
#define X__X_XXX 151
#define X__XX___ 152
#define X__XX__X 153
#define X__XX_X_ 154
#define X__XX_XX 155
#define X__XXX__ 156
#define X__XXX_X 157
#define X__XXXX_ 158
#define X__XXXXX 159
#define X_X_____ 160
#define X_X____X 161
#define X_X___X_ 162
#define X_X___XX 163
#define X_X__X__ 164
#define X_X__X_X 165
#define X_X__XX_ 166
#define X_X__XXX 167
#define X_X_X___ 168
#define X_X_X__X 169
#define X_X_X_X_ 170
#define X_X_X_XX 171
#define X_X_XX__ 172
#define X_X_XX_X 173
#define X_X_XXX_ 174
#define X_X_XXXX 175
#define X_XX____ 176
#define X_XX___X 177
#define X_XX__X_ 178
#define X_XX__XX 179
#define X_XX_X__ 180
#define X_XX_X_X 181
#define X_XX_XX_ 182
#define X_XX_XXX 183
#define X_XXX___ 184
#define X_XXX__X 185
#define X_XXX_X_ 186
#define X_XXX_XX 187
#define X_XXXX__ 188
#define X_XXXX_X 189
#define X_XXXXX_ 190
#define X_XXXXXX 191
#define XX______ 192
#define XX_____X 193
#define XX____X_ 194
#define XX____XX 195
#define XX___X__ 196
#define XX___X_X 197
#define XX___XX_ 198
#define XX___XXX 199
#define XX__X___ 200
#define XX__X__X 201
#define XX__X_X_ 202
#define XX__X_XX 203
#define XX__XX__ 204
#define XX__XX_X 205
#define XX__XXX_ 206
#define XX__XXXX 207
#define XX_X____ 208
#define XX_X___X 209
#define XX_X__X_ 210
#define XX_X__XX 211
#define XX_X_X__ 212
#define XX_X_X_X 213
#define XX_X_XX_ 214
#define XX_X_XXX 215
#define XX_XX___ 216
#define XX_XX__X 217
#define XX_XX_X_ 218
#define XX_XX_XX 219
#define XX_XXX__ 220
#define XX_XXX_X 221
#define XX_XXXX_ 222
#define XX_XXXXX 223
#define XXX_____ 224
#define XXX____X 225
#define XXX___X_ 226
#define XXX___XX 227
#define XXX__X__ 228
#define XXX__X_X 229
#define XXX__XX_ 230
#define XXX__XXX 231
#define XXX_X___ 232
#define XXX_X__X 233
#define XXX_X_X_ 234
#define XXX_X_XX 235
#define XXX_XX__ 236
#define XXX_XX_X 237
#define XXX_XXX_ 238
#define XXX_XXXX 239
#define XXXX____ 240
#define XXXX___X 241
#define XXXX__X_ 242
#define XXXX__XX 243
#define XXXX_X__ 244
#define XXXX_X_X 245
#define XXXX_XX_ 246
#define XXXX_XXX 247
#define XXXXX___ 248
#define XXXXX__X 249
#define XXXXX_X_ 250
#define XXXXX_XX 251
#define XXXXXX__ 252
#define XXXXXX_X 253
#define XXXXXXX_ 254
#define XXXXXXXX 255

/* ----------------------------------------------------------------------- */

#endif /* PIXELS_H */

// vim: ts=8 sts=2 sw=2 et
