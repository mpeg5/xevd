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

#ifndef _XEVDM_DEF_H_
#define _XEVDM_DEF_H_

#include "xevd.h"
#include "xevd_def.h"

/* Profiles definitions */
#define PROFILE_MAIN                                 1

#define PROFILE_STILL_PIC_MAIN                       3

#define ENC_SUCO_FAST_CONFIG                         1  /* fast config: 1(low complexity), 2(medium complexity), 4(high_complexity) */

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                              SIMD Optimizations                            //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
#if X86_SSE

#define OPT_SIMD_DMVR_MR_SAD                         1
#else

#define OPT_SIMD_DMVR_MR_SAD                         0
#endif

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                         Certain Tools Parameters                           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

/* Partitioning (START) */
#define INC_QT_DEPTH(qtd, smode)           (smode == SPLIT_QUAD? (qtd + 1) : qtd)
#define INC_BTT_DEPTH(bttd, smode, bound)  (bound? 0: (smode != SPLIT_QUAD? (bttd + 1) : bttd))

/* MMVD (START) */
#define MMVD_BASE_MV_NUM                   4
#define MMVD_DIST_NUM                      8
#define MMVD_MAX_REFINE_NUM               (MMVD_DIST_NUM * 4)
#define MMVD_SKIP_CON_NUM                  4
#define MMVD_GRP_NUM                       3
#define MMVD_THRESHOLD                     1.5
/* MMVD (END) */

/* AMVR (START) */
#define MAX_NUM_MVR                        5
#define FAST_MVR_IDX                       2
#define SKIP_MVR_IDX                       1
#define MAX_NUM_BI                         3
/* AMVR (END)  */

#define DBF_ADDB_BS_INTRA_STRONG           4
#define DBF_ADDB_BS_INTRA                  3
#define DBF_ADDB_BS_CODED                  2
#define DBF_ADDB_BS_DIFF_REFS              1
#define DBF_ADDB_BS_OTHERS                 0
/* DBF (END) */

/* MERGE (START) */
#define MERGE_MVP                          1
#define INCREASE_MVP_NUM                   1
/* MERGE (END) */

/* DMVR (START) */
#define DMVR_SUBCU_SIZE                    16
#define DMVR_ITER_COUNT                    2
#define REF_PRED_POINTS_NUM                9
#define REF_PRED_EXTENTION_PEL_COUNT       1
#define REF_PRED_POINTS_PER_LINE_NUM       3
#define REF_PRED_POINTS_LINES_NUM          3
#define DMVR_NEW_VERSION_ITER_COUNT        8
#define REF_PRED_POINTS_CROSS              5

enum SAD_POINT_INDEX
{
    SAD_NOT_AVAILABLE = -1,
    SAD_BOTTOM = 0,
    SAD_TOP,
    SAD_RIGHT,
    SAD_LEFT,
    SAD_TOP_LEFT,
    SAD_TOP_RIGHT,
    SAD_BOTTOM_LEFT,
    SAD_BOTTOM_RIGHT,
    SAD_CENTER,
    SAD_COUNT
};


#define DEC_XEVD_MAX_TASK_CNT           8
#define MC_PRECISION_ADD                2 
#define SCALE_NUMFBITS                  9   // # frac. bits for scale (Y/Cb/Cr)
#define INVSCALE_NUMFBITS               9   // # frac. bits for inv. scale (Y/Cb/Cr)
#define OFFSET_NUMFBITS                 7   // # frac. bits for offset (Y/Cb/Cr)

#define DRA_LUT_MAXSIZE                1024

#define NUM_CHROMA_QP_OFFSET_LOG       55
#define NUM_CHROMA_QP_SCALE_EXP        25

#define MAX_QP_TABLE_SIZE              58
#define MAX_QP_TABLE_SIZE_EXT          94




/* DMVR (END) */

/* HISTORY (START) */

/* ADMVP (END) */

/* ALF (START) */
#define MAX_NUM_TLAYER                     6
#define MAX_NUM_ALFS_PER_TLAYER            6
#define ALF_LAMBDA_SCALE                   17

