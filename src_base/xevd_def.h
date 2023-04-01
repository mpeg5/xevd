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

#ifndef _XEVD_DEF_H_
#define _XEVD_DEF_H_

#include "xevd.h"
#include "xevd_port.h"
#include "xevd_tp.h"

/* xevd decoder magic code */
#define XEVD_MAGIC_CODE                              0x45565944

/* Profiles definitions */
#define PROFILE_BASELINE                             0
#define PROFILE_STILL_PIC_BASELINE                   2
#define GET_QP(qp,dqp)                               ((qp + dqp + 52) % 52)
#define GET_LUMA_QP(qp, qp_bd_offset)                (qp + 6 * qp_bd_offset)

#define MC_PRECISION                                 4

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                              SIMD Optimizations                            //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
#if X86_SSE
#define OPT_SIMD_MC_L                                1
#define OPT_SIMD_MC_C                                1
#else
#define OPT_SIMD_MC_L                                0
#define OPT_SIMD_MC_C                                0
#endif

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                         Certain Tools Parameters                           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#define MAX_NUM_PPS                        64

/* Partitioning (START) */
#define MAX_SPLIT_NUM                      6
#define SPLIT_CHECK_NUM                    6
/* Partitioning (END) */

/* CABAC (START) */
#define PROB_INIT                         (512) /* 1/2 of initialization with mps = 0 */
/* CABAC (END) */

/* Multiple Referene (START) */
#define MAX_NUM_ACTIVE_REF_FRAME_B         2  /* Maximum number of active reference frames for RA condition */
#define MAX_NUM_ACTIVE_REF_FRAME_LDB       4  /* Maximum number of active reference frames for LDB condition */
#define MVP_SCALING_PRECISION              5  /* Scaling precision for motion vector prediction (2^MVP_SCALING_PRECISION) */
/* Multiple Reference (END) */

// Constants
#define DBF_LENGTH                         4
#define DBF_LENGTH_CHROMA                  2
/* DBF (END) */

/* HISTORY (START) */
#define ALLOWED_CHECKED_NUM                23
#define ALLOWED_CHECKED_NUM_SMALL_CU       15
#define ALLOWED_CHECKED_AMVP_NUM           4

#define COEF_SCAN_ZIGZAG                   0
#define COEF_SCAN_DIAG                     1
#define COEF_SCAN_DIAG_CG                  2
#define COEF_SCAN_TYPE_NUM                 3

/* CABAC ZERO WORD (START) */
#define CABAC_ZERO_WORD                    1
#if CABAC_ZERO_WORD
#define CABAC_ZERO_PARAM                   32
#endif
/* CABAC ZERO WORD (END) */

/* Common routines (START) */

typedef int BOOL;
#define TRUE  1
#define FALSE 0
/* Common stuff (END) */

/* For debugging (START) */
#define USE_DRAW_PARTITION_DEC             0
#define ENC_DEC_TRACE                      0

#if ENC_DEC_TRACE
#define TRACE_COEFFS                       0 ///< Trace coefficients
#define TRACE_BIN                          0 //!< trace each bin
#define TRACE_START_POC                    0 //!< POC of frame from which we start to write output tracing information
#define TRACE_COSTS                        0 //!< Trace cost information
#define TRACE_REMOVE_COUNTER               0 //!< Remove trace counter
#define TRACE_ADDITIONAL_FLAGS             0
#define TRACE_DBF                          0 //!< Trace only DBF
#define TRACE_HLS                          0 //!< Trace SPS, PPS, APS, Slice Header, etc.

extern FILE *fp_trace;
extern int fp_trace_print;
extern int fp_trace_counter;
#if TRACE_START_POC
extern int fp_trace_started;
#endif

#define XEVD_TRACE_SET(A) fp_trace_print=A
#define XEVD_TRACE_STR(STR) if(fp_trace_print) { fprintf(fp_trace, STR); fflush(fp_trace); }
#define XEVD_TRACE_DOUBLE(DOU) if(fp_trace_print) { fprintf(fp_trace, "%g", DOU); fflush(fp_trace); }
#define XEVD_TRACE_INT(INT) if(fp_trace_print) { fprintf(fp_trace, "%d ", INT); fflush(fp_trace); }
#define XEVD_TRACE_INT_HEX(INT) if(fp_trace_print) { fprintf(fp_trace, "0x%x ", INT); fflush(fp_trace); }
#if TRACE_REMOVE_COUNTER
#define XEVD_TRACE_COUNTER
#else
#define XEVD_TRACE_COUNTER  XEVD_TRACE_INT(fp_trace_counter++); XEVD_TRACE_STR("\t")
#endif
#define XEVD_TRACE_MV(X, Y) if(fp_trace_print) { fprintf(fp_trace, "(%d, %d) ", X, Y); fflush(fp_trace); }
#define XEVD_TRACE_FLUSH    if(fp_trace_print) fflush(fp_trace)
#else
#define XEVD_TRACE_SET(A)
#define XEVD_TRACE_STR(str)
#define XEVD_TRACE_DOUBLE(DOU)
#define XEVD_TRACE_INT(INT)
#define XEVD_TRACE_INT_HEX(INT)
#define XEVD_TRACE_COUNTER
#define XEVD_TRACE_MV(X, Y)
#define XEVD_TRACE_FLUSH
#endif
/* For debugging (END) */

#define PEL2BYTE(pel,cs)                      ((pel)*(((XEVD_CS_GET_BIT_DEPTH(cs)) + 7)>>3))
#define STRIDE_IMGB2PIC(s_imgb)            ((s_imgb)>>1)

#define Y_C                                0  /* Y luma */
#define U_C                                1  /* Cb Chroma */
#define V_C                                2  /* Cr Chroma */
#define N_C                                3  /* number of color component */

#define REFP_0                             0
#define REFP_1                             1
#define REFP_NUM                           2

/* Deblock directions */
#define DBK_VER 0
#define DBK_HOR 1
#define DBK_MAX 2

/* X direction motion vector indicator */
#define MV_X                               0
/* Y direction motion vector indicator */
#define MV_Y                               1
/* Maximum count (dimension) of motion */
#define MV_D                               2
/* Reference index indicator */
#define REFI                               2

#define N_REF                              3  /* left, up, right */
#define NUM_NEIB                           4  /* LR: 00, 10, 01, 11*/

#define MAX_CU_LOG2                        7
#define MIN_CU_LOG2                        2
#define MAX_CU_SIZE                       (1 << MAX_CU_LOG2)
#define MIN_CU_SIZE                       (1 << MIN_CU_LOG2)
#define MAX_CU_DIM                        (MAX_CU_SIZE * MAX_CU_SIZE)
#define MIN_CU_DIM                        (MIN_CU_SIZE * MIN_CU_SIZE)
#define MAX_CU_DEPTH                       10  /* 128x128 ~ 4x4 */
#define NUM_CU_DEPTH                      (MAX_CU_DEPTH + 1)

#define MAX_TR_LOG2                        6  /* 64x64 */
#define MIN_TR_LOG2                        1  /* 2x2 */
#define MAX_TR_SIZE                       (1 << MAX_TR_LOG2)
#define MIN_TR_SIZE                       (1 << MIN_TR_LOG2)
#define MAX_TR_DIM                        (MAX_TR_SIZE * MAX_TR_SIZE)
#define MIN_TR_DIM                        (MIN_TR_SIZE * MIN_TR_SIZE)

#define MAX_BEF_DATA_NUM                  (NUM_NEIB << 1)

/* maximum CB count in a LCB */
#define MAX_CU_CNT_IN_LCU                  (MAX_CU_DIM/MIN_CU_DIM)
/* pixel position to SCB position */
#define PEL2SCU(pel)                       ((pel) >> MIN_CU_LOG2)

