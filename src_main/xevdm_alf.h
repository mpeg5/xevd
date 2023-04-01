/* The copyright in this software is being made available under the BSD
   License, included below. This software may be subject to contributor and
   other third party rights, including patent rights, and no such rights are
   granted under this license.

   Copyright (c) 2020, Samsung Electronics Co., Ltd.
   All Rights Reserved. */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

   - Neither the name of the copyright owner, nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _XEVDM_ALF_H_
#define _XEVDM_ALF_H_


#include "xevdm_def.h"

#define CHECK(a,b) assert((!(a)) && (b));

#define NUM_BITS                             10
#define CLASSIFICATION_BLK_SIZE              32  //non-normative, local buffer size
#define FIXED_FILTER_NUM                     64

#define MAX_NUM_ALF_CLASSES                  25
#define MAX_NUM_ALF_LUMA_COEFF               13
#define MAX_NUM_ALF_CHROMA_COEFF             7
#define MAX_ALF_FILTER_LENGTH                7
#define MAX_NUM_ALF_COEFF                    (MAX_ALF_FILTER_LENGTH * MAX_ALF_FILTER_LENGTH / 2 + 1)
#define ALF_FIXED_FILTER_NUM                 16

typedef u8 ALF_CLASSIFIER;

typedef struct _ADAPTIVE_LOOP_FILTER ADAPTIVE_LOOP_FILTER;
typedef struct _ALF_FILTER_SHAPE ALF_FILTER_SHAPE;
typedef struct _ALF_SLICE_PARAM ALF_SLICE_PARAM;

typedef struct AREA
{
    int x;
    int y;
    int width;
    int height;
} AREA;

typedef enum _ALF_FILTER_TYPE
{
    ALF_FILTER_5,
    ALF_FILTER_7,
    ALF_NUM_OF_FILTER_TYPES
} ALF_FILTER_TYPE;

enum DIRECTION
{
    HOR,
    VER,
    DIAG0,
    DIAG1,
    NUM_DIRECTIONS
};

typedef struct CLIP_RANGE
{
    int min;
    int max;
    int bd;
    int n;
} CLIP_RANGE;

typedef struct CLIP_RNAGES
{
    CLIP_RANGE comp[N_C]; ///< the bit depth as indicated in the SPS
    BOOL used;
    BOOL chroma;
} CLIP_RNAGES;

typedef enum _ChannelType
{
    CHANNEL_TYPE_LUMA = 0,
    CHANNEL_TYPE_CHROMA = 1,
    MAX_NUM_CHANNEL_TYPE = 2
} CHANNEL_TYPE;

typedef enum _ComponentID
{
    COMPONENT_Y = 0,
    COMPONENT_Cb = 1,
    COMPONENT_Cr = 2,
    MAX_NUM_COMPONENT = 3,
    MAX_NUM_TBLOCKS = MAX_NUM_COMPONENT
} COMPONENT_ID;

static __inline int clip_pel(const int a, const CLIP_RANGE clip_range)
{
    return XEVD_CLIP3(clip_range.min, clip_range.max, a);
}

typedef struct CODING_STRUCTURE
{
    void    * ctx;
    XEVD_PIC * pic;

    int temp_stride; //to pass strides easily
    int pic_stride;
} CODING_STRUCTURE;

static const int pattern5[25] =
{
    0,
    1,  2,  3,
    4,  5,  6,  5,  4,
    3,  2,  1,
    0
};

static const int pattern7[25] =
{
    0,
    1,  2,  3,
    4,  5,  6,  7,  8,
    9, 10, 11, 12, 11, 10, 9,
    8,  7,  6,  5,  4,
    3,  2,  1,
    0
};

static const int weights5[14] =
{
    2,
    2, 2, 2,
    2, 2, 1, 1
};

static const int weights7[14] =
{
    2,
    2,  2,  2,
    2,  2,  2,  2,  2,
    2,  2,  2,  1,  1
};

static const int golombIdx5[14] =
{
    0,
    0, 1, 0,
    0, 1
};

