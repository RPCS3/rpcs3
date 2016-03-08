#pragma once

using SceSulphaCallback = void(vm::ptr<void> arg);

struct SceSulphaConfig
{
	vm::lptr<SceSulphaCallback> notifyCallback;
	le_t<u32> port;
	le_t<u32> bookmarkCount;
};

struct SceSulphaAgentsRegister;

using SceSulphaHandle = void;

extern psv_log_base sceSulpha;
