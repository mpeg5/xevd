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


#include "xevdm_tbl.h"
#include "xevdm_mc.h"
#include "xevdm_util.h"
#include <assert.h>



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


/* padding for store intermediate values, which should be larger than
1+ half of filter tap */
#define MC_IBUF_PAD_C          8
#define MC_IBUF_PAD_L          8
#define MC_IBUF_PAD_BL         2


static int g_aff_mvDevBB2_125[5] = { 128, 256, 544, 1120, 2272 };


const s16 xevd_tbl_bl_mc_l_coeff[4 << MC_PRECISION_ADD][2] =
{
    { 64,  0 },
#if MC_PRECISION_ADD
    { 60,  4 },
    { 56,  8 },
    { 52, 12 },
#endif
    { 48, 16 },
#if MC_PRECISION_ADD
    { 44, 20 },
    { 40, 24 },
    { 36, 28 },
#endif
    { 32, 32 },
#if MC_PRECISION_ADD
    { 28, 36 },
    { 24, 40 },
    { 20, 44 },
#endif
    { 16, 48 },
#if MC_PRECISION_ADD
    { 12, 52 },
    { 8,  56 },
    { 4,  60 }
#endif
};

s16 tbl_mc_l_coeff_main[16][8] =
{
    {  0, 0,   0, 64,  0,   0,  0,  0 },
    {  0, 1,  -3, 63,  4,  -2,  1,  0 },
    { -1, 2,  -5, 62,  8,  -3,  1,  0 },
    { -1, 3,  -8, 60, 13,  -4,  1,  0 },
    { -1, 4, -10, 58, 17,  -5,  1,  0 },
    { -1, 4, -11, 52, 26,  -8,  3, -1 },
    { -1, 3,  -9, 47, 31, -10,  4, -1 },
    { -1, 4, -11, 45, 34, -10,  4, -1 },
    { -1, 4, -11, 40, 40, -11,  4, -1 },
    { -1, 4, -10, 34, 45, -11,  4, -1 },
    { -1, 4, -10, 31, 47,  -9,  3, -1 },
    { -1, 3,  -8, 26, 52, -11,  4, -1 },
    {  0, 1,  -5, 17, 58, -10,  4, -1 },
    {  0, 1,  -4, 13, 60,  -8,  3, -1 },
    {  0, 1,  -3,  8, 62,  -5,  2, -1 },
    {  0, 1,  -2,  4, 63,  -3,  1,  0 },
};

s16 tbl_mc_c_coeff_main[32][4] =
{
    {  0, 64,  0,  0 },
    { -1, 63,  2,  0 },
    { -2, 62,  4,  0 },
    { -2, 60,  7, -1 },
    { -2, 58, 10, -2 },
    { -3, 57, 12, -2 },
    { -4, 56, 14, -2 },
    { -4, 55, 15, -2 },
    { -4, 54, 16, -2 },
    { -5, 53, 18, -2 },
    { -6, 52, 20, -2 },
    { -6, 49, 24, -3 },
    { -6, 46, 28, -4 },
    { -5, 44, 29, -4 },
    { -4, 42, 30, -4 },
    { -4, 39, 33, -4 },
    { -4, 36, 36, -4 },
    { -4, 33, 39, -4 },
    { -4, 30, 42, -4 },
    { -4, 29, 44, -5 },
    { -4, 28, 46, -6 },
    { -3, 24, 49, -6 },
    { -2, 20, 52, -6 },
    { -2, 18, 53, -5 },
    { -2, 16, 54, -4 },
    { -2, 15, 55, -4 },
    { -2, 14, 56, -4 },
    { -2, 12, 57, -3 },
    { -2, 10, 58, -2 },
    { -1,  7, 60, -2 },
    {  0,  4, 62, -2 },
    {  0,  2, 63, -1 },
};


static const s16 tbl_bl_eif_32_phases_mc_l_coeff[32][2] =
{
  { 64, 0  },
  { 62, 2  },
  { 60, 4  },
  { 58, 6  },
  { 56, 8  },
  { 54, 10 },
  { 52, 12 },
  { 50, 14 },
  { 48, 16 },
  { 46, 18 },
  { 44, 20 },
  { 42, 22 },
  { 40, 24 },
  { 38, 26 },
  { 36, 28 },
  { 34, 30 },
  { 32, 32 },
  { 30, 34 },
  { 28, 36 },
  { 26, 38 },
  { 24, 40 },
  { 22, 42 },
  { 20, 44 },
  { 18, 46 },
  { 16, 48 },
  { 14, 50 },
  { 12, 52 },
  { 10, 54 },
  { 8,  56 },
  { 6,  58 },
  { 4,  60 },
  { 2,  62 }
};

XEVDM_DMVR_MC_L(*xevdm_func_dmvr_mc_l)[2];
XEVDM_DMVR_MC_C(*xevdm_func_dmvr_mc_c)[2];
XEVD_MC_C(*xevdm_func_bl_mc_l)[2];
//XEVDM_DMVR_SAD *xevdm_func_dmvr_sad;

/****************************************************************************
 * motion compensation for luma
 ****************************************************************************/


void xevd_mc_dmvr_l_00(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int i, j;

#if MC_PRECISION_ADD
    gmv_x >>= 4;
    gmv_y >>= 4;
#else
    gmv_x >>= 2;
    gmv_y >>= 2;
#endif
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

void xevd_mc_dmvr_l_n0(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int i, j, dx;
    s32 pt;

#if MC_PRECISION_ADD
    dx = gmv_x & 15;
    //ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4) - 3;
    ref = ref - 3;
#else
    dx = gmv_x & 0x3;
    ref += (gmv_y >> 2) * s_ref + (gmv_x >> 2) - 3;
#endif
    {
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                pt = MAC_8TAP_N0(tbl_mc_l_coeff[dx], ref[j], ref[j + 1], ref[j + 2], ref[j + 3], ref[j + 4], ref[j + 5], ref[j + 6], ref[j + 7]);

                pred[j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, pt);

            }
            ref += s_ref;
            pred += s_pred;
        }
    }
}

void xevd_mc_dmvr_l_0n(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int i, j, dy;
    s32 pt;

#if MC_PRECISION_ADD
    dy = gmv_y & 15;
    //ref += ((gmv_y >> 4) - 3) * s_ref + (gmv_x >> 4);
    ref = ref - (3 * s_ref);
#else
    dy = gmv_y & 0x3;
    ref += ((gmv_y >> 2) - 3) * s_ref + (gmv_x >> 2);
#endif
    {
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                pt = MAC_8TAP_0N(tbl_mc_l_coeff[dy], ref[j], ref[s_ref + j], ref[s_ref * 2 + j], ref[s_ref * 3 + j], ref[s_ref * 4 + j], ref[s_ref * 5 + j], ref[s_ref * 6 + j], ref[s_ref * 7 + j]);

                pred[j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, pt);


            }
            ref += s_ref;
            pred += s_pred;
        }
    }
}

void xevd_mc_dmvr_l_nn(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_L)*MAX_CU_SIZE];
    s16        *b;
    int         i, j, dx, dy;
    s32         pt;

#if MC_PRECISION_ADD
    dx = gmv_x & 15;
    dy = gmv_y & 15;
    //  ref += ((gmv_y >> 4) - 3) * s_ref + (gmv_x >> 4) - 3;
    ref = ref - (3 * s_ref + 3);
#else
    dx = gmv_x & 0x3;
    dy = gmv_y & 0x3;
    ref += ((gmv_y >> 2) - 3) * s_ref + (gmv_x >> 2) - 3;
#endif


    int shift1 = XEVD_MIN(4, bit_depth - 8);
    int shift2 = XEVD_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));

    {
        b = buf;
        for (i = 0; i < h + 7; i++)
        {
            for (j = 0; j < w; j++)
            {
                b[j] = MAC_8TAP_NN_S1(tbl_mc_l_coeff[dx], ref[j], ref[j + 1], ref[j + 2], ref[j + 3], ref[j + 4], ref[j + 5], ref[j + 6], ref[j + 7], offset1, shift1);
            }
            ref += s_ref;
            b += w;
        }

        b = buf;
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                pt = MAC_8TAP_NN_S2(tbl_mc_l_coeff[dy], b[j], b[j + w], b[j + w * 2], b[j + w * 3], b[j + w * 4], b[j + w * 5], b[j + w * 6], b[j + w * 7], offset2, shift2);
                pred[j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, pt);
            }
            pred += s_pred;
            b += w;
        }
    }
}


void xevdm_bl_mc_l_00(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int i, j;

#if MC_PRECISION_ADD
    gmv_x >>= 4;
    gmv_y >>= 4;
#else
    gmv_x >>= 2;
    gmv_y >>= 2;
#endif

    ref += gmv_y * s_ref + gmv_x;

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

void xevdm_bl_mc_l_n0(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int i, j, dx;
    s32 pt;


#if MC_PRECISION_ADD
    dx = gmv_x & 15;
    ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4);
#else
    dx = gmv_x & 0x3;
    ref += (gmv_y >> 2) * s_ref + (gmv_x >> 2);
#endif

    {
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                pt = MAC_BL_N0(xevd_tbl_bl_mc_l_coeff[dx], ref[j], ref[j + 1]);
                pred[j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, pt);
            }
            ref += s_ref;
            pred += s_pred;
        }
    }
}

void xevdm_bl_mc_l_0n(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int i, j, dy;
    s32 pt;

#if MC_PRECISION_ADD
    dy = gmv_y & 15;
    ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4);
#else
    dy = gmv_y & 0x3;
    ref += (gmv_y >> 2) * s_ref + (gmv_x >> 2);
#endif
    {
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                pt = MAC_BL_0N(xevd_tbl_bl_mc_l_coeff[dy], ref[j], ref[s_ref + j]);
                pred[j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, pt);
            }
            ref += s_ref;
            pred += s_pred;
        }
    }
}

void xevdm_bl_mc_l_nn(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_L)*(MAX_CU_SIZE + MC_IBUF_PAD_L)];
    s16        *b;
    int         i, j, dx, dy;
    s32         pt;

#if MC_PRECISION_ADD
    dx = gmv_x & 15;
    dy = gmv_y & 15;
    ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4);
#else
    dx = gmv_x & 0x3;
    dy = gmv_y & 0x3;
    ref += (gmv_y >> 2) * s_ref + (gmv_x >> 2);
#endif


    int shift1 = XEVD_MIN(4, bit_depth - 8);
    int shift2 = XEVD_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));

    {
        b = buf;
        for (i = 0; i < h + 1; i++)
        {
            for (j = 0; j < w; j++)
            {
                b[j] = MAC_BL_NN_S1(xevd_tbl_bl_mc_l_coeff[dx], ref[j], ref[j + 1], offset1, shift1);
            }
            ref += s_ref;
            b += w;
        }

        b = buf;
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                pt = MAC_BL_NN_S2(xevd_tbl_bl_mc_l_coeff[dy], b[j], b[j + w], offset2, shift2);
                pred[j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, pt);
            }
            pred += s_pred;
            b += w;
        }
    }
}

/****************************************************************************
 * motion compensation for chroma
 ****************************************************************************/


void xevd_mc_dmvr_c_00(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int i, j;

#if MC_PRECISION_ADD
    gmv_x >>= 5;
    gmv_y >>= 5;
#else
    gmv_x >>= 3;
    gmv_y >>= 3;
#endif

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

void xevd_mc_dmvr_c_n0(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int       i, j, dx;
    s32       pt;

#if MC_PRECISION_ADD
    dx = gmv_x & 31;
    ref -= 1;
#else
    dx = gmv_x & 0x7;
    ref += (gmv_y >> 3) * s_ref + (gmv_x >> 3) - 1;
#endif

    {
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                pt = MAC_4TAP_N0(tbl_mc_c_coeff[dx], ref[j], ref[j + 1], ref[j + 2], ref[j + 3]);
                pred[j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, pt);
            }
            pred += s_pred;
            ref += s_ref;
        }
    }
}

void xevd_mc_dmvr_c_0n(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int i, j, dy;
    s32       pt;

#if MC_PRECISION_ADD
    dy = gmv_y & 31;
    ref -= 1 * s_ref;
#else
    dy = gmv_y & 0x7;
    ref += ((gmv_y >> 3) - 1) * s_ref + (gmv_x >> 3);
#endif

    {
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                pt = MAC_4TAP_0N(tbl_mc_c_coeff[dy], ref[j], ref[s_ref + j], ref[s_ref * 2 + j], ref[s_ref * 3 + j]);
                pred[j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, pt);
            }
            pred += s_pred;
            ref += s_ref;
        }
    }
}

void xevd_mc_dmvr_c_nn(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_C)*MAX_CU_SIZE];
    s16        *b;
    int         i, j;
    s32         pt;
    int         dx, dy;

#if MC_PRECISION_ADD
    dx = gmv_x & 31;
    dy = gmv_y & 31;
    //ref += ((gmv_y >> 5) - 1) * s_ref + (gmv_x >> 5) - 1;
    ref -= (1 * s_ref + 1);
#else
    dx = gmv_x & 0x7;
    dy = gmv_y & 0x7;
    ref += ((gmv_y >> 3) - 1) * s_ref + (gmv_x >> 3) - 1;
#endif


    int shift1 = XEVD_MIN(4, bit_depth - 8);
    int shift2 = XEVD_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));

    {
        b = buf;
        for (i = 0; i < h + 3; i++)
        {
            for (j = 0; j < w; j++)
            {
                b[j] = MAC_4TAP_NN_S1(tbl_mc_c_coeff[dx], ref[j], ref[j + 1], ref[j + 2], ref[j + 3], offset1, shift1);
            }
            ref += s_ref;
            b += w;
        }

        b = buf;
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                pt = MAC_4TAP_NN_S2(tbl_mc_c_coeff[dy], b[j], b[j + w], b[j + 2 * w], b[j + 3 * w],offset2, shift2);
                pred[j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, pt);
            }
            pred += s_pred;
            b += w;
        }
    }
}



