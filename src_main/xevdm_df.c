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

#include "xevdm_def.h"
#include "xevdm_df.h"
#include "xevdm_tbl.h"

const u8 * xevdm_get_tbl_qp_to_st(u32 mcu0, u32 mcu1, s8 *refi0, s8 *refi1, s16 (*mv0)[MV_D], s16 (*mv1)[MV_D])
{
    int idx = 3;

    if(MCU_GET_IF(mcu0) || MCU_GET_IF(mcu1))
    {
        idx = 0;
    }
    else if(MCU_GET_CBFL(mcu0) == 1 || MCU_GET_CBFL(mcu1) == 1)
    {
        idx = 1;
    }
    else if (MCU_GET_IBC(mcu0) || MCU_GET_IBC(mcu1))
    {
      idx = 2;
    }
    else
    {
        int mv0_l0[2] = {mv0[REFP_0][MV_X], mv0[REFP_0][MV_Y]};
        int mv0_l1[2] = {mv0[REFP_1][MV_X], mv0[REFP_1][MV_Y]};
        int mv1_l0[2] = {mv1[REFP_0][MV_X], mv1[REFP_0][MV_Y]};
        int mv1_l1[2] = {mv1[REFP_1][MV_X], mv1[REFP_1][MV_Y]};

        if(!REFI_IS_VALID(refi0[REFP_0]))
        {
            mv0_l0[0] = mv0_l0[1] = 0;
        }

        if(!REFI_IS_VALID(refi0[REFP_1]))
        {
            mv0_l1[0] = mv0_l1[1] = 0;
        }

        if(!REFI_IS_VALID(refi1[REFP_0]))
        {
            mv1_l0[0] = mv1_l0[1] = 0;
        }

        if(!REFI_IS_VALID(refi1[REFP_1]))
        {
            mv1_l1[0] = mv1_l1[1] = 0;
        }

        if(((refi0[REFP_0] == refi1[REFP_0]) && (refi0[REFP_1] == refi1[REFP_1])))
        {
            idx = (XEVD_ABS(mv0_l0[MV_X] - mv1_l0[MV_X]) >= 4 ||
                   XEVD_ABS(mv0_l0[MV_Y] - mv1_l0[MV_Y]) >= 4 ||
                   XEVD_ABS(mv0_l1[MV_X] - mv1_l1[MV_X]) >= 4 ||
                   XEVD_ABS(mv0_l1[MV_Y] - mv1_l1[MV_Y]) >= 4 ) ? 2 : 3;
        }
        else if((refi0[REFP_0] == refi1[REFP_1]) && (refi0[REFP_1] == refi1[REFP_0]))
        {
            idx = (XEVD_ABS(mv0_l0[MV_X] - mv1_l1[MV_X]) >= 4 ||
                   XEVD_ABS(mv0_l0[MV_Y] - mv1_l1[MV_Y]) >= 4 ||
                   XEVD_ABS(mv0_l1[MV_X] - mv1_l0[MV_X]) >= 4 ||
                   XEVD_ABS(mv0_l1[MV_Y] - mv1_l0[MV_Y]) >= 4) ? 2 : 3;
        }
        else
        {
            idx = 2;
        }
    }

    return xevd_tbl_df_st[idx];
}

static void deblock_cu_hor(XEVD_CTX *ctx, XEVD_PIC *pic, int x_pel, int y_pel, int cuw, int cuh, TREE_CONS tree_cons, int boundary_filtering)
{
    pel       * y, *u, *v;
    const u8  * tbl_qp_to_st;
    int         i, t, qp, s_l, s_c, st;
    int         w = cuw >> MIN_CU_LOG2;
    int         h = cuh >> MIN_CU_LOG2;
    u32       * map_scu_tmp;
    int         j;
    int         t1, t_copy; // Next row scu number
    u32        *map_scu = ctx->map_scu;
    s8         (*map_refi)[REFP_NUM] = ctx->map_refi;
    s16        (*map_mv)[REFP_NUM][MV_D] = ctx->map_mv;
    int        w_scu = ctx->w_scu;
    u8         *map_tidx = ctx->map_tidx;
    int         bit_depth_chroma = ctx->sps->bit_depth_chroma_minus8 + 8;
    int         bit_depth_luma = ctx->sps->bit_depth_luma_minus8 + 8;
    int         chroma_format_idc = ctx->sps->chroma_format_idc;

    t = (x_pel >> MIN_CU_LOG2) + (y_pel >> MIN_CU_LOG2) * w_scu;
    t_copy = t;
    t1 = (x_pel >> MIN_CU_LOG2) + ((y_pel - (1 << MIN_CU_LOG2)) >> MIN_CU_LOG2) * w_scu;
    map_scu += t;
    map_refi += t;
    map_mv += t;
    map_scu_tmp = map_scu;
    s_l = pic->s_l;
    s_c = pic->s_c;
    y = pic->y + x_pel + y_pel * s_l;
    t = (x_pel >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))) + (y_pel >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc))) * s_c;
    u = pic->u + t;
    v = pic->v + t;

    unsigned int no_boundary = 0;
    if (y_pel > 0)
    {
        no_boundary = (map_tidx[t_copy] == map_tidx[t1]) || boundary_filtering;
    }

    /* horizontal filtering */
    if(y_pel > 0 && (no_boundary))
    {
        for(i = 0; i < (cuw >> MIN_CU_LOG2); i++)
        {
            tbl_qp_to_st = xevdm_get_tbl_qp_to_st(map_scu[i], map_scu[i - w_scu], map_refi[i], map_refi[i - w_scu], map_mv[i], map_mv[i - w_scu]);

            qp = MCU_GET_QP(map_scu[i]);
            t = (i << MIN_CU_LOG2);
            st = tbl_qp_to_st[qp] << (bit_depth_luma - 8);
            if (xevd_check_luma_fn(tree_cons) && st)
            {
                (*ctx->fn_dbk)[1](y + t, st, s_l, bit_depth_luma - 8, chroma_format_idc);
            }

            if(xevd_check_chroma_fn(tree_cons) && (chroma_format_idc != 0))
            {
                t = t >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc));
                int qp_u = XEVD_CLIP3(-6 * (bit_depth_chroma - 8), 57, qp + pic->pic_qp_u_offset);
                int qp_v = XEVD_CLIP3(-6 * (bit_depth_chroma - 8), 57, qp + pic->pic_qp_v_offset);
                int st_u = tbl_qp_to_st[xevd_qp_chroma_dynamic[0][qp_u]] << (bit_depth_chroma - 8);
                int st_v = tbl_qp_to_st[xevd_qp_chroma_dynamic[1][qp_v]] << (bit_depth_chroma - 8);
                if (st_u || st_v)
                {
                    (*ctx->fn_dbk_chroma)[1](u + t, v + t, st_u, st_v, s_c, bit_depth_chroma - 8, chroma_format_idc);
                }
            }
        }
    }

    map_scu = map_scu_tmp;
    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            MCU_SET_COD(map_scu[j]);
        }
        map_scu += w_scu;
    }
}

