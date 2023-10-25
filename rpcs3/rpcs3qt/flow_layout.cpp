/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#include "flow_layout.h"

flow_layout::flow_layout(QWidget* parent, int margin, bool dynamic_spacing, int hSpacing, int vSpacing)
	: QLayout(parent)
	, m_dynamic_spacing(dynamic_spacing)
	, m_hSpaceInitial(hSpacing)
	, m_vSpaceInitial(vSpacing)
	, m_hSpace(hSpacing)
	, m_vSpace(vSpacing)
{
	setContentsMargins(margin, margin, margin, margin);
}

flow_layout::flow_layout(int margin, bool dynamic_spacing, int hSpacing, int vSpacing)
	: m_dynamic_spacing(dynamic_spacing)
	, m_hSpaceInitial(hSpacing)
	, m_vSpaceInitial(vSpacing)
	, m_hSpace(hSpacing)
	, m_vSpace(vSpacing)
{
	setContentsMargins(margin, margin, margin, margin);
}

flow_layout::~flow_layout()
{
	clear();
}

void flow_layout::clear()
{
	while (QLayoutItem* item = takeAt(0))
	{
		delete item->widget();
		delete item;
	}
	itemList.clear();
	m_positions.clear();
}

void flow_layout::addItem(QLayoutItem* item)
{
	m_positions.append(position{ .row = -1, .col = -1 });
	itemList.append(item);
}

int flow_layout::horizontalSpacing() const
{
	if (m_hSpace >= 0)
	{
		return m_hSpace;
	}

	return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

int flow_layout::verticalSpacing() const
{
	if (m_vSpace >= 0)
	{
		return m_vSpace;
	}

	return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}

int flow_layout::count() const
{
	return itemList.size();
}

QLayoutItem* flow_layout::itemAt(int index) const
{
	return itemList.value(index);
}

QLayoutItem* flow_layout::takeAt(int index)
{
	if (index >= 0 && index < itemList.size())
	{
		m_positions.takeAt(index);
		return itemList.takeAt(index);
	}

	return nullptr;
}

Qt::Orientations flow_layout::expandingDirections() const
{
	return {};
}

bool flow_layout::hasHeightForWidth() const
{
	return true;
}

int flow_layout::heightForWidth(int width) const
{
	return doLayout(QRect(0, 0, width, 0), true);
}

void flow_layout::setGeometry(const QRect& rect)
{
	QLayout::setGeometry(rect);
	doLayout(rect, false);
}

QSize flow_layout::sizeHint() const
{
	return minimumSize();
}

QSize flow_layout::minimumSize() const
{
	QSize size;
	for (const QLayoutItem* item : itemList)
	{
		if (item)
		{
			size = size.expandedTo(item->minimumSize());
		}
	}

	const QMargins margins = contentsMargins();
	size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
	return size;
}

int flow_layout::doLayout(const QRect& rect, bool testOnly) const
{
	int left, top, right, bottom;
	getContentsMargins(&left, &top, &right, &bottom);
	const QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
	int x = effectiveRect.x();
	int y = effectiveRect.y();
	int lineHeight = 0;
	int rows = 0;
	int cols = 0;

	if (m_dynamic_spacing)
	{
		const int available_width = effectiveRect.width();
		const int min_spacing = smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
		bool fits_into_width = true;
		int width = 0;
		int index = 0;

		for (; index < itemList.size(); index++)
		{
			if (QLayoutItem* item = itemList.at(index))
			{
				const int new_width = width + item->sizeHint().width() + (width > 0 ? min_spacing : 0);

				if (new_width > effectiveRect.width())
				{
					fits_into_width = false;
					break;
				}

				width = new_width;
			}
		}

		// Try to evenly distribute the items across the width
		m_hSpace = (index == 0) ? -1 : ((available_width - width) / index);

		if (fits_into_width)
		{
			// Make sure there aren't huge gaps between the items
			m_hSpace = smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
		}
	}
	else
	{
		m_hSpace = m_hSpaceInitial;
	}

	for (int i = 0, row = 0, col = 0; i < itemList.size(); i++)
	{
		QLayoutItem* item = itemList.at(i);
		if (!item)
			continue;

		const QWidget* wid = item->widget();
		if (!wid)
			continue;

		int spaceX = horizontalSpacing();
		if (spaceX == -1)
			spaceX = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);

		int spaceY = verticalSpacing();
		if (spaceY == -1)
			spaceY = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);

		int nextX = x + item->sizeHint().width() + spaceX;
		if (nextX - spaceX > effectiveRect.right() && lineHeight > 0)
		{
			x = effectiveRect.x();
			y = y + lineHeight + spaceY;
			nextX = x + item->sizeHint().width() + spaceX;
			lineHeight = 0;
			col = 0;
			row++;
		}

		position& pos = m_positions[i];
		pos.row = row;
		pos.col = col++;

		rows = std::max(rows, pos.row + 1);
		cols = std::max(cols, pos.col + 1);

		if (!testOnly)
			item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

		x = nextX;
		lineHeight = qMax(lineHeight, item->sizeHint().height());
	}

	m_rows = rows;
	m_cols = cols;

	return y + lineHeight - rect.y() + bottom;
}

int flow_layout::smartSpacing(QStyle::PixelMetric pm) const
{
	const QObject* parent = this->parent();
	if (!parent)
	{
		return -1;
	}

	if (parent->isWidgetType())
	{
		const QWidget* pw = static_cast<const QWidget*>(parent);
		return pw->style()->pixelMetric(pm, nullptr, pw);
	}

	return static_cast<const QLayout*>(parent)->spacing();
}
