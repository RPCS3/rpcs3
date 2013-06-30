#pragma once
#include "VertexProgram.h"
#include "FragmentProgram.h"

struct Program
{
private:
	struct Location
	{
		int loc;
		wxString name;
	};

	Array<Location> m_locations;

public:
	u32 id;

	Program();

	int GetLocation(const wxString& name);
	bool IsCreated() const;
	void Create(const u32 vp, const u32 fp);
	void Use();
	void SetTex(u32 index);
	void Delete();
};