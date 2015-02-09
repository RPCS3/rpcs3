#pragma once

struct psv_mutex_t
{
	char name[32];
	u32 attr;
	s32 count;

private:
	psv_mutex_t() = delete;
	psv_mutex_t(const psv_mutex_t&) = delete;
	psv_mutex_t(psv_mutex_t&&) = delete;

	psv_mutex_t& operator =(const psv_mutex_t&) = delete;
	psv_mutex_t& operator =(psv_mutex_t&&) = delete;

public:
	psv_mutex_t(const char* name, u32 attr, s32 count);
	void on_init(s32 id) {}
	void on_stop() {}

};

extern psv_object_list_t<psv_mutex_t, SCE_KERNEL_THREADMGR_UID_CLASS_MUTEX> g_psv_mutex_list;