#define PIC_PAD_SIZE_L                     (MAX_CU_SIZE + 16)
#define PIC_PAD_SIZE_C                     (PIC_PAD_SIZE_L >> 1)

/* number of MVP candidates */
#if INCREASE_MVP_NUM
#define MAX_NUM_MVP_SMALL_CU               4
#define MAX_NUM_MVP                        6
#define NUM_SAMPLES_BLOCK                  32 // 16..64
#define ORG_MAX_NUM_MVP                    4
#else
#define MAX_NUM_MVP                        4
#endif
#define MAX_NUM_POSSIBLE_SCAND             13

/* DPB Extra size */
#define DELAYED_FRAME                      8 /* Maximum number of delayed frames */
#define EXTRA_FRAME                       (XEVD_MAX_NUM_ACTIVE_REF_FRAME + DELAYED_FRAME)

/* maximum picture buffer size */
#define MAX_PB_SIZE                       (XEVD_MAX_NUM_REF_PICS + EXTRA_FRAME)


#define MAX_NUM_TILES_ROW                  22
#define MAX_NUM_TILES_COL                  20

/* Neighboring block availability flag bits */
#define AVAIL_BIT_UP                       0
#define AVAIL_BIT_LE                       1
#define AVAIL_BIT_RI                       3
#define AVAIL_BIT_LO                       4
#define AVAIL_BIT_UP_LE                    5
#define AVAIL_BIT_UP_RI                    6
#define AVAIL_BIT_LO_LE                    7
#define AVAIL_BIT_LO_RI                    8
#define AVAIL_BIT_RI_UP                    9
#define AVAIL_BIT_UP_LE_LE                 10
#define AVAIL_BIT_UP_RI_RI                 11

/* Neighboring block availability flags */
#define AVAIL_UP                          (1 << AVAIL_BIT_UP)
#define AVAIL_LE                          (1 << AVAIL_BIT_LE)
#define AVAIL_RI                          (1 << AVAIL_BIT_RI)
#define AVAIL_LO                          (1 << AVAIL_BIT_LO)
#define AVAIL_UP_LE                       (1 << AVAIL_BIT_UP_LE)
#define AVAIL_UP_RI                       (1 << AVAIL_BIT_UP_RI)
#define AVAIL_LO_LE                       (1 << AVAIL_BIT_LO_LE)
#define AVAIL_LO_RI                       (1 << AVAIL_BIT_LO_RI)
#define AVAIL_RI_UP                       (1 << AVAIL_BIT_RI_UP)
#define AVAIL_UP_LE_LE                    (1 << AVAIL_BIT_UP_LE_LE)
#define AVAIL_UP_RI_RI                    (1 << AVAIL_BIT_UP_RI_RI)

/* MB availability check macro */
#define IS_AVAIL(avail, pos)            (((avail)&(pos)) == (pos))
/* MB availability set macro */
#define SET_AVAIL(avail, pos)             (avail) |= (pos)
/* MB availability remove macro */
#define REM_AVAIL(avail, pos)             (avail) &= (~(pos))
/* MB availability into bit flag */
#define GET_AVAIL_FLAG(avail, bit)      (((avail)>>(bit)) & 0x1)

/*****************************************************************************
 * slice type
 *****************************************************************************/
#define SLICE_I                            XEVD_ST_I
#define SLICE_P                            XEVD_ST_P
#define SLICE_B                            XEVD_ST_B

#define IS_INTRA_SLICE(slice_type)       ((slice_type) == SLICE_I))
#define IS_INTER_SLICE(slice_type)      (((slice_type) == SLICE_P) || ((slice_type) == SLICE_B))

/*****************************************************************************
 * prediction mode
 *****************************************************************************/
#define MODE_INTRA                         0
#define MODE_INTER                         1
#define MODE_SKIP                          2
#define MODE_DIR                           3
/*****************************************************************************
 * prediction direction
 *****************************************************************************/
/* inter pred direction, look list0 side */
#define PRED_L0                            0
/* inter pred direction, look list1 side */
#define PRED_L1                            1
/* inter pred direction, look both list0, list1 side */
#define PRED_BI                            2
/* inter pred direction, look both list0, list1 side */
#define PRED_SKIP                          3
/* inter pred direction, look both list0, list1 side */
#define PRED_DIR                           4

#define LR_00                              0
#define LR_10                              1
#define LR_01                              2
#define LR_11                              3

/*****************************************************************************
 * bi-prediction type
 *****************************************************************************/
#define BI_NON                             0
#define BI_NORMAL                          1
#define BI_FL0                             2
#define BI_FL1                             3

/*****************************************************************************
 * intra prediction direction
 *****************************************************************************/
#define IPD_DC                             0
#define IPD_PLN                            1  /* Luma, Planar */
#define IPD_BI                             2  /* Luma, Bilinear */
#define IPD_HOR                            24 /* Luma, Horizontal */
#define IPD_VER                            12 /* Luma, Vertical */
#define IPD_DM_C                           0  /* Chroma, DM */

#define IPD_BI_C                           1  /* Chroma, Bilinear */
#define IPD_DC_C                           2  /* Chroma, DC */
#define IPD_HOR_C                          3  /* Chroma, Horizontal*/
#define IPD_VER_C                          4  /* Chroma, Vertical */

#define IPD_RDO_CNT                        5

#define IPD_DC_B                           0
#define IPD_HOR_B                          1 /* Luma, Horizontal */
#define IPD_VER_B                          2 /* Luma, Vertical */
#define IPD_UL_B                           3
#define IPD_UR_B                           4

#define IPD_DC_C_B                         0  /* Chroma, DC */
#define IPD_HOR_C_B                        1  /* Chroma, Horizontal*/
#define IPD_VER_C_B                        2  /* Chroma, Vertical */
#define IPD_UL_C_B                         3
#define IPD_UR_C_B                         4

#define IPD_CNT_B                          5
#define IPD_CNT                            33

#define IPD_CHROMA_CNT                     5
#define IPD_INVALID                       (-1)

#define IPD_DIA_R                          18 /* Luma, Right diagonal */ /* (IPD_VER + IPD_HOR) >> 1 */
#define IPD_DIA_L                          6  /* Luma, Left diagonal */
#define IPD_DIA_U                          30 /* Luma, up diagonal */

#define INTRA_MPM_NUM                      2
#define INTRA_PIMS_NUM                     8

/*****************************************************************************
* Transform
*****************************************************************************/
typedef void(*XEVD_ITXB)(void* coef, void* t, int shift, int line, int step);
#define PI                                (3.14159265358979323846)

typedef void(*XEVD_DBK)(pel *buf, int st, int stride, int bit_depth_minus8, int chroma_format_idc);
typedef void(*XEVD_DBK_CH)(pel *u, pel *v, int st_u, int st_v, int stride, int bit_depth_minus8, int chroma_format_idc);
/*****************************************************************************
 * reference index
 *****************************************************************************/
#define REFI_INVALID                      (-1)
#define REFI_IS_VALID(refi)               ((refi) >= 0)
#define SET_REFI(refi, idx0, idx1)        (refi)[REFP_0] = (idx0); (refi)[REFP_1] = (idx1)

 /*****************************************************************************
 * macros for CU map

 - [ 0: 6] : slice number (0 ~ 128)
 - [ 7:14] : reserved
 - [15:15] : 1 -> intra CU, 0 -> inter CU
 - [16:22] : QP
 - [23:23] : skip mode flag
 - [24:24] : luma cbf
 - [26:26] : IBC mode flag
 - [27:30] : reserved
 - [31:31] : 0 -> no encoded/decoded CU, 1 -> encoded/decoded CU
 *****************************************************************************/
