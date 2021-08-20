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

#include "xevd_recon_avx.h"

void xevd_recon_avx(s16 *coef, pel *pred, int is_coef, int cuw, int cuh, int s_rec, pel *rec, int bit_depth)
{
    int i, j;
    s16 t0;
    int max = ((1 << bit_depth) - 1);
    int min = 0;

    __m128i mm_min = _mm_set1_epi16(min);
    __m128i mm_max = _mm_set1_epi16(max);

    __m256i mm_min_256 = _mm256_set1_epi16(min);
    __m256i mm_max_256 = _mm256_set1_epi16(max);

    if (is_coef == 0) /* just copy pred to rec */
    {

        if ((cuh & 3) == 0)
        {
            if ((cuw & 0xF) == 0)
            {

                __m256i m00_16x16, m01_16x16, m02_16x16, m03_16x16, temp1_16x16, temp2_16x16, temp3_16x16, temp4_16x16;

                for (i = 0; i < cuh; i += 4)
                {
                    for (j = 0; j < cuw; j += 16)
                    {

                        m00_16x16 = _mm256_loadu_si256((__m256i*)(pred + j));
                        m01_16x16 = _mm256_loadu_si256((__m256i*)(pred + j + cuw));
                        m02_16x16 = _mm256_loadu_si256((__m256i*)(pred + j + 2 * cuw));
                        m03_16x16 = _mm256_loadu_si256((__m256i*)(pred + j + 3 * cuw));


                        temp1_16x16 = _mm256_min_epi16(m00_16x16, mm_max_256);
                        temp2_16x16 = _mm256_min_epi16(m01_16x16, mm_max_256);
                        temp3_16x16 = _mm256_min_epi16(m02_16x16, mm_max_256);
                        temp4_16x16 = _mm256_min_epi16(m03_16x16, mm_max_256);
                        m00_16x16 = _mm256_max_epi16(temp1_16x16, mm_min_256);
                        m01_16x16 = _mm256_max_epi16(temp2_16x16, mm_min_256);
                        m02_16x16 = _mm256_max_epi16(temp3_16x16, mm_min_256);
                        m03_16x16 = _mm256_max_epi16(temp4_16x16, mm_min_256);

                        _mm256_storeu_si256((__m256i*)(rec + j), m00_16x16);
                        _mm256_storeu_si256((__m256i*)(rec + j + s_rec), m01_16x16);
                        _mm256_storeu_si256((__m256i*)(rec + j + 2 * s_rec), m02_16x16);
                        _mm256_storeu_si256((__m256i*)(rec + j + 3 * s_rec), m03_16x16);
                    }
                    pred += cuw * 4;
                    rec += s_rec * 4;
                }
            }
            else if ((cuw & 0x7) == 0)
            {

                __m128i m00, m01, m02, m03, temp1, temp2, temp3, temp4;

                for (i = 0; i < cuh; i += 4)
                {
                    for (j = 0; j < cuw; j += 8)
                    {
                        m00 = _mm_loadu_si128((__m128i*)(pred + j));
                        m01 = _mm_loadu_si128((__m128i*)(pred + j + cuw));
                        m02 = _mm_loadu_si128((__m128i*)(pred + j + 2 * cuw));
                        m03 = _mm_loadu_si128((__m128i*)(pred + j + 3 * cuw));

                        temp1 = _mm_min_epi16(m00, mm_max);
                        temp2 = _mm_min_epi16(m01, mm_max);
                        temp3 = _mm_min_epi16(m02, mm_max);
                        temp4 = _mm_min_epi16(m03, mm_max);

                        m00 = _mm_max_epi16(temp1, mm_min);
                        m01 = _mm_max_epi16(temp2, mm_min);
                        m02 = _mm_max_epi16(temp3, mm_min);
                        m03 = _mm_max_epi16(temp4, mm_min);

                        _mm_storeu_si128((__m128i*)(rec + j), m00);
                        _mm_storeu_si128((__m128i*)(rec + j + s_rec), m01);
                        _mm_storeu_si128((__m128i*)(rec + j + 2 * s_rec), m02);
                        _mm_storeu_si128((__m128i*)(rec + j + 3 * s_rec), m03);
                    }
                    pred += cuw * 4;
                    rec += s_rec * 4;
                }
            }
            else if ((cuw & 0x3) == 0)
            {
                __m128i m00, m01, m02, m03, temp1, temp2, temp3, temp4;

                for (i = 0; i < cuh; i += 2)
                {
                    for (j = 0; j < cuw; j += 4)
                    {
                        m00 = _mm_loadl_epi64((__m128i*)(pred + j));
                        m01 = _mm_loadl_epi64((__m128i*)(pred + j + cuw));
                        m02 = _mm_loadl_epi64((__m128i*)(pred + j + 2 * cuw));
                        m03 = _mm_loadl_epi64((__m128i*)(pred + j + 3 * cuw));
                        temp1 = _mm_min_epi16(m00, mm_max);
                        temp2 = _mm_min_epi16(m01, mm_max);
                        temp3 = _mm_min_epi16(m02, mm_max);
                        temp4 = _mm_min_epi16(m03, mm_max);
                        m00 = _mm_max_epi16(temp1, mm_min);
                        m01 = _mm_max_epi16(temp2, mm_min);
                        m02 = _mm_max_epi16(temp3, mm_min);
                        m03 = _mm_max_epi16(temp4, mm_min);
                        _mm_storel_epi64((__m128i*)(rec + j), m00);
                        _mm_storel_epi64((__m128i*)(rec + j + s_rec), m01);
                        _mm_storel_epi64((__m128i*)(rec + j + 2 * s_rec), m02);
                        _mm_storel_epi64((__m128i*)(rec + j + 3 * s_rec), m03);
                    }
                    pred += cuw * 4;
                    rec += s_rec * 4;
                }

            }
            else
            {
                for (i = 0; i < cuh; i++)
                {
                    for (j = 0; j < cuw; j++)
                    {
                        rec[i * s_rec + j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, pred[i * cuw + j]);
                    }
                }
            }
        }
        else if ((cuh & 1) == 0)
        {
            if ((cuw & 0xF) == 0)
            {

                __m256i m00_16x16, m01_16x16, temp1_16x16, temp2_16x16;

                for (i = 0; i < cuh; i += 2)
                {
                    for (j = 0; j < cuw; j += 16)
                    {

                        m00_16x16 = _mm256_loadu_si256((__m256i*)(pred + j));
                        m01_16x16 = _mm256_loadu_si256((__m256i*)(pred + j + cuw));


                        temp1_16x16 = _mm256_min_epi16(m00_16x16, mm_max_256);
                        temp2_16x16 = _mm256_min_epi16(m01_16x16, mm_max_256);
                        m00_16x16 = _mm256_max_epi16(temp1_16x16, mm_min_256);
                        m01_16x16 = _mm256_max_epi16(temp2_16x16, mm_min_256);

                        _mm256_storeu_si256((__m256i*)(rec + j), m00_16x16);
                        _mm256_storeu_si256((__m256i*)(rec + j + s_rec), m01_16x16);
                    }
                    pred += cuw * 2;
                    rec += s_rec * 2;
                }
            }
            else if ((cuw & 0x7) == 0)
            {

                __m128i m00, m01, temp1, temp2;

                for (i = 0; i < cuh; i += 2)
                {
                    for (j = 0; j < cuw; j += 8)
                    {
                        m00 = _mm_loadu_si128((__m128i*)(pred + j));
                        m01 = _mm_loadu_si128((__m128i*)(pred + j + cuw));

                        temp1 = _mm_min_epi16(m00, mm_max);
                        temp2 = _mm_min_epi16(m01, mm_max);
                        m00 = _mm_max_epi16(temp1, mm_min);
                        m01 = _mm_max_epi16(temp2, mm_min);

                        _mm_storeu_si128((__m128i*)(rec + j), m00);
                        _mm_storeu_si128((__m128i*)(rec + j + s_rec), m01);
                    }
                    pred += cuw * 2;
                    rec += s_rec * 2;
                }
            }
            else if ((cuw & 0x3) == 0)
            {
                __m128i m00, m01, temp1, temp2;

                for (i = 0; i < cuh; i += 2)
                {
                    for (j = 0; j < cuw; j += 4)
                    {
                        m00 = _mm_loadl_epi64((__m128i*)(pred + j));
                        m01 = _mm_loadl_epi64((__m128i*)(pred + j + cuw));
                        temp1 = _mm_min_epi16(m00, mm_max);
                        temp2 = _mm_min_epi16(m01, mm_max);
                        m00 = _mm_max_epi16(temp1, mm_min);
                        m01 = _mm_max_epi16(temp2, mm_min);
                        _mm_storel_epi64((__m128i*)(rec + j), m00);
                        _mm_storel_epi64((__m128i*)(rec + j + s_rec), m01);
                    }
                    pred += cuw * 2;
                    rec += s_rec * 2;
                }

            }
            else
            {
                for (i = 0; i < cuh; i++)
                {
                    for (j = 0; j < cuw; j++)
                    {
                        rec[i * s_rec + j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, pred[i * cuw + j]);
                    }
                }
            }
        }
    }
    else  /* add b/w pred and coef and copy it into rec */
    {
        if ((cuh & 3) == 0)
        {
            if ((cuw & 0xF) == 0)
            {
                __m256i m00_16x16, m01_16x16, m02_16x16, m03_16x16, temp1_16x16, temp2_16x16, temp3_16x16, temp4_16x16;
                __m256i c00_16x16, c01_16x16, c02_16x16, c03_16x16;
                for (i = 0; i < cuh; i += 4)
                {
                    for (j = 0; j < cuw; j += 16)
                    {
                        m00_16x16 = _mm256_loadu_si256((__m256i*)(pred + j));
                        m01_16x16 = _mm256_loadu_si256((__m256i*)(pred + j + cuw));
                        m02_16x16 = _mm256_loadu_si256((__m256i*)(pred + j + 2 * cuw));
                        m03_16x16 = _mm256_loadu_si256((__m256i*)(pred + j + 3 * cuw));
                        c00_16x16 = _mm256_loadu_si256((__m256i*)(coef + j));
                        c01_16x16 = _mm256_loadu_si256((__m256i*)(coef + j + cuw));
                        c02_16x16 = _mm256_loadu_si256((__m256i*)(coef + j + 2 * cuw));
                        c03_16x16 = _mm256_loadu_si256((__m256i*)(coef + j + 3 * cuw));

                        m00_16x16 = _mm256_add_epi16(m00_16x16, c00_16x16);
                        m01_16x16 = _mm256_add_epi16(m01_16x16, c01_16x16);
                        m02_16x16 = _mm256_add_epi16(m02_16x16, c02_16x16);
                        m03_16x16 = _mm256_add_epi16(m03_16x16, c03_16x16);

                        temp1_16x16 = _mm256_min_epi16(m00_16x16, mm_max_256);
                        temp2_16x16 = _mm256_min_epi16(m01_16x16, mm_max_256);
                        temp3_16x16 = _mm256_min_epi16(m02_16x16, mm_max_256);
                        temp4_16x16 = _mm256_min_epi16(m03_16x16, mm_max_256);

                        m00_16x16 = _mm256_max_epi16(temp1_16x16, mm_min_256);
                        m01_16x16 = _mm256_max_epi16(temp2_16x16, mm_min_256);
                        m02_16x16 = _mm256_max_epi16(temp3_16x16, mm_min_256);
                        m03_16x16 = _mm256_max_epi16(temp4_16x16, mm_min_256);

                        _mm256_storeu_si256((__m256i*)(rec + j), m00_16x16);
                        _mm256_storeu_si256((__m256i*)(rec + j + s_rec), m01_16x16);
                        _mm256_storeu_si256((__m256i*)(rec + j + 2 * s_rec), m02_16x16);
                        _mm256_storeu_si256((__m256i*)(rec + j + 3 * s_rec), m03_16x16);
                    }
                    pred += cuw * 4;
                    coef += cuw * 4;
                    rec += s_rec * 4;
                }
            }
            else if ((cuw & 0x7) == 0)
            {
                __m128i m00, m01, m02, m03, c00, c01, c02, c03, temp1, temp2, temp3, temp4;

                for (i = 0; i < cuh; i += 4)
                {
                    for (j = 0; j < cuw; j += 8)
                    {
                        m00 = _mm_loadu_si128((__m128i*)(pred + j));
                        m01 = _mm_loadu_si128((__m128i*)(pred + j + cuw));
                        m02 = _mm_loadu_si128((__m128i*)(pred + j + 2 * cuw));
                        m03 = _mm_loadu_si128((__m128i*)(pred + j + 3 * cuw));

                        c00 = _mm_loadu_si128((__m128i*)(coef + j));
                        c01 = _mm_loadu_si128((__m128i*)(coef + j + cuw));
                        c02 = _mm_loadu_si128((__m128i*)(coef + j + 2 * cuw));
                        c03 = _mm_loadu_si128((__m128i*)(coef + j + 3 * cuw));

                        m00 = _mm_add_epi16(m00, c00);
                        m01 = _mm_add_epi16(m01, c01);
                        m02 = _mm_add_epi16(m02, c02);
                        m03 = _mm_add_epi16(m03, c03);

                        temp1 = _mm_min_epi16(m00, mm_max);
                        temp2 = _mm_min_epi16(m01, mm_max);
                        temp3 = _mm_min_epi16(m02, mm_max);
                        temp4 = _mm_min_epi16(m03, mm_max);
                        m00 = _mm_max_epi16(temp1, mm_min);
                        m01 = _mm_max_epi16(temp2, mm_min);
                        m02 = _mm_max_epi16(temp3, mm_min);
                        m03 = _mm_max_epi16(temp4, mm_min);

                        _mm_storeu_si128((__m128i*)(rec + j), m00);
                        _mm_storeu_si128((__m128i*)(rec + j + s_rec), m01);
                        _mm_storeu_si128((__m128i*)(rec + j + 2 * s_rec), m02);
                        _mm_storeu_si128((__m128i*)(rec + j + 3 * s_rec), m03);
                    }
                    pred += cuw * 4;
                    coef += cuw * 4;
                    rec += s_rec * 4;
                }
            }
            else if ((cuw & 0x3) == 0)
            {
                __m128i m00, m01, m02, m03, c00, c01, c02, c03, temp1, temp2, temp3, temp4;

                for (i = 0; i < cuh; i += 4)
                {
                    for (j = 0; j < cuw; j += 4)
                    {
                        m00 = _mm_loadl_epi64((__m128i*)(pred + j));
                        m01 = _mm_loadl_epi64((__m128i*)(pred + j + cuw));
                        m02 = _mm_loadl_epi64((__m128i*)(pred + j + 2 * cuw));
                        m03 = _mm_loadl_epi64((__m128i*)(pred + j + 3 * cuw));
                        c00 = _mm_loadl_epi64((__m128i*)(coef + j));
                        c01 = _mm_loadl_epi64((__m128i*)(coef + j + cuw));
                        c02 = _mm_loadl_epi64((__m128i*)(coef + j + 2 * cuw));
                        c03 = _mm_loadl_epi64((__m128i*)(coef + j + 3 * cuw));
                        m00 = _mm_add_epi16(m00, c00);
                        m01 = _mm_add_epi16(m01, c01);
                        m02 = _mm_add_epi16(m02, c02);
                        m03 = _mm_add_epi16(m03, c03);

                        temp1 = _mm_min_epi16(m00, mm_max);
                        temp2 = _mm_min_epi16(m01, mm_max);
                        temp3 = _mm_min_epi16(m02, mm_max);
                        temp4 = _mm_min_epi16(m03, mm_max);
                        m00 = _mm_max_epi16(temp1, mm_min);
                        m01 = _mm_max_epi16(temp2, mm_min);
                        m02 = _mm_max_epi16(temp3, mm_min);
                        m03 = _mm_max_epi16(temp4, mm_min);
                        _mm_storel_epi64((__m128i*)(rec + j), m00);
                        _mm_storel_epi64((__m128i*)(rec + j + s_rec), m01);
                        _mm_storel_epi64((__m128i*)(rec + j + 2 * s_rec), m02);
                        _mm_storel_epi64((__m128i*)(rec + j + 3 * s_rec), m03);
                    }
                    pred += cuw * 4;
                    coef += cuw * 4;
                    rec += s_rec * 4;
                }
            }
            else
            {
                for (i = 0; i < cuh; i++)
                {
                    for (j = 0; j < cuw; j++)
                    {
                        t0 = coef[i * cuw + j] + pred[i * cuw + j];
                        rec[i * s_rec + j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, t0);
                    }
                }
            }

        }
        else if ((cuh & 1) == 0)
        {
            if ((cuw & 0xF) == 0)
            {
                __m256i m00_16x16, m01_16x16, c00_16x16, c01_16x16, temp1_16x16, temp2_16x16;
                for (i = 0; i < cuh; i += 2)
                {
                    for (j = 0; j < cuw; j += 16)
                    {
                        m00_16x16 = _mm256_loadu_si256((__m256i*)(pred + j));
                        m01_16x16 = _mm256_loadu_si256((__m256i*)(pred + j + cuw));
                        c00_16x16 = _mm256_loadu_si256((__m256i*)(coef + j));
                        c01_16x16 = _mm256_loadu_si256((__m256i*)(coef + j + cuw));

                        m00_16x16 = _mm256_add_epi16(m00_16x16, c00_16x16);
                        m01_16x16 = _mm256_add_epi16(m01_16x16, c01_16x16);

                        temp1_16x16 = _mm256_min_epi16(m00_16x16, mm_max_256);
                        temp2_16x16 = _mm256_min_epi16(m01_16x16, mm_max_256);
                        m00_16x16 = _mm256_max_epi16(temp1_16x16, mm_min_256);
                        m01_16x16 = _mm256_max_epi16(temp2_16x16, mm_min_256);

                        _mm256_storeu_si256((__m256i*)(rec + j), m00_16x16);
                        _mm256_storeu_si256((__m256i*)(rec + j + s_rec), m01_16x16);
                    }
                    pred += cuw * 2;
                    coef += cuw * 2;
                    rec += s_rec * 2;
                }
            }
            else if ((cuw & 0x7) == 0)
            {
                __m128i m00, m01, c00, c01, temp1, temp2;

                for (i = 0; i < cuh; i += 2)
                {
                    for (j = 0; j < cuw; j += 8)
                    {
                        m00 = _mm_loadu_si128((__m128i*)(pred + j));
                        m01 = _mm_loadu_si128((__m128i*)(pred + j + cuw));

                        c00 = _mm_loadu_si128((__m128i*)(coef + j));
                        c01 = _mm_loadu_si128((__m128i*)(coef + j + cuw));

                        m00 = _mm_add_epi16(m00, c00);
                        m01 = _mm_add_epi16(m01, c01);

                        temp1 = _mm_min_epi16(m00, mm_max);
                        temp2 = _mm_min_epi16(m01, mm_max);
                        m00 = _mm_max_epi16(temp1, mm_min);
                        m01 = _mm_max_epi16(temp2, mm_min);

                        _mm_storeu_si128((__m128i*)(rec + j), m00);
                        _mm_storeu_si128((__m128i*)(rec + j + s_rec), m01);
                    }
                    pred += cuw * 2;
                    coef += cuw * 2;
                    rec += s_rec * 2;
                }
            }
            else if ((cuw & 0x3) == 0)
            {
                __m128i m00, m01, c00, c01, temp1, temp2;

                for (i = 0; i < cuh; i += 2)
                {
                    for (j = 0; j < cuw; j += 4)
                    {
                        m00 = _mm_loadl_epi64((__m128i*)(pred + j));
                        m01 = _mm_loadl_epi64((__m128i*)(pred + j + cuw));
                        c00 = _mm_loadl_epi64((__m128i*)(coef + j));
                        c01 = _mm_loadl_epi64((__m128i*)(coef + j + cuw));
                        m00 = _mm_add_epi16(m00, c00);
                        m01 = _mm_add_epi16(m01, c01);

                        temp1 = _mm_min_epi16(m00, mm_max);
                        temp2 = _mm_min_epi16(m01, mm_max);
                        m00 = _mm_max_epi16(temp1, mm_min);
                        m01 = _mm_max_epi16(temp2, mm_min);
                        _mm_storel_epi64((__m128i*)(rec + j), m00);
                        _mm_storel_epi64((__m128i*)(rec + j + s_rec), m01);
                    }
                    pred += cuw * 2;
                    coef += cuw * 2;
                    rec += s_rec * 2;
                }
            }
            else
            {
                for (i = 0; i < cuh; i++)
                {
                    for (j = 0; j < cuw; j++)
                    {
                        t0 = coef[i * cuw + j] + pred[i * cuw + j];
                        rec[i * s_rec + j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, t0);
                    }
                }
            }

        }
    }
}