static const int golombIdx7[14] =
{
    0,
    0, 1, 0,
    0, 1, 2, 1, 0,
    0, 1, 2
};

static const int pattern_to_large_filter5[13] =
{
    0,
    0, 1, 0,
    0, 2, 3, 4, 0,
    0, 5, 6, 7
};

static const int pattern_to_large_filter7[13] =
{
    1,
    2, 3, 4,
    5, 6, 7, 8, 9,
    10,11,12,13
};

static const int alf_fixed_filter_coef[FIXED_FILTER_NUM][13] =
{
    { 0,   2,   7, -12,  -4, -11,  -2,  31,  -9,   6,  -4,  30, 444 - (1 << (NUM_BITS - 1)) },
    { -26,   4,  17,  22,  -7,  19,  40,  47,  49, -28,  35,  48,  72 - (1 << (NUM_BITS - 1)) },
    { -24,  -8,  30,  64, -13,  18,  18,  27,  80,   0,  31,  19,  28 - (1 << (NUM_BITS - 1)) },
    { -4, -14,  44, 100,  -7,   6,  -4,   8,  90,  26,  26, -12,  -6 - (1 << (NUM_BITS - 1)) },
    { -17,  -9,  23,  -3, -15,  20,  53,  48,  16, -25,  42,  66, 114 - (1 << (NUM_BITS - 1)) },
    { -12,  -2,   1, -19,  -5,   8,  66,  80,  -2, -25,  20,  78, 136 - (1 << (NUM_BITS - 1)) },
    { 2,   8, -23, -14,  -3, -23,  64,  86,  35, -17,  -4,  79, 132 - (1 << (NUM_BITS - 1)) },
    { 12,   4, -39,  -7,   1, -20,  78,  13,  -8,  11, -42,  98, 310 - (1 << (NUM_BITS - 1)) },
    { 0,   3,  -4,   0,   2,  -7,   6,   0,   0,   3,  -8,  11, 500 - (1 << (NUM_BITS - 1)) },
    { 4,  -7, -25, -19,  -9,   8,  86,  65, -14,  -7,  -7,  97, 168 - (1 << (NUM_BITS - 1)) },
    { 3,   3,   2, -30,   6, -34,  43,  71, -10,   4, -23,  77, 288 - (1 << (NUM_BITS - 1)) },
    { 12,  -3, -34, -14,  -5, -14,  88,  28, -12,   8, -34, 112, 248 - (1 << (NUM_BITS - 1)) },
    { -1,   6,   8, -29,   7, -27,  15,  60,  -4,   6, -21,  39, 394 - (1 << (NUM_BITS - 1)) },
    { 8,  -1,  -7, -22,   5, -41,  63,  40, -13,   7, -28, 105, 280 - (1 << (NUM_BITS - 1)) },
    { 1,   3,  -5,  -1,   1, -10,  12,  -1,   0,   3,  -9,  19, 486 - (1 << (NUM_BITS - 1)) },
    { 10,  -1, -23, -14,  -3, -27,  78,  24, -14,   8, -28, 102, 288 - (1 << (NUM_BITS - 1)) },
    { 0,   0,  -1,   0,   0,  -1,   1,   0,   0,   0,   0,   1, 512 - (1 << (NUM_BITS - 1)) },
    { 7,   3, -19,  -7,   2, -27,  51,   8,  -6,   7, -24,  64, 394 - (1 << (NUM_BITS - 1)) },
    { 11, -10, -22, -22, -11, -12,  87,  49, -20,   4, -16, 108, 220 - (1 << (NUM_BITS - 1)) },
    { 17,  -2, -69,  -4,  -4,  22, 106,  31,  -7,  13, -63, 121, 190 - (1 << (NUM_BITS - 1)) },
    { 1,   4,  -1,  -7,   5, -26,  24,   0,   1,   3, -18,  51, 438 - (1 << (NUM_BITS - 1)) },
    { 3,   5, -10,  -2,   4, -17,  17,   1,  -2,   6, -16,  27, 480 - (1 << (NUM_BITS - 1)) },
    { 9,   2, -23,  -5,   6, -45,  90, -22,   1,   7, -39, 121, 308 - (1 << (NUM_BITS - 1)) },
    { 4,   5, -15,  -2,   4, -22,  34,  -2,  -2,   7, -22,  48, 438 - (1 << (NUM_BITS - 1)) },
    { 6,   8, -22,  -3,   4, -32,  57,  -3,  -4,  11, -43, 102, 350 - (1 << (NUM_BITS - 1)) },
    { 2,   5, -11,   1,  12, -46,  64, -32,   7,   4, -31,  85, 392 - (1 << (NUM_BITS - 1)) },
    { 5,   5, -12,  -8,   6, -48,  74, -13,  -1,   7, -41, 129, 306 - (1 << (NUM_BITS - 1)) },
    { 0,   1,  -1,   0,   1,  -3,   2,   0,   0,   1,  -3,   4, 508 - (1 << (NUM_BITS - 1)) },
    { -1,   3,  16, -42,   6, -16,   2, 105,   6,   6, -31,  43, 318 - (1 << (NUM_BITS - 1)) },
    { 7,   8, -27,  -4,  -4, -23,  46,  79,  64,  -8, -13,  68, 126 - (1 << (NUM_BITS - 1)) },
    { -3,  12,  -4, -34,  14,  -6, -24, 179,  56,   2, -48,  15, 194 - (1 << (NUM_BITS - 1)) },
    { 8,   0, -16, -25,  -1, -29,  68,  84,   3,  -3, -18,  94, 182 - (1 << (NUM_BITS - 1)) },
    { -3,  -1,  22, -32,   2, -20,   5,  89,   0,   9, -18,  40, 326 - (1 << (NUM_BITS - 1)) },
    { 14,   6, -51,  22, -10, -22,  36,  75, 106,  -4, -11,  56,  78 - (1 << (NUM_BITS - 1)) },
    { 1,  38, -59,  14,   8, -44, -18, 156,  80,  -1, -42,  29, 188 - (1 << (NUM_BITS - 1)) },
    { -1,   2,   4,  -9,   3, -13,   7,  17,  -4,   2,  -6,  17, 474 - (1 << (NUM_BITS - 1)) },
    { 11,  -2, -15, -36,   2, -32,  67,  89, -19,  -1, -14, 103, 206 - (1 << (NUM_BITS - 1)) },
    { -1,  10,   3, -28,   7, -27,   7, 117,  34,   1, -35,  51, 234 - (1 << (NUM_BITS - 1)) },
    { 3,   3,   4, -18,   6, -40,  36,  18,  -8,   7, -25,  86, 368 - (1 << (NUM_BITS - 1)) },
    { -1,   3,   9, -18,   5, -26,  12,  37, -11,   3,  -7,  32, 436 - (1 << (NUM_BITS - 1)) },
    { 0,  17, -38,  -9, -28, -17,  25,  48, 103,   2,  40,  69,  88 - (1 << (NUM_BITS - 1)) },
    { 6,   4, -11, -20,   5, -32,  51,  77,  17,   0, -25,  84, 200 - (1 << (NUM_BITS - 1)) },
    { 0,  -5,  28, -24,  -1, -22,  18,  -9,  17,  -1, -12, 107, 320 - (1 << (NUM_BITS - 1)) },
    { -10,  -4,  17, -30, -29,  31,  40,  49,  44, -26,  67,  67,  80 - (1 << (NUM_BITS - 1)) },
    { -30, -12,  39,  15, -21,  32,  29,  26,  71,  20,  43,  28,  32 - (1 << (NUM_BITS - 1)) },
    { 6,  -7,  -7, -34, -21,  15,  53,  60,  12, -26,  45,  89, 142 - (1 << (NUM_BITS - 1)) },
    { -1,  -5,  59, -58,  -8, -30,   2,  17,  34,  -7,  25, 111, 234 - (1 << (NUM_BITS - 1)) },
    { 7,   1,  -7, -20,  -9, -22,  48,  27,  -4,  -6,   0, 107, 268 - (1 << (NUM_BITS - 1)) },
    { -2,  22,  29, -70,  -4, -28,   2,  19,  94, -40,  14, 110, 220 - (1 << (NUM_BITS - 1)) },
    { 13,   0, -22, -27, -11, -15,  66,  44,  -7,  -5, -10, 121, 218 - (1 << (NUM_BITS - 1)) },
    { 10,   6, -22, -14,  -2, -33,  68,  15,  -9,   5, -35, 135, 264 - (1 << (NUM_BITS - 1)) },
    { 2,  11,   4, -32,  -3, -20,  23,  18,  17,  -1, -28,  88, 354 - (1 << (NUM_BITS - 1)) },
    { 0,   3,  -2,  -1,   3, -16,  16,  -3,   0,   2, -12,  35, 462 - (1 << (NUM_BITS - 1)) },
    { 1,   6,  -6,  -3,  10, -51,  70, -31,   5,   6, -42, 125, 332 - (1 << (NUM_BITS - 1)) },
    { 5,  -7,  61, -71, -36,  -6,  -2,  15,  57,  18,  14, 108, 200 - (1 << (NUM_BITS - 1)) },
    { 9,   1,  35, -70, -73,  28,  13,   1,  96,  40,  36,  80, 120 - (1 << (NUM_BITS - 1)) },
    { 11,  -7,  33, -72, -78,  48,  33,  37,  35,   7,  85,  76,  96 - (1 << (NUM_BITS - 1)) },
    { 4,  15,   1, -26, -24, -19,  32,  29,  -8,  -6,  21, 125, 224 - (1 << (NUM_BITS - 1)) },
    { 11,   8,  14, -57, -63,  21,  34,  51,   7,  -3,  69,  89, 150 - (1 << (NUM_BITS - 1)) },
    { 7,  16,  -7, -31, -38,  -5,  41,  44, -11, -10,  45, 109, 192 - (1 << (NUM_BITS - 1)) },
    { 5,  16,  16, -46, -55,   3,  22,  32,  13,   0,  48, 107, 190 - (1 << (NUM_BITS - 1)) },
    { 2,  10,  -3, -14,  -9, -28,  39,  15, -10,  -5,  -1, 123, 274 - (1 << (NUM_BITS - 1)) },
    { 3,  11,  11, -27, -17, -24,  18,  22,   2,   4,   3, 100, 300 - (1 << (NUM_BITS - 1)) },
    { 0,   1,   7,  -9,   3, -20,  16,   3,  -2,   0,  -9,  61, 410 - (1 << (NUM_BITS - 1)) },
};