/* set slice number to map */
#define MCU_SET_SN(m, sn)       (m)=(((m) & 0xFFFFFF80)|((sn) & 0x7F))
/* get slice number from map */
#define MCU_GET_SN(m)           (int)((m) & 0x7F)

/* set intra CU flag to map */
#define MCU_SET_IF(m)           (m)=((m)|(1<<15))
/* get intra CU flag from map */
#define MCU_GET_IF(m)           (int)(((m)>>15) & 1)
/* clear intra CU flag in map */
#define MCU_CLR_IF(m)           (m)=((m) & 0xFFFF7FFF)

/* set QP to map */
#define MCU_SET_QP(m, qp)       (m)=((m)|((qp)&0x7F)<<16)
/* get QP from map */
#define MCU_GET_QP(m)           (int)(((m)>>16)&0x7F)
#define MCU_RESET_QP(m)         (m)=((m) & (~((127)<<16)))

/* set skip mode flag */
#define MCU_SET_SF(m)           (m)=((m)|(1<<23))
/* get skip mode flag */
#define MCU_GET_SF(m)           (int)(((m)>>23) & 1)
/* clear skip mode flag */
#define MCU_CLR_SF(m)           (m)=((m) & (~(1<<23)))

/* set luma cbf flag */
#define MCU_SET_CBFL(m)         (m)=((m)|(1<<24))
/* get luma cbf flag */
#define MCU_GET_CBFL(m)         (int)(((m)>>24) & 1)
/* clear luma cbf flag */
#define MCU_CLR_CBFL(m)         (m)=((m) & (~(1<<24)))

/* set encoded/decoded CU to map */
#define MCU_SET_COD(m)          (m)=((m)|(1<<31))
/* get encoded/decoded CU flag from map */
#define MCU_GET_COD(m)          (int)(((m)>>31) & 1)
/* clear encoded/decoded CU flag to map */
#define MCU_CLR_COD(m)          (m)=((m) & 0x7FFFFFFF)

/* multi bit setting: intra flag, encoded/decoded flag, slice number */
#define MCU_SET_IF_COD_SN_QP(m, i, sn, qp) \
    (m) = (((m)&0xFF807F80)|((sn)&0x7F)|((qp)<<16)|((i)<<15)|(1<<31))

#define MCU_SET_IF_SN_QP(m, i, sn, qp) \
    (m) = (((m)&0xFF807F80)|((sn)&0x7F)|((qp)<<16)|((i)<<15))
#define MCU_IS_COD_NIF(m)      ((((m)>>15) & 0x10001) == 0x10000)

/* set log2_cuw & log2_cuh to map */
#define MCU_SET_LOGW(m, v)       (m)=((m & 0xF0FFFFFF)|((v)&0x0F)<<24)
#define MCU_SET_LOGH(m, v)       (m)=((m & 0x0FFFFFFF)|((v)&0x0F)<<28)
/* get log2_cuw & log2_cuh to map */
#define MCU_GET_LOGW(m)          (int)(((m)>>24)&0x0F)
#define MCU_GET_LOGH(m)          (int)(((m)>>28)&0x0F)

typedef u16 SBAC_CTX_MODEL;
//ADDED
#define NUM_CTX_LAST_SIG_COEFF_LUMA        18
#define NUM_CTX_LAST_SIG_COEFF_CHROMA      3
#define NUM_CTX_LAST_SIG_COEFF             (NUM_CTX_LAST_SIG_COEFF_LUMA + NUM_CTX_LAST_SIG_COEFF_CHROMA)

#define NUM_CTX_SIG_COEFF_LUMA             39  /* number of context models for luma sig coeff flag */
#define NUM_CTX_SIG_COEFF_CHROMA           8   /* number of context models for chroma sig coeff flag */
#define NUM_CTX_SIG_COEFF_LUMA_TU          13  /* number of context models for luma sig coeff flag per TU */
#define NUM_CTX_SIG_COEFF_FLAG             (NUM_CTX_SIG_COEFF_LUMA + NUM_CTX_SIG_COEFF_CHROMA)  /* number of context models for sig coeff flag */
#define NUM_CTX_GTX_LUMA                   13
#define NUM_CTX_GTX_CHROMA                 5
#define NUM_CTX_GTX                        (NUM_CTX_GTX_LUMA + NUM_CTX_GTX_CHROMA)  /* number of context models for gtA/B flag */

#define NUM_CTX_SKIP_FLAG                  2
#define NUM_CTX_CBF_LUMA                   1
#define NUM_CTX_CBF_CB                     1
#define NUM_CTX_CBF_CR                     1
#define NUM_CTX_CBF_ALL                    1
#define NUM_CTX_PRED_MODE                  3
#define NUM_CTX_INTER_PRED_IDC             2       /* number of context models for inter prediction direction */
#define NUM_CTX_DIRECT_MODE_FLAG           1
#define NUM_CTX_MERGE_MODE_FLAG            1
#define NUM_CTX_REF_IDX                    2
#define NUM_CTX_MERGE_IDX                  5
#define NUM_CTX_MVP_IDX                    3
#define NUM_CTX_BI_PRED_IDX                2
#define NUM_CTX_MVD                        1       /* number of context models for motion vector difference */
#define NUM_CTX_INTRA_PRED_MODE            2
#define NUM_CTX_INTRA_LUMA_PRED_MPM_FLAG   1
#define NUM_CTX_INTRA_LUMA_PRED_MPM_IDX    1
#define NUM_CTX_INTRA_CHROMA_PRED_MODE     1
#define NUM_CTX_CC_RUN                     24
#define NUM_CTX_CC_LAST                    2
#define NUM_CTX_CC_LEVEL                   24
#define NUM_CTX_SPLIT_CU_FLAG              1
#define NUM_CTX_DELTA_QP                   1


#define NUM_CTX_MMVD_FLAG                  1
#define NUM_CTX_MMVD_GROUP_IDX            (3 - 1)
#define NUM_CTX_MMVD_MERGE_IDX            (4 - 1)
#define NUM_CTX_MMVD_DIST_IDX             (8 - 1)
#define NUM_CTX_MMVD_DIRECTION_IDX         2
#define NUM_CTX_AFFINE_MVD_FLAG            2       /* number of context models for affine_mvd_flag_l0 and affine_mvd_flag_l1 (1st one is for affine_mvd_flag_l0 and 2nd one if for affine_mvd_flag_l1) */

#define NUM_CTX_IBC_FLAG                   2
#define NUM_CTX_BTT_SPLIT_FLAG             15
#define NUM_CTX_BTT_SPLIT_DIR              5
#define NUM_CTX_BTT_SPLIT_TYPE             1
#define NUM_CTX_SUCO_FLAG                  14

#define NUM_CTX_MODE_CONS                  3

#define NUM_CTX_AMVR_IDX                   4

#define NUM_CTX_AFFINE_FLAG                2
#define NUM_CTX_AFFINE_MODE                1
#define NUM_CTX_AFFINE_MRG                 5
#define NUM_CTX_AFFINE_MVP_IDX            (2 - 1)

#define NUM_CTX_ALF_CTB_FLAG               1

#define NUM_CTX_ATS_INTRA_CU_FLAG          1
#define NUM_CTX_ATS_MODE_FLAG              1
#define NUM_CTX_ATS_INTER_FLAG             2
#define NUM_CTX_ATS_INTER_QUAD_FLAG        1
#define NUM_CTX_ATS_INTER_HOR_FLAG         3
#define NUM_CTX_ATS_INTER_POS_FLAG         1

