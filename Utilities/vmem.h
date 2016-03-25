#pragma once
#include "range.h"
#include "runtime_ptr.h"

namespace vmem
{
	enum access : u8
	{
		none = 0,
		read = 1 << 0,
		write = 1 << 1,
		execute = 1 << 2,
		read_write = read | write,
		read_write_execute = read | write | execute,
		all = read_write_execute,
	};

	DECLARE_ENUM_CLASS_BITWISE_OPERATORS(access);

	enum
	{
		object_type_texture,

		object_type_end
	};

	using memory_range = range<std::uintptr_t>;
	using set_memory_access_fun_t = void(memory_range, access);
	set_memory_access_fun_t* set_memory_access;

	class protected_region;
	class protected_object;

	struct protected_object_info : memory_range
	{
		virtual std::size_t hash() const = 0;
		virtual bool operator==(const runtime_ptr<protected_object_info>& rhs) const = 0;
	};

	struct protected_object_factory
	{
		virtual std::shared_ptr<protected_object> make_protected_object(const runtime_ptr<protected_object_info> &) = 0;
	};

	class protected_object
	{
		std::unique_ptr<protected_object_info> m_info;
		bool m_build_combined_views;

	protected:
		protected_region* m_region;

	public:
		protected_object(std::unique_ptr<protected_object_info> info_, bool build_combined_views)
			: m_info(std::move(info_))
			, m_build_combined_views(build_combined_views)
		{
		}

		protected_object_info& info() const
		{
			return *m_info;
		}

		bool build_combined_views() const
		{
			return m_build_combined_views;
		}

		virtual ~protected_object() = 0;
		virtual void on_init() = 0;
		virtual void on_remove() = 0;
		virtual void on_unprotect() = 0;
		virtual access requires_protection() const = 0;
	};

	class protected_region : public memory_range
	{
		using base = memory_range;

	protected:
		protected_region* m_parent = nullptr;

		std::list<protected_region> m_childs;
		std::list<std::shared_ptr<protected_object>> m_objects[object_type_end];
		std::list<std::vector<protected_region *>> m_combined_view;

		std::list<protected_region>::iterator remove_child(std::list<protected_region>::iterator child_iterator)
		{
			protected_region *child_region = std::addressof(*child_iterator);

			for (auto it = m_combined_view.begin(); it != m_combined_view.end();)
			{
				bool removed = false;

				for (auto region : *it)
				{
					if (region == child_region)
					{
						it = m_combined_view.erase(it);
						removed = true;
						break;
					}
				}

				if (!removed)
				{
					++it;
				}
			}

			return m_childs.erase(child_iterator);
		}

	public:
		protected_region(protected_region* parent, memory_range range)
			: m_parent(parent)
			, base(range)
		{
		}

		protected_region() = default;

		~protected_region()
		{
			for (auto &objects : m_objects)
			{
				for (auto object : objects)
				{
					object->on_remove();
				}
			}
		}

		access m_current_access;

		void protect()
		{
			access target_access = ~requires_protection() & access::all;

			if (target_access != m_current_access)
			{
				set_memory_access(*this, target_access);
				m_current_access = target_access;
			}
		}

		void unprotect()
		{
			if (m_current_access != access::all)
			{
				set_memory_access(*this, access::all);
				on_unprotect();
			}
		}

		access requires_protection() const
		{
			access result = access::none;

			for (auto &objects : m_objects)
			{
				for (auto object : objects)
				{
					result |= object->requires_protection();

					if (result == access::all)
					{
						return access::all;
					}
				}
			}

			for (auto& child : m_childs)
			{
				result |= child.requires_protection();

				if (result == access::all)
				{
					return access::all;
				}
			}

			return result;
		}

		void on_unprotect()
		{
			for (auto &objects : m_objects)
			{
				for (auto object : objects)
				{
					object->on_unprotect();
				}
			}

			for (auto &child : m_childs)
			{
				child.on_unprotect();
			}
		}

		void insert(std::size_t type_id, std::shared_ptr<protected_object>& object)
		{
			memory_range object_range = object->info();

			if (object_range == *this)
			{
				m_objects[type_id].push_back(object);
				return;
			}

			if (object_range.out_of(*this))
			{
				extend(object_range);

				if (m_parent)
				{
					for (auto it = m_parent->m_childs.begin(); it != m_parent->m_childs.end(); )
					{
						auto &parent_child = *it;

						if (std::addressof(parent_child) != this && overlaps(parent_child))
						{
							it = m_parent->remove_child(it);
						}
						else
						{
							++it;
						}
					}
				}
			}

			for (auto &child : m_childs)
			{
				if (child.overlaps(object_range))
				{
					child.insert(type_id, object);
					return;
				}
			}

			m_childs.emplace_back(this, object_range);
			protected_region& new_child = m_childs.back();
			new_child.m_objects[type_id].emplace_back(object);

			if (object->build_combined_views())
			{
				//TODO
			}
		}
	};

	class root_protected_region : public protected_region
	{
	public:
		void insert(std::size_t type_id, std::shared_ptr<protected_object>& object)
		{
			memory_range object_range = object->info();

			for (auto& region : m_childs)
			{
				if (object_range.overlaps(region))
				{
					region.insert(type_id, object);
					return;
				}
			}

			m_childs.emplace_back(this, object_range, true);
			m_childs.back().insert(type_id, object);
		}

		bool access_violation(std::uintptr_t address, access access_)
		{
			for (auto& region : m_childs)
			{
				if (region.contains(address))
				{
					region.unprotect();

					return true;
				}
			}

			return false;
		}
	};

	class protected_memory
	{
	public:
		using objects_storage_t = std::unordered_map<runtime_ptr<protected_object_info>, std::weak_ptr<protected_object>, value_internal_hasher, value_equal_to>;
		using objects_factory_t = std::unique_ptr<protected_object_factory>;

	private:
		objects_storage_t m_storage;
		objects_factory_t m_factories[object_type_end];
		root_protected_region m_root_region;

	private:
		std::shared_ptr<protected_object> object(std::size_t type_id, runtime_ptr<protected_object_info> info)
		{
			objects_factory_t &factory = m_factories[type_id];

			if (!factory)
			{
				throw EXCEPTION("factory not registered for %u type", type_id);
			}

			auto it = m_storage.find(info);

			if (it != m_storage.end())
			{
				if (std::shared_ptr<protected_object> result = it->second.lock())
				{
					return result;
				}
			}

			std::shared_ptr<protected_object> result = factory->make_protected_object(info);

			if (it != m_storage.end())
			{
				it->second = result;
			}
			else
			{
				m_storage.emplace(info, result);
			}

			m_root_region.insert(type_id, result);
			result->on_init();

			return result;
		}

	public:
		template<std::size_t TypeIndex>
		std::shared_ptr<protected_object> object(const protected_object_info& info)
		{
			static_assert(TypeIndex < max_objects_types_count, "");

			return object(m_factories[TypeIndex], info);
		}

		template<std::size_t TypeIndex>
		void register_factory(objects_factory_t factory)
		{
			static_assert(TypeIndex < max_objects_types_count, "");

			m_factories[TypeIndex] = std::move(factory);
		}

		bool access_violation(std::uintptr_t address, access access_)
		{
			return m_root_region.access_violation(address, access_);
		}
	};
}
