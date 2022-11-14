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


#ifndef _XEVDM_UTIL_H_
#define _XEVDM_UTIL_H_


#include "xevdm_def.h"
#include "xevd_util.h"
#include <stdlib.h>

u16 xevdm_get_avail_inter(int x_scu, int y_scu, int w_scu, int h_scu, int scup, int cuw, int cuh, u32 *map_scu, u8* map_tidx);
u16 xevdm_get_avail_ibc(int x_scu, int y_scu, int w_scu, int h_scu, int scup, int cuw, int cuh, u32 * map_scu, u8* map_tidx);
XEVD_PIC* xevdm_picbuf_alloc_exp(int w, int h, int pad_l, int pad_c, int *err, int idc, int bitdepth);
void xevdm_picbuf_free(PICBUF_ALLOCATOR* pa, XEVD_PIC *pic);

void xevdm_get_mmvd_mvp_list(s8(*map_refi)[REFP_NUM], XEVD_REFP refp[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], int w_scu, int h_scu, int scup, u16 avail, int cuw, int cuh, int slice_t, int real_mv[][2][3], u32 *map_scu, int REF_SET[][XEVD_MAX_NUM_ACTIVE_REF_FRAME], u16 avail_lr
    , u32 curr_ptr, u8 num_refp[REFP_NUM]
    , XEVD_HISTORY_BUFFER history_buffer, int admvp_flag, XEVD_SH* sh, int log2_max_cuwh, u8 * map_tidx, int mmvd_idx);

void xevdm_check_motion_availability(int scup, int cuw, int cuh, int w_scu, int h_scu, int neb_addr[MAX_NUM_POSSIBLE_SCAND], int valid_flag[MAX_NUM_POSSIBLE_SCAND], u32 *map_scu, u16 avail_lr, int num_mvp, int is_ibc, u8 * map_tidx);
void xevdm_get_default_motion(int neb_addr[MAX_NUM_POSSIBLE_SCAND], int valid_flag[MAX_NUM_POSSIBLE_SCAND], s8 cur_refi, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], s8 *refi, s16 mv[MV_D]
    , u32 *map_scu, s16(*map_unrefined_mv)[REFP_NUM][MV_D], int scup, int w_scu, XEVD_HISTORY_BUFFER history_buffer, int hmvp_flag);

s8 xevdm_get_first_refi(int scup, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], int cuw, int cuh, int w_scu, int h_scu, u32 *map_scu, u8 mvr_idx, u16 avail_lr
    , s16(*map_unrefined_mv)[REFP_NUM][MV_D], XEVD_HISTORY_BUFFER history_buffer, int hmvp_flag, u8 * map_tidx);

void xevdm_get_motion(int scup, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D],
    XEVD_REFP(*refp)[REFP_NUM], int cuw, int cuh, int w_scu, u16 avail, s8 refi[MAXM_NUM_MVP], s16 mvp[MAXM_NUM_MVP][MV_D]);
void xevdm_get_motion_merge_main(int poc, int slice_type, int scup, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D],
    XEVD_REFP refp[REFP_NUM], int cuw, int cuh, int w_scu, int h_scu, s8 refi[REFP_NUM][MAXM_NUM_MVP], s16 mvp[REFP_NUM][MAXM_NUM_MVP][MV_D], u32 *map_scu, u16 avail_lr
    , s16(*map_unrefined_mv)[REFP_NUM][MV_D], XEVD_HISTORY_BUFFER history_buffer, u8 ibc_flag, XEVD_REFP(*refplx)[REFP_NUM], XEVD_SH* sh, int log2_max_cuwh, u8 *map_tidx);
void xevdm_get_merge_insert_mv(s8* refi_dst, s16 *mvp_dst_L0, s16 *mvp_dst_L1, s8* map_refi_src, s16* map_mv_src, int slice_type, int cuw, int cuh, int is_sps_admvp);
void xevdm_get_motion_skip_baseline(int slice_type, int scup, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D],
    XEVD_REFP refp[REFP_NUM], int cuw, int cuh, int w_scu, s8 refi[REFP_NUM][MAXM_NUM_MVP], s16 mvp[REFP_NUM][MAXM_NUM_MVP][MV_D], u16 avail_lr);
void xevdm_get_mv_collocated(XEVD_REFP(*refp)[REFP_NUM], u32 poc, int scup, int c_scu, u16 w_scu, u16 h_scu, s16 mvp[REFP_NUM][MV_D], s8 *availablePredIdx, XEVD_SH* sh);
void xevdm_get_motion_from_mvr(u8 mvr_idx, int poc, int scup, int lidx, s8 cur_refi, int num_refp, \
    s16(*map_mv)[REFP_NUM][MV_D], s8(*map_refi)[REFP_NUM], XEVD_REFP(*refp)[REFP_NUM], \
    int cuw, int cuh, int w_scu, int h_scu, u16 avail, s16 mvp[MAXM_NUM_MVP][MV_D], s8 refi_pred[MAXM_NUM_MVP], u32* map_scu, u16 avail_lr
    , s16(*map_unrefined_mv)[REFP_NUM][MV_D], XEVD_HISTORY_BUFFER history_buffer, int hmvp_flag, u8* map_tidx);


