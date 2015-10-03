#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "GSManager.h"
#include "GSRender.h"


draw_context_t GSFrameBase::new_context()
{
	return std::shared_ptr<void>(make_context(), [this](void* ctxt) { delete_context(ctxt); });
}
