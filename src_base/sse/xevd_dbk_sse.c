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

#include "xevd_dbk_sse.h"

void deblock_scu_hor_sse(pel *buf, int st, int stride, int bit_depth_minus8, int chroma_format_idc)
{
    //size assumed 4x4
    __m128i AA, BB, CC, DD;
    AA = _mm_loadl_epi64((__m128i*)&buf[-2 * stride]);
    BB = _mm_loadl_epi64((__m128i*)&buf[-stride]);
    CC = _mm_loadl_epi64((__m128i*)&buf[0]);
    DD = _mm_loadl_epi64((__m128i*)&buf[stride]);

    __m128i t1, t2;
    __m128i d, abs_d, sign, clip, sst;
    sst = _mm_set1_epi16(st);
    t1 = _mm_slli_epi16(BB, 2);
    t2 = _mm_slli_epi16(CC, 2);
    t1 = _mm_sub_epi16(AA, t1);
    t2 = _mm_sub_epi16(t2, DD);
    d = _mm_add_epi16(t1, t2);
    abs_d = _mm_abs_epi16(d);
    abs_d = _mm_srli_epi16(abs_d, 3);

    __m128i zero, MAX;
    zero = _mm_set1_epi16(0);
    MAX = _mm_set1_epi16((1 << (bit_depth_minus8 + 8)) - 1);

    __m128i d1, d2, t16, ad;
    t16 = _mm_sub_epi16(abs_d, sst);
    t16 = _mm_slli_epi16(t16, 1);
    t16 = _mm_max_epi16(zero, t16);
    clip = _mm_sub_epi16(abs_d, t16);
    clip = _mm_max_epi16(zero, clip);
    d1 = _mm_sign_epi16(clip, d);

    clip = _mm_srli_epi16(clip, 1);
    d2 = _mm_sub_epi16(AA, DD);

    ad = _mm_abs_epi16(d2);
    ad = _mm_srli_epi16(ad, 2);
    ad = _mm_min_epi16(ad, clip);
    d2 = _mm_sign_epi16(ad, d2);

    AA = _mm_sub_epi16(AA, d2);
    BB = _mm_add_epi16(BB, d1);
    CC = _mm_sub_epi16(CC, d1);
    DD = _mm_add_epi16(DD, d2);

    __m128i tmp;
    tmp = _mm_max_epi16(zero, AA);
    AA = _mm_min_epi16(tmp, MAX);
    tmp = _mm_max_epi16(zero, BB);
    BB = _mm_min_epi16(tmp, MAX);
    tmp = _mm_max_epi16(zero, CC);
    CC = _mm_min_epi16(tmp, MAX);
    tmp = _mm_max_epi16(zero, DD);
    DD = _mm_min_epi16(tmp, MAX);

    _mm_storel_epi64((__m128i*)&buf[-2 * stride], AA);
    _mm_storel_epi64((__m128i*)&buf[-stride], BB);
    _mm_storel_epi64((__m128i*)&buf[0], CC);
    _mm_storel_epi64((__m128i*)&buf[stride], DD);
}

