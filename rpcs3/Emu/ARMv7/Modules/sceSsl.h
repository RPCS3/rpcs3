#pragma once

using SceSslCert = void;
using SceSslCertName = void;

struct SceSslMemoryPoolStats
{
	le_t<u32> poolSize;
	le_t<u32> maxInuseSize;
	le_t<u32> currentInuseSize;
	le_t<s32> reserved;
};

extern psv_log_base sceSsl;