/* context models for arithemetic coding */
typedef struct _XEVD_SBAC_CTX
{
    SBAC_CTX_MODEL   skip_flag[NUM_CTX_SKIP_FLAG];
    SBAC_CTX_MODEL   ibc_flag[NUM_CTX_IBC_FLAG];
    SBAC_CTX_MODEL   mmvd_flag[NUM_CTX_MMVD_FLAG];
    SBAC_CTX_MODEL   mmvd_merge_idx[NUM_CTX_MMVD_MERGE_IDX];
    SBAC_CTX_MODEL   mmvd_distance_idx[NUM_CTX_MMVD_DIST_IDX];
    SBAC_CTX_MODEL   mmvd_direction_idx[NUM_CTX_MMVD_DIRECTION_IDX];
    SBAC_CTX_MODEL   mmvd_group_idx[NUM_CTX_MMVD_GROUP_IDX];
    SBAC_CTX_MODEL   direct_mode_flag[NUM_CTX_DIRECT_MODE_FLAG];
    SBAC_CTX_MODEL   merge_mode_flag[NUM_CTX_MERGE_MODE_FLAG];
    SBAC_CTX_MODEL   inter_dir[NUM_CTX_INTER_PRED_IDC];
    SBAC_CTX_MODEL   intra_dir[NUM_CTX_INTRA_PRED_MODE];
    SBAC_CTX_MODEL   intra_luma_pred_mpm_flag[NUM_CTX_INTRA_LUMA_PRED_MPM_FLAG];
    SBAC_CTX_MODEL   intra_luma_pred_mpm_idx[NUM_CTX_INTRA_LUMA_PRED_MPM_IDX];
    SBAC_CTX_MODEL   intra_chroma_pred_mode[NUM_CTX_INTRA_CHROMA_PRED_MODE];
    SBAC_CTX_MODEL   pred_mode[NUM_CTX_PRED_MODE];
    SBAC_CTX_MODEL   mode_cons[NUM_CTX_MODE_CONS];
    SBAC_CTX_MODEL   refi[NUM_CTX_REF_IDX];
    SBAC_CTX_MODEL   merge_idx[NUM_CTX_MERGE_IDX];
    SBAC_CTX_MODEL   mvp_idx[NUM_CTX_MVP_IDX];
    SBAC_CTX_MODEL   affine_mvp_idx[NUM_CTX_AFFINE_MVP_IDX];
    SBAC_CTX_MODEL   mvr_idx[NUM_CTX_AMVR_IDX];
    SBAC_CTX_MODEL   bi_idx[NUM_CTX_BI_PRED_IDX];
    SBAC_CTX_MODEL   mvd[NUM_CTX_MVD];
    SBAC_CTX_MODEL   cbf_all[NUM_CTX_CBF_ALL];
    SBAC_CTX_MODEL   cbf_luma[NUM_CTX_CBF_LUMA];
    SBAC_CTX_MODEL   cbf_cb[NUM_CTX_CBF_CB];
    SBAC_CTX_MODEL   cbf_cr[NUM_CTX_CBF_CR];
    SBAC_CTX_MODEL   run[NUM_CTX_CC_RUN];
    SBAC_CTX_MODEL   last[NUM_CTX_CC_LAST];
    SBAC_CTX_MODEL   level[NUM_CTX_CC_LEVEL];
    SBAC_CTX_MODEL   sig_coeff_flag[NUM_CTX_SIG_COEFF_FLAG];
    SBAC_CTX_MODEL   coeff_abs_level_greaterAB_flag[NUM_CTX_GTX];
    SBAC_CTX_MODEL   last_sig_coeff_x_prefix[NUM_CTX_LAST_SIG_COEFF];
    SBAC_CTX_MODEL   last_sig_coeff_y_prefix[NUM_CTX_LAST_SIG_COEFF];
    SBAC_CTX_MODEL   btt_split_flag[NUM_CTX_BTT_SPLIT_FLAG];
    SBAC_CTX_MODEL   btt_split_dir[NUM_CTX_BTT_SPLIT_DIR];
    SBAC_CTX_MODEL   btt_split_type[NUM_CTX_BTT_SPLIT_TYPE];
    SBAC_CTX_MODEL   affine_flag[NUM_CTX_AFFINE_FLAG];
    SBAC_CTX_MODEL   affine_mode[NUM_CTX_AFFINE_MODE];
    SBAC_CTX_MODEL   affine_mrg[NUM_CTX_AFFINE_MRG];
    SBAC_CTX_MODEL   affine_mvd_flag[NUM_CTX_AFFINE_MVD_FLAG];
    SBAC_CTX_MODEL   suco_flag[NUM_CTX_SUCO_FLAG];
    SBAC_CTX_MODEL   alf_ctb_flag[NUM_CTX_ALF_CTB_FLAG];
    SBAC_CTX_MODEL   split_cu_flag[NUM_CTX_SPLIT_CU_FLAG];

    SBAC_CTX_MODEL   delta_qp[NUM_CTX_DELTA_QP];

    SBAC_CTX_MODEL   ats_mode[NUM_CTX_ATS_MODE_FLAG];
    SBAC_CTX_MODEL   ats_cu_inter_flag[NUM_CTX_ATS_INTER_FLAG];
    SBAC_CTX_MODEL   ats_cu_inter_quad_flag[NUM_CTX_ATS_INTER_QUAD_FLAG];
    SBAC_CTX_MODEL   ats_cu_inter_hor_flag[NUM_CTX_ATS_INTER_HOR_FLAG];
    SBAC_CTX_MODEL   ats_cu_inter_pos_flag[NUM_CTX_ATS_INTER_POS_FLAG];
    int              sps_cm_init_flag;
} XEVD_SBAC_CTX;

/* Maximum transform dynamic range (excluding sign bit) */
#define MAX_TX_DYNAMIC_RANGE               15
#define MAX_TX_VAL                       ((1 << MAX_TX_DYNAMIC_RANGE) - 1)
#define MIN_TX_VAL                      (-(1 << MAX_TX_DYNAMIC_RANGE))

#define QUANT_SHIFT                        14
#define QUANT_IQUANT_SHIFT                 20

/* neighbor CUs
   neighbor position:

   D     B     C

   A     X,<G>

   E          <F>
*/
#define MAX_NEB                            5
#define NEB_A                              0  /* left */
#define NEB_B                              1  /* up */
#define NEB_C                              2  /* up-right */
#define NEB_D                              3  /* up-left */
#define NEB_E                              4  /* low-left */

#define NEB_F                              5  /* co-located of low-right */
#define NEB_G                              6  /* co-located of X */
#define NEB_X                              7  /* center (current block) */
#define NEB_H                              8  /* right */
#define NEB_I                              9  /* low-right */
#define MAX_NEB2                           10

#define XEVD_MAX_QP_TABLE_SIZE                  58
#define XEVD_MAX_QP_TABLE_SIZE_EXT              94
#define XEVD_MAX_NUM_REF_PICS                   21
#define XEVD_MAX_NUM_ACTIVE_REF_FRAME           5
#define XEVD_MAX_NUM_RPLS                       32

/* SEI UUID ISO Length */
#define ISO_IEC_11578_LEN                       16

/* rpl structure */
 typedef struct _XEVD_RPL
{
    int poc;
    int tid;
    int ref_pic_num;
    int ref_pic_active_num;
    int ref_pics[XEVD_MAX_NUM_REF_PICS];
    char pic_type;
}XEVD_RPL;

