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


#include "../xevd_tbl.h"
#include "../xevdm_def.h"
#include "xevdm_mc_sse.h"
#include <assert.h>

#if X86_SSE


#define MAC_8TAP(c, r0, r1, r2, r3, r4, r5, r6, r7) \
    ((c)[0]*(r0)+(c)[1]*(r1)+(c)[2]*(r2)+(c)[3]*(r3)+(c)[4]*(r4)+\
    (c)[5]*(r5)+(c)[6]*(r6)+(c)[7]*(r7))
#define MAC_8TAP_N0(c, r0, r1, r2, r3, r4, r5, r6, r7) \
    ((MAC_8TAP(c, r0, r1, r2, r3, r4, r5, r6, r7) + MAC_ADD_N0) >> MAC_SFT_N0)
#define MAC_8TAP_0N(c, r0, r1, r2, r3, r4, r5, r6, r7) \
    ((MAC_8TAP(c, r0, r1, r2, r3, r4, r5, r6, r7) + MAC_ADD_0N) >> MAC_SFT_0N)

#define MAC_8TAP_NN_S1(c, r0, r1, r2, r3, r4, r5, r6, r7, offset, shift) \
    ((MAC_8TAP(c,r0,r1,r2,r3,r4,r5,r6,r7) + offset) >> shift)
#define MAC_8TAP_NN_S2(c, r0, r1, r2, r3, r4, r5, r6, r7, offset, shift) \
    ((MAC_8TAP(c,r0,r1,r2,r3,r4,r5,r6,r7) + offset) >> shift)

#define MAC_4TAP(c, r0, r1, r2, r3) \
    ((c)[0]*(r0)+(c)[1]*(r1)+(c)[2]*(r2)+(c)[3]*(r3))
#define MAC_4TAP_N0(c, r0, r1, r2, r3) \
    ((MAC_4TAP(c, r0, r1, r2, r3) + MAC_ADD_N0) >> MAC_SFT_N0)
#define MAC_4TAP_0N(c, r0, r1, r2, r3) \
    ((MAC_4TAP(c, r0, r1, r2, r3) + MAC_ADD_0N) >> MAC_SFT_0N)

#define MAC_4TAP_NN_S1(c, r0, r1, r2, r3, offset, shift) \
    ((MAC_4TAP(c, r0, r1, r2, r3) + offset) >> shift)
#define MAC_4TAP_NN_S2(c, r0, r1, r2, r3, offset, shift) \
    ((MAC_4TAP(c, r0, r1, r2, r3) + offset) >> shift)


#define MAC_BL(c, r0, r1) \
    ((c)[0]*(r0)+(c)[1]*(r1))
#define MAC_BL_N0(c, r0, r1) \
    ((MAC_BL(c, r0, r1) + MAC_ADD_N0) >> MAC_SFT_N0)
#define MAC_BL_0N(c, r0, r1) \
    ((MAC_BL(c, r0, r1) + MAC_ADD_0N) >> MAC_SFT_0N)

#define MAC_BL_NN_S1(c, r0, r1, offset, shift) \
    ((MAC_BL(c, r0, r1) + offset) >> shift)
#define MAC_BL_NN_S2(c, r0, r1, offset, shift) \
    ((MAC_BL(c, r0, r1) + offset) >> shift)


static const s8 shuffle_2Tap[16] = { 0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9 };

