#pragma once

struct psv_sema_t
{
	char name[32];
	u32 attr;
	s32 value;
	s32 max;

private:
	psv_sema_t() = delete;
	psv_sema_t(const psv_sema_t&) = delete;
	psv_sema_t(psv_sema_t&&) = delete;

	psv_sema_t& operator =(const psv_sema_t&) = delete;
	psv_sema_t& operator =(psv_sema_t&&) = delete;

public:
	psv_sema_t(const char* name, u32 attr, s32 init_value, s32 max_value);
	void on_init(s32 id) {}
	void on_stop() {}

};

extern psv_object_list_t<psv_sema_t, SCE_KERNEL_THREADMGR_UID_CLASS_SEMA> g_psv_sema_list;
