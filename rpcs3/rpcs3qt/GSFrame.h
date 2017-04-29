#ifndef GSFRAME_H
#define GSFRAME_H

#include "stdafx.h"
#include "Emu/RSX/GSRender.h"

#include <QWidget>
#include <QWindow>

class GSFrame : public QWindow, public GSFrameBase
{
	Q_OBJECT
	u64 m_frames = 0;
	QString m_render;

public:
	GSFrame(const QString& title, int w, int h);
signals:
	void RequestCommand(std::function<void()> func);
protected:
	virtual void paintEvent(QPaintEvent *event);
	virtual void closeEvent(QCloseEvent *event);

	void keyPressEvent(QKeyEvent *keyEvent);
	void OnFullScreen();

	void close() override;

	bool shown() override;
	void hide() override;
	void show() override;
	/*void OnLeftDclick(wxMouseEvent&)
	{
	OnFullScreen();
	}*/

	//void SetSize(int width, int height);

	void* handle() const override;

	void* make_context() override;
	void set_current(draw_context_t context) override;
	void delete_context(void* context) override;
	void flip(draw_context_t context) override;
	int client_width() override;
	int client_height() override;

private slots:
	void HandleCommandRequest(std::function<void()> func);

private:
	void HandleUICommand(std::function<void()> func);
};

#endif
