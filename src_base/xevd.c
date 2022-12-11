
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

#include <math.h>
#include "xevd_def.h"



/* convert XEVD into XEVD_CTX */
#define XEVD_ID_TO_CTX_R(id, ctx) \
    xevd_assert_r((id)); \
    (ctx) = (XEVD_CTX *)id; \
    xevd_assert_r((ctx)->magic == XEVD_MAGIC_CODE);

/* convert XEVD into XEVD_CTX with return value if assert on */
#define XEVD_ID_TO_CTX_RV(id, ctx, ret) \
    xevd_assert_rv((id), (ret)); \
    (ctx) = (XEVD_CTX *)id; \
    xevd_assert_rv((ctx)->magic == XEVD_MAGIC_CODE, (ret));

static XEVD_CTX * ctx_alloc(void)
{
    XEVD_CTX * ctx;

    ctx = (XEVD_CTX*)xevd_malloc_fast(sizeof(XEVD_CTX));

    xevd_assert_rv(ctx != NULL, NULL);
    xevd_mset_x64a(ctx, 0, sizeof(XEVD_CTX));

    /* set default value */
    ctx->pic_cnt       = 0;

    return ctx;
}

static void ctx_free(XEVD_CTX * ctx)
{
    xevd_mfree_fast(ctx);
}

static XEVD_CORE * core_alloc(void)
{
    XEVD_CORE * core;

    core = (XEVD_CORE*)xevd_malloc_fast(sizeof(XEVD_CORE));

    xevd_assert_rv(core, NULL);
    xevd_mset_x64a(core, 0, sizeof(XEVD_CORE));

    return core;
}

static void core_free(XEVD_CORE * core)
{
    xevd_mfree_fast(core);
}


void xevd_free_1d(void** dst, int size)
{
    xevd_mfree_fast(*dst);
}

void xevd_free_2d(s8*** dst, int size_1d, int size_2d, int type_size)
{
    int i;

    xevd_mfree_fast((*dst)[0]);
    xevd_mfree_fast(*dst);
}

int xevd_delete_cu_data(XEVD_CU_DATA* cu_data, int log2_cuw, int log2_cuh)
{
    int i, j;
    int cuw_scu, cuh_scu;
    int size_8b, size_16b, size_32b, cu_cnt, pixel_cnt;

    cuw_scu = 1 << log2_cuw;
    cuh_scu = 1 << log2_cuh;

    size_8b = cuw_scu * cuh_scu * sizeof(s8);
    size_16b = cuw_scu * cuh_scu * sizeof(s16);
    size_32b = cuw_scu * cuh_scu * sizeof(s32);
    cu_cnt = cuw_scu * cuh_scu;
    pixel_cnt = cu_cnt << 4;

    xevd_free_1d((void**)&cu_data->qp_y, size_8b);
    xevd_free_1d((void**)&cu_data->qp_u, size_8b);
    xevd_free_1d((void**)&cu_data->qp_v, size_8b);
    xevd_free_1d((void**)&cu_data->pred_mode, size_8b);
    xevd_free_1d((void**)&cu_data->pred_mode_chroma, size_8b);

    xevd_free_2d((s8***)&cu_data->mpm, 2, cu_cnt, sizeof(u8));
    xevd_free_2d((s8***)&cu_data->ipm, 2, cu_cnt, sizeof(u8));
    xevd_free_2d((s8***)&cu_data->mpm_ext, 8, cu_cnt, sizeof(u8));
    xevd_free_1d((void**)&cu_data->skip_flag, size_8b);
    xevd_free_1d((void**)&cu_data->ibc_flag, size_8b);
    xevd_free_1d((void**)&cu_data->dmvr_flag, size_8b);

    xevd_free_2d((s8***)&cu_data->refi, cu_cnt, REFP_NUM, sizeof(u8));
    xevd_free_2d((s8***)&cu_data->mvp_idx, cu_cnt, REFP_NUM, sizeof(u8));
    xevd_free_1d((void**)&cu_data->mvr_idx, size_8b);
    xevd_free_1d((void**)&cu_data->bi_idx, size_8b);
    xevd_free_1d((void**)&cu_data->inter_dir, size_8b);
    xevd_free_1d((void**)&cu_data->mmvd_idx, size_16b);
    xevd_free_1d((void**)&cu_data->mmvd_flag, size_8b);

    xevd_free_1d((void**)&cu_data->ats_intra_cu, size_8b);
    xevd_free_1d((void**)&cu_data->ats_mode_h, size_8b);
    xevd_free_1d((void**)&cu_data->ats_mode_v, size_8b);

    xevd_free_1d((void**)&cu_data->ats_inter_info, size_8b);

    for (i = 0; i < N_C; i++)
    {
        xevd_free_1d((void**)&cu_data->nnz[i], size_32b);
    }
    for (i = 0; i < N_C; i++)
    {
        for (j = 0; j < 4; j++)
        {
            xevd_free_1d((void**)&cu_data->nnz_sub[i][j], size_32b);
        }
    }
    xevd_free_1d((void**)&cu_data->map_scu, size_32b);
    xevd_free_1d((void**)&cu_data->affine_flag, size_8b);
    xevd_free_1d((void**)&cu_data->map_affine, size_32b);
    xevd_free_1d((void**)&cu_data->map_cu_mode, size_32b);
    xevd_free_1d((void**)&cu_data->depth, size_8b);

    for (i = 0; i < N_C; i++)
    {
        xevd_free_1d((void**)&cu_data->coef[i], (pixel_cnt >> (!!(i) * 2)) * sizeof(s16));
        xevd_free_1d((void**)&cu_data->reco[i], (pixel_cnt >> (!!(i) * 2)) * sizeof(pel));
    }

    return XEVD_OK;
}

static void sequence_deinit(XEVD_CTX * ctx)
{
    xevd_mfree(ctx->map_scu);
    xevd_mfree(ctx->cod_eco);
    xevd_mfree(ctx->map_split);
    xevd_mfree(ctx->map_ipm);
    xevd_mfree(ctx->map_cu_mode);

    for (int i = 0; i < (int)ctx->f_lcu; i++)
    {
        xevd_delete_cu_data(ctx->map_cu_data + i, ctx->log2_max_cuwh - MIN_CU_LOG2, ctx->log2_max_cuwh - MIN_CU_LOG2);
    }

    xevd_mfree(ctx->map_cu_data);
    xevd_mfree(ctx->tile);
    xevd_mfree_fast(ctx->map_tidx);
    xevd_picman_deinit(&ctx->dpm);
    free((void*)ctx->sync_flag);
	  ctx->sync_flag = NULL;
    free((void*)ctx->sync_row);
    ctx->sync_row =  NULL;
}

int xevd_create_cu_data(XEVD_CU_DATA *cu_data, int log2_cuw, int log2_cuh);

static int picture_init(XEVD_CTX * ctx)
{
    ctx->w_tile = 1;
    ctx->tile_cnt =1;

    if (ctx->tc.task_num_in_tile == NULL)
    {
        ctx->tc.task_num_in_tile = (int*)xevd_malloc(sizeof(int) * ctx->tile_cnt);
    }
    else
    {
        xevd_mfree(ctx->tc.task_num_in_tile);
        ctx->tc.task_num_in_tile = (int*)xevd_malloc(sizeof(int) * ctx->tile_cnt);
    }

    if (ctx->tile == NULL)
    {
        int size = sizeof(XEVD_TILE) * ctx->tile_cnt;
        ctx->tile = xevd_malloc(size);
        xevd_assert_rv(ctx->tile, XEVD_ERR_OUT_OF_MEMORY);
    }
    else
    {
        xevd_mfree(ctx->tile);
        int size = sizeof(XEVD_TILE) * ctx->tile_cnt;
        ctx->tile = xevd_malloc(size);
        xevd_assert_rv(ctx->tile, XEVD_ERR_OUT_OF_MEMORY);
    }

    if (ctx->tc.max_task_cnt > 1)
    {
        ctx->tc.tile_task_num = 1;
        ctx->tc.task_num_in_tile[0] = ctx->tc.max_task_cnt;
    }
    else
    {
        ctx->tc.tile_task_num = 1;
        for (u32 i = 0; i < ctx->tile_cnt; i++)
        {
            ctx->tc.task_num_in_tile[i] = 1;
        }
    }

    return XEVD_OK;
}