XEVD_MC_L xevdm_tbl_dmvr_mc_l[2][2] =
{
  {
    xevd_mc_dmvr_l_00, /* dx == 0 && dy == 0 */
    xevd_mc_dmvr_l_0n  /* dx == 0 && dy != 0 */
  },
  {
    xevd_mc_dmvr_l_n0, /* dx != 0 && dy == 0 */
    xevd_mc_dmvr_l_nn  /* dx != 0 && dy != 0 */
  }
};

XEVD_MC_C xevdm_tbl_dmvr_mc_c[2][2] =
{
  {
    xevd_mc_dmvr_c_00, /* dx == 0 && dy == 0 */
    xevd_mc_dmvr_c_0n  /* dx == 0 && dy != 0 */
  },
  {
    xevd_mc_dmvr_c_n0, /* dx != 0 && dy == 0 */
    xevd_mc_dmvr_c_nn  /* dx != 0 && dy != 0 */
  }
};


/* luma and chroma will remain the same */
XEVD_MC_L xevdm_tbl_bl_mc_l[2][2] =
{
    {
        xevdm_bl_mc_l_00,
        xevdm_bl_mc_l_0n
    },
    {
        xevdm_bl_mc_l_n0,
        xevdm_bl_mc_l_nn
    }
};

typedef enum XEVD_POINT_INDEX
{
    CENTER = 0,
    CENTER_TOP,
    CENTER_BOTTOM,
    LEFT_CENTER,
    RIGHT_CENTER,
    LEFT_TOP,
    RIGHT_TOP,
    LEFT_BOTTOM,
    RIGHT_BOTTOM,
    REF_PRED_POINTS_INDEXS_NUM
} XEVD_POINT_INDEX;
enum NSAD_SRC
{
    SRC1 = 0,
    SRC2,
    SRC_NUM
};
enum NSAD_BORDER_LINE
{
    LINE_TOP = 0,
    LINE_BOTTOM,
    LINE_LEFT,
    LINE_RIGHT,
    LINE_NUM
};
int xevdm_dmvr_sad(int w, int h, pel * src1, pel * src2,
    int s_src1, int s_src2, s16 delta, int bit_depth)
{
    int sad;
    sad = 0;
    int i, j;
    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            sad += abs(src1[j] - (src2[j] << 1) - delta);
        }
        src1 += s_src1;
        src2 += s_src2;
    }
    return sad;
}
s32 simple_sad(int w, int h, pel *src1, pel *src2, int s_src1, int s_src2, s32(*average_for_next_interation)[REF_PRED_POINTS_INDEXS_NUM], s32 *avg_borders_array, s32 *avg_extra_borders_array, XEVD_POINT_INDEX point_index
    , s32 *center_point_avgs_l0_l1, u8 l0_or_l1, BOOL do_not_sad, int bit_depth)
{
    int i, j;
    s32 sad = 0;
    s32 avg2 = 0;
    pel *src1_org = src1;
    pel *src2_org = src2;
    s32 delta;

    s32 mean_a;
    s32 mean_b;
    s32 mean_c;
    s32 mean_t;

    if (avg_borders_array == NULL)
    {
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                avg2 += src2[j];
            }
            src2 += s_src2;
        }
    }
    else
    {
        avg2 = center_point_avgs_l0_l1[l0_or_l1];
        switch (point_index)
        {
        case CENTER:
            // borders calculation
            // top border
            src1 = src2_org;
            src2 = src1 - REF_PRED_EXTENTION_PEL_COUNT * s_src2;
            for (j = 0; j < w; j++)
            {
                avg_borders_array[LINE_TOP] += src1[j];
                avg_extra_borders_array[LINE_TOP] += src2[j];
            }
            // bottom border
            src1 = src2_org + (h - REF_PRED_EXTENTION_PEL_COUNT) * s_src2;
            src2 = src1 + REF_PRED_EXTENTION_PEL_COUNT * s_src2;
            for (j = 0; j < w; j++)
            {
                avg_borders_array[LINE_BOTTOM] += src1[j];
                avg_extra_borders_array[LINE_BOTTOM] += src2[j];
            }
            // left border
            src1 = src2_org;
            src2 = src1 - REF_PRED_EXTENTION_PEL_COUNT;
            for (j = 0; j < h; j++)
            {
                avg_borders_array[LINE_LEFT] += *src1;
                avg_extra_borders_array[LINE_LEFT] += *src2;
                src1 += s_src2;
                src2 += s_src2;
            }
            // right border
            src1 = src2_org + w - REF_PRED_EXTENTION_PEL_COUNT;
            src2 = src1 + REF_PRED_EXTENTION_PEL_COUNT;
            for (j = 0; j < h; j++)
            {
                avg_borders_array[LINE_RIGHT] += *src1;
                avg_extra_borders_array[LINE_RIGHT] += *src2;
                src1 += s_src2;
                src2 += s_src2;
            }
            // preparing src2 averages for the next iterations
            average_for_next_interation[SRC2][CENTER_BOTTOM] = avg2 + avg_extra_borders_array[LINE_BOTTOM] - avg_borders_array[LINE_TOP];

            // center top
            average_for_next_interation[SRC2][CENTER_TOP] = avg2 + avg_extra_borders_array[LINE_TOP] - avg_borders_array[LINE_BOTTOM];

            // left center
            average_for_next_interation[SRC2][LEFT_CENTER] = avg2 + avg_extra_borders_array[LINE_LEFT] - avg_borders_array[LINE_RIGHT];

            // right center
            average_for_next_interation[SRC2][RIGHT_CENTER] = avg2 + avg_extra_borders_array[LINE_RIGHT] - avg_borders_array[LINE_LEFT];
            break;
        case CENTER_TOP:
        case CENTER_BOTTOM:
        case LEFT_CENTER:
        case RIGHT_CENTER:
            avg2 = average_for_next_interation[SRC2][point_index];
            break;
        case LEFT_TOP:
            // substruct right and bottom lines
            avg2 = average_for_next_interation[SRC2][CENTER];
            avg2 -= avg_borders_array[LINE_RIGHT];
            avg2 -= avg_borders_array[LINE_BOTTOM];
            // add left and top lines
            avg2 += avg_extra_borders_array[LINE_LEFT];
            avg2 += avg_extra_borders_array[LINE_TOP];
            // add left top corner pel, due to not added by lines
            avg2 += *src2;
            src2 += w;
            // substract right edge top line point
            avg2 -= *src2;
            src2 += h * s_src2;
            // add right bottom corner pel, due to substructed by lines
            avg2 += *src2;
            src2 -= w;
            // substract bottom edge left line point
            avg2 -= *src2;
            break;
        case RIGHT_TOP:
            // substruct left and bottom lines
            avg2 = average_for_next_interation[SRC2][CENTER];
            avg2 -= avg_borders_array[LINE_LEFT];
            avg2 -= avg_borders_array[LINE_BOTTOM];
            // add right and top lines
            avg2 += avg_extra_borders_array[LINE_RIGHT];
            avg2 += avg_extra_borders_array[LINE_TOP];
            src2 -= REF_PRED_EXTENTION_PEL_COUNT;
            // substract left edge top line point
            avg2 -= *src2;
            src2 += w;
            // add right top corner pel, due to not added by lines
            avg2 += *src2;
            src2 += h * s_src2;
            // substract right edge bottom line point
            avg2 -= *src2;
            src2 -= w;
            // add left bottom corner pel, due to substructed by lines
            avg2 += *src2;
            break;
        case LEFT_BOTTOM:
            // substruct right and top lines
            avg2 = average_for_next_interation[SRC2][CENTER];
            avg2 -= avg_borders_array[LINE_RIGHT];
            avg2 -= avg_borders_array[LINE_TOP];
            // add left and bottom lines
            avg2 += avg_extra_borders_array[LINE_LEFT];
            avg2 += avg_extra_borders_array[LINE_BOTTOM];
            src2 -= REF_PRED_EXTENTION_PEL_COUNT * s_src2;
            // substract left edge top line point
            avg2 -= *src2;
            src2 += w;
            // add right top corner pel, due to substructed by lines
            avg2 += *src2;
            src2 += h * s_src2;
            // substract bottom edge right line point
            avg2 -= *src2;
            src2 -= w;
            // add left bottom corner pel, due to not added by lines
            avg2 += *src2;
            break;
        case RIGHT_BOTTOM:
            // substruct left and top lines
            avg2 = average_for_next_interation[SRC2][CENTER];
            avg2 -= avg_borders_array[LINE_LEFT];
            avg2 -= avg_borders_array[LINE_TOP];
            // add right and bottom lines
            avg2 += avg_extra_borders_array[LINE_RIGHT];
            avg2 += avg_extra_borders_array[LINE_BOTTOM];
            src2 -= (REF_PRED_EXTENTION_PEL_COUNT * s_src2 + REF_PRED_EXTENTION_PEL_COUNT);
            // add left top corner pel, due to substructed by lines
            avg2 += *src2;
            src2 += w;
            // substract right edge top line point
            avg2 -= *src2;
            src2 += h * s_src2;
            // add right bottom corner pel, due to not added by lines
            avg2 += *src2;
            src2 -= w;
            // substract left edge bottom line point
            avg2 -= *src2;
            break;
        default:
            assert(!"Undefined case");
            break;
        }
        average_for_next_interation[SRC2][point_index] = avg2;
    }
    if (do_not_sad)
    {
        return 0;
    }
    src1 = src1_org;
    src2 = src2_org;

    mean_a = (((center_point_avgs_l0_l1[2 + !l0_or_l1] << 5) / (w*h)) + 16) >> 5;
    mean_b = (((center_point_avgs_l0_l1[2 + l0_or_l1] << 5) / (w*h)) + 16) >> 5;
    mean_c = (((avg2 << 5) / (w*h)) + 16) >> 5;
    mean_t = mean_a + mean_b;
    delta = mean_t - 2 * mean_c;

    sad = xevdm_dmvr_sad(w, h, src1, src2, s_src1, s_src2, delta, bit_depth);


    return sad;
}

void generate_template(pel *dst, pel *src1, s16 *src2, s32 src1_stride, s32 src2_stride, int w, int h)
{
    int i, j;
    for (i = 0; i < h; ++i)
    {
        for (j = 0; j < w; ++j)
        {
            dst[j] = (src1[j] + src2[j]);
        }
        dst += w;
        src1 += src1_stride;
        src2 += src2_stride;
    }
}

static void mv_clip_only_one_ref(int x, int y, int pic_w, int pic_h, int w, int h, s16 mv[MV_D], s16(*mv_t))
{
    int min_clip[MV_D], max_clip[MV_D];

    x <<= 2;
    y <<= 2;
    w <<= 2;
    h <<= 2;
    min_clip[MV_X] = -(MAX_CU_SIZE << 2);
    min_clip[MV_Y] = -(MAX_CU_SIZE << 2);
    max_clip[MV_X] = (pic_w - 1 + MAX_CU_SIZE) << 2;
    max_clip[MV_Y] = (pic_h - 1 + MAX_CU_SIZE) << 2;

    mv_t[MV_X] = mv[MV_X];
    mv_t[MV_Y] = mv[MV_Y];

    if (x + mv[MV_X] < min_clip[MV_X]) mv_t[MV_X] = min_clip[MV_X] - x;
    if (y + mv[MV_Y] < min_clip[MV_Y]) mv_t[MV_Y] = min_clip[MV_Y] - y;
    if (x + mv[MV_X] + w - 4 > max_clip[MV_X]) mv_t[MV_X] = max_clip[MV_X] - x - w + 4;
    if (y + mv[MV_Y] + h - 4 > max_clip[MV_Y]) mv_t[MV_Y] = max_clip[MV_Y] - y - h + 4;
}

static BOOL mv_clip_only_one_ref_dmvr(int x, int y, int pic_w, int pic_h, int w, int h, s16 mv[MV_D], s16(*mv_t))
{
    BOOL clip_flag = 0;
    int min_clip[MV_D], max_clip[MV_D];

    x <<= 2;
    y <<= 2;
    w <<= 2;
    h <<= 2;
    min_clip[MV_X] = -(MAX_CU_SIZE << 2);
    min_clip[MV_Y] = -(MAX_CU_SIZE << 2);
    max_clip[MV_X] = (pic_w - 1 + MAX_CU_SIZE) << 2;
    max_clip[MV_Y] = (pic_h - 1 + MAX_CU_SIZE) << 2;


    mv_t[MV_X] = mv[MV_X];
    mv_t[MV_Y] = mv[MV_Y];

    if (x + mv[MV_X] < min_clip[MV_X])
    {
        clip_flag = 1;
        mv_t[MV_X] = min_clip[MV_X] - x;
    }
    if (y + mv[MV_Y] < min_clip[MV_Y])
    {
        clip_flag = 1;
        mv_t[MV_Y] = min_clip[MV_Y] - y;
    }
    if (x + mv[MV_X] + w - 4 > max_clip[MV_X])
    {
        clip_flag = 1;
        mv_t[MV_X] = max_clip[MV_X] - x - w + 4;
    }
    if (y + mv[MV_Y] + h - 4 > max_clip[MV_Y])
    {
        clip_flag = 1;
        mv_t[MV_Y] = max_clip[MV_Y] - y - h + 4;
    }
    return clip_flag;
}