static const int alf_class_to_filter_mapping[MAX_NUM_ALF_CLASSES][ALF_FIXED_FILTER_NUM] =
{
    { 0,   1,   2,   3,   4,   5,   6,   7,   9,  19,  32,  41,  42,  44,  46,  63 },
    { 0,   1,   2,   4,   5,   6,   7,   9,  11,  16,  25,  27,  28,  31,  32,  47 },
    { 5,   7,   9,  11,  12,  14,  15,  16,  17,  18,  19,  21,  22,  27,  31,  35 },
    { 7,   8,   9,  11,  14,  15,  16,  17,  18,  19,  22,  23,  24,  25,  35,  36 },
    { 7,   8,  11,  13,  14,  15,  16,  17,  19,  20,  21,  22,  23,  24,  25,  27 },
    { 1,   2,   3,   4,   6,  19,  29,  30,  33,  34,  37,  41,  42,  44,  47,  54 },
    { 1,   2,   3,   4,   6,  11,  28,  29,  30,  31,  32,  33,  34,  37,  47,  63 },
    { 0,   1,   4,   6,  10,  12,  13,  19,  28,  29,  31,  32,  34,  35,  36,  37 },
    { 6,   9,  10,  12,  13,  16,  19,  20,  28,  31,  35,  36,  37,  38,  39,  52 },
    { 7,   8,  10,  11,  12,  13,  19,  23,  25,  27,  28,  31,  35,  36,  38,  39 },
    { 1,   2,   3,   5,  29,  30,  33,  34,  40,  43,  44,  46,  54,  55,  59,  62 },
    { 1,   2,   3,   4,  29,  30,  31,  33,  34,  37,  40,  41,  43,  44,  59,  61 },
    { 0,   1,   3,   6,  19,  28,  29,  30,  31,  32,  33,  34,  37,  41,  44,  61 },
    { 1,   6,  10,  13,  19,  28,  29,  30,  32,  33,  34,  35,  37,  41,  48,  52 },
    { 0,   5,   6,  10,  19,  27,  28,  29,  32,  37,  38,  40,  41,  47,  49,  58 },
    { 1,   2,   3,   4,  11,  29,  33,  42,  43,  44,  45,  46,  48,  55,  56,  59 },
    { 0,   1,   2,   5,   7,   9,  29,  40,  43,  44,  45,  47,  48,  56,  59,  63 },
    { 0,   4,   5,   9,  14,  19,  26,  35,  36,  43,  45,  47,  48,  49,  50,  51 },
    { 9,  11,  12,  14,  16,  19,  20,  24,  26,  36,  38,  47,  49,  50,  51,  53 },
    { 7,   8,  13,  14,  20,  21,  24,  25,  26,  27,  35,  38,  47,  50,  52,  53 },
    { 1,   2,   4,  29,  33,  40,  41,  42,  43,  44,  45,  46,  54,  55,  56,  58 },
    { 2,   4,  32,  40,  42,  43,  44,  45,  46,  54,  55,  56,  58,  59,  60,  62 },
    { 0,  19,  42,  43,  45,  46,  48,  54,  55,  56,  57,  58,  59,  60,  61,  62 },
    { 8,  13,  36,  42,  45,  46,  51,  53,  54,  57,  58,  59,  60,  61,  62,  63 },
    { 8,  13,  20,  27,  36,  38,  42,  46,  52,  53,  56,  57,  59,  61,  62,  63 },
};