#define MAX_NUM_ALF_CLASSES                25
#define MAX_NUM_ALF_LUMA_COEFF             13
#define MAX_NUM_ALF_CHROMA_COEFF           7
#define MAX_ALF_FILTER_LENGTH              7
#define MAX_NUM_ALF_COEFF                 (MAX_ALF_FILTER_LENGTH * MAX_ALF_FILTER_LENGTH / 2 + 1)
/* ALF (END) */

/* AFFINE (START) */
// AFFINE Constant
#define VER_NUM                            4
#define AFFINE_MAX_NUM_LT                  3 ///< max number of motion candidates in top-left corner
#define AFFINE_MAX_NUM_RT                  3 ///< max number of motion candidates in top-right corner
#define AFFINE_MAX_NUM_LB                  2 ///< max number of motion candidates in left-bottom corner
#define AFFINE_MAX_NUM_RB                  2 ///< max number of motion candidates in right-bottom corner
#define AFFINE_MIN_BLOCK_SIZE              4 ///< Minimum affine MC block size
#define AFF_MAX_NUM_MVP                    2 // maximum affine inter candidates
#define AFF_MAX_CAND                       5 // maximum affine merge candidates
#define AFF_MODEL_CAND                     5 // maximum affine model based candidates

// AFFINE ME configuration (non-normative)
#define AF_ITER_UNI                        7 // uni search iteration time
#define AF_ITER_BI                         5 // bi search iteration time
#define AFFINE_BI_ITER                     1
#if X86_SSE
#define AFFINE_SIMD                        1
#else
#define AFFINE_SIMD                        0
#endif

/* EIF (START) */
#define AFFINE_ADAPT_EIF_SIZE                                   8
#define EIF_SUBBLOCK_SIZE                                       4
#define EIF_NUM_ALLOWED_FETCHED_LINES_FOR_THE_FIRST_LINE        3
#define EIF_MV_PRECISION_BILINEAR                               5
#define BOUNDING_BLOCK_MARGIN                                   7
#define MEMORY_BANDWIDTH_THRESHOLD                              (8 + 2 + BOUNDING_BLOCK_MARGIN) / 8
#define MAX_MEMORY_ACCESS_BI                                    72
/* EIF (END) */

/* AFFINE (END) */

/* ALF (START) */
#define m_MAX_SCAN_VAL                     11
#define m_MAX_EXP_GOLOMB                   16
#define MAX_NUM_ALF_LUMA_COEFF             13
#define MAX_NUM_ALF_CLASSES                25
#define MAX_NUM_ALF_LUMA_COEFF             13
#define MAX_NUM_ALF_CHROMA_COEFF           7
#define MAX_ALF_FILTER_LENGTH              7
#define MAX_NUM_ALF_COEFF                 (MAX_ALF_FILTER_LENGTH * MAX_ALF_FILTER_LENGTH / 2 + 1)

#define APS_MAX_NUM                        32
#define APS_MAX_NUM_IN_BITS                5
#define APS_TYPE_ID_BITS                   3

// The structure below must be aligned to identical structure in xevd_alf.c!
typedef struct _XEVD_ALF_FILTER_SHAPE
{
    int filterType;
    int filterLength;
    int numCoeff;
    int filterSize;
    int pattern[25];
    int weights[14];
    int golombIdx[14];
    int patternToLargeFilter[13];
} XEVD_ALF_FILTER_SHAPE;
/* ALF (END) */

/* TRANSFORM PACKAGE (START) */
#define ATS_INTRA_FAST                     0
#if ATS_INTRA_FAST
#define ATS_INTER_INTRA_SKIP_THR           1.05
#define ATS_INTRA_Y_NZZ_THR                1.00
#define ATS_INTRA_IPD_THR                  1.10
#endif

