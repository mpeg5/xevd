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


#ifndef _XEVDM_MC_H_
#define _XEVDM_MC_H_

extern s16 tbl_mc_l_coeff_main[16][8];
extern s16 tbl_mc_c_coeff_main[32][4];
extern const s16 xevd_tbl_bl_mc_l_coeff[4 << MC_PRECISION_ADD][2];

typedef void(*XEVDM_DMVR_MC_L) (pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth);
typedef void(*XEVDM_DMVR_MC_C) (pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth);

extern XEVDM_DMVR_MC_L xevdm_tbl_dmvr_mc_l[2][2];
extern XEVDM_DMVR_MC_C xevdm_tbl_dmvr_mc_c[2][2];
extern XEVD_MC_C xevdm_tbl_bl_mc_l[2][2];

extern XEVDM_DMVR_MC_L (*xevdm_func_dmvr_mc_l)[2];
extern XEVDM_DMVR_MC_C (*xevdm_func_dmvr_mc_c)[2];
extern XEVD_MC_C (*xevdm_func_bl_mc_l)[2];
#if ARM_NEON
#include "xevdm_mc_neon.h"
#elif X86_SSE
#include "x86/xevdm_mc_sse.h"
#endif

#define xevdm_dmvr_mc_l(ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth) \
    (xevdm_func_dmvr_mc_l[((gmv_x) | ((gmv_x)>>1) | ((gmv_x)>>2) | ((gmv_x)>>3)) & 0x1])\
        [((gmv_y) | ((gmv_y)>>1) | ((gmv_y)>>2) | ((gmv_y)>>3)) & 0x1]\
        (ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth)

#define xevdm_dmvr_mc_c(ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth) \
    (xevdm_func_dmvr_mc_c[((gmv_x) | ((gmv_x)>>1) | ((gmv_x)>>2)| ((gmv_x)>>3) | ((gmv_x)>>4)) & 0x1]\
        [((gmv_y) | ((gmv_y)>>1) | ((gmv_y)>>2) | ((gmv_y)>>3) | ((gmv_y)>>4)) & 0x1])\
        (ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth)

#define xevdm_bl_mc_l(ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth) \
    (xevdm_func_bl_mc_l[((gmv_x) | ((gmv_x)>>1) | ((gmv_x)>>2) | ((gmv_x)>>3)) & 0x1])\
        [((gmv_y) | ((gmv_y)>>1) | ((gmv_y)>>2) | ((gmv_y)>>3)) & 0x1]\
        (ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth)

void xevdm_mc(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16(*mv)[MV_D], XEVD_REFP(*refp)[REFP_NUM], pel pred[REFP_NUM][N_C][MAX_CU_DIM]
            , int poc_c, pel* dmvr_current_template, pel dmvr_ref_pred_interpolated[REFP_NUM][(MAX_CU_SIZE + ((DMVR_NEW_VERSION_ITER_COUNT + 1) * REF_PRED_EXTENTION_PEL_COUNT)) * (MAX_CU_SIZE + ((DMVR_NEW_VERSION_ITER_COUNT + 1) * REF_PRED_EXTENTION_PEL_COUNT))]
            , pel dmvr_half_pred_interpolated[REFP_NUM][(MAX_CU_SIZE + 1) * (MAX_CU_SIZE + 1)], BOOL apply_DMVR, pel dmvr_padding_buf[REFP_NUM][N_C][PAD_BUFFER_STRIDE * PAD_BUFFER_STRIDE], u8* cu_dmvr_flag, s16 dmvr_mv[MAX_CU_CNT_IN_LCU][REFP_NUM][MV_D]
            , int sps_admvp_flag, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc);

void xevdm_IBC_mc(int x, int y, int log2_cuw, int log2_cuh, s16 mv[MV_D], XEVD_PIC *ref_pic, pel(*pred)[MAX_CU_DIM], TREE_CONS tree_cons, int chroma_format_idc);

void mv_clip(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16 mv[REFP_NUM][MV_D], s16(*mv_t)[MV_D]);
void xevdm_affine_mc(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16 mv[REFP_NUM][VER_NUM][MV_D], XEVD_REFP(*refp)[REFP_NUM], pel pred[2][N_C][MAX_CU_DIM], int vertex_num, pel* tmp_buffer
                   , int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc);
void xevdm_affine_mc_l(int x, int y, int pic_w, int pic_h, int cuw, int cuh, s16 ac_mv[VER_NUM][MV_D], XEVD_PIC* ref_pic, pel pred[MAX_CU_DIM], int vertex_num, pel* tmp_buffer
                     , int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc);
void xevdm_affine_mc_lc(int x, int y, int pic_w, int pic_h, int cuw, int cuh, s16 ac_mv[VER_NUM][MV_D], XEVD_PIC* ref_pic, pel pred[N_C][MAX_CU_DIM], int vertex_num, int sub_w, int sub_h
                      , pel* tmp_buffer, BOOL mem_band_conditions_for_eif_are_satisfied, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc);
void xevdm_eif_mc(int block_width, int block_height, int x, int y, int mv_scale_hor, int mv_scale_ver, int dmv_hor_x, int dmv_hor_y, int dmv_ver_x, int dmv_ver_y,int hor_max
                , int ver_max, int hor_min, int ver_min, pel* p_ref, int ref_stride, pel *p_dst, int dst_stride, pel* p_tmp_buf, char affine_mv_prec, s8 comp, int bit_depth, int chroma_format_idc);
#endif /* _XEVD_MC_H_ */