static const u8 tb_max[257] = { 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7,
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8 };

typedef struct _ALF_FILTER_SHAPE
{
    int filter_type;
    int filterLength;
    int num_coef;
    int filter_size;
    int pattern[25];
    int weights[14];
    int golombIdx[14];
    int pattern_to_large_filter[13];

} ALF_FILTER_SHAPE;

struct _ALF_SLICE_PARAM
{
    BOOL                is_ctb_alf_on;
    u8                * alf_ctb_flag;
    BOOL                enable_flag[N_C];                                        // alf_slice_enable_flag, alf_chroma_idc
    ALF_FILTER_TYPE     luma_filter_type;                                        // filter_type_flag
    BOOL                chroma_ctb_present_flag;                                 // alf_chroma_ctb_present_flag
    BOOL                chroma_filter_present;
    short               luma_coef[MAX_NUM_ALF_CLASSES * MAX_NUM_ALF_LUMA_COEFF]; // alf_coeff_luma_delta[i][j]
    short               chroma_coef[MAX_NUM_ALF_CHROMA_COEFF];                   // alf_coeff_chroma[i]
    short               filter_coef_delta_idx[MAX_NUM_ALF_CLASSES];              // filter_coeff_delta[i]
    BOOL                filter_coef_flag[MAX_NUM_ALF_CLASSES];                   // filter_coefficient_flag[i]
    int                 num_luma_filters;                                        // number_of_filters_minus1 + 1
    BOOL                coef_delta_flag;                                         // alf_coefficients_delta_flag
    BOOL                coef_delta_pred_mode_flag;                               // coeff_delta_pred_mode_flag
    ALF_FILTER_SHAPE(*filter_shapes)[2];