#define ATS_INTER_SL_NUM                   16
#define get_ats_inter_idx(s)               (s & 0xf)
#define get_ats_inter_pos(s)               ((s>>4) & 0xf)
#define get_ats_inter_info(idx, pos)       (idx + (pos << 4))
#define is_ats_inter_horizontal(idx)       (idx == 2 || idx == 4)
#define is_ats_inter_quad_size(idx)        (idx == 3 || idx == 4)
/* TRANSFORM PACKAGE (END) */

/* ADCC (START) */
#define LOG2_RATIO_GTA                     1
#define LOG2_RATIO_GTB                     4
#define LOG2_CG_SIZE                       4
#define MLS_GRP_NUM                        1024
#define CAFLAG_NUMBER                      8
#define CBFLAG_NUMBER                      1

#define SBH_THRESHOLD                      4
#define MAX_GR_ORDER_RESIDUAL              10
#define COEF_REMAIN_BIN_REDUCTION          3
#define LAST_SIGNIFICANT_GROUPS            14

/* ADCC (END) */

/* IBC (START) */
#define IBC_SEARCH_RANGE                     64
#define IBC_NUM_CANDIDATES                   64
#define IBC_FAST_METHOD_BUFFERBV             0X01
#define IBC_FAST_METHOD_ADAPTIVE_SEARCHRANGE 0X02
/* IBC (END) */

/* HDR (START) */
#define HDR_MD5_CHECK                      1


/* number of MVP candidates */
#if INCREASE_MVP_NUM
#define MAX_NUM_MVP_SMALL_CU               4
#define MAXM_NUM_MVP                        6
#define NUM_SAMPLES_BLOCK                  32 // 16..64
#define ORG_MAX_NUM_MVP                    4
#else
#define MAXM_NUM_MVP                        4
#endif

/* maximum picture buffer size */
#define DRA_FRAME 1
#define MAXM_PB_SIZE                       (XEVD_MAX_NUM_REF_PICS + EXTRA_FRAME + DRA_FRAME)

#define MODE_SKIP_MMVD                     4
#define MODE_DIR_MMVD                      5
#define MODE_IBC                           6
/*****************************************************************************
* prediction direction
*****************************************************************************/


#define PRED_SKIP_MMVD                     5
#define PRED_DIR_MMVD                      6
/* IBC pred direction, look current picture as reference */
#define PRED_IBC                           7
#define PRED_FL0_BI                        10
#define PRED_FL1_BI                        11
#define PRED_BI_REF                        12
#define ORG_PRED_NUM                       13
#define PRED_NUM                          (ORG_PRED_NUM * MAX_NUM_MVR)

#define START_NUM                         (ORG_PRED_NUM * MAX_NUM_MVR)

#define AFF_L0                            (START_NUM)          // 5  7  42
#define AFF_L1                            (START_NUM + 1)      // 6  8  43
#define AFF_BI                            (START_NUM + 2)      // 7  9  44
#define AFF_SKIP                          (START_NUM + 3)      // 8  10 45
#define AFF_DIR                           (START_NUM + 4)      // 9  11 46

#define AFF_6_L0                          (START_NUM + 5)      // 10 12 47
#define AFF_6_L1                          (START_NUM + 6)      // 11 13 48
#define AFF_6_BI                          (START_NUM + 7)      // 12 14 49

#undef PRED_NUM
#define PRED_NUM                          (START_NUM + 8)


#define IBC_MAX_CU_LOG2                      6 /* max block size for ibc search in unit of log2 */
//#define IBC_MAX_CAND_SIZE                    (1 << IBC_MAX_CU_LOG2)


/* set dmvr flag */
#define MCU_SET_DMVRF(m)         (m)=((m)|(1<<25))
/* get dmvr flag */
#define MCU_GET_DMVRF(m)         (int)(((m)>>25) & 1)
/* clear dmvr flag */
#define MCU_CLR_DMVRF(m)         (m)=((m) & (~(1<<25)))

/* set ibc mode flag */
#define MCU_SET_IBC(m)          (m)=((m)|(1<<26))
/* get ibc mode flag */
#define MCU_GET_IBC(m)          (int)(((m)>>26) & 1)
/* clear ibc mode flag */
#define MCU_CLR_IBC(m)          (m)=((m) & (~(1<<26)))