static void deblock_cu_ver(XEVD_CTX *ctx, XEVD_PIC *pic, int x_pel, int y_pel, int cuw, int cuh, TREE_CONS tree_cons, int boundary_filtering)
{
      
    pel         * y, *u, *v;
    const u8    * tbl_qp_to_st;
    int         i, t, qp, s_l, s_c, st;
    int         w = cuw >> MIN_CU_LOG2;
    int         h = cuh >> MIN_CU_LOG2;
    int         j;
    u32         * map_scu_tmp;
    s8         (*map_refi)[REFP_NUM];
    s8         (*map_refi_tmp)[REFP_NUM];
    s16        (*map_mv)[REFP_NUM][MV_D];
    s16        (*map_mv_tmp)[REFP_NUM][MV_D];
    u32        *map_scu = ctx->map_scu;
    u32        *map_cu = ctx->map_cu_mode;
    int        w_scu = ctx->w_scu;
    u8         *map_tidx = ctx->map_tidx;
    int        bit_depth_chroma = ctx->sps->bit_depth_chroma_minus8 + 8;
    int        bit_depth_luma = ctx->sps->bit_depth_luma_minus8 + 8;
    int        chroma_format_idc = ctx->sps->chroma_format_idc;
    int        t1, t2, t_copy; // Next row scu number
    map_refi = ctx->map_refi;
    map_mv = ctx->map_mv;

    t = (x_pel >> MIN_CU_LOG2) + (y_pel >> MIN_CU_LOG2) * w_scu;
    t_copy = t;
    t1 = ((x_pel - (1 << MIN_CU_LOG2)) >> MIN_CU_LOG2) + (y_pel >> MIN_CU_LOG2) * w_scu;
    t2 = ((x_pel + (w << MIN_CU_LOG2)) >> MIN_CU_LOG2) + (y_pel >> MIN_CU_LOG2) * w_scu;
    map_scu += t;
    map_refi += t;
    map_mv += t;

    s_l = pic->s_l;
    s_c = pic->s_c;
    y = pic->y + x_pel + y_pel * s_l;
    t = (x_pel >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))) + (y_pel >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc))) * s_c;
    u = pic->u + t;
    v = pic->v + t;

    map_scu_tmp = map_scu;
    map_refi_tmp = map_refi;
    map_mv_tmp = map_mv;

    int no_boundary = 0;
    if (x_pel > 0)
    {
        no_boundary = (map_tidx[t_copy] == map_tidx[t1]) || boundary_filtering;
    }

    /* vertical filtering */
    if(x_pel > 0 && MCU_GET_COD(map_scu[-1]) && (no_boundary))
    {
        for(i = 0; i < (cuh >> MIN_CU_LOG2); i++)
        {
            tbl_qp_to_st = xevdm_get_tbl_qp_to_st(map_scu[0], map_scu[-1], \
                                            map_refi[0], map_refi[-1], map_mv[0], map_mv[-1]);
            qp = MCU_GET_QP(map_scu[0]);
            st = tbl_qp_to_st[qp] << (bit_depth_luma - 8);
            if (xevd_check_luma_fn(tree_cons) && st)
            {
                (*ctx->fn_dbk)[0](y, st, s_l, bit_depth_luma - 8, chroma_format_idc);
            }

            if (xevd_check_chroma_fn(tree_cons) && (chroma_format_idc != 0))
            {
                int qp_u = XEVD_CLIP3(-6 * (bit_depth_chroma - 8), 57, qp + pic->pic_qp_u_offset);
                int qp_v = XEVD_CLIP3(-6 * (bit_depth_chroma - 8), 57, qp + pic->pic_qp_v_offset);
                int st_u = tbl_qp_to_st[xevd_qp_chroma_dynamic[0][qp_u]] << (bit_depth_chroma - 8);
                int st_v = tbl_qp_to_st[xevd_qp_chroma_dynamic[1][qp_v]] << (bit_depth_chroma - 8);
                if (st_u || st_v)
                {
                    (*ctx->fn_dbk_chroma)[0](u, v, st_u, st_v, s_c, bit_depth_chroma - 8, chroma_format_idc);
                }
             }

            y += (s_l << MIN_CU_LOG2);
            u += (s_c << (MIN_CU_LOG2 - (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))));
            v += (s_c << (MIN_CU_LOG2 - (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))));
            map_scu += w_scu;
            map_refi += w_scu;
            map_mv += w_scu;
        }
    }

    no_boundary = 0;
    if (x_pel + cuw < pic->w_l)
    {
        no_boundary = (map_tidx[t_copy] == map_tidx[t2]) || boundary_filtering;
    }

    map_scu = map_scu_tmp;
    map_refi = map_refi_tmp;
    map_mv = map_mv_tmp;
    if(x_pel + cuw < pic->w_l && MCU_GET_COD(map_scu[w]) && (no_boundary))
    {
        y = pic->y + x_pel + y_pel * s_l;
        u = pic->u + t;
        v = pic->v + t;

        y += cuw;
        u += (cuw >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)));
        v += (cuw >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)));

        for(i = 0; i < (cuh >> MIN_CU_LOG2); i++)
        {
            tbl_qp_to_st = xevdm_get_tbl_qp_to_st(map_scu[w], map_scu[w - 1], map_refi[w], map_refi[w - 1], map_mv[w], map_mv[w - 1]);
            qp = MCU_GET_QP(map_scu[w]);
            st = tbl_qp_to_st[qp] << (bit_depth_luma - 8);
            if (xevd_check_luma_fn(tree_cons) && st)
            {
                (*ctx->fn_dbk)[0](y, st, s_l, bit_depth_luma - 8, chroma_format_idc);
            }
            if(xevd_check_chroma_fn(tree_cons) && (chroma_format_idc != 0))
            {
                int qp_u = XEVD_CLIP3(-6 * (bit_depth_chroma - 8), 57, qp + pic->pic_qp_u_offset);
                int qp_v = XEVD_CLIP3(-6 * (bit_depth_chroma - 8), 57, qp + pic->pic_qp_v_offset);
                int st_u = tbl_qp_to_st[xevd_qp_chroma_dynamic[0][qp_u]] << (bit_depth_chroma - 8);
                int st_v = tbl_qp_to_st[xevd_qp_chroma_dynamic[1][qp_v]] << (bit_depth_chroma - 8);
                if (st_u || st_v)
                {
                    (*ctx->fn_dbk_chroma)[0](u, v, st_u, st_v, s_c, bit_depth_chroma - 8, chroma_format_idc);
                }
             }

            y += (s_l << MIN_CU_LOG2);
            u += (s_c << (MIN_CU_LOG2 - (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))));
            v += (s_c << (MIN_CU_LOG2 - (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))));
            map_scu += w_scu;
            map_refi += w_scu;
            map_mv += w_scu;
        }
    }

    map_scu = map_scu_tmp;
    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            MCU_SET_COD(map_scu[j]);
        }
        map_scu += w_scu;
    }
}