/* chromaQP table structure to be signalled in SPS*/
typedef struct _XEVD_CHROMA_TABLE
{
    int chroma_qp_table_present_flag;
    int same_qp_table_for_chroma;
    int global_offset_flag;
    int num_points_in_qp_table_minus1[2];
    int delta_qp_in_val_minus1[2][XEVD_MAX_QP_TABLE_SIZE];
    int delta_qp_out_val[2][XEVD_MAX_QP_TABLE_SIZE];
}XEVD_CHROMA_TABLE;

/* picture store structure */
typedef struct _XEVD_PIC
{
    /* Address of Y buffer (include padding) */
    pel             *buf_y;
    /* Address of U buffer (include padding) */
    pel             *buf_u;
    /* Address of V buffer (include padding) */
    pel             *buf_v;
    /* Start address of Y component (except padding) */
    pel             *y;
    /* Start address of U component (except padding)  */
    pel             *u;
    /* Start address of V component (except padding)  */
    pel             *v;
    /* Stride of luma picture */
    int              s_l;
    /* Stride of chroma picture */
    int              s_c;
    /* Width of luma picture */
    int              w_l;
    /* Height of luma picture */
    int              h_l;
    /* Width of chroma picture */
    int              w_c;
    /* Height of chroma picture */
    int              h_c;
    /* padding size of luma */
    int              pad_l;
    /* padding size of chroma */
    int              pad_c;
    /* image buffer */
    XEVD_IMGB       * imgb;
    /* presentation temporal reference of this picture */
    s32              poc;
    /* 0: not used for reference buffer, reference picture type */
    u8               is_ref;
    /* needed for output? */
    u8               need_for_out;
    /* scalable layer id */
    u8               temporal_id;
    s16            (*map_mv)[REFP_NUM][MV_D];
    s16            (*map_unrefined_mv)[REFP_NUM][MV_D];
    s8             (*map_refi)[REFP_NUM];
    u32              list_poc[XEVD_MAX_NUM_REF_PICS];
    int              pic_deblock_alpha_offset;
    int              pic_deblock_beta_offset;
    int              pic_qp_u_offset;
    int              pic_qp_v_offset;
    u8               digest[N_C][16];
} XEVD_PIC;

/*****************************************************************************
 * picture buffer allocator
 *****************************************************************************/
typedef struct _PICBUF_ALLOCATOR PICBUF_ALLOCATOR;
struct _PICBUF_ALLOCATOR
{
    /* address of picture buffer allocation function */
    XEVD_PIC      *(* fn_alloc)(PICBUF_ALLOCATOR *pa, int *ret, int bit_depth);
    /* address of picture buffer free function */
    void           (*fn_free)(PICBUF_ALLOCATOR *pa, XEVD_PIC *pic);
    /* width */
    int              w;
    /* height */
    int              h;
    /* pad size for luma */
    int              pad_l;
    /* pad size for chroma */
    int              pad_c;
    /* arbitrary data, if needs */
    int              ndata[4];
    /* arbitrary address, if needs */
    void            *pdata[4];
    int              idc;

};

/*****************************************************************************
 * picture manager for DPB in decoder and RPB in encoder
 *****************************************************************************/
typedef struct _XEVD_PM
{
    /* picture store (including reference and non-reference) */
    XEVD_PIC        * pic[MAX_PB_SIZE];
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
} XEVD_PM;

/* reference picture structure */
typedef struct _XEVD_REFP
{
    /* address of reference picture */
    XEVD_PIC        * pic;
    /* POC of reference picture */
    u32              poc;
    s16(*map_mv)[REFP_NUM][MV_D];

    s8(*map_refi)[REFP_NUM];
    u32             *list_poc;
} XEVD_REFP;

/*****************************************************************************
 * NALU header
 *****************************************************************************/
typedef struct _XEVD_NALU
{
    int nal_unit_size;
    int forbidden_zero_bit;
    int nal_unit_type_plus1;
    int nuh_temporal_id;
    int nuh_reserved_zero_5bits;
    int nuh_extension_flag;
} XEVD_NALU;


#define     EXTENDED_SAR 255
#define     NUM_CPB 32

/*****************************************************************************
* Hypothetical Reference Decoder (HRD) parameters, part of VUI
*****************************************************************************/
typedef struct _XEVD_HRD
{
    int cpb_cnt_minus1;
    int bit_rate_scale;
    int cpb_size_scale;
    int bit_rate_value_minus1[NUM_CPB];
    int cpb_size_value_minus1[NUM_CPB];
    int cbr_flag[NUM_CPB];
    int initial_cpb_removal_delay_length_minus1;
    int cpb_removal_delay_length_minus1;
    int dpb_output_delay_length_minus1;
    int time_offset_length;
} XEVD_HRD;

/*****************************************************************************
* video usability information (VUI) part of SPS
*****************************************************************************/
typedef struct _XEVD_VUI
{
    int aspect_ratio_info_present_flag;
    int aspect_ratio_idc;
    int sar_width;
    int sar_height;
    int overscan_info_present_flag;
    int overscan_appropriate_flag;
    int video_signal_type_present_flag;
    int video_format;
    int video_full_range_flag;
    int colour_description_present_flag;
    int colour_primaries;
    int transfer_characteristics;
    int matrix_coefficients;
    int chroma_loc_info_present_flag;
    int chroma_sample_loc_type_top_field;
    int chroma_sample_loc_type_bottom_field;
    int neutral_chroma_indication_flag;
    int field_seq_flag;
    int timing_info_present_flag;
    int num_units_in_tick;
    int time_scale;
    int fixed_pic_rate_flag;
    int nal_hrd_parameters_present_flag;
    int vcl_hrd_parameters_present_flag;
    int low_delay_hrd_flag;
    int pic_struct_present_flag;
    int bitstream_restriction_flag;
    int motion_vectors_over_pic_boundaries_flag;
    int max_bytes_per_pic_denom;
    int max_bits_per_mb_denom;
    int log2_max_mv_length_horizontal;
    int log2_max_mv_length_vertical;
    int num_reorder_pics;
    int max_dec_pic_buffering;

    XEVD_HRD hrd_parameters;
} XEVD_VUI;

/*****************************************************************************
* sequence parameter set
*****************************************************************************/
typedef struct _XEVD_SPS
{
    int              sps_seq_parameter_set_id;
    int              profile_idc;
    int              level_idc;
    int              toolset_idc_h;
    int              toolset_idc_l;
    int              chroma_format_idc;
    u32              pic_width_in_luma_samples;
    u32              pic_height_in_luma_samples;
    int              bit_depth_luma_minus8;
    int              bit_depth_chroma_minus8;
    int              sps_btt_flag;
    int              sps_suco_flag;
    int              log2_ctu_size_minus5;
    int              log2_min_cb_size_minus2;
    int              log2_diff_ctu_max_14_cb_size;
    int              log2_diff_ctu_max_tt_cb_size;
    int              log2_diff_min_cb_min_tt_cb_size_minus2;
    int              log2_diff_ctu_size_max_suco_cb_size;
    int              log2_diff_max_suco_min_suco_cb_size;
    int              tool_amvr;
    int              tool_mmvd;
    int              tool_affine;
    int              tool_dmvr;
    int              tool_addb;
    int              tool_alf;
    int              tool_htdf;
    int              tool_admvp;

    int              tool_hmvp;

    int              tool_eipd;
    int              tool_iqt;
    int              tool_cm_init;
    int              tool_ats;
    int              tool_rpl;
    int              tool_pocs;
    int              log2_sub_gop_length;
    int              log2_ref_pic_gap_length;
    int              tool_adcc;
    int              log2_max_pic_order_cnt_lsb_minus4;
    int              sps_max_dec_pic_buffering_minus1;
    int              max_num_ref_pics;
    u32              long_term_ref_pics_flag;
    /* HLS_RPL  */
    int              rpl1_same_as_rpl0_flag;
    int              num_ref_pic_lists_in_sps0;
    XEVD_RPL          rpls_l0[XEVD_MAX_NUM_RPLS];
    int              num_ref_pic_lists_in_sps1;
    XEVD_RPL          rpls_l1[XEVD_MAX_NUM_RPLS];

    int              picture_cropping_flag;
    int              picture_crop_left_offset;
    int              picture_crop_right_offset;
    int              picture_crop_top_offset;
    int              picture_crop_bottom_offset;

    int              dquant_flag;              /*1 specifies the improved delta qp signaling processes is used*/

    XEVD_CHROMA_TABLE chroma_qp_table_struct;
    u32              ibc_flag;                   /* 1 bit : flag of enabling IBC or not */
    int              ibc_log_max_size;           /* log2 max ibc size */
    int              vui_parameters_present_flag;

    int tool_dra;

    XEVD_VUI vui_parameters;

} XEVD_SPS;