pel* refinement_motion_vectors_in_one_ref(int x, int y, int pic_w, int pic_h, int w, int h, const XEVD_PIC *ref_pic, s16(*mv), pel dmvr_current_template[MAX_CU_SIZE*MAX_CU_SIZE], pel(*dmvr_ref_pred_interpolated), int stride
    , BOOL calculate_center, s32 *center_cost
    , s32 *center_point_avgs_l0_l1, u8 l0_or_l1, int bit_depth)
{
    int BEST_COST_FROM_INTEGER = 0;
    int COST_FROM_L0 = 1;

    s16    ref_pred_mv_scaled_step = REF_PRED_EXTENTION_PEL_COUNT << 2;
    s32 offset = 0;

    s32 average_for_next_interation[SRC_NUM][REF_PRED_POINTS_INDEXS_NUM] =
    {
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    };

    s32 avg_extra_borders_array[LINE_NUM] = { 0, 0, 0, 0 };
    s32 avg_borders_array[LINE_NUM] = { 0, 0, 0, 0 };
    u8    points_flag[REF_PRED_POINTS_CROSS] = { FALSE, FALSE, FALSE, FALSE, FALSE };
    s32    cost[REF_PRED_POINTS_NUM] = { 1 << 30, 1 << 30, 1 << 30, 1 << 30, 1 << 30, 1 << 30, 1 << 30, 1 << 30, 1 << 30 };
    s32 cost_best = 1 << 30;
    XEVD_POINT_INDEX point_index;
    XEVD_POINT_INDEX best_cost_point = CENTER;

    // in a plus sign order, starting with the central point
    // cost calculation firstly only for 5 points in the plus sign
    for (point_index = CENTER; point_index < REF_PRED_POINTS_CROSS; ++point_index)
    {
        if (l0_or_l1 == REFP_1 && point_index == CENTER)
        {
            cost[point_index] = center_cost[COST_FROM_L0];
            simple_sad(w, h, dmvr_current_template, dmvr_ref_pred_interpolated + offset, w, stride, average_for_next_interation, avg_borders_array, avg_extra_borders_array, point_index
                , center_point_avgs_l0_l1, l0_or_l1, TRUE
                ,  bit_depth);
        }
        else
        {
            cost[point_index] = simple_sad(w, h, dmvr_current_template, dmvr_ref_pred_interpolated + offset, w, stride, average_for_next_interation, avg_borders_array, avg_extra_borders_array, point_index
                , center_point_avgs_l0_l1, l0_or_l1, FALSE
                , bit_depth);
        }
        switch (point_index)
        {
        case CENTER:
            if (l0_or_l1 == REFP_0)
            {
                center_cost[COST_FROM_L0] = cost[point_index];
            }

            offset -= REF_PRED_EXTENTION_PEL_COUNT * stride;
            break;
        case CENTER_TOP:
            offset += (REF_PRED_EXTENTION_PEL_COUNT << 1) * stride;
            break;
        case CENTER_BOTTOM:
            offset -= (REF_PRED_EXTENTION_PEL_COUNT * stride + REF_PRED_EXTENTION_PEL_COUNT);
            break;
        case LEFT_CENTER:
            offset += (REF_PRED_EXTENTION_PEL_COUNT << 1);
            break;
        case RIGHT_CENTER:
            break;

        default:
            assert(!"Undefined case");
            break;
        }
    }
    // check which additional point we need
    if (cost[CENTER_TOP] < cost[CENTER_BOTTOM])
    {
        points_flag[CENTER_TOP] = TRUE;
    }
    else
    {
        points_flag[CENTER_BOTTOM] = TRUE;
    }

    if (cost[LEFT_CENTER] < cost[RIGHT_CENTER])
    {
        points_flag[LEFT_CENTER] = TRUE;
    }
    else
    {
        points_flag[RIGHT_CENTER] = TRUE;
    }

    // left top point
    if (points_flag[CENTER_TOP] && points_flag[LEFT_CENTER])
    {
        // obtain left top point position from last used right center point
        offset -= (REF_PRED_EXTENTION_PEL_COUNT * stride + (REF_PRED_EXTENTION_PEL_COUNT << 1));
        cost[LEFT_TOP] = simple_sad(w, h, dmvr_current_template, dmvr_ref_pred_interpolated + offset, w, stride, average_for_next_interation, avg_borders_array, avg_extra_borders_array, LEFT_TOP
            , center_point_avgs_l0_l1, l0_or_l1, FALSE
            , bit_depth);
    }
    // right top point
    else if (points_flag[CENTER_TOP] && points_flag[RIGHT_CENTER])
    {
        // obtain right top point position from last used right center point
        offset -= (REF_PRED_EXTENTION_PEL_COUNT * stride);
        cost[RIGHT_TOP] = simple_sad(w, h, dmvr_current_template, dmvr_ref_pred_interpolated + offset, w, stride, average_for_next_interation, avg_borders_array, avg_extra_borders_array, RIGHT_TOP
            , center_point_avgs_l0_l1, l0_or_l1, FALSE
            , bit_depth);
    }
    // left bottom point
    else if (points_flag[CENTER_BOTTOM] && points_flag[LEFT_CENTER])
    {
        // obtain left bottom point position from last used right center point
        offset -= (-(REF_PRED_EXTENTION_PEL_COUNT * stride) + (REF_PRED_EXTENTION_PEL_COUNT << 1));
        cost[LEFT_BOTTOM] = simple_sad(w, h, dmvr_current_template, dmvr_ref_pred_interpolated + offset, w, stride, average_for_next_interation, avg_borders_array, avg_extra_borders_array, LEFT_BOTTOM
            , center_point_avgs_l0_l1, l0_or_l1, FALSE
            , bit_depth);
    }
    // right bottom point
    else if (points_flag[CENTER_BOTTOM] && points_flag[RIGHT_CENTER])
    {
        // obtain right bottom point position from last used right center point
        offset += (REF_PRED_EXTENTION_PEL_COUNT * stride);
        cost[RIGHT_BOTTOM] = simple_sad(w, h, dmvr_current_template, dmvr_ref_pred_interpolated + offset, w, stride, average_for_next_interation, avg_borders_array, avg_extra_borders_array, RIGHT_BOTTOM
            , center_point_avgs_l0_l1, l0_or_l1, FALSE
            , bit_depth);
    }

    for (point_index = CENTER; point_index < REF_PRED_POINTS_NUM; ++point_index)
    {
        if (cost[point_index] < cost_best)
        {
            cost_best = cost[point_index];
            best_cost_point = point_index;
        }
    }

    center_cost[BEST_COST_FROM_INTEGER] = cost[best_cost_point];
    center_point_avgs_l0_l1[l0_or_l1] = average_for_next_interation[SRC2][best_cost_point];
    // refine oroginal MV
    switch (best_cost_point)
    {
    case CENTER:
        offset = 0;

        break;
    case CENTER_TOP:
        offset = -REF_PRED_EXTENTION_PEL_COUNT * stride;

        mv[MV_Y] -= ref_pred_mv_scaled_step;
        break;
    case CENTER_BOTTOM:
        offset = REF_PRED_EXTENTION_PEL_COUNT * stride;

        mv[MV_Y] += ref_pred_mv_scaled_step;
        break;
    case LEFT_CENTER:
        offset = -REF_PRED_EXTENTION_PEL_COUNT;

        mv[MV_X] -= ref_pred_mv_scaled_step;
        break;
    case RIGHT_CENTER:
        offset = REF_PRED_EXTENTION_PEL_COUNT;

        mv[MV_X] += ref_pred_mv_scaled_step;
        break;
    case LEFT_TOP:
        offset = -REF_PRED_EXTENTION_PEL_COUNT * stride - REF_PRED_EXTENTION_PEL_COUNT;

        mv[MV_X] -= ref_pred_mv_scaled_step;
        mv[MV_Y] -= ref_pred_mv_scaled_step;
        break;
    case RIGHT_TOP:
        offset = -REF_PRED_EXTENTION_PEL_COUNT * stride + REF_PRED_EXTENTION_PEL_COUNT;

        mv[MV_X] += ref_pred_mv_scaled_step;
        mv[MV_Y] -= ref_pred_mv_scaled_step;
        break;
    case LEFT_BOTTOM:
        offset = REF_PRED_EXTENTION_PEL_COUNT * stride - REF_PRED_EXTENTION_PEL_COUNT;

        mv[MV_X] -= ref_pred_mv_scaled_step;
        mv[MV_Y] += ref_pred_mv_scaled_step;
        break;
    case RIGHT_BOTTOM:
        offset = REF_PRED_EXTENTION_PEL_COUNT * stride + REF_PRED_EXTENTION_PEL_COUNT;

        mv[MV_X] += ref_pred_mv_scaled_step;
        mv[MV_Y] += ref_pred_mv_scaled_step;
        break;

    default:
        assert(!"Undefined case");
        break;
    }

    return dmvr_ref_pred_interpolated + offset;
}

void copy_to_dst(int w, int h, pel *src, pel *dst, int s_stride, int d_stride)
{
    int i;
    for (i = 0; i < h; i++)
    {
        xevd_mcpy(dst, src, sizeof(pel) * w);
        src += s_stride;
        dst += d_stride;
    }
}

void predict_new_line(int x, int y, int pic_w, int pic_h, const XEVD_PIC *ref_pic, const s16(*mv), const s16(*mv_current), pel *preds_array, const s16(*mv_offsets), int stride, int w, int h
    , BOOL all_sides
    , int bit_depth_luma, int bit_depth_chroma)
{
    s16       ref_pred_mv_scaled_step = REF_PRED_EXTENTION_PEL_COUNT << 2;
    int       qpel_gmv_x, qpel_gmv_y;
    s16       mv_t[MV_D];
    s16       new_mv_t[MV_D];
    s32       pred_buffer_offset;

    if (mv_offsets[MV_X]
        || all_sides
        )
    {
        // go right
        if (mv_offsets[MV_X] > 0
            || all_sides
            )
        {
            new_mv_t[MV_X] = mv[MV_X] + ref_pred_mv_scaled_step * w;
            new_mv_t[MV_Y] = mv[MV_Y] - ref_pred_mv_scaled_step;
            mv_clip_only_one_ref(x, y, pic_w, pic_h, w, h, new_mv_t, mv_t);
            qpel_gmv_x = (x << 2) + mv_t[MV_X];
            qpel_gmv_y = (y << 2) + mv_t[MV_Y];
            pred_buffer_offset = -REF_PRED_EXTENTION_PEL_COUNT * stride + REF_PRED_EXTENTION_PEL_COUNT * w;

            xevdm_bl_mc_l(ref_pic->y, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_l, stride, preds_array + pred_buffer_offset, REF_PRED_EXTENTION_PEL_COUNT, h + (REF_PRED_EXTENTION_PEL_COUNT << 1), bit_depth_luma);
        }
        // go left

        if (mv_offsets[MV_X] < 0
            || all_sides
            )
        {
            new_mv_t[MV_X] = mv[MV_X] - ref_pred_mv_scaled_step;
            new_mv_t[MV_Y] = mv[MV_Y] - ref_pred_mv_scaled_step;
            mv_clip_only_one_ref(x, y, pic_w, pic_h, w, h, new_mv_t, mv_t);
            qpel_gmv_x = (x << 2) + mv_t[MV_X];
            qpel_gmv_y = (y << 2) + mv_t[MV_Y];
            pred_buffer_offset = -REF_PRED_EXTENTION_PEL_COUNT * stride - REF_PRED_EXTENTION_PEL_COUNT;

            xevdm_bl_mc_l(ref_pic->y, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_l, stride, preds_array + pred_buffer_offset, REF_PRED_EXTENTION_PEL_COUNT, h + (REF_PRED_EXTENTION_PEL_COUNT << 1), bit_depth_luma);
        }
    }

    if (mv_offsets[MV_Y]
        || all_sides
        )
    {
        // go down
        if (mv_offsets[MV_Y] > 0
            || all_sides
            )
        {
            new_mv_t[MV_X] = mv[MV_X] - ref_pred_mv_scaled_step;
            new_mv_t[MV_Y] = mv[MV_Y] + ref_pred_mv_scaled_step * (h);
            mv_clip_only_one_ref(x, y, pic_w, pic_h, w, h, new_mv_t, mv_t);
            qpel_gmv_x = (x << 2) + mv_t[MV_X];
            qpel_gmv_y = (y << 2) + mv_t[MV_Y];
            pred_buffer_offset = REF_PRED_EXTENTION_PEL_COUNT * stride * h - REF_PRED_EXTENTION_PEL_COUNT;

            xevdm_bl_mc_l(ref_pic->y, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_l, stride, preds_array + pred_buffer_offset, w + (REF_PRED_EXTENTION_PEL_COUNT << 1), REF_PRED_EXTENTION_PEL_COUNT, bit_depth_luma);

        }
        // go up
        if (mv_offsets[MV_Y] < 0
            || all_sides
            )
        {
            new_mv_t[MV_X] = mv[MV_X] - ref_pred_mv_scaled_step;
            new_mv_t[MV_Y] = mv[MV_Y] - ref_pred_mv_scaled_step;
            mv_clip_only_one_ref(x, y, pic_w, pic_h, w, h, new_mv_t, mv_t);
            qpel_gmv_x = (x << 2) + mv_t[MV_X];
            qpel_gmv_y = (y << 2) + mv_t[MV_Y];
            pred_buffer_offset = -REF_PRED_EXTENTION_PEL_COUNT * stride - REF_PRED_EXTENTION_PEL_COUNT;

            xevdm_bl_mc_l(ref_pic->y, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_l, stride, preds_array + pred_buffer_offset, w + (REF_PRED_EXTENTION_PEL_COUNT << 1), REF_PRED_EXTENTION_PEL_COUNT, bit_depth_luma);

        }
    }
}

s32 xevd_DMVR_cost(int w, int h, pel *src1, pel *src2, int s_src1, int s_src2)
{
    s32 sad = 0;
    s32 i, j;

    pel *src1_temp;
    pel *src2_temp;

    src1_temp = src1;
    src2_temp = src2;
    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            sad += abs(src1_temp[j] - src2_temp[j]);

        }
        src1_temp += s_src1;
        src2_temp += s_src2;
    }
    return sad;
}