#define DEFAULT_INTRA_TC_OFFSET             2
#define MAX_QP                              51

#define TCOFFSETDIV2                        0
#define BETAOFFSETDIV2                      0

#define CU_THRESH                           16

static const u8 sm_tc_table[MAX_QP + 1 + DEFAULT_INTRA_TC_OFFSET] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,5,5,6,6,7,8,9,10,11,13,14,16,18,20,22,24
};

static const u8 sm_beta_table[MAX_QP + 1] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,7,8,9,10,11,12,13,14,15,16,17,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64
};


static const u8 compare_mvs(const int mv0[2], const int mv1[2])
{
    // Return 1 if vetors difference less then 1 pixel
    return (XEVD_ABS(mv0[0] - mv1[0]) < 4) && (XEVD_ABS(mv0[1] - mv1[1]) < 4);
}

// ETM8.0 Reference Modification
static const u8 get_index(const s8 qp, const s8 offset)
{
    return XEVD_CLIP3(0, MAX_QP, qp + offset);
}

static const u8 get_bs(u32 mcu0, u32 x0, u32 y0, u32 mcu1, u32 x1, u32 y1, u32 log2_max_cuwh, s8 *refi0, s8 *refi1
                     , s16(*mv0)[MV_D], s16(*mv1)[MV_D], XEVD_REFP(*refp)[REFP_NUM], u8 ats_present)
{
    u8 bs = DBF_ADDB_BS_OTHERS;
    u8 isIntraBlock = MCU_GET_IF(mcu0) || MCU_GET_IF(mcu1);
    int log2_cuwh = log2_max_cuwh;
    u8 sameXLCU = (x0 >> log2_cuwh) == (x1 >> log2_cuwh);
    u8 sameYLCU = (y0 >> log2_cuwh) == (y1 >> log2_cuwh);
#if TRACE_DBF
    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("Calculate BS. Input params: mcu0 = ");
    XEVD_TRACE_INT_HEX(mcu0);
    XEVD_TRACE_STR(", x0 = ");
    XEVD_TRACE_INT(x0);
    XEVD_TRACE_STR(", y0 = ");
    XEVD_TRACE_INT(y0);
    XEVD_TRACE_STR(", mcu1 = ");
    XEVD_TRACE_INT_HEX(mcu1);
    XEVD_TRACE_STR(", x1 = ");
    XEVD_TRACE_INT(x1);
    XEVD_TRACE_STR(", y1 = ");
    XEVD_TRACE_INT(y1);
    XEVD_TRACE_STR(", log2_max_cuwh = ");
    XEVD_TRACE_INT(log2_max_cuwh);
    XEVD_TRACE_STR(". isIntraBlock = ");
    XEVD_TRACE_INT(isIntraBlock ? 1 : 0);
    XEVD_TRACE_STR(". sameXLCU = ");
    XEVD_TRACE_INT(sameXLCU ? 1 : 0);
    XEVD_TRACE_STR(". sameYLCU = ");
    XEVD_TRACE_INT(sameYLCU ? 1 : 0);
    XEVD_TRACE_STR(". MCU_GET_CBFL(mcu0) = ");
    XEVD_TRACE_INT(MCU_GET_CBFL(mcu0) ? 1 : 0);
    XEVD_TRACE_STR(". MCU_GET_CBFL(mcu1) = ");
    XEVD_TRACE_INT(MCU_GET_CBFL(mcu1) ? 1 : 0);
    XEVD_TRACE_STR(". MCU_GET_IBC(mcu0) = ");
    XEVD_TRACE_INT(MCU_GET_IBC(mcu0) ? 1 : 0);
    XEVD_TRACE_STR(". MCU_GET_IBC(mcu1) = ");
    XEVD_TRACE_INT(MCU_GET_IBC(mcu1) ? 1 : 0);
#endif

    if (isIntraBlock && (!sameXLCU || !sameYLCU) )
    {
        // One of the blocks is Intra and blocks lies in the different LCUs
        bs = DBF_ADDB_BS_INTRA_STRONG;
    }
    else if (isIntraBlock)
    {
        // One of the blocks is Intra
        bs = DBF_ADDB_BS_INTRA;
    }
    else if (MCU_GET_IBC(mcu0) || MCU_GET_IBC(mcu1))
    {
        bs = DBF_ADDB_BS_INTRA;
    }
    else if ((MCU_GET_CBFL(mcu0) == 1 || MCU_GET_CBFL(mcu1) == 1) || ats_present)
    {
        // One of the blocks has coded residuals
        bs = DBF_ADDB_BS_CODED;
    }
    else
    {
        XEVD_PIC *refPics0[2], *refPics1[2];
        refPics0[REFP_0] = (REFI_IS_VALID(refi0[REFP_0])) ? refp[refi0[REFP_0]][REFP_0].pic : NULL;
        refPics0[REFP_1] = (REFI_IS_VALID(refi0[REFP_1])) ? refp[refi0[REFP_1]][REFP_1].pic : NULL;
        refPics1[REFP_0] = (REFI_IS_VALID(refi1[REFP_0])) ? refp[refi1[REFP_0]][REFP_0].pic : NULL;
        refPics1[REFP_1] = (REFI_IS_VALID(refi1[REFP_1])) ? refp[refi1[REFP_1]][REFP_1].pic : NULL;
        int mv0_l0[2] = { mv0[REFP_0][MV_X], mv0[REFP_0][MV_Y] };
        int mv0_l1[2] = { mv0[REFP_1][MV_X], mv0[REFP_1][MV_Y] };
        int mv1_l0[2] = { mv1[REFP_0][MV_X], mv1[REFP_0][MV_Y] };
        int mv1_l1[2] = { mv1[REFP_1][MV_X], mv1[REFP_1][MV_Y] };
#if TRACE_DBF
        XEVD_TRACE_STR(". MV info: refi0[REFP_0] = ");
        XEVD_TRACE_INT(refi0[REFP_0]);
        XEVD_TRACE_STR(", refi0[REFP_1] = ");
        XEVD_TRACE_INT(refi0[REFP_1]);
        XEVD_TRACE_STR(", refi1[REFP_0] = ");
        XEVD_TRACE_INT(refi1[REFP_0]);
        XEVD_TRACE_STR(", refi1[REFP_1] = ");
        XEVD_TRACE_INT(refi1[REFP_1]);
        XEVD_TRACE_STR("; mv0_l0 = {");
        XEVD_TRACE_INT(mv0[REFP_0][MV_X]);
        XEVD_TRACE_STR(", ");
        XEVD_TRACE_INT(mv0[REFP_0][MV_Y]);
        XEVD_TRACE_STR("}, mv0_l1 = {");
        XEVD_TRACE_INT(mv0[REFP_1][MV_X]);
        XEVD_TRACE_STR(", ");
        XEVD_TRACE_INT(mv0[REFP_1][MV_Y]);
        XEVD_TRACE_STR("}, mv1_l0 = {");
        XEVD_TRACE_INT(mv1[REFP_0][MV_X]);
        XEVD_TRACE_STR(", ");
        XEVD_TRACE_INT(mv1[REFP_0][MV_Y]);
        XEVD_TRACE_STR("}, mv1_l1 = {");
        XEVD_TRACE_INT(mv1[REFP_1][MV_X]);
        XEVD_TRACE_STR(", ");
        XEVD_TRACE_INT(mv1[REFP_1][MV_Y]);
        XEVD_TRACE_STR("}");
#endif

        if (!REFI_IS_VALID(refi0[REFP_0]))
        {
            mv0_l0[0] = mv0_l0[1] = 0;
        }

        if (!REFI_IS_VALID(refi0[REFP_1]))
        {
            mv0_l1[0] = mv0_l1[1] = 0;
        }

        if (!REFI_IS_VALID(refi1[REFP_0]))
        {
            mv1_l0[0] = mv1_l0[1] = 0;
        }

        if (!REFI_IS_VALID(refi1[REFP_1]))
        {
            mv1_l1[0] = mv1_l1[1] = 0;
        }


        if ((((refPics0[REFP_0] == refPics1[REFP_0]) && (refPics0[REFP_1] == refPics1[REFP_1])))
            || ((refPics0[REFP_0] == refPics1[REFP_1]) && (refPics0[REFP_1] == refPics1[REFP_0])))
        {
            if (refPics0[REFP_0] == refPics0[REFP_1])
            {
                // Are vectors the same? Yes - 0, otherwise - 1.
                bs = (compare_mvs(mv0_l0, mv1_l0) && compare_mvs(mv0_l1, mv1_l1)
                    && compare_mvs(mv0_l0, mv1_l1) && compare_mvs(mv0_l1, mv1_l0)) ? DBF_ADDB_BS_OTHERS : DBF_ADDB_BS_DIFF_REFS;
            }
            else
            {
                if (((refPics0[REFP_0] == refPics1[REFP_0]) && (refPics0[REFP_1] == refPics1[REFP_1])))
                {
                    bs = (compare_mvs(mv0_l0, mv1_l0) && compare_mvs(mv0_l1, mv1_l1)) ? DBF_ADDB_BS_OTHERS : DBF_ADDB_BS_DIFF_REFS;
                }
                else if ((refPics0[REFP_0] == refPics1[REFP_1]) && (refPics0[REFP_1] == refPics1[REFP_0]))
                {
                    bs = (compare_mvs(mv0_l0, mv1_l1) && compare_mvs(mv0_l1, mv1_l0)) ? DBF_ADDB_BS_OTHERS : DBF_ADDB_BS_DIFF_REFS;
                }
            }
        }
        else
        {
            bs = DBF_ADDB_BS_DIFF_REFS;
        }
    }
#if TRACE_DBF
    XEVD_TRACE_STR(". Answer, bs = ");
    XEVD_TRACE_INT(bs);
    XEVD_TRACE_STR(")\n");
#endif

    return bs;
}