/*****************************************************************************
* picture parameter set
*****************************************************************************/
typedef struct _XEVD_PPS
{
    int pps_pic_parameter_set_id;
    int pps_seq_parameter_set_id;
    int num_ref_idx_default_active_minus1[2];
    int additional_lt_poc_lsb_len;
    int rpl1_idx_present_flag;
    int single_tile_in_pic_flag;
    int num_tile_columns_minus1;
    int num_tile_rows_minus1;
    int uniform_tile_spacing_flag;
    int tile_column_width_minus1[MAX_NUM_TILES_ROW];
    int tile_row_height_minus1[MAX_NUM_TILES_COL];
    int loop_filter_across_tiles_enabled_flag;
    int tile_offset_lens_minus1;
    int tile_id_len_minus1;
    int explicit_tile_id_flag;
  int pic_dra_enabled_flag;
    int tile_id_val[MAX_NUM_TILES_ROW][MAX_NUM_TILES_COL];
    int arbitrary_slice_present_flag;
    int constrained_intra_pred_flag;
    int cu_qp_delta_enabled_flag;
    int cu_qp_delta_area;
    int pic_dra_aps_id;
} XEVD_PPS;

/*****************************************************************************
 * slice header
 *****************************************************************************/

typedef struct _XEVD_SH
{
    int              slice_pic_parameter_set_id;
    int              single_tile_in_slice_flag;
    int              first_tile_id;
    int              arbitrary_slice_flag;
    int              last_tile_id;
    int              num_remaining_tiles_in_slice_minus1;
    int              delta_tile_id_minus1[MAX_NUM_TILES_ROW * MAX_NUM_TILES_COL];
    int              slice_type;
    int              no_output_of_prior_pics_flag;
    int              temporal_mvp_asigned_flag;
    int              collocated_from_list_idx;        // Specifies source (List ID) of the collocated picture, equialent of the collocated_from_l0_flag
    int              collocated_from_ref_idx;         // Specifies source (RefID_ of the collocated picture, equialent of the collocated_ref_idx
    int              collocated_mvp_source_list_idx;  // Specifies source (List ID) in collocated pic that provides MV information
    s32              poc_lsb;

    /*   HLS_RPL */
    u32              ref_pic_list_sps_flag[2];
    int              rpl_l0_idx;                            //-1 means this slice does not use RPL candidate in SPS for RPL0
    int              rpl_l1_idx;                            //-1 means this slice does not use RPL candidate in SPS for RPL1

    XEVD_RPL          rpl_l0;
    XEVD_RPL          rpl_l1;

    u32              num_ref_idx_active_override_flag;
    int              deblocking_filter_on;
    int              sh_deblock_alpha_offset;
    int              sh_deblock_beta_offset;
    int              qp;
    int              qp_u;
    int              qp_v;
    int              qp_u_offset;
    int              qp_v_offset;
    u32              entry_point_offset_minus1[MAX_NUM_TILES_ROW * MAX_NUM_TILES_COL];
    /*QP of previous cu in decoding order (used for dqp)*/
    u8               qp_prev_eco;
    u8               dqp;
    u8               qp_prev_mode;
    u16              num_ctb;
    u16              num_tiles_in_slice;
} XEVD_SH;

/*****************************************************************************
* Tiles
*****************************************************************************/
typedef struct _XEVD_TILE
{
    /* tile width in CTB unit */
    u16             w_ctb;
    /* tile height in CTB unit */
    u16             h_ctb;
    /* tile size in CTB unit (= w_ctb * h_ctb) */
    u32             f_ctb;
    /* first ctb address in raster scan order */
    u16             ctba_rs_first;
    u8              qp;
    u8              qp_prev_eco;
} XEVD_TILE;

/*****************************************************************************/

typedef struct _XEVD_POC
{
    /* current picture order count value */
    int             poc_val;
    /* the picture order count of the previous Tid0 picture */
    s32             prev_poc_val;
    /* the decoding order count of the previous picture */
    int             prev_doc_offset;
    /* the maximum picture index of the previous picture */
    s32             prev_pic_max_poc_val;
} XEVD_POC;


typedef enum _XEVE_SEI_PAYLOAD_TYPE
{
    XEVD_BUFFERING_PERIOD = 0,
    XEVD_PICTURE_TIMING = 1,
    XEVD_USER_DATA_REGISTERED_ITU_T_T35 = 4,
    XEVD_USER_DATA_UNREGISTERED = 5,
    XEVD_RECOVERY_POINT = 6,
    XEVD_MASTERING_DISPLAY_INFO = 137,
    XEVD_CONTENT_LIGHT_LEVEL_INFO = 144,
    XEVD_AMBIENT_VIEWING_ENVIRONMENT = 148,
} XEVE_SEI_PAYLOAD_TYPE;

typedef struct _XEVE_SEI_PAYLOAD
{
    int payload_size;
    XEVE_SEI_PAYLOAD_TYPE payload_type;
    u8* payload;
} XEVE_SEI_PAYLOAD;

typedef struct _XEVE_SEI
{
    int num_payloads;
    XEVE_SEI_PAYLOAD *payloads;
} XEVE_SEI;

/*****************************************************************************
 * user data types
 *****************************************************************************/
#define XEVD_UD_PIC_SIGNATURE              0x10
#define XEVD_UD_END                        0xFF

/*****************************************************************************
 * for binary and triple tree structure
 *****************************************************************************/
typedef enum _SPLIT_MODE
{
    NO_SPLIT        = 0,
    SPLIT_BI_VER    = 1,
    SPLIT_BI_HOR    = 2,
    SPLIT_TRI_VER   = 3,
    SPLIT_TRI_HOR   = 4,
    SPLIT_QUAD      = 5,
} SPLIT_MODE;

typedef enum _SPLIT_DIR
{
    SPLIT_VER = 0,
    SPLIT_HOR = 1,
} SPLIT_DIR;

typedef enum _BLOCK_SHAPE
{
    NON_SQUARE_14,
    NON_SQUARE_12,
    SQUARE,
    NON_SQUARE_21,
    NON_SQUARE_41,
    NUM_BLOCK_SHAPE,
} BLOCK_SHAPE;

typedef enum _BLOCK_PARAMETER
{
    BLOCK_11,
    BLOCK_12,
    BLOCK_14,
    BLOCK_TT,
    NUM_BLOCK_PARAMETER,
} BLOCK_PARAMETER;

typedef enum _BLOCK_PARAMETER_IDX
{
    IDX_MAX,
    IDX_MIN,
    NUM_BLOCK_IDX,
} BLOCK_PARAMETER_IDX;