void xevd_DMVR_refine(int w, int h, pel *ref_l0, int s_ref_l0, pel *ref_l1, int s_ref_l1,
    s32 *minCost, s16 *delta_mvX, s16 *delta_mvY, s32 *SAD_Array)
{
    enum SAD_POINT_INDEX idx;
    s32 LineMeanL0[4] = { 0, 0, 0, 0 };
    s32 LineMeanL1[4] = { 0, 0, 0, 0 };
    s32 meanL0 = 0, meanL1 = 0;
    s32 searchOffsetX[5] = { 0,  0, 1, -1, 0 };
    s32 searchOffsetY[5] = { 1, -1, 0,  0, 0 };
    pel *ref_l0_Orig = ref_l0;
    pel *ref_l1_Orig = ref_l1;
    for (idx = SAD_BOTTOM; idx <= SAD_TOP_LEFT; ++idx)
    {
        int sum = 0;
        ref_l0 = ref_l0_Orig + searchOffsetX[idx] + (searchOffsetY[idx] * s_ref_l0);
        ref_l1 = ref_l1_Orig - searchOffsetX[idx] - (searchOffsetY[idx] * s_ref_l1);

        s32 cost = xevd_DMVR_cost(w, h, ref_l0, ref_l1, s_ref_l0, s_ref_l1);
        *(SAD_Array + idx) = cost;
        if (idx == SAD_LEFT)
        {
            s32 down = -1, right = -1;
            if (*(SAD_Array + SAD_BOTTOM) <= *(SAD_Array + SAD_TOP))
            {
                down = 1;
            }
            if (*(SAD_Array + SAD_RIGHT) <= *(SAD_Array + SAD_LEFT))
            {
                right = 1;
            }
            searchOffsetX[SAD_TOP_LEFT] = right;
            searchOffsetY[SAD_TOP_LEFT] = down;
        }
        if (cost < *minCost)
        {
            *minCost = cost;

            *delta_mvX = searchOffsetX[idx];
            *delta_mvY = searchOffsetY[idx];
        }
    }/*end of search point loop*/
    ref_l0 = ref_l0_Orig;
    ref_l1 = ref_l1_Orig;
}

static __inline s32 div_for_maxq7(s64 N, s64 D)
{
    s32 sign, q;

    sign = 0;
    if (N < 0)
    {
        sign = 1;
        N = -N;
    }

    q = 0;
    D = (D << 3);
    if (N >= D)
    {
        N -= D;
        q++;
    }
    q = (q << 1);

    D = (D >> 1);
    if (N >= D)
    {
        N -= D;
        q++;
    }
    q = (q << 1);

    if (N >= (D >> 1))
        q++;

    if (sign)
        return (-q);
    return(q);
}
void xevd_SubPelErrorSrfc(
    int *sadBuffer,
    int *deltaMv
)
{
    s64 iNum, iDenom;
    int iMvDelta_SubPel;
    int MvSubPel_lvl = 4;/*1: half pel, 2: Qpel, 3:1/8, 4: 1/16*/
                                                          /*horizontal*/
    iNum = (s64)((sadBuffer[1] - sadBuffer[3]) << MvSubPel_lvl);
    iDenom = (s64)((sadBuffer[1] + sadBuffer[3] - (sadBuffer[0] << 1)));

    if (0 != iDenom)
    {
        if ((sadBuffer[1] != sadBuffer[0]) && (sadBuffer[3] != sadBuffer[0]))
        {
            iMvDelta_SubPel = div_for_maxq7(iNum, iDenom);
            deltaMv[0] = (iMvDelta_SubPel);
        }
        else
        {
            if (sadBuffer[1] == sadBuffer[0])
            {
                deltaMv[0] = -8;// half pel
            }
            else
            {
                deltaMv[0] = 8;// half pel
            }
        }
    }
    /*vertical*/
    iNum = (s64)((sadBuffer[2] - sadBuffer[4]) << MvSubPel_lvl);
    iDenom = (s64)((sadBuffer[2] + sadBuffer[4] - (sadBuffer[0] << 1)));
    if (0 != iDenom)
    {
        if ((sadBuffer[2] != sadBuffer[0]) && (sadBuffer[4] != sadBuffer[0]))
        {
            iMvDelta_SubPel = div_for_maxq7(iNum, iDenom);
            deltaMv[1] = (iMvDelta_SubPel);
        }
        else
        {
            if (sadBuffer[2] == sadBuffer[0])
            {
                deltaMv[1] = -8;// half pel
            }
            else
            {
                deltaMv[1] = 8;// half pel
            }
        }
    }
    return;
}


static void copy_buffer(pel *src, int src_stride, pel *dst, int dst_stride, int width, int height)
{
    int numBytes = width * sizeof(pel);
    for (int i = 0; i < height; i++)
    {
        xevd_mcpy(dst + i * dst_stride, src + i * src_stride, numBytes);
    }
}

static void padding(pel *ptr, int iStride, int iWidth, int iHeight, int PadLeftsize, int PadRightsize, int PadTopsize, int PadBottomSize)
{
    /*left padding*/
    pel *ptr_temp = ptr;
    int offset = 0;
    for (int i = 0; i < iHeight; i++)
    {
        offset = iStride * i;
        for (int j = 1; j <= PadLeftsize; j++)
        {
            *(ptr_temp - j + offset) = *(ptr_temp + offset);
        }
    }
    /*Right padding*/
    ptr_temp = ptr + (iWidth - 1);
    for (int i = 0; i < iHeight; i++)
    {
        offset = iStride * i;
        for (int j = 1; j <= PadRightsize; j++)
        {
            *(ptr_temp + j + offset) = *(ptr_temp + offset);
        }
    }
    /*Top padding*/
    int numBytes = (iWidth + PadLeftsize + PadRightsize) * sizeof(pel);
    ptr_temp = (ptr - PadLeftsize);
    for (int i = 1; i <= PadTopsize; i++)
    {
        xevd_mcpy(ptr_temp - (i * iStride), (ptr_temp), numBytes);
    }
    /*Bottom padding*/
    numBytes = (iWidth + PadLeftsize + PadRightsize) * sizeof(pel);
    ptr_temp = (ptr + (iStride * (iHeight - 1)) - PadLeftsize);
    for (int i = 1; i <= PadBottomSize; i++)
    {
        xevd_mcpy(ptr_temp + (i * iStride), (ptr_temp), numBytes);
    }
}

static void prefetch_for_mc(int x, int y,int pu_x, int pu_y, int pu_w, int pu_h,
    int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16(*mv)[MV_D], XEVD_REFP(*refp)[REFP_NUM]
                     , int iteration, pel dmvr_padding_buf[REFP_NUM][N_C][PAD_BUFFER_STRIDE * PAD_BUFFER_STRIDE]
                     , int chroma_format_idc)
{
    s16          mv_temp[REFP_NUM][MV_D];

    int l_w = pu_w, l_h = pu_h;

    int c_w = pu_w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc));
    int c_h = pu_h >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));

    int topleft_x_offset = pu_x - x;
    int topleft_y_offset = pu_y - y;

    int num_extra_pixel_left_for_filter;
    for (int i = 0; i < REFP_NUM; ++i)
    {
        int filtersize = NTAPS_LUMA;
        num_extra_pixel_left_for_filter = ((filtersize >> 1) - 1);

        int offset = (DMVR_ITER_COUNT + topleft_y_offset) * PAD_BUFFER_STRIDE + topleft_x_offset + DMVR_ITER_COUNT;
        int padsize = DMVR_PAD_LENGTH;
        int          qpel_gmv_x, qpel_gmv_y;
        XEVD_PIC    *ref_pic;
        mv_clip_only_one_ref_dmvr(x, y, pic_w, pic_h, w, h, mv[i], mv_temp[i]);

        qpel_gmv_x = ((pu_x << 2) + mv_temp[i][MV_X]) << 2;
        qpel_gmv_y = ((pu_y << 2) + mv_temp[i][MV_Y]) << 2;

        ref_pic = refp[refi[i]][i].pic;
        pel *ref = ref_pic->y + ((qpel_gmv_y >> 4) - num_extra_pixel_left_for_filter) * ref_pic->s_l +
            (qpel_gmv_x >> 4) - num_extra_pixel_left_for_filter;

        pel *dst = dmvr_padding_buf[i][0] + offset;
        copy_buffer(ref, ref_pic->s_l, dst, PAD_BUFFER_STRIDE, (l_w + filtersize), (l_h + filtersize));

        padding(dst, PAD_BUFFER_STRIDE, (l_w + filtersize - 1), (l_h + filtersize - 1), padsize,
            padsize, padsize, padsize);

        // chroma
        filtersize = NTAPS_CHROMA;
        num_extra_pixel_left_for_filter = ((filtersize >> 1) - 1);

        offset = (DMVR_ITER_COUNT + (topleft_y_offset >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)))) * PAD_BUFFER_STRIDE + (topleft_x_offset >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc))) + DMVR_ITER_COUNT;

        padsize = DMVR_PAD_LENGTH >> 1;


        if(chroma_format_idc)
        {
            ref = ref_pic->u + ((qpel_gmv_y >> 5) - 1) * ref_pic->s_c + (qpel_gmv_x >> 5) - 1;
            dst = dmvr_padding_buf[i][1] + offset;
            copy_buffer(ref, ref_pic->s_c, dst, PAD_BUFFER_STRIDE, (c_w + filtersize), (c_h + filtersize));
            padding(dst, PAD_BUFFER_STRIDE, (c_w + filtersize - 1), (c_h + filtersize - 1), padsize,
                padsize, padsize, padsize);

            ref = ref_pic->v + ((qpel_gmv_y >> 5) - 1) * ref_pic->s_c + (qpel_gmv_x >> 5) - 1;
            dst = dmvr_padding_buf[i][2] + offset;
            copy_buffer(ref, ref_pic->s_c, dst, PAD_BUFFER_STRIDE, (c_w + filtersize), (c_h + filtersize));
            padding(dst, PAD_BUFFER_STRIDE, (c_w + filtersize - 1), (c_h + filtersize - 1), padsize,
                padsize, padsize, padsize);
        }
    }
}



void final_paddedMC_forDMVR(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16(*inital_mv)[MV_D], s32(*refined_mv)[MV_D], XEVD_REFP(*refp)[REFP_NUM], pel pred[REFP_NUM][N_C][MAX_CU_DIM]
    , int sub_pred_offset_x
    , int sub_pred_offset_y
    , int cu_pred_stride
    , pel dmvr_padding_buf[REFP_NUM][N_C][PAD_BUFFER_STRIDE * PAD_BUFFER_STRIDE]
    , int bit_depth_luma, int bit_depth_chroma
                            , int chroma_format_idc)
{
    int i;
    XEVD_PIC    *ref_pic;
    s16          mv_temp[REFP_NUM][MV_D];
    for (i = 0; i < REFP_NUM; ++i)
    {
        int          qpel_gmv_x, qpel_gmv_y;

        ref_pic = refp[refi[i]][i].pic;

        s16 temp_uncliped_mv[MV_D] = { refined_mv[i][MV_X] >> 2, refined_mv[i][MV_Y] >> 2 };

        BOOL clip_flag = mv_clip_only_one_ref_dmvr(x, y, pic_w, pic_h, w, h, temp_uncliped_mv, mv_temp[i]);

        if (clip_flag)
        {
            qpel_gmv_x = (x << 4) + (mv_temp[i][MV_X] << 2);
            qpel_gmv_y = (y << 4) + (mv_temp[i][MV_Y] << 2);
        }
        else
        {
            qpel_gmv_x = (x << 4) + (refined_mv[i][MV_X]);
            qpel_gmv_y = (y << 4) + (refined_mv[i][MV_Y]);
        }

        int delta_x_l = 0;
        int delta_y_l = 0;
        int delta_x_c = 0;
        int delta_y_c = 0;
        int offset = 0;
        int filter_size = NTAPS_LUMA;
        int num_extra_pixel_left_for_filter = ((filter_size >> 1) - 1);

        if (clip_flag == 0)
        {
            // int pixel movement from inital mv
            delta_x_l = (refined_mv[i][MV_X] >> 4) - (inital_mv[i][MV_X] >> 2);
            delta_y_l = (refined_mv[i][MV_Y] >> 4) - (inital_mv[i][MV_Y] >> 2);

            delta_x_c = (refined_mv[i][MV_X] >> 5) - (inital_mv[i][MV_X] >> 3);
            delta_y_c = (refined_mv[i][MV_Y] >> 5) - (inital_mv[i][MV_Y] >> 3);
        }
        else
        {
            // int pixel movement from inital mv
            delta_x_l = (mv_temp[i][MV_X] >> 2) - (inital_mv[i][MV_X] >> 2);
            delta_y_l = (mv_temp[i][MV_Y] >> 2) - (inital_mv[i][MV_Y] >> 2);

            delta_x_c = (mv_temp[i][MV_X] >> 3) - (inital_mv[i][MV_X] >> 3);
            delta_y_c = (mv_temp[i][MV_Y] >> 3) - (inital_mv[i][MV_Y] >> 3);
        }
        offset = (DMVR_ITER_COUNT + num_extra_pixel_left_for_filter) * ((PAD_BUFFER_STRIDE + 1));
        offset += (delta_y_l)* PAD_BUFFER_STRIDE;
        offset += (delta_x_l);

        pel *src = dmvr_padding_buf[i][0] + offset + sub_pred_offset_x + sub_pred_offset_y * PAD_BUFFER_STRIDE;;
        pel *temp = pred[i][Y_C] + sub_pred_offset_x + sub_pred_offset_y * cu_pred_stride;

        xevdm_dmvr_mc_l(src, qpel_gmv_x, qpel_gmv_y, PAD_BUFFER_STRIDE, cu_pred_stride, temp, w, h, bit_depth_luma);

        filter_size = NTAPS_CHROMA;
        num_extra_pixel_left_for_filter = ((filter_size >> 1) - 1);
        offset = (DMVR_ITER_COUNT + num_extra_pixel_left_for_filter) * ((PAD_BUFFER_STRIDE + 1));
        offset += (delta_y_c)* PAD_BUFFER_STRIDE;
        offset += (delta_x_c);

        if(chroma_format_idc)
        {
            src = dmvr_padding_buf[i][1] + offset + (sub_pred_offset_x >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))) + (sub_pred_offset_y >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc))) * PAD_BUFFER_STRIDE;
            temp = pred[i][U_C] + (sub_pred_offset_x >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))) + (sub_pred_offset_y >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc))) * (cu_pred_stride >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)));

            xevdm_dmvr_mc_c(src, qpel_gmv_x, qpel_gmv_y, PAD_BUFFER_STRIDE, cu_pred_stride >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), temp, w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))
                          , h >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)), bit_depth_chroma);

            src = dmvr_padding_buf[i][2] + offset + (sub_pred_offset_x >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))) + (sub_pred_offset_y >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc))) * PAD_BUFFER_STRIDE;
            temp = pred[i][V_C] + (sub_pred_offset_x >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))) + (sub_pred_offset_y >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc))) * (cu_pred_stride >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)));

            xevdm_dmvr_mc_c(src, qpel_gmv_x, qpel_gmv_y, PAD_BUFFER_STRIDE, cu_pred_stride >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), temp, w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))
                          , h >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)), bit_depth_chroma);
        }

    }
}