void mc_filter_bilin_horz_sse(s16 const *ref,
    int src_stride,
    s16 *pred,
    int dst_stride,
    const short *coeff,
    int width,
    int height,
    int min_val,
    int max_val,
    int offset,
    int shift,
    s8  is_last)
{
    int row, col, rem_w, rem_h;
    int src_stride2, src_stride3;
    s16 const *inp_copy;
    s16 *dst_copy;

    __m128i offset_4x32b = _mm_set1_epi32(offset);
    __m128i mm_min = _mm_set1_epi16(min_val);
    __m128i mm_max = _mm_set1_epi16(max_val);

    __m128i row1, row11, row2, row22, row3, row33, row4, row44;
    __m128i res0, res1, res2, res3;
    __m128i coeff0_1_8x16b, shuffle;

    rem_w = width;
    inp_copy = ref;
    dst_copy = pred;

    src_stride2 = (src_stride << 1);
    src_stride3 = (src_stride * 3);

    /* load 8 8-bit coefficients and convert 8-bit into 16-bit  */
    coeff0_1_8x16b = _mm_loadl_epi64((__m128i*)coeff);      /*w0 w1 x x x x x x*/
    coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);  /*w0 w1 w0 w1 w0 w1 w0 w1*/

    shuffle = _mm_loadu_si128((__m128i*)shuffle_2Tap);

    rem_h = (height & 0x3);

    if (rem_w > 7)
    {
        for (row = height; row > 3; row -= 4)
        {
            int cnt = 0;
            for (col = rem_w; col > 7; col -= 8)
            {
                /*load 8 pixel values from row 0*/
                row1 = _mm_loadu_si128((__m128i*)(inp_copy + cnt));                             /*a0 a1 a2 a3 a4 a5 a6 a7*/
                row11 = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 1));                        /*a1 a2 a3 a4 a5 a6 a7 a8*/
                row2 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + cnt));       /*b0 b1 b2 b3 b4 b5 b6 b7*/
                row22 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + cnt + 1));  /*b1 b2 b3 b4 b5 b6 b7 b8*/
                row3 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride2 + cnt));
                row33 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride2 + cnt + 1));
                row4 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride3 + cnt));
                row44 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride3 + cnt + 1));

                row1 = _mm_madd_epi16(row1, coeff0_1_8x16b);            /*a0+a1 a2+a3 a4+a5 a6+a7*/
                row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);          /*a1+a2 a3+a4 a5+a6 a7+a8*/
                row2 = _mm_madd_epi16(row2, coeff0_1_8x16b);
                row22 = _mm_madd_epi16(row22, coeff0_1_8x16b);
                row3 = _mm_madd_epi16(row3, coeff0_1_8x16b);
                row33 = _mm_madd_epi16(row33, coeff0_1_8x16b);
                row4 = _mm_madd_epi16(row4, coeff0_1_8x16b);
                row44 = _mm_madd_epi16(row44, coeff0_1_8x16b);

                row1 = _mm_add_epi32(row1, offset_4x32b);
                row11 = _mm_add_epi32(row11, offset_4x32b);
                row2 = _mm_add_epi32(row2, offset_4x32b);
                row22 = _mm_add_epi32(row22, offset_4x32b);
                row3 = _mm_add_epi32(row3, offset_4x32b);
                row33 = _mm_add_epi32(row33, offset_4x32b);
                row4 = _mm_add_epi32(row4, offset_4x32b);
                row44 = _mm_add_epi32(row44, offset_4x32b);

                row1 = _mm_srai_epi32(row1, shift);
                row11 = _mm_srai_epi32(row11, shift);
                row2 = _mm_srai_epi32(row2, shift);
                row22 = _mm_srai_epi32(row22, shift);
                row3 = _mm_srai_epi32(row3, shift);
                row33 = _mm_srai_epi32(row33, shift);
                row4 = _mm_srai_epi32(row4, shift);
                row44 = _mm_srai_epi32(row44, shift);

                row1 = _mm_packs_epi32(row1, row2);
                row11 = _mm_packs_epi32(row11, row22);
                row3 = _mm_packs_epi32(row3, row4);
                row33 = _mm_packs_epi32(row33, row44);

                res0 = _mm_unpacklo_epi16(row1, row11);
                res1 = _mm_unpackhi_epi16(row1, row11);
                res2 = _mm_unpacklo_epi16(row3, row33);
                res3 = _mm_unpackhi_epi16(row3, row33);

                if (is_last)
                {
                    res0 = _mm_min_epi16(res0, mm_max);
                    res1 = _mm_min_epi16(res1, mm_max);
                    res2 = _mm_min_epi16(res2, mm_max);
                    res3 = _mm_min_epi16(res3, mm_max);

                    res0 = _mm_max_epi16(res0, mm_min);
                    res1 = _mm_max_epi16(res1, mm_min);
                    res2 = _mm_max_epi16(res2, mm_min);
                    res3 = _mm_max_epi16(res3, mm_min);
                }

                /* to store the 8 pixels res. */
                _mm_storeu_si128((__m128i *)(dst_copy + cnt), res0);
                _mm_storeu_si128((__m128i *)(dst_copy + dst_stride + cnt), res1);
                _mm_storeu_si128((__m128i *)(dst_copy + dst_stride * 2 + cnt), res2);
                _mm_storeu_si128((__m128i *)(dst_copy + dst_stride * 3 + cnt), res3);

                cnt += 8; /* To pointer updates*/
            }

            inp_copy += (src_stride << 2);
            dst_copy += (dst_stride << 2);
        }

        /*extra height to be done --- one row at a time*/
        for (row = 0; row < rem_h; row++)
        {
            int cnt = 0;
            for (col = rem_w; col > 7; col -= 8)
            {
                /*load 8 pixel values from row 0*/
                row1 = _mm_loadu_si128((__m128i*)(inp_copy + cnt));       /*a0 a1 a2 a3 a4 a5 a6 a7*/
                row11 = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 1));  /*a1 a2 a3 a4 a5 a6 a7 a8*/

                row1 = _mm_madd_epi16(row1, coeff0_1_8x16b);              /*a0+a1 a2+a3 a4+a5 a6+a7*/
                row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);            /*a1+a2 a3+a4 a5+a6 a7+a8*/

                row1 = _mm_add_epi32(row1, offset_4x32b);
                row11 = _mm_add_epi32(row11, offset_4x32b);

                row1 = _mm_srai_epi32(row1, shift);
                row11 = _mm_srai_epi32(row11, shift);

                row1 = _mm_packs_epi32(row1, row11);    /*a0 a2 a4 a6 a1 a3 a5 a7*/

                res0 = _mm_unpackhi_epi64(row1, row1);  /*a1 a3 a5 a7*/
                res1 = _mm_unpacklo_epi16(row1, res0);  /*a0 a1 a2 a3 a4 a5 a6 a7*/

                if (is_last)
                {
                    res1 = _mm_min_epi16(res1, mm_max);
                    res1 = _mm_max_epi16(res1, mm_min);
                }

                /* to store the 8 pixels res. */
                _mm_storeu_si128((__m128i *)(dst_copy + cnt), res1);

                cnt += 8;
            }

            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x7;

    if (rem_w > 3)
    {
        inp_copy = ref + ((width / 8) * 8);
        dst_copy = pred + ((width / 8) * 8);

        for (row = height; row > 3; row -= 4)
        {
            /*load 8 pixel values from row 0*/
            row1 = _mm_loadu_si128((__m128i*)(inp_copy));                        /*a0 a1 a2 a3 a4 a5 a6 a7*/
            row2 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride));  /*a1 a2 a3 a4 a5 a6 a7 a8*/
            row3 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride2));
            row4 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride3));

            row1 = _mm_shuffle_epi8(row1, shuffle);  /*a0 a1 a1 a2 a2 a3 a3 a4 */
            row2 = _mm_shuffle_epi8(row2, shuffle);
            row3 = _mm_shuffle_epi8(row3, shuffle);
            row4 = _mm_shuffle_epi8(row4, shuffle);

            row1 = _mm_madd_epi16(row1, coeff0_1_8x16b);  /*a0+a1 a1+a2 a2+a3 a3+a4*/
            row2 = _mm_madd_epi16(row2, coeff0_1_8x16b);
            row3 = _mm_madd_epi16(row3, coeff0_1_8x16b);
            row4 = _mm_madd_epi16(row4, coeff0_1_8x16b);

            row1 = _mm_add_epi32(row1, offset_4x32b);
            row2 = _mm_add_epi32(row2, offset_4x32b);
            row3 = _mm_add_epi32(row3, offset_4x32b);
            row4 = _mm_add_epi32(row4, offset_4x32b);

            row1 = _mm_srai_epi32(row1, shift);
            row2 = _mm_srai_epi32(row2, shift);
            row3 = _mm_srai_epi32(row3, shift);
            row4 = _mm_srai_epi32(row4, shift);

            res0 = _mm_packs_epi32(row1, row2);
            res1 = _mm_packs_epi32(row3, row4);

            if (is_last)
            {
                res0 = _mm_min_epi16(res0, mm_max);
                res1 = _mm_min_epi16(res1, mm_max);

                res0 = _mm_max_epi16(res0, mm_min);
                res1 = _mm_max_epi16(res1, mm_min);
            }

            /* to store the 8 pixels res. */
            _mm_storel_epi64((__m128i *)(dst_copy), res0);
            _mm_storel_epi64((__m128i *)(dst_copy + dst_stride * 2), res1);
            _mm_storel_epi64((__m128i *)(dst_copy + dst_stride), _mm_unpackhi_epi64(res0, res0));
            _mm_storel_epi64((__m128i *)(dst_copy + dst_stride * 3), _mm_unpackhi_epi64(res1, res1));

            inp_copy += (src_stride << 2);
            dst_copy += (dst_stride << 2);
        }

        for (row = 0; row < rem_h; row++)
        {
            /*load 8 pixel values from row 0*/
            row1 = _mm_loadu_si128((__m128i*)(inp_copy));  /*a0 a1 a2 a3 a4 a5 a6 a7*/

            res0 = _mm_shuffle_epi8(row1, shuffle);        /*a0 a1 a1 a2 a2 a3 a3 a4 */
            res0 = _mm_madd_epi16(res0, coeff0_1_8x16b);   /*a0+a1 a1+a2 a2+a3 a3+a4*/
            res0 = _mm_add_epi32(res0, offset_4x32b);
            res0 = _mm_srai_epi32(res0, shift);
            res0 = _mm_packs_epi32(res0, res0);

            if (is_last)
            {
                res0 = _mm_min_epi16(res0, mm_max);
                res0 = _mm_max_epi16(res0, mm_min);
            }

            _mm_storel_epi64((__m128i *)(dst_copy), res0);

            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x3;

    if (rem_w)
    {
        int sum, sum1;

        inp_copy = ref + ((width / 4) * 4);
        dst_copy = pred + ((width / 4) * 4);

        for (row = height; row > 3; row -= 4)
        {
            for (col = 0; col < rem_w; col++)
            {
                row1 = _mm_loadu_si128((__m128i*)(inp_copy + col));                        /*a0 a1 x x x x x x*/
                row2 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + col));  /*b0 b1 x x x x x x*/
                row3 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride2 + col));
                row4 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride3 + col));

                row1 = _mm_unpacklo_epi32(row1, row2);  /*a0 a1 b0 b1*/
                row3 = _mm_unpacklo_epi32(row3, row4);  /*c0 c1 d0 d1*/
                row1 = _mm_unpacklo_epi64(row1, row3);  /*a0 a1 b0 b1 c0 c1 d0 d1*/

                row1 = _mm_madd_epi16(row1, coeff0_1_8x16b);  /*a0+a1 b0+b1 c0+c1 d0+d1*/

                row1 = _mm_add_epi32(row1, offset_4x32b);
                row1 = _mm_srai_epi32(row1, shift);
                res0 = _mm_packs_epi32(row1, row1);

                if (is_last)
                {
                    res0 = _mm_min_epi16(res0, mm_max);
                    res0 = _mm_max_epi16(res0, mm_min);
                }

                /*extract 32 bit integer form register and store it in dst_copy*/
                sum = _mm_extract_epi32(res0, 0);
                sum1 = _mm_extract_epi32(res0, 1);

                dst_copy[col] = (s16)(sum & 0xffff);
                dst_copy[col + dst_stride] = (s16)(sum >> 16);
                dst_copy[col + (dst_stride << 1)] = (s16)(sum1 & 0xffff);
                dst_copy[col + (dst_stride * 3)] = (s16)(sum1 >> 16);
            }
            inp_copy += (src_stride << 2);
            dst_copy += (dst_stride << 2);
        }

        for (row = 0; row < rem_h; row++)
        {
            for (col = 0; col < rem_w; col++)
            {
                s16 val;
                int sum;

                sum = inp_copy[col + 0] * coeff[0];
                sum += inp_copy[col + 1] * coeff[1];

                val = (sum + offset) >> shift;
                dst_copy[col] = (is_last ? (XEVD_CLIP3(min_val, max_val, val)) : val);
            }
            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
}

