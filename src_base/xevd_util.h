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

#ifndef _XEVD_UTIL_H_
#define _XEVD_UTIL_H_

#include "xevd_def.h"
#include <stdlib.h>

/* determine maximum */
#define XEVD_MAX(a,b)                   (((a) > (b)) ? (a) : (b))

/* determine minimum */
#define XEVD_MIN(a,b)                   (((a) < (b)) ? (a) : (b))

/* absolute a value */
#define XEVD_ABS(a)                     abs(a)

/* absolute a 64-bit value */
#define XEVD_ABS64(a)                   (((a)^((a)>>63)) - ((a)>>63))

/* absolute a 32-bit value */
#define XEVD_ABS32(a)                   (((a)^((a)>>31)) - ((a)>>31))

/* absolute a 16-bit value */
#define XEVD_ABS16(a)                   (((a)^((a)>>15)) - ((a)>>15))

/* clipping within min and max */
#define XEVD_CLIP3(min_x, max_x, value)   XEVD_MAX((min_x), XEVD_MIN((max_x), (value)))

/* clipping within min and max */
#define XEVD_CLIP(n,min,max)            (((n)>(max))? (max) : (((n)<(min))? (min) : (n)))

#define XEVD_SIGN(x)                    (((x) < 0) ? -1 : 1)

/* get a sign from a 16-bit value.
operation: if(val < 0) return 1, else return 0 */
#define XEVD_SIGN_GET(val)              ((val<0)? 1: 0)

/* set sign into a value.
operation: if(sign == 0) return val, else if(sign == 1) return -val */
#define XEVD_SIGN_SET(val, sign)        ((sign)? -val : val)

/* get a sign from a 16-bit value.
operation: if(val < 0) return 1, else return 0 */
#define XEVD_SIGN_GET16(val)            (((val)>>15) & 1)

/* rounded right shift. NOTE: Assume shift >= 1 */
#define XEVD_RRSHIFT(val, shift) (((val) + (1<<((shift) - 1))) >> (shift))


/* set sign into a 16-bit value.
operation: if(sign == 0) return val, else if(sign == 1) return -val */
#define XEVD_SIGN_SET16(val, sign)      (((val) ^ ((s16)((sign)<<15)>>15)) + (sign))
#define XEVD_ALIGN(val, align)          ((((val) + (align) - 1) / (align)) * (align))
#define XEVD_CONV_LOG2(v)               (xevd_tbl_log2[v])
#define XEVD_IMGB_OPT_NONE              (0)

#define XEVD_GET_CHROMA_W_SHIFT(idc)    \
    (((idc+10)==XEVD_CF_YCBCR400) ? 1 : (((idc+10)==XEVD_CF_YCBCR420) ? 1 : (((idc+10)==XEVD_CF_YCBCR422) ? 1 : 0)))

#define XEVD_GET_CHROMA_H_SHIFT(idc)    \
    (((idc+10)==XEVD_CF_YCBCR400) ? 1 : (((idc+10)==XEVD_CF_YCBCR420) ? 1 : 0))

/* create image buffer */
XEVD_IMGB * xevd_imgb_create(int w, int h, int cs, int opt, int pad[XEVD_IMGB_MAX_PLANE], int align[XEVD_IMGB_MAX_PLANE]);
u16 xevd_get_avail_inter(int x_scu, int y_scu, int w_scu, int h_scu, int scup, int cuw, int cuh, u32 *map_scu, u8* map_tidx);
u16 xevd_get_avail_intra(int x_scu, int y_scu, int w_scu, int h_scu, int scup, int log2_cuw, int log2_cuh, u32 *map_scu, u8* map_tidx);
XEVD_PIC* xevd_picbuf_lc_alloc(int w, int h, int pad_l, int pad_c, int *err, int idc, int bit_depth);
void xevd_picbuf_lc_free(XEVD_PIC *pic);
void xevd_picbuf_lc_expand(XEVD_PIC *pic, int exp_l, int exp_c);
void xevd_poc_derivation(XEVD_SPS * sps, int tid, XEVD_POC *poc);
void xevd_get_motion(int scup, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], XEVD_REFP(*refp)[REFP_NUM], int cuw, int cuh, int w_scu, u16 avail, s8 refi[MAX_NUM_MVP], s16 mvp[MAX_NUM_MVP][MV_D]);
void xevd_get_motion_skip_baseline(int slice_type, int scup, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], XEVD_REFP refp[REFP_NUM], int cuw, int cuh, int w_scu,
    s8 refi[REFP_NUM][MAX_NUM_MVP], s16 mvp[REFP_NUM][MAX_NUM_MVP][MV_D], u16 avail_lr);

enum
{
    SPLIT_MAX_PART_COUNT = 4
};

typedef struct _XEVD_SPLIT_STRUCT
{
    int       part_count;
    int       cud[SPLIT_MAX_PART_COUNT];
    int       width[SPLIT_MAX_PART_COUNT];
    int       height[SPLIT_MAX_PART_COUNT];
    int       log_cuw[SPLIT_MAX_PART_COUNT];
    int       log_cuh[SPLIT_MAX_PART_COUNT];
    int       x_pos[SPLIT_MAX_PART_COUNT];
    int       y_pos[SPLIT_MAX_PART_COUNT];
    int       cup[SPLIT_MAX_PART_COUNT];
} XEVD_SPLIT_STRUCT;