static void processDMVR(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16(*mv)[MV_D], XEVD_REFP(*refp)[REFP_NUM], pel pred[REFP_NUM][N_C][MAX_CU_DIM], \
    int poc_c, pel *dmvr_current_template, pel dmvr_ref_pred_interpolated[REFP_NUM][(MAX_CU_SIZE + ((DMVR_NEW_VERSION_ITER_COUNT + 1) * REF_PRED_EXTENTION_PEL_COUNT)) * (MAX_CU_SIZE + ((DMVR_NEW_VERSION_ITER_COUNT + 1) * REF_PRED_EXTENTION_PEL_COUNT))]
    , pel dmvr_half_pred_interpolated[REFP_NUM][(MAX_CU_SIZE + 1) * (MAX_CU_SIZE + 1)], int iteration, pel dmvr_padding_buf[REFP_NUM][N_C][PAD_BUFFER_STRIDE * PAD_BUFFER_STRIDE]
    , s16 dmvr_mv[MAX_CU_CNT_IN_LCU][REFP_NUM][MV_D], int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc)
{

    s32 sub_pu_L0[(MAX_CU_SIZE * MAX_CU_SIZE) >> (MIN_CU_LOG2 << 1)][MV_D];
    s32 sub_pu_L1[(MAX_CU_SIZE * MAX_CU_SIZE) >> (MIN_CU_LOG2 << 1)][MV_D];


    int stride = w + (iteration << 1);
    s16 ref_pred_mv_scaled_step = 2;

    s16 tempMv[MV_D];

    s32 refined_mv[REFP_NUM][MV_D];

    s16 starting_mv[REFP_NUM][MV_D];
    mv_clip(x, y, pic_w, pic_h, w, h, refi, mv, starting_mv);

    int          qpel_gmv_x, qpel_gmv_y;
    XEVD_PIC    *ref_pic;

    // centre address holder for pred
    pel          *preds_array[REFP_NUM];
    preds_array[REFP_0] = dmvr_ref_pred_interpolated[REFP_0];
    preds_array[REFP_1] = dmvr_ref_pred_interpolated[REFP_1];

    // REF_PIC_LIST_0
    ref_pic = refp[refi[REFP_0]][REFP_0].pic;
    //produce iteration lines extra
    tempMv[MV_X] = starting_mv[REFP_0][MV_X] - (iteration << ref_pred_mv_scaled_step);
    tempMv[MV_Y] = starting_mv[REFP_0][MV_Y] - (iteration << ref_pred_mv_scaled_step);
    qpel_gmv_x = (x << 2) + tempMv[MV_X];
    qpel_gmv_y = (y << 2) + tempMv[MV_Y];

    xevdm_bl_mc_l(ref_pic->y, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_l, stride, preds_array[REFP_0], (w + iteration * 2), (h + iteration * 2), bit_depth_luma);


    // REF_PIC_LIST_1
    ref_pic = refp[refi[REFP_1]][REFP_1].pic;
    //produce iteration lines extra
    tempMv[MV_X] = starting_mv[REFP_1][MV_X] - (iteration << ref_pred_mv_scaled_step);
    tempMv[MV_Y] = starting_mv[REFP_1][MV_Y] - (iteration << ref_pred_mv_scaled_step);
    qpel_gmv_x = (x << 2) + tempMv[MV_X];
    qpel_gmv_y = (y << 2) + tempMv[MV_Y];

    xevdm_bl_mc_l(ref_pic->y, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_l, stride, preds_array[REFP_1], (w + iteration * 2), (h + iteration * 2),bit_depth_luma);


    // go to the center point
    pel *preds_centre_array[REFP_NUM];
    preds_centre_array[REFP_0] = preds_array[REFP_0] + (stride * iteration + iteration);
    preds_centre_array[REFP_1] = preds_array[REFP_1] + (stride * iteration + iteration);

    int minCost = INT_MAX;

    int lastDirection = -1;

    int arrayCost[SAD_COUNT];

    int dx, dy;

    dy = XEVD_MIN(h, DMVR_SUBCU_SIZE);
    dx = XEVD_MIN(w, DMVR_SUBCU_SIZE);


    int num = 0;
    int subPuStartX, subPuStartY, startX, startY;
    for (startY = 0, subPuStartY = y; subPuStartY < (y + h); subPuStartY = subPuStartY + dy, startY += dy)
    {
        for (startX = 0, subPuStartX = x; subPuStartX < (x + w); subPuStartX = subPuStartX + dx, startX += dx)
        {
            s16 total_delta_mv[MV_D] = { 0, 0 };
            BOOL notZeroCost = 1;

            pel *addr_subpu_l0 = preds_centre_array[REFP_0] + startX + startY * stride;
            pel *addr_subpu_l1 = preds_centre_array[REFP_1] + startX + startY * stride;

            for (int i = 0; i < iteration; i++)
            {
                s16 delta_mv[MV_D] = { 0, 0 };
                pel *addr_l0 = addr_subpu_l0 + (total_delta_mv[MV_X] + total_delta_mv[MV_Y] * stride);
                pel *addr_l1 = addr_subpu_l1 - (total_delta_mv[MV_X] + total_delta_mv[MV_Y] * stride);

                for (int loop = 0; loop < SAD_COUNT; loop++)
                {
                    arrayCost[loop] = INT_MAX;
                }

                if (i == 0)
                {
                    minCost = xevd_DMVR_cost(dx, dy, addr_l0, addr_l1, stride, stride);
                }

                if ((i>0 && minCost == 0) || (i==0 && minCost < dy*dx))
                {
                    notZeroCost = 0;
                    break;
                }
                arrayCost[SAD_CENTER] = minCost;
                xevd_DMVR_refine(dx, dy, addr_l0, stride, addr_l1, stride
                    , &minCost
                    , &delta_mv[MV_X], &delta_mv[MV_Y]
                    , arrayCost);

                if (delta_mv[MV_X] == 0 && delta_mv[MV_Y] == 0)
                {
                    break;
                }
                total_delta_mv[MV_X] += delta_mv[MV_X];
                total_delta_mv[MV_Y] += delta_mv[MV_Y];
            }

            total_delta_mv[MV_X] = (total_delta_mv[MV_X] << 4);
            total_delta_mv[MV_Y] = (total_delta_mv[MV_Y] << 4);

            if (notZeroCost && (minCost == arrayCost[SAD_CENTER]))
            {
                int sadbuffer[5];
                int deltaMv[MV_D] = { 0, 0 };
                sadbuffer[0] = arrayCost[SAD_CENTER];
                sadbuffer[1] = arrayCost[SAD_LEFT];
                sadbuffer[2] = arrayCost[SAD_TOP];
                sadbuffer[3] = arrayCost[SAD_RIGHT];
                sadbuffer[4] = arrayCost[SAD_BOTTOM];
                xevd_SubPelErrorSrfc(sadbuffer, deltaMv);

                total_delta_mv[MV_X] += deltaMv[MV_X];
                total_delta_mv[MV_Y] += deltaMv[MV_Y];
            }

            refined_mv[REFP_0][MV_X] = (starting_mv[REFP_0][MV_X] << 2) + (total_delta_mv[MV_X]);
            refined_mv[REFP_0][MV_Y] = (starting_mv[REFP_0][MV_Y] << 2) + (total_delta_mv[MV_Y]);

            refined_mv[REFP_1][MV_X] = (starting_mv[REFP_1][MV_X] << 2) - (total_delta_mv[MV_X]);
            refined_mv[REFP_1][MV_Y] = (starting_mv[REFP_1][MV_Y] << 2) - (total_delta_mv[MV_Y]);

            sub_pu_L0[num][MV_X] = refined_mv[REFP_0][MV_X];
            sub_pu_L0[num][MV_Y] = refined_mv[REFP_0][MV_Y];

            sub_pu_L1[num][MV_X] = refined_mv[REFP_1][MV_X];
            sub_pu_L1[num][MV_Y] = refined_mv[REFP_1][MV_Y];
            num++;

            u32 idx = (startX >> MIN_CU_LOG2) + ((startY >> MIN_CU_LOG2) * (w >> MIN_CU_LOG2));
            int i, j;
            for (j = 0; j < dy >> MIN_CU_LOG2; j++)
            {
                for (i = 0; i < dx >> MIN_CU_LOG2; i++)
                {
                    dmvr_mv[idx + i][REFP_0][MV_X] = refined_mv[REFP_0][MV_X] >> 2;
                    dmvr_mv[idx + i][REFP_0][MV_Y] = refined_mv[REFP_0][MV_Y] >> 2;

                    dmvr_mv[idx + i][REFP_1][MV_X] = refined_mv[REFP_1][MV_X] >> 2;
                    dmvr_mv[idx + i][REFP_1][MV_Y] = refined_mv[REFP_1][MV_Y] >> 2;
                }
                idx += w >> MIN_CU_LOG2;
            }
        }
    }

    // produce padded buffer for exact MC
    num = 0;
    for (int startY = 0, subPuStartY = y; subPuStartY < (y + h); subPuStartY = subPuStartY + dy, startY += dy)
    {
        for (int startX = 0, subPuStartX = x; subPuStartX < (x + w); subPuStartX = subPuStartX + dx, startX += dx)
        {
            prefetch_for_mc(x, y, subPuStartX, subPuStartY, dx, dy, pic_w, pic_h, w, h, refi, starting_mv, refp, iteration, dmvr_padding_buf
                            , chroma_format_idc);

            s32 dmvr_mv[REFP_NUM][MV_D] = { { sub_pu_L0[num][MV_X], sub_pu_L0[num][MV_Y] },
                                            { sub_pu_L1[num][MV_X], sub_pu_L1[num][MV_Y] }
            };

            final_paddedMC_forDMVR(subPuStartX, subPuStartY, pic_w, pic_h, dx, dy, refi, starting_mv, dmvr_mv, refp, pred
                , startX
                , startY
                , w
                , dmvr_padding_buf
                ,  bit_depth_luma,  bit_depth_chroma
                                   , chroma_format_idc

            );
            num++;
        }
    }
}
void mv_clip(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16 mv[REFP_NUM][MV_D], s16(*mv_t)[MV_D])
{
    int min_clip[MV_D], max_clip[MV_D];

    x <<= 2;
    y <<= 2;
    w <<= 2;
    h <<= 2;
    min_clip[MV_X] = -(MAX_CU_SIZE << 2);
    min_clip[MV_Y] = -(MAX_CU_SIZE << 2);
    max_clip[MV_X] = (pic_w - 1 + MAX_CU_SIZE) << 2;
    max_clip[MV_Y] = (pic_h - 1 + MAX_CU_SIZE) << 2;

    mv_t[REFP_0][MV_X] = mv[REFP_0][MV_X];
    mv_t[REFP_0][MV_Y] = mv[REFP_0][MV_Y];
    mv_t[REFP_1][MV_X] = mv[REFP_1][MV_X];
    mv_t[REFP_1][MV_Y] = mv[REFP_1][MV_Y];

    if (REFI_IS_VALID(refi[REFP_0]))
    {
        if (x + mv[REFP_0][MV_X] < min_clip[MV_X]) mv_t[REFP_0][MV_X] = min_clip[MV_X] - x;
        if (y + mv[REFP_0][MV_Y] < min_clip[MV_Y]) mv_t[REFP_0][MV_Y] = min_clip[MV_Y] - y;
        if (x + mv[REFP_0][MV_X] + w - 4 > max_clip[MV_X]) mv_t[REFP_0][MV_X] = max_clip[MV_X] - x - w + 4;
        if (y + mv[REFP_0][MV_Y] + h - 4 > max_clip[MV_Y]) mv_t[REFP_0][MV_Y] = max_clip[MV_Y] - y - h + 4;
    }
    if (REFI_IS_VALID(refi[REFP_1]))
    {
        if (x + mv[REFP_1][MV_X] < min_clip[MV_X]) mv_t[REFP_1][MV_X] = min_clip[MV_X] - x;
        if (y + mv[REFP_1][MV_Y] < min_clip[MV_Y]) mv_t[REFP_1][MV_Y] = min_clip[MV_Y] - y;
        if (x + mv[REFP_1][MV_X] + w - 4 > max_clip[MV_X]) mv_t[REFP_1][MV_X] = max_clip[MV_X] - x - w + 4;
        if (y + mv[REFP_1][MV_Y] + h - 4 > max_clip[MV_Y]) mv_t[REFP_1][MV_Y] = max_clip[MV_Y] - y - h + 4;
    }
}

