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

#ifndef _XEVD_BSR_H_
#define _XEVD_BSR_H_

#include "xevd_port.h"

typedef struct _XEVD_BSR XEVD_BSR;
/*! Function pointer for */
typedef int (*XEVD_BSR_FN_FLUSH)(XEVD_BSR *bs, int byte);

struct _XEVD_BSR
{
    /* temporary read code buffer */
    u32                code;
    /* left bits count in code */
    int                leftbits;
    /*! address of current bitstream position */
    u8               * cur;
    /*! address of bitstream end */
    u8               * end;
    /*! address of bitstream begin */
    u8               * beg;
    /*! size of original bitstream in byte */
    int                size;
    /*! Function pointer for bs_flush */
    XEVD_BSR_FN_FLUSH  fn_flush;
    /*! arbitrary data, if needs */
    int                ndata[4];
    /*! arbitrary address, if needs */
    void             * pdata[4];
};

#define XEVD_BSR_SKIP_CODE(bs, size) \
    xevd_assert((bs)->leftbits >= (size)); \
    if((size) == 32) {(bs)->code = 0; (bs)->leftbits = 0;} \
    else           {(bs)->code <<= (size); (bs)->leftbits -= (size);}

/*! Is bitstream byte aligned? */
#define XEVD_BSR_IS_BYTE_ALIGN(bs) ((((bs)->leftbits & 0x7) == 0) ? 1: 0)

/* get number of byte consumed */
#define XEVD_BSR_GET_READ_BYTE(bs) ((int)((bs)->cur - (bs)->beg) - ((bs)->leftbits >> 3))

void xevd_bsr_init(XEVD_BSR * bs, u8 * buf, int size, XEVD_BSR_FN_FLUSH fn_flush);
int xevd_bsr_flush(XEVD_BSR * bs, int byte);
int xevd_bsr_clz_in_code(u32 code);
#if TRACE_HLS
#define xevd_bsr_read(A, B, C) xevd_bsr_read_trace(A, B, #B, C)
void xevd_bsr_read_trace(XEVD_BSR * bs, u32 * val, char * name, int size);

#define xevd_bsr_read1(A, B) xevd_bsr_read1_trace(A, B, #B)
void xevd_bsr_read1_trace(XEVD_BSR * bs, u32 * val, char * name);

#define xevd_bsr_read_ue(A, B) xevd_bsr_read_ue_trace(A, B, #B)
void xevd_bsr_read_ue_trace(XEVD_BSR * bs, u32 * val, char * name);

#define xevd_bsr_read_se(A, B) xevd_bsr_read_se_trace(A, B, #B)
void xevd_bsr_read_se_trace(XEVD_BSR * bs, s32 * val, char * name);
#else
void xevd_bsr_read(XEVD_BSR * bs, u32 * val, int size);
void xevd_bsr_read1(XEVD_BSR * bs, u32 * val);
void xevd_bsr_read_ue(XEVD_BSR * bs, u32 * val);
void xevd_bsr_read_se(XEVD_BSR * bs, s32 * val);
#endif

#endif /* _XEVD_BSR_H_ */
