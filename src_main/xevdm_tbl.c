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

#define NA 255 //never split
#define NB 14  //not reach in current setting of max AR 1:4
#define NC 15  //not reach in current setting of max AR 1:4

s16 xevd_tbl_tr2[NUM_TRANS_TYPE][2][2];
s16 xevd_tbl_tr4[NUM_TRANS_TYPE][4][4];
s16 xevd_tbl_tr8[NUM_TRANS_TYPE][8][8];
s16 xevd_tbl_tr16[NUM_TRANS_TYPE][16][16];
s16 xevd_tbl_tr32[NUM_TRANS_TYPE][32][32];
s16 xevd_tbl_tr64[NUM_TRANS_TYPE][64][64];
s16 xevd_tbl_tr128[NUM_TRANS_TYPE][128][128];

int xevd_tbl_tr_subset_intra[4] = { DST7, DCT8 };

s16 xevd_tbl_inv_tr2[NUM_TRANS_TYPE][2][2];
s16 xevd_tbl_inv_tr4[NUM_TRANS_TYPE][4][4];
s16 xevd_tbl_inv_tr8[NUM_TRANS_TYPE][8][8];
s16 xevd_tbl_inv_tr16[NUM_TRANS_TYPE][16][16];
s16 xevd_tbl_inv_tr32[NUM_TRANS_TYPE][32][32];
s16 xevd_tbl_inv_tr64[NUM_TRANS_TYPE][64][64];
s16 xevd_tbl_inv_tr128[NUM_TRANS_TYPE][128][128];

u16 xevd_split_tbl[SPLIT_CHECK_NUM][2];


const s16 init_skip_flag[2][NUM_CTX_SKIP_FLAG] =
{
    {    0,    0, },
    {  711,  233, },
};

const s16 init_direct_mode_flag[2][NUM_CTX_DIRECT_MODE_FLAG] =
{
    {    0, },
    {    0, },
};

const s16 init_merge_mode_flag[2][NUM_CTX_MERGE_MODE_FLAG] =
{
    {    0, },
    {  464, },
};

const s16 init_inter_dir[2][NUM_CTX_INTER_PRED_IDC] =
{
    {    0,    0, },
    {  242,   80, },
};

const s16 init_intra_luma_pred_mpm_flag[2][NUM_CTX_INTRA_LUMA_PRED_MPM_FLAG] =
{
    {  263, },
    {  225, },
};

const s16 init_intra_luma_pred_mpm_idx[2][NUM_CTX_INTRA_LUMA_PRED_MPM_IDX] =
{
    {  436, },
    {  724, },
};

const s16 init_intra_chroma_pred_mode[2][NUM_CTX_INTRA_CHROMA_PRED_MODE] =
{
    {  465, },
    {  560, },
};

const s16 init_intra_dir[2][NUM_CTX_INTRA_PRED_MODE] =
{
    {    0,    0, },
    {    0,    0, },
};

const s16 init_pred_mode[2][NUM_CTX_PRED_MODE] =
{
    {   64,    0,    0, },
    {  481,   16,  368, },
};

const s16 init_refi[2][NUM_CTX_REF_IDX] =
{
    {    0,    0, },
    {  288,    0, },
};

const s16 init_merge_idx[2][NUM_CTX_MERGE_IDX] =
{
    {    0,    0,    0,  496,  496, },
    {   18,  128,  146,   37,   69, },
};

const s16 init_mvp_idx[2][NUM_CTX_MVP_IDX] =
{
    {    0,    0,    0, },
    {    0,    0,    0, },
};

const s16 init_bi_idx[2][NUM_CTX_BI_PRED_IDX] =
{
    {    0,    0, },
    {   49,   17, },
};

const s16 init_mvd[2][NUM_CTX_MVD] =
{
    {    0, },
    {   18, },
};

const s16 init_cbf_all[2][NUM_CTX_CBF_ALL] =
{
    {    0, },
    {  794, },
};

const s16 init_cbf_luma[2][NUM_CTX_CBF_LUMA] =
{
    {  664, },
    {  368, },
};

const s16 init_cbf_cb[2][NUM_CTX_CBF_CB] =
{
    {  384, },
    {  416, },
};

