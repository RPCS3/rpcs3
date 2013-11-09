#pragma once
#include "GLVertexProgram.h"
#include "GLFragmentProgram.h"

struct GLProgram
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

	GLProgram();

	int GetLocation(const wxString& name);
	bool IsCreated() const;
	void Create(const u32 vp, const u32 fp);
	void Use();
	void UnUse();
	void SetTex(u32 index);
	void Delete();
};