void deblock_scu_hor_chroma_sse(pel *u, pel *v, int st_u, int st_v, int stride, int bit_depth_minus8, int chroma_format_idc)
{
    int size;

    size = MIN_CU_SIZE >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));

    __m128i Au, Bu, Cu, Du, Av, Bv, Cv, Dv, AA, BB, CC, DD;
    if (st_u)
    {
        Au = _mm_loadl_epi64((__m128i*)&u[-2 * stride]);
        Bu = _mm_loadl_epi64((__m128i*)&u[-stride]);
        Cu = _mm_loadl_epi64((__m128i*)&u[0]);
        Du = _mm_loadl_epi64((__m128i*)&u[stride]);
    }
    if (st_v)
    {
        Av = _mm_loadl_epi64((__m128i*)&v[-2 * stride]);
        Bv = _mm_loadl_epi64((__m128i*)&v[-stride]);
        Cv = _mm_loadl_epi64((__m128i*)&v[0]);
        Dv = _mm_loadl_epi64((__m128i*)&v[stride]);
    }

    AA = _mm_unpacklo_epi64(Au, Av);
    BB = _mm_unpacklo_epi64(Bu, Bv);
    CC = _mm_unpacklo_epi64(Cu, Cv);
    DD = _mm_unpacklo_epi64(Du, Dv);

    __m128i t1, t2;

    __m128i d, abs_d, sign, clip, sst1, sst;
    sst = _mm_set1_epi16(st_u);
    sst1 = _mm_set1_epi16(st_v);
    sst = _mm_unpacklo_epi64(sst, sst1);
    t1 = _mm_slli_epi16(BB, 2);
    t2 = _mm_slli_epi16(CC, 2);
    t1 = _mm_sub_epi16(AA, t1);
    t2 = _mm_sub_epi16(t2, DD);
    d = _mm_add_epi16(t1, t2);
    abs_d = _mm_abs_epi16(d);
    abs_d = _mm_srli_epi16(abs_d, 3);

    __m128i zero, MAX;
    zero = _mm_set1_epi16(0);
    MAX = _mm_set1_epi16((1 << (bit_depth_minus8 + 8)) - 1);

    __m128i d1, d2, t16, ad;
    t16 = _mm_sub_epi16(abs_d, sst);
    t16 = _mm_slli_epi16(t16, 1);
    t16 = _mm_max_epi16(zero, t16);
    clip = _mm_sub_epi16(abs_d, t16);
    clip = _mm_max_epi16(zero, clip);
    d1 = _mm_sign_epi16(clip, d);

    BB = _mm_add_epi16(BB, d1);
    CC = _mm_sub_epi16(CC, d1);

    __m128i tmp;
    tmp = _mm_max_epi16(zero, BB);
    BB = _mm_min_epi16(tmp, MAX);
    tmp = _mm_max_epi16(zero, CC);
    CC = _mm_min_epi16(tmp, MAX);

    Av = _mm_srli_si128(AA, 8);
    Bv = _mm_srli_si128(BB, 8);
    Cv = _mm_srli_si128(CC, 8);
    Dv = _mm_srli_si128(DD, 8);
    if (st_u)
    {
        if (size == 2)
        {
            u[-stride] = _mm_extract_epi16(BB, 0);
            u[-stride + 1] = _mm_extract_epi16(BB, 1);
            u[0] = _mm_extract_epi16(CC, 0);
            u[1] = _mm_extract_epi16(CC, 1);
        }
        else if (size == 4)
        {
            _mm_storel_epi64((__m128i*)&u[-stride], BB);
            _mm_storel_epi64((__m128i*)&u[0], CC);
        }

    }
    if (st_v)
    {
        if (size == 2)
        {
            v[-stride] = _mm_extract_epi16(Bv, 0);
            v[-stride + 1] = _mm_extract_epi16(Bv, 1);
            v[0] = _mm_extract_epi16(Cv, 0);
            v[1] = _mm_extract_epi16(Cv, 1);
        }
        else if (size == 4)
        {
            _mm_storel_epi64((__m128i*)&v[-stride], Bv);
            _mm_storel_epi64((__m128i*)&v[0], Cv);
        }

    }
}

void deblock_scu_ver_sse(pel *buf, int st, int stride, int bit_depth_minus8, int chroma_format_idc)
{
    //size assumes 4x4
    __m128i AA, BB, CC, DD;
    AA = _mm_loadl_epi64((__m128i*)&buf[-2]);
    BB = _mm_loadl_epi64((__m128i*)&buf[stride - 2]);
    CC = _mm_loadl_epi64((__m128i*)&buf[2 * stride - 2]);
    DD = _mm_loadl_epi64((__m128i*)&buf[3 * stride - 2]);

    __m128i t1, t2;
    t1 = _mm_unpacklo_epi16(AA, BB);
    t2 = _mm_unpacklo_epi16(CC, DD);
    AA = _mm_unpacklo_epi32(t1, t2);
    BB = _mm_srli_si128(AA, 8);
    CC = _mm_unpackhi_epi32(t1, t2);
    DD = _mm_srli_si128(CC, 8);

    __m128i d, abs_d, sign, clip, sst;
    sst = _mm_set1_epi16(st);
    t1 = _mm_slli_epi16(BB, 2);
    t2 = _mm_slli_epi16(CC, 2);
    t1 = _mm_sub_epi16(AA, t1);
    t2 = _mm_sub_epi16(t2, DD);
    d = _mm_add_epi16(t1, t2);
    abs_d = _mm_abs_epi16(d);
    abs_d = _mm_srli_epi16(abs_d, 3);

    __m128i zero, MAX;
    zero = _mm_set1_epi16(0);
    MAX = _mm_set1_epi16((1 << (bit_depth_minus8 + 8)) - 1);

    __m128i d1, d2, t16, ad;
    t16 = _mm_sub_epi16(abs_d, sst);
    t16 = _mm_slli_epi16(t16, 1);
    t16 = _mm_max_epi16(zero, t16);
    clip = _mm_sub_epi16(abs_d, t16);
    clip = _mm_max_epi16(zero, clip);
    d1 = _mm_sign_epi16(clip, d);

    clip = _mm_srli_epi16(clip, 1);
    d2 = _mm_sub_epi16(AA, DD);

    ad = _mm_abs_epi16(d2);
    ad = _mm_srli_epi16(ad, 2);
    ad = _mm_min_epi16(ad, clip);
    d2 = _mm_sign_epi16(ad, d2);

    AA = _mm_sub_epi16(AA, d2);
    BB = _mm_add_epi16(BB, d1);
    CC = _mm_sub_epi16(CC, d1);
    DD = _mm_add_epi16(DD, d2);

    __m128i tmp;
    tmp = _mm_max_epi16(zero, AA);
    AA = _mm_min_epi16(tmp, MAX);
    tmp = _mm_max_epi16(zero, BB);
    BB = _mm_min_epi16(tmp, MAX);
    tmp = _mm_max_epi16(zero, CC);
    CC = _mm_min_epi16(tmp, MAX);
    tmp = _mm_max_epi16(zero, DD);
    DD = _mm_min_epi16(tmp, MAX);

    t1 = _mm_unpacklo_epi16(AA, BB);
    t2 = _mm_unpacklo_epi16(CC, DD);
    AA = _mm_unpacklo_epi32(t1, t2);
    BB = _mm_srli_si128(AA, 8);
    CC = _mm_unpackhi_epi32(t1, t2);
    DD = _mm_srli_si128(CC, 8);

    _mm_storel_epi64((__m128i*)&buf[-2], AA);
    _mm_storel_epi64((__m128i*)&buf[stride - 2], BB);
    _mm_storel_epi64((__m128i*)&buf[2 * stride - 2], CC);
    _mm_storel_epi64((__m128i*)&buf[3 * stride - 2], DD);
}

