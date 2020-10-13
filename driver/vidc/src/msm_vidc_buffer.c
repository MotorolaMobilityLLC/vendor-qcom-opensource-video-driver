// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 */

#include <media/msm_media_info.h>
#include "msm_vidc_buffer.h"
#include "msm_vidc_inst.h"
#include "msm_vidc_core.h"
#include "msm_vidc_driver.h"
#include "msm_vidc_debug.h"
#include "msm_vidc_internal.h"

#define MIN_INPUT_BUFFERS 4
#define MIN_ENC_OUTPUT_BUFFERS 4

u32 msm_vidc_input_min_count(struct msm_vidc_inst *inst)
{
	u32 input_min_count = 0;
	//struct v4l2_ctrl *max_layer = NULL;

	if (!inst) {
		d_vpr_e("%s: invalid params\n", __func__);
		return 0;
	}

	if (!is_decode_session(inst) && !is_encode_session(inst))
		return 0;

	input_min_count = MIN_INPUT_BUFFERS;

	if (is_thumbnail_session(inst))
		input_min_count = 1;

	//if (is_grid_session(inst))
	//	input_min_count = 2;

	//if (is_hier_b_session(inst)) {
		//max_layer = get_ctrl(inst,
		//	V4L2_CID_MPEG_VIDC_VIDEO_HEVC_MAX_HIER_CODING_LAYER);
		//input_min_count = (1 << (max_layer->val - 1)) + 2;
	//}

	return input_min_count;
}

u32 msm_vidc_output_min_count(struct msm_vidc_inst *inst)
{
	u32 output_min_count;

	if (!inst) {
		d_vpr_e("%s: invalid params\n", __func__);
		return 0;
	}

	if (!is_decode_session(inst) && !is_encode_session(inst))
		return 0;

	if (is_thumbnail_session(inst))
		return inst->codec == MSM_VIDC_VP9 ? 8 : 1;

	if (is_decode_session(inst)) {
		switch (inst->codec) {
		case MSM_VIDC_H264:
		case MSM_VIDC_HEVC:
			output_min_count = 4;
			break;
		case MSM_VIDC_VP9:
			output_min_count = 9;
			break;
		case MSM_VIDC_MPEG2:
			output_min_count = 6;
			break;
		default:
			output_min_count = 4;
		}
	} else {
		output_min_count = MIN_ENC_OUTPUT_BUFFERS;
	}

	//if (is_vpp_delay_allowed(inst)) {
	//	output_min_count =
	//		max(output_min_count, (u32)MAX_BSE_VPP_DELAY);
	//	output_min_count =
	//		max(output_min_count, (u32)(msm_vidc_vpp_delay & 0x1F));
	//}

	return output_min_count;
}

u32 msm_vidc_input_extra_count(struct msm_vidc_inst *inst)
{
	u32 extra_input_count = 0;
	struct msm_vidc_core *core;

	if (!inst || !inst->core) {
		d_vpr_e("%s: invalid params %pK\n", __func__, inst);
		return 0;
	}
	core = inst->core;

	if (is_thumbnail_session(inst))
		return extra_input_count;

	if (is_decode_session(inst)) {
		/* add dcvs buffers */
		/* add batching buffers */
		extra_input_count = 6;
	} else if (is_encode_session(inst)) {
		/* add dcvs buffers */
		extra_input_count = 4;
	}
	return extra_input_count;
}

u32 msm_vidc_output_extra_count(struct msm_vidc_inst *inst)
{
	u32 extra_output_count = 0;
	struct msm_vidc_core *core;

	if (!inst || !inst->core) {
		d_vpr_e("%s: invalid params %pK\n", __func__, inst);
		return 0;
	}
	core = inst->core;

	if (is_thumbnail_session(inst))
		return 0;

	if (is_decode_session(inst)) {
		/* add dcvs buffers */
		/* add batching buffers */
		extra_output_count = 6;
	} else if (is_encode_session(inst)) {
		/* add heif buffers */
		//extra_output_count = 8
	}
	return extra_output_count;
}