//! Get array of split modes tried sequentially in RDO
void xevdm_split_get_split_rdo_order(int cuw, int cuh, SPLIT_MODE splits[MAX_SPLIT_NUM]);

//! Get SUCO partition order
void xevdm_split_get_suco_order(int suco_flag, SPLIT_MODE mode, int suco_order[SPLIT_MAX_PART_COUNT]);
//! Is mode triple tree?
int  xevdm_split_is_TT(SPLIT_MODE mode);
//! Is mode BT?
int  xevdm_split_is_BT(SPLIT_MODE mode);

void xevdm_get_mv_dir(XEVD_REFP refp[REFP_NUM], u32 poc, int scup, int c_scu, u16 w_scu, u16 h_scu, s16 mvp[REFP_NUM][MV_D], int sps_admvp_flag);

int  xevdm_get_suco_flag(s8* suco_flag, int cud, int cup, int cuw, int cuh, int lcu_s, s8 (* suco_flag_buf)[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU]);
void xevdm_set_suco_flag(s8  suco_flag, int cud, int cup, int cuw, int cuh, int lcu_s, s8 (* suco_flag_buf)[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU]);
u8   xevdm_check_suco_cond(int cuw, int cuh, s8 split_mode, int boundary, u8 log2_max_cuwh, u8 suco_max_depth, u8 suco_depth, u8 log2_min_cu_size);
void xevdm_get_ctx_some_flags(int x_scu, int y_scu, int cuw, int cuh, int w_scu, u32* map_scu, u8* cod_eco, u32* map_cu_mode, u8* ctx, u8 slice_type, int sps_cm_init_flag, u8 ibc_flag, u8 ibc_log_max_size, u8* map_tidx, int eco_flag);
void xevdm_mv_rounding_s32(s32 hor, int ver, s32 * rounded_hor, s32 * rounded_ver, s32 right_shift, int left_shift);
void xevdm_rounding_s32(s32 comp, s32 *rounded_comp, int right_shift, int left_shift);
void xevdm_derive_affine_subblock_size_bi(s16 ac_mv[REFP_NUM][VER_NUM][MV_D], s8 refi[REFP_NUM], int cuw, int cuh, int *sub_w, int *sub_h, int vertex_num, BOOL*mem_band_conditions_for_eif_are_satisfied);
void xevdm_derive_affine_subblock_size(s16 ac_mv[VER_NUM][MV_D], int cuw, int cuh, int *sub_w, int *sub_h, int vertex_num, BOOL*mem_band_conditions_for_eif_are_satisfied);

BOOL xevdm_check_eif_applicability_bi(s16 ac_mv[REFP_NUM][VER_NUM][MV_D], s8 refi[REFP_NUM], int cuw, int cuh, int vertex_num, BOOL* mem_band_conditions_are_satisfied);
BOOL xevdm_check_eif_applicability_uni(s16 ac_mv[VER_NUM][MV_D], int cuw, int cuh, int vertex_num, BOOL* mem_band_conditions_are_satisfied);

void xevdm_get_affine_motion_scaling(int poc, int scup, int lidx, s8 cur_refi, int num_refp, \
    s16(*map_mv)[REFP_NUM][MV_D], s8(*map_refi)[REFP_NUM], XEVD_REFP(*refp)[REFP_NUM], \
    int cuw, int cuh, int w_scu, int h_scu, u16 avail, s16 mvp[MAXM_NUM_MVP][VER_NUM][MV_D], s8 refi[MAXM_NUM_MVP]
    , u32* map_scu, u32* map_affine, int vertex_num, u16 avail_lr, int log2_max_cuwh, s16(*map_unrefined_mv)[REFP_NUM][MV_D], u8* map_tidx);

int xevdm_get_affine_merge_candidate(int poc, int slice_type, int scup, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D],
    XEVD_REFP(*refp)[REFP_NUM], int cuw, int cuh, int w_scu, int h_scu, u16 avail, s8 mrg_list_refi[AFF_MAX_CAND][REFP_NUM], s16 mrg_list_cp_mv[AFF_MAX_CAND][REFP_NUM][VER_NUM][MV_D], int mrg_list_cp_num[AFF_MAX_CAND], u32* map_scu, u32* map_affine
    , int log2_max_cuwh, s16(*map_unrefined_mv)[REFP_NUM][MV_D], u16 avail_lr, XEVD_SH * sh, u8 * map_tidx);