static void deblock_get_pq(pel *buf, int offset, pel* p, pel* q, int size)
{
    // p and q has DBF_LENGTH elements
    u8 i;
    for (i = 0; i < size; ++i)
    {
        q[i] = buf[i * offset];
        p[i] = buf[(i+1) * -offset];
    }
}

static void deblock_set_pq(pel *buf, int offset, pel* p, pel* q, int size)
{
    // p and q has DBF_LENGTH elements
    u8 i;
#if TRACE_DBF
    XEVD_TRACE_STR(" Set (P, Q): ");
#endif
    for (i = 0; i < size; ++i)
    {
        buf[i * offset] = q[i];
        buf[(i + 1) * -offset] = p[i];
#if TRACE_DBF
        if (i != 0)
        {
            XEVD_TRACE_STR(", ");
        }
        XEVD_TRACE_STR("(");
        XEVD_TRACE_INT(q[i]);
        XEVD_TRACE_STR(", ");
        XEVD_TRACE_INT(p[i]);
        XEVD_TRACE_STR(")");
#endif
    }
}
static const u8 deblock_line_apply(pel *p, pel* q, u16 alpha, u8 beta)
{
    return (XEVD_ABS(p[0] - q[0]) < alpha) && (XEVD_ABS(p[1] - p[0]) < beta) && (XEVD_ABS(q[1] - q[0]) < beta);
}

static void deblock_line_chroma_strong(pel* x, pel* y, pel* x_out)
{
    x_out[0] = (2 * x[1] + x[0] + y[1] + 2) >> 2;
}

static void deblock_line_luma_strong(pel* x, pel* y, pel* x_out)
{
    x_out[0] = (x[2] + 2 * (x[1] + x[0] + y[0]) + y[1] + 4) >> 3;
    x_out[1] = (x[2] + x[1] + x[0] + y[0] + 2) >> 2;
    x_out[2] = (2 * x[3] + 3 * x[2] + x[1] + x[0] + y[0] + 4) >> 3;
}
static void deblock_line_check(u16 alpha, u8 beta, pel *p, pel* q, u8 *ap, u8 *aq)
{
    *ap = (XEVD_ABS(p[0] - p[2]) < beta) ? 1 : 0;
    *aq = (XEVD_ABS(q[0] - q[2]) < beta) ? 1 : 0;
}

static pel deblock_line_normal_delta0(u8 c0, pel* p, pel* q)
{
    // This part of code wrote according to AdaptiveDeblocking Filter by P.List, and etc. IEEE transactions on circuits and ... Vol. 13, No. 7, 2003
    // and inconsists with code in JM 19.0
    return XEVD_CLIP3(-(pel)c0, (pel)c0, (4 * (q[0] - p[0]) + p[1] - q[1] + 4) >> 3);
}

static pel deblock_line_normal_delta1(u8 c1, pel* x, pel* y)
{
    return XEVD_CLIP3(-(pel)c1, (pel)c1, ((((x[2] + x[0] + y[0]) * 3) - 8 * x[1] - y[1])) >> 4);
}

