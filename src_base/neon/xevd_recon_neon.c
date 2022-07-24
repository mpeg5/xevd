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

#include "xevd_recon_neon.h"

void xevd_recon_neon(s16 *coef, pel *pred, int is_coef, int cuw, int cuh, int s_rec, pel *rec, int bit_depth)
{
    int i, j;
    s16 t0;
    int max = ((1 << bit_depth) - 1);
    int min = 0;

    int16x8_t mm_min = vdupq_n_s16(min);
    int16x8_t mm_max = vdupq_n_s16(max);

    int16x4_t mm_min_x4 = vdup_n_s16(min);
    int16x4_t mm_max_x4 = vdup_n_s16(max);

    if (is_coef == 0) /* just copy pred to rec */
    {

        if ((cuh & 3) == 0)
        {
            if ((cuw & 0x7) == 0)
            {

                int16x8_t m00, m01, m02, m03, temp1, temp2, temp3, temp4;

                for (i = 0; i < cuh; i += 4)
                {
                    for (j = 0; j < cuw; j += 8)
                    {
                        m00 = vld1q_s16((pred + j));
                        m01 = vld1q_s16((pred + j + cuw));
                        m02 = vld1q_s16((pred + j + 2 * cuw));
                        m03 = vld1q_s16((pred + j + 3 * cuw));

                        temp1 = vminq_s16(m00, mm_max);
                        temp2 = vminq_s16(m01, mm_max);
                        temp3 = vminq_s16(m02, mm_max);
                        temp4 = vminq_s16(m03, mm_max);

                        m00 = vmaxq_s16(temp1, mm_min);
                        m01 = vmaxq_s16(temp2, mm_min);
                        m02 = vmaxq_s16(temp3, mm_min);
                        m03 = vmaxq_s16(temp4, mm_min);

                        vst1q_s16((rec + j), m00);
                        vst1q_s16((rec + j + s_rec), m01);
                        vst1q_s16((rec + j + 2 * s_rec), m02);
                        vst1q_s16((rec + j + 3 * s_rec), m03);
                    }
                    pred += (cuw << 2);
                    rec += (s_rec << 2);
                }
            }
            else if ((cuw & 0x3) == 0)
            {
                int16x4_t m00, m01, m02, m03, temp1, temp2, temp3, temp4;

                for (i = 0; i < cuh; i += 4)
                {
                    for (j = 0; j < cuw; j += 4)
                    {
                        m00 = vld1_s16((pred + j));
                        m01 = vld1_s16((pred + j + cuw));
                        m02 = vld1_s16((pred + j + 2 * cuw));
                        m03 = vld1_s16((pred + j + 3 * cuw));
                        
                        temp1 = vmin_s16(m00, mm_max_x4);
                        temp2 = vmin_s16(m01, mm_max_x4);
                        temp3 = vmin_s16(m02, mm_max_x4);
                        temp4 = vmin_s16(m03, mm_max_x4);
                        
                        m00 = vmax_s16(temp1, mm_min_x4);
                        m01 = vmax_s16(temp2, mm_min_x4);
                        m02 = vmax_s16(temp3, mm_min_x4);
                        m03 = vmax_s16(temp4, mm_min_x4);
                        
                        vst1_s16((rec + j), m00);
                        vst1_s16((rec + j + s_rec), m01);
                        vst1_s16((rec + j + 2 * s_rec), m02);
                        vst1_s16((rec + j + 3 * s_rec), m03);
                    }
                    pred += (cuw << 2);
                    rec += (s_rec << 2);
                }

            }
            else
            {
                for (i = 0; i < cuh; ++i)
                {
                    for (j = 0; j < cuw; ++j)
                    {
                        rec[i * s_rec + j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, pred[i * cuw + j]);
                    }
                }
            }
        }
        else if ((cuh & 1) == 0)
        {
            if ((cuw & 0x7) == 0)
            {

                int16x8_t m00, m01, temp1, temp2;

                for (i = 0; i < cuh; i += 2)
                {
                    for (j = 0; j < cuw; j += 8)
                    {
                        m00 = vld1q_s16((pred + j));
                        m01 = vld1q_s16((pred + j + cuw));

                        temp1 = vminq_s16(m00, mm_max);
                        temp2 = vminq_s16(m01, mm_max);
                        
                        m00 = vmaxq_s16(temp1, mm_min);
                        m01 = vmaxq_s16(temp2, mm_min);

                        vst1q_s16((rec + j), m00);
                        vst1q_s16((rec + j + s_rec), m01);
                    }
                    pred += (cuw << 1);
                    rec += (s_rec << 1);
                }
            }
            else if ((cuw & 0x3) == 0)
            {
                int16x4_t m00, m01, temp1, temp2;

                for (i = 0; i < cuh; i += 2)
                {
                    for (j = 0; j < cuw; j += 4)
                    {
                        m00 = vld1_s16((pred + j));
                        m01 = vld1_s16((pred + j + cuw));
                        
                        temp1 = vmin_s16(m00, mm_max_x4);
                        temp2 = vmin_s16(m01, mm_max_x4);
                        
                        m00 = vmax_s16(temp1, mm_min_x4);
                        m01 = vmax_s16(temp2, mm_min_x4);
                        
                        vst1_s16((rec + j), m00);
                        vst1_s16((rec + j + s_rec), m01);
                    }
                    pred += (cuw << 1);
                    rec += (s_rec << 1);
                }

            }
            else
            {
                for (i = 0; i < cuh; ++i)
                {
                    for (j = 0; j < cuw; ++j)
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
            if ((cuw & 0x7) == 0)
            {
                int16x8_t m00, m01, m02, m03, c00, c01, c02, c03, temp1, temp2, temp3, temp4;

                for (i = 0; i < cuh; i += 4)
                {
                    for (j = 0; j < cuw; j += 8)
                    {
                        m00 = vld1q_s16((pred + j));
                        m01 = vld1q_s16((pred + j + cuw));
                        m02 = vld1q_s16((pred + j + 2 * cuw));
                        m03 = vld1q_s16((pred + j + 3 * cuw));

                        c00 = vld1q_s16((coef + j));
                        c01 = vld1q_s16((coef + j + cuw));
                        c02 = vld1q_s16((coef + j + 2 * cuw));
                        c03 = vld1q_s16((coef + j + 3 * cuw));

                        m00 = vaddq_s16(m00, c00);
                        m01 = vaddq_s16(m01, c01);
                        m02 = vaddq_s16(m02, c02);
                        m03 = vaddq_s16(m03, c03);

                        temp1 = vminq_s16(m00, mm_max);
                        temp2 = vminq_s16(m01, mm_max);
                        temp3 = vminq_s16(m02, mm_max);
                        temp4 = vminq_s16(m03, mm_max);
                        
                        m00 = vmaxq_s16(temp1, mm_min);
                        m01 = vmaxq_s16(temp2, mm_min);
                        m02 = vmaxq_s16(temp3, mm_min);
                        m03 = vmaxq_s16(temp4, mm_min);

                        vst1q_s16((rec + j), m00);
                        vst1q_s16((rec + j + s_rec), m01);
                        vst1q_s16((rec + j + 2 * s_rec), m02);
                        vst1q_s16((rec + j + 3 * s_rec), m03);
                    }
                    pred += (cuw << 2);
                    coef += (cuw << 2);
                    rec += (s_rec << 2);
                }
            }
            else if ((cuw & 0x3) == 0)
            {
                int16x4_t m00, m01, m02, m03, c00, c01, c02, c03, temp1, temp2, temp3, temp4;

                for (i = 0; i < cuh; i += 4)
                {
                    for (j = 0; j < cuw; j += 4)
                    {
                        m00 = vld1_s16((pred + j));
                        m01 = vld1_s16((pred + j + cuw));
                        m02 = vld1_s16((pred + j + 2 * cuw));
                        m03 = vld1_s16((pred + j + 3 * cuw));
                        
                        c00 = vld1_s16((coef + j));
                        c01 = vld1_s16((coef + j + cuw));
                        c02 = vld1_s16((coef + j + 2 * cuw));
                        c03 = vld1_s16((coef + j + 3 * cuw));
                        
                        m00 = vadd_s16(m00, c00);
                        m01 = vadd_s16(m01, c01);
                        m02 = vadd_s16(m02, c02);
                        m03 = vadd_s16(m03, c03);

                        temp1 = vmin_s16(m00, mm_max_x4);
                        temp2 = vmin_s16(m01, mm_max_x4);
                        temp3 = vmin_s16(m02, mm_max_x4);
                        temp4 = vmin_s16(m03, mm_max_x4);
                        
                        m00 = vmax_s16(temp1, mm_min_x4);
                        m01 = vmax_s16(temp2, mm_min_x4);
                        m02 = vmax_s16(temp3, mm_min_x4);
                        m03 = vmax_s16(temp4, mm_min_x4);
                        
                        vst1_s16((rec + j), m00);
                        vst1_s16((rec + j + s_rec), m01);
                        vst1_s16((rec + j + 2 * s_rec), m02);
                        vst1_s16((rec + j + 3 * s_rec), m03);
                    }
                    pred += (cuw << 2);
                    coef += (cuw << 2);
                    rec += (s_rec << 2);
                }
            }
            else
            {
                for (i = 0; i < cuh; ++i)
                {
                    for (j = 0; j < cuw; ++j)
                    {
                        t0 = coef[i * cuw + j] + pred[i * cuw + j];
                        rec[i * s_rec + j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, t0);
                    }
                }
            }

        }
        else if ((cuh & 1) == 0)
        {
            if ((cuw & 0x7) == 0)
            {
                int16x8_t m00, m01, c00, c01, temp1, temp2;

                for (i = 0; i < cuh; i += 2)
                {
                    for (j = 0; j < cuw; j += 8)
                    {
                        m00 = vld1q_s16((pred + j));
                        m01 = vld1q_s16((pred + j + cuw));

                        c00 = vld1q_s16((coef + j));
                        c01 = vld1q_s16((coef + j + cuw));

                        m00 = vaddq_s16(m00, c00);
                        m01 = vaddq_s16(m01, c01);

                        temp1 = vminq_s16(m00, mm_max);
                        temp2 = vminq_s16(m01, mm_max);
                        m00 = vmaxq_s16(temp1, mm_min);
                        m01 = vmaxq_s16(temp2, mm_min);

                        vst1q_s16((rec + j), m00);
                        vst1q_s16((rec + j + s_rec), m01);
                    }
                    pred += (cuw << 1);
                    coef += (cuw << 1);
                    rec += (s_rec << 1);
                }
            }
            else if ((cuw & 0x3) == 0)
            {
                int16x4_t m00, m01, c00, c01, temp1, temp2;

                for (i = 0; i < cuh; i += 2)
                {
                    for (j = 0; j < cuw; j += 4)
                    {
                        m00 = vld1_s16((pred + j));
                        m01 = vld1_s16((pred + j + cuw));
                        c00 = vld1_s16((coef + j));
                        c01 = vld1_s16((coef + j + cuw));
                        m00 = vadd_s16(m00, c00);
                        m01 = vadd_s16(m01, c01);

                        temp1 = vmin_s16(m00, mm_max_x4);
                        temp2 = vmin_s16(m01, mm_max_x4);
                        m00 = vmax_s16(temp1, mm_min_x4);
                        m01 = vmax_s16(temp2, mm_min_x4);
                        vst1_s16((rec + j), m00);
                        vst1_s16((rec + j + s_rec), m01);
                    }
                    pred += (cuw << 1);
                    coef += (cuw << 1);
                    rec += (s_rec << 1);
                }
            }
            else
            {
                for (i = 0; i < cuh; ++i)
                {
                    for (j = 0; j < cuw; ++j)
                    {
                        t0 = coef[i * cuw + j] + pred[i * cuw + j];
                        rec[i * s_rec + j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, t0);
                    }
                }
            }

        }
    }
}