u32 msm_vidc_decoder_input_size(struct msm_vidc_inst *inst)
{
	u32 size = ALIGN(15 * 1024 * 1024, SZ_4K);
	size = 4; // TODO
	return size;
}

u32 msm_vidc_decoder_output_size(struct msm_vidc_inst *inst)
{
	u32 size;
	u32 format;
	struct v4l2_format *f;

	f = &inst->fmts[OUTPUT_PORT];
	format = v4l2_colorformat_to_media(f->fmt.pix_mp.pixelformat, __func__);
	size = VENUS_BUFFER_SIZE(format, f->fmt.pix_mp.width,
			f->fmt.pix_mp.height);
	return size;
}

u32 msm_vidc_decoder_input_meta_size(struct msm_vidc_inst *inst)
{
	return ALIGN(16 * 1024, SZ_4K);
}

u32 msm_vidc_decoder_output_meta_size(struct msm_vidc_inst *inst)
{
	return ALIGN(16 * 1024, SZ_4K);
}

u32 msm_vidc_encoder_input_size(struct msm_vidc_inst *inst)
{
	u32 size;
	u32 format;
	struct v4l2_format *f;

	f = &inst->fmts[INPUT_PORT];
	format = v4l2_colorformat_to_media(f->fmt.pix_mp.pixelformat, __func__);
	size = VENUS_BUFFER_SIZE(format, f->fmt.pix_mp.width,
			f->fmt.pix_mp.height);
	return size;
}

u32 msm_vidc_encoder_output_size(struct msm_vidc_inst *inst)
{
	u32 frame_size;
	u32 mbs_per_frame;
	u32 width, height;
	struct v4l2_format *f;

	f = &inst->fmts[OUTPUT_PORT];
	/*
	 * Encoder output size calculation: 32 Align width/height
	 * For resolution < 720p : YUVsize * 4
	 * For resolution > 720p & <= 4K : YUVsize / 2
	 * For resolution > 4k : YUVsize / 4
	 * Initially frame_size = YUVsize * 2;
	 */

	/* if (is_grid_session(inst)) {
		f->fmt.pix_mp.width = f->fmt.pix_mp.height = HEIC_GRID_DIMENSION;
	} */
	width = ALIGN(f->fmt.pix_mp.width, BUFFER_ALIGNMENT_SIZE(32));
	height = ALIGN(f->fmt.pix_mp.height, BUFFER_ALIGNMENT_SIZE(32));
	mbs_per_frame = NUM_MBS_PER_FRAME(width, height);
	frame_size = (width * height * 3);

	if (mbs_per_frame < NUM_MBS_720P)
		frame_size = frame_size << 1;
	else if (mbs_per_frame <= NUM_MBS_4k)
		frame_size = frame_size >> 2;
	else
		frame_size = frame_size >> 3;

	/*if ((inst->rc_type == RATE_CONTROL_OFF) ||
		(inst->rc_type == V4L2_MPEG_VIDEO_BITRATE_MODE_CQ))
		frame_size = frame_size << 1;

	if (inst->rc_type == RATE_CONTROL_LOSSLESS)
		frame_size = (width * height * 9) >> 2; */

	/* multiply by 10/8 (1.25) to get size for 10 bit case */
	if (f->fmt.pix_mp.pixelformat == V4L2_PIX_FMT_HEVC)
		frame_size = frame_size + (frame_size >> 2);

	return ALIGN(frame_size, SZ_4K);
}

u32 msm_vidc_encoder_input_meta_size(struct msm_vidc_inst *inst)
{
	return ALIGN(1 * 1024 * 1024, SZ_4K);
}

u32 msm_vidc_encoder_output_meta_size(struct msm_vidc_inst *inst)
{
	return ALIGN(16 * 1024, SZ_4K);
}