static void deblock_scu_line_luma(pel *buf, int stride, u8 bs, u16 alpha, u8 beta, u8 c1, int bit_depth_minus8)
{
    pel p[DBF_LENGTH], q[DBF_LENGTH];
    pel p_out[DBF_LENGTH], q_out[DBF_LENGTH];

    deblock_get_pq(buf, stride, p, q, DBF_LENGTH);
    xevd_mcpy(p_out, p, DBF_LENGTH * sizeof(p[0]));
    xevd_mcpy(q_out, q, DBF_LENGTH * sizeof(q[0]));
#if TRACE_DBF
    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("Process luma line (bs = ");
    XEVD_TRACE_INT(bs);
    XEVD_TRACE_STR(", alpha = ");
    XEVD_TRACE_INT(alpha);
    XEVD_TRACE_STR(", beta = ");
    XEVD_TRACE_INT(beta);
    XEVD_TRACE_STR(", c1 = ");
    XEVD_TRACE_INT(c1);
    XEVD_TRACE_STR("). P = {");
    for (int i = 0; i < DBF_LENGTH; ++i)
    {
        if (i != 0)
        {
            XEVD_TRACE_STR(", ");
        }
        XEVD_TRACE_INT(p[i]);
    }
    XEVD_TRACE_STR("}. Q = {");
    for (int i = 0; i < DBF_LENGTH; ++i)
    {
        if (i != 0)
        {
            XEVD_TRACE_STR(", ");
        }
        XEVD_TRACE_INT(q[i]);
    }
    XEVD_TRACE_STR("}.");
#endif
    if (bs && deblock_line_apply(p, q, alpha, beta))
    {

        u8 ap, aq;
        deblock_line_check(alpha, beta, p, q, &ap, &aq);
#if TRACE_DBF
        XEVD_TRACE_STR(" Ap = ");
        XEVD_TRACE_INT(ap);
        XEVD_TRACE_STR(" Aq = ");
        XEVD_TRACE_INT(aq);
#endif
        if (bs == DBF_ADDB_BS_INTRA_STRONG)
        {
            if (ap && (XEVD_ABS(p[0] - q[0]) < ((alpha >> 2) + 2)))
            {
                deblock_line_luma_strong(p, q, p_out);
            }
            else
            {
                deblock_line_chroma_strong(p, q, p_out);
            }
            if (aq && (XEVD_ABS(p[0] - q[0]) < ((alpha >> 2) + 2)))
            {
                deblock_line_luma_strong(q, p, q_out);
            }
            else
            {
                deblock_line_chroma_strong(q, p, q_out);
            }
        }
        else
        {
            u8 c0;
            pel delta0, delta1;
            int pel_max = (1 << (bit_depth_minus8 + 8)) - 1;
            c0 = c1 + ((ap + aq) << XEVD_MAX(0, (bit_depth_minus8 + 8) - 9));
#if TRACE_DBF
            XEVD_TRACE_STR(" c1 = ");
            XEVD_TRACE_INT(c1);
            XEVD_TRACE_STR(" c0 = ");
            XEVD_TRACE_INT(c0);
#endif

            delta0 = deblock_line_normal_delta0(c0, p, q);
#if TRACE_DBF
            XEVD_TRACE_STR(" delta0 = ");
            XEVD_TRACE_INT(delta0);
#endif
            p_out[0] = XEVD_CLIP3(0, pel_max, p[0] + delta0);
            q_out[0] = XEVD_CLIP3(0, pel_max, q[0] - delta0);
            if (ap)
            {
                delta1 = deblock_line_normal_delta1(c1, p, q);
                p_out[1] = p[1] + delta1;
#if TRACE_DBF
                XEVD_TRACE_STR(" AP_delta1 = ");
                XEVD_TRACE_INT(delta1);
#endif
            }
            if (aq)
            {
                delta1 = deblock_line_normal_delta1(c1, q, p);
                q_out[1] = q[1] + delta1;
#if TRACE_DBF
                XEVD_TRACE_STR(" AQ_delta1 = ");
                XEVD_TRACE_INT(delta1);
#endif
            }
        }
        int pel_max = (1 << (bit_depth_minus8 + 8)) - 1;
        p_out[0] = XEVD_CLIP3(0, pel_max, p_out[0]);
        q_out[0] = XEVD_CLIP3(0, pel_max, q_out[0]);
        p_out[1] = XEVD_CLIP3(0, pel_max, p_out[1]);
        q_out[1] = XEVD_CLIP3(0, pel_max, q_out[1]);
        p_out[2] = XEVD_CLIP3(0, pel_max, p_out[2]);
        q_out[2] = XEVD_CLIP3(0, pel_max, q_out[2]);
        p_out[3] = XEVD_CLIP3(0, pel_max, p_out[3]);
        q_out[3] = XEVD_CLIP3(0, pel_max, q_out[3]);
        deblock_set_pq(buf, stride, p_out, q_out, DBF_LENGTH);
    }
#if TRACE_DBF
    else
    {
        XEVD_TRACE_STR("Line won't processed");
    }
    XEVD_TRACE_STR("\n");
#endif
}
static void deblock_scu_line_chroma(pel *buf, int stride, u8 bs, u16 alpha, u8 beta, u8 c0, int bit_depth_minus8)
{
    pel p[DBF_LENGTH_CHROMA], q[DBF_LENGTH_CHROMA];
    pel p_out[DBF_LENGTH_CHROMA], q_out[DBF_LENGTH_CHROMA];

    deblock_get_pq(buf, stride, p, q, DBF_LENGTH_CHROMA);
    xevd_mcpy(p_out, p, DBF_LENGTH_CHROMA * sizeof(p[0]));
    xevd_mcpy(q_out, q, DBF_LENGTH_CHROMA * sizeof(q[0]));
#if TRACE_DBF
    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("Process chroma line (bs = ");
    XEVD_TRACE_INT(bs);
    XEVD_TRACE_STR(", alpha = ");
    XEVD_TRACE_INT(alpha);
    XEVD_TRACE_STR(", beta = ");
    XEVD_TRACE_INT(beta);
    XEVD_TRACE_STR(", c0 = ");
    XEVD_TRACE_INT(c0);
    XEVD_TRACE_STR("). P = {");
    for (int i = 0; i < DBF_LENGTH_CHROMA; ++i)
    {
        if (i != 0)
        {
            XEVD_TRACE_STR(", ");
        }
        XEVD_TRACE_INT(p[i]);
    }
    XEVD_TRACE_STR("}. Q = {");
    for (int i = 0; i < DBF_LENGTH_CHROMA; ++i)
    {
        if (i != 0)
        {
            XEVD_TRACE_STR(", ");
        }
        XEVD_TRACE_INT(q[i]);
    }
    XEVD_TRACE_STR("}.");
#endif
    if (bs && deblock_line_apply(p, q, alpha, beta))
    {
        if (bs == DBF_ADDB_BS_INTRA_STRONG)
        {
            deblock_line_chroma_strong(p, q, p_out);
            deblock_line_chroma_strong(q, p, q_out);
        }
        else
        {
            pel delta0;
            int pel_max = (1 << (bit_depth_minus8+8)) - 1;
            delta0 = deblock_line_normal_delta0(c0, p, q);
            p_out[0] = XEVD_CLIP3(0, pel_max, p[0] + delta0);
            q_out[0] = XEVD_CLIP3(0, pel_max, q[0] - delta0);
#if TRACE_DBF
            XEVD_TRACE_STR(" delta0 = ");
            XEVD_TRACE_INT(delta0);
#endif
        }
        int pel_max = (1 << (bit_depth_minus8 + 8)) - 1;
        p_out[0] = XEVD_CLIP3(0, pel_max, p_out[0]);
        q_out[0] = XEVD_CLIP3(0, pel_max, q_out[0]);
        p_out[1] = XEVD_CLIP3(0, pel_max, p_out[1]);
        q_out[1] = XEVD_CLIP3(0, pel_max, q_out[1]);
        deblock_set_pq(buf, stride, p_out, q_out, DBF_LENGTH_CHROMA);
    }
#if TRACE_DBF
    else
    {
        XEVD_TRACE_STR("Line won't processed");
    }
    XEVD_TRACE_STR("\n");
#endif
}
static void deblock_scu_addb_ver_luma(pel *buf, int stride, u8 bs, u16 alpha, u8 beta, u8 c1, int bit_depth_minus8)
{
    u8 i;
    pel *cur_buf = buf;
    for (i = 0; i < MIN_CU_SIZE; ++i, cur_buf += stride)
    {
        deblock_scu_line_luma(cur_buf, 1, bs, alpha, beta, c1, bit_depth_minus8);
    }
}
static void deblock_scu_addb_hor_luma(pel *buf, int stride, u8 bs, u16 alpha, u8 beta, u8 c1, int bit_depth_minus8)
{
    u8 i;
    pel *cur_buf = buf;
    for (i = 0; i < MIN_CU_SIZE; ++i, ++cur_buf)
    {
        deblock_scu_line_luma(cur_buf, stride, bs, alpha, beta, c1, bit_depth_minus8);
    }
}
static void deblock_scu_addb_ver_chroma(pel *buf, int stride, u8 bs, u16 alpha, u8 beta, u8 c0, int bit_depth_minus8, int chroma_format_idc)
{
    u8 i;
    pel *cur_buf = buf;
    int size = (MIN_CU_SIZE >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)));
    for(i = 0; i <size; ++i, cur_buf += stride)
    {
        deblock_scu_line_chroma(cur_buf, 1, bs, alpha, beta, c0, bit_depth_minus8);
    }
}
static void deblock_scu_addb_hor_chroma(pel *buf, int stride, u8 bs, u16 alpha, u8 beta, u8 c0, int bit_depth_minus8, int chroma_format_idc)
{
    u8 i;
    pel *cur_buf = buf;
    int size = (MIN_CU_SIZE >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)));
    for(i = 0; i < size; ++i, ++cur_buf)
    {
        deblock_scu_line_chroma(cur_buf, stride, bs, alpha, beta, c0, bit_depth_minus8);
    }
}

