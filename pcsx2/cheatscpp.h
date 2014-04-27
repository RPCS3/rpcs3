/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#ifndef CHEATSCPP_H_INCLUDED
#define CHEATSCPP_H_INCLUDED

class Group
{
public:
	string title;
	bool enabled;
	int  parentIndex;

	Group(int nParent,bool nEnabled, string &nTitle);

};

class Patch
{
public:
	string title;
	int  group;
	bool enabled;
	int  patchIndex;

	Patch(int patch, int grp, bool en, string &ttl);

	Patch operator =(const Patch&p);
};

extern vector<Group> groups;
extern vector<Patch> patches;

#endif//CHEATSCPP_H_INCLUDED