/*
- [8:9] : affine vertex number, 00: 1(trans); 01: 2(affine); 10: 3(affine); 11: 4(affine)
*/

/* set affine CU mode to map */
#define MCU_SET_AFF(m, v)       (m)=((m & 0xFFFFFCFF)|((v)&0x03)<<8)
/* get affine CU mode from map */
#define MCU_GET_AFF(m)          (int)(((m)>>8)&0x03)
/* clear affine CU mode to map */
#define MCU_CLR_AFF(m)          (m)=((m) & 0xFFFFFCFF)

/*****************************************************************************
* macros for affine CU map

- [ 0: 7] : log2 cu width
- [ 8:15] : log2 cu height
- [16:23] : x offset
- [24:31] : y offset
*****************************************************************************/
#define MCU_SET_AFF_LOGW(m, v)       (m)=((m & 0xFFFFFF00)|((v)&0xFF)<<0)
#define MCU_SET_AFF_LOGH(m, v)       (m)=((m & 0xFFFF00FF)|((v)&0xFF)<<8)
#define MCU_SET_AFF_XOFF(m, v)       (m)=((m & 0xFF00FFFF)|((v)&0xFF)<<16)
#define MCU_SET_AFF_YOFF(m, v)       (m)=((m & 0x00FFFFFF)|((v)&0xFF)<<24)

#define MCU_GET_AFF_LOGW(m)          (int)(((m)>>0)&0xFF)
#define MCU_GET_AFF_LOGH(m)          (int)(((m)>>8)&0xFF)
#define MCU_GET_AFF_XOFF(m)          (int)(((m)>>16)&0xFF)
#define MCU_GET_AFF_YOFF(m)          (int)(((m)>>24)&0xFF)

/* set MMVD skip flag to map */
#define MCU_SET_MMVDS(m)            (m)=((m)|(1<<2))
/* get MMVD skip flag from map */
#define MCU_GET_MMVDS(m)            (int)(((m)>>2) & 1)
/* clear MMVD skip flag in map */
#define MCU_CLR_MMVDS(m)            (m)=((m) & (~(1<<2)))



/*****************************************************************************
* picture manager for DPB in decoder and RPB in encoder
*****************************************************************************/
typedef struct _XEVDM_PM
{
    /* picture store (including reference and non-reference) */
    XEVD_PIC        * pic[MAXM_PB_SIZE];
    /* address of reference pictures */
    XEVD_PIC        * pic_ref[XEVD_MAX_NUM_REF_PICS];
    /* maximum reference picture count */
    u8               max_num_ref_pics;
    /* current count of available reference pictures in PB */
    u8               cur_num_ref_pics;
    /* number of reference pictures */
    u8               num_refp[REFP_NUM];
    /* next output POC */
    s32              poc_next_output;
    /* POC increment */
    u8               poc_increase;
    /* max number of picture buffer */
    u8               max_pb_size;
    /* current picture buffer size */
    u8               cur_pb_size;
    /* address of leased picture for current decoding/encoding buffer */
    XEVD_PIC        * pic_lease;
    /* picture buffer allocator */
    PICBUF_ALLOCATOR pa;
} XEVDM_PM;