static int sequence_init(XEVD_CTX * ctx, XEVD_SPS * sps)
{
    int size;
    int ret;

    if(sps->pic_width_in_luma_samples != ctx->w || sps->pic_height_in_luma_samples != ctx->h)
    {
        /* resolution was changed */
        sequence_deinit(ctx);

        ctx->w = sps->pic_width_in_luma_samples;
        ctx->h = sps->pic_height_in_luma_samples;
        ctx->max_cuwh = 1 << 6;
        ctx->min_cuwh = 1 << 2;
        ctx->log2_max_cuwh = XEVD_CONV_LOG2(ctx->max_cuwh);
        ctx->log2_min_cuwh = XEVD_CONV_LOG2(ctx->min_cuwh);
    }

    size = ctx->max_cuwh;
    ctx->w_lcu = (ctx->w + (size - 1)) / size;
    ctx->h_lcu = (ctx->h + (size - 1)) / size;
    ctx->f_lcu = ctx->w_lcu * ctx->h_lcu;
    ctx->w_scu = (ctx->w + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    ctx->h_scu = (ctx->h + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    ctx->f_scu = ctx->w_scu * ctx->h_scu;

    ctx->internal_codec_bit_depth = sps->bit_depth_luma_minus8 + 8;
    ctx->internal_codec_bit_depth_luma = sps->bit_depth_luma_minus8 + 8;
    ctx->internal_codec_bit_depth_chroma = sps->bit_depth_chroma_minus8 + 8;

    /* alloc SCU map */
    if(ctx->map_scu == NULL)
    {
        size = sizeof(u32) * ctx->f_scu;
        ctx->map_scu = (u32 *)xevd_malloc(size);
        xevd_assert_gv(ctx->map_scu, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(ctx->map_scu, 0, size);
    }

    if (ctx->cod_eco == NULL)
    {
        size = sizeof(u8) * ctx->f_scu;
        ctx->cod_eco = (u8 *)xevd_malloc(size);
        xevd_assert_gv(ctx->cod_eco, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(ctx->cod_eco, 0, size);
    }
    /* alloc cu mode SCU map */
    if(ctx->map_cu_mode == NULL)
    {
        size = sizeof(u32) * ctx->f_scu;
        ctx->map_cu_mode = (u32 *)xevd_malloc(size);
        xevd_assert_gv(ctx->map_cu_mode, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(ctx->map_cu_mode, 0, size);
    }

    /* alloc map for CU split flag */
    if(ctx->map_split == NULL)
    {
        size = sizeof(s8) * ctx->f_lcu * NUM_CU_DEPTH * NUM_BLOCK_SHAPE * MAX_CU_CNT_IN_LCU;
        ctx->map_split = xevd_malloc(size);
        xevd_assert_gv(ctx->map_split, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(ctx->map_split, 0, size);
    }

    /* alloc map for intra prediction mode */
    if(ctx->map_ipm == NULL)
    {
        size = sizeof(s8) * ctx->f_scu;
        ctx->map_ipm = (s8 *)xevd_malloc(size);
        xevd_assert_gv(ctx->map_ipm, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(ctx->map_ipm, -1, size);
    }

    /* alloc tile index map in SCU unit */
    if (ctx->map_tidx == NULL)
    {
        size = sizeof(u8) * ctx->f_scu;
        ctx->map_tidx = (u8 *)xevd_malloc(size);
        xevd_assert_gv(ctx->map_tidx, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(ctx->map_tidx, 0, size);
    }

    if(ctx->map_cu_data == NULL)
    {
        size = sizeof(XEVD_CU_DATA) * ctx->f_lcu;
        ctx->map_cu_data = (XEVD_CU_DATA*)xevd_malloc_fast(size);
        xevd_assert_gv(ctx->map_cu_data, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(ctx->map_cu_data, 0, size);
        for(int i = 0; i < (int)ctx->f_lcu; i++)
        {
            xevd_create_cu_data(ctx->map_cu_data + i, ctx->log2_max_cuwh - MIN_CU_LOG2, ctx->log2_max_cuwh - MIN_CU_LOG2);
        }
    }

    /* initialize reference picture manager */
    ctx->pa.fn_alloc = xevd_picbuf_alloc;
    ctx->pa.fn_free = xevd_picbuf_free;
    ctx->pa.w = ctx->w;
    ctx->pa.h = ctx->h;
    ctx->pa.pad_l = PIC_PAD_SIZE_L;
    ctx->pa.pad_c = PIC_PAD_SIZE_L >> (XEVD_GET_CHROMA_H_SHIFT(sps->chroma_format_idc));
    ctx->ref_pic_gap_length = (int)pow(2.0, sps->log2_ref_pic_gap_length);
    ctx->pa.idc = sps->chroma_format_idc;

    ret = xevd_picman_init(&ctx->dpm, MAX_PB_SIZE, XEVD_MAX_NUM_REF_PICS, &ctx->pa);
    xevd_assert_g(XEVD_SUCCEEDED(ret), ERR);

    xevd_set_chroma_qp_tbl_loc(sps->bit_depth_luma_minus8 + 8);
    xevd_tbl_qp_chroma_adjust = xevd_tbl_qp_chroma_adjust_base;

    if (sps->chroma_qp_table_struct.chroma_qp_table_present_flag)
    {
        xevd_derived_chroma_qp_mapping_tables(&(sps->chroma_qp_table_struct), sps->bit_depth_chroma_minus8 + 8);
    }
    else
    {
        xevd_mcpy(&(xevd_tbl_qp_chroma_dynamic_ext[0][6 * sps->bit_depth_chroma_minus8]), xevd_tbl_qp_chroma_adjust, XEVD_MAX_QP_TABLE_SIZE * sizeof(int));
        xevd_mcpy(&(xevd_tbl_qp_chroma_dynamic_ext[1][6 * sps->bit_depth_chroma_minus8]), xevd_tbl_qp_chroma_adjust, XEVD_MAX_QP_TABLE_SIZE * sizeof(int));
    }

    ctx->sync_flag = (volatile s32 *)xevd_malloc(ctx->f_lcu * sizeof(int));
    xevd_mset((void *)ctx->sync_flag, 0, ctx->f_lcu * sizeof(ctx->sync_flag[0]));

    ctx->sync_row = (volatile s32 *)xevd_malloc(ctx->h_lcu * sizeof(int));
    xevd_mset((void *)ctx->sync_row, 0, ctx->h_lcu * sizeof(ctx->sync_row[0]));

    if (sps->vui_parameters_present_flag && sps->vui_parameters.bitstream_restriction_flag)
    {
        ctx->max_coding_delay = sps->vui_parameters.num_reorder_pics;
    }

    return XEVD_OK;
ERR:
    sequence_deinit(ctx);

    return ret;
}

static int slice_init(XEVD_CTX * ctx, XEVD_CORE * core, XEVD_SH * sh)
{
    core->lcu_num = 0;
    core->x_lcu = 0;
    core->y_lcu = 0;
    core->x_pel = 0;
    core->y_pel = 0;

    core->qp_y = ctx->sh.qp + 6 * ctx->sps->bit_depth_luma_minus8;
    core->qp_u = xevd_qp_chroma_dynamic[0][sh->qp_u] + 6 * ctx->sps->bit_depth_chroma_minus8;
    core->qp_v = xevd_qp_chroma_dynamic[1][sh->qp_v] + 6 * ctx->sps->bit_depth_chroma_minus8;

    if (ctx->tc.max_task_cnt > 1)
    {
        xevd_mset((void *)ctx->sync_flag, 0, ctx->f_lcu * sizeof(ctx->sync_flag[0]));
        xevd_mset((void *)ctx->sync_row, 0, ctx->h_lcu * sizeof(ctx->sync_row[0]));
    }

    /* clear maps */
    xevd_mset_x64a(ctx->map_scu, 0, sizeof(u32) * ctx->f_scu);
    xevd_mset_x64a(ctx->cod_eco, 0, sizeof(u8) * ctx->f_scu);
    xevd_mset_x64a(ctx->map_cu_mode, 0, sizeof(u32) * ctx->f_scu);

    if(ctx->sh.slice_type == SLICE_I)
    {
        ctx->last_intra_poc = ctx->poc.poc_val;
    }
    return XEVD_OK;
}

static void make_stat(XEVD_CTX * ctx, int nalu_type, XEVD_STAT * stat)
{
    int i, j;
    stat->nalu_type = nalu_type;
    stat->stype = 0;
    stat->fnum = -1;
    if(ctx)
    {
        stat->read += XEVD_BSR_GET_READ_BYTE(&ctx->bs);
        if(nalu_type < XEVD_NUT_SPS)
        {
            stat->fnum = ctx->pic_cnt;
            stat->stype = ctx->sh.slice_type;

            /* increase decoded picture count */
            ctx->pic_cnt++;
            stat->poc = ctx->poc.poc_val;
            stat->tid = ctx->nalu.nuh_temporal_id;

            for(i = 0; i < 2; i++)
            {
                stat->refpic_num[i] = ctx->dpm.num_refp[i];
                for(j = 0; j < stat->refpic_num[i]; j++)
                {
                    stat->refpic[i][j] = ctx->refp[j][i].poc;
                }
            }
        }
    }
}

static void xevd_lc_itdq(XEVD_CTX * ctx, XEVD_CORE * core)
{
    xevd_sub_block_itdq(ctx, core->coef, core->log2_cuw, core->log2_cuh, core->qp_y, core->qp_u, core->qp_v, core->is_coef, core->is_coef_sub
                      , ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->chroma_format_idc);
}

static void get_nbr_yuv(int x, int y, int cuw, int cuh, XEVD_CTX * ctx, XEVD_CORE * core)
{
    int  s_rec;
    pel *rec;
    int constrained_intra_flag = core->pred_mode == MODE_INTRA && ctx->pps.constrained_intra_pred_flag;

    /* Y */
    s_rec = ctx->pic->s_l;
    rec = ctx->pic->y + (y * s_rec) + x;

    xevd_get_nbr_b(x, y, cuw, cuh, rec, s_rec, core->avail_cu, core->nb, core->scup, ctx->map_scu, ctx->w_scu, ctx->h_scu, Y_C, constrained_intra_flag,ctx->map_tidx
                  , ctx->sps->bit_depth_luma_minus8+8, ctx->sps->chroma_format_idc);

   if (ctx->sps->chroma_format_idc)
    {
        cuw >>= (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc));
        cuh >>= (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc));
        x >>= (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc));
        y >>= (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc));
        s_rec = ctx->pic->s_c;

        /* U */
        rec = ctx->pic->u + (y * s_rec) + x;
        xevd_get_nbr_b(x, y, cuw, cuh, rec, s_rec, core->avail_cu, core->nb, core->scup, ctx->map_scu, ctx->w_scu, ctx->h_scu, U_C, constrained_intra_flag, ctx->map_tidx
                     , ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->chroma_format_idc);
        /* V */
        rec = ctx->pic->v + (y * s_rec) + x;
        xevd_get_nbr_b(x, y, cuw, cuh, rec, s_rec, core->avail_cu, core->nb, core->scup, ctx->map_scu, ctx->w_scu, ctx->h_scu, V_C, constrained_intra_flag, ctx->map_tidx
                     , ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->chroma_format_idc);
    }
}

void xevd_get_direct_motion(XEVD_CTX * ctx, XEVD_CORE * core)
{
    s8            srefi[REFP_NUM][MAX_NUM_MVP];
    s16           smvp[REFP_NUM][MAX_NUM_MVP][MV_D];
    u32           cuw, cuh;

    cuw = (1 << core->log2_cuw);
    cuh = (1 << core->log2_cuh);

    xevd_get_motion_skip_baseline(ctx->sh.slice_type, core->scup, ctx->map_refi, ctx->map_mv, ctx->refp[0], cuw, cuh, ctx->w_scu, srefi, smvp, core->avail_cu);

    core->refi[REFP_0] = srefi[REFP_0][core->mvp_idx[REFP_0]];
    core->refi[REFP_1] = srefi[REFP_1][core->mvp_idx[REFP_1]];

    core->mv[REFP_0][MV_X] = smvp[REFP_0][core->mvp_idx[REFP_0]][MV_X];
    core->mv[REFP_0][MV_Y] = smvp[REFP_0][core->mvp_idx[REFP_0]][MV_Y];

    if (ctx->sh.slice_type == SLICE_P)
    {
        core->refi[REFP_1] = REFI_INVALID;
        core->mv[REFP_1][MV_X] = 0;
        core->mv[REFP_1][MV_Y] = 0;
    }
    else
    {
        core->mv[REFP_1][MV_X] = smvp[REFP_1][core->mvp_idx[REFP_1]][MV_X];
        core->mv[REFP_1][MV_Y] = smvp[REFP_1][core->mvp_idx[REFP_1]][MV_Y];
    }
}

void xevd_get_skip_motion(XEVD_CTX * ctx, XEVD_CORE * core)
{
    int REF_SET[3][XEVD_MAX_NUM_ACTIVE_REF_FRAME] = { {0,0,}, };
    int cuw, cuh, inter_dir = 0;
    s8            srefi[REFP_NUM][MAX_NUM_MVP];
    s16           smvp[REFP_NUM][MAX_NUM_MVP][MV_D];

    cuw = (1 << core->log2_cuw);
    cuh = (1 << core->log2_cuh);

    xevd_get_motion(core->scup, REFP_0, ctx->map_refi, ctx->map_mv, ctx->refp, cuw, cuh, ctx->w_scu, core->avail_cu, srefi[REFP_0], smvp[REFP_0]);

    core->refi[REFP_0] = srefi[REFP_0][core->mvp_idx[REFP_0]];

    core->mv[REFP_0][MV_X] = smvp[REFP_0][core->mvp_idx[REFP_0]][MV_X];
    core->mv[REFP_0][MV_Y] = smvp[REFP_0][core->mvp_idx[REFP_0]][MV_Y];

    if (ctx->sh.slice_type == SLICE_P)
    {
        core->refi[REFP_1] = REFI_INVALID;
        core->mv[REFP_1][MV_X] = 0;
        core->mv[REFP_1][MV_Y] = 0;
    }
    else
    {
        xevd_get_motion(core->scup, REFP_1, ctx->map_refi, ctx->map_mv, ctx->refp, cuw, cuh, ctx->w_scu, core->avail_cu, srefi[REFP_1], smvp[REFP_1]);

        core->refi[REFP_1] = srefi[REFP_1][core->mvp_idx[REFP_1]];
        core->mv[REFP_1][MV_X] = smvp[REFP_1][core->mvp_idx[REFP_1]][MV_X];
        core->mv[REFP_1][MV_Y] = smvp[REFP_1][core->mvp_idx[REFP_1]][MV_Y];
    }
}

void xevd_get_inter_motion(XEVD_CTX * ctx, XEVD_CORE * core)
{
    int cuw, cuh;
    s16           mvp[MAX_NUM_MVP][MV_D];
    s8            refi[MAX_NUM_MVP];

    cuw = (1 << core->log2_cuw);
    cuh = (1 << core->log2_cuh);

    for (int inter_dir_idx = 0; inter_dir_idx < 2; inter_dir_idx++)
    {
        /* 0: forward, 1: backward */
        if (((core->inter_dir + 1) >> inter_dir_idx) & 1)
        {
            xevd_get_motion(core->scup, inter_dir_idx, ctx->map_refi, ctx->map_mv, ctx->refp, cuw, cuh, ctx->w_scu, core->avail_cu, refi, mvp);
            core->mv[inter_dir_idx][MV_X] = mvp[core->mvp_idx[inter_dir_idx]][MV_X] + core->mvd[inter_dir_idx][MV_X];
            core->mv[inter_dir_idx][MV_Y] = mvp[core->mvp_idx[inter_dir_idx]][MV_Y] + core->mvd[inter_dir_idx][MV_Y];
        }
        else
        {
            core->refi[inter_dir_idx] = REFI_INVALID;
            core->mv[inter_dir_idx][MV_X] = 0;
            core->mv[inter_dir_idx][MV_Y] = 0;
        }
    }
}

static int cu_init(XEVD_CTX *ctx, XEVD_CORE *core, int x, int y, int cuw, int cuh)
{
    XEVD_CU_DATA *cu_data = &ctx->map_cu_data[core->lcu_num];

    int x_in_lcu = x - (core->x_lcu << ctx->log2_max_cuwh);
    int y_in_lcu = y - (core->y_lcu << ctx->log2_max_cuwh);
    int cup = (y_in_lcu << (ctx->log2_max_cuwh - MIN_CU_LOG2 * 2)) + (x_in_lcu >> MIN_CU_LOG2);

    core->cuw = cuw;
    core->cuh = cuh;
    core->log2_cuw = XEVD_CONV_LOG2(cuw);
    core->log2_cuh = XEVD_CONV_LOG2(cuh);
    core->x_scu = PEL2SCU(x);
    core->y_scu = PEL2SCU(y);
    core->scup = ((u32)core->y_scu * ctx->w_scu) + core->x_scu;
    core->avail_cu = 0;
    core->skip_flag = 0;
    core->qp_y = cu_data->qp_y[cup];
    core->qp_u = cu_data->qp_u[cup];
    core->qp_v = cu_data->qp_v[cup];
    core->qp = core->qp_y - (6 * ctx->sps->bit_depth_luma_minus8);

    for (int c = Y_C; c < N_C; c++)
    {
        core->is_coef[c] = cu_data->nnz[c][cup];
        for (int sb = 0; sb < MAX_SUB_TB_NUM; sb++)
        {
            core->is_coef_sub[c][sb] = cu_data->nnz_sub[c][sb][cup];
        }
    }

    core->pred_mode = cu_data->pred_mode[cup];

    if (core->pred_mode == MODE_INTRA)
    {
        core->avail_cu = xevd_get_avail_intra(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, core->log2_cuw, core->log2_cuh, ctx->map_scu, ctx->map_tidx);

        core->mv[REFP_0][MV_X] = 0;
        core->mv[REFP_0][MV_Y] = 0;
        core->mv[REFP_1][MV_X] = 0;
        core->mv[REFP_1][MV_Y] = 0;
        core->refi[REFP_0] = -1;
        core->refi[REFP_1] = -1;
        core->ipm[0] = cu_data->ipm[0][cup];
        core->ipm[1] = cu_data->ipm[1][cup];
    }
    else
    {
        core->refi[REFP_0] = cu_data->refi[cup][REFP_0];
        core->refi[REFP_1] = cu_data->refi[cup][REFP_1];
        core->mvp_idx[REFP_0] = cu_data->mvp_idx[cup][REFP_0];
        core->mvp_idx[REFP_1] = cu_data->mvp_idx[cup][REFP_1];
        core->mvr_idx = cu_data->mvr_idx[cup];
        core->bi_idx = cu_data->bi_idx[cup];
        core->inter_dir = cu_data->inter_dir[cup];
        core->skip_flag = 0;

        core->mv[REFP_0][MV_X] = cu_data->mv[cup][REFP_0][MV_X];
        core->mv[REFP_0][MV_Y] = cu_data->mv[cup][REFP_0][MV_Y];
        core->mv[REFP_1][MV_X] = cu_data->mv[cup][REFP_1][MV_X];
        core->mv[REFP_1][MV_Y] = cu_data->mv[cup][REFP_1][MV_Y];

        core->mvd[REFP_0][MV_X] = cu_data->mvd[cup][REFP_0][MV_X];
        core->mvd[REFP_0][MV_Y] = cu_data->mvd[cup][REFP_0][MV_Y];
        core->mvd[REFP_1][MV_X] = cu_data->mvd[cup][REFP_1][MV_X];
        core->mvd[REFP_1][MV_Y] = cu_data->mvd[cup][REFP_1][MV_Y];
    }

    core->avail_lr = xevd_check_nev_avail(core->x_scu, core->y_scu, cuw, cuh, ctx->w_scu, ctx->h_scu, ctx->map_scu, ctx->map_tidx);

    return XEVD_OK;
}

static void coef_rect_to_series(XEVD_CTX * ctx, s16 *coef_src[N_C], int x, int y, int cuw, int cuh, s16 coef_dst[N_C][MAX_CU_DIM], XEVD_CORE * core)
{
    int i, j, sidx, didx;

    sidx = (x&(ctx->max_cuwh - 1)) + ((y&(ctx->max_cuwh - 1)) << ctx->log2_max_cuwh);
    didx = 0;

    for (j = 0; j < cuh; j++)
    {
        for (int k = 0; k < cuw; k++)
        {
            coef_dst[Y_C][k + didx] = coef_src[Y_C][k + sidx];
        }
        didx += cuw;
        sidx += ctx->max_cuwh;
    }

    x >>= (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc));
    y >>= (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc));
    cuw >>= (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc));
    cuh >>= (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc));

    sidx = (x&((ctx->max_cuwh >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) - 1)) + ((y&((ctx->max_cuwh >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) - 1)) << (ctx->log2_max_cuwh - (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))));

    didx = 0;

    for (j = 0; j < cuh; j++)
    {
        for (int k = 0; k < cuw; k++)
        {
            coef_dst[U_C][k + didx] = coef_src[U_C][k + sidx];
            coef_dst[V_C][k + didx] = coef_src[V_C][k + sidx];
        }
        didx += cuw;
        sidx += (ctx->max_cuwh >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)));
    }
}