/*****************************************************************************
* history-based MV prediction buffer (slice level)
*****************************************************************************/
typedef struct _XEVD_HISTORY_BUFFER
{
    s16 history_mv_table[ALLOWED_CHECKED_NUM][REFP_NUM][MV_D];
    s8  history_refi_table[ALLOWED_CHECKED_NUM][REFP_NUM];
    int currCnt;
    int m_maxCnt;
} XEVD_HISTORY_BUFFER;

typedef enum _CTX_NEV_IDX
{
    CNID_SKIP_FLAG,
    CNID_PRED_MODE,
    CNID_MODE_CONS,
    CNID_AFFN_FLAG,
    CNID_IBC_FLAG,
    NUM_CNID,
} CTX_NEV_IDX;

typedef enum _MSL_IDX
{
    MSL_SKIP,  //skip
    MSL_MERG,  //merge or direct
    MSL_LIS0,  //list 0
    MSL_LIS1,  //list 1
    MSL_BI,    //bi pred
    NUM_MODE_SL,

} MSL_IDX;



static const int NTAPS_LUMA = 8; ///< Number of taps for luma
static const int NTAPS_CHROMA = 4; ///< Number of taps for chroma

#define MAX_SUB_TB_NUM 4
enum TQC_RUN {
    RUN_L = 1,
    RUN_CB = 2,
    RUN_CR = 4
};

typedef struct _XEVD_CTX XEVD_CTX;

/*****************************************************************************
 * SBAC structure
 *****************************************************************************/
typedef struct _XEVD_SBAC
{
    u32            range;
    u32            value;
    XEVD_SBAC_CTX  ctx;
} XEVD_SBAC;

/*****************************************************************************
 * CORE information used for decoding process.
 *
 * The variables in this structure are very often used in decoding process.
 *****************************************************************************/
typedef struct _XEVD_CU_DATA
{
    s8  split_mode[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU];
    s8  suco_flag[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU];
    u8  *qp_y;
    u8  *qp_u;
    u8  *qp_v;
    u8  *pred_mode;
    u8  *pred_mode_chroma;
    u8  **mpm;
    u8  **mpm_ext;
    s8  **ipm;
    u8  *skip_flag;
    u8  *ibc_flag;

    u8  *dmvr_flag;

    s8  **refi;
    u8  **mvp_idx;
    u8  *mvr_idx;
    u8  *bi_idx;
    u8  *inter_dir;
    s16 *mmvd_idx;
    u8  *mmvd_flag;
    s32 affine_bzero[MAX_CU_CNT_IN_LCU][REFP_NUM];
    s16 affine_mvd[MAX_CU_CNT_IN_LCU][REFP_NUM][3][MV_D];
    s16 bv_chroma[MAX_CU_CNT_IN_LCU][MV_D];
    s16 mv[MAX_CU_CNT_IN_LCU][REFP_NUM][MV_D];

    s16 unrefined_mv[MAX_CU_CNT_IN_LCU][REFP_NUM][MV_D];

    s16 mvd[MAX_CU_CNT_IN_LCU][REFP_NUM][MV_D];
    int *nnz[N_C];
    int *nnz_sub[N_C][4];
    u32 *map_scu;
    u8  *affine_flag;
    u32 *map_affine;
    u8* ats_intra_cu;
    u8* ats_mode_h;
    u8* ats_mode_v;
    u8  *ats_inter_info;
    u32 *map_cu_mode;
    s8  *depth;
    s16 *coef[N_C];
    pel *reco[N_C];
} XEVD_CU_DATA;

/* time stamp */
typedef struct _XEVD_TIME_STAMP
{
    XEVD_MTIME         frame_first_dts;
    XEVD_MTIME         frame_duration_time;
} XEVD_TIME_STAMP;

#include "xevd_bsr.h"

typedef struct _XEVD_CORE
{
    /************** current CU **************/
    /* coefficient buffer of current CU */
    s16            coef[N_C][MAX_CU_DIM];
    /* pred buffer of current CU */
    /* [1] is used for bi-pred. */
    pel            pred[2][N_C][MAX_CU_DIM];
    /* neighbor pixel buffer for intra prediction */
    pel            nb[N_C][N_REF][MAX_CU_SIZE * 3];
    /* reference index for current CU */
    s8             refi[REFP_NUM];
    /* motion vector for current CU */
    s16            mv[REFP_NUM][MV_D];

    /* CU position in current frame in SCU unit */
    u32            scup;
    /* CU position X in a frame in SCU unit */
    u16            x_scu;
    /* CU position Y in a frame in SCU unit */
    u16            y_scu;
    /* neighbor CUs availability of current CU */
    u16            avail_cu;
    /* Left, right availability of current CU */
    u16            avail_lr;
    /* intra prediction direction of current CU */
    u8             ipm[2];
    /* most probable mode for intra prediction */
    u8             * mpm_b_list;
    u8             mpm[2];
    u8             mpm_ext[8];
    u8             pims[IPD_CNT]; /* probable intra mode set*/
    /* prediction mode of current CU: INTRA, INTER, ... */
    u8             pred_mode;
    /* log2 of cuw */
    u8             log2_cuw;
    /* log2 of cuh */
    u8             log2_cuh;
    /* is there coefficient? */
    int            is_coef[N_C];
    int            is_coef_sub[N_C][MAX_SUB_TB_NUM];
    /* QP for Luma of current encoding MB */
    u8             qp_y;
    /* QP for Chroma of current encoding MB */
    u8             qp_u;
    u8             qp_v;
    u8             qp;
    u8             cu_qp_delta_code;
    u8             cu_qp_delta_is_coded;

    /************** current LCU *************/
    /* address of current LCU,  */
    s16            lcu_num;
    /* X address of current LCU */
    u16            x_lcu;
    /* Y address of current LCU */
    u16            y_lcu;
    /* left pel position of current LCU */
    u16            x_pel;
    /* top pel position of current LCU */
    u16            y_pel;

    /* split mode map for current LCU */
    s8             (*split_mode)[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU];

    /* platform specific data, if needed */
    void          *pf;

    /* temporal pixel buffer for inter prediction */
    pel            eif_tmp_buffer[ (MAX_CU_SIZE + 2) * (MAX_CU_SIZE + 2) ];
    u8             mvr_idx;
    /* history-based motion vector prediction candidate list */
    XEVD_HISTORY_BUFFER     history_buffer;

    int            mvp_idx[REFP_NUM];
    s16            mvd[REFP_NUM][MV_D];
    int            inter_dir;
    int            bi_idx;

    int            tile_num;
    int            filter_across_boundary;

    XEVD_BSR      * bs;
    XEVD_SBAC    * sbac;
    XEVD_CTX     * ctx;
    u8             skip_flag;
    int            cuw, cuh;
    u16            x;
    u16            y;
    u16            thread_idx;
    u8             ctx_flags[NUM_CNID];
    u8             deblock_is_hor;

} XEVD_CORE;

typedef struct _XEVD_SCAN_TABLES XEVD_SCAN_TABLES;

/******************************************************************************
 * CONTEXT used for decoding process.
 *
 * All have to be stored are in this structure.
 *****************************************************************************/
struct _XEVD_CTX
{
    /* magic code */
    u32                     magic;
    /* XEVD identifier */
    XEVD                    id;
    /* CORE information used for fast operation */
    XEVD_CORE             * core;
    /* current decoding bitstream */
    XEVD_BSR                 bs;
    XEVD_BSR                 bs_mt[XEVD_MAX_TASK_CNT];

    /* current nalu header */
    XEVD_NALU                nalu;
    /* current slice header */
    XEVD_SH                  sh;
    /* decoded picture buffer management */
    XEVD_PM                  dpm;
    /* create descriptor */
    XEVD_CDSC               cdsc;
    /* sequence parameter set */
    XEVD_SPS               * sps;
    XEVD_SPS                 sps_array[16];
    int                      sps_id;
    int                      sps_count;