/*****************************************************************************
* slice header
*****************************************************************************/
typedef struct _XEVD_ALF_SLICE_PARAM
{
    BOOL is_ctb_alf_on;
    u8 *alf_ctu_enable_flag;
    u8 *alf_ctu_enable_flag_chroma;
    u8 *alf_ctu_enable_flag_chroma2;

    BOOL                         enabled_flag[3];                                          // alf_slice_enable_flag, alf_chroma_idc
    int                          luma_filter_type;                                          // filter_type_flag
    BOOL                         chroma_ctb_present_flag;                                    // alf_chroma_ctb_present_flag
    short                        luma_coeff[MAX_NUM_ALF_CLASSES * MAX_NUM_ALF_LUMA_COEFF]; // alf_coeff_luma_delta[i][j]
    short                        chroma_coeff[MAX_NUM_ALF_CHROMA_COEFF];                   // alf_coeff_chroma[i]
    short                        filter_coeff_delta_idx[MAX_NUM_ALF_CLASSES];                // filter_coeff_delta[i]
    BOOL                         filter_coeff_flag[MAX_NUM_ALF_CLASSES];                    // filter_coefficient_flag[i]
    int                          num_luma_filters;                                          // number_of_filters_minus1 + 1
    BOOL                         coeff_delta_flag;                                          // alf_coefficients_delta_flag
    BOOL                         coeff_delta_pred_mode_flag;                                  // coeff_delta_pred_mode_flag

    int fixed_filter_pattern;
    int fixed_filter_idx[MAX_NUM_ALF_CLASSES];
    u8  fixed_filter_usage_flag[MAX_NUM_ALF_CLASSES];
    int t_layer;
    BOOL temporal_alf_flag;
    int prev_idx;
    int prev_idx_comp[2];
    BOOL reset_alf_buffer_flag;
    BOOL store2_alf_buffer_flag;
    BOOL chroma_filter_present;

} XEVD_ALF_SLICE_PARAM;



typedef struct _XEVD_APS_GEN
{
    int                         signal_flag;
    int                         aps_type_id;                    // adaptation_parameter_set_type_id
    int                         aps_id;                    // adaptation_parameter_set_id
    void                         * aps_data;
} XEVD_APS_GEN;
typedef struct _XEVD_APS
{
    int                         aps_id;                    // adaptation_parameter_set_id
    int                         aps_id_y;
    int                         aps_id_ch;
    XEVD_ALF_SLICE_PARAM        alf_aps_param;              // alf data
} XEVD_APS;

typedef struct _XEVDM_SH
{
    u32              alf_on;
    u32              mmvd_group_enable_flag;
    u8               ctb_alf_on;
    int              aps_signaled;
    int              aps_id_y;
    int              aps_id_ch;
    XEVD_APS*         aps;
    XEVD_ALF_SLICE_PARAM alf_sh_param;
    u32              alf_chroma_idc;
    u32              chroma_alf_enabled_flag;
    u32              chroma_alf_enabled2_flag;
    u32              alf_chroma_map_signalled;
    u32              alf_chroma2_map_signalled;
    int              aps_id_ch2;
} XEVDM_SH;

/*****************************************************************************
* Tiles
*****************************************************************************/


typedef enum _TREE_TYPE
{
    TREE_LC = 0,
    TREE_L = 1,
    TREE_C = 2,
} TREE_TYPE;

typedef enum _MODE_CONS
{
    eOnlyIntra,
    eOnlyInter,
    eAll
} MODE_CONS;

typedef struct _TREE_CONS
{
    BOOL            changed;
    TREE_TYPE       tree_type;
    MODE_CONS       mode_cons;
} TREE_CONS;

typedef struct _TREE_CONS_NEW
{
    TREE_TYPE       tree_type;
    MODE_CONS       mode_cons;
} TREE_CONS_NEW;



#define DMVR_PAD_LENGTH                                        2
#define EXTRA_PIXELS_FOR_FILTER                                7 // Maximum extraPixels required for final MC based on fiter size
#define PAD_BUFFER_STRIDE                               ((MAX_CU_SIZE + EXTRA_PIXELS_FOR_FILTER + (DMVR_ITER_COUNT * 2)))

#define EIF_MV_PRECISION_INTERNAL                                       (2 + MAX_CU_LOG2 + 0) //2 + MAX_CU_LOG2 is MV precision in regular affine

#if EIF_MV_PRECISION_INTERNAL > 14 || EIF_MV_PRECISION_INTERNAL < 9
#error "Invalid EIF_MV_PRECISION_INTERNAL"
#endif

#if EIF_MV_PRECISION_BILINEAR > EIF_MV_PRECISION_INTERNAL
#error "EIF_MV_PRECISION_BILINEAR should be less than EIF_MV_PRECISION_INTERNAL"
#endif

#if EIF_MV_PRECISION_BILINEAR < 3
#error "EIF_MV_PRECISION_BILINEAR is to small"
#endif