void mc_filter_bilin_vert_sse(s16 const *ref,
    int src_stride,
    s16 *pred,
    int dst_stride,
    const short *coeff,
    int width,
    int height,
    int min_val,
    int max_val,
    int offset,
    int shift,
    s8  is_last)
{
    int row, col, rem_w, rem_h;
    int src_stride2, src_stride3, src_stride4;
    s16 const *inp_copy;
    s16 *dst_copy;

    __m128i offset_4x32b = _mm_set1_epi32(offset);
    __m128i mm_min = _mm_set1_epi16(min_val);
    __m128i mm_max = _mm_set1_epi16(max_val);

    __m128i row1, row11, row2, row22, row3, row33, row4, row44, row5;
    __m128i res0, res1, res2, res3;
    __m128i coeff0_1_8x16b;

    rem_w = width;
    inp_copy = ref;
    dst_copy = pred;

    src_stride2 = (src_stride << 1);
    src_stride3 = (src_stride * 3);
    src_stride4 = (src_stride << 2);

    coeff0_1_8x16b = _mm_loadl_epi64((__m128i*)coeff);      /*w0 w1 x x x x x x*/
    coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);  /*w0 w1 w0 w1 w0 w1 w0 w1*/

    rem_h = height & 0x3;

    if (rem_w > 7)
    {
        for (row = height; row > 3; row -= 4)
        {
            int cnt = 0;
            for (col = rem_w; col > 7; col -= 8)
            {
                /*load 8 pixel values from row 0*/
                row1 = _mm_loadu_si128((__m128i*)(inp_copy + cnt));                        /*a0 a1 a2 a3 a4 a5 a6 a7*/
                row2 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + cnt));  /*b0 b1 b2 b3 b4 b5 b6 b7*/
                row3 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride2 + cnt));
                row4 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride3 + cnt));
                row5 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride4 + cnt));

                row11 = _mm_unpacklo_epi16(row1, row2);   /*a0 b0 a1 b1 a2 b2 a3 b3*/
                row1 = _mm_unpackhi_epi16(row1, row2);    /*a4 b4 a5 b5 a6 b6 a7 b7*/
                row22 = _mm_unpacklo_epi16(row2, row3);
                row2 = _mm_unpackhi_epi16(row2, row3);
                row33 = _mm_unpacklo_epi16(row3, row4);
                row3 = _mm_unpackhi_epi16(row3, row4);
                row44 = _mm_unpacklo_epi16(row4, row5);
                row4 = _mm_unpackhi_epi16(row4, row5);

                row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);  /*a0+a1 a2+a3 a4+a5 a6+a7*/
                row1 = _mm_madd_epi16(row1, coeff0_1_8x16b);    /*a1+a2 a3+a4 a5+a6 a7+a8*/
                row22 = _mm_madd_epi16(row22, coeff0_1_8x16b);
                row2 = _mm_madd_epi16(row2, coeff0_1_8x16b);
                row33 = _mm_madd_epi16(row33, coeff0_1_8x16b);
                row3 = _mm_madd_epi16(row3, coeff0_1_8x16b);
                row44 = _mm_madd_epi16(row44, coeff0_1_8x16b);
                row4 = _mm_madd_epi16(row4, coeff0_1_8x16b);

                row11 = _mm_add_epi32(row11, offset_4x32b);
                row1 = _mm_add_epi32(row1, offset_4x32b);
                row22 = _mm_add_epi32(row22, offset_4x32b);
                row2 = _mm_add_epi32(row2, offset_4x32b);
                row33 = _mm_add_epi32(row33, offset_4x32b);
                row3 = _mm_add_epi32(row3, offset_4x32b);
                row44 = _mm_add_epi32(row44, offset_4x32b);
                row4 = _mm_add_epi32(row4, offset_4x32b);

                row11 = _mm_srai_epi32(row11, shift);
                row1 = _mm_srai_epi32(row1, shift);
                row22 = _mm_srai_epi32(row22, shift);
                row2 = _mm_srai_epi32(row2, shift);
                row33 = _mm_srai_epi32(row33, shift);
                row3 = _mm_srai_epi32(row3, shift);
                row44 = _mm_srai_epi32(row44, shift);
                row4 = _mm_srai_epi32(row4, shift);

                res0 = _mm_packs_epi32(row11, row1);
                res1 = _mm_packs_epi32(row22, row2);
                res2 = _mm_packs_epi32(row33, row3);
                res3 = _mm_packs_epi32(row44, row4);

                if (is_last)
                {
                    res0 = _mm_min_epi16(res0, mm_max);
                    res1 = _mm_min_epi16(res1, mm_max);
                    res2 = _mm_min_epi16(res2, mm_max);
                    res3 = _mm_min_epi16(res3, mm_max);

                    res0 = _mm_max_epi16(res0, mm_min);
                    res1 = _mm_max_epi16(res1, mm_min);
                    res2 = _mm_max_epi16(res2, mm_min);
                    res3 = _mm_max_epi16(res3, mm_min);
                }

                /* to store the 8 pixels res. */
                _mm_storeu_si128((__m128i *)(dst_copy + cnt), res0);
                _mm_storeu_si128((__m128i *)(dst_copy + dst_stride + cnt), res1);
                _mm_storeu_si128((__m128i *)(dst_copy + dst_stride * 2 + cnt), res2);
                _mm_storeu_si128((__m128i *)(dst_copy + dst_stride * 3 + cnt), res3);

                cnt += 8;  /* To pointer updates*/
            }

            inp_copy += (src_stride << 2);
            dst_copy += (dst_stride << 2);
        }

        /*extra height to be done --- one row at a time*/
        for (row = 0; row < rem_h; row++)
        {
            int cnt = 0;
            for (col = rem_w; col > 7; col -= 8)
            {
                /*load 8 pixel values from row 0*/
                row1 = _mm_loadu_si128((__m128i*)(inp_copy + cnt));                        /*a0 a1 a2 a3 a4 a5 a6 a7*/
                row2 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + cnt));  /*b0 b1 b2 b3 b4 b5 b6 b7*/

                row11 = _mm_unpacklo_epi16(row1, row2);  /*a0 b0 a1 b1 a2 b2 a3 b3*/
                row1 = _mm_unpackhi_epi16(row1, row2);   /*a4 b4 a5 b5 a6 b6 a7 b7*/

                row1 = _mm_madd_epi16(row1, coeff0_1_8x16b);    /*a0+a1 a2+a3 a4+a5 a6+a7*/
                row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);  /*a1+a2 a3+a4 a5+a6 a7+a8*/

                row1 = _mm_add_epi32(row1, offset_4x32b);
                row11 = _mm_add_epi32(row11, offset_4x32b);

                row1 = _mm_srai_epi32(row1, shift);
                row11 = _mm_srai_epi32(row11, shift);

                res1 = _mm_packs_epi32(row11, row1);

                if (is_last)
                {
                    res1 = _mm_min_epi16(res1, mm_max);
                    res1 = _mm_max_epi16(res1, mm_min);
                }

                /* to store the 8 pixels res. */
                _mm_storeu_si128((__m128i *)(dst_copy + cnt), res1);

                cnt += 8;
            }

            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x7;

    if (rem_w > 3)
    {
        inp_copy = ref + ((width / 8) * 8);
        dst_copy = pred + ((width / 8) * 8);

        for (row = height; row > 3; row -= 4)
        {
            /*load 4 pixel values */
            row1 = _mm_loadl_epi64((__m128i*)(inp_copy));                        /*a0 a1 a2 a3 x x x x*/
            row2 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride));  /*b0 b1 b2 b3 x x x x*/
            row3 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride2));
            row4 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride3));
            row5 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride4));

            row11 = _mm_unpacklo_epi16(row1, row2);  /*a0 b0 a1 b1 a2 b2 a3 b3*/
            row22 = _mm_unpacklo_epi16(row2, row3);
            row33 = _mm_unpacklo_epi16(row3, row4);
            row44 = _mm_unpacklo_epi16(row4, row5);

            row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);  /*a0+a1 a1+a2 a2+a3 a3+a4*/
            row22 = _mm_madd_epi16(row22, coeff0_1_8x16b);
            row33 = _mm_madd_epi16(row33, coeff0_1_8x16b);
            row44 = _mm_madd_epi16(row44, coeff0_1_8x16b);

            row11 = _mm_add_epi32(row11, offset_4x32b);
            row22 = _mm_add_epi32(row22, offset_4x32b);
            row33 = _mm_add_epi32(row33, offset_4x32b);
            row44 = _mm_add_epi32(row44, offset_4x32b);

            row11 = _mm_srai_epi32(row11, shift);
            row22 = _mm_srai_epi32(row22, shift);
            row33 = _mm_srai_epi32(row33, shift);
            row44 = _mm_srai_epi32(row44, shift);

            res0 = _mm_packs_epi32(row11, row22);
            res1 = _mm_packs_epi32(row33, row44);

            if (is_last)
            {
                res0 = _mm_min_epi16(res0, mm_max);
                res1 = _mm_min_epi16(res1, mm_max);
                res0 = _mm_max_epi16(res0, mm_min);
                res1 = _mm_max_epi16(res1, mm_min);
            }

            /* to store the 8 pixels res. */
            _mm_storel_epi64((__m128i *)(dst_copy), res0);
            _mm_storel_epi64((__m128i *)(dst_copy + dst_stride), _mm_unpackhi_epi64(res0, res0));
            _mm_storel_epi64((__m128i *)(dst_copy + dst_stride * 2), res1);
            _mm_storel_epi64((__m128i *)(dst_copy + dst_stride * 3), _mm_unpackhi_epi64(res1, res1));

            inp_copy += (src_stride << 2);
            dst_copy += (dst_stride << 2);
        }

        for (row = 0; row < rem_h; row++)
        {
            /*load 8 pixel values from row 0*/
            row1 = _mm_loadl_epi64((__m128i*)(inp_copy));                        /*a0 a1 a2 a3 x x x x*/
            row2 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride));  /*b0 b1 b2 b3 x x x x*/

            row11 = _mm_unpacklo_epi16(row1, row2);         /*a0 b0 a1 b1 a2 b2 a3 b3*/
            row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);  /*a0+a1 a1+a2 a2+a3 a3+a4*/
            row11 = _mm_add_epi32(row11, offset_4x32b);
            row11 = _mm_srai_epi32(row11, shift);
            row11 = _mm_packs_epi32(row11, row11);

            if (is_last)
            {
                row11 = _mm_min_epi16(row11, mm_max);
                row11 = _mm_max_epi16(row11, mm_min);
            }

            _mm_storel_epi64((__m128i *)(dst_copy), row11);

            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x3;

    if (rem_w)
    {
        inp_copy = ref + ((width / 4) * 4);
        dst_copy = pred + ((width / 4) * 4);

        for (row = 0; row < height; row++)
        {
            for (col = 0; col < rem_w; col++)
            {
                s16 val;
                int sum;

                sum = inp_copy[col + 0 * src_stride] * coeff[0];
                sum += inp_copy[col + 1 * src_stride] * coeff[1];

                val = (sum + offset) >> shift;
                dst_copy[col] = (is_last ? (XEVD_CLIP3(min_val, max_val, val)) : val);
            }

            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
}



void xevd_mc_dmvr_l_00_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int i, j;

    gmv_x >>= 4;
    gmv_y >>= 4;

    if (((w & 0x7) == 0) && ((h & 1) == 0))
    {
        __m128i m00, m01;

        for (i = 0; i < h; i += 2)
        {
            for (j = 0; j < w; j += 8)
            {
                m00 = _mm_loadu_si128((__m128i*)(ref + j));
                m01 = _mm_loadu_si128((__m128i*)(ref + j + s_ref));

                _mm_storeu_si128((__m128i*)(pred + j), m00);
                _mm_storeu_si128((__m128i*)(pred + j + s_pred), m01);
            }
            pred += s_pred * 2;
            ref += s_ref * 2;
        }
    }
    else if ((w & 0x3) == 0)
    {
        __m128i m00;

        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j += 4)
            {
                m00 = _mm_loadl_epi64((__m128i*)(ref + j));
                _mm_storel_epi64((__m128i*)(pred + j), m00);
            }
            pred += s_pred;
            ref += s_ref;
        }
    }
    else
    {
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                pred[j] = ref[j];
            }
            pred += s_pred;
            ref += s_ref;
        }
    }
}

