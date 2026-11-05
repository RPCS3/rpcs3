#pragma once

namespace rsx
{
	// Per-context host resources (texture, surface and vertex caches) of a render backend.
	class context_resources
	{
	public:
		virtual ~context_resources() = default;
	};
}