#define ALLOW_SPLIT_RATIO(long_side, block_ratio) (block_ratio <= BLOCK_14 && (long_side <= xevd_split_tbl[block_ratio][IDX_MAX] && long_side >= xevd_split_tbl[block_ratio][IDX_MIN]) ? 1 : 0)
#define ALLOW_SPLIT_TRI(long_side) ((long_side <= xevd_split_tbl[BLOCK_TT][IDX_MAX] && long_side >= xevd_split_tbl[BLOCK_TT][IDX_MIN]) ? 1 : 0)
void xevdm_check_split_mode(int *split_allow, int log2_cuw, int log2_cuh, int boundary, int boundary_b, int boundary_r, int log2_max_cuwh
    , const int parent_split, int* same_layer_split, const int node_idx, const int* parent_split_allow, int qt_depth, int btt_depth
    , int x, int y, int im_w, int im_h
    , u8* remaining_split, int sps_btt_flag
    , MODE_CONS mode_cons
);


void xevdm_init_scan_sr(int *scan, int size_x, int size_y, int width, int height, int scan_type);
int xevdm_get_ctx_sig_coeff_inc(s16 *pcoeff, int blkpos, int width, int height, int ch_type);
int xevdm_get_ctx_gtA_inc(s16 *pcoeff, int blkpos, int width, int height, int ch_type);
int xevdm_get_ctx_gtB_inc(s16 *pcoeff, int blkpos, int width, int height, int ch_type);
int xevdm_get_ctx_remain_inc(s16 *pcoeff, int blkpos, int width, int height, int ch_type);
int xevdm_get_rice_para(s16 *pcoeff, int blkpos, int width, int height, int base_level);


int xevdm_get_transform_shift(int log2_size, int type, int bit_depth);


#if SIMD_CLIP
void clip_simd(const pel* src, int src_stride, pel *dst, int dst_stride, int width, int height, const int clp_rng_min, const int clp_rng_max);
#endif

u8 xevdm_check_ats_inter_info_coded(int cuw, int cuh, int pred_mode, int tool_ats);
void xevdm_get_tu_size(u8 ats_inter_info, int log2_cuw, int log2_cuh, int* log2_tuw, int* log2_tuh);
void xevdm_get_ats_inter_trs(u8 ats_inter_info, int log2_cuw, int log2_cuh, u8* ats_cu, u8* ats_mode);
void xevdm_set_cu_cbf_flags(u8 cbf_y, u8 ats_inter_info, int log2_cuw, int log2_cuh, u32 *map_scu, int w_scu);
BOOL xevdm_check_bi_applicability(int slice_type, int cuw, int cuh, int is_sps_admvp);

u8 xevd_is_chroma_split_allowed(int w, int h, SPLIT_MODE split);
u8 xevd_check_luma_fn(TREE_CONS tree_cons);
u8 xevd_check_chroma_fn(TREE_CONS tree_cons);
u8 xevd_check_all_fn(TREE_CONS tree_cons);
u8 xevd_check_only_intra_fn(TREE_CONS tree_cons);
u8 xevd_check_only_inter_fn(TREE_CONS tree_cons);
u8 xevd_check_all_preds_fn(TREE_CONS tree_cons);
MODE_CONS xevdm_get_mode_cons_by_split(SPLIT_MODE split_mode, int cuw, int cuh);
XEVD_PIC * xevdm_picbuf_alloc(PICBUF_ALLOCATOR * pa, int * ret, int bitdepth);
int xevdm_picbuf_check_signature(XEVD_PIC * pic, u8 signature[N_C][16], XEVD_IMGB *imgb, int compare_md5);
void xevdm_get_mmvd_motion(XEVD_CTX * ctx, XEVD_CORE * core);
void xevdm_set_affine_mvf(XEVD_CTX * ctx, XEVD_CORE * core);
static int xevdm_hmvp_init(XEVD_CORE * core);

/* set decoded information, such as MVs, inter_dir, etc. */
void xevdm_set_dec_info(XEVD_CTX * ctx, XEVD_CORE * core);

void xevdm_split_tbl_init(XEVD_CTX *ctx, XEVD_SPS * sps);

#if USE_DRAW_PARTITION_DEC
void xevd_draw_partition(XEVD_CTX * ctx, XEVD_PIC * pic);
#endif

u8 xevd_check_luma(XEVD_CTX *ctx, XEVD_CORE * core);
u8 xevd_check_chroma(XEVD_CTX *ctx, XEVD_CORE * core);
u8 xevd_check_all(XEVD_CTX *ctx, XEVD_CORE * core);
u8 xevd_check_only_intra(XEVD_CTX *ctx, XEVD_CORE * core);
u8 xevd_check_only_inter(XEVD_CTX *ctx, XEVD_CORE * core);
u8 xevd_check_all_preds(XEVD_CTX *ctx, XEVD_CORE * core);
MODE_CONS xevd_derive_mode_cons(XEVD_CTX *ctx, int scup);
XEVD_IMGB * xevd_imgb_generate(int w, int h, int padl, int padc, int idc, int bit_depth);
void xevd_imgb_destroy(XEVD_IMGB *imgb);
void xevd_imgb_cpy(XEVD_IMGB * dst, XEVD_IMGB * src);
#endif /* _XEVD_UTIL_H_ */