void xevd_mc_dmvr_l_n0_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int dx;

    dx = gmv_x & 15;
    ref = ref - 3;

    int max = ((1 << bit_depth) - 1);

    int min = 0;
    xevd_mc_filter_l_8pel_horz_clip_sse(ref, s_ref, pred, s_pred, tbl_mc_l_coeff[dx], w, h, min, max, MAC_ADD_N0, MAC_SFT_N0);
}

void xevd_mc_dmvr_l_0n_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int dy;

    dy = gmv_y & 15;
    ref = ref - (3 * s_ref);

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    xevd_mc_filter_l_8pel_vert_clip_sse(ref, s_ref, pred, s_pred, tbl_mc_l_coeff[dy], w, h, min, max, MAC_ADD_0N, MAC_SFT_0N);
}

void xevd_mc_dmvr_l_nn_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_L)*MAX_CU_SIZE];
    int         dx, dy;

    dx = gmv_x & 15;
    dy = gmv_y & 15;
    ref = ref - (3 * s_ref + 3);

    int shift1 = XEVD_MIN(4, bit_depth - 8);
    int shift2 = XEVD_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    xevd_mc_filter_l_8pel_horz_no_clip_sse(ref, s_ref, buf, w, tbl_mc_l_coeff[dx], w, (h + 7), offset1, shift1);
    xevd_mc_filter_l_8pel_vert_clip_sse(buf, w, pred, s_pred, tbl_mc_l_coeff[dy], w, h, min, max, offset2, shift2);
}