void xevdm_mc(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16(*mv)[MV_D], XEVD_REFP(*refp)[REFP_NUM], pel pred[REFP_NUM][N_C][MAX_CU_DIM]
    , int poc_c, pel *dmvr_current_template, pel dmvr_ref_pred_interpolated[REFP_NUM][(MAX_CU_SIZE + ((DMVR_NEW_VERSION_ITER_COUNT + 1) * REF_PRED_EXTENTION_PEL_COUNT)) * (MAX_CU_SIZE + ((DMVR_NEW_VERSION_ITER_COUNT + 1) * REF_PRED_EXTENTION_PEL_COUNT))]
    , pel dmvr_half_pred_interpolated[REFP_NUM][(MAX_CU_SIZE + 1) * (MAX_CU_SIZE + 1)], BOOL apply_DMVR, pel dmvr_padding_buf[REFP_NUM][N_C][PAD_BUFFER_STRIDE * PAD_BUFFER_STRIDE], u8 *cu_dmvr_flag, s16 dmvr_mv[MAX_CU_CNT_IN_LCU][REFP_NUM][MV_D]
    , int sps_admvp_flag, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc)
{
    XEVD_PIC    *ref_pic;
#if !OPT_SIMD_MC_L
    pel         *p2, *p3;
#endif
    int          qpel_gmv_x, qpel_gmv_y;
    int          bidx = 0;
    s16          mv_t[REFP_NUM][MV_D];
    s16          mv_before_clipping[REFP_NUM][MV_D]; //store it to pass it to interpolation function for deriving correct interpolation filter

    mv_before_clipping[REFP_0][MV_X] = mv[REFP_0][MV_X];
    mv_before_clipping[REFP_0][MV_Y] = mv[REFP_0][MV_Y];
    mv_before_clipping[REFP_1][MV_X] = mv[REFP_1][MV_X];
    mv_before_clipping[REFP_1][MV_Y] = mv[REFP_1][MV_Y];

    mv_clip(x, y, pic_w, pic_h, w, h, refi, mv, mv_t);

    s16          mv_refine[REFP_NUM][MV_D] = { {mv[REFP_0][MV_X], mv[REFP_0][MV_Y]},
                                              {mv[REFP_1][MV_X], mv[REFP_1][MV_Y]} };

    s16          inital_mv[REFP_NUM][MV_D] = { { mv[REFP_0][MV_X], mv[REFP_0][MV_Y] },
                                               { mv[REFP_1][MV_X], mv[REFP_1][MV_Y] } };

    s32          extend_width = (DMVR_NEW_VERSION_ITER_COUNT + 1) * REF_PRED_EXTENTION_PEL_COUNT;
    s32          extend_width_minus1 = DMVR_NEW_VERSION_ITER_COUNT * REF_PRED_EXTENTION_PEL_COUNT;
    int          stride = w + (extend_width << 1);
    s16          mv_offsets[REFP_NUM][MV_D] = { {0,}, };
    s32          center_point_avgs_l0_l1[2 * REFP_NUM] = { 0, 0, 0, 0 }; // center_point_avgs_l0_l1[2,3] for "A" and "B" current center point average
    int iterations_count = DMVR_ITER_COUNT;

    BOOL         dmvr_poc_condition;
    if (!REFI_IS_VALID(refi[REFP_0]) || !REFI_IS_VALID(refi[REFP_1]))
    {
        apply_DMVR = 0;
        dmvr_poc_condition = 0;
    }
    else
    {
        int          poc0 = refp[refi[REFP_0]][REFP_0].poc;
        int          poc1 = refp[refi[REFP_1]][REFP_1].poc;
        dmvr_poc_condition = ((BOOL)((poc_c - poc0)*(poc_c - poc1) < 0)) && (abs(poc_c - poc0) == abs(poc_c - poc1));

        apply_DMVR = apply_DMVR && dmvr_poc_condition;
        apply_DMVR = apply_DMVR && (REFI_IS_VALID(refi[REFP_0]) && REFI_IS_VALID(refi[REFP_1]));
        apply_DMVR = apply_DMVR && !(refp[refi[REFP_0]][REFP_0].pic->poc == refp[refi[REFP_1]][REFP_1].pic->poc &&  mv_t[REFP_0][MV_X] == mv_t[REFP_1][MV_X] && mv_t[REFP_0][MV_Y] == mv_t[REFP_1][MV_Y]);
        apply_DMVR = apply_DMVR && w >= 8 && h >= 8;

    }


    *cu_dmvr_flag = 0;
     if(sps_admvp_flag == 1)
    {
        tbl_mc_l_coeff = tbl_mc_l_coeff_main;
        tbl_mc_c_coeff = tbl_mc_c_coeff_main;
    }
    else
    {
        tbl_mc_l_coeff = xevd_tbl_mc_l_coeff;
        tbl_mc_c_coeff = xevd_tbl_mc_c_coeff;
    }

    if (REFI_IS_VALID(refi[REFP_0]))
    {
        /* forward */
        ref_pic = refp[refi[REFP_0]][REFP_0].pic;
        qpel_gmv_x = (x << 2) + mv_t[REFP_0][MV_X];
        qpel_gmv_y = (y << 2) + mv_t[REFP_0][MV_Y];


        if (!apply_DMVR)
        {
            xevd_mc_l(mv_before_clipping[REFP_0][MV_X] << 2, mv_before_clipping[REFP_0][MV_Y] << 2, ref_pic->y, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_l, w, pred[0][Y_C], w, h, bit_depth_luma);
        }

        if ((!REFI_IS_VALID(refi[REFP_1]) || !apply_DMVR || !dmvr_poc_condition)

            && chroma_format_idc!=0

            )
        {
            xevd_mc_c(mv_before_clipping[REFP_0][MV_X] << 2, mv_before_clipping[REFP_0][MV_Y] << 2, ref_pic->u, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_c, w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))
                     , pred[0][U_C], w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), h >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)), bit_depth_chroma);
            xevd_mc_c(mv_before_clipping[REFP_0][MV_X] << 2, mv_before_clipping[REFP_0][MV_Y] << 2, ref_pic->v, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_c, w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))
                     , pred[0][V_C], w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), h >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)), bit_depth_chroma);
        }

        bidx++;
    }

    /* check identical motion */
    if (REFI_IS_VALID(refi[REFP_0]) && REFI_IS_VALID(refi[REFP_1]))
    {
        if (refp[refi[REFP_0]][REFP_0].pic->poc == refp[refi[REFP_1]][REFP_1].pic->poc &&  mv_t[REFP_0][MV_X] == mv_t[REFP_1][MV_X] && mv_t[REFP_0][MV_Y] == mv_t[REFP_1][MV_Y])
        {
            return;
        }
    }

    if (REFI_IS_VALID(refi[REFP_1]))
    {
        /* backward */
        ref_pic = refp[refi[REFP_1]][REFP_1].pic;
        qpel_gmv_x = (x << 2) + mv_t[REFP_1][MV_X];
        qpel_gmv_y = (y << 2) + mv_t[REFP_1][MV_Y];

        if (!apply_DMVR)
        {

            xevd_mc_l(mv_before_clipping[REFP_1][MV_X] << 2, mv_before_clipping[REFP_1][MV_Y] << 2, ref_pic->y, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_l, w, pred[bidx][Y_C], w, h, bit_depth_luma);

        }

        if ((!REFI_IS_VALID(refi[REFP_0]) || !apply_DMVR || !dmvr_poc_condition)

            && chroma_format_idc!=0

            )
        {

            xevd_mc_c(mv_before_clipping[REFP_1][MV_X] << 2, mv_before_clipping[REFP_1][MV_Y] << 2, ref_pic->u, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_c, w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))
                     , pred[bidx][U_C], w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), h >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)), bit_depth_chroma);
            xevd_mc_c(mv_before_clipping[REFP_1][MV_X] << 2, mv_before_clipping[REFP_1][MV_Y] << 2, ref_pic->v, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_c, w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))
                     , pred[bidx][V_C], w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), h >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)), bit_depth_chroma);

        }

        bidx++;
    }

    if (bidx == 2)
    {
        BOOL template_needs_update = FALSE;
        s32 center_cost[2] = { 1 << 30, 1 << 30 };

        //only if the references are located on opposite sides of the current frame
        if (apply_DMVR && dmvr_poc_condition)
        {
            if (apply_DMVR)
            {
                *cu_dmvr_flag = 1;
                processDMVR(x, y, pic_w, pic_h, w, h, refi, mv, refp, pred, poc_c, dmvr_current_template, dmvr_ref_pred_interpolated
                    , dmvr_half_pred_interpolated, iterations_count, dmvr_padding_buf, dmvr_mv,  bit_depth_luma,  bit_depth_chroma, chroma_format_idc);
            }

            mv[REFP_0][MV_X] = inital_mv[REFP_0][MV_X];
            mv[REFP_0][MV_Y] = inital_mv[REFP_0][MV_Y];

            mv[REFP_1][MV_X] = inital_mv[REFP_1][MV_X];
            mv[REFP_1][MV_Y] = inital_mv[REFP_1][MV_Y];
        } //if (apply_DMVR && ((poc_c - poc0)*(poc_c - poc1) < 0))


        xevd_func_average_no_clip(pred[0][Y_C], pred[1][Y_C], pred[0][Y_C], w, w, w, w, h

            , bit_depth_luma

        );
        w >>= (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc));
        h >>= (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));


        if(chroma_format_idc)
        {
            xevd_func_average_no_clip(pred[0][U_C], pred[1][U_C], pred[0][U_C], w, w, w, w, h
            , bit_depth_chroma
        );
            xevd_func_average_no_clip(pred[0][V_C], pred[1][V_C], pred[0][V_C], w, w, w, w, h
            , bit_depth_chroma

        );
        }

    }
}

void xevdm_IBC_mc(int x, int y, int log2_cuw, int log2_cuh, s16 mv[MV_D], XEVD_PIC *ref_pic, pel (*pred)[MAX_CU_DIM], TREE_CONS tree_cons, int chroma_format_idc)
{

    int i = 0, j = 0;
    int size = 0;

    int cuw = 0, cuh = 0;
    int stride = 0;
    int mv_x = 0, mv_y = 0;

    pel *dst = NULL;
    pel *ref = NULL;

    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;
    mv_x = mv[0];
    mv_y = mv[1];
    stride = ref_pic->s_l;

    if (xevd_check_luma_fn(tree_cons))
    {
        dst = pred[0];
        ref = ref_pic->y + (mv_y + y) * stride + (mv_x + x);
        size = sizeof(pel) * cuw;

        for (i = 0; i < cuh; i++)
        {
            xevd_mcpy(dst, ref, size);
            ref += stride;
            dst += cuw;
        }
    }

    if (xevd_check_chroma_fn(tree_cons) && (chroma_format_idc != 0))
    {
        cuw >>=(XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc));
        cuh >>= (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));
        x >>= (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc));
        y >>= (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));
        mv_x >>= (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc));
        mv_y >>= (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));
        log2_cuw -= (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc));
        log2_cuh -= (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));

        stride = ref_pic->s_c;

        dst = pred[1];
        ref = ref_pic->u + (mv_y + y) * stride + (mv_x + x);
        size = sizeof(pel) * cuw;
        for (i = 0; i < cuh; i++)
        {
            xevd_mcpy(dst, ref, size);
            ref += stride;
            dst += cuw;
        }

        dst = pred[2];
        ref = ref_pic->v + (mv_y + y) * stride + (mv_x + x);
        size = sizeof(pel) * cuw;
        for (i = 0; i < cuh; i++)
        {
            xevd_mcpy(dst, ref, size);
            ref += stride;
            dst += cuw;
        }
    }
}

static void eif_derive_mv_clip_range(int x, int y, int cuw, int cuh, int dmv_hor[MV_D], int dmv_ver[MV_D], int mv_scale[MV_D],
    int pic_w, int pic_h, BOOL range_clip, int max_mv[MV_D], int min_mv[MV_D])
{
    int max_mv_pic[MV_D] = { (pic_w + MAX_CU_SIZE - x - cuw - 1) << 5, (pic_h + MAX_CU_SIZE - y - cuh - 1) << 5 };             //1 for bilinear interpolation
    int min_mv_pic[MV_D] = { (-x - MAX_CU_SIZE) << 5, (-y - MAX_CU_SIZE) << 5 };

    s32 mv_center[MV_D] = { 0, 0 };
    int pos_center[MV_D] = { cuw >> 1, cuh >> 1 };

    for (int comp = MV_X; comp < MV_D; ++comp)
    {
        if (!range_clip)
        {
            max_mv[comp] = max_mv_pic[comp];
            min_mv[comp] = min_mv_pic[comp];
        }
        else
        {
            mv_center[comp] = mv_scale[comp] + dmv_hor[comp] * pos_center[MV_X] + dmv_ver[comp] * pos_center[MV_Y];

            xevdm_rounding_s32(mv_center[comp], mv_center + comp, 4, 0);

            int mv_spread = comp == MV_X ? g_aff_mvDevBB2_125[xevd_tbl_log2[cuw] - 3] : g_aff_mvDevBB2_125[xevd_tbl_log2[cuh] - 3];

            min_mv[comp] = mv_center[comp] - mv_spread;
            max_mv[comp] = mv_center[comp] + mv_spread;

            if (min_mv[comp] < min_mv_pic[comp])
            {
                min_mv[comp] = min_mv_pic[comp];
                max_mv[comp] = XEVD_MIN(max_mv_pic[comp], min_mv_pic[comp] + 2 * mv_spread);
            }
            else if (max_mv[comp] > max_mv_pic[comp])
            {
                max_mv[comp] = max_mv_pic[comp];
                min_mv[comp] = XEVD_MAX(min_mv_pic[comp], max_mv_pic[comp] - 2 * mv_spread);
            }
        }

        max_mv[comp] = XEVD_CLIP3(-(1 << 17), (1 << 17) - 1, max_mv[comp]);
        min_mv[comp] = XEVD_CLIP3(-(1 << 17), (1 << 17) - 1, min_mv[comp]);
    }
}

