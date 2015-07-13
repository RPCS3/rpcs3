#pragma once

typedef void SceSslCert;
typedef void SceSslCertName;

struct SceSslMemoryPoolStats
{
	u32 poolSize;
	u32 maxInuseSize;
	u32 currentInuseSize;
	s32 reserved;
};

extern psv_log_base sceSsl;