static u32* deblock_set_coded_block(u32* map_scu, int w, int h, int w_scu)
{
    int i, j;
    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            MCU_SET_COD(map_scu[j]);
        }
        map_scu += w_scu;
    }
    return map_scu;
}

static void deblock_addb_cu_hor(XEVD_PIC *pic, int x_pel, int y_pel, int cuw, int cuh, u32 *map_scu, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D]
                             , int w_scu, int log2_max_cuwh, XEVD_REFP(*refp)[REFP_NUM], int ats_inter_mode, TREE_CONS tree_cons, u8* map_tidx
                             , int boundary_filtering, u8* map_ats_inter, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc)
{
    pel       * y, *u, *v;
    int         i, t, qp, s_l, s_c;
    int         w = cuw >> MIN_CU_LOG2;
    int         h = cuh >> MIN_CU_LOG2;
    u8          indexA, indexB;
    u16         alpha;
    u8          beta;
    u8          c0, c1;
    u32       * map_scu_tmp;
    int bitdepth_scale = (bit_depth_luma - 8);

    int align_8_8_grid = 0;
    int   w_shift = XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc);
    int   h_shift = XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc);
    if (y_pel % 8 == 0)
    {
        align_8_8_grid = 1;
    }
    int  t1, t_copy; // Next row scu number
    t = (x_pel >> MIN_CU_LOG2) + (y_pel >> MIN_CU_LOG2) * w_scu;
    t_copy = t;
    t1 = (x_pel >> MIN_CU_LOG2) + ((y_pel - (1 << MIN_CU_LOG2)) >> MIN_CU_LOG2) * w_scu;

    map_scu += t;
    map_refi += t;
    map_mv += t;
    map_ats_inter += t;
    map_scu_tmp = map_scu;
    s_l = pic->s_l;
    s_c = pic->s_c;
    y = pic->y + x_pel + y_pel * s_l;
    t = (x_pel >> w_shift) + (y_pel >> h_shift) * s_c;
    u = pic->u + t;
    v = pic->v + t;

    int no_boundary = 0;
    if (y_pel > 0)
    {
        no_boundary = (map_tidx[t_copy] == map_tidx[t1]) || boundary_filtering;
    }

    if (align_8_8_grid  && y_pel > 0 && (no_boundary))
    {

        for (i = 0; i < (cuw >> MIN_CU_LOG2); ++i)
        {
#if TRACE_DBF
            XEVD_TRACE_COUNTER;
            XEVD_TRACE_STR("Start filtering hor boundary of SCU (");
            XEVD_TRACE_INT(x_pel);
            XEVD_TRACE_STR(", ");
            XEVD_TRACE_INT(y_pel);
            XEVD_TRACE_STR(") ats_inter_mode = ");
            XEVD_TRACE_INT(ats_inter_mode);
            XEVD_TRACE_STR(" tree_type = ");
            XEVD_TRACE_INT(tree_cons.tree_type);
            XEVD_TRACE_STR(" mode_cons = ");
            XEVD_TRACE_INT(tree_cons.mode_cons);
            XEVD_TRACE_STR("\n");
#endif

            t = (i << MIN_CU_LOG2);
            int cur_x_pel = x_pel + t;
            u8 current_ats = map_ats_inter[i];
            u8 neighbor_ats = map_ats_inter[i - w_scu];
            u8 ats_present = current_ats || neighbor_ats;
            u8 bs_cur = get_bs(map_scu[i], cur_x_pel, y_pel, map_scu[i - w_scu], cur_x_pel, y_pel - 1, log2_max_cuwh, map_refi[i]
                             , map_refi[i - w_scu], map_mv[i], map_mv[i - w_scu], refp, ats_present);
            qp = (MCU_GET_QP(map_scu[i]) + MCU_GET_QP(map_scu[i - w_scu]) + 1) >> 1;

            indexA = get_index(qp, pic->pic_deblock_alpha_offset);            //! \todo Add offset for IndexA
            indexB = get_index(qp, pic->pic_deblock_beta_offset);            //! \todo Add offset for IndexB

            alpha = ALPHA_TABLE[indexA] << bitdepth_scale;
            beta = BETA_TABLE[indexB] << bitdepth_scale;
            c1 = CLIP_TAB[indexA][bs_cur] << XEVD_MAX(0, (bit_depth_luma - 9));

            if (xevd_check_luma_fn(tree_cons))
            {
                deblock_scu_addb_hor_luma(y + t, s_l, bs_cur, alpha, beta, c1, bit_depth_luma - 8);
            }
            if(xevd_check_chroma_fn(tree_cons) && (chroma_format_idc != 0))
            {
                t >>= w_shift;
                int qp_u = XEVD_CLIP3(-6 * (bit_depth_chroma - 8), 57, qp + pic->pic_qp_u_offset);
                indexA = get_index(xevd_qp_chroma_dynamic[0][qp_u], pic->pic_deblock_alpha_offset);
                indexB = get_index(xevd_qp_chroma_dynamic[0][qp_u], pic->pic_deblock_beta_offset);
                alpha = ALPHA_TABLE[indexA] << bitdepth_scale;
                beta = BETA_TABLE[indexB] << bitdepth_scale;
                c1 = CLIP_TAB[indexA][bs_cur];
                c0 = (c1 + 1) << XEVD_MAX(0, (bit_depth_chroma - 9));
                deblock_scu_addb_hor_chroma(u + t, s_c, bs_cur, alpha, beta, c0, bit_depth_chroma - 8, chroma_format_idc);

                int qp_v = XEVD_CLIP3(-6 * (bit_depth_chroma - 8), 57, qp + pic->pic_qp_v_offset);
                indexA = get_index(xevd_qp_chroma_dynamic[1][qp_v], pic->pic_deblock_alpha_offset);
                indexB = get_index(xevd_qp_chroma_dynamic[1][qp_v], pic->pic_deblock_beta_offset);
                alpha = ALPHA_TABLE[indexA] << bitdepth_scale;
                beta = BETA_TABLE[indexB] << bitdepth_scale;
                c1 = CLIP_TAB[indexA][bs_cur];
                c0 = (c1 + 1) << XEVD_MAX(0, (bit_depth_chroma - 9));
                deblock_scu_addb_hor_chroma(v + t, s_c, bs_cur, alpha, beta, c0, bit_depth_chroma - 8, chroma_format_idc);
            }
        }
    }

    map_scu = deblock_set_coded_block(map_scu_tmp, w, h, w_scu);
}

