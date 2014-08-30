#pragma once

struct rPlatform
{
	static std::string getConfigDir();
};

/**********************************************************************
*********** RSX Debugger
************************************************************************/

struct RSXDebuggerProgram
{
	u32 id;
	u32 vp_id;
	u32 fp_id;
	std::string vp_shader;
	std::string fp_shader;
	bool modified;

	RSXDebuggerProgram()
		: modified(false)
	{
	}
};

extern std::vector<RSXDebuggerProgram> m_debug_programs;

/**********************************************************************
*********** Image stuff
************************************************************************/
enum rImageType
{
	rBITMAP_TYPE_PNG
};
struct rImage
{
	rImage();
	rImage(const rImage &) = delete;
	~rImage();
	void Create(int width , int height, void *data, void *alpha);
	void SaveFile(const std::string& name, rImageType type);

	void *handle;
};
