/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 * 
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

class BaseCpuUsageProvider
{
public:
	BaseCpuUsageProvider() {}
	virtual ~BaseCpuUsageProvider() throw() {}

	virtual bool IsImplemented() const=0;
	virtual void UpdateStats()=0;
	virtual int GetEEcorePct() const=0;
	virtual int GetGsPct() const=0;
	virtual int GetGuiPct() const=0;
};


class CpuUsageProvider : public BaseCpuUsageProvider
{
protected:
	ScopedPtr<BaseCpuUsageProvider>		m_Implementation;

public:
	CpuUsageProvider();
	virtual ~CpuUsageProvider() throw();

	virtual bool IsImplemented() const	{ return m_Implementation->IsImplemented(); }
	virtual void UpdateStats()			{ m_Implementation->UpdateStats(); }
	virtual int GetEEcorePct() const	{ return m_Implementation->GetEEcorePct(); }
	virtual int GetGsPct() const		{ return m_Implementation->GetGsPct(); }
	virtual int GetGuiPct() const		{ return m_Implementation->GetGuiPct(); }
};

class DefaultCpuUsageProvider : public BaseCpuUsageProvider
{
protected:
	u64		m_lasttime_ee;
	u64		m_lasttime_gs;
	u64		m_lasttime_ui;

	u64		m_lasttime_update;

	u32		m_pct_ee;
	u32		m_pct_gs;
	u32		m_pct_ui;

public:
	DefaultCpuUsageProvider();
	virtual ~DefaultCpuUsageProvider() throw() {}

	bool IsImplemented() const;
	void UpdateStats();
	int GetEEcorePct() const;
	int GetGsPct() const;
	int GetGuiPct() const;
};