static void deblock_addb_cu_ver_yuv(XEVD_PIC *pic, int x_pel, int y_pel, int log2_max_cuwh, pel *y, pel* u, pel *v, int s_l, int s_c, int cuh
                                  , u32 *map_scu, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], int w_scu, XEVD_REFP(*refp)[REFP_NUM]
                                  , int ats_inter_mode, TREE_CONS tree_cons, u8* map_ats_inter, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc)
{
    int i, qp;
    int h = cuh >> MIN_CU_LOG2;
    u8 indexA, indexB;
    u16 alpha;
    u8 beta;
    u8 c0, c1;
    const int bitdepth_scale = (bit_depth_luma - 8);
    for (i = 0; i < h; i++)
    {
#if TRACE_DBF
        XEVD_TRACE_COUNTER;
        XEVD_TRACE_STR("Start filtering ver boundary of SCU (");
        XEVD_TRACE_INT(x_pel);
        XEVD_TRACE_STR(", ");
        XEVD_TRACE_INT(y_pel);
        XEVD_TRACE_STR(") ats_inter_mode = ");
        XEVD_TRACE_INT(ats_inter_mode);
        XEVD_TRACE_STR(" tree_type = ");
        XEVD_TRACE_INT(tree_cons.tree_type);
        XEVD_TRACE_STR(" mode_cons = ");
        XEVD_TRACE_INT(tree_cons.mode_cons);
        XEVD_TRACE_STR("\n");
#endif

        {
            int cur_y_pel = y_pel + (i << MIN_CU_LOG2);
            u8 current_ats = map_ats_inter[0];
            u8 neighbor_ats = map_ats_inter[-1];
            u8 ats_present = current_ats || neighbor_ats;
            u8 bs_cur = get_bs(map_scu[0], x_pel, cur_y_pel, map_scu[-1], x_pel - 1, cur_y_pel, log2_max_cuwh
                             , map_refi[0], map_refi[-1], map_mv[0], map_mv[-1], refp, ats_present);
            qp = (MCU_GET_QP(map_scu[0]) + MCU_GET_QP(map_scu[-1]) + 1) >> 1;

            //neb_w = 1 << MCU_GET_LOGW(map_cu[-1]);

            if (xevd_check_luma_fn(tree_cons))
            {
                indexA = get_index(qp, pic->pic_deblock_alpha_offset);            //! \todo Add offset for IndexA
                indexB = get_index(qp, pic->pic_deblock_beta_offset);            //! \todo Add offset for IndexB

                alpha = ALPHA_TABLE[indexA] << bitdepth_scale;
                beta = BETA_TABLE[indexB] << bitdepth_scale;
                c1 = CLIP_TAB[indexA][bs_cur] << XEVD_MAX(0, (bit_depth_luma - 9));

                deblock_scu_addb_ver_luma(y, s_l, bs_cur, alpha, beta, c1, bit_depth_luma - 8);
            }
            if(xevd_check_chroma_fn(tree_cons) && (chroma_format_idc != 0))
            {
                int qp_u = XEVD_CLIP3(-6 * (bit_depth_chroma - 8), 57, qp + pic->pic_qp_u_offset);
                indexA = get_index(xevd_qp_chroma_dynamic[0][qp_u], pic->pic_deblock_alpha_offset);
                indexB = get_index(xevd_qp_chroma_dynamic[0][qp_u], pic->pic_deblock_beta_offset);

                alpha = ALPHA_TABLE[indexA] << bitdepth_scale;
                beta = BETA_TABLE[indexB] << bitdepth_scale;

                c1 = CLIP_TAB[indexA][bs_cur];
                c0 = (c1 + 1) << XEVD_MAX(0, (bit_depth_chroma - 9));
                deblock_scu_addb_ver_chroma(u, s_c, bs_cur, alpha, beta, c0, bit_depth_chroma - 8, chroma_format_idc);

                int qp_v = XEVD_CLIP3(-6 * (bit_depth_chroma - 8), 57, qp + pic->pic_qp_v_offset);
                indexA = get_index(xevd_qp_chroma_dynamic[1][qp_v], pic->pic_deblock_alpha_offset);
                indexB = get_index(xevd_qp_chroma_dynamic[1][qp_v], pic->pic_deblock_beta_offset);

                alpha = ALPHA_TABLE[indexA] << bitdepth_scale;
                beta = BETA_TABLE[indexB] << bitdepth_scale;

                c1 = CLIP_TAB[indexA][bs_cur];
                c0 = (c1 + 1) << XEVD_MAX(0, (bit_depth_chroma - 9));

                deblock_scu_addb_ver_chroma(v, s_c, bs_cur, alpha, beta, c0, bit_depth_chroma - 8, chroma_format_idc);
            }

            y += (s_l << MIN_CU_LOG2);
            u += (s_c << (MIN_CU_LOG2 - (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))));
            v += (s_c << (MIN_CU_LOG2 - (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))));
            map_scu += w_scu;
            map_refi += w_scu;
            map_mv += w_scu;
            map_ats_inter += w_scu;
            //map_cu += w_scu;
        }
    }

}