//! Count of partitions, correspond to split_mode
int xevd_split_part_count(int split_mode);
//! Get partition size
int xevd_split_get_part_size(int split_mode, int part_num, int length);
//! Get partition size log
int xevd_split_get_part_size_idx(int split_mode, int part_num, int length_idx);
//! Get partition split structure
void xevd_split_get_part_structure(int split_mode, int x0, int y0, int cuw, int cuh, int cup, int cud, int log2_culine, XEVD_SPLIT_STRUCT* split_struct);
//! Get split direction. Quad will return vertical direction.
SPLIT_DIR xevd_split_get_direction(SPLIT_MODE mode);
//! Check that mode is vertical
int xevd_split_is_vertical(SPLIT_MODE mode);
//! Check that mode is horizontal
int xevd_split_is_horizontal(SPLIT_MODE mode);

void xevd_get_mv_dir(XEVD_REFP refp[REFP_NUM], u32 poc, int scup, int c_scu, u16 w_scu, u16 h_scu, s16 mvp[REFP_NUM][MV_D]);
int  xevd_get_avail_cu(int neb_scua[MAX_NEB2], u32 * map_cu, u8 * map_tidx);
int  xevd_scan_tbl_init(XEVD_CTX * ctx);
int  xevd_scan_tbl_delete(XEVD_CTX * ctx);
int  xevd_get_split_mode(s8* split_mode, int cud, int cup, int cuw, int cuh, int lcu_s, s8 (* split_mode_buf)[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU]);

void xevd_set_split_mode(s8  split_mode, int cud, int cup, int cuw, int cuh, int lcu_s, s8 (*split_mode_buf)[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU]);
u16 xevd_check_nev_avail(int x_scu, int y_scu, int cuw, int cuh, int w_scu, int h_scu, u32 * map_scu, u8* map_tidx);
u16 xevd_check_eco_nev_avail(int x_scu, int y_scu, int cuw, int cuh, int w_scu, int h_scu, u8 * cod_eco, u8* map_tidx);
/* MD5 structure */
typedef struct _XEVD_MD5
{
    u32     h[4]; /* hash state ABCD */
    u8      msg[64]; /*input buffer (nalu message) */
    u32     bits[2]; /* number of bits, modulo 2^64 (lsb first)*/
} XEVD_MD5;

/* MD5 Functions */
void xevd_md5_init(XEVD_MD5 * md5);
void xevd_md5_update(XEVD_MD5 * md5, void * buf, u32 len);
void xevd_md5_update_16(XEVD_MD5 * md5, void * buf, u32 len);
void xevd_md5_finish(XEVD_MD5 * md5, u8 digest[16]);
int xevd_md5_imgb(XEVD_IMGB * imgb, u8 digest[N_C][16]);

int xevd_picbuf_signature(XEVD_PIC * pic, u8 md5_out[N_C][16]);

int xevd_atomic_inc(volatile int * pcnt);
int xevd_atomic_dec(volatile int * pcnt);

void xevd_init_inverse_scan_sr(u16 *scan_inv, u16 *scan_orig, int width, int height, int scan_type);
void xevd_get_ctx_last_pos_xy_para(int ch_type, int width, int height, int *result_offset_x, int *result_offset_y, int *result_shift_x, int *result_shift_y);
void xevd_eco_sbac_ctx_initialize(SBAC_CTX_MODEL *ctx, s16 *ctx_init_model, u16 num_ctx, u8 slice_type, u8 slice_qp);

BOOL xevd_check_bi_applicability(int slice_type, int cuw, int cuh);
void xevd_block_copy(s16 * src, int src_stride, s16 * dst, int dst_stride, int log2_copy_w, int log2_copy_h);
int xevd_get_luma_cup(int x_scu, int y_scu, int cu_w_scu, int cu_h_scu, int w_scu);

extern const int xevd_chroma_format_idc_to_imgb_cs[4];

void xevd_picbuf_expand(XEVD_CTX * ctx, XEVD_PIC * pic);
XEVD_PIC * xevd_picbuf_alloc(PICBUF_ALLOCATOR * pa, int * ret, int bit_depth);
void xevd_picbuf_free(PICBUF_ALLOCATOR * pa, XEVD_PIC * pic);
int xevd_picbuf_check_signature(XEVD_PIC * pic, u8 signature[N_C][16], int bit_depth);

/* set decoded information, such as MVs, inter_dir, etc. */
void xevd_set_dec_info(XEVD_CTX * ctx, XEVD_CORE * core);

#define XEVD_CPU_INFO_SSE2     0x7A // ((3 << 5) | 26)
#define XEVD_CPU_INFO_SSE3     0x40 // ((2 << 5) |  0)
#define XEVD_CPU_INFO_SSSE3    0x49 // ((2 << 5) |  9)
#define XEVD_CPU_INFO_SSE41    0x53 // ((2 << 5) | 19)
#define XEVD_CPU_INFO_OSXSAVE  0x5B // ((2 << 5) | 27)
#define XEVD_CPU_INFO_AVX      0x5C // ((2 << 5) | 28)
#define XEVD_CPU_INFO_AVX2     0x25 // ((1 << 5) |  5)

int  xevd_check_cpu_info();

#endif /* _XEVD_UTIL_H_ */
