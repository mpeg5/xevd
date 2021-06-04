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
#include "xevd_tbl.h"
#include "xevd_mc.h"
#include "xevd_util.h"
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




XEVD_MC_L (*xevd_func_mc_l)[2];
XEVD_MC_C (*xevd_func_mc_c)[2];
XEVD_AVG_NO_CLIP xevd_func_average_no_clip;


s16 xevd_tbl_mc_l_coeff[16][8] =
{
    {  0, 0,   0, 64,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 1,  -5, 52, 20,  -5,  1,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 2, -10, 40, 40, -10,  2,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 1,  -5, 20, 52,  -5,  1,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
};

s16 xevd_tbl_mc_c_coeff[32][4] =
{
    {  0, 64,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    { -2, 58, 10, -2 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    { -4, 52, 20, -4 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    { -6, 46, 30, -6 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    { -8, 40, 40, -8 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    { -6, 30, 46, -6 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    { -4, 20, 52, -4 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    { -2, 10, 58, -2 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
};


s16 (*tbl_mc_l_coeff)[8] = xevd_tbl_mc_l_coeff;
s16 (*tbl_mc_c_coeff)[4] = xevd_tbl_mc_c_coeff;


/****************************************************************************
 * motion compensation for luma
 ****************************************************************************/

void xevd_average_16b_no_clip(s16 *src, s16 *ref, s16 *dst, int s_src, int s_ref, int s_dst, int wd, int ht
    , int bit_depth
)
{
    pel *p0, *p1;
    int i, j, w, h;
    p0 = src;
    p1 = ref;
    w = wd;
    h = ht;
         
    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            p0[i] = (p0[i] + p1[i] + 1) >> 1;
        }
        p0 += w;
        p1 += w;
     }

}


void xevd_mc_l_00(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int i, j;

    gmv_x >>= 4;
    gmv_y >>= 4;

    ref += gmv_y * s_ref + gmv_x;
 
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

void xevd_mc_l_n0(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h
,int bit_depth)
{
    int i, j, dx;
    s32 pt;

    dx = gmv_x & 15;
    ref += (gmv_y >> MC_PRECISION) * s_ref + (gmv_x >> MC_PRECISION) - 3;
  
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


void xevd_mc_l_0n(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h
,int bit_depth)
{
    int i, j, dy;
    s32 pt;


    dy = gmv_y & 15;
    ref += ((gmv_y >> MC_PRECISION) - 3) * s_ref + (gmv_x >> MC_PRECISION);

   
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


void xevd_mc_l_nn(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h
,int bit_depth)
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_L)*(MAX_CU_SIZE + MC_IBUF_PAD_L)];
    s16        *b;
    int         i, j, dx, dy;
    s32         pt;

    dx = gmv_x & 15;
    dy = gmv_y & 15;
    ref += ((gmv_y >> MC_PRECISION) - 3) * s_ref + (gmv_x >> MC_PRECISION) - 3;


    int shift1 = XEVD_MIN(4, bit_depth - 8);
    int shift2 = XEVD_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));

    b = buf;
    for (i = 0; i < h + 7; i++)
    {
        for (j = 0; j < w; j++)
        {
    
            b[j] = MAC_8TAP_NN_S1(tbl_mc_l_coeff[dx], ref[j], ref[j + 1], ref[j + 2], ref[j + 3], ref[j + 4], ref[j + 5], ref[j + 6], ref[j + 7],offset1, shift1);
    
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


/****************************************************************************
 * motion compensation for chroma
 ****************************************************************************/
void xevd_mc_c_00(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int i, j;

    gmv_x >>= (MC_PRECISION + 1);
    gmv_y >>= (MC_PRECISION + 1);

    ref += gmv_y * s_ref + gmv_x;

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

void xevd_mc_c_n0(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int       i, j, dx;
    s32       pt;

    dx = gmv_x & 31;
    ref += (gmv_y >> (MC_PRECISION + 1)) * s_ref + (gmv_x >> (MC_PRECISION + 1)) - 1;


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

void xevd_mc_c_0n(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int i, j, dy;
    s32       pt;

    dy = gmv_y & 31;
    ref += ((gmv_y >> (MC_PRECISION + 1)) - 1) * s_ref + (gmv_x >> (MC_PRECISION + 1));


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

void xevd_mc_c_nn(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_C)*MAX_CU_SIZE];
    s16        *b;
    int         i, j;
    s32         pt;
    int         dx, dy;

    dx = gmv_x & 31;
    dy = gmv_y & 31;
    ref += ((gmv_y >> (MC_PRECISION + 1)) - 1) * s_ref + (gmv_x >> (MC_PRECISION + 1)) - 1;

    int shift1 = XEVD_MIN(4, bit_depth - 8);
    int shift2 = XEVD_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));


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


            pt = MAC_4TAP_NN_S2(tbl_mc_c_coeff[dy], b[j], b[j + w], b[j + 2 * w], b[j + 3 * w], offset2, shift2);
            pred[j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, pt);


        }
        pred += s_pred;
        b += w;
    }


}

XEVD_MC_L xevd_tbl_mc_l[2][2] =
{
    {
        xevd_mc_l_00, /* dx == 0 && dy == 0 */
        xevd_mc_l_0n  /* dx == 0 && dy != 0 */
    },
    {
        xevd_mc_l_n0, /* dx != 0 && dy == 0 */
        xevd_mc_l_nn  /* dx != 0 && dy != 0 */
    }
};

XEVD_MC_C xevd_tbl_mc_c[2][2] =
{
    {
        xevd_mc_c_00, /* dx == 0 && dy == 0 */
        xevd_mc_c_0n  /* dx == 0 && dy != 0 */
    },
    {
        xevd_mc_c_n0, /* dx != 0 && dy == 0 */
        xevd_mc_c_nn  /* dx != 0 && dy != 0 */
    }
};


void xevd_mv_clip(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16 mv[REFP_NUM][MV_D], s16(*mv_t)[MV_D])
{
    int min_clip[MV_D], max_clip[MV_D];

    x <<= 2;
    y <<= 2;
    w <<= 2;
    h <<= 2;
    min_clip[MV_X] = (-MAX_CU_SIZE) << 2;
    min_clip[MV_Y] = (-MAX_CU_SIZE) << 2;
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

void xevd_mc(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16(*mv)[MV_D], XEVD_REFP(*refp)[REFP_NUM], pel pred[REFP_NUM][N_C][MAX_CU_DIM]
           , int poc_c,  int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc) 
{
    XEVD_PIC    *ref_pic;
    int          qpel_gmv_x, qpel_gmv_y;
    int          bidx = 0;
    s16          mv_t[REFP_NUM][MV_D];
    s16          mv_before_clipping[REFP_NUM][MV_D]; //store it to pass it to interpolation function for deriving correct interpolation filter

    mv_before_clipping[REFP_0][MV_X] = mv[REFP_0][MV_X];
    mv_before_clipping[REFP_0][MV_Y] = mv[REFP_0][MV_Y];
    mv_before_clipping[REFP_1][MV_X] = mv[REFP_1][MV_X];
    mv_before_clipping[REFP_1][MV_Y] = mv[REFP_1][MV_Y];

    xevd_mv_clip(x, y, pic_w, pic_h, w, h, refi, mv, mv_t);

    s16          mv_refine[REFP_NUM][MV_D] = { { mv[REFP_0][MV_X], mv[REFP_0][MV_Y] },
                                               { mv[REFP_1][MV_X], mv[REFP_1][MV_Y] } };
    s16          inital_mv[REFP_NUM][MV_D] = { { mv[REFP_0][MV_X], mv[REFP_0][MV_Y] },
                                               { mv[REFP_1][MV_X], mv[REFP_1][MV_Y] } };
    s16          mv_offsets[REFP_NUM][MV_D] = { { 0, }, };
    s32          center_point_avgs_l0_l1[2 * REFP_NUM] = { 0, 0, 0, 0 }; // center_point_avgs_l0_l1[2,3] for "A" and "B" current center point average



    if (REFI_IS_VALID(refi[REFP_0]))
    {
        /* forward */
        ref_pic = refp[refi[REFP_0]][REFP_0].pic;
        qpel_gmv_x = (x << 2) + mv_t[REFP_0][MV_X];
        qpel_gmv_y = (y << 2) + mv_t[REFP_0][MV_Y];

        xevd_mc_l(mv_before_clipping[REFP_0][MV_X] << 2, mv_before_clipping[REFP_0][MV_Y] << 2, ref_pic->y, (qpel_gmv_x << 2), (qpel_gmv_y << 2)
                , ref_pic->s_l, w, pred[0][Y_C], w, h, bit_depth_luma);
        xevd_mc_c(mv_before_clipping[REFP_0][MV_X] << 2, mv_before_clipping[REFP_0][MV_Y] << 2, ref_pic->u, (qpel_gmv_x << 2), (qpel_gmv_y << 2)
                , ref_pic->s_c, w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), pred[0][U_C], w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))
                , h >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)), bit_depth_chroma);
        xevd_mc_c(mv_before_clipping[REFP_0][MV_X] << 2, mv_before_clipping[REFP_0][MV_Y] << 2, ref_pic->v, (qpel_gmv_x << 2), (qpel_gmv_y << 2)
                , ref_pic->s_c, w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), pred[0][V_C], w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))
                , h >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)), bit_depth_chroma);
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


        xevd_mc_l(mv_before_clipping[REFP_1][MV_X] << 2, mv_before_clipping[REFP_1][MV_Y] << 2, ref_pic->y, (qpel_gmv_x << 2), (qpel_gmv_y << 2)
                , ref_pic->s_l, w, pred[bidx][Y_C], w, h, bit_depth_luma);
        xevd_mc_c(mv_before_clipping[REFP_1][MV_X] << 2, mv_before_clipping[REFP_1][MV_Y] << 2, ref_pic->u, (qpel_gmv_x << 2), (qpel_gmv_y << 2)
                , ref_pic->s_c, w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), pred[bidx][U_C], w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))
                , h >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)), bit_depth_chroma);
        xevd_mc_c(mv_before_clipping[REFP_1][MV_X] << 2, mv_before_clipping[REFP_1][MV_Y] << 2, ref_pic->v, (qpel_gmv_x << 2), (qpel_gmv_y << 2)
                , ref_pic->s_c, w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), pred[bidx][V_C], w >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))
                , h >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)), bit_depth_chroma);
        bidx++;
    }

    if (bidx == 2)
    {
        BOOL template_needs_update = FALSE;
        s32 center_cost[2] = { 1 << 30, 1 << 30 };

        //only if the references are located on opposite sides of the current frame
        xevd_func_average_no_clip(pred[0][Y_C], pred[1][Y_C], pred[0][Y_C], w, w, w, w, h, bit_depth_luma);

        if (chroma_format_idc)
        {
             w >>= (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc));
             h >>= (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));

            xevd_func_average_no_clip(pred[0][U_C], pred[1][U_C], pred[0][U_C], w, w, w, w, h, bit_depth_chroma);
            xevd_func_average_no_clip(pred[0][V_C], pred[1][V_C], pred[0][V_C], w, w, w, w, h, bit_depth_chroma);
        }
     }
}