void xevdm_bl_mc_l_00_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int i, j;

    gmv_x >>= 4;
    gmv_y >>= 4;

    ref += gmv_y * s_ref + gmv_x;

    if (((w & 0x7) == 0) && ((h & 1) == 0))
    {
        __m128i m00, m01;

        for (i = 0; i < h; i += 2)
        {
            for (j = 0; j < w; j += 8)
            {
                m00 = _mm_loadu_si128((__m128i*)(ref + j));
                m01 = _mm_loadu_si128((__m128i*)(ref + j + s_ref));

                _mm_storeu_si128((__m128i*)(pred + j), m00);
                _mm_storeu_si128((__m128i*)(pred + j + s_pred), m01);
            }
            pred += s_pred * 2;
            ref += s_ref * 2;
        }
    }
    else if ((w & 0x3) == 0)
    {
        __m128i m00;

        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j += 4)
            {
                m00 = _mm_loadl_epi64((__m128i*)(ref + j));
                _mm_storel_epi64((__m128i*)(pred + j), m00);
            }
            pred += s_pred;
            ref += s_ref;
        }
    }
    else
    {
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                pred[j] = ref[j];
            }
            pred += s_pred;
            ref += s_ref;
        }
    }
}

void xevdm_bl_mc_l_n0_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int dx;

    dx = gmv_x & 15;
    ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4);

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_bilin_horz_sse(ref, s_ref, pred, s_pred, xevd_tbl_bl_mc_l_coeff[dx], w, h, min, max, MAC_ADD_N0, MAC_SFT_N0, 1);
}

void xevdm_bl_mc_l_0n_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int dy;

    dy = gmv_y & 15;
    ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4);

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_bilin_vert_sse(ref, s_ref, pred, s_pred, xevd_tbl_bl_mc_l_coeff[dy], w, h, min, max, MAC_ADD_0N, MAC_SFT_0N, 1);
}

void xevdm_bl_mc_l_nn_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_L)*(MAX_CU_SIZE + MC_IBUF_PAD_L)];
    int         dx, dy;

    dx = gmv_x & 15;
    dy = gmv_y & 15;
    ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4);

    int shift1 = XEVD_MIN(4, bit_depth - 8);
    int shift2 = XEVD_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_bilin_horz_sse(ref, s_ref, buf, w, xevd_tbl_bl_mc_l_coeff[dx], w, (h + 1), min, max, offset1, shift1, 0);
    mc_filter_bilin_vert_sse(buf, w, pred, s_pred, xevd_tbl_bl_mc_l_coeff[dy], w, h, min, max, offset2, shift2, 1);
}

/****************************************************************************
* motion compensation for chroma
****************************************************************************/


void xevd_mc_dmvr_c_00_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int i, j;

    gmv_x >>= 5;
    gmv_y >>= 5;

    if (((w & 0x7) == 0) && ((h & 1) == 0))
    {
        __m128i m00, m01;

        for (i = 0; i < h; i += 2)
        {
            for (j = 0; j < w; j += 8)
            {
                m00 = _mm_loadu_si128((__m128i*)(ref + j));
                m01 = _mm_loadu_si128((__m128i*)(ref + j + s_ref));

                _mm_storeu_si128((__m128i*)(pred + j), m00);
                _mm_storeu_si128((__m128i*)(pred + j + s_pred), m01);
            }
            pred += s_pred * 2;
            ref += s_ref * 2;
        }
    }
    else if (((w & 0x3) == 0))
    {
        __m128i m00;

        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j += 4)
            {
                m00 = _mm_loadl_epi64((__m128i*)(ref + j));
                _mm_storel_epi64((__m128i*)(pred + j), m00);
            }
            pred += s_pred;
            ref += s_ref;
        }
    }
    else
    {
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                pred[j] = ref[j];
            }
            pred += s_pred;
            ref += s_ref;
        }
    }
}