    int                 fixed_filter_pattern;                                    // 0: no pred from pre-defined filters; 1: all are predicted but could be different values; 2: some predicted and some not
                                                                                    // when ALF_LOWDELAY is 1, fixed_filter_pattern 0: all are predected, fixed_filter_pattern 1: some predicted and some not
    int                 fixed_filter_idx[MAX_NUM_ALF_CLASSES];
    u8                  fixed_filter_usage_flag[MAX_NUM_ALF_CLASSES];
    int                 t_layer;
    BOOL                temporal_alf_flag;                                       // indicate whether reuse previous ALF coefficients
    int                 prev_idx;                                                // index of the reused ALF coefficients
    int                 prev_idx_comp[N_C];
    BOOL                reset_alf_buf_flag;
    BOOL                store2_alf_buf_flag;
    u32                 filter_poc;                                              // store POC value for which filter was produced
    u32                 min_idr_poc;                                             // Minimal of 2 IDR POC available for current coded nalu  (to identify availability of this filter for temp prediction)
    u32                 max_idr_poc;                                             // Max of 2 IDR POC available for current coded nalu  (to identify availability of this filter for temp prediction)
};

struct _ADAPTIVE_LOOP_FILTER
{
    short               coef_final[MAX_NUM_ALF_CLASSES * MAX_NUM_ALF_LUMA_COEFF];
    int                 input_bit_depth[N_C];
    ALF_SLICE_PARAM     ac_alf_line_buf[APS_MAX_NUM];
    u8                  alf_idx_in_scan_order[APS_MAX_NUM];
    u8                  next_free_alf_idx_in_buf;
    u32                 first_idx_poc;
    u32                 last_idr_poc;
    u32                 curr_poc;
    u32                 curr_temp_layer;
    u32                 i_period;
    int                 alf_present_idr;
    int                 alf_idx_idr;
    u8                  ac_alf_line_buf_curr_size;
    pel               * temp_buf, *temp_buf1, *temp_buf2;
    int                 pic_width;
    int                 pic_height;
    int                 max_cu_width;
    int                 max_cu_height;
    int                 max_cu_depth;
    int                 num_ctu_in_widht;
    int                 num_ctu_in_height;
    int                 num_ctu_in_pic;
    ALF_CLASSIFIER   ** classifier;
    ALF_CLASSIFIER   ** classifier_mt;
    int                 chroma_format;
    int                 last_ras_poc;
    BOOL                pending_ras_init;
    u8                * ctu_enable_flag[N_C];
    CLIP_RNAGES         clip_ranges;
    BOOL                strore2_alf_buf_flag;
    BOOL                reset_alf_buf_flag;
    ALF_FILTER_SHAPE    filter_shapes[N_C][2];