static int xevd_recon_unit(XEVD_CTX * ctx, XEVD_CORE * core, int x, int y, int log2_cuw, int log2_cuh, int cup)
{
    int  cuw, cuh;
    XEVD_CU_DATA *cu_data = &ctx->map_cu_data[core->lcu_num];

    core->log2_cuw = log2_cuw;
    core->log2_cuh = log2_cuh;
    core->x_scu = PEL2SCU(x);
    core->y_scu = PEL2SCU(y);
    core->scup = core->x_scu + core->y_scu * ctx->w_scu;

    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;

    int chroma_format_idc = ctx->sps->chroma_format_idc;
    cu_init(ctx, core, x, y, cuw, cuh);

    core->avail_lr = xevd_check_nev_avail(core->x_scu, core->y_scu, cuw, cuh, ctx->w_scu, ctx->h_scu, ctx->map_scu, ctx->map_tidx);

    /* inverse transform and dequantization */
    if(core->pred_mode != MODE_SKIP)
    {
        coef_rect_to_series(ctx, cu_data->coef, x, y, cuw, cuh, core->coef, core);
        xevd_lc_itdq(ctx, core);
    }

    /* prediction */
    if(core->pred_mode != MODE_INTRA)
    {
        core->avail_cu = xevd_get_avail_inter(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, cuw, cuh, ctx->map_scu, ctx->map_tidx);

        if (core->pred_mode == MODE_SKIP)
        {
            xevd_get_skip_motion(ctx, core);
        }
        else
        {
            if (core->inter_dir == PRED_DIR)
            {
                xevd_get_mv_dir(ctx->refp[0], ctx->poc.poc_val, core->scup + ((1 << (core->log2_cuw - MIN_CU_LOG2)) - 1) + ((1 << (core->log2_cuh - MIN_CU_LOG2)) - 1) * ctx->w_scu, core->scup, ctx->w_scu, ctx->h_scu, core->mv);
                core->refi[REFP_0] = 0;
                core->refi[REFP_1] = 0;
            }
            else
            {
                xevd_get_inter_motion(ctx, core);
            }
        }
        xevd_mc(x, y, ctx->w, ctx->h, cuw, cuh, core->refi, core->mv, ctx->refp, core->pred, ctx->poc.poc_val
            , ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->bit_depth_chroma_minus8 + 8, ctx->sps->chroma_format_idc);

        xevd_set_dec_info(ctx, core);
    }
    else
    {
        core->avail_cu = xevd_get_avail_intra(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, core->log2_cuw, core->log2_cuh, ctx->map_scu, ctx->map_tidx);
        get_nbr_yuv(x, y, cuw, cuh, ctx, core);

        xevd_ipred_b(core->nb[0][0] + 2, core->nb[0][1] + cuh, core->nb[0][2] + 2, core->avail_lr, core->pred[0][Y_C], core->ipm[0], cuw, cuh);
        xevd_ipred_uv_b(core->nb[1][0] + 2, core->nb[1][1] + (cuh >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc))), core->nb[1][2] + 2, core->avail_lr, core->pred[0][U_C]
                      , core->ipm[1], core->ipm[0], cuw >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), cuh >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)));
        xevd_ipred_uv_b(core->nb[2][0] + 2, core->nb[2][1] + (cuh >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc))), core->nb[2][2] + 2, core->avail_lr, core->pred[0][V_C]
                      , core->ipm[1], core->ipm[0], cuw >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), cuh >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)));
    }

    /* reconstruction */
    xevd_recon_yuv(ctx, core, x, y, cuw, cuh);

    u32 *map_scu = ctx->map_scu + core->scup;
    for (int j = 0; j < cuh >> MIN_CU_LOG2; j++)
    {
        for (int i = 0; i < cuw >> MIN_CU_LOG2; i++)
        {
            MCU_SET_COD(map_scu[i]);
        }
        map_scu += ctx->w_scu;
    }
    return XEVD_OK;
}

static void copy_to_cu_data(XEVD_CTX *ctx, XEVD_CORE *core, int cud);

static int xevd_entropy_dec_unit(XEVD_CTX * ctx, XEVD_CORE * core, int x, int y, int log2_cuw, int log2_cuh, int cud)
{
    int ret, cuw, cuh;

    core->log2_cuw = log2_cuw;
    core->log2_cuh = log2_cuh;
    core->x_scu = PEL2SCU(x);
    core->y_scu = PEL2SCU(y);
    core->scup = core->x_scu + core->y_scu * ctx->w_scu;
    core->x = x;
    core->y = y;
    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;
    int chroma_format_idc = ctx->sps->chroma_format_idc;

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("poc: ");
    XEVD_TRACE_INT(ctx->poc.poc_val);
    XEVD_TRACE_STR("x pos ");
    XEVD_TRACE_INT(x);
    XEVD_TRACE_STR("y pos ");
    XEVD_TRACE_INT(y);
    XEVD_TRACE_STR("width ");
    XEVD_TRACE_INT(cuw);
    XEVD_TRACE_STR("height ");
    XEVD_TRACE_INT(cuh);
    XEVD_TRACE_STR("\n");

    core->avail_lr = xevd_check_eco_nev_avail(core->x_scu, core->y_scu, cuw, cuh, ctx->w_scu, ctx->h_scu, ctx->cod_eco, ctx->map_tidx);

    /* parse CU info */
    ret = xevd_eco_cu(ctx, core);
    xevd_assert_g(ret == XEVD_OK, ERR);

    copy_to_cu_data(ctx, core, cud);

    xevd_set_dec_info(ctx, core);

    u8 *cod_eco = ctx->cod_eco + core->scup;
    xevd_mset(cod_eco, 1, (cuw >> MIN_CU_LOG2) * sizeof(u8));
    for (int i = 1; i < cuh >> MIN_CU_LOG2; i++)
    {
        xevd_mcpy(cod_eco + i*ctx->w_scu, cod_eco, (cuw >> MIN_CU_LOG2) * sizeof(u8));
    }
    return XEVD_OK;
ERR:
    return ret;
}