void xevd_mc_dmvr_c_n0_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int       dx;

    dx = gmv_x & 31;
    ref -= 1;

    int max = ((1 << bit_depth) - 1);
    int min = 0;
    xevd_mc_filter_c_4pel_horz_sse(ref, s_ref, pred, s_pred, tbl_mc_c_coeff[dx], w, h, min, max, MAC_ADD_N0, MAC_SFT_N0, 1);
}

void xevd_mc_dmvr_c_0n_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int dy;

    dy = gmv_y & 31;
    ref -= 1 * s_ref;

    int max = ((1 << bit_depth) - 1);
    int min = 0;
    xevd_mc_filter_c_4pel_vert_sse(ref, s_ref, pred, s_pred, tbl_mc_c_coeff[dy], w, h, min, max, MAC_ADD_0N, MAC_SFT_0N, 1);
}

void xevd_mc_dmvr_c_nn_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_C)*MAX_CU_SIZE];
    int         dx, dy;

    dx = gmv_x & 31;
    dy = gmv_y & 31;
    ref -= (1 * s_ref + 1);

    int shift1 = XEVD_MIN(4, bit_depth - 8);
    int shift2 = XEVD_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));


    int max = ((1 << bit_depth) - 1);
    int min = 0;
    xevd_mc_filter_c_4pel_horz_sse(ref, s_ref, buf, w, tbl_mc_c_coeff[dx], w, (h + 3), min, max, offset1, shift1, 0);
    xevd_mc_filter_c_4pel_vert_sse(buf, w, pred, s_pred, tbl_mc_c_coeff[dy], w, h, min, max, offset2, shift2, 1);
}

XEVD_MC_L xevdm_tbl_dmvr_mc_l_sse[2][2] =
{
    {
        xevd_mc_dmvr_l_00_sse, /* dx == 0 && dy == 0 */
        xevd_mc_dmvr_l_0n_sse  /* dx == 0 && dy != 0 */
    },
    {
        xevd_mc_dmvr_l_n0_sse, /* dx != 0 && dy == 0 */
        xevd_mc_dmvr_l_nn_sse  /* dx != 0 && dy != 0 */
    }
};

XEVD_MC_C xevdm_tbl_dmvr_mc_c_sse[2][2] =
{
    {
        xevd_mc_dmvr_c_00_sse, /* dx == 0 && dy == 0 */
        xevd_mc_dmvr_c_0n_sse  /* dx == 0 && dy != 0 */
    },
    {
        xevd_mc_dmvr_c_n0_sse, /* dx != 0 && dy == 0 */
        xevd_mc_dmvr_c_nn_sse  /* dx != 0 && dy != 0 */
    }
};

/* luma and chroma will remain the same */
XEVD_MC_L xevdm_tbl_bl_mc_l_sse[2][2] =
{
    {
        xevdm_bl_mc_l_00_sse,
        xevdm_bl_mc_l_0n_sse
    },
    {
        xevdm_bl_mc_l_n0_sse,
        xevdm_bl_mc_l_nn_sse
    }
};

