/* Copyright (c) 2020, Samsung Electronics Co., Ltd.
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

#include "xevd_dbk_neon.h"

void deblock_scu_hor_neon(pel *buf, int st, int stride, int bit_depth_minus8, int chroma_format_idc)
{
    //size assumed 4x4
    int16x8_t AA, BB, CC, DD;
    int16x8_t t1, t2;
    int16x8_t d, abs_d, sign, clip, sst;
    int16x8_t zero, MAX;
    int16x8_t d1, d2, t16, ad;
    int16x8_t tmp;

    zero = vdupq_n_s16(0);
    MAX = vdupq_n_s16((1 << (bit_depth_minus8 + 8)) - 1);
    sst = vdupq_n_s16(st);

    AA = vcombine_s16(vld1_s16(&buf[-2 * stride]), vcreate_s16(0));
    BB = vcombine_s16(vld1_s16(&buf[-stride]), vcreate_s16(0));
    CC = vcombine_s16(vld1_s16(&buf[0]), vcreate_s16(0));
    DD = vcombine_s16(vld1_s16(&buf[stride]), vcreate_s16(0));


    t1 = vshlq_s16(BB, vdupq_n_s16(2));
    t2 = vshlq_s16(CC, vdupq_n_s16(2));
    t1 = vsubq_s16(AA, t1);
    t2 = vsubq_s16(t2, DD);
    d = vaddq_s16(t1, t2);
    abs_d = vabsq_s16(d);
    abs_d = vshlq_u16(abs_d, vdupq_n_s16(-3));

    t16 = vsubq_s16(abs_d, sst);
    t16 = vshlq_s16(t16, vdupq_n_s16(1));
    t16 = vmaxq_s16(zero, t16);
    clip = vsubq_s16(abs_d, t16);
    clip = vmaxq_s16(zero, clip);
    uint16x8_t lt_mask = vshrq_n_s16(d, 15); // get sign
    uint16x8_t zeromask = vceqzq_s16(d);

    d1 = vbicq_s16(vbslq_s16(lt_mask, vnegq_s16(clip), clip), zeromask);

    clip = vshlq_u16(clip, vdupq_n_s16(-1));
    d2 = vsubq_s16(AA, DD);

    ad = vabsq_s16(d2);
    ad = vshlq_u16(ad, vdupq_n_s16(-2));
    ad = vminq_s16(ad, clip);
    lt_mask = vshrq_n_s16(d2, 15); // get sign
    zeromask = vceqzq_s16(d2);

    d2 = vbicq_s16(vbslq_s16(lt_mask, vnegq_s16(ad), ad), zeromask);

    AA = vsubq_s16(AA, d2);
    BB = vaddq_s16(BB, d1);
    CC = vsubq_s16(CC, d1);
    DD = vaddq_s16(DD, d2);

    
    tmp = vmaxq_s16(zero, AA);
    AA = vminq_s16(tmp, MAX);
    tmp = vmaxq_s16(zero, BB);
    BB = vminq_s16(tmp, MAX);
    tmp = vmaxq_s16(zero, CC);
    CC = vminq_s16(tmp, MAX);
    tmp = vmaxq_s16(zero, DD);
    DD = vminq_s16(tmp, MAX);

    vst1_s16(&buf[-2 * stride], vget_low_s16(AA));
    vst1_s16(&buf[-stride], vget_low_s16(BB));
    vst1_s16(&buf[0], vget_low_s16(CC));
    vst1_s16(&buf[stride], vget_low_s16(DD));
}

void deblock_scu_hor_chroma_neon(pel *u, pel *v, int st_u, int st_v, int stride, int bit_depth_minus8, int chroma_format_idc)
{
    int size;

    size = MIN_CU_SIZE >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));

    int16x8_t Au, Bu, Cu, Du, Av, Bv, Cv, Dv, AA, BB, CC, DD;
    int16x8_t t1, t2;
    int16x8_t d1, d2, t16, ad;
    int16x8_t d, abs_d, sign, clip, sst1, sst;
    int16x8_t zero, MAX;
    int16x8_t tmp;

    zero = vdupq_n_s16(0);
    MAX = vdupq_n_s16((1 << (bit_depth_minus8 + 8)) - 1);
    sst = vdupq_n_s16(st_u);
    sst1 = vdupq_n_s16(st_v);

    if (st_u)
    {
        Au = vcombine_s16(vld1_s16(&u[-2 * stride]), vcreate_s16(0));
        Bu = vcombine_s16(vld1_s16(&u[-stride]), vcreate_s16(0));
        Cu = vcombine_s16(vld1_s16(&u[0]), vcreate_s16(0));
        Du = vcombine_s16(vld1_s16(&u[stride]), vcreate_s16(0));
    }
    if (st_v)
    {
        Av = vcombine_s16(vld1_s16(&v[-2 * stride]), vcreate_s16(0));
        Bv = vcombine_s16(vld1_s16(&v[-stride]), vcreate_s16(0));
        Cv = vcombine_s16(vld1_s16(&v[0]), vcreate_s16(0));
        Dv = vcombine_s16(vld1_s16(&v[stride]), vcreate_s16(0));
    }

    AA = vzip1q_s64(Au, Av);
    BB = vzip1q_s64(Bu, Bv);
    CC = vzip1q_s64(Cu, Cv);
    DD = vzip1q_s64(Du, Dv);

    sst = vzip1q_s64(sst, sst1);
    t1 = vshlq_s16(BB, vdupq_n_s16(2));
    t2 = vshlq_s16(CC, vdupq_n_s16(2));
    t1 = vsubq_s16(AA, t1);
    t2 = vsubq_s16(t2, DD);
    d = vaddq_s16(t1, t2);
    abs_d = vabsq_s16(d);
    abs_d = vshlq_u16(abs_d, vdupq_n_s16(-3));

    t16 = vsubq_s16(abs_d, sst);
    t16 = vshlq_s16(t16, vdupq_n_s16(1));
    t16 = vmaxq_s16(zero, t16);
    clip = vsubq_s16(abs_d, t16);
    clip = vmaxq_s16(zero, clip);
    uint16x8_t lt_mask = vshrq_n_s16(d, 15); // get sign
    uint16x8_t zeromask = vceqzq_s16(d);

    d1 = vbicq_s16(vbslq_s16(lt_mask, vnegq_s16(clip), clip), zeromask);

    BB = vaddq_s16(BB, d1);
    CC = vsubq_s16(CC, d1);

    
    tmp = vmaxq_s16(zero, BB);
    BB = vminq_s16(tmp, MAX);
    tmp = vmaxq_s16(zero, CC);
    CC = vminq_s16(tmp, MAX);

    
    Bv = vcombine_s16(vget_high_s16(BB), vcreate_s16(0));
    Cv = vcombine_s16(vget_high_s16(CC), vcreate_s16(0));
    
    if (st_u)
    {
        if (size == 2)
        {
            u[-stride] = vgetq_lane_u16(BB, 0);
            u[-stride + 1] = vgetq_lane_u16(BB, 1);
            u[0] = vgetq_lane_u16(CC, 0);
            u[1] = vgetq_lane_u16(CC, 1);
        }
        else if (size == 4)
        {
            vst1_s16(&u[-stride], vget_low_s16(BB));
            vst1_s16(&u[0], vget_low_s16(CC));
        }

    }
    if (st_v)
    {
        if (size == 2)
        {
            v[-stride] = vgetq_lane_u16(Bv, 0);
            v[-stride + 1] = vgetq_lane_u16(Bv, 1);
            v[0] = vgetq_lane_u16(Cv, 0);
            v[1] = vgetq_lane_u16(Cv, 1);
        }
        else if (size == 4)
        {
            vst1_s16(&v[-stride], vget_low_s16(Bv));
            vst1_s16(&v[0], vget_low_s16(Cv));
        }

    }
}

void deblock_scu_ver_neon(pel *buf, int st, int stride, int bit_depth_minus8, int chroma_format_idc)
{
    //size assumes 4x4
    int16x8_t AA, BB, CC, DD;
    int16x8_t t1, t2;
    int16x8_t d, abs_d, sign, clip, sst;
    int16x8_t d1, d2, t16, ad;
    int16x8_t zero, MAX;
    int16x8_t tmp;

    zero = vdupq_n_s16(0);
    MAX = vdupq_n_s16((1 << (bit_depth_minus8 + 8)) - 1);
    sst = vdupq_n_s16(st);

    AA = vcombine_s16(vld1_s16(&buf[-2]), vcreate_s16(0));
    BB = vcombine_s16(vld1_s16(&buf[stride - 2]), vcreate_s16(0));
    CC = vcombine_s16(vld1_s16(&buf[2 * stride - 2]), vcreate_s16(0));
    DD = vcombine_s16(vld1_s16(&buf[3 * stride - 2]), vcreate_s16(0));

    t1 = vzip1q_s16(AA, BB);
    t2 = vzip1q_s16(CC, DD);
    AA = vzip1q_s32(t1, t2);

    BB = vcombine_s16(vget_high_s16(AA), vcreate_s16(0));
    CC = vzip2q_s32(t1, t2);
    DD = vcombine_s16(vget_high_s16(CC), vcreate_s16(0));

    t1 = vshlq_s16(BB, vdupq_n_s16(2));
    t2 = vshlq_s16(CC, vdupq_n_s16(2));
    t1 = vsubq_s16(AA, t1);
    t2 = vsubq_s16(t2, DD);
    d = vaddq_s16(t1, t2);
    abs_d = vabsq_s16(d);
    abs_d = vshlq_u16(abs_d, vdupq_n_s16(-3));

    t16 = vsubq_s16(abs_d, sst);
    t16 = vshlq_s16(t16, vdupq_n_s16(1));
    t16 = vmaxq_s16(zero, t16);
    clip = vsubq_s16(abs_d, t16);
    clip = vmaxq_s16(zero, clip);
    uint16x8_t lt_mask = vshrq_n_s16(d, 15); // get sign
    uint16x8_t zeromask = vceqzq_s16(d);

    d1 = vbicq_s16(vbslq_s16(lt_mask, vnegq_s16(clip), clip), zeromask);

    clip = vshlq_u16(clip, vdupq_n_s16(-1));
    d2 = vsubq_s16(AA, DD);

    ad = vabsq_s16(d2);
    ad = vshlq_u16(ad, vdupq_n_s16(-2));
    ad = vminq_s16(ad, clip);

    lt_mask = vshrq_n_s16(d2, 15); // get sign
    zeromask = vceqzq_s16(d2);

    d2 = vbicq_s16(vbslq_s16(lt_mask, vnegq_s16(ad), ad), zeromask);

    AA = vsubq_s16(AA, d2);
    BB = vaddq_s16(BB, d1);
    CC = vsubq_s16(CC, d1);
    DD = vaddq_s16(DD, d2);


    tmp = vmaxq_s16(zero, AA);
    AA = vminq_s16(tmp, MAX);
    tmp = vmaxq_s16(zero, BB);
    BB = vminq_s16(tmp, MAX);
    tmp = vmaxq_s16(zero, CC);
    CC = vminq_s16(tmp, MAX);
    tmp = vmaxq_s16(zero, DD);
    DD = vminq_s16(tmp, MAX);

    t1 = vzip1q_s16(AA, BB);
    t2 = vzip1q_s16(CC, DD);
    AA = vzip1q_s32(t1, t2);
    BB = vcombine_s16(vget_high_s16(AA), vcreate_s16(0));
    CC = vzip2q_s32(t1, t2);
    DD = vcombine_s16(vget_high_s16(CC), vcreate_s16(0));

    /* Store the results */
    vst1_s16(&buf[-2], vget_low_s16(AA));
    vst1_s16(&buf[stride - 2], vget_low_s16(BB));
    vst1_s16(&buf[2 * stride - 2], vget_low_s16(CC));
    vst1_s16(&buf[3 * stride - 2], vget_low_s16(DD));
}