static void copy_to_cu_data(XEVD_CTX *ctx, XEVD_CORE *core, int cud)
{
    XEVD_CU_DATA *cu_data;
    int idx, size;
    int log2_cuw, log2_cuh;
    int x_in_lcu = core->x - (core->x_lcu << ctx->log2_max_cuwh);
    int y_in_lcu = core->y - (core->y_lcu << ctx->log2_max_cuwh);

    log2_cuw = core->log2_cuw;
    log2_cuh = core->log2_cuh;
    core->cuw = 1 << core->log2_cuw;
    core->cuh = 1 << core->log2_cuh;
    cu_data = &ctx->map_cu_data[core->lcu_num];


    /* copy coef */
    size = core->cuw * sizeof(s16);
    s16* dst_coef = cu_data->coef[Y_C] + (y_in_lcu << ctx->log2_max_cuwh) + x_in_lcu;
    s16* src_coef = core->coef[Y_C];
    for (int j = 0; j < core->cuh; j++)
    {
        xevd_mcpy(dst_coef, src_coef, size);
        dst_coef += ctx->max_cuwh;
        src_coef += core->cuw;
    }

    /* copy mode info */
    idx = (y_in_lcu << (ctx->log2_max_cuwh - MIN_CU_LOG2 * 2)) + (x_in_lcu >> MIN_CU_LOG2);

    cu_data->pred_mode[idx] = core->pred_mode;
    cu_data->skip_flag[idx] = core->skip_flag;
    cu_data->nnz[Y_C][idx] = core->is_coef[Y_C];

    for (int sb = 0; sb < MAX_SUB_TB_NUM; sb++)
    {
        cu_data->nnz_sub[Y_C][sb][idx] = core->is_coef_sub[Y_C][sb];
    }

    cu_data->qp_y[idx] = core->qp_y;
    cu_data->depth[idx] = cud;

    if (core->pred_mode == MODE_INTRA)
    {
        cu_data->ipm[0][idx] = core->ipm[0];
        cu_data->mv[idx][REFP_0][MV_X] = 0;
        cu_data->mv[idx][REFP_0][MV_Y] = 0;
        cu_data->mv[idx][REFP_1][MV_X] = 0;
        cu_data->mv[idx][REFP_1][MV_Y] = 0;
        cu_data->refi[idx][REFP_0] = -1;
        cu_data->refi[idx][REFP_1] = -1;
    }
    else
    {
        cu_data->refi[idx][REFP_0] = core->refi[REFP_0];
        cu_data->refi[idx][REFP_1] = core->refi[REFP_1];
        cu_data->mvp_idx[idx][REFP_0] = core->mvp_idx[REFP_0];
        cu_data->mvp_idx[idx][REFP_1] = core->mvp_idx[REFP_1];
        cu_data->mvr_idx[idx] = core->mvr_idx;
        cu_data->bi_idx[idx] = core->bi_idx;
        cu_data->inter_dir[idx] = core->inter_dir;

        cu_data->mv[idx][REFP_0][MV_X] = core->mv[REFP_0][MV_X];
        cu_data->mv[idx][REFP_0][MV_Y] = core->mv[REFP_0][MV_Y];
        cu_data->mv[idx][REFP_1][MV_X] = core->mv[REFP_1][MV_X];
        cu_data->mv[idx][REFP_1][MV_Y] = core->mv[REFP_1][MV_Y];

        cu_data->mvd[idx][REFP_0][MV_X] = core->mvd[REFP_0][MV_X];
        cu_data->mvd[idx][REFP_0][MV_Y] = core->mvd[REFP_0][MV_Y];
        cu_data->mvd[idx][REFP_1][MV_X] = core->mvd[REFP_1][MV_X];
        cu_data->mvd[idx][REFP_1][MV_Y] = core->mvd[REFP_1][MV_Y];
    }

    size = (core->cuw >> 1) * sizeof(s16);
    for (int c = U_C; c <= V_C; c++)
    {
        s16* dst_coef = cu_data->coef[c] + (y_in_lcu << (ctx->log2_max_cuwh - 2)) + (x_in_lcu >> 1);
        s16* src_coef = core->coef[c];
        for (int j = 0; j < core->cuh >> 1; j++)
        {
            xevd_mcpy(dst_coef, src_coef, size);
            dst_coef += ctx->max_cuwh >> 1;
            src_coef += core->cuw >> 1;
        }
    }

    /* copy mode info */
    idx = (y_in_lcu << (ctx->log2_max_cuwh - MIN_CU_LOG2 * 2)) + (x_in_lcu >> MIN_CU_LOG2);

    cu_data->pred_mode_chroma[idx] = core->pred_mode;
    cu_data->nnz[U_C][idx] = core->is_coef[U_C];
    cu_data->nnz[V_C][idx] = core->is_coef[V_C];
    for (int c = U_C; c < N_C; c++)
    {
        for (int sb = 0; sb < MAX_SUB_TB_NUM; sb++)
        {
            cu_data->nnz_sub[c][sb][idx] = core->is_coef_sub[c][sb];
        }
    }

    cu_data->qp_u[idx] = core->qp_u;
    cu_data->qp_v[idx] = core->qp_v;

    if (core->pred_mode == MODE_INTRA)
    {
        cu_data->ipm[1][idx] = core->ipm[1];
    }

}

static int xevd_entropy_decode_tree(XEVD_CTX * ctx, XEVD_CORE * core, int x0, int y0, int log2_cuw, int log2_cuh, int cup, int cud
                                  , XEVD_BSR * bs, XEVD_SBAC * sbac, int next_split, const int parent_split, const int node_idx
                                  , const int* parent_split_allow, int cu_qp_delta_code)
{
    int ret;
    s8  split_mode;
    int cuw, cuh;
    int split_allow[6];

    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;

    if (cuw > ctx->min_cuwh || cuh > ctx->min_cuwh)
    {
        if(x0 + cuw <= ctx->w && y0 + cuh <= ctx->h)
        {
            if(next_split)
            {
                split_mode = xevd_eco_split_mode(bs, sbac, cuw, cuh);
                XEVD_TRACE_COUNTER;
                XEVD_TRACE_STR("x pos ");
                XEVD_TRACE_INT(core->x_pel + ((cup % (ctx->max_cuwh >> MIN_CU_LOG2) << MIN_CU_LOG2)));
                XEVD_TRACE_STR("y pos ");
                XEVD_TRACE_INT(core->y_pel + ((cup / (ctx->max_cuwh >> MIN_CU_LOG2) << MIN_CU_LOG2)));
                XEVD_TRACE_STR("width ");
                XEVD_TRACE_INT(cuw);
                XEVD_TRACE_STR("height ");
                XEVD_TRACE_INT(cuh);
                XEVD_TRACE_STR("depth ");
                XEVD_TRACE_INT(cud);
                XEVD_TRACE_STR("split mode ");
                XEVD_TRACE_INT(split_mode);
                XEVD_TRACE_STR("\n");
            }
            else
            {
                split_mode = NO_SPLIT;
            }
        }
        else
        {
            split_mode = xevd_eco_split_mode(bs, sbac, cuw, cuh);
            XEVD_TRACE_COUNTER;
            XEVD_TRACE_STR("x pos ");
            XEVD_TRACE_INT(core->x_pel + ((cup % (ctx->max_cuwh >> MIN_CU_LOG2) << MIN_CU_LOG2)));
            XEVD_TRACE_STR("y pos ");
            XEVD_TRACE_INT(core->y_pel + ((cup / (ctx->max_cuwh >> MIN_CU_LOG2) << MIN_CU_LOG2)));
            XEVD_TRACE_STR("width ");
            XEVD_TRACE_INT(cuw);
            XEVD_TRACE_STR("height ");
            XEVD_TRACE_INT(cuh);
            XEVD_TRACE_STR("depth ");
            XEVD_TRACE_INT(cud);
            XEVD_TRACE_STR("split mode ");
            XEVD_TRACE_INT(split_mode);
            XEVD_TRACE_STR("\n");
        }
    }
    else
    {
        split_mode = NO_SPLIT;
    }

    xevd_set_split_mode(split_mode, cud, cup, cuw, cuh, ctx->max_cuwh, core->split_mode);

    if(split_mode != NO_SPLIT)
    {
        XEVD_SPLIT_STRUCT split_struct;

        xevd_split_get_part_structure(split_mode, x0, y0, cuw, cuh, cup, cud, ctx->log2_max_cuwh - MIN_CU_LOG2, &split_struct );

        int curr_part_num;
        for(int part_num = 0; part_num < split_struct.part_count; ++part_num)
        {
            int cur_part_num = part_num;
            int log2_sub_cuw = split_struct.log_cuw[cur_part_num];
            int log2_sub_cuh = split_struct.log_cuh[cur_part_num];
            int x_pos = split_struct.x_pos[cur_part_num];
            int y_pos = split_struct.y_pos[cur_part_num];

            if(x_pos < ctx->w && y_pos < ctx->h)
            {
                ret = xevd_entropy_decode_tree(ctx, core, x_pos, y_pos, log2_sub_cuw, log2_sub_cuh, split_struct.cup[cur_part_num], split_struct.cud[cur_part_num]
                                             , bs, sbac, 1, split_mode, part_num, split_allow, cu_qp_delta_code);
                xevd_assert_g(ret == XEVD_OK, ERR);
            }
            curr_part_num = cur_part_num;
        }

    }
    else
    {
        core->cu_qp_delta_code = cu_qp_delta_code;
        ret = xevd_entropy_dec_unit(ctx, core, x0, y0, log2_cuw, log2_cuh, cud);
        xevd_assert_g(ret == XEVD_OK, ERR);
    }
    return XEVD_OK;
ERR:
    return ret;
}

static int xevd_recon_tree(XEVD_CTX * ctx, XEVD_CORE * core, int x, int y, int cuw, int cuh, int cud, int cup)
{
    s8  split_mode;
    int lcu_num;

    lcu_num = core->lcu_num; //(x >> ctx->log2_max_cuwh) + (y >> ctx->log2_max_cuwh) * ctx->w_lcu;
    xevd_get_split_mode(&split_mode, cud, cup, cuw, cuh, ctx->max_cuwh, &ctx->map_split[lcu_num]);

    if(split_mode != NO_SPLIT)
    {
        XEVD_SPLIT_STRUCT split_struct;
        xevd_split_get_part_structure(split_mode, x, y, cuw, cuh, cup, cud, ctx->log2_max_cuwh - MIN_CU_LOG2, &split_struct);

        for(int part_num = 0; part_num < split_struct.part_count; ++part_num)
        {
            int cur_part_num = part_num;
            int sub_cuw = split_struct.width[cur_part_num];
            int sub_cuh = split_struct.height[cur_part_num];
            int x_pos = split_struct.x_pos[cur_part_num];
            int y_pos = split_struct.y_pos[cur_part_num];

            if (x_pos < ctx->w && y_pos < ctx->h)
            {
                xevd_recon_tree(ctx, core, x_pos, y_pos, sub_cuw, sub_cuh, split_struct.cud[cur_part_num]
                              , split_struct.cup[cur_part_num]);
            }
        }

    }

    if (split_mode == NO_SPLIT)
    {
        xevd_recon_unit(ctx, core, x, y, XEVD_CONV_LOG2(cuw), XEVD_CONV_LOG2(cuh), cup);
    }

    return XEVD_OK;
}

static void deblock_tree(XEVD_CTX * ctx, XEVD_PIC * pic, int x, int y, int cuw, int cuh, int cud, int cup, int is_hor_edge
                       , XEVD_CORE * core, int boundary_filtering)
{
    s8  split_mode;
    int lcu_num;

    lcu_num = (x >> ctx->log2_max_cuwh) + (y >> ctx->log2_max_cuwh) * ctx->w_lcu;
    xevd_get_split_mode(&split_mode, cud, cup, cuw, cuh, ctx->max_cuwh, &ctx->map_split[lcu_num]);

    if(split_mode != NO_SPLIT)
    {
        XEVD_SPLIT_STRUCT split_struct;
        xevd_split_get_part_structure(split_mode, x, y, cuw, cuh, cup, cud, ctx->log2_max_cuwh - MIN_CU_LOG2, &split_struct);

        for(int part_num = 0; part_num < split_struct.part_count; ++part_num)
        {
            int cur_part_num = part_num;
            int sub_cuw = split_struct.width[cur_part_num];
            int sub_cuh = split_struct.height[cur_part_num];
            int x_pos = split_struct.x_pos[cur_part_num];
            int y_pos = split_struct.y_pos[cur_part_num];

            if(x_pos < ctx->w && y_pos < ctx->h)
            {
                deblock_tree(ctx, pic, x_pos, y_pos, sub_cuw, sub_cuh, split_struct.cud[cur_part_num], split_struct.cup[cur_part_num]
                           , is_hor_edge, core, boundary_filtering);
            }
        }
    }

    if (split_mode == NO_SPLIT)
    {
        if(is_hor_edge)
        {
            if (cuh > MAX_TR_SIZE)
            {
                xevd_deblock_cu_hor(ctx, pic, x, y, cuw, cuh >> 1, boundary_filtering);
                xevd_deblock_cu_hor(ctx, pic, x, y + MAX_TR_SIZE, cuw, cuh >> 1, boundary_filtering);
            }
            else
            {
                xevd_deblock_cu_hor(ctx, pic, x, y, cuw, cuh, boundary_filtering);
            }
        }
        else
        {
            if (cuw > MAX_TR_SIZE)
            {
                xevd_deblock_cu_ver(ctx, pic, x, y, cuw >> 1, cuh, boundary_filtering);
                xevd_deblock_cu_ver(ctx, pic, x + MAX_TR_SIZE, y, cuw >> 1, cuh, boundary_filtering);
            }
            else
            {
                xevd_deblock_cu_ver(ctx, pic, x, y, cuw, cuh, boundary_filtering);
            }
        }
    }
}