    /* picture parameter set */
    XEVD_PPS                 pps;

    XEVD_PPS                 pps_array[64];

    /* current decoded (decoding) picture buffer */
    XEVD_PIC               * pic;
    /* SBAC */
    XEVD_SBAC               sbac_dec;
    XEVD_SBAC               sbac_dec_mt[XEVD_MAX_TASK_CNT];

    /* time stamp */
    XEVD_TIME_STAMP         ts;

    /* decoding picture width */
    u16                     w;
    /* decoding picture height */
    u16                     h;
    /* maximum CU width and height */
    u16                     max_cuwh;
    /* log2 of maximum CU width and height */
    u8                      log2_max_cuwh;
    /* minimum CU width and height */
    u16                     min_cuwh;
    /* log2 of minimum CU width and height */
    u8                      log2_min_cuwh;

    /* MAPS *******************************************************************/
    /* SCU map for CU information */
    u32                   * map_scu;
    u8                    * cod_eco;
    /* LCU split information */
    s8                   (* map_split)[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU];

    /* decoded motion vector for every blocks */
    s16                  (* map_mv)[REFP_NUM][MV_D];

    /* reference frame indices */
    s8                   (* map_refi)[REFP_NUM];
    /* intra prediction modes */
    s8                    * map_ipm;
    /* new coding tool flag*/
    u32                   * map_cu_mode;
   /* structure to keep the data for each CTU for CTU level parallelism*/
    XEVD_CU_DATA          * map_cu_data;
    /**************************************************************************/
    /* current slice number, which is increased whenever decoding a slice.
    when receiving a slice for new picture, this value is set to zero.
    this value can be used for distinguishing b/w slices */
    u16                     slice_num;
    /* last coded intra picture's picture order count */
    int                     last_intra_poc;
    /* picture width in LCU unit */
    u16                     w_lcu;
    /* picture height in LCU unit */
    u16                     h_lcu;
    /* picture size in LCU unit (= w_lcu * h_lcu) */
    s32                     f_lcu;
    /* picture width in SCU unit */
    u16                     w_scu;
    /* picture height in SCU unit */
    u16                     h_scu;
    /* picture size in SCU unit (= w_scu * h_scu) */
    u32                     f_scu;
    /* the picture order count value */
    XEVD_POC                 poc;
    /* the picture order count of the previous Tid0 picture */
    u32                     prev_pic_order_cnt_val;
    /* the decoding order count of the previous picture */
    u32                     prev_doc_offset;
    /* the number of currently decoded pictures */
    u32                     pic_cnt;
    /* flag whether current picture is refecened picture or not */
    u8                      slice_ref_flag;
    /* distance between ref pics in addition to closest ref ref pic in LD*/
    int                     ref_pic_gap_length;
    /* picture buffer allocator */
    PICBUF_ALLOCATOR        pa;
    /* bitstream has an error? */
    u8                      bs_err;
    /* reference picture (0: foward, 1: backward) */
    XEVD_REFP                refp[XEVD_MAX_NUM_REF_PICS][REFP_NUM];
    /* flag for picture signature enabling */
    u8                      use_pic_sign;
    /* picture signature (MD5 digest 128bits) for each component */
    u8                      pic_sign[N_C][16];
    /* flag to indicate picture signature existing or not */
    u8                      pic_sign_exist;
    /* tile index map (width in SCU x height in SCU) of
    raster scan order in a frame */
    u8                    * map_tidx;
    /* Number of tils in the picture*/
    u32                     tile_cnt;
    /* Tile information for each index */
    XEVD_TILE              * tile;
    /* number of tile columns */
    u16                     w_tile;
    /* number of tile rows */
    u16                     h_tile;
    /* tile to slice map */
    u16                     tile_in_slice[MAX_NUM_TILES_COL*MAX_NUM_TILES_ROW];
    u16                     tile_order_slice[MAX_NUM_TILES_COL*MAX_NUM_TILES_ROW];
     /* Number of tiles in slice*/
    u16                     num_tiles_in_slice;
    u32                     num_ctb;
    THREAD_CONTROLLER       tc;
    POOL_THREAD             thread_pool[XEVD_MAX_TASK_CNT];
    XEVD_CORE             * core_mt[XEVD_MAX_TASK_CNT];

    int                    internal_codec_bit_depth;
    int                    internal_codec_bit_depth_luma;
    int                    internal_codec_bit_depth_chroma;

    int                     parallel_rows; //Number of parallel rows which can be encoded
    volatile s32          * sync_flag;
    volatile s32          * sync_row;
    SYNC_OBJ                sync_block; //has to be initialized at context creation and has to be released on context destruction
    /* mximum number of coding delay */
    s32                     max_coding_delay;
    /* Frame rate */
	s32                     frame_rate_num;
	s32                     frame_rate_den;
    /* address of ready function */
    int  (* fn_ready)(XEVD_CTX * ctx);
    /* address of flush function */
    void (* fn_flush)(XEVD_CTX * ctx);
    /* function address of decoding input bitstream */
    int  (* fn_dec_cnk)(XEVD_CTX * ctx, XEVD_BITB * bitb, XEVD_STAT * stat);
    /* function address of decoding slice */
    int  (* fn_dec_slice)(XEVD_CTX * ctx, XEVD_CORE * core);
    /* function address of pulling decoded picture */
    int  (* fn_pull)(XEVD_CTX * ctx, XEVD_IMGB ** img);
    /* function address of deblocking filter */
    int  ( * fn_deblock)(void * arg);
    /* function address of picture buffer expand */
    void (* fn_picbuf_expand)(XEVD_CTX * ctx, XEVD_PIC * pic);
    const XEVD_ITXB ( * fn_itxb)[MAX_TR_LOG2];
    void  ( * fn_recon) (s16 *coef, pel *pred, int is_coef, int cuw, int cuh, int s_rec, pel *rec,int bit_depth);
    const XEVD_DBK (*fn_dbk)[2];
    const XEVD_DBK_CH(*fn_dbk_chroma)[2];
    /* platform specific data, if needed */
    void                  * pf;

    XEVD_SCAN_TABLES      * scan_tables;
};



/* prototypes of internal functions */
int  xevd_platform_init(XEVD_CTX * ctx);
void xevd_platform_deinit(XEVD_CTX * ctx);
int  xevd_ready(XEVD_CTX * ctx);
void xevd_flush(XEVD_CTX * ctx);
int  xevd_deblock(void * args);

int  xevd_dec_slice(XEVD_CTX * ctx, XEVD_CORE * core);


#include "xevd_util.h"
#include "xevd_eco.h"
#include "xevd_picman.h"

#include "xevd_tbl.h"
#include "xevd_util.h"
#include "xevd_recon.h"
#include "xevd_ipred.h"
#include "xevd_tbl.h"
#include "xevd_itdq.h"
#include "xevd_picman.h"
#include "xevd_mc.h"
#include "xevd_eco.h"
#include "xevd_df.h"
#if defined(__AVX2__)
#include "x86/xevd_mc_sse.h"
#include "x86/xevd_mc_avx.h"
#include "x86/xevd_itdq_sse.h"
#include "x86/xevd_itdq_avx.h"
#include "x86/xevd_recon_avx.h"
#include "x86/xevd_recon_sse.h"
#include "x86/xevd_dbk_sse.h"
#elif defined(ARM)
#include "xevd_mc_neon.h"
#include "xevd_itdq_neon.h"
#include "xevd_recon_neon.h"
#include "xevd_dbk_neon.h"
#endif
#endif /* _XEVD_DEF_H_ */