const s16 init_cbf_cr[2][NUM_CTX_CBF_CR] =
{
    {  320, },
    {  288, },
};

const s16 init_dqp[2][NUM_CTX_DELTA_QP] =
{
    {    4, },
    {    4, },
};

const s16 init_run[2][NUM_CTX_CC_RUN] =
{
    {   48,  112,  128,    0,  321,   82,  419,  160,  385,  323,  353,  129,  225,  193,  387,  389,  453,  227,  453,  161,  421,  161,  481,  225, },
    {  129,  178,  453,   97,  583,  259,  517,  259,  453,  227,  871,  355,  291,  227,  195,   97,  161,   65,   97,   33,   65,    1, 1003,  227, },
};

const s16 init_last[2][NUM_CTX_CC_LAST] =
{
    {  421,  337, },
    {   33,  790, },
};

const s16 init_level[2][NUM_CTX_CC_LEVEL] =
{
    {  416,   98,  128,   66,   32,   82,   17,   48,  272,  112,   52,   50,  448,  419,  385,  355,  161,  225,   82,   97,  210,    0,  416,  224, },
    {  805,  775,  775,  581,  355,  389,   65,  195,   48,   33,  224,  225,  775,  227,  355,  161,  129,   97,   33,   65,   16,    1,  841,  355, },
};

const s16 init_split_cu_flag[2][NUM_CTX_SPLIT_CU_FLAG] =
{
    {    0, },
    {    0, },
};

const s16 init_ibc_flag[2][NUM_CTX_IBC_FLAG] =
{
    {    0,    0, },
    {  711,  233, },
};

const s16 init_mmvd_flag[2][NUM_CTX_MMVD_FLAG] =
{
    {    0, },
    {  194, },
};

const s16 init_mmvd_merge_idx[2][NUM_CTX_MMVD_MERGE_IDX] =
{
    {    0,    0,    0, },
    {   49,  129,   82, },
};

const s16 init_mmvd_distance_idx[2][NUM_CTX_MMVD_DIST_IDX] =
{
    {    0,    0,    0,    0,    0,    0,    0, },
    {  179,    5,  133,  131,  227,   64,  128, },
};

const s16 init_mmvd_direction_idx[2][NUM_CTX_MMVD_DIRECTION_IDX] =
{
    {    0,    0, },
    {  161,   33, },
};

const s16 init_mmvd_group_idx[2][NUM_CTX_MMVD_GROUP_IDX] =
{
    {    0,    0, },
    {  453,   48, },
};


const s16 init_mode_cons[2][NUM_CTX_MODE_CONS] =
{
    {   64,    0,    0, },
    {  481,   16,  368, },
};



const s16 init_affine_mvp_idx[2][NUM_CTX_AFFINE_MVP_IDX] =
{
    {    0, },
    {  161, },
};

const s16 init_mvr_idx[2][NUM_CTX_AMVR_IDX] =
{
    {    0,    0,    0,  496, },
    {  773,  101,  421,  199, },
};



const s16 init_sig_coeff_flag[2][NUM_CTX_SIG_COEFF_FLAG] =
{
    {  387,   98,  233,  346,  717,  306,  233,   37,  321,  293,  244,   37,  329,  645,  408,  493,  164,  781,  101,  179,  369,  871,  585,  244,  361,  147,  416,  408,  628,  352,  406,  502,  566,  466,   54,   97,  521,  113,  147,  519,   36,  297,  132,  457,  308,  231,  534, },
    {   66,   34,  241,  321,  293,  113,   35,   83,  226,  519,  553,  229,  751,  224,  129,  133,  162,  227,  178,  165,  532,  417,  357,   33,  489,  199,  387,  939,  133,  515,   32,  131,    3,  305,  579,  323,   65,   99,  425,  453,  291,  329,  679,  683,  391,  751,   51, },
};

const s16 init_coeff_abs_level_greaterAB_flag[2][NUM_CTX_GTX] =
{
    {   40,  225,  306,  272,   85,  120,  389,  664,  209,  322,  291,  536,  338,  709,   54,  244,   19,  566, },
    {   38,  352,  340,   19,  305,  258,   18,   33,  209,  773,  517,  406,  719,  741,  613,  295,   37,  498, },
};