int xevd_deblock(void * arg)
{
    xevd_assert(arg != NULL);
    XEVD_CORE  * core = (XEVD_CORE *)arg;
    XEVD_CTX   * ctx = core->ctx;
    int          tile_idx;
    int          filter_across_boundary = core->filter_across_boundary;
    int          i, j;

    tile_idx = 0;
    ctx->pic->pic_deblock_alpha_offset = ctx->sh.sh_deblock_alpha_offset;
    ctx->pic->pic_deblock_beta_offset = ctx->sh.sh_deblock_beta_offset;
    ctx->pic->pic_qp_u_offset = ctx->sh.qp_u_offset;
    ctx->pic->pic_qp_v_offset = ctx->sh.qp_v_offset;

    int boundary_filtering = 0;
    int x_l, x_r, y_l, y_r, l_scu, r_scu, t_scu, b_scu;
    u32 k1;
    int scu_in_lcu_wh = 1 << (ctx->log2_max_cuwh - MIN_CU_LOG2);

    if (filter_across_boundary)
    {
        x_l = (ctx->tile[0].ctba_rs_first) % ctx->w_lcu; //entry point lcu's x location
        y_l = (ctx->tile[0].ctba_rs_first) / ctx->w_lcu; // entry point lcu's y location
        x_r = x_l + ctx->tile[0].w_ctb;
        y_r = y_l + ctx->tile[0].h_ctb;
        l_scu = x_l * scu_in_lcu_wh;
        r_scu = XEVD_CLIP3(0, ctx->w_scu, x_r*scu_in_lcu_wh);
        t_scu = y_l * scu_in_lcu_wh;
        b_scu = XEVD_CLIP3(0, ctx->h_scu, y_r*scu_in_lcu_wh);


        boundary_filtering = 1;
        j = t_scu;
        for (i = l_scu; i < r_scu; i++)
        {
            k1 = i + j * ctx->w_scu;
            MCU_CLR_COD(ctx->map_scu[k1]);
        }

        /* horizontal filtering */
        j = y_l;
        for (i = x_l; i < x_r; i++)
        {
            deblock_tree(ctx, ctx->pic, (i << ctx->log2_max_cuwh), (j << ctx->log2_max_cuwh), ctx->max_cuwh, ctx->max_cuwh, 0, 0, 0/*0 - horizontal filtering of vertical edge*/
                , core, boundary_filtering);
        }

        i = l_scu;
        for (j = t_scu; j < b_scu; j++)
        {
            MCU_CLR_COD(ctx->map_scu[i + j * ctx->w_scu]);
        }

        /* vertical filtering */
        i = x_l;
        for (j = y_l; j < y_r; j++)
        {
            deblock_tree(ctx, ctx->pic, (i << ctx->log2_max_cuwh), (j << ctx->log2_max_cuwh), ctx->max_cuwh, ctx->max_cuwh, 0, 0, 1/*1 - vertical filtering of horizontal edge*/
                , core, boundary_filtering);
        }
    }
    else
    {
        while(core->y_lcu < ctx->h_lcu)
        {
            x_l = core->x_lcu;
            y_l = core->y_lcu;
            x_r = x_l + ctx->w_lcu;
            y_r = y_l + 1;
            l_scu = x_l * scu_in_lcu_wh;
            r_scu = XEVD_CLIP3(0, ctx->w_scu, x_r*scu_in_lcu_wh);
            t_scu = y_l * scu_in_lcu_wh;
            b_scu = XEVD_CLIP3(0, ctx->h_scu, y_r*scu_in_lcu_wh);
            if (core->deblock_is_hor)
            {
                for (j = t_scu; j < b_scu; j++)
                {
                    for (i = l_scu; i < r_scu; i++)
                    {
                        k1 = i + j * ctx->w_scu;
                        MCU_CLR_COD(ctx->map_scu[k1]);
                    }
                }

                /* horizontal filtering */
                for (j = y_l; j < y_r; j++)
                {
                    for (i = x_l; i < x_r; i++)
                    {
                        deblock_tree(ctx, ctx->pic, (i << ctx->log2_max_cuwh), (j << ctx->log2_max_cuwh), ctx->max_cuwh, ctx->max_cuwh, 0, 0, 0/*0 - horizontal filtering of vertical edge*/
                            , core, boundary_filtering);
                    }
                }
            }
            else
            {
                core->lcu_num = core->x_lcu + core->y_lcu * ctx->w_lcu;
                if (core->y_lcu != 0)
                {
                    xevd_spinlock_wait(&ctx->sync_flag[core->lcu_num - ctx->w_lcu], THREAD_TERMINATED);
                }
                for (j = t_scu; j < b_scu; j++)
                {
                    for (i = l_scu; i < r_scu; i++)
                    {
                        MCU_CLR_COD(ctx->map_scu[i + j * ctx->w_scu]);
                    }
                }

                /* vertical filtering */
                for (j = y_l; j < y_r; j++)
                {
                    for (i = x_l; i < x_r; i++)
                    {
                        deblock_tree(ctx, ctx->pic, (i << ctx->log2_max_cuwh), (j << ctx->log2_max_cuwh), ctx->max_cuwh, ctx->max_cuwh, 0, 0, 1/*1 - vertical filtering of horizontal edge*/
                            , core, boundary_filtering);
                    }
                }
                xevd_threadsafe_assign(&ctx->sync_flag[core->lcu_num], THREAD_TERMINATED);
                core->x_lcu++;
            }
            core->y_lcu = core->y_lcu + ctx->tc.task_num_in_tile[0];
            core->x_lcu = 0;
        }
    }
    return XEVD_OK;
}

static void update_core_loc_param(XEVD_CTX * ctx, XEVD_CORE * core)
{
    core->x_pel = core->x_lcu << ctx->log2_max_cuwh;  // entry point's x location in pixel
    core->y_pel = core->y_lcu << ctx->log2_max_cuwh;  // entry point's y location in pixel
    core->x_scu = core->x_lcu << (ctx->log2_max_cuwh - MIN_CU_LOG2); // set x_scu location
    core->y_scu = core->y_lcu << (ctx->log2_max_cuwh - MIN_CU_LOG2); // set y_scu location
    core->lcu_num = core->x_lcu + core->y_lcu*ctx->w_lcu; // Init the first lcu_num in tile
}

/* updating core location parameters for CTU parallel encoding case*/
static void update_core_loc_param1(XEVD_CTX * ctx, XEVD_CORE * core)
{
    core->x_pel = core->x_lcu << ctx->log2_max_cuwh;  // entry point's x location in pixel
    core->y_pel = core->y_lcu << ctx->log2_max_cuwh;  // entry point's y location in pixel
    core->x_scu = core->x_lcu << (ctx->log2_max_cuwh - MIN_CU_LOG2); // set x_scu location
    core->y_scu = core->y_lcu << (ctx->log2_max_cuwh - MIN_CU_LOG2); // set y_scu location
}

static int mt_get_next_ctu_num(XEVD_CTX * ctx, XEVD_CORE * core, int skip_ctb_line_cnt)
{
    int sp_x_lcu = ctx->tile[0].ctba_rs_first % ctx->w_lcu;
    int sp_y_lcu = ctx->tile[0].ctba_rs_first / ctx->w_lcu;
    //core->lcu_num++;
    core->x_lcu++;
    if (core->x_lcu == sp_x_lcu + ctx->tile[0].w_ctb)
    {
        core->x_lcu = sp_x_lcu;
        core->y_lcu += skip_ctb_line_cnt;
    }

    core->lcu_num = core->y_lcu * ctx->w_lcu + core->x_lcu;
    /* check to exceed height of ctb line */
    if (core->y_lcu >= sp_y_lcu + ctx->tile[0].h_ctb)
    {
        return -1;
    }
    update_core_loc_param1(ctx, core);

    return core->lcu_num;
}


static int set_tile_info(XEVD_CTX * ctx, XEVD_CORE *core, XEVD_PPS *pps)
{
    XEVD_TILE   * tile;
    int          i, j, size, x, y, w, h, w_tile, h_tile, w_lcu, h_lcu, tidx, t0;
    int          col_w[MAX_NUM_TILES_COL], row_h[MAX_NUM_TILES_ROW], f_tile;
    u8         * map_tidx;
    XEVD_SH       * sh;
    u32         *  map_scu;
    //Below variable need to be handeled separately in multicore environment
    int slice_num = 0;

    sh = &(ctx->sh);
    ctx->tile_cnt = 1;
    ctx->w_tile = 1;
    ctx->h_tile = 1;
    w_tile = ctx->w_tile;
    h_tile = ctx->h_tile;
    f_tile = w_tile * h_tile;
    w_lcu = ctx->w_lcu;
    h_lcu = ctx->h_lcu;

    int first_tile_col_idx, last_tile_col_idx, delta_tile_idx;
    int w_tile_slice, h_tile_slice;
    int tmp1, tmp2;

    first_tile_col_idx = sh->first_tile_id % w_tile;
    last_tile_col_idx = sh->last_tile_id % w_tile;
    delta_tile_idx = sh->last_tile_id - sh->first_tile_id;

    if (sh->last_tile_id < sh->first_tile_id)
    {
        if (first_tile_col_idx > last_tile_col_idx)
        {
            delta_tile_idx += ctx->tile_cnt + w_tile;
        }
        else
        {
            delta_tile_idx += ctx->tile_cnt;
        }
    }
    else if (first_tile_col_idx > last_tile_col_idx)
    {
        delta_tile_idx += w_tile;
    }

    w_tile_slice = (delta_tile_idx % w_tile) + 1; //Number of tiles in slice width
    h_tile_slice = (delta_tile_idx / w_tile) + 1; //Number of tiles in slice height
    ctx->num_tiles_in_slice = w_tile_slice * h_tile_slice;

    int st_row_slice = sh->first_tile_id / w_tile;
    int st_col_slice = sh->first_tile_id % w_tile;

    i = 0;
    for (tmp1 = 0; tmp1 < h_tile_slice; tmp1++)
    {
        for (tmp2 = 0; tmp2 < w_tile_slice; tmp2++)
        {
            int curr_col_slice = (st_col_slice + tmp2) % w_tile;
            int curr_row_slice = (st_row_slice + tmp1) % h_tile;
            ctx->tile_in_slice[i++] = curr_row_slice * w_tile + curr_col_slice;
        }
    }

    /* alloc tile information */
    if (slice_num == 0)
    {
        size = sizeof(XEVD_TILE) * f_tile;
        xevd_mset(ctx->tile, 0, size);
    }

    /* set tile information */
    for (i = 0, t0 = 0; i<(w_tile - 1); i++)
    {
        col_w[i] = pps->tile_column_width_minus1[i] + 1;
        t0 += col_w[i];
    }
    col_w[i] = w_lcu - t0;

    for (i = 0, t0 = 0; i<(h_tile - 1); i++)
    {
        row_h[i] = pps->tile_row_height_minus1[i] + 1;
        t0 += row_h[i];
    }
    row_h[i] = h_lcu - t0;

    /* update tile information */
    tidx = 0;
    tile = &ctx->tile[tidx];
    tile->w_ctb = col_w[0];
    tile->h_ctb = row_h[0];
    tile->f_ctb = tile->w_ctb * tile->h_ctb;
    tile->ctba_rs_first = 0;
    {
        tile = ctx->tile + tidx;

        x = PEL2SCU((tile->ctba_rs_first % w_lcu) << ctx->log2_max_cuwh);
        y = PEL2SCU((tile->ctba_rs_first / w_lcu) << ctx->log2_max_cuwh);
        t0 = PEL2SCU(tile->w_ctb << ctx->log2_max_cuwh);
        w = XEVD_MIN((ctx->w_scu - x), t0);
        t0 = PEL2SCU(tile->h_ctb << ctx->log2_max_cuwh);
        h = XEVD_MIN((ctx->h_scu - y), t0);

        map_tidx = ctx->map_tidx + x + y * ctx->w_scu;
        map_scu = ctx->map_scu + x + y * ctx->w_scu;
        for (j = 0; j<h; j++)
        {
            for (i = 0; i<w; i++)
            {
                map_tidx[i] = tidx;
                MCU_SET_SN(map_scu[i], slice_num);  //Mapping CUs to the slices
            }
            map_tidx += ctx->w_scu;
            map_scu += ctx->w_scu;
        }
    }

    return XEVD_OK;
}