    void(*derive_classification_blk)(ALF_CLASSIFIER** classifier, const pel * src_luma, const int src_stride, const AREA * blk, const int shift, int bit_depth);
    void(*filter_5x5_blk)(ALF_CLASSIFIER** classifier, pel * rec_dst, const int dst_stride, const pel * rec_src, const int src_stride, const AREA* blk, const u8 comp_id, short* filter_set, const CLIP_RANGE* clip_range);
    void(*filter_7x7_blk)(ALF_CLASSIFIER** classifier, pel * rec_dst, const int dst_stride, const pel * rec_src, const int src_stride, const AREA* blk, const u8 comp_id, short* filter_set, const CLIP_RANGE* clip_range);
};

ADAPTIVE_LOOP_FILTER* new_alf(int bit_depth);
void delete_alf(ADAPTIVE_LOOP_FILTER* alf);

int xevd_alf_create(ADAPTIVE_LOOP_FILTER * alf, const int pic_width, const int pic_height, const int max_cu_width, const int max_cu_height, const int max_cu_depth, int chroma_format_idc, int bit_depth);
void xevd_alf_destroy(ADAPTIVE_LOOP_FILTER * alf);
void xevd_alf_init(ADAPTIVE_LOOP_FILTER * alf, int bit_depth);
void xevd_alf_init_filter_shape(void * filter_shape, int size);

int call_dec_alf_process_aps(ADAPTIVE_LOOP_FILTER* alf, XEVD_CTX * ctx, XEVD_PIC * pic);
void store_dec_aps_to_buffer(XEVD_CTX * ctx);
#endif