int dmvr_sad_mr_16b_sse(int w, int h, void * src1, void * src2, int s_src1, int s_src2, s16 delta, int bit_depth)
{
    int  i, j, rem_w;
    int mr_sad = 0;

    short *pu2_inp;
    short *pu2_ref;

    __m128i src1_8x16b_0, src1_8x16b_1, src1_8x16b_2, src1_8x16b_3;
    __m128i src2_8x16b_0, src2_8x16b_1, src2_8x16b_2, src2_8x16b_3;

    __m128i /*mm_zero, */mm_result1, mm_result2, mm_delta;

    assert(bit_depth <= 14);


    mm_delta = _mm_set1_epi16(delta);

    pu2_inp = src1;
    pu2_ref = src2;
    rem_w = w;

    mm_result1 = _mm_setzero_si128();
    mm_result2 = _mm_setzero_si128();

    /* 16 pixels at a time */
    if (rem_w > 15)
    {
        for (i = h; i > 1; i -= 2)
        {
            for (j = 0; j < w; j += 16)
            {
                src2_8x16b_0 = _mm_loadu_si128((__m128i *) (pu2_ref));
                src2_8x16b_1 = _mm_loadu_si128((__m128i *) (pu2_ref + 8));
                src2_8x16b_2 = _mm_loadu_si128((__m128i *) (pu2_ref + s_src2));
                src2_8x16b_3 = _mm_loadu_si128((__m128i *) (pu2_ref + s_src2 + 8));

                src1_8x16b_0 = _mm_loadu_si128((__m128i *) (pu2_inp));
                src1_8x16b_1 = _mm_loadu_si128((__m128i *) (pu2_inp + 8));
                src1_8x16b_2 = _mm_loadu_si128((__m128i *) (pu2_inp + s_src1));
                src1_8x16b_3 = _mm_loadu_si128((__m128i *) (pu2_inp + s_src1 + 8));

                src2_8x16b_0 = _mm_slli_epi16(src2_8x16b_0, 1);
                src2_8x16b_1 = _mm_slli_epi16(src2_8x16b_1, 1);
                src2_8x16b_2 = _mm_slli_epi16(src2_8x16b_2, 1);
                src2_8x16b_3 = _mm_slli_epi16(src2_8x16b_3, 1);

                src1_8x16b_0 = _mm_sub_epi16(src1_8x16b_0, mm_delta);
                src1_8x16b_1 = _mm_sub_epi16(src1_8x16b_1, mm_delta);
                src1_8x16b_2 = _mm_sub_epi16(src1_8x16b_2, mm_delta);
                src1_8x16b_3 = _mm_sub_epi16(src1_8x16b_3, mm_delta);

                src1_8x16b_0 = _mm_sub_epi16(src1_8x16b_0, src2_8x16b_0);
                src1_8x16b_1 = _mm_sub_epi16(src1_8x16b_1, src2_8x16b_1);
                src1_8x16b_2 = _mm_sub_epi16(src1_8x16b_2, src2_8x16b_2);
                src1_8x16b_3 = _mm_sub_epi16(src1_8x16b_3, src2_8x16b_3);

                src1_8x16b_0 = _mm_abs_epi16(src1_8x16b_0);
                src1_8x16b_1 = _mm_abs_epi16(src1_8x16b_1);
                src1_8x16b_2 = _mm_abs_epi16(src1_8x16b_2);
                src1_8x16b_3 = _mm_abs_epi16(src1_8x16b_3);

                src1_8x16b_0 = _mm_add_epi16(src1_8x16b_0, src1_8x16b_1);
                src1_8x16b_2 = _mm_add_epi16(src1_8x16b_2, src1_8x16b_3);

                src1_8x16b_1 = _mm_srli_si128(src1_8x16b_0, 8);
                src1_8x16b_3 = _mm_srli_si128(src1_8x16b_2, 8);
                src1_8x16b_0 = _mm_cvtepi16_epi32(src1_8x16b_0);
                src1_8x16b_2 = _mm_cvtepi16_epi32(src1_8x16b_2);
                src1_8x16b_1 = _mm_cvtepi16_epi32(src1_8x16b_1);
                src1_8x16b_3 = _mm_cvtepi16_epi32(src1_8x16b_3);

                src1_8x16b_0 = _mm_add_epi32(src1_8x16b_0, src1_8x16b_1);
                src1_8x16b_2 = _mm_add_epi32(src1_8x16b_2, src1_8x16b_3);

                mm_result1 = _mm_add_epi32(mm_result1, src1_8x16b_0);
                mm_result2 = _mm_add_epi32(mm_result2, src1_8x16b_2);

                pu2_inp += 16;
                pu2_ref += 16;
            }

            pu2_inp = pu2_inp - ((w >> 4) << 4) + s_src1 * 2;
            pu2_ref = pu2_ref - ((w >> 4) << 4) + s_src2 * 2;
        }

        /* Last 1 row */
        if (h & 0x1)
        {
            for (j = 0; j < w; j += 16)
            {
                src2_8x16b_0 = _mm_loadu_si128((__m128i *) (pu2_ref));
                src2_8x16b_1 = _mm_loadu_si128((__m128i *) (pu2_ref + 8));

                src1_8x16b_0 = _mm_loadu_si128((__m128i *) (pu2_inp));
                src1_8x16b_1 = _mm_loadu_si128((__m128i *) (pu2_inp + 8));

                src2_8x16b_0 = _mm_slli_epi16(src2_8x16b_0, 1);
                src2_8x16b_1 = _mm_slli_epi16(src2_8x16b_1, 1);

                src1_8x16b_0 = _mm_sub_epi16(src1_8x16b_0, mm_delta);
                src1_8x16b_1 = _mm_sub_epi16(src1_8x16b_1, mm_delta);

                src1_8x16b_0 = _mm_sub_epi16(src1_8x16b_0, src2_8x16b_0);
                src1_8x16b_1 = _mm_sub_epi16(src1_8x16b_1, src2_8x16b_1);

                src1_8x16b_0 = _mm_abs_epi16(src1_8x16b_0);
                src1_8x16b_1 = _mm_abs_epi16(src1_8x16b_1);

                src1_8x16b_0 = _mm_add_epi16(src1_8x16b_0, src1_8x16b_1);

                src1_8x16b_1 = _mm_srli_si128(src1_8x16b_0, 8);
                src1_8x16b_0 = _mm_cvtepi16_epi32(src1_8x16b_0);
                src1_8x16b_1 = _mm_cvtepi16_epi32(src1_8x16b_1);

                mm_result1 = _mm_add_epi32(mm_result1, src1_8x16b_0);
                mm_result2 = _mm_add_epi32(mm_result2, src1_8x16b_1);

                pu2_inp += 16;
                pu2_ref += 16;
            }
        }
    }

    rem_w &= 0xF;

    /* 8 pixels at a time */
    if (rem_w > 7)
    {
        pu2_inp = (short *)src1 + ((w >> 4) << 4);
        pu2_ref = (short *)src2 + ((w >> 4) << 4);

        for (i = h; i > 3; i -= 4)
        {
            for (j = 0; j < w; j += 8)
            {
                src2_8x16b_0 = _mm_loadu_si128((__m128i *) (pu2_ref));
                src2_8x16b_1 = _mm_loadu_si128((__m128i *) (pu2_ref + s_src2));
                src2_8x16b_2 = _mm_loadu_si128((__m128i *) (pu2_ref + (s_src2 * 2)));
                src2_8x16b_3 = _mm_loadu_si128((__m128i *) (pu2_ref + (s_src2 * 3)));

                src1_8x16b_0 = _mm_loadu_si128((__m128i *) (pu2_inp));
                src1_8x16b_1 = _mm_loadu_si128((__m128i *) (pu2_inp + s_src1));
                src1_8x16b_2 = _mm_loadu_si128((__m128i *) (pu2_inp + (s_src1 * 2)));
                src1_8x16b_3 = _mm_loadu_si128((__m128i *) (pu2_inp + (s_src1 * 3)));

                src2_8x16b_0 = _mm_slli_epi16(src2_8x16b_0, 1);
                src2_8x16b_1 = _mm_slli_epi16(src2_8x16b_1, 1);
                src2_8x16b_2 = _mm_slli_epi16(src2_8x16b_2, 1);
                src2_8x16b_3 = _mm_slli_epi16(src2_8x16b_3, 1);

                src1_8x16b_0 = _mm_sub_epi16(src1_8x16b_0, mm_delta);
                src1_8x16b_1 = _mm_sub_epi16(src1_8x16b_1, mm_delta);
                src1_8x16b_2 = _mm_sub_epi16(src1_8x16b_2, mm_delta);
                src1_8x16b_3 = _mm_sub_epi16(src1_8x16b_3, mm_delta);

                src1_8x16b_0 = _mm_sub_epi16(src1_8x16b_0, src2_8x16b_0);
                src1_8x16b_1 = _mm_sub_epi16(src1_8x16b_1, src2_8x16b_1);
                src1_8x16b_2 = _mm_sub_epi16(src1_8x16b_2, src2_8x16b_2);
                src1_8x16b_3 = _mm_sub_epi16(src1_8x16b_3, src2_8x16b_3);

                src1_8x16b_0 = _mm_abs_epi16(src1_8x16b_0);
                src1_8x16b_1 = _mm_abs_epi16(src1_8x16b_1);
                src1_8x16b_2 = _mm_abs_epi16(src1_8x16b_2);
                src1_8x16b_3 = _mm_abs_epi16(src1_8x16b_3);

                src1_8x16b_0 = _mm_add_epi16(src1_8x16b_0, src1_8x16b_1);
                src1_8x16b_2 = _mm_add_epi16(src1_8x16b_2, src1_8x16b_3);

                src1_8x16b_1 = _mm_srli_si128(src1_8x16b_0, 8);
                src1_8x16b_3 = _mm_srli_si128(src1_8x16b_2, 8);
                src1_8x16b_0 = _mm_cvtepi16_epi32(src1_8x16b_0);
                src1_8x16b_2 = _mm_cvtepi16_epi32(src1_8x16b_2);
                src1_8x16b_1 = _mm_cvtepi16_epi32(src1_8x16b_1);
                src1_8x16b_3 = _mm_cvtepi16_epi32(src1_8x16b_3);

                src1_8x16b_0 = _mm_add_epi32(src1_8x16b_0, src1_8x16b_1);
                src1_8x16b_2 = _mm_add_epi32(src1_8x16b_2, src1_8x16b_3);

                mm_result1 = _mm_add_epi32(mm_result1, src1_8x16b_0);
                mm_result2 = _mm_add_epi32(mm_result2, src1_8x16b_2);

                pu2_inp += 8;
                pu2_ref += 8;
            }

            pu2_inp = pu2_inp - ((rem_w >> 3) << 3) + s_src1 * 4;
            pu2_ref = pu2_ref - ((rem_w >> 3) << 3) + s_src2 * 4;
        }

        /* Rem. Last rows (max 3) */
        for (i = 0; i < (h & 3); i++)
        {
            for (j = 0; j < w; j += 8)
            {
                src2_8x16b_0 = _mm_loadu_si128((__m128i *) (pu2_ref));
                src1_8x16b_0 = _mm_loadu_si128((__m128i *) (pu2_inp));

                src2_8x16b_0 = _mm_slli_epi16(src2_8x16b_0, 1);

                src1_8x16b_0 = _mm_sub_epi16(src1_8x16b_0, mm_delta);
                src1_8x16b_0 = _mm_sub_epi16(src1_8x16b_0, src2_8x16b_0);

                src1_8x16b_0 = _mm_abs_epi16(src1_8x16b_0);

                src1_8x16b_1 = _mm_srli_si128(src1_8x16b_0, 8);
                src1_8x16b_0 = _mm_cvtepi16_epi32(src1_8x16b_0);
                src1_8x16b_1 = _mm_cvtepi16_epi32(src1_8x16b_1);

                mm_result1 = _mm_add_epi32(mm_result1, src1_8x16b_0);
                mm_result2 = _mm_add_epi32(mm_result2, src1_8x16b_1);

                pu2_inp += 8;
                pu2_ref += 8;
            }

            pu2_inp = pu2_inp - ((rem_w >> 3) << 3) + s_src1;
            pu2_ref = pu2_ref - ((rem_w >> 3) << 3) + s_src2;
        }
    }

    rem_w &= 0x7;

    /* 4 pixels at a time */
    if (rem_w > 3)
    {
        pu2_inp = (short *)src1 + ((w >> 3) << 3);
        pu2_ref = (short *)src2 + ((w >> 3) << 3);

        for (i = h; i > 3; i -= 4)
        {
            for (j = 0; j < w; j += 4)
            {
                src2_8x16b_0 = _mm_loadl_epi64((__m128i *) (pu2_ref));
                src2_8x16b_1 = _mm_loadl_epi64((__m128i *) (pu2_ref + s_src2));
                src2_8x16b_2 = _mm_loadl_epi64((__m128i *) (pu2_ref + (s_src2 * 2)));
                src2_8x16b_3 = _mm_loadl_epi64((__m128i *) (pu2_ref + (s_src2 * 3)));

                src1_8x16b_0 = _mm_loadl_epi64((__m128i *) (pu2_inp));
                src1_8x16b_1 = _mm_loadl_epi64((__m128i *) (pu2_inp + s_src1));
                src1_8x16b_2 = _mm_loadl_epi64((__m128i *) (pu2_inp + (s_src1 * 2)));
                src1_8x16b_3 = _mm_loadl_epi64((__m128i *) (pu2_inp + (s_src1 * 3)));

                src2_8x16b_0 = _mm_unpacklo_epi16(src2_8x16b_0, src2_8x16b_1);
                src2_8x16b_2 = _mm_unpacklo_epi16(src2_8x16b_2, src2_8x16b_3);

                src1_8x16b_0 = _mm_unpacklo_epi16(src1_8x16b_0, src1_8x16b_1);
                src1_8x16b_2 = _mm_unpacklo_epi16(src1_8x16b_2, src1_8x16b_3);

                src2_8x16b_0 = _mm_slli_epi16(src2_8x16b_0, 1);
                src2_8x16b_2 = _mm_slli_epi16(src2_8x16b_2, 1);

                src1_8x16b_0 = _mm_sub_epi16(src1_8x16b_0, mm_delta);
                src1_8x16b_2 = _mm_sub_epi16(src1_8x16b_2, mm_delta);

                src1_8x16b_0 = _mm_sub_epi16(src1_8x16b_0, src2_8x16b_0);
                src1_8x16b_2 = _mm_sub_epi16(src1_8x16b_2, src2_8x16b_2);

                src1_8x16b_0 = _mm_abs_epi16(src1_8x16b_0);
                src1_8x16b_2 = _mm_abs_epi16(src1_8x16b_2);

                src1_8x16b_0 = _mm_add_epi16(src1_8x16b_0, src1_8x16b_2);

                src1_8x16b_1 = _mm_srli_si128(src1_8x16b_0, 8);
                src1_8x16b_0 = _mm_cvtepi16_epi32(src1_8x16b_0);
                src1_8x16b_1 = _mm_cvtepi16_epi32(src1_8x16b_1);

                mm_result1 = _mm_add_epi32(mm_result1, src1_8x16b_0);
                mm_result2 = _mm_add_epi32(mm_result2, src1_8x16b_1);

                pu2_inp += 4;
                pu2_ref += 4;
            }

            pu2_inp = pu2_inp - ((rem_w >> 2) << 2) + s_src1 * 4;
            pu2_ref = pu2_ref - ((rem_w >> 2) << 2) + s_src2 * 4;
        }

        /* Rem. Last rows (max 3) */
        for (i = 0; i < (h & 3); i++)
        {
            for (j = 0; j < w; j += 4)
            {
                src2_8x16b_0 = _mm_loadl_epi64((__m128i *) (pu2_ref));
                src1_8x16b_0 = _mm_loadl_epi64((__m128i *) (pu2_inp));

                src2_8x16b_0 = _mm_slli_epi16(src2_8x16b_0, 1);

                src1_8x16b_0 = _mm_sub_epi16(src1_8x16b_0, mm_delta);
                src1_8x16b_0 = _mm_sub_epi16(src1_8x16b_0, src2_8x16b_0);

                src1_8x16b_0 = _mm_abs_epi16(src1_8x16b_0);

                src1_8x16b_0 = _mm_cvtepi16_epi32(src1_8x16b_0);

                mm_result1 = _mm_add_epi32(mm_result1, src1_8x16b_0);

                pu2_inp += 4;
                pu2_ref += 4;
            }

            pu2_inp = pu2_inp - ((rem_w >> 2) << 2) + s_src1;
            pu2_ref = pu2_ref - ((rem_w >> 2) << 2) + s_src2;
        }
    }

    rem_w &= 0x3;
    if (rem_w)
    {
        pu2_inp = (short *)src1 + ((w >> 2) << 2);
        pu2_ref = (short *)src2 + ((w >> 2) << 2);

        for (i = 0; i < h; i++)
        {
            for (j = 0; j < rem_w; j++)
            {
                mr_sad += abs(pu2_inp[j] - (pu2_ref[j] << 1) - delta);
            }
            pu2_inp += s_src1;
            pu2_ref += s_src2;
        }
    }

    mm_result1 = _mm_add_epi32(mm_result1, mm_result2);
    {
        int *val = (int*)&mm_result1;
        mr_sad += (val[0] + val[1] + val[2] + val[3]);
    }

    return (mr_sad);
}

#endif
