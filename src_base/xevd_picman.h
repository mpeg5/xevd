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

#ifndef _XEVD_PICMAN_H_
#define _XEVD_PICMAN_H_

int xevd_picman_refp_init(XEVD_PM *pm, int max_num_ref_pics, int slice_type, s32 poc, u8 layer_id, int last_intra, XEVD_REFP (*refp)[REFP_NUM]);
XEVD_PIC * xevd_picman_get_empty_pic(XEVD_PM *pm, int *err, int bit_depth);
int xevd_picman_put_pic(XEVD_PM *pm, XEVD_PIC *pic, int is_idr, s32 poc, u8 layer_id, int need_for_output, XEVD_REFP (*refp)[REFP_NUM], int ref_pic, int ref_pic_gap_length);
XEVD_PIC * xevd_picman_out_pic(XEVD_PM *pm, int *err);
int xevd_picman_deinit(XEVD_PM *pm);
int xevd_picman_init(XEVD_PM *pm, int max_pb_size, int max_num_ref_pics, PICBUF_ALLOCATOR *pa);

#endif /* _XEVD_PICMAN_H_ */
