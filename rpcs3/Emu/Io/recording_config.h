#pragma once

#include "Utilities/Config.h"

struct cfg_recording final : cfg::node
{
	cfg_recording();
	bool load();
	void save() const;

	struct node_video : cfg::node
	{
		node_video(cfg::node* _this) : cfg::node(_this, "Video") {}

		cfg::uint<0, 60> framerate{this, "Framerate", 30};
		cfg::uint<0, 7680> width{this, "Width", 1280};
		cfg::uint<0, 4320> height{this, "Height", 720};
		cfg::uint<0, 192> pixel_format{this, "AVPixelFormat", 0}; // AVPixelFormat::AV_PIX_FMT_YUV420P
		cfg::uint<0, 0xFFFF> video_codec{this, "AVCodecID", 12}; // AVCodecID::AV_CODEC_ID_MPEG4
		cfg::uint<0, 25000000> video_bps{this, "Video Bitrate", 4000000};
		cfg::uint<0, 5> max_b_frames{this, "Max B-Frames", 2};
		cfg::uint<0, 20> gop_size{this, "Group of Pictures Size", 12};

	} video{ this };

	struct node_audio : cfg::node
	{
		node_audio(cfg::node* _this) : cfg::node(_this, "Audio") {}
		
		cfg::uint<0x10000, 0x17000> audio_codec{this, "AVCodecID", 86018}; // AVCodecID::AV_CODEC_ID_AAC
		cfg::uint<0, 25000000> audio_bps{this, "Audio Bitrate", 320000};

	} audio{ this };

	const std::string path;
};

extern cfg_recording g_cfg_recording;