int xevd_tile_eco(void * arg)
{
    XEVD_CORE * core = (XEVD_CORE *)arg;
    XEVD_CTX  * ctx  = core->ctx;
    XEVD_BSR   * bs   = core->bs;
    XEVD_SBAC * sbac = core->sbac;
    XEVD_TILE  * tile;

    int         ret;
    int         col_bd = 0;
    int         lcu_cnt_in_tile = 0;
    int         tile_idx;

    xevd_assert(arg != NULL);

    tile_idx = 0;
    col_bd = 0;

    xevd_eco_sbac_reset(bs, ctx->sh.slice_type, ctx->sh.qp);
    tile = &(ctx->tile[0]);
    core->x_lcu = (ctx->tile[0].ctba_rs_first) % ctx->w_lcu; //entry point lcu's x location
    core->y_lcu = (ctx->tile[0].ctba_rs_first) / ctx->w_lcu; // entry point lcu's y location
    lcu_cnt_in_tile = ctx->tile[0].f_ctb; //Total LCUs in the current tile
    update_core_loc_param(ctx, core);

    while (1) //LCU entropy decoding in a tile
    {
        int split_allow[6] = { 0, 0, 0, 0, 0, 1 };
        xevd_assert_rv(core->lcu_num < ctx->f_lcu, XEVD_ERR_UNEXPECTED);
        core->split_mode = &ctx->map_split[core->lcu_num];

        //Recursion to do entropy decoding for the entire CTU
        ret = xevd_entropy_decode_tree(ctx, core, core->x_pel, core->y_pel, ctx->log2_max_cuwh, ctx->log2_max_cuwh, 0, 0, bs, sbac, 1
            , NO_SPLIT, 0, split_allow, 0);

        xevd_assert_g(XEVD_SUCCEEDED(ret), ERR);

        lcu_cnt_in_tile--;
        if (lcu_cnt_in_tile == 0)
        {
            xevd_threadsafe_assign(&ctx->sync_row[core->y_lcu], THREAD_TERMINATED);
            xevd_assert_gv(xevd_eco_tile_end_flag(bs, sbac) == 1, ret, XEVD_ERR, ERR);
            ret = xevd_eco_cabac_zero_word(bs);
            xevd_assert_g(XEVD_SUCCEEDED(ret), ERR);
            break;
        }
        core->x_lcu++;
        if (core->x_lcu >= ctx->tile[0].w_ctb + col_bd)
        {
            xevd_threadsafe_assign(&ctx->sync_row[core->y_lcu], THREAD_TERMINATED);
            core->x_lcu = (tile->ctba_rs_first) % ctx->w_lcu;
            core->y_lcu++;
        }
        update_core_loc_param(ctx, core);
    }


    return XEVD_OK;
ERR:
    return ret;
}

int xevd_ctu_row_rec_mt(void * arg)
{
    XEVD_CORE * core = (XEVD_CORE *)arg;
    XEVD_CTX  * ctx = core->ctx;
    XEVD_TILE  * tile;

    int         ret;
    int         lcu_cnt_in_tile = 0;
    int         tile_idx;

    xevd_assert(arg != NULL);

    tile_idx = 0;
    tile = &(ctx->tile[0]);
    lcu_cnt_in_tile = ctx->tile[0].f_ctb; //Total LCUs in the current tile
    update_core_loc_param1(ctx, core);

    int sp_x_lcu = ctx->tile[0].ctba_rs_first % ctx->w_lcu;
    int sp_y_lcu = ctx->tile[0].ctba_rs_first / ctx->w_lcu;

    //LCU decoding with in a tile
    while (ctx->tile[0].f_ctb > 0)
    {
        if(ctx->tc.task_num_in_tile[0] > 2)
        {
            xevd_spinlock_wait(&ctx->sync_row[core->y_lcu], THREAD_TERMINATED);
        }
        if (core->y_lcu != sp_y_lcu && core->x_lcu < (sp_x_lcu + ctx->tile[0].w_ctb - 1))
        {
            /* up-right CTB */
            xevd_spinlock_wait(&ctx->sync_flag[core->lcu_num - ctx->w_lcu + 1], THREAD_TERMINATED);
        }
        xevd_assert_rv(core->lcu_num < ctx->f_lcu, XEVD_ERR_UNEXPECTED);

        ret = xevd_recon_tree(ctx, core, (core->x_lcu << ctx->log2_max_cuwh), (core->y_lcu << ctx->log2_max_cuwh), ctx->max_cuwh, ctx->max_cuwh, 0, 0);
        xevd_assert_g(XEVD_SUCCEEDED(ret), ERR);

        xevd_threadsafe_assign(&ctx->sync_flag[core->lcu_num], THREAD_TERMINATED);
        xevd_threadsafe_decrement(ctx->sync_block, (volatile s32 *)&ctx->tile[0].f_ctb);

        if (ctx->tc.task_num_in_tile[0] > 2)
        {
            core->lcu_num = mt_get_next_ctu_num(ctx, core, ctx->tc.task_num_in_tile[0] - 1);
        }
        else
        {
        core->lcu_num = mt_get_next_ctu_num(ctx, core, ctx->tc.task_num_in_tile[0]);
        }
        if (core->lcu_num == -1)
        {
            break;
        }
    }
    return XEVD_OK;
ERR:
    return ret;
}

int xevd_tile_mt(void * arg)
{
    XEVD_CORE  * core = (XEVD_CORE *)arg;
    XEVD_CTX   * ctx = core->ctx;
    int          res, ret = XEVD_OK;
    int          thread_idx = core->thread_idx + ctx->tc.tile_task_num;
    xevd_mset((void *)ctx->sync_row, 0, ctx->tile[0].h_ctb * sizeof(ctx->sync_row[0]));
    if (ctx->tc.task_num_in_tile[0] > 2)
    {
        for (int thread_cnt = 1; thread_cnt < ctx->tc.task_num_in_tile[0]; thread_cnt++)
        {
            if (thread_cnt < ctx->tile[0].h_ctb)
            {
                xevd_mcpy(ctx->core_mt[thread_idx], core, sizeof(XEVD_CORE));
                ctx->core_mt[thread_idx]->x_lcu = ((ctx->tile[0].ctba_rs_first) % ctx->w_lcu);               //entry point lcu's x location
                ctx->core_mt[thread_idx]->y_lcu = ((ctx->tile[0].ctba_rs_first) / ctx->w_lcu) + thread_cnt - 1; // entry point lcu's y location //MULTICORE_ENT_RECON
                ctx->core_mt[thread_idx]->lcu_num = ctx->core_mt[thread_idx]->y_lcu * ctx->w_lcu + ctx->core_mt[thread_idx]->x_lcu;
                ctx->core_mt[thread_idx]->thread_idx = thread_idx;
                ctx->tc.run(ctx->thread_pool[thread_idx], xevd_ctu_row_rec_mt, (void*)ctx->core_mt[thread_idx]);
            }
            thread_idx += ctx->tc.tile_task_num;
        }
        ret = xevd_tile_eco(arg);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
        thread_idx = core->thread_idx + ctx->tc.tile_task_num;
        for (int thread_cnt = 1; thread_cnt < ctx->tc.task_num_in_tile[0]; thread_cnt++)
        {
            if (thread_cnt < ctx->tile[0].h_ctb)
            {
                ctx->tc.join(ctx->thread_pool[thread_idx], &res);
                if (XEVD_FAILED(res))
                {
                    ret = res;
                }
            }
            thread_idx += ctx->tc.tile_task_num;
        }
    }
    else
    {

    ret = xevd_tile_eco(arg);
    xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

    for (int thread_cnt = 1; thread_cnt < ctx->tc.task_num_in_tile[0]; thread_cnt++)
    {
        if (thread_cnt < ctx->tile[0].h_ctb)
        {
            xevd_mcpy(ctx->core_mt[thread_idx], core, sizeof(XEVD_CORE));
            ctx->core_mt[thread_idx]->x_lcu = ((ctx->tile[0].ctba_rs_first) % ctx->w_lcu);               //entry point lcu's x location
            ctx->core_mt[thread_idx]->y_lcu = ((ctx->tile[0].ctba_rs_first) / ctx->w_lcu) + thread_cnt; // entry point lcu's y location
            ctx->core_mt[thread_idx]->lcu_num = ctx->core_mt[thread_idx]->y_lcu * ctx->w_lcu + ctx->core_mt[thread_idx]->x_lcu;
            ctx->core_mt[thread_idx]->thread_idx = thread_idx;
            ctx->tc.run(ctx->thread_pool[thread_idx], xevd_ctu_row_rec_mt, (void*)ctx->core_mt[thread_idx]);
        }
        thread_idx += ctx->tc.tile_task_num;
    }

    core->x_lcu = ((ctx->tile[0].ctba_rs_first) % ctx->w_lcu);
    core->y_lcu = ((ctx->tile[0].ctba_rs_first) / ctx->w_lcu);
    core->lcu_num = core->y_lcu*ctx->w_lcu + core->x_lcu;
    xevd_ctu_row_rec_mt(arg);

    thread_idx = core->thread_idx + ctx->tc.tile_task_num;
    for (int thread_cnt = 1; thread_cnt < ctx->tc.task_num_in_tile[0]; thread_cnt++)
    {
        if (thread_cnt < ctx->tile[0].h_ctb)
        {
            ctx->tc.join(ctx->thread_pool[thread_idx], &res);
            if (XEVD_FAILED(res))
            {
                ret = res;
            }
        }
        thread_idx += ctx->tc.tile_task_num;
        }
    }
    return ret;
}

int xevd_dec_slice(XEVD_CTX * ctx, XEVD_CORE * core)
{
    XEVD_BSR   * bs;
    XEVD_SBAC * sbac;
    XEVD_TILE  * tile;
    XEVD_CORE * core_mt;
    XEVD_BSR     bs_temp;
    XEVD_SBAC    sbac_temp;

    int         ret;
    int         res = 0;
    int         lcu_cnt_in_tile = 0;
    int         col_bd = 0;
    int         num_tiles_in_slice = 1;

    bs   = &ctx->bs;
    sbac = GET_SBAC_DEC(bs);
    xevd_mcpy(&bs_temp, bs, sizeof(XEVD_BSR));
    xevd_mcpy(&sbac_temp, sbac, sizeof(XEVD_SBAC));
    ctx->sh.qp_prev_eco = ctx->sh.qp;

    xevd_mcpy(ctx->core_mt[0], core, sizeof(XEVD_CORE));
    core_mt = ctx->core_mt[0];

    core_mt->ctx = ctx;
    core_mt->bs = &core_mt->ctx->bs_mt[0];
    core_mt->sbac = &core_mt->ctx->sbac_dec_mt[0];
    core_mt->tile_num = 0;
    core_mt->thread_idx = 0;

    ctx->tile[0].qp_prev_eco = ctx->sh.qp;
    ctx->tile[0].qp = ctx->sh.qp;

    xevd_mcpy(core_mt->bs, &bs_temp, sizeof(XEVD_BSR));
    xevd_mcpy(core_mt->sbac, &sbac_temp, sizeof(XEVD_SBAC));
    SET_SBAC_DEC(core_mt->bs, core_mt->sbac);

    ret = xevd_tile_mt((void *)core_mt);
    xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

    tile = &(ctx->tile[0]);
    ctx->num_ctb -= (tile->w_ctb * tile->h_ctb);

    xevd_mcpy(&ctx->bs, ctx->core_mt[0]->bs, sizeof(XEVD_BSR));
    xevd_mcpy(&ctx->sbac_dec, ctx->core_mt[0]->sbac, sizeof(XEVD_SBAC));

    return XEVD_OK;
}

void xevd_malloc_1d(void** dst, int size)
{
    if(*dst == NULL)
    {
        *dst = xevd_malloc_fast(size);
        xevd_mset(*dst, 0, size);
    }
}

void xevd_malloc_2d(s8*** dst, int size_1d, int size_2d, int type_size)
{
    int i;

    if(*dst == NULL)
    {
        *dst = xevd_malloc_fast(size_1d * sizeof(s8*));
        xevd_mset(*dst, 0, size_1d * sizeof(s8*));

        (*dst)[0] = xevd_malloc_fast(size_1d * size_2d * type_size);
        xevd_mset((*dst)[0], 0, size_1d * size_2d * type_size);

        for(i = 1; i < size_1d; i++)
        {
            (*dst)[i] = (*dst)[i - 1] + size_2d * type_size;
        }
    }
}