const s16 init_last_sig_coeff_x_prefix[2][NUM_CTX_LAST_SIG_COEFF] =
{
    {  762,  310,  288,  828,  342,  451,  502,   51,   97,  416,  662,  890,  340,  146,   20,  337,  468,  975,  216,   66,   54, },
    {  892,   84,  581,  600,  278,  419,  372,  568,  408,  485,  338,  632,  666,  732,   17,  178,  180,  585,  581,   34,  257, },
};

const s16 init_last_sig_coeff_y_prefix[2][NUM_CTX_LAST_SIG_COEFF] =
{
    {   81,  440,    4,  534,  406,  226,  370,  370,  259,   38,  598,  792,  860,  312,   88,  662,  924,  161,  248,   20,   54, },
    {  470,  376,  323,  276,  602,   52,  340,  600,  376,  378,  598,  502,  730,  538,   17,  195,  504,  378,  320,  160,  572, },
};


const s16 init_btt_split_flag[2][NUM_CTX_BTT_SPLIT_FLAG] =
{
    {  145,  560,  528,  308,  594,  560,  180,  500,  626,   84,  406,  662,  320,   36,  340, },
    {  536,  726,  594,   66,  338,  528,  258,  404,  464,   98,  342,  370,  384,  256,   65, },
};

const s16 init_btt_split_dir[2][NUM_CTX_BTT_SPLIT_DIR] =
{
    {    0,  417,  389,   99,    0, },
    {    0,  128,   81,   49,    0, },
};

const s16 init_btt_split_type[2][NUM_CTX_BTT_SPLIT_TYPE] =
{
    {  257, },
    {  225, },
};

const s16 init_affine_flag[2][NUM_CTX_AFFINE_FLAG] =
{
    {    0,    0, },
    {  320,  210, },
};

const s16 init_affine_mode[2][NUM_CTX_AFFINE_MODE] =
{
    {    0, },
    {  225, },
};

const s16 init_affine_mrg[2][NUM_CTX_AFFINE_MRG] =
{
    {    0,    0,    0,    0,    0, },
    {  193,  129,   32,  323,    0, },
};

const s16 init_affine_mvd_flag[2][NUM_CTX_AFFINE_MVD_FLAG] =
{
    {    0,    0, },
    {  547,  645, },
};

const s16 init_suco_flag[2][NUM_CTX_SUCO_FLAG] =
{
    {    0,    0,    0,    0,    0,    0,  545,    0,  481,  515,    0,   32,    0,    0, },
    {    0,    0,    0,    0,    0,    0,  577,    0,  481,    2,    0,   97,    0,    0, },
};

const s16 init_alf_ctb_flag[2][NUM_CTX_ALF_CTB_FLAG] =
{
    {    0, },
    {    0, },
};



const s16 init_ats_intra_cu[2][NUM_CTX_ATS_INTRA_CU_FLAG] =
{
    {  999, },
    { 1003, },
};

const s16 init_ats_mode[2][NUM_CTX_ATS_MODE_FLAG] =
{
    {  512, },
    {  673, },
};

const s16 init_ats_cu_inter_flag[2][NUM_CTX_ATS_INTER_FLAG] =
{
    {    0,    0, },
    {    0,    0, },
};

const s16 init_ats_cu_inter_quad_flag[2][NUM_CTX_ATS_INTER_QUAD_FLAG] =
{
    {    0, },
    {    0, },
};

const s16 init_ats_cu_inter_hor_flag[2][NUM_CTX_ATS_INTER_HOR_FLAG] =
{
    {    0,    0,    0, },
    {    0,    0,    0, },
};

