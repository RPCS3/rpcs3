#pragma once

#include "stdafx.h"
#include "GSFrame.h"

class GLGSFrame : public GSFrame
{
private:
	QSurfaceFormat m_format;

public:
	GLGSFrame(int w, int h, QIcon appIcon);

	void* make_context() override;
	void set_current(draw_context_t context) override;
	void delete_context(void* context) override;
	void flip(draw_context_t context) override;
};
