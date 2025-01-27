#pragma once

#include "Input/camera_video_sink.h"

#include <QVideoFrame>
#include <QVideoSink>

class qt_camera_video_sink final : public camera_video_sink, public QVideoSink
{
public:
	qt_camera_video_sink(bool front_facing, QObject *parent = nullptr);
	virtual ~qt_camera_video_sink();

	bool present(const QVideoFrame& frame);
};