static void deblock_addb_cu_ver(XEVD_PIC *pic, int x_pel, int y_pel, int cuw, int cuh, u32 *map_scu, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D]
                              , int w_scu, int log2_max_cuwh, u32  *map_cu, XEVD_REFP(*refp)[REFP_NUM], int ats_inter_mode, TREE_CONS tree_cons, u8* map_tidx
                              , int boundary_filtering, u8* map_ats_inter, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc)
{
    pel       * y, *u, *v;
    int         t, s_l, s_c;
    int w = cuw >> MIN_CU_LOG2;
    int h = cuh >> MIN_CU_LOG2;
    u32 *map_scu_tmp;
    s8(*map_refi_tmp)[REFP_NUM];
    s16(*map_mv_tmp)[REFP_NUM][MV_D];
    u8 *map_ats_inter_tmp;
    //int neb_w;
    u32  *map_cu_tmp;
    int align_8_8_grid = 0;
    int    w_shift = XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc);
    int    h_shift = XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc);

    if (x_pel % 8 == 0)
    {
        align_8_8_grid = 1;
    }
    int  t1, t2, t_copy; // Next row scu number
    t = (x_pel >> MIN_CU_LOG2) + (y_pel >> MIN_CU_LOG2) * w_scu;
    t_copy = t;

    t1 = ((x_pel - (1 << MIN_CU_LOG2)) >> MIN_CU_LOG2) + (y_pel >> MIN_CU_LOG2) * w_scu;
    t2 = ((x_pel + (w << MIN_CU_LOG2)) >> MIN_CU_LOG2) + (y_pel >> MIN_CU_LOG2) * w_scu;

    map_scu += t;
    map_refi += t;
    map_mv += t;
    map_ats_inter +=t;
    map_cu += t;

    s_l = pic->s_l;
    s_c = pic->s_c;
    y = pic->y + x_pel + y_pel * s_l;
    t = (x_pel >> w_shift) + (y_pel >> h_shift) * s_c;
    u = pic->u + t;
    v = pic->v + t;

    map_scu_tmp = map_scu;
    map_refi_tmp = map_refi;
    map_mv_tmp = map_mv;
    map_ats_inter_tmp = map_ats_inter;
    map_cu_tmp = map_cu;

    /* vertical filtering */
    int no_boundary = 0;
    if (x_pel > 0)
    {
        no_boundary = (map_tidx[t_copy] == map_tidx[t1]) || boundary_filtering;
    }

    if (align_8_8_grid && x_pel > 0 && MCU_GET_COD(map_scu[-1]) && (no_boundary))
    {
        deblock_addb_cu_ver_yuv(pic, x_pel, y_pel, log2_max_cuwh, y, u, v, s_l, s_c, cuh, map_scu, map_refi, map_mv, w_scu, refp, ats_inter_mode
                              , tree_cons, map_ats_inter, bit_depth_luma, bit_depth_chroma, chroma_format_idc);
    }

    map_scu = map_scu_tmp;
    map_refi = map_refi_tmp;
    map_mv = map_mv_tmp;
    map_ats_inter = map_ats_inter_tmp;
    map_cu = map_cu_tmp;

    no_boundary = 0;
    if (x_pel + cuw < pic->w_l)
    {
        no_boundary = (map_tidx[t_copy] == map_tidx[t2]) || boundary_filtering;
    }

    if ((x_pel + cuw) % 8 == 0)
    {
        align_8_8_grid = 1;
    }
    else
    {
        align_8_8_grid = 0;
    }
    if (align_8_8_grid && x_pel + cuw < pic->w_l && MCU_GET_COD(map_scu[w]) && (no_boundary))
    {
        y = pic->y + x_pel + y_pel * s_l;
        u = pic->u + t;
        v = pic->v + t;

        y += cuw;
        u += (cuw >> w_shift);
        v += (cuw >> w_shift);
        map_scu += w;
        map_refi += w;
        map_mv += w;
        map_ats_inter += w;
        deblock_addb_cu_ver_yuv(pic, x_pel + cuw, y_pel, log2_max_cuwh, y, u, v, s_l, s_c, cuh, map_scu, map_refi, map_mv, w_scu, refp, ats_inter_mode
                              , tree_cons, map_ats_inter, bit_depth_luma, bit_depth_chroma, chroma_format_idc);
    }

    map_scu = deblock_set_coded_block(map_scu_tmp, w, h, w_scu);
}

void xevdm_deblock_cu_hor(  XEVD_CTX *ctx, XEVD_PIC *pic, int x_pel, int y_pel, int cuw, int cuh, u32 *map_scu, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D]
                       , int w_scu, int log2_max_cuwh, XEVD_REFP(*refp)[REFP_NUM], int ats_inter_mode, TREE_CONS tree_cons, u8* map_tidx
                        , int boundary_filtering, int tool_addb, u8* map_ats_inter, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc)

  
{
    if (ctx->sps->tool_addb)
    {
        deblock_addb_cu_hor(pic, x_pel, y_pel, cuw, cuh, map_scu, map_refi, map_mv, w_scu, log2_max_cuwh, refp, ats_inter_mode, tree_cons
                           , map_tidx, boundary_filtering, map_ats_inter, bit_depth_luma, bit_depth_chroma, chroma_format_idc);
    }
    else
    {
        deblock_cu_hor(ctx, pic, x_pel, y_pel, cuw, cuh, tree_cons, boundary_filtering);
    }
}

void xevdm_deblock_cu_ver(XEVD_CTX *ctx, XEVD_PIC *pic, int x_pel, int y_pel, int cuw, int cuh, u32 *map_scu, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D]
                        , int w_scu, int log2_max_cuwh, u32  *map_cu, XEVD_REFP(*refp)[REFP_NUM], int ats_inter_mode, TREE_CONS tree_cons, u8* map_tidx
                        , int boundary_filtering, int tool_addb, u8* map_ats_inter, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc)
{
    if (tool_addb)
    {
        deblock_addb_cu_ver(pic, x_pel, y_pel, cuw, cuh, map_scu, map_refi, map_mv, w_scu, log2_max_cuwh, map_cu, refp, ats_inter_mode
                          , tree_cons, map_tidx, boundary_filtering, map_ats_inter, bit_depth_luma, bit_depth_chroma, chroma_format_idc);
    }
    else
    {
        deblock_cu_ver(ctx, pic, x_pel, y_pel, cuw, cuh, tree_cons, boundary_filtering);
    }
}