typedef struct _XEVDM_CTX XEVDM_CTX;

typedef struct _XEVDM_CORE
{
    XEVD_CORE core;

    pel            dmvr_template[MAX_CU_DIM];
    pel            dmvr_half_pred_interpolated[REFP_NUM][(MAX_CU_SIZE + 1) * (MAX_CU_SIZE + 1)];
    pel            dmvr_ref_pred_interpolated[REFP_NUM][(MAX_CU_SIZE + ((DMVR_NEW_VERSION_ITER_COUNT + 1) * REF_PRED_EXTENTION_PEL_COUNT)) * (MAX_CU_SIZE + ((DMVR_NEW_VERSION_ITER_COUNT + 1) * REF_PRED_EXTENTION_PEL_COUNT))];

    pel  dmvr_padding_buf[2][N_C][PAD_BUFFER_STRIDE * PAD_BUFFER_STRIDE];
    /* dmvr refined motion vector for current CU */
    s16             dmvr_mv[MAX_CU_CNT_IN_LCU][REFP_NUM][MV_D];

    u8             dmvr_enable;
    s16            affine_mv[REFP_NUM][VER_NUM][MV_D];
    u8             affine_flag;

    u8             ibc_flag;
    u8             ibc_skip_flag;
    u8             ibc_merge_flag;
    /* SUCO flag for current LCU */
    s8          (* suco_flag)[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU];

    s16            mmvd_idx;
    u8             mmvd_flag;
    /* ATS_INTRA flags */
    u8             ats_intra_cu;
    u8             ats_intra_mode_h;
    u8             ats_intra_mode_v;

    /* ATS_INTER info (index + position)*/
    u8             ats_inter_info;
    u8             dmvr_flag;
    // spatial neighboring MV of affine block
    s8             refi_sp[REFP_NUM];
    s16            mv_sp[REFP_NUM][MV_D];
    int            affine_bzero[REFP_NUM];
    s16            affine_mvd[REFP_NUM][3][MV_D];

    TREE_CONS      tree_cons;

} XEVDM_CORE;
/******************************************************************************
* CONTEXT used for decoding process.
*
* All have to be stored are in this structure.
*****************************************************************************/
typedef struct _SIG_PARAM_DRA SIG_PARAM_DRA;

struct _XEVDM_CTX
{
    XEVD_CTX bctx;
    /* adaptive loop filter */
    void                  * alf;
    XEVDM_SH                 sh;
    XEVDM_PM                 dpm;
    /* adaptation parameter set */
    XEVD_APS                 aps;
    u8                      aps_temp;
    void                  * pps_dra_params;
    XEVD_APS_GEN          * aps_gen_array;
    SIG_PARAM_DRA         * dra_array;
    s8(*map_suco)[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU];


    /* decoded motion vector for every blocks */
    s16(*map_unrefined_mv)[REFP_NUM][MV_D];
    u32                   * map_affine;
    /* ats_inter info map */
    u8                    * map_ats_inter;

    /* function address of ALF */
    int(*fn_alf)(XEVD_CTX * ctx, XEVD_PIC * pic);
};

/* prototypes of internal functions */
int  xevdm_platform_init(XEVD_CTX * ctx);
void xevdm_platform_deinit(XEVD_CTX * ctx);
int  xevdm_ready(XEVD_CTX * ctx);
void xevdm_flush(XEVD_CTX * ctx);
int  xevdm_deblock(void * args);

int  xevdm_dec_slice(XEVD_CTX * ctx, XEVD_CORE * core);



#include "xevdm_util.h"
#include "xevdm_eco.h"
#include "xevdm_picman.h"

#include "xevdm_tbl.h"
#include "xevdm_recon.h"
#include "xevdm_ipred.h"
#include "xevdm_tbl.h"
#include "xevdm_itdq.h"
#include "xevdm_picman.h"
#include "xevdm_mc.h"
#include "xevd_util.h"
#include "xevdm_dra.h"
#endif /* _XEVD_DEF_H_ */
