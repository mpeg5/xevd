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


#include "xevd_tbl.h"
#include "xevdm_def.h"
#include "xevdm_mc_neon.h"
#include <assert.h>

#if ARM_NEON

#define vmadd_s16(a, b)\
    vpaddq_s32(vmull_s16(vget_low_s16(a), vget_low_s16(b)), vmull_s16(vget_high_s16(a), vget_high_s16(b)));

#define vmadd1_s16(a, coef)\
    vpaddq_s32(vmull_s16(a.val[0], vget_low_s16(coef)), vmull_s16(a.val[1], vget_high_s16(coef)));


static const s8 shuffle_2Tap[16] = { 0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9 };

void mc_filter_bilin_horz_neon(s16 const* ref,
    int src_stride,
    s16* pred,
    int dst_stride,
    const short* coeff,
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
    s16 const* inp_copy;
    s16* dst_copy;

    int16x8_t offset_neon = vdupq_n_s32(offset);
    int16x8_t min_neon = vdupq_n_s16(min_val);
    int16x8_t max_neon = vdupq_n_s16(max_val);

    int16x8_t row1, row11, row2, row22, row3, row33, row4, row44;
    int16x8_t res0, res1, res2, res3;
    int16x8_t coeff0_1_neon;
    int8x16_t shuffle;

    rem_w = width;
    inp_copy = ref;
    dst_copy = pred;

    src_stride2 = (src_stride << 1);
    src_stride3 = (src_stride2 + src_stride);

    coeff0_1_neon = vcombine_s16(vld1_s16(coeff), vcreate_s16(0));
    coeff0_1_neon = vdupq_n_s32(vgetq_lane_s32(coeff0_1_neon, 0));

    int32x4_t shift_neon = vdupq_n_s32(-shift);


    shuffle = vld1q_s8(shuffle_2Tap);

    rem_h = (height & 0x3);

    if (rem_w > 7)
    {
        for (row = height; row > 3; row -= 4)
        {
            int cnt = 0;
            for (col = rem_w; col > 7; col -= 8)
            {
                row1  = vld1q_s16((inp_copy + cnt));
                row11 = vld1q_s16((inp_copy + cnt + 1));
                row2  = vld1q_s16((inp_copy + src_stride + cnt));
                row22 = vld1q_s16((inp_copy + src_stride + cnt + 1));
                row3  = vld1q_s16((inp_copy + src_stride2 + cnt));
                row33 = vld1q_s16((inp_copy + src_stride2 + cnt + 1));
                row4  = vld1q_s16((inp_copy + src_stride3 + cnt));
                row44 = vld1q_s16((inp_copy + src_stride3 + cnt + 1));

                row1 = vmadd_s16(row1, coeff0_1_neon);
                row1 = vaddq_s32(row1, offset_neon);
                row1 = vshlq_s32(row1, shift_neon);

                row11 = vmadd_s16(row11, coeff0_1_neon);
                row11 = vaddq_s32(row11, offset_neon);
                row11 = vshlq_s32(row11, shift_neon);

                row2 = vmadd_s16(row2, coeff0_1_neon);
                row2 = vaddq_s32(row2, offset_neon);
                row2 = vshlq_s32(row2, shift_neon);

                row22 = vmadd_s16(row22, coeff0_1_neon);
                row22 = vaddq_s32(row22, offset_neon);
                row22 = vshlq_s32(row22, shift_neon);

                row3 = vmadd_s16(row3, coeff0_1_neon);
                row3 = vaddq_s32(row3, offset_neon);
                row3 = vshlq_s32(row3, shift_neon);

                row33 = vmadd_s16(row33, coeff0_1_neon);
                row33 = vaddq_s32(row33, offset_neon);
                row33 = vshlq_s32(row33, shift_neon);

                row4 = vmadd_s16(row4, coeff0_1_neon);
                row4 = vaddq_s32(row4, offset_neon);
                row4 = vshlq_s32(row4, shift_neon);

                row44 = vmadd_s16(row44, coeff0_1_neon);
                row44 = vaddq_s32(row44, offset_neon);
                row44 = vshlq_s32(row44, shift_neon);

                row1  = vcombine_s16(vqmovn_s32(row1),  vqmovn_s32(row2));
                row11 = vcombine_s16(vqmovn_s32(row11), vqmovn_s32(row22));
                row3  = vcombine_s16(vqmovn_s32(row3),  vqmovn_s32(row4));
                row33 = vcombine_s16(vqmovn_s32(row33), vqmovn_s32(row44));

                res0 = vzip1q_s16(row1, row11);
                res1 = vzip2q_s16(row1, row11);
                res2 = vzip1q_s16(row3, row33);
                res3 = vzip2q_s16(row3, row33);

                if (is_last)
                {
                    res0 = vminq_s16(res0, max_neon);
                    res0 = vmaxq_s16(res0, min_neon);

                    res1 = vminq_s16(res1, max_neon);
                    res1 = vmaxq_s16(res1, min_neon);

                    res2 = vminq_s16(res2, max_neon);
                    res2 = vmaxq_s16(res2, min_neon);

                    res3 = vminq_s16(res3, max_neon);
                    res3 = vmaxq_s16(res3, min_neon);
                }


                vst1q_s16((dst_copy + cnt), res0);
                vst1q_s16((dst_copy + dst_stride + cnt), res1);
                vst1q_s16((dst_copy + dst_stride * 2 + cnt), res2);
                vst1q_s16((dst_copy + dst_stride * 3 + cnt), res3);

                cnt += 8; /* To pointer updates*/
            }

            inp_copy += (src_stride << 2);
            dst_copy += (dst_stride << 2);
        }

        /*extra height to be done --- one row at a time*/
        for (row = 0; row < rem_h; ++row)
        {
            int cnt = 0;
            for (col = rem_w; col > 7; col -= 8)
            {
                /*load 8 pixel values from row 0*/
                row1 = vld1q_s16((inp_copy + cnt));
                row11 = vld1q_s16((inp_copy + cnt + 1));

                row1 = vmadd_s16(row1, coeff0_1_neon);
                row1 = vaddq_s32(row1, offset_neon);
                row1 = vshlq_s32(row1, shift_neon);

                row11 = vmadd_s16(row11, coeff0_1_neon);
                row11 = vaddq_s32(row11, offset_neon);
                row11 = vshlq_s32(row11, shift_neon);

                /* Pack to 16 bits */
                row1 = vcombine_s16(vqmovn_s32(row1), vqmovn_s32(row11));

                res0 = vzip2q_s64(row1, row1);
                res1 = vzip1q_s16(row1, res0);

                /* Clip */
                if (is_last)
                {
                    res1 = vminq_s16(res1, max_neon);
                    res1 = vmaxq_s16(res1, min_neon);
                }

                /* to store the 8 pixels res. */
                vst1q_s16((dst_copy + cnt), res1);

                cnt += 8;
            }

            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x7;

    if (rem_w > 3)
    {
        inp_copy = ref + ((width >> 3) << 3);
        dst_copy = pred + ((width >> 3) << 3);

        for (row = height; row > 3; row -= 4)
        {
            /*load 8 pixel values from row 0*/
            row1 = vld1q_s16((inp_copy));
            row2 = vld1q_s16((inp_copy + src_stride));
            row3 = vld1q_s16((inp_copy + src_stride2));
            row4 = vld1q_s16((inp_copy + src_stride3));

            row1 = vqtbl1q_s8(row1, vandq_u8(shuffle, vdupq_n_u8(0x8F)));
            row2 = vqtbl1q_s8(row2, vandq_u8(shuffle, vdupq_n_u8(0x8F)));
            row3 = vqtbl1q_s8(row3, vandq_u8(shuffle, vdupq_n_u8(0x8F)));
            row4 = vqtbl1q_s8(row4, vandq_u8(shuffle, vdupq_n_u8(0x8F)));

            row1 = vmadd_s16(row1, coeff0_1_neon);
            row1 = vaddq_s32(row1, offset_neon);
            row1 = vshlq_s32(row1, shift_neon);

            row2 = vmadd_s16(row2, coeff0_1_neon);
            row2 = vaddq_s32(row2, offset_neon);
            row2 = vshlq_s32(row2, shift_neon);

            row3 = vmadd_s16(row3, coeff0_1_neon);
            row3 = vaddq_s32(row3, offset_neon);
            row3 = vshlq_s32(row3, shift_neon);

            row4 = vmadd_s16(row4, coeff0_1_neon);
            row4 = vaddq_s32(row4, offset_neon);
            row4 = vshlq_s32(row4, shift_neon);

            /* Pack to 16 bits */
            res0 = vcombine_s16(vqmovn_s32(row1), vqmovn_s32(row2));
            res1 = vcombine_s16(vqmovn_s32(row3), vqmovn_s32(row4));

            /* Clip */
            if (is_last)
            {
                res0 = vminq_s16(res0, max_neon);
                res0 = vmaxq_s16(res0, min_neon);

                res1 = vminq_s16(res1, max_neon);
                res1 = vmaxq_s16(res1, min_neon);
            }
            /* Store */
            vst1_s16((dst_copy), vget_low_s16(res0));
            vst1_s16((dst_copy + dst_stride * 2), vget_low_s16(res1));
            vst1_s16((dst_copy + dst_stride), vget_low_s16(vzip2q_s64(res0, res0)));
            vst1_s16((dst_copy + dst_stride * 3), vget_low_s16(vzip2q_s64(res1, res1)));

            inp_copy += (src_stride << 2);
            dst_copy += (dst_stride << 2);
        }

        for (row = 0; row < rem_h; ++row)
        {
            /*load 8 pixel values from row 0*/
            row1 = vld1q_s16((inp_copy));

            res0 = vqtbl1q_s8(row1, vandq_u8(shuffle, vdupq_n_u8(0x8F)));
            
            res0 = vmadd_s16(res0, coeff0_1_neon);
            res0 = vaddq_s32(res0, offset_neon);
            res0 = vshlq_s32(res0, shift_neon);
            
            /* Pack to 16 bits */
            res0 = vcombine_s16(vqmovn_s32(res0), vqmovn_s32(res0));
            
            /* Clip */
            if (is_last)
            {
                res0 = vminq_s16(res0, max_neon);
                res0 = vmaxq_s16(res0, min_neon);
            }
            /* Store */
            vst1_s16((dst_copy), vget_low_s16(res0));
            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x3;

    if (rem_w)
    {
        int sum, sum1;

        inp_copy = ref + ((width >> 2) >> 2);
        dst_copy = pred + ((width >> 2) >> 2);

        for (row = height; row > 3; row -= 4)
        {
            for (col = 0; col < rem_w; ++col)
            {
                row1 = vld1q_s16((inp_copy + col));
                row2 = vld1q_s16((inp_copy + src_stride + col));
                row3 = vld1q_s16((inp_copy + src_stride2 + col));
                row4 = vld1q_s16((inp_copy + src_stride3 + col));

                row1 = vzip1q_s32(row1, row2);
                row3 = vzip1q_s32(row3, row4);
                row1 = vzip1q_s64(row1, row3);

                row1 = vmadd_s16(row1, coeff0_1_neon);
                row1 = vaddq_s32(row1, offset_neon);
                row1 = vshlq_s32(row1, shift_neon);

                /* Pack to 16 bits */
                res0 = vcombine_s16(vqmovn_s32(row1), vqmovn_s32(row1));

                /* Clip */
                if (is_last)
                {
                    res0 = vminq_s16(res0, max_neon);
                    res0 = vmaxq_s16(res0, min_neon);
                }

                sum = vgetq_lane_s32(res0, 0);
                sum1 = vgetq_lane_s32(res0, 1);

                dst_copy[col] = (s16)(sum & 0xffff);
                dst_copy[col + dst_stride] = (s16)(sum >> 16);
                dst_copy[col + (dst_stride << 1)] = (s16)(sum1 & 0xffff);
                dst_copy[col + (dst_stride * 3)] = (s16)(sum1 >> 16);
            }
            inp_copy += (src_stride << 2);
            dst_copy += (dst_stride << 2);
        }

        for (row = 0; row < rem_h; ++row)
        {
            for (col = 0; col < rem_w; ++col)
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

void mc_filter_bilin_vert_neon(s16 const* ref,
    int src_stride,
    s16* pred,
    int dst_stride,
    const short* coeff,
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
    s16 const* inp_copy;
    s16* dst_copy;

    int16x8_t offset_neon = vdupq_n_s32(offset);
    int16x8_t min_neon = vdupq_n_s16(min_val);
    int16x8_t max_neon = vdupq_n_s16(max_val);

    int16x8_t row1, row11, row2, row22, row3, row33, row4, row44, row5;
    int16x8_t res0, res1, res2, res3;
    int16x8_t coeff0_1_neon;

    rem_w = width;
    inp_copy = ref;
    dst_copy = pred;

    src_stride2 = (src_stride << 1);
    src_stride3 = (src_stride * 3);
    src_stride4 = (src_stride << 2);

    int32x4_t shift_neon = vdupq_n_s32(-shift);
    coeff0_1_neon = vcombine_s16(vld1_s16(coeff), vcreate_s16(0));
    coeff0_1_neon = vdupq_n_s32(vgetq_lane_s32(coeff0_1_neon, 0));
        
    rem_h = height & 0x3;

    if (rem_w > 7)
    {
        for (row = height; row > 3; row -= 4)
        {
            int cnt = 0;
            for (col = rem_w; col > 7; col -= 8)
            {
                row1 = vld1q_s16((inp_copy + cnt));
                row2 = vld1q_s16((inp_copy + src_stride + cnt));
                row3 = vld1q_s16((inp_copy + src_stride2 + cnt));
                row4 = vld1q_s16((inp_copy + src_stride3 + cnt));
                row5 = vld1q_s16((inp_copy + src_stride4 + cnt));

                row11 = vzip1q_s16(row1, row2);
                row11 = vmadd_s16(row11, coeff0_1_neon);
                row11 = vaddq_s32(row11, offset_neon);
                row11 = vshlq_s32(row11, shift_neon);

                row1 = vzip2q_s16(row1, row2);
                row1 = vmadd_s16(row1, coeff0_1_neon);
                row1 = vaddq_s32(row1, offset_neon);
                row1 = vshlq_s32(row1, shift_neon);

                row22 = vzip1q_s16(row2, row3);
                row22 = vmadd_s16(row22, coeff0_1_neon);
                row22 = vaddq_s32(row22, offset_neon);
                row22 = vshlq_s32(row22, shift_neon);

                row2 = vzip2q_s16(row2, row3);
                row2 = vmadd_s16(row2, coeff0_1_neon);
                row2 = vaddq_s32(row2, offset_neon);
                row2 = vshlq_s32(row2, shift_neon);

                row33 = vzip1q_s16(row3, row4);
                row33 = vmadd_s16(row33, coeff0_1_neon);
                row33 = vaddq_s32(row33, offset_neon);
                row33 = vshlq_s32(row33, shift_neon);

                row3 = vzip2q_s16(row3, row4);
                row3 = vmadd_s16(row3, coeff0_1_neon);
                row3 = vaddq_s32(row3, offset_neon);
                row3 = vshlq_s32(row3, shift_neon);

                row44 = vzip1q_s16(row4, row5);
                row44 = vmadd_s16(row44, coeff0_1_neon);
                row44 = vaddq_s32(row44, offset_neon);
                row44 = vshlq_s32(row44, shift_neon);

                row4 = vzip2q_s16(row4, row5);
                row4 = vmadd_s16(row4, coeff0_1_neon);
                row4 = vaddq_s32(row4, offset_neon);
                row4 = vshlq_s32(row4, shift_neon);

                /* Pack to 16 bits */
                res0 = vcombine_s16(vqmovn_s32(row11), vqmovn_s32(row1));
                res1 = vcombine_s16(vqmovn_s32(row22), vqmovn_s32(row2));
                res2 = vcombine_s16(vqmovn_s32(row33), vqmovn_s32(row3));
                res3 = vcombine_s16(vqmovn_s32(row44), vqmovn_s32(row4));

                /* Clip */
                if (is_last)
                {
                    res0 = vminq_s16(res0, max_neon);
                    res1 = vminq_s16(res1, max_neon);
                    res2 = vminq_s16(res2, max_neon);
                    res3 = vminq_s16(res3, max_neon);

                    res0 = vmaxq_s16(res0, min_neon);
                    res1 = vmaxq_s16(res1, min_neon);
                    res2 = vmaxq_s16(res2, min_neon);
                    res3 = vmaxq_s16(res3, min_neon);
                }

                /* Store */
                vst1q_s16((dst_copy + cnt), res0);
                vst1q_s16((dst_copy + dst_stride + cnt), res1);
                vst1q_s16((dst_copy + dst_stride * 2 + cnt), res2);
                vst1q_s16((dst_copy + dst_stride * 3 + cnt), res3);

                cnt += 8;  /* To pointer updates*/
            }

            inp_copy += (src_stride << 2);
            dst_copy += (dst_stride << 2);
        }

        /*extra height to be done --- one row at a time*/
        for (row = 0; row < rem_h; ++row)
        {
            int cnt = 0;
            for (col = rem_w; col > 7; col -= 8)
            {
                /*load 8 pixel values from row 0*/
                row1 = vld1q_s16((inp_copy + cnt));
                row2 = vld1q_s16((inp_copy + src_stride + cnt));

                row11 = vzip1q_s16(row1, row2);
                row11 = vmadd_s16(row11, coeff0_1_neon);
                row11 = vaddq_s32(row11, offset_neon);
                row11 = vshlq_s32(row11, shift_neon);

                row1 = vzip2q_s16(row1, row2);
                row1 = vmadd_s16(row1, coeff0_1_neon);
                row1 = vaddq_s32(row1, offset_neon);
                row1 = vshlq_s32(row1, shift_neon);

                /* Pack to 16 bits */
                res1 = vcombine_s16(vqmovn_s32(row11), vqmovn_s32(row1));

                /* Clip */
                if (is_last)
                {
                    res1 = vminq_s16(res1, max_neon);
                    res1 = vmaxq_s16(res1, min_neon);
                }

                /* Store */
                vst1q_s16((dst_copy + cnt), res1);

                cnt += 8;
            }

            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x7;

    if (rem_w > 3)
    {
        inp_copy = ref + ((width >> 3) << 3);
        dst_copy = pred + ((width >> 3) << 3);

        for (row = height; row > 3; row -= 4)
        {
            /*load 4 pixel values */
            row1 = vcombine_s16(vld1_s16((inp_copy)), vcreate_s16(0));
            row2 = vcombine_s16(vld1_s16((inp_copy + src_stride)), vcreate_s16(0));
            row3 = vcombine_s16(vld1_s16((inp_copy + src_stride2)), vcreate_s16(0));
            row4 = vcombine_s16(vld1_s16((inp_copy + src_stride3)), vcreate_s16(0));
            row5 = vcombine_s16(vld1_s16((inp_copy + src_stride4)), vcreate_s16(0));

            row11 = vzip1q_s16(row1, row2);
            row11 = vmadd_s16(row11, coeff0_1_neon);
            row11 = vaddq_s32(row11, offset_neon);
            row11 = vshlq_s32(row11, shift_neon);

            row22 = vzip1q_s16(row2, row3);
            row22 = vmadd_s16(row22, coeff0_1_neon);
            row22 = vaddq_s32(row22, offset_neon);
            row22 = vshlq_s32(row22, shift_neon);

            row33 = vzip1q_s16(row3, row4);
            row33 = vmadd_s16(row33, coeff0_1_neon);
            row33 = vaddq_s32(row33, offset_neon);
            row33 = vshlq_s32(row33, shift_neon);

            row44 = vzip1q_s16(row4, row5);
            row44 = vmadd_s16(row44, coeff0_1_neon);
            row44 = vaddq_s32(row44, offset_neon);
            row44 = vshlq_s32(row44, shift_neon);

            res0 = vcombine_s16(vqmovn_s32(row11), vqmovn_s32(row22));
            res1 = vcombine_s16(vqmovn_s32(row33), vqmovn_s32(row44));

            if (is_last)
            {
                res0 = vminq_s16(res0, max_neon);
                res1 = vminq_s16(res1, max_neon);
                res0 = vmaxq_s16(res0, min_neon);
                res1 = vmaxq_s16(res1, min_neon);
            }

            /* to store the 8 pixels res. */
            vst1_s16((dst_copy), vget_low_s16(res0));
            vst1_s16((dst_copy + dst_stride), vget_low_s16(vzip2q_s64(res0, res0)));
            vst1_s16((dst_copy + dst_stride * 2), vget_low_s16(res1));
            vst1_s16((dst_copy + dst_stride * 3), vget_low_s16(vzip2q_s64(res1, res1)));

            inp_copy += (src_stride << 2);
            dst_copy += (dst_stride << 2);
        }

        for (row = 0; row < rem_h; ++row)
        {
            row1 = vcombine_s16(vld1_s16(inp_copy), vcreate_s16(0));
            row2 = vcombine_s16(vld1_s16(inp_copy + src_stride), vcreate_s16(0));

            row11 = vzip1q_s16(row1, row2);
            row11 = vmadd_s16(row11, coeff0_1_neon);
            row11 = vaddq_s32(row11, offset_neon);

            row11 = vshlq_s32(row11, shift_neon);

            /* Pack to 16 bits */
            row11 = vcombine_s16(vqmovn_s32(row11), vqmovn_s32(row11));

            /* Clip */
            if (is_last)
            {
                row11 = vminq_s16(row11, max_neon);
                row11 = vmaxq_s16(row11, min_neon);
            }
            /* Store */
            vst1_s16((dst_copy), vget_low_s16(row11));

            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x3;

    if (rem_w)
    {
        inp_copy = ref + ((width >> 2) >> 2);
        dst_copy = pred + ((width >> 2) >> 2);

        for (row = 0; row < height; ++row)
        {
            for (col = 0; col < rem_w; ++col)
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



void xevd_mc_dmvr_l_00_neon(pel* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel* pred, int w, int h, int bit_depth)
{
    int i, j;

    gmv_x >>= 4;
    gmv_y >>= 4;

    if (((w & 0x7) == 0) && ((h & 1) == 0))
    {
        int16x8_t m00, m01;

        for (i = 0; i < h; i += 2)
        {
            for (j = 0; j < w; j += 8)
            {
                m00 = vld1q_s16((ref + j));
                m01 = vld1q_s16((ref + j + s_ref));

                vst1q_s16((pred + j), m00);
                vst1q_s16((pred + j + s_pred), m01);
            }
            pred += (s_pred << 1);
            ref += (s_ref << 1);
        }
    }
    else if ((w & 0x3) == 0)
    {
        int16x4_t m00;

        for (i = 0; i < h; ++i)
        {
            for (j = 0; j < w; j += 4)
            {
                m00 = vld1_s16(ref + j);
                vst1_s16((pred + j), (m00));
            }
            pred += s_pred;
            ref += s_ref;
        }
    }
    else
    {
        for (i = 0; i < h; ++i)
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

void xevd_mc_dmvr_l_n0_neon(pel* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel* pred, int w, int h, int bit_depth)
{
    int dx;

    dx = gmv_x & 15;
    ref = ref - 3;

    int max = ((1 << bit_depth) - 1);

    int min = 0;
    xevd_mc_filter_l_8pel_horz_clip_neon(ref, s_ref, pred, s_pred, tbl_mc_l_coeff[dx], w, h, min, max, MAC_ADD_N0, MAC_SFT_N0);
}

void xevd_mc_dmvr_l_0n_neon(pel* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel* pred, int w, int h, int bit_depth)
{
    int dy;

    dy = gmv_y & 15;
    ref = ref - (3 * s_ref);

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    xevd_mc_filter_l_8pel_vert_clip_neon(ref, s_ref, pred, s_pred, tbl_mc_l_coeff[dy], w, h, min, max, MAC_ADD_0N, MAC_SFT_0N);
}

void xevd_mc_dmvr_l_nn_neon(s16* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16* pred, int w, int h, int bit_depth)
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_L) * MAX_CU_SIZE];
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

    xevd_mc_filter_l_8pel_horz_no_clip_neon(ref, s_ref, buf, w, tbl_mc_l_coeff[dx], w, (h + 7), offset1, shift1);
    xevd_mc_filter_l_8pel_vert_clip_neon(buf, w, pred, s_pred, tbl_mc_l_coeff[dy], w, h, min, max, offset2, shift2);
}


void xevdm_bl_mc_l_00_neon(pel* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel* pred, int w, int h, int bit_depth)
{
    int i, j;

    gmv_x >>= 4;
    gmv_y >>= 4;

    ref += gmv_y * s_ref + gmv_x;

    if (((w & 0x7) == 0) && ((h & 1) == 0))
    {
        int16x8_t m00, m01;

        for (i = 0; i < h; i += 2)
        {
            for (j = 0; j < w; j += 8)
            {
                m00 = vld1q_s16((ref + j));
                m01 = vld1q_s16((ref + j + s_ref));

                vst1q_s16((pred + j), m00);
                vst1q_s16((pred + j + s_pred), m01);
            }
            pred += (s_pred << 1);
            ref += (s_ref << 1);
        }
    }
    else if ((w & 0x3) == 0)
    {
        int16x4_t m00;

        for (i = 0; i < h; ++i)
        {
            for (j = 0; j < w; j += 4)
            {
                m00 = vld1_s16(ref + j);
                vst1_s16((pred + j), (m00));
            }
            pred += s_pred;
            ref += s_ref;
        }
    }
    else
    {
        for (i = 0; i < h; ++i)
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

void xevdm_bl_mc_l_n0_neon(pel* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel* pred, int w, int h, int bit_depth)
{
    int dx;

    dx = gmv_x & 15;
    ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4);

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_bilin_horz_neon(ref, s_ref, pred, s_pred, xevd_tbl_bl_mc_l_coeff[dx], w, h, min, max, MAC_ADD_N0, MAC_SFT_N0, 1);
}

void xevdm_bl_mc_l_0n_neon(pel* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel* pred, int w, int h, int bit_depth)
{
    int dy;

    dy = gmv_y & 15;
    ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4);

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_bilin_vert_neon(ref, s_ref, pred, s_pred, xevd_tbl_bl_mc_l_coeff[dy], w, h, min, max, MAC_ADD_0N, MAC_SFT_0N, 1);
}

void xevdm_bl_mc_l_nn_neon(s16* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16* pred, int w, int h, int bit_depth)
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_L) * (MAX_CU_SIZE + MC_IBUF_PAD_L)];
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

    mc_filter_bilin_horz_neon(ref, s_ref, buf, w, xevd_tbl_bl_mc_l_coeff[dx], w, (h + 1), min, max, offset1, shift1, 0);
    mc_filter_bilin_vert_neon(buf, w, pred, s_pred, xevd_tbl_bl_mc_l_coeff[dy], w, h, min, max, offset2, shift2, 1);
}

/****************************************************************************
* motion compensation for chroma
****************************************************************************/


void xevd_mc_dmvr_c_00_neon(s16* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16* pred, int w, int h, int bit_depth)
{
    int i, j;

    gmv_x >>= 5;
    gmv_y >>= 5;

    if (((w & 0x7) == 0) && ((h & 1) == 0))
    {
        int16x8_t m00, m01;

        for (i = 0; i < h; i += 2)
        {
            for (j = 0; j < w; j += 8)
            {
                m00 = vld1q_s16((ref + j));
                m01 = vld1q_s16((ref + j + s_ref));

                vst1q_s16((pred + j), m00);
                vst1q_s16((pred + j + s_pred), m01);
            }
            pred += s_pred * 2;
            ref += s_ref * 2;
        }
    }
    else if (((w & 0x3) == 0))
    {
        int16x4_t m00;

        for (i = 0; i < h; ++i)
        {
            for (j = 0; j < w; j += 4)
            {
                m00 = vld1_s16(ref + j);
                vst1_s16((pred + j), (m00));
            }
            pred += s_pred;
            ref += s_ref;
        }
    }
    else
    {
        for (i = 0; i < h; ++i)
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

void xevd_mc_dmvr_c_n0_neon(s16* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16* pred, int w, int h, int bit_depth)
{
    int       dx;

    dx = gmv_x & 31;
    ref -= 1;

    int max = ((1 << bit_depth) - 1);
    int min = 0;
    xevd_mc_filter_c_4pel_horz_neon(ref, s_ref, pred, s_pred, tbl_mc_c_coeff[dx], w, h, min, max, MAC_ADD_N0, MAC_SFT_N0, 1);
}

void xevd_mc_dmvr_c_0n_neon(s16* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16* pred, int w, int h, int bit_depth)
{
    int dy;

    dy = gmv_y & 31;
    ref -= 1 * s_ref;

    int max = ((1 << bit_depth) - 1);
    int min = 0;
    xevd_mc_filter_c_4pel_vert_neon(ref, s_ref, pred, s_pred, tbl_mc_c_coeff[dy], w, h, min, max, MAC_ADD_0N, MAC_SFT_0N, 1);
}

void xevd_mc_dmvr_c_nn_neon(s16* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16* pred, int w, int h, int bit_depth)
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_C) * MAX_CU_SIZE];
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
    xevd_mc_filter_c_4pel_horz_neon(ref, s_ref, buf, w, tbl_mc_c_coeff[dx], w, (h + 3), min, max, offset1, shift1, 0);
    xevd_mc_filter_c_4pel_vert_neon(buf, w, pred, s_pred, tbl_mc_c_coeff[dy], w, h, min, max, offset2, shift2, 1);
}

XEVD_MC_L xevdm_tbl_dmvr_mc_l_neon[2][2] =
{
    {
        xevd_mc_dmvr_l_00_neon, /* dx == 0 && dy == 0 */
        xevd_mc_dmvr_l_0n_neon  /* dx == 0 && dy != 0 */
    },
    {
        xevd_mc_dmvr_l_n0_neon, /* dx != 0 && dy == 0 */
        xevd_mc_dmvr_l_nn_neon  /* dx != 0 && dy != 0 */
    }
};

XEVD_MC_C xevdm_tbl_dmvr_mc_c_neon[2][2] =
{
    {
        xevd_mc_dmvr_c_00_neon, /* dx == 0 && dy == 0 */
        xevd_mc_dmvr_c_0n_neon  /* dx == 0 && dy != 0 */
    },
    {
        xevd_mc_dmvr_c_n0_neon, /* dx != 0 && dy == 0 */
        xevd_mc_dmvr_c_nn_neon  /* dx != 0 && dy != 0 */
    }
};

/* luma and chroma will remain the same */
XEVD_MC_L xevdm_tbl_bl_mc_l_neon[2][2] =
{
    {
        xevdm_bl_mc_l_00_neon,
        xevdm_bl_mc_l_0n_neon
    },
    {
        xevdm_bl_mc_l_n0_neon,
        xevdm_bl_mc_l_nn_neon
    }
};

#undef vmadd_s16
#undef vmadd1_s16
#endif
