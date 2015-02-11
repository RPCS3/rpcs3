#pragma once

struct psv_cond_t
{
	char name[32];
	u32 attr;
	s32 mutexId;

private:
	psv_cond_t() = delete;
	psv_cond_t(const psv_cond_t&) = delete;
	psv_cond_t(psv_cond_t&&) = delete;

	psv_cond_t& operator =(const psv_cond_t&) = delete;
	psv_cond_t& operator =(psv_cond_t&&) = delete;

public:
	psv_cond_t(const char* name, u32 attr, s32 mutexId);
	void on_init(s32 id) {}
	void on_stop() {}

};

extern psv_object_list_t<psv_cond_t, SCE_KERNEL_THREADMGR_UID_CLASS_COND> g_psv_cond_list;