void xevdm_affine_mc_l(int x, int y, int pic_w, int pic_h, int cuw, int cuh, s16 ac_mv[VER_NUM][MV_D], XEVD_PIC* ref_pic, pel pred[MAX_CU_DIM], int vertex_num, pel* tmp_buffer

    , int bit_depth_luma, int bit_depth_chroma
                     , int chroma_format_idc

)
{
    int qpel_gmv_x, qpel_gmv_y;
    pel *pred_y = pred;
    int sub_w, sub_h;
    int w, h;
    int half_w, half_h;
    int bit = MAX_CU_LOG2;

#if MC_PRECISION_ADD
    int mc_prec = 2 + MC_PRECISION_ADD;
    int shift = bit - MC_PRECISION_ADD;
#else
    int mc_prec = 4;
    int shift = bit;
#endif
    int dmv_hor_x, dmv_ver_x, dmv_hor_y, dmv_ver_y;
    int mv_scale_hor = ac_mv[0][MV_X] << bit;
    int mv_scale_ver = ac_mv[0][MV_Y] << bit;
    int mv_scale_tmp_hor, mv_scale_tmp_ver;
    int hor_max, hor_min, ver_max, ver_min;

    // get clip MV Range
    hor_max = (pic_w + MAX_CU_SIZE - x - cuw) << mc_prec;
    ver_max = (pic_h + MAX_CU_SIZE - y - cuh) << mc_prec;
    hor_min = (-MAX_CU_SIZE - x) << mc_prec;
    ver_min = (-MAX_CU_SIZE - y) << mc_prec;

    // get sub block size
    BOOL mem_band_conditions_for_eif_are_satisfied = FALSE;

    xevdm_derive_affine_subblock_size(ac_mv, cuw, cuh, &sub_w, &sub_h, vertex_num, &mem_band_conditions_for_eif_are_satisfied);

    half_w = sub_w >> 1;
    half_h = sub_h >> 1;

    // convert to 2^(storeBit + bit) precision
    dmv_hor_x = ((ac_mv[1][MV_X] - ac_mv[0][MV_X]) << bit) >> xevd_tbl_log2[cuw];     // deltaMvHor
    dmv_hor_y = ((ac_mv[1][MV_Y] - ac_mv[0][MV_Y]) << bit) >> xevd_tbl_log2[cuw];
    if (vertex_num == 3)
    {
        dmv_ver_x = ((ac_mv[2][MV_X] - ac_mv[0][MV_X]) << bit) >> xevd_tbl_log2[cuh]; // deltaMvVer
        dmv_ver_y = ((ac_mv[2][MV_Y] - ac_mv[0][MV_Y]) << bit) >> xevd_tbl_log2[cuh];
    }
    else
    {
        dmv_ver_x = -dmv_hor_y;                                                       // deltaMvVer
        dmv_ver_y = dmv_hor_x;
    }

    int b_eif = sub_w < AFFINE_ADAPT_EIF_SIZE || sub_h < AFFINE_ADAPT_EIF_SIZE;
    int d_hor[MV_D] = { dmv_hor_x, dmv_hor_y }, d_ver[MV_D] = { dmv_ver_x, dmv_ver_y };
    int mv_precision = MAX_CU_LOG2 + MC_PRECISION_ADD;
    BOOL clipMV = FALSE;

    if (b_eif)
    {
        int mv_scale[MV_D] = { mv_scale_hor, mv_scale_ver };
        int max_mv[MV_D] = { 0, 0 };
        int min_mv[MV_D] = { 0, 0 };

        eif_derive_mv_clip_range(x, y, cuw, cuh, d_hor, d_ver, mv_scale, pic_w, pic_h, !mem_band_conditions_for_eif_are_satisfied, max_mv, min_mv);

        xevdm_eif_mc(cuw, cuh, x, y, mv_scale_hor, mv_scale_ver, dmv_hor_x, dmv_hor_y, dmv_ver_x, dmv_ver_y,
            max_mv[MV_X], max_mv[MV_Y], min_mv[MV_X], min_mv[MV_Y],
            ref_pic->y, ref_pic->s_l, pred, cuw, tmp_buffer, bit + 2, Y_C

            , bit_depth_luma
                   , chroma_format_idc

        );

        return;
    }

    int mv_scale_tmp_hor_ori, mv_scale_tmp_ver_ori;
    // get prediction block by block
    for (h = 0; h < cuh; h += sub_h)
    {
        for (w = 0; w < cuw; w += sub_w)
        {
            mv_scale_tmp_hor = (mv_scale_hor + dmv_hor_x * half_w + dmv_ver_x * half_h);
            mv_scale_tmp_ver = (mv_scale_ver + dmv_hor_y * half_w + dmv_ver_y * half_h);
            xevdm_mv_rounding_s32(mv_scale_tmp_hor, mv_scale_tmp_ver, &mv_scale_tmp_hor, &mv_scale_tmp_ver, shift, 0);
            mv_scale_tmp_hor = XEVD_CLIP3(-(1 << 17), (1 << 17) - 1, mv_scale_tmp_hor);
            mv_scale_tmp_ver = XEVD_CLIP3(-(1 << 17), (1 << 17) - 1, mv_scale_tmp_ver);
            mv_scale_tmp_ver_ori = mv_scale_tmp_ver;
            mv_scale_tmp_hor_ori = mv_scale_tmp_hor;
            // clip
            mv_scale_tmp_hor = XEVD_MIN(hor_max, XEVD_MAX(hor_min, mv_scale_tmp_hor));
            mv_scale_tmp_ver = XEVD_MIN(ver_max, XEVD_MAX(ver_min, mv_scale_tmp_ver));

            qpel_gmv_x = ((x + w) << mc_prec) + mv_scale_tmp_hor;
            qpel_gmv_y = ((y + h) << mc_prec) + mv_scale_tmp_ver;

            xevd_mc_l(mv_scale_tmp_hor_ori, mv_scale_tmp_ver_ori, ref_pic->y, qpel_gmv_x, qpel_gmv_y, ref_pic->s_l, cuw, (pred_y + w), sub_w, sub_h, bit_depth_luma);

        }
        pred_y += (cuw * sub_h);
    }
}

void xevdm_affine_mc_lc(int x, int y, int pic_w, int pic_h, int cuw, int cuh, s16 ac_mv[VER_NUM][MV_D], XEVD_PIC* ref_pic, pel pred[N_C][MAX_CU_DIM], int vertex_num
    , int sub_w, int sub_h, pel* tmp_buffer_for_eif, BOOL mem_band_conditions_for_eif_are_satisfied

    , int bit_depth_luma, int bit_depth_chroma
                      , int chroma_format_idc

)
{
    int qpel_gmv_x, qpel_gmv_y;
    pel *pred_y = pred[Y_C], *pred_u = pred[U_C], *pred_v = pred[V_C];
    int w, h;
    int half_w, half_h;
    int bit = MAX_CU_LOG2;

#if MC_PRECISION_ADD
    int mc_prec = 2 + MC_PRECISION_ADD;
    int shift = bit - MC_PRECISION_ADD;
#else
    int mc_prec = 4;
    int shift = bit;
#endif
    int dmv_hor_x, dmv_ver_x, dmv_hor_y, dmv_ver_y;
    int mv_scale_hor = ac_mv[0][MV_X] << bit;
    int mv_scale_ver = ac_mv[0][MV_Y] << bit;
    int mv_scale_tmp_hor, mv_scale_tmp_ver;
    int hor_max, hor_min, ver_max, ver_min;

    // get clip MV Range
    hor_max = (pic_w + MAX_CU_SIZE - x - cuw) << mc_prec;
    ver_max = (pic_h + MAX_CU_SIZE - y - cuh) << mc_prec;
    hor_min = (-MAX_CU_SIZE - x) << mc_prec;
    ver_min = (-MAX_CU_SIZE - y) << mc_prec;

    // get sub block size
    half_w = sub_w >> 1;
    half_h = sub_h >> 1;

    // convert to 2^(storeBit + bit) precision
    dmv_hor_x = ((ac_mv[1][MV_X] - ac_mv[0][MV_X]) << bit) >> xevd_tbl_log2[cuw];     // deltaMvHor
    dmv_hor_y = ((ac_mv[1][MV_Y] - ac_mv[0][MV_Y]) << bit) >> xevd_tbl_log2[cuw];
    if (vertex_num == 3)
    {
        dmv_ver_x = ((ac_mv[2][MV_X] - ac_mv[0][MV_X]) << bit) >> xevd_tbl_log2[cuh]; // deltaMvVer
        dmv_ver_y = ((ac_mv[2][MV_Y] - ac_mv[0][MV_Y]) << bit) >> xevd_tbl_log2[cuh];
    }
    else
    {
        dmv_ver_x = -dmv_hor_y;                                                       // deltaMvVer
        dmv_ver_y = dmv_hor_x;
    }

    int b_eif = sub_w < AFFINE_ADAPT_EIF_SIZE || sub_h < AFFINE_ADAPT_EIF_SIZE;
    int d_hor[MV_D] = { dmv_hor_x, dmv_hor_y }, d_ver[MV_D] = { dmv_ver_x, dmv_ver_y };
    int mv_precision = MAX_CU_LOG2 + MC_PRECISION_ADD;
    BOOL clipMV = FALSE;

    if (b_eif)
    {
        int mv_scale[MV_D] = { mv_scale_hor, mv_scale_ver };
        int max_mv[MV_D] = { 0, 0 };
        int min_mv[MV_D] = { 0, 0 };

        eif_derive_mv_clip_range(x, y, cuw, cuh, d_hor, d_ver, mv_scale, pic_w, pic_h, !mem_band_conditions_for_eif_are_satisfied, max_mv, min_mv);
        xevdm_eif_mc(cuw, cuh, x, y, mv_scale_hor, mv_scale_ver, dmv_hor_x, dmv_hor_y, dmv_ver_x, dmv_ver_y,
            max_mv[MV_X], max_mv[MV_Y], min_mv[MV_X], min_mv[MV_Y],
            ref_pic->y, ref_pic->s_l, pred[Y_C], cuw, tmp_buffer_for_eif, bit + 2, Y_C

            , bit_depth_luma
                   , chroma_format_idc

        );

        if(chroma_format_idc)
        {
        xevdm_eif_mc(cuw, cuh, x, y, mv_scale_hor, mv_scale_ver, dmv_hor_x, dmv_hor_y, dmv_ver_x, dmv_ver_y,
            max_mv[MV_X], max_mv[MV_Y], min_mv[MV_X], min_mv[MV_Y],
                       ref_pic->u, ref_pic->s_c, pred[U_C], cuw >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), tmp_buffer_for_eif, bit + 2, U_C


            , bit_depth_chroma
                       , chroma_format_idc

        );

        xevdm_eif_mc(cuw, cuh, x, y, mv_scale_hor, mv_scale_ver, dmv_hor_x, dmv_hor_y, dmv_ver_x, dmv_ver_y,
            max_mv[MV_X], max_mv[MV_Y], min_mv[MV_X], min_mv[MV_Y],
                       ref_pic->v, ref_pic->s_c, pred[V_C], cuw >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), tmp_buffer_for_eif, bit + 2, V_C
            ,  bit_depth_chroma
                       , chroma_format_idc);
        }
        return;
    }

    int mv_scale_tmp_hor_ori, mv_scale_tmp_ver_ori;

    // get prediction block by block
    for (h = 0; h < cuh; h += sub_h)
    {
        for (w = 0; w < cuw; w += sub_w)
        {
            mv_scale_tmp_hor = (mv_scale_hor + dmv_hor_x * half_w + dmv_ver_x * half_h);
            mv_scale_tmp_ver = (mv_scale_ver + dmv_hor_y * half_w + dmv_ver_y * half_h);
            xevdm_mv_rounding_s32(mv_scale_tmp_hor, mv_scale_tmp_ver, &mv_scale_tmp_hor, &mv_scale_tmp_ver, shift, 0);
            mv_scale_tmp_hor = XEVD_CLIP3(-(1 << 17), (1 << 17) - 1, mv_scale_tmp_hor);
            mv_scale_tmp_ver = XEVD_CLIP3(-(1 << 17), (1 << 17) - 1, mv_scale_tmp_ver);
            mv_scale_tmp_ver_ori = mv_scale_tmp_ver;
            mv_scale_tmp_hor_ori = mv_scale_tmp_hor;
            // clip
            mv_scale_tmp_hor = XEVD_MIN(hor_max, XEVD_MAX(hor_min, mv_scale_tmp_hor));
            mv_scale_tmp_ver = XEVD_MIN(ver_max, XEVD_MAX(ver_min, mv_scale_tmp_ver));

            qpel_gmv_x = ((x + w) << mc_prec) + mv_scale_tmp_hor;
            qpel_gmv_y = ((y + h) << mc_prec) + mv_scale_tmp_ver;

            xevd_mc_l(mv_scale_tmp_hor_ori, mv_scale_tmp_ver_ori, ref_pic->y, qpel_gmv_x, qpel_gmv_y, ref_pic->s_l, cuw, (pred_y + w), sub_w, sub_h, bit_depth_luma);

            if(chroma_format_idc)
            {
                xevd_mc_c(mv_scale_tmp_hor_ori, mv_scale_tmp_ver_ori, ref_pic->u, qpel_gmv_x, qpel_gmv_y, ref_pic->s_c, cuw >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))
                         , pred_u + (w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))), sub_w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), sub_h >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)), bit_depth_chroma);
                xevd_mc_c(mv_scale_tmp_hor_ori, mv_scale_tmp_ver_ori, ref_pic->v, qpel_gmv_x, qpel_gmv_y, ref_pic->s_c, cuw >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))
                         , pred_v + (w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))), sub_w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), sub_h >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)), bit_depth_chroma);
            }

        }

        pred_y += (cuw * sub_h);

        pred_u += (cuw * sub_h) >> ((XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)) + (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)));
        pred_v += (cuw * sub_h) >> ((XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)) + (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)));

    }
}

