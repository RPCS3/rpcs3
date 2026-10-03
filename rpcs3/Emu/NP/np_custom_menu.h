#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/Modules/sceNp.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace np
{
	struct custom_menu_action
	{
		s32 id = 0;
		u32 mask = SCE_NP_CUSTOM_MENU_ACTION_MASK_ME;
		std::string name;
		u64 generation = 0;

		bool operator==(const custom_menu_action&) const = default;
	};

	// Protected by np_handler::mutex_custom_menu. Does not execute guest code.
	struct custom_menu_state
	{
		u64 generation = 0;
		bool registered = false;
		vm::ptr<SceNpCustomMenuEventHandler> handler{};
		vm::ptr<void> user_arg{};
		std::vector<custom_menu_action> actions;
		SceNpCustomMenuIndexArray activation{};
		std::vector<SceNpCustomMenuActionExceptions> exception_list;

		void reset()
		{
			generation++;
			registered = false;
			handler = {};
			user_arg = {};
			actions.clear();
			activation = {};
			exception_list.clear();
		}

		void register_actions(std::vector<custom_menu_action> new_actions, vm::ptr<SceNpCustomMenuEventHandler> callback, vm::ptr<void> arg)
		{
			reset();
			for (auto& action : new_actions)
			{
				action.generation = generation;
			}
			actions = std::move(new_actions);
			handler = callback;
			user_arg = arg;
			registered = true;
		}

		bool is_local_action_available(const custom_menu_action& action) const
		{
			const u32 index = static_cast<u32>(action.id);
			return registered && handler && action.generation == generation && index < actions.size() &&
				index < SCE_NP_CUSTOM_MENU_INDEX_SETSIZE && actions[index].id == action.id &&
				(actions[index].mask & SCE_NP_CUSTOM_MENU_ACTION_MASK_ME) && SCE_NP_CUSTOM_MENU_INDEX_ISSET(index, &activation);
		}

		std::vector<custom_menu_action> get_local_actions() const
		{
			std::vector<custom_menu_action> result;
			for (const auto& action : actions)
			{
				if (is_local_action_available(action))
				{
					result.push_back(action);
				}
			}
			return result;
		}

		struct selection
		{
			u32 index;
			vm::ptr<SceNpCustomMenuEventHandler> handler;
			vm::ptr<void> user_arg;
		};

		std::optional<selection> resolve_local_action(const custom_menu_action& action) const
		{
			if (!is_local_action_available(action))
			{
				return std::nullopt;
			}
			return selection{static_cast<u32>(action.id), handler, user_arg};
		}
	};
}
