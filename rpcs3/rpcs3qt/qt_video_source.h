#pragma once

#include "util/video_source.h"
#include "util/atomic.hpp"
#include "Utilities/mutex.h"

#include <QMovie>
#include <QBuffer>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QPixmap>

class qt_video_source : public video_source
{
public:
	qt_video_source();
	virtual ~qt_video_source();

	void set_video_path(const std::string& video_path) override;
	const QString& video_path() const { return m_video_path; }

	void get_image(std::vector<u8>& data, int& w, int& h, int& ch, int& bpp) override;
	bool has_new() const override { return m_has_new; }

	void set_active(bool active) override;
	bool get_active() const override { return m_active; }

	void start_movie();
	void stop_movie();

	QPixmap get_movie_image(const QVideoFrame& frame) const;

	void image_change_callback(const QVideoFrame& frame = {}) const;
	void set_image_change_callback(const std::function<void(const QVideoFrame&)>& func);

protected:
	void init_movie();

	shared_mutex m_image_mutex;

	atomic_t<bool> m_active = false;
	atomic_t<bool> m_has_new = false;

	QString m_video_path;
	QByteArray m_video_data{};
	QImage m_image{};
	std::vector<u8> m_image_path;

	std::unique_ptr<QBuffer> m_video_buffer;
	std::unique_ptr<QMediaPlayer> m_media_player;
	std::unique_ptr<QVideoSink> m_video_sink;
	std::unique_ptr<QMovie> m_movie;

	std::function<void(const QVideoFrame&)> m_image_change_callback = nullptr;

	friend class qt_video_source_wrapper;
};

// Wrapper for emulator usage
class qt_video_source_wrapper : public video_source
{
public:
	qt_video_source_wrapper() : video_source() {}
	virtual ~qt_video_source_wrapper();

	void set_video_path(const std::string& video_path) override;
	void set_active(bool active) override;
	bool get_active() const override;
	bool has_new() const override { return m_qt_video_source && m_qt_video_source->has_new(); }
	void get_image(std::vector<u8>& data, int& w, int& h, int& ch, int& bpp) override;

private:
	std::unique_ptr<qt_video_source> m_qt_video_source;
};