static BOOL can_mv_clipping_occurs(int block_width, int block_height, int mv0[MV_D], int d_x[MV_D], int d_y[MV_D], int mv_max[MV_D], int mv_min[MV_D])
{
    int mv_corners[2][2][MV_D];
    BOOL mv_clip_occurs[MV_D] = { FALSE, FALSE };

    int mv[MV_D] = { mv0[MV_X] - d_x[MV_X] - d_y[MV_X], mv0[MV_Y] - d_x[MV_Y] - d_y[MV_Y] }; //set to pos (-1, -1)

    block_width = block_width + 1;
    block_height = block_height + 1;

    assert(MV_Y - MV_X == 1);

    for (int coord = MV_X; coord <= MV_Y; ++coord)
    {
        mv_corners[0][0][coord] = mv[coord];
        mv_corners[0][1][coord] = mv[coord] + block_width * d_x[coord];
        mv_corners[1][0][coord] = mv[coord] + block_height * d_y[coord];
        mv_corners[1][1][coord] = mv[coord] + block_width * d_x[coord] + block_height * d_y[coord];

        mv_corners[0][0][coord] >>= 4;
        mv_corners[0][1][coord] >>= 4;
        mv_corners[1][0][coord] >>= 4;
        mv_corners[1][1][coord] >>= 4;

        for (int i = 0; i < 2; ++i)
            for (int j = 0; j < 2; ++j)
            {
                if (mv_corners[i][j][coord] > mv_max[coord] || mv_corners[i][j][coord] < mv_min[coord])
                    mv_clip_occurs[coord] = TRUE;
            }
    }

    return mv_clip_occurs[MV_X] || mv_clip_occurs[MV_Y];
}

void xevdm_eif_filter(int block_width, int block_height, pel* p_tmp_buf, int tmp_buf_stride, pel *p_dst, int dst_stride, int shifts[4], int offsets[4], int bit_depth)
{
    pel* p_buf = p_tmp_buf + 1;

    for (int y = 0; y <= block_height + 1; ++y, p_buf += tmp_buf_stride)
    {
        pel* t = p_buf;

        for (int x = 1; x <= block_width; ++x, ++t)
            t[-1] = (-t[-1] + (t[0] * 10) - t[1] + offsets[2]) >> shifts[2];
    }

    p_buf = p_tmp_buf + tmp_buf_stride;

    for (int y = 0; y < block_height; ++y, p_buf += tmp_buf_stride, p_dst += dst_stride)
    {
        pel* p_dst_buf = p_dst;
        pel* t = p_buf;

        for (int x = 0; x < block_width; ++x, ++t, ++p_dst_buf)
        {
            pel res = (-t[-tmp_buf_stride] + (t[0] * 10) - t[tmp_buf_stride] + offsets[3]) >> shifts[3];

            *p_dst_buf = XEVD_MIN((1 << bit_depth) - 1, XEVD_MAX(0, res));
        }
    }
}

void xevdm_eif_bilinear_clip(int block_width, int block_height, int mv0[MV_D], int d_x[MV_D], int d_y[MV_D], int mv_max[MV_D], int mv_min[MV_D], pel* p_ref, int ref_stride, pel* p_dst, int dst_stride, int shifts[4], int offsets[4]
    , int bit_depth

)
{
    int mv[MV_D] = { mv0[MV_X], mv0[MV_Y] };

    const pel fracMask = (1 << EIF_MV_PRECISION_BILINEAR) - 1;

    pel* p_buf = p_dst;

    int shift1 = XEVD_MIN(4, bit_depth - 8);
    int shift2 = XEVD_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));


    int tmp_mv_for_line[MV_D] = { mv0[MV_X] - d_x[MV_X] - d_y[MV_X], mv0[MV_Y] - d_x[MV_Y] - d_y[MV_Y] }; //set to pos (-1, -1)

    for (int y = -1; y <= block_height; ++y, p_buf += dst_stride, tmp_mv_for_line[MV_X] += d_y[MV_X], tmp_mv_for_line[MV_Y] += d_y[MV_Y])
    {
        int tmp_mv[MV_D] = { tmp_mv_for_line[MV_X], tmp_mv_for_line[MV_Y] };

        for (int x = -1; x <= block_width; ++x, tmp_mv[MV_X] += d_x[MV_X], tmp_mv[MV_Y] += d_x[MV_Y])
        {
            mv[MV_X] = XEVD_MIN(mv_max[MV_X], XEVD_MAX(mv_min[MV_X], tmp_mv[MV_X] >> (EIF_MV_PRECISION_INTERNAL - EIF_MV_PRECISION_BILINEAR)));
            mv[MV_Y] = XEVD_MIN(mv_max[MV_Y], XEVD_MAX(mv_min[MV_Y], tmp_mv[MV_Y] >> (EIF_MV_PRECISION_INTERNAL - EIF_MV_PRECISION_BILINEAR)));

            int xInt = x + (mv[MV_X] >> EIF_MV_PRECISION_BILINEAR);
            int yInt = y + (mv[MV_Y] >> EIF_MV_PRECISION_BILINEAR);

            pel xFrac = mv[MV_X] & fracMask;
            pel yFrac = mv[MV_Y] & fracMask;

            pel* r = p_ref + yInt * ref_stride + xInt;

            pel s1 = MAC_BL_NN_S1(tbl_bl_eif_32_phases_mc_l_coeff[xFrac], r[0], r[1], offset1, shift1);
            pel s2 = MAC_BL_NN_S1(tbl_bl_eif_32_phases_mc_l_coeff[xFrac], r[ref_stride], r[ref_stride + 1], offset1, shift1);
            p_buf[x + 1] = MAC_BL_NN_S2(tbl_bl_eif_32_phases_mc_l_coeff[yFrac], s1, s2, offset2, shift2);


        }
    }
}

void xevdm_eif_bilinear_no_clip(int block_width, int block_height, int mv0[MV_D], int d_x[MV_D], int d_y[MV_D], pel* p_ref, int ref_stride, pel* p_dst, int dst_stride, int shifts[4], int offsets[4], int bit_depth)
{
    int mv[MV_D] = { mv0[MV_X], mv0[MV_Y] };

    const pel fracMask = (1 << EIF_MV_PRECISION_BILINEAR) - 1;

    pel* p_buf = p_dst;

    int shift1 = XEVD_MIN(4, bit_depth - 8);
    int shift2 = XEVD_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));


    int tmp_mv_for_line[MV_D] = { mv0[MV_X] - d_x[MV_X] - d_y[MV_X], mv0[MV_Y] - d_x[MV_Y] - d_y[MV_Y] }; //set to pos (-1, -1)

    for (int y = -1; y <= block_height; ++y, p_buf += dst_stride, tmp_mv_for_line[MV_X] += d_y[MV_X], tmp_mv_for_line[MV_Y] += d_y[MV_Y])
    {
        int tmp_mv[MV_D] = { tmp_mv_for_line[MV_X], tmp_mv_for_line[MV_Y] };

        for (int x = -1; x <= block_width; ++x, tmp_mv[MV_X] += d_x[MV_X], tmp_mv[MV_Y] += d_x[MV_Y])
        {
            mv[MV_X] = tmp_mv[MV_X] >> (EIF_MV_PRECISION_INTERNAL - EIF_MV_PRECISION_BILINEAR);
            mv[MV_Y] = tmp_mv[MV_Y] >> (EIF_MV_PRECISION_INTERNAL - EIF_MV_PRECISION_BILINEAR);

            int xInt = x + (mv[MV_X] >> EIF_MV_PRECISION_BILINEAR);
            int yInt = y + (mv[MV_Y] >> EIF_MV_PRECISION_BILINEAR);

            pel xFrac = mv[MV_X] & fracMask;
            pel yFrac = mv[MV_Y] & fracMask;

            pel* r = p_ref + yInt * ref_stride + xInt;

            pel s1 = MAC_BL_NN_S1(tbl_bl_eif_32_phases_mc_l_coeff[xFrac], r[0], r[1], offset1, shift1);
            pel s2 = MAC_BL_NN_S1(tbl_bl_eif_32_phases_mc_l_coeff[xFrac], r[ref_stride], r[ref_stride + 1], offset1, shift1);
            p_buf[x + 1] = MAC_BL_NN_S2(tbl_bl_eif_32_phases_mc_l_coeff[yFrac], s1, s2, offset2, shift2);


        }
    }
}

void xevdm_eif_mc(int block_width, int block_height, int x, int y, int mv_scale_hor, int mv_scale_ver, int dmv_hor_x, int dmv_hor_y, int dmv_ver_x, int dmv_ver_y,
    int hor_max, int ver_max, int hor_min, int ver_min, pel* p_ref, int ref_stride, pel *p_dst, int dst_stride, pel* p_tmp_buf, char affine_mv_prec, s8 comp
    , int bit_depth
                , int chroma_format_idc
)
{
    assert(EIF_MV_PRECISION_INTERNAL >= affine_mv_prec);  //For current affine internal MV precision is (2 + bit) bits; 2 means qpel
    assert(EIF_MV_PRECISION_INTERNAL >= 2 + MC_PRECISION_ADD);  //For current affine internal MV precision is (2 + bit) bits; 2 means qpel

    int mv0[MV_D] = { mv_scale_hor << (EIF_MV_PRECISION_INTERNAL - affine_mv_prec),
                      mv_scale_ver << (EIF_MV_PRECISION_INTERNAL - affine_mv_prec) };
    int d_x[MV_D] = { dmv_hor_x << (EIF_MV_PRECISION_INTERNAL - affine_mv_prec),
                      dmv_hor_y << (EIF_MV_PRECISION_INTERNAL - affine_mv_prec) };
    int d_y[MV_D] = { dmv_ver_x << (EIF_MV_PRECISION_INTERNAL - affine_mv_prec),
                      dmv_ver_y << (EIF_MV_PRECISION_INTERNAL - affine_mv_prec) };

    int mv_max[MV_D] = { hor_max, ver_max };
    int mv_min[MV_D] = { hor_min, ver_min };

    if (comp > Y_C)
    {
        mv0[MV_X] >>= (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc));
        mv0[MV_Y] >>= (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));
        mv_max[MV_X] >>= (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc));
        mv_max[MV_Y] >>= (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));
        mv_min[MV_X] >>= (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc));
        mv_min[MV_Y] >>= (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));
        block_width >>= (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc));
        block_height >>= (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));
        x >>= (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc));
        y >>= (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));

    }

    p_ref += ref_stride * y + x;

    const int tmp_buf_stride = MAX_CU_SIZE + 2;

    assert(bit_depth < 16);


    int shifts[4] = { 0, 0, XEVD_MAX(bit_depth + 5 - 16, 0), 6 - XEVD_MAX(bit_depth + 5 - 16, 0) };


    int offsets[4] = { 0, 0, 0, 0 };

    for (int i = 0; i < 4; ++i)
        offsets[i] = 1 << (shifts[i] - 1);

    BOOL is_mv_clip_needed = can_mv_clipping_occurs(block_width, block_height, mv0, d_x, d_y, mv_max, mv_min);

    if (is_mv_clip_needed)
        xevdm_eif_bilinear_clip(block_width, block_height, mv0, d_x, d_y, mv_max, mv_min, p_ref, ref_stride, p_tmp_buf, tmp_buf_stride, shifts, offsets
            , bit_depth
        );
    else
        xevdm_eif_bilinear_no_clip(block_width, block_height, mv0, d_x, d_y, p_ref, ref_stride, p_tmp_buf, tmp_buf_stride, shifts, offsets
            , bit_depth
        );

    xevdm_eif_filter(block_width, block_height, p_tmp_buf, tmp_buf_stride, p_dst, dst_stride, shifts, offsets, bit_depth);
}

void xevdm_affine_mc(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16 mv[REFP_NUM][VER_NUM][MV_D], XEVD_REFP(*refp)[REFP_NUM], pel pred[2][N_C][MAX_CU_DIM], int vertex_num, pel* tmp_buffer
    , int bit_depth_luma, int bit_depth_chroma
                   , int chroma_format_idc
)
{
    XEVD_PIC *ref_pic;
    pel      *p0, *p1, *p2, *p3;
    int       i, j, bidx = 0;

    // derive sub-block size
    int sub_w = 4, sub_h = 4;

    BOOL mem_band_conditions_for_eif_are_satisfied = FALSE;

    xevdm_derive_affine_subblock_size_bi(mv, refi, w, h, &sub_w, &sub_h, vertex_num, &mem_band_conditions_for_eif_are_satisfied);

    if (REFI_IS_VALID(refi[REFP_0]))
    {
        /* forward */
        ref_pic = refp[refi[REFP_0]][REFP_0].pic;
        xevdm_affine_mc_lc(x, y, pic_w, pic_h, w, h, mv[REFP_0], ref_pic, pred[0], vertex_num, sub_w, sub_h, tmp_buffer, mem_band_conditions_for_eif_are_satisfied
            ,  bit_depth_luma,  bit_depth_chroma
                         , chroma_format_idc
        );

        bidx++;
    }

    if (REFI_IS_VALID(refi[REFP_1]))
    {
        /* backward */
        ref_pic = refp[refi[REFP_1]][REFP_1].pic;
        xevdm_affine_mc_lc(x, y, pic_w, pic_h, w, h, mv[REFP_1], ref_pic, pred[bidx], vertex_num, sub_w, sub_h, tmp_buffer, mem_band_conditions_for_eif_are_satisfied
            ,  bit_depth_luma,  bit_depth_chroma
                         , chroma_format_idc

        );

        bidx++;
    }

    if (bidx == 2)
    {
        p0 = pred[0][Y_C];
        p1 = pred[1][Y_C];
        for (j = 0; j < h; j++)
        {
            for (i = 0; i < w; i++)
            {
                p0[i] = (p0[i] + p1[i] + 1) >> 1;
            }
            p0 += w;
            p1 += w;
        }
        p0 = pred[0][U_C];
        p1 = pred[1][U_C];
        p2 = pred[0][V_C];
        p3 = pred[1][V_C];

        w >>= (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc));
        h >>= (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));


        if(chroma_format_idc)
        {
        for (j = 0; j < h; j++)
        {
            for (i = 0; i < w; i++)
            {
                p0[i] = (p0[i] + p1[i] + 1) >> 1;
                p2[i] = (p2[i] + p3[i] + 1) >> 1;
            }
            p0 += w;
            p1 += w;
            p2 += w;
            p3 += w;
        }
    }
}
}