void deblock_scu_ver_chroma_sse(pel *u, pel *v, int st_u, int st_v, int stride, int bit_depth_minus8, int chroma_format_idc)
{
    int size;

    size = MIN_CU_SIZE >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));
    if (size == 2)
    {
        __m128i AA, BB, CC, DD;
        if (st_u)
        {
            AA = _mm_loadl_epi64((__m128i*)&u[-2]);
            BB = _mm_loadl_epi64((__m128i*)&u[stride - 2]);
        }
        if (st_v)
        {
            CC = _mm_loadl_epi64((__m128i*)&v[-2]);
            DD = _mm_loadl_epi64((__m128i*)&v[stride - 2]);
        }
        __m128i t1, t2;
        t1 = _mm_unpacklo_epi16(AA, BB);
        t2 = _mm_unpacklo_epi16(CC, DD);
        AA = _mm_unpacklo_epi32(t1, t2);
        BB = _mm_srli_si128(AA, 8);
        CC = _mm_unpackhi_epi32(t1, t2);
        DD = _mm_srli_si128(CC, 8);

        __m128i d, abs_d, sign, clip, sst1, sst;
        sst = _mm_set1_epi16(st_u);
        sst1 = _mm_set1_epi16(st_v);
        sst = _mm_unpacklo_epi32(sst, sst1);
        t1 = _mm_slli_epi16(BB, 2);
        t2 = _mm_slli_epi16(CC, 2);
        t1 = _mm_sub_epi16(AA, t1);
        t2 = _mm_sub_epi16(t2, DD);
        d = _mm_add_epi16(t1, t2);
        abs_d = _mm_abs_epi16(d);
        abs_d = _mm_srli_epi16(abs_d, 3);

        __m128i zero, MAX;
        zero = _mm_set1_epi16(0);
        MAX = _mm_set1_epi16((1 << (bit_depth_minus8 + 8)) - 1);

        __m128i d1, d2, t16, ad;
        t16 = _mm_sub_epi16(abs_d, sst);
        t16 = _mm_slli_epi16(t16, 1);
        t16 = _mm_max_epi16(zero, t16);
        clip = _mm_sub_epi16(abs_d, t16);
        clip = _mm_max_epi16(zero, clip);
        d1 = _mm_sign_epi16(clip, d);

        BB = _mm_add_epi16(BB, d1);
        CC = _mm_sub_epi16(CC, d1);

        __m128i tmp;
        tmp = _mm_max_epi16(zero, BB);
        BB = _mm_min_epi16(tmp, MAX);
        tmp = _mm_max_epi16(zero, CC);
        CC = _mm_min_epi16(tmp, MAX);

        t1 = _mm_unpacklo_epi16(AA, BB);
        t2 = _mm_unpacklo_epi16(CC, DD);
        AA = _mm_unpacklo_epi32(t1, t2);
        BB = _mm_srli_si128(AA, 8);
        CC = _mm_unpackhi_epi32(t1, t2);
        DD = _mm_srli_si128(CC, 8);

        if (st_u)
        {
            _mm_storel_epi64((__m128i*)&u[-2], AA);
            _mm_storel_epi64((__m128i*)&u[stride - 2], BB);
        }
        if (st_v)
        {
            _mm_storel_epi64((__m128i*)&v[-2], CC);
            _mm_storel_epi64((__m128i*)&v[stride - 2], DD);
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

const XEVD_DBK xevd_tbl_dbk_sse[2] =
{
    deblock_scu_ver_sse,
    deblock_scu_hor_sse,
};

const XEVD_DBK_CH xevd_tbl_dbk_chroma_sse[2] =
{
    deblock_scu_ver_chroma_sse,
    deblock_scu_hor_chroma_sse
};