int xevd_create_cu_data(XEVD_CU_DATA *cu_data, int log2_cuw, int log2_cuh)
{
    int i, j;
    int cuw_scu, cuh_scu;
    int size_8b, size_16b, size_32b, cu_cnt, pixel_cnt;

    cuw_scu = 1 << log2_cuw;
    cuh_scu = 1 << log2_cuh;

    size_8b = cuw_scu * cuh_scu * sizeof(s8);
    size_16b = cuw_scu * cuh_scu * sizeof(s16);
    size_32b = cuw_scu * cuh_scu * sizeof(s32);
    cu_cnt = cuw_scu * cuh_scu;
    pixel_cnt = cu_cnt << 4;

    xevd_malloc_1d((void**)&cu_data->qp_y, size_8b);
    xevd_malloc_1d((void**)&cu_data->qp_u, size_8b);
    xevd_malloc_1d((void**)&cu_data->qp_v, size_8b);
    xevd_malloc_1d((void**)&cu_data->pred_mode, size_8b);
    xevd_malloc_1d((void**)&cu_data->pred_mode_chroma, size_8b);

    xevd_malloc_2d((s8***)&cu_data->mpm, 2, cu_cnt, sizeof(u8));
    xevd_malloc_2d((s8***)&cu_data->ipm, 2, cu_cnt, sizeof(u8));
    xevd_malloc_2d((s8***)&cu_data->mpm_ext, 8, cu_cnt, sizeof(u8));
    xevd_malloc_1d((void**)&cu_data->skip_flag, size_8b);

    xevd_malloc_2d((s8***)&cu_data->refi, cu_cnt, REFP_NUM, sizeof(u8));
    xevd_malloc_2d((s8***)&cu_data->mvp_idx, cu_cnt, REFP_NUM, sizeof(u8));
    xevd_malloc_1d((void**)&cu_data->mvr_idx, size_8b);
    xevd_malloc_1d((void**)&cu_data->bi_idx, size_8b);
    xevd_malloc_1d((void**)&cu_data->inter_dir, size_8b);

    for(i = 0; i < N_C; i++)
    {
        xevd_malloc_1d((void**)&cu_data->nnz[i], size_32b);
    }
    for (i = 0; i < N_C; i++)
    {
        for (j = 0; j < 4; j++)
        {
            xevd_malloc_1d((void**)&cu_data->nnz_sub[i][j], size_32b);
        }
    }
    xevd_malloc_1d((void**)&cu_data->map_scu, size_32b);
    xevd_malloc_1d((void**)&cu_data->map_cu_mode, size_32b);
    xevd_malloc_1d((void**)&cu_data->depth, size_8b);

    for(i = 0; i < N_C; i++)
    {
        xevd_malloc_1d((void**)&cu_data->coef[i], (pixel_cnt >> (!!(i)* 2)) * sizeof(s16));
        xevd_malloc_1d((void**)&cu_data->reco[i], (pixel_cnt >> (!!(i)* 2)) * sizeof(pel));
    }
    return XEVD_OK;
}

