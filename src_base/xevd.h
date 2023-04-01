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

#ifndef _XEVD_H_
#define _XEVD_H_

#ifdef __cplusplus

extern "C"
{
#endif

#include <stdint.h>
#include "xevd_exports.h"

/* xevd decoder const */
#define XEVD_MAX_TASK_CNT                  8

/*****************************************************************************
 * return values and error code
 *****************************************************************************/
/* no more frames, but it is OK */
#define XEVD_OK_NO_MORE_FRM              (205)
/* progress success, but output is not available temporarily */
#define XEVD_OK_OUT_NOT_AVAILABLE        (204)
/* frame dimension (width or height) has been changed */
#define XEVD_OK_DIM_CHANGED              (203)
/* decoding success, but output frame has been delayed */
#define XEVD_OK_FRM_DELAYED              (202)
/* CRC value presented but ignored at decoder*/
#define XEVD_WARN_CRC_IGNORED            (200)

#define XEVD_OK                          (0)

#define XEVD_ERR                         (-1) /* generic error */
#define XEVD_ERR_INVALID_ARGUMENT        (-101)
#define XEVD_ERR_OUT_OF_MEMORY           (-102)
#define XEVD_ERR_REACHED_MAX             (-103)
#define XEVD_ERR_UNSUPPORTED             (-104)
#define XEVD_ERR_UNEXPECTED              (-105)
#define XEVD_ERR_UNSUPPORTED_COLORSPACE  (-201)
#define XEVD_ERR_MALFORMED_BITSTREAM     (-202)
#define XEVD_ERR_THREAD_ALLOCATION       (-203)
/* not matched CRC value */
#define XEVD_ERR_BAD_CRC                 (-300)
#define XEVD_ERR_UNKNOWN                 (-32767) /* unknown error */

/* return value checking *****************************************************/
#define XEVD_SUCCEEDED(ret)              ((ret) >= XEVD_OK)
#define XEVD_FAILED(ret)                 ((ret) < XEVD_OK)

/*****************************************************************************
 * color spaces
 *****************************************************************************/

/* color formats */
#define XEVD_CF_UNKNOWN                 0 /* unknown color format */
#define XEVD_CF_YCBCR400                10 /* Y only */
#define XEVD_CF_YCBCR420                11 /* YCbCr 420 */
#define XEVD_CF_YCBCR422                12 /* YCBCR 422 narrow chroma*/
#define XEVD_CF_YCBCR444                13 /* YCBCR 444*/
#define XEVD_CF_YCBCR422N               XEVD_CF_YCBCR422
#define XEVD_CF_YCBCR422W               18 /* YCBCR422 wide chroma */

/* macro for color space */
#define XEVD_CS_GET_FORMAT(cs)           (((cs) >> 0) & 0xFF)
#define XEVD_CS_GET_BIT_DEPTH(cs)        (((cs) >> 8) & 0x3F)
#define XEVD_CS_GET_BYTE_DEPTH(cs)       ((XEVD_CS_GET_BIT_DEPTH(cs)+7)>>3)
#define XEVD_CS_GET_ENDIAN(cs)           (((cs) >> 14) & 0x1)
#define XEVD_CS_SET(f, bit, e)           (((e) << 14) | ((bit) << 8) | (f))
#define XEVD_CS_SET_FORMAT(cs, v)        (((cs) & ~0xFF) | ((v) << 0))
#define XEVD_CS_SET_BIT_DEPTH(cs, v)     (((cs) & ~(0x3F<<8)) | ((v) << 8))
#define XEVD_CS_SET_ENDIAN(cs, v)        (((cs) & ~(0x1<<14)) | ((v) << 14))

/* pre-defined color spaces */
#define XEVD_CS_UNKNOWN                  XEVD_CS_SET(0,0,0)
#define XEVD_CS_YCBCR400                 XEVD_CS_SET(XEVD_CF_YCBCR400, 8, 0)
#define XEVD_CS_YCBCR420                 XEVD_CS_SET(XEVD_CF_YCBCR420, 8, 0)
#define XEVD_CS_YCBCR422                 XEVD_CS_SET(XEVD_CF_YCBCR422, 8, 0)
#define XEVD_CS_YCBCR444                 XEVD_CS_SET(XEVD_CF_YCBCR444, 8, 0)
#define XEVD_CS_YCBCR400_10LE            XEVD_CS_SET(XEVD_CF_YCBCR400, 10, 0)
#define XEVD_CS_YCBCR420_10LE            XEVD_CS_SET(XEVD_CF_YCBCR420, 10, 0)
#define XEVD_CS_YCBCR422_10LE            XEVD_CS_SET(XEVD_CF_YCBCR422, 10, 0)
#define XEVD_CS_YCBCR444_10LE            XEVD_CS_SET(XEVD_CF_YCBCR444, 10, 0)
#define XEVD_CS_YCBCR400_12LE            XEVD_CS_SET(XEVD_CF_YCBCR400, 12, 0)
#define XEVD_CS_YCBCR420_12LE            XEVD_CS_SET(XEVD_CF_YCBCR420, 12, 0)
#define XEVD_CS_YCBCR400_14LE            XEVD_CS_SET(XEVD_CF_YCBCR400, 14, 0)
#define XEVD_CS_YCBCR420_14LE            XEVD_CS_SET(XEVD_CF_YCBCR420, 14, 0)

/*****************************************************************************
* config types for decoder
*****************************************************************************/
#define XEVD_CFG_SET_USE_PIC_SIGNATURE  (301)
#define XEVD_CFG_GET_CODEC_BIT_DEPTH    (401)
#define XEVD_CFG_GET_WIDTH              (402)
#define XEVD_CFG_GET_HEIGHT             (403)
#define XEVD_CFG_GET_CODED_WIDTH        (404)
#define XEVD_CFG_GET_CODED_HEIGHT       (405)
#define XEVD_CFG_GET_COLOR_SPACE        (406)
#define XEVD_CFG_GET_MAX_CODING_DELAY   (407)


/*****************************************************************************
 * NALU types
 *****************************************************************************/
#define XEVD_NAL_UNIT_LENGTH_BYTE        (4)
#define XEVD_NUT_NONIDR                  (0)
#define XEVD_NUT_IDR                     (1)
#define XEVD_NUT_SPS                     (24)
#define XEVD_NUT_PPS                     (25)
#define XEVD_NUT_APS                     (26)
#define XEVD_NUT_FD                      (27)
#define XEVD_NUT_SEI                     (28)

/*****************************************************************************
 * slice type
 *****************************************************************************/
#define XEVD_ST_UNKNOWN                  (-1)
#define XEVD_ST_B                        (0)
#define XEVD_ST_P                        (1)
#define XEVD_ST_I                        (2)

/*****************************************************************************
 * type and macro for media time
 *****************************************************************************/
typedef long long                       XEVD_MTIME; /* media time in 100-nanosec unit */
#define XEVD_TS_PTS                     0
#define XEVD_TS_DTS                     1
#define XEVD_TS_NUM                     2

/*****************************************************************************
 * macro for arbitrary data and arbitrary addresses
 *****************************************************************************/
#define XEVD_NDATA_NUM                  4
#define XEVD_PDATA_NUM                  4

/*****************************************************************************
 * image buffer format
 *****************************************************************************
 baddr
    +---------------------------------------------------+ ---
    |                                                   |  ^
    |                                              |    |  |
    |    a                                         v    |  |
    |   --- +-----------------------------------+ ---   |  |
    |    ^  |  (x, y)                           |  y    |  |
    |    |  |   +---------------------------+   + ---   |  |
    |    |  |   |                           |   |  ^    |  |
    |    |  |   |                           |   |  |    |  |
    |    |  |   |                           |   |  |    |  |
    |    |  |   |                           |   |  |    |  |
    |       |   |                           |   |       |
    |    ah |   |                           |   |  h    |  e
    |       |   |                           |   |       |
    |    |  |   |                           |   |  |    |  |
    |    |  |   |                           |   |  |    |  |
    |    |  |   |                           |   |  v    |  |
    |    |  |   +---------------------------+   | ---   |  |
    |    v  |                                   |       |  |
    |   --- +---+-------------------------------+       |  |
    |     ->| x |<----------- w ----------->|           |  |
    |       |<--------------- aw -------------->|       |  |
    |                                                   |  v
    +---------------------------------------------------+ ---

    |<---------------------- s ------------------------>|

 *****************************************************************************/

#define XEVD_IMGB_MAX_PLANE              (4)

typedef struct _XEVD_IMGB XEVD_IMGB;
struct _XEVD_IMGB
{
    int                 cs; /* color space */
    int                 np; /* number of plane */
    /* width (in unit of pixel) */
    int                 w[XEVD_IMGB_MAX_PLANE];
    /* height (in unit of pixel) */
    int                 h[XEVD_IMGB_MAX_PLANE];
    /* X position of left top (in unit of pixel) */
    int                 x[XEVD_IMGB_MAX_PLANE];
    /* Y postion of left top (in unit of pixel) */
    int                 y[XEVD_IMGB_MAX_PLANE];
    /* buffer stride (in unit of byte) */
    int                 s[XEVD_IMGB_MAX_PLANE];
    /* buffer elevation (in unit of byte) */
    int                 e[XEVD_IMGB_MAX_PLANE];
    /* address of each plane */
    void              * a[XEVD_IMGB_MAX_PLANE];

    /* time-stamps */
    XEVD_MTIME          ts[XEVD_TS_NUM];

    int                 ndata[XEVD_NDATA_NUM]; /* arbitrary data, if needs */
    void              * pdata[XEVD_PDATA_NUM]; /* arbitrary adedress if needs */

    /* aligned width (in unit of pixel) */
    int                 aw[XEVD_IMGB_MAX_PLANE];
    /* aligned height (in unit of pixel) */
    int                 ah[XEVD_IMGB_MAX_PLANE];

    /* left padding size (in unit of pixel) */
    int                 padl[XEVD_IMGB_MAX_PLANE];
    /* right padding size (in unit of pixel) */
    int                 padr[XEVD_IMGB_MAX_PLANE];
    /* up padding size (in unit of pixel) */
    int                 padu[XEVD_IMGB_MAX_PLANE];
    /* bottom padding size (in unit of pixel) */
    int                 padb[XEVD_IMGB_MAX_PLANE];

    /* address of actual allocated buffer */
    void              * baddr[XEVD_IMGB_MAX_PLANE];
    /* actual allocated buffer size */
    int                 bsize[XEVD_IMGB_MAX_PLANE];

    /* life cycle management */
    int                 refcnt;
    int                 (*addref)(XEVD_IMGB * imgb);
    int                 (*getref)(XEVD_IMGB * imgb);
    int                 (*release)(XEVD_IMGB * imgb);
    int                 crop_idx;
    int                 crop_l;
    int                 crop_r;
    int                 crop_t;
    int                 crop_b;
    int                 imgb_active_pps_id;
    int                 imgb_active_aps_id;
};

/*****************************************************************************
 * Bitstream buffer
 *****************************************************************************/
typedef struct _XEVD_BITB
{
    /* user space address indicating buffer */
    void              * addr;
    /* physical address indicating buffer, if any */
    void              * pddr;
    /* byte size of buffer memory */
    int                 bsize;
    /* byte size of bitstream in buffer */
    int                 ssize;
    /* bitstream has an error? */
    int                 err;
    /* arbitrary data, if needs */
    int                 ndata[XEVD_NDATA_NUM];
    /* arbitrary address, if needs */
    void              * pdata[XEVD_PDATA_NUM];
    /* time-stamps */
    XEVD_MTIME          ts[XEVD_TS_NUM];

} XEVD_BITB;

/*****************************************************************************
 * description for creating of decoder
 *****************************************************************************/
typedef struct _XEVD_CDSC
{
    int            threads; /* number of thread */
} XEVD_CDSC;

/*****************************************************************************
 * status after decoder operation
 *****************************************************************************/
typedef struct _XEVD_STAT
{
    /* byte size of decoded bitstream (read size of bitstream) */
    int            read;
    /* nal unit type */
    int            nalu_type;
    /* slice type */
    int            stype;
    /* frame number monotonically increased whenever decoding a frame.
    note that it has negative value if the decoded data is not frame */
    int            fnum;
    /* picture order count */
    int            poc;
    /* layer id */
    int            tid;

    /* number of reference pictures */
    unsigned char  refpic_num[2];
    /* list of reference pictures */
    int            refpic[2][16];
} XEVD_STAT;

/*****************************************************************************
 * brief information of bitstream
 *****************************************************************************/
typedef struct _XEVD_INFO
{
    /* nal unit length (available for Annex-B format) */
    int            nalu_len;
    /* nal unit type */
    int            nalu_type;
    /* nal unit temporal id */
    int            nalu_tid;
} XEVD_INFO;

/*****************************************************************************
 * API for decoder only
 *****************************************************************************/
/* XEVD instance identifier for decoder */
typedef void  * XEVD;

XEVD XEVD_EXPORT xevd_create(XEVD_CDSC * cdsc, int * err);
void XEVD_EXPORT xevd_delete(XEVD id);
int  XEVD_EXPORT xevd_decode(XEVD id, XEVD_BITB * bitb, XEVD_STAT * stat);
int  XEVD_EXPORT xevd_pull(XEVD id, XEVD_IMGB ** img);
int  XEVD_EXPORT xevd_config(XEVD id, int cfg, void * buf, int * size);
int  XEVD_EXPORT xevd_info(void * bits, int bits_size, int is_annexb, XEVD_INFO * info);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _XEVD_H_ */