const s16 init_ats_cu_inter_pos_flag[2][NUM_CTX_ATS_INTER_POS_FLAG] =
{
    {    0, },
    {    0, },
};
const u8 ALPHA_TABLE[52] = { 0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,4,4,5,6,  7,8,9,10,12,13,15,17,  20,22,25,28,32,36,40,45,  50,56,63,71,80,90,101,113,  127,144,162,182,203,226,255,255 };
const u8 BETA_TABLE[52] = { 0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,2,2,2,3,  3,3,3, 4, 4, 4, 6, 6,   7, 7, 8, 8, 9, 9,10,10,  11,11,12,12,13,13, 14, 14,   15, 15, 16, 16, 17, 17, 18, 18 };
const u8 CLIP_TAB[52][5] =
{
    { 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0 },{ 0, 0, 0, 1, 1 },{ 0, 0, 0, 1, 1 },{ 0, 0, 0, 1, 1 },{ 0, 0, 0, 1, 1 },{ 0, 0, 1, 1, 1 },{ 0, 0, 1, 1, 1 },{ 0, 1, 1, 1, 1 },
    { 0, 1, 1, 1, 1 },{ 0, 1, 1, 1, 1 },{ 0, 1, 1, 1, 1 },{ 0, 1, 1, 2, 2 },{ 0, 1, 1, 2, 2 },{ 0, 1, 1, 2, 2 },{ 0, 1, 1, 2, 2 },{ 0, 1, 2, 3, 3 },
    { 0, 1, 2, 3, 3 },{ 0, 2, 2, 3, 3 },{ 0, 2, 2, 4, 4 },{ 0, 2, 3, 4, 4 },{ 0, 2, 3, 4, 4 },{ 0, 3, 3, 5, 5 },{ 0, 3, 4, 6, 6 },{ 0, 3, 4, 6, 6 },
    { 0, 4, 5, 7, 7 },{ 0, 4, 5, 8, 8 },{ 0, 4, 6, 9, 9 },{ 0, 5, 7,10,10 },{ 0, 6, 8,11,11 },{ 0, 6, 8,13,13 },{ 0, 7,10,14,14 },{ 0, 8,11,16,16 },
    { 0, 9,12,18,18 },{ 0,10,13,20,20 },{ 0,11,15,23,23 },{ 0,13,17,25,25 }
};

const int g_group_idx[MAX_TR_SIZE] = { 0,1,2,3,4,4,5,5,6,6,6,6,7,7,7,7,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9, 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11 };

const int g_go_rice_para_coeff[32] =
{
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3
};

const int g_min_in_group[LAST_SIGNIFICANT_GROUPS] = { 0,1,2,3,4,6,8,12,16,24,32,48,64,96 };

const int g_go_rice_range[MAX_GR_ORDER_RESIDUAL] =
{
    6, 5, 6, COEF_REMAIN_BIN_REDUCTION, COEF_REMAIN_BIN_REDUCTION, COEF_REMAIN_BIN_REDUCTION, COEF_REMAIN_BIN_REDUCTION, COEF_REMAIN_BIN_REDUCTION, COEF_REMAIN_BIN_REDUCTION, COEF_REMAIN_BIN_REDUCTION
};

int g_lumaInvScaleLUT[DRA_LUT_MAXSIZE];               // LUT for luma and correspionding QP offset
double g_chromaInvScaleLUT[2][DRA_LUT_MAXSIZE];               // LUT for chroma scales
int g_intChromaInvScaleLUT[2][DRA_LUT_MAXSIZE];               // LUT for chroma scales


// input to table is in the range 0<input<256, as a result of multiplication of 2 scales with max value of <16.
const int g_dra_chroma_qp_offset_tbl[NUM_CHROMA_QP_OFFSET_LOG] =  // Approximation of Log function at accuracy 1<<9 bits
{
  0, 1, 1, 1, 1, 1, 2, 2, 3, 4, 4, 6, 7, 9, 11, 14, 18, 23, 29, 36, 45,
  57, 72, 91, 114, 144, 181, 228, 287, 362, 456, 575, 724, 912, 1149, 1448, 1825, 2299,
  2896, 3649, 4598, 5793, 7298, 9195, 11585, 14596, 18390, 23170, 29193, 36781, 46341, 58386, 73562, 92682, 116772
};

// input to this table is deltaQP introduced to QPi (lumaQP+chromaQPoffset) by the chromaQPOffset table. Currently max offset 6 is supported, increase to 12 (?).
const int g_dra_exp_nom_v2[NUM_CHROMA_QP_SCALE_EXP] =   // Approximation of exp function at accuracy 1 << 9 bits
{
    128, 144, 161, 181, 203, 228, 256, 287, 322, 362, 406, 456, 512, 574, 645, 724, 812, 912, 1024, 1149, 1290, 1448, 1625, 1825, 2048
};