int xevd_ready(XEVD_CTX *ctx)
{
    int ret = XEVD_OK;
    XEVD_CORE *core = NULL;

    xevd_assert(ctx);

    core = core_alloc();
    xevd_assert_gv(core != NULL, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);

    ctx->core = core;

    for(int i = 0; i < XEVD_MAX_TASK_CNT; i++)
    {
        core = core_alloc();
        xevd_assert_gv(core != NULL, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        ctx->core_mt[i] = core;
    }
    return XEVD_OK;
ERR:
    if(core)
    {
        core_free(core);
    }
    return ret;
}

void xevd_flush(XEVD_CTX * ctx)
{
    if(ctx->core)
    {
        core_free(ctx->core);
        ctx->core = NULL;
    }

    for(int i = 0; i < XEVD_MAX_TASK_CNT; i++)
    {
        if(ctx->core_mt[i])
        {
            core_free(ctx->core_mt[i]);
            ctx->core_mt[i] = NULL;
        }
    }
}


int xevd_dec_nalu(XEVD_CTX * ctx, XEVD_BITB * bitb, XEVD_STAT * stat)
{
    XEVD_BSR  *bs = &ctx->bs;
    XEVD_SPS  *sps = &ctx->sps_array[ctx->sps_id];
    XEVD_PPS  *pps = &ctx->pps;
    XEVD_SH   *sh = &ctx->sh;
    XEVD_NALU *nalu = &ctx->nalu;
    int        ret;

    ret = XEVD_OK;
    /* set error status */
    ctx->bs_err = bitb->err;

    /* bitstream reader initialization */
    xevd_bsr_init(bs, bitb->addr, bitb->ssize, NULL);
    SET_SBAC_DEC(bs, &ctx->sbac_dec);

    /* parse nalu header */
    ret = xevd_eco_nalu(bs, nalu);
    xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
    if(nalu->nal_unit_type_plus1 - 1 == XEVD_NUT_SPS)
    {
        XEVD_SPS sps_new;
        xevd_mset(&sps_new, 0, sizeof(XEVD_SPS));
        ret = xevd_eco_sps(bs, &sps_new);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

        ctx->sps_id = sps_new.sps_seq_parameter_set_id;
        xevd_mcpy(&ctx->sps_array[ctx->sps_id], &sps_new, sizeof(XEVD_SPS));
        ctx->sps = &ctx->sps_array[ctx->sps_id];

        ret = sequence_init(ctx, ctx->sps);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
        ctx->sps_count++;
    }
    else if (nalu->nal_unit_type_plus1 - 1 == XEVD_NUT_PPS)
    {
        ret = xevd_eco_pps(bs, sps, pps);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
        ret = picture_init(ctx);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
    }
    else if (nalu->nal_unit_type_plus1 - 1 < XEVD_NUT_SPS)
    {
        /* decode slice header */
        sh->num_ctb = ctx->f_lcu;

        ret = xevd_eco_sh(bs, ctx->sps, &ctx->pps, sh, ctx->nalu.nal_unit_type_plus1 - 1);

        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

        if (ctx->num_ctb == 0)
        {
           ctx->num_ctb = ctx->f_lcu;
        }

        /* POC derivation process */
        if (ctx->poc.poc_val > ctx->poc.prev_pic_max_poc_val)
        {
            ctx->poc.prev_pic_max_poc_val = ctx->poc.poc_val;
        }

        if (ctx->nalu.nal_unit_type_plus1 - 1 == XEVD_NUT_IDR)
        {
            sh->poc_lsb = 0;
            ctx->poc.prev_doc_offset = -1;
            ctx->poc.prev_poc_val = 0;
            ctx->slice_ref_flag = (ctx->nalu.nuh_temporal_id == 0 || ctx->nalu.nuh_temporal_id < ctx->sps->log2_sub_gop_length);
            ctx->poc.poc_val = 0;
        }
        else
        {
            ctx->slice_ref_flag = (ctx->nalu.nuh_temporal_id == 0 || ctx->nalu.nuh_temporal_id < ctx->sps->log2_sub_gop_length);
            xevd_poc_derivation(ctx->sps, ctx->nalu.nuh_temporal_id, &ctx->poc);
            sh->poc_lsb = ctx->poc.poc_val;
        }

        s32 pic_delay = (ctx->poc.poc_val - (s32)ctx->poc.prev_pic_max_poc_val - 1);
        if (ctx->max_coding_delay < pic_delay)
        {
            ctx->max_coding_delay = pic_delay;
        }

        ret = slice_init(ctx, ctx->core, sh);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
        static u16 slice_num = 0;

        ret = set_tile_info(ctx, ctx->core, pps);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
        if (ctx->num_ctb == 0)
        {
            ctx->num_ctb = ctx->f_lcu;
            slice_num = 0;
        }
        ctx->slice_num = slice_num;
        slice_num++;

        /* initialize reference pictures */
        ret = xevd_picman_refp_init(&ctx->dpm, ctx->sps->max_num_ref_pics, sh->slice_type, ctx->poc.poc_val, ctx->nalu.nuh_temporal_id, ctx->last_intra_poc, ctx->refp);
        xevd_assert_rv(ret == XEVD_OK, ret);

        if (ctx->num_ctb == ctx->f_lcu)
        {
            /* get available frame buffer for decoded image */
            ctx->pic = xevd_picman_get_empty_pic(&ctx->dpm, &ret, ctx->internal_codec_bit_depth);
            xevd_assert_rv(ctx->pic, ret);

            /* get available frame buffer for decoded image */
            ctx->map_refi = ctx->pic->map_refi;
            ctx->map_mv = ctx->pic->map_mv;

            int size;
            size = sizeof(s8) * ctx->f_scu * REFP_NUM;
            xevd_mset_x64a(ctx->map_refi, -1, size);
            size = sizeof(s16) * ctx->f_scu * REFP_NUM * MV_D;
            xevd_mset_x64a(ctx->map_mv, 0, size);
        }

        /* decode slice layer */
        ret = ctx->fn_dec_slice(ctx, ctx->core);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

        /* deblocking filter */
        if (ctx->sh.deblocking_filter_on)
        {

            int fitler_across_boundary = 0;
            int i, j;
            int num_tiles_in_slice = ctx->num_tiles_in_slice;
            int res = 0;
            XEVD_CORE * core_mt;

            /* Horizontal Filtering*/

            for (j = 1; j < ctx->tc.task_num_in_tile[0]; j++)
            {
                core_mt = ctx->core_mt[j];
                core_mt->ctx = ctx;
                core_mt->y_lcu = j;
                core_mt->x_lcu = 0;
                core_mt->deblock_is_hor = 1;
                core_mt->filter_across_boundary = 0;
                ret = ctx->tc.run(ctx->thread_pool[j], ctx->fn_deblock, (void *)core_mt);
                xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

            }

            core_mt = ctx->core_mt[0];
            core_mt->ctx = ctx;
            core_mt->y_lcu = 0;
            core_mt->x_lcu = 0;
            core_mt->deblock_is_hor = 1;
            core_mt->filter_across_boundary = 0;
            ret = ctx->fn_deblock((void *)core_mt);
            xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

            for (i = 1; i < ctx->tc.task_num_in_tile[0]; i++)
            {
                ret = ctx->tc.join(ctx->thread_pool[i], &res);
            }

            xevd_mset((void *)ctx->sync_flag, 0, ctx->f_lcu * sizeof(ctx->sync_flag[0]));
            /* Vertical filtering*/

            for (j = 1; j < ctx->tc.task_num_in_tile[0]; j++)
            {

                core_mt = ctx->core_mt[j];
                core_mt->ctx = ctx;
                core_mt->y_lcu = j;
                core_mt->x_lcu = 0;
                core_mt->deblock_is_hor = 0;
                core_mt->filter_across_boundary = 0;
                ret = ctx->tc.run(ctx->thread_pool[j], ctx->fn_deblock, (void *)core_mt);
                xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

            }
            core_mt = ctx->core_mt[0];
            core_mt->ctx = ctx;
            core_mt->y_lcu = 0;
            core_mt->x_lcu = 0;
            core_mt->deblock_is_hor = 0;
            core_mt->filter_across_boundary = 0;
            ret = ctx->fn_deblock((void *)core_mt);
            xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

            for (i = 1; i < ctx->tc.task_num_in_tile[0]; i++)
            {
                ret = ctx->tc.join(ctx->thread_pool[i], &res);
            }
        }
        if (ctx->num_ctb == 0)
        {
            /* expand pixels to padding area */
            ctx->fn_picbuf_expand(ctx, ctx->pic);

            /* put decoded picture to DPB */
            ret = xevd_picman_put_pic(&ctx->dpm, ctx->pic, ctx->nalu.nal_unit_type_plus1 - 1 == XEVD_NUT_IDR, ctx->poc.poc_val, ctx->nalu.nuh_temporal_id, 1, ctx->refp, ctx->slice_ref_flag, ctx->ref_pic_gap_length);
            xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
        }

        if(ctx->pic_cnt == 0) {
            ctx->ts.frame_first_dts = bitb->ts[XEVD_TS_DTS];
            ctx->ts.frame_duration_time = 0;
        } else if(ctx->pic_cnt == 1) {
            ctx->ts.frame_duration_time = bitb->ts[XEVD_TS_DTS] - ctx->ts.frame_first_dts;
        }
        ctx->pic->imgb->ts[XEVD_TS_DTS] = bitb->ts[XEVD_TS_DTS];

        int coding_delay = ctx->poc.poc_val - (int)ctx->pic_cnt; // number of frames between coding and presentation
        ctx->pic->imgb->ts[XEVD_TS_PTS] = bitb->ts[XEVD_TS_DTS] + coding_delay * ctx->ts.frame_duration_time;

        for (int i=0; i<XEVD_NDATA_NUM; i++) {
            ctx->pic->imgb->ndata[i] = bitb->ndata[i];
        }

        for (int i=0; i<XEVD_PDATA_NUM; i++) {
            ctx->pic->imgb->pdata[i] = bitb->pdata[i];
        }
    }
    else if (nalu->nal_unit_type_plus1 - 1 == XEVD_NUT_SEI)
    {
        ret = xevd_eco_sei(ctx, bs);

        if (ctx->pic_sign_exist)
        {
            if (ctx->use_pic_sign)
            {
                ret = xevd_picbuf_check_signature(ctx->pic, ctx->pic_sign, ctx->sps->bit_depth_luma_minus8 + 8);
                ctx->pic_sign_exist = 0;
            }
            else
            {
                ret = XEVD_WARN_CRC_IGNORED;
            }
        }
    }
    else
    {
        assert(!"wrong NALU type");
    }

    make_stat(ctx, nalu->nal_unit_type_plus1 - 1, stat);

    if (ctx->num_ctb > 0)
    {
        stat->fnum = -1;
    }

    return ret;
}

int xevd_pull_frm(XEVD_CTX *ctx, XEVD_IMGB **imgb)
{
    int ret;
    XEVD_PIC *pic;

    *imgb = NULL;

    pic = xevd_picman_out_pic(&ctx->dpm, &ret);

    if(pic)
    {
        xevd_assert_rv(pic->imgb != NULL, XEVD_ERR);

        /* increase reference count */
        pic->imgb->addref(pic->imgb);
        *imgb = pic->imgb;
        if (ctx->sps->picture_cropping_flag)
        {
            int end_comp = ctx->sps->chroma_format_idc ? N_C : Y_C;
            for (int i = 0; i < end_comp; i++)
            {
                int cs_offset = i == Y_C ? 2 : 1;
                (*imgb)->x[i] = ctx->sps->picture_crop_left_offset * cs_offset;
                (*imgb)->y[i] = ctx->sps->picture_crop_top_offset * cs_offset;
                (*imgb)->h[i] = (*imgb)->ah[i] - (ctx->sps->picture_crop_top_offset + ctx->sps->picture_crop_bottom_offset) * cs_offset;
                (*imgb)->w[i] = (*imgb)->aw[i] - (ctx->sps->picture_crop_left_offset + ctx->sps->picture_crop_right_offset) * cs_offset;
            }
        }
    }
    return ret;
}

int xevd_platform_init(XEVD_CTX *ctx)
{

#if ARM_NEON
    xevd_func_mc_l = xevd_tbl_mc_l_neon;
    xevd_func_mc_c = xevd_tbl_mc_c_neon;
    xevd_func_average_no_clip = &xevd_average_16b_no_clip_neon;
    ctx->fn_itxb   = &xevd_tbl_itxb_neon;
    ctx->fn_recon = &xevd_recon_neon;
    ctx->fn_dbk = &xevd_tbl_dbk_neon;
    ctx->fn_dbk_chroma = &xevd_tbl_dbk_chroma_neon;


#else
#if X86_SSE
    int check_cpu, support_sse, support_avx, support_avx2;

    check_cpu = xevd_check_cpu_info();
    support_sse = (check_cpu >> 1) & 1;
    support_avx = check_cpu & 1;
    support_avx2 = (check_cpu >> 2) & 1;

    if (support_avx2)
    {
        xevd_func_mc_l = xevd_tbl_mc_l_avx;
        xevd_func_mc_c = xevd_tbl_mc_c_avx;
        xevd_func_average_no_clip = &xevd_average_16b_no_clip_sse;
        ctx->fn_itxb   = &xevd_tbl_itxb_avx;
        ctx->fn_recon = &xevd_recon_avx;
        ctx->fn_dbk = &xevd_tbl_dbk_sse;
        ctx->fn_dbk_chroma = &xevd_tbl_dbk_chroma_sse;
    }
    else if (support_sse)
    {
        xevd_func_mc_l = xevd_tbl_mc_l_sse;
        xevd_func_mc_c = xevd_tbl_mc_c_sse;
        xevd_func_average_no_clip = &xevd_average_16b_no_clip_sse;
        ctx->fn_itxb   = &xevd_tbl_itxb_sse;
        ctx->fn_recon = &xevd_recon_sse;
        ctx->fn_dbk = &xevd_tbl_dbk_sse;
        ctx->fn_dbk_chroma = &xevd_tbl_dbk_chroma_sse;
    }
    else
    {
        xevd_func_mc_l = xevd_tbl_mc_l;
        xevd_func_mc_c = xevd_tbl_mc_c;
        xevd_func_average_no_clip = &xevd_average_16b_no_clip;
        ctx->fn_itxb   = &xevd_tbl_itxb;
        ctx->fn_recon = &xevd_recon;
        ctx->fn_dbk = &xevd_tbl_dbk;
        ctx->fn_dbk_chroma = &xevd_tbl_dbk_chroma;
    }
#else
    {
        xevd_func_mc_l = xevd_tbl_mc_l;
        xevd_func_mc_c = xevd_tbl_mc_c;
        xevd_func_average_no_clip = &xevd_average_16b_no_clip;
        ctx->fn_itxb   = &xevd_tbl_itxb;
        ctx->fn_recon = &xevd_recon;
        ctx->fn_dbk = &xevd_tbl_dbk;
        ctx->fn_dbk_chroma = &xevd_tbl_dbk_chroma;
    }
#endif
#endif

    ctx->fn_ready         = xevd_ready;
    ctx->fn_flush         = xevd_flush;
    ctx->fn_dec_cnk       = xevd_dec_nalu;
    ctx->fn_dec_slice     = xevd_dec_slice;
    ctx->fn_pull          = xevd_pull_frm;
    ctx->fn_deblock       = xevd_deblock;
    ctx->fn_picbuf_expand = xevd_picbuf_expand;
    ctx->pf               = NULL;

    return XEVD_OK;
}

void xevd_platform_deinit(XEVD_CTX * ctx)
{
    xevd_assert(ctx->pf == NULL);

    ctx->fn_ready         = NULL;
    ctx->fn_flush         = NULL;
    ctx->fn_dec_cnk       = NULL;
    ctx->fn_dec_slice     = NULL;
    ctx->fn_pull          = NULL;
    ctx->fn_deblock       = NULL;

    ctx->fn_picbuf_expand = NULL;
}

XEVD xevd_create(XEVD_CDSC * cdsc, int * err)
{
    XEVD_CTX *ctx = NULL;
    int ret;

    ctx = ctx_alloc();
    xevd_assert_gv(ctx != NULL, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
    xevd_mcpy(&ctx->cdsc, cdsc, sizeof(XEVD_CDSC));
    xevd_assert_gv(!(cdsc->threads > XEVD_MAX_TASK_CNT), ret, XEVD_ERR_THREAD_ALLOCATION, ERR);
    ret = xevd_init_thread_controller(&ctx->tc, cdsc->threads);
    xevd_assert_g(XEVD_SUCCEEDED(ret), ERR);

     //initialize the threads to NULL
    for (int i = 0; i < XEVD_MAX_TASK_CNT; i++)
    {
        ctx->thread_pool[i] = 0;
    }

    //get the context synchronization handle
    ctx->sync_block = xevd_get_synchronized_object();
    xevd_assert_gv(ctx->sync_block != NULL, ret, XEVD_ERR_UNKNOWN, ERR);

    if (ctx->tc.max_task_cnt > 1)
    {
        for (int i = 1; i < ctx->tc.max_task_cnt; i++)
        {
            ctx->thread_pool[i] = ctx->tc.create(&ctx->tc, i);
            xevd_assert_gv(ctx->thread_pool[i] != NULL, ret, XEVD_ERR_UNKNOWN, ERR);
        }
    }

    /* additional initialization for each platform, if needed */
    ret = xevd_platform_init(ctx);
    xevd_assert_g(ret == XEVD_OK, ERR);

    ret = xevd_scan_tbl_init(ctx);
    xevd_assert_g(ret == XEVD_OK, ERR);

    if(ctx->fn_ready)
    {
        ret = ctx->fn_ready(ctx);
        xevd_assert_g(ret == XEVD_OK, ERR);
    }

    /* Set CTX variables to default value */
    ctx->magic = XEVD_MAGIC_CODE;
    ctx->id = (XEVD)ctx;

    return (ctx->id);
ERR:
    if (ctx)
    {
        if (ctx->sync_block)
        {
            xevd_release_synchornized_object(&ctx->sync_block);
        }
        if (ctx->tc.max_task_cnt)
        {
            //thread controller instance is present
            //terminate the created thread
            for (int i = 1; i < ctx->tc.max_task_cnt; i++)
            {
                if (ctx->thread_pool[i])
                {
                    //valid thread instance
                    ctx->tc.release(&ctx->thread_pool[i]);
                }
            }
            //dinitialize the tc
            xevd_dinit_thread_controller(&ctx->tc);
        }
        if (ctx->fn_flush) ctx->fn_flush(ctx);
        xevd_platform_deinit(ctx);
        ctx_free(ctx);
    }

    if(err) *err = ret;

    return NULL;
}

void xevd_delete(XEVD id)
{
    XEVD_CTX *ctx;
    XEVD_ID_TO_CTX_R(id, ctx);

    sequence_deinit(ctx);

    if (ctx->sync_block)
    {
        xevd_release_synchornized_object(&ctx->sync_block);
    }

    //release the thread pool
    if (ctx->tc.max_task_cnt)
    {
        //thread controller instance is present
        //terminate the created thread
        for (int i = 1; i < ctx->tc.max_task_cnt; i++)
        {
            if (ctx->thread_pool[i])
            {
                //valid thread instance
                ctx->tc.release(&ctx->thread_pool[i]);
            }
        }
        //dinitialize the tc
        xevd_dinit_thread_controller(&ctx->tc);
    }

    if(ctx->fn_flush) ctx->fn_flush(ctx);

    /* addtional deinitialization for each platform, if needed */
    xevd_platform_deinit(ctx);
    xevd_scan_tbl_delete(ctx);
    ctx_free(ctx);
}

int xevd_config(XEVD id, int cfg, void * buf, int * size)
{
    XEVD_CTX *ctx;
    int t0 = 0;

    XEVD_ID_TO_CTX_RV(id, ctx, XEVD_ERR_INVALID_ARGUMENT);

    switch(cfg)
    {
    /* set config ************************************************************/
    case XEVD_CFG_SET_USE_PIC_SIGNATURE:
        ctx->use_pic_sign = (*((int *)buf)) ? 1 : 0;
        break;

    /* get config ************************************************************/
    case XEVD_CFG_GET_CODEC_BIT_DEPTH:
        xevd_assert_rv(*size == sizeof(int), XEVD_ERR_INVALID_ARGUMENT);
        *((int *)buf) = ctx->internal_codec_bit_depth;
        break;

    case XEVD_CFG_GET_WIDTH:
        xevd_assert_rv(*size == sizeof(int), XEVD_ERR_INVALID_ARGUMENT);
        t0 = ctx->sps->picture_crop_left_offset + ctx->sps->picture_crop_right_offset;
        if(ctx->sps->chroma_format_idc) { t0 *= 2; /* unit is chroma */}
        *((int *)buf) = ctx->w - t0;
        break;

    case XEVD_CFG_GET_HEIGHT:
        xevd_assert_rv(*size == sizeof(int), XEVD_ERR_INVALID_ARGUMENT);
        t0 = ctx->sps->picture_crop_top_offset + ctx->sps->picture_crop_bottom_offset;
        if(ctx->sps->chroma_format_idc) { t0 *= 2; /* unit is chroma */}
        *((int *)buf) = ctx->h - t0;
        break;

    case XEVD_CFG_GET_CODED_WIDTH:
        xevd_assert_rv(*size == sizeof(int), XEVD_ERR_INVALID_ARGUMENT);
        *((int *)buf) = ctx->w;
        break;

    case XEVD_CFG_GET_CODED_HEIGHT:
        xevd_assert_rv(*size == sizeof(int), XEVD_ERR_INVALID_ARGUMENT);
        *((int *)buf) = ctx->h;
        break;

    case XEVD_CFG_GET_COLOR_SPACE:
        xevd_assert_rv(*size == sizeof(int), XEVD_ERR_INVALID_ARGUMENT);
        *((int *)buf) = xevd_chroma_format_idc_to_imgb_cs[ctx->sps->chroma_format_idc];
        break;

    case XEVD_CFG_GET_MAX_CODING_DELAY:
        xevd_assert_rv(*size == sizeof(int), XEVD_ERR_INVALID_ARGUMENT);
        *((int *)buf) = ctx->max_coding_delay;
        break;

    default:
        xevd_assert_rv(0, XEVD_ERR_UNSUPPORTED);
    }
    return XEVD_OK;
}

int xevd_decode(XEVD id, XEVD_BITB * bitb, XEVD_STAT * stat)

{
    XEVD_CTX *ctx;

    XEVD_ID_TO_CTX_RV(id, ctx, XEVD_ERR_INVALID_ARGUMENT);

    xevd_assert_rv(ctx->fn_dec_cnk, XEVD_ERR_UNEXPECTED);

    return ctx->fn_dec_cnk(ctx, bitb, stat);
}

int xevd_pull(XEVD id, XEVD_IMGB ** img)
{
    XEVD_CTX *ctx;

    XEVD_ID_TO_CTX_RV(id, ctx, XEVD_ERR_INVALID_ARGUMENT);
    xevd_assert_rv(ctx->fn_pull, XEVD_ERR_UNKNOWN);

    return ctx->fn_pull(ctx, img);
}