void deblock_scu_ver_chroma_neon(pel* u, pel* v, int st_u, int st_v, int stride, int bit_depth_minus8, int chroma_format_idc)
{
    int size;

    size = MIN_CU_SIZE >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));
    if (size == 2)
    {
        int16x8_t AA, BB, CC, DD;
        if (st_u)
        {
            AA = vcombine_s16(vld1_s16(&u[-2]), vcreate_s16(0));
            BB = vcombine_s16(vld1_s16(&u[stride - 2]), vcreate_s16(0));
        }
        if (st_v)
        {
            CC = vcombine_s16(vld1_s16(&v[-2]), vcreate_s16(0));
            DD = vcombine_s16(vld1_s16(&v[stride - 2]), vcreate_s16(0));
        }
        int16x8_t t1, t2;
        t1 = vzip1q_s16(AA, BB);
        t2 = vzip1q_s16(CC, DD);
        AA = vzip1q_s32(t1, t2);
        BB = vcombine_s16(vget_high_s16(AA), vcreate_s16(0));
        CC = vzip2q_s32(t1, t2);
        DD = vcombine_s16(vget_high_s16(CC), vcreate_s16(0));

        int16x8_t d, abs_d, sign, clip, sst1, sst;
        sst = vdupq_n_s16(st_u);
        sst1 = vdupq_n_s16(st_v);
        sst = vzip1q_s32(sst, sst1);
        t1 = vshlq_s16(BB, vdupq_n_s16(2));
        t2 = vshlq_s16(CC, vdupq_n_s16(2));
        t1 = vsubq_s16(AA, t1);
        t2 = vsubq_s16(t2, DD);
        d = vaddq_s16(t1, t2);
        abs_d = vabsq_s16(d);
        abs_d = vshlq_u16(abs_d, vdupq_n_s16(-3));

        int16x8_t zero, MAX;
        zero = vdupq_n_s16(0);
        MAX = vdupq_n_s16((1 << (bit_depth_minus8 + 8)) - 1);

        int16x8_t d1, d2, t16, ad;
        t16 = vsubq_s16(abs_d, sst);
        t16 = vshlq_s16(t16, vdupq_n_s16(1));
        t16 = vmaxq_s16(zero, t16);
        clip = vsubq_s16(abs_d, t16);
        clip = vmaxq_s16(zero, clip);
        uint16x8_t lt_mask = vshrq_n_s16(d, 15); // get sign
        uint16x8_t zeromask = vceqzq_s16(d);

        d1 = vbicq_s16(vbslq_s16(lt_mask, vnegq_s16(clip), clip), zeromask);

        BB = vaddq_s16(BB, d1);
        CC = vsubq_s16(CC, d1);

        int16x8_t tmp;
        tmp = vmaxq_s16(zero, BB);
        BB = vminq_s16(tmp, MAX);
        tmp = vmaxq_s16(zero, CC);
        CC = vminq_s16(tmp, MAX);

        t1 = vzip1q_s16(AA, BB);
        t2 = vzip1q_s16(CC, DD);
        AA = vzip1q_s32(t1, t2);
        BB = vcombine_s16(vget_high_s16(AA), vcreate_s16(0));
        CC = vzip2q_s32(t1, t2);
        DD = vcombine_s16(vget_high_s16(CC), vcreate_s16(0));

        if (st_u)
        {
            vst1_s16(&u[-2], vget_low_s16(AA));
            vst1_s16(&u[stride - 2], vget_low_s16(BB));
        }
        if (st_v)
        {
            vst1_s16(&v[-2], vget_low_s16(CC));
            vst1_s16(&v[stride - 2], vget_low_s16(DD));
        }
    }
    else if (size == 4)
    {
        s16 A, B, C, D, d, d1;
        s16 abs_d, t16, clip, sign;
        int i;
        for (i = 0; i < size; i++)
        {
            if (st_u)
            {
                A = u[-2];
                B = u[-1];
                C = u[0];
                D = u[1];

                d = (A - (B << 2) + (C << 2) - D) / 8;

                abs_d = XEVD_ABS16(d);
                sign = XEVD_SIGN_GET16(d);

                t16 = XEVD_MAX(0, ((abs_d - st_u) << 1));
                clip = XEVD_MAX(0, (abs_d - t16));
                d1 = XEVD_SIGN_SET16(clip, sign);

                B += d1;
                C -= d1;

                u[-1] = XEVD_CLIP3(0, (1 << (bit_depth_minus8 + 8)) - 1, B);
                u[0] = XEVD_CLIP3(0, (1 << (bit_depth_minus8 + 8)) - 1, C);

                u += stride;
            }
            if (st_v)
            {
                A = v[-2];
                B = v[-1];
                C = v[0];
                D = v[1];

                d = (A - (B << 2) + (C << 2) - D) / 8;

                abs_d = XEVD_ABS16(d);
                sign = XEVD_SIGN_GET16(d);

                t16 = XEVD_MAX(0, ((abs_d - st_v) << 1));
                clip = XEVD_MAX(0, (abs_d - t16));
                d1 = XEVD_SIGN_SET16(clip, sign);

                B += d1;
                C -= d1;

                v[-1] = XEVD_CLIP3(0, (1 << (bit_depth_minus8 + 8)) - 1, B);
                v[0] = XEVD_CLIP3(0, (1 << (bit_depth_minus8 + 8)) - 1, C);

                v += stride;
            }
        }
    }
}

const XEVD_DBK xevd_tbl_dbk_neon[2] =
{
    deblock_scu_ver_neon,
    deblock_scu_hor_neon,
};

const XEVD_DBK_CH xevd_tbl_dbk_chroma_neon[2] =
{
    deblock_scu_ver_chroma_neon,
    deblock_scu_hor_chroma_neon
};

