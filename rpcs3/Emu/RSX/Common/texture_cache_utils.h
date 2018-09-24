﻿#pragma once

#include "../rsx_cache.h"
#include "../rsx_utils.h"
#include "TextureUtils.h"

#include <list>
#include <atomic>


namespace rsx
{
	/**
	 * Helper enums/structs
	 */
	enum invalidation_chain_policy
	{
		invalidation_chain_none,         // No chaining: Only sections that overlap the faulting page get invalidated.
		invalidation_chain_full,         // Full chaining: Sections overlapping the faulting page get invalidated, as well as any sections overlapping invalidated sections.
		invalidation_chain_nearby        // Invalidations chain if they are near to the fault (<X pages away)
	};

	enum invalidation_chain_direction
	{
		chain_direction_both,
		chain_direction_forward,  // Only higher-base-address sections chain (unless they overlap the fault)
		chain_direction_backward, // Only lower-base-address pages chain (unless they overlap the fault)
	};

	enum texture_create_flags
	{
		default_component_order = 0,
		native_component_order = 1,
		swapped_native_component_order = 2,
	};

	enum memory_read_flags
	{
		flush_always = 0,
		flush_once = 1
	};

	struct invalidation_cause {
		enum enum_type {
			invalid = 0,
			read,
			deferred_read,
			write,
			deferred_write,
			unmap
		} cause;

		bool valid() const
		{
			return cause != invalid;
		}

		bool is_read() const
		{
			AUDIT(valid());
			return (cause == read || cause == deferred_read);
		}

		bool is_write() const
		{
			AUDIT(valid());
			return (cause == write || cause == deferred_write || cause == unmap);
		}

		bool is_deferred() const
		{
			AUDIT(valid());
			return (cause == deferred_read || cause == deferred_write);
		}

		bool allow_flush() const
		{
			return (cause == read || cause == write || cause == unmap);
		}

		bool exclude_fault_range() const
		{
			return (cause == unmap);
		}

		invalidation_cause undefer() const
		{
			AUDIT(is_deferred());
			if (is_read())
				return read;
			else if (is_write())
				return write;
			else
				fmt::throw_exception("Unreachable " HERE);
		}

		invalidation_cause() : cause(invalid) {}
		invalidation_cause(enum_type _cause) : cause(_cause) {}
		operator enum_type&() { return cause; }
		operator enum_type() const { return cause; }
	};

	struct typeless_xfer
	{
		bool src_is_typeless = false;
		bool dst_is_typeless = false;
		bool src_is_depth = false;
		bool dst_is_depth = false;
		u32 src_gcm_format = 0;
		u32 dst_gcm_format = 0;
		f32 src_scaling_hint = 1.f;
		f32 dst_scaling_hint = 1.f;
		texture_upload_context src_context = texture_upload_context::blit_engine_src;
		texture_upload_context dst_context = texture_upload_context::blit_engine_dst;

		void analyse()
		{
			if (src_is_typeless && dst_is_typeless)
			{
				if (src_scaling_hint == dst_scaling_hint &&
					src_scaling_hint != 1.f)
				{
					if (src_is_depth == dst_is_depth)
					{
						src_is_typeless = dst_is_typeless = false;
						src_scaling_hint = dst_scaling_hint = 1.f;
					}
				}
			}
		}
	};



	/**
	 * List structure used in Ranged Storage Blocks
	 * List of Arrays
	 * (avoids reallocation without the significant disadvantages of slow iteration through a list)
	 */
	template <typename section_storage_type, size_t array_size>
	class ranged_storage_block_list
	{
		static_assert(array_size > 0, "array_elements must be positive non-zero");

	public:
		using value_type = section_storage_type;
		using array_type = typename std::array<value_type, array_size>;
		using list_type = typename std::list<array_type>;
		using size_type = u32;

		// Iterator
		template <typename T, typename block_list, typename list_iterator>
		class iterator_tmpl
		{
		public:
			// Traits
			using value_type = T;
			using pointer = T * ;
			using difference_type = int;
			using reference = T & ;
			using iterator_category = std::forward_iterator_tag;

			// Constructors
			iterator_tmpl() = default;
			iterator_tmpl(block_list *_block) :
				block(_block),
				list_it(_block->m_data.begin()),
				idx(0)
			{
				if (_block->empty())
					idx = UINT32_MAX;
			}

		private:
			// Members
			block_list *block;
			list_iterator list_it = {};
			size_type idx = UINT32_MAX;
			size_type array_idx = 0;

			inline void next()
			{
				idx++;
				if (idx >= block->size())
				{
					idx = UINT32_MAX;
					return;
				}

				array_idx++;
				if (array_idx >= array_size)
				{
					array_idx = 0;
					list_it++;
				}
			}

		public:
			inline reference operator*() const { return (*list_it)[array_idx]; }
			inline pointer operator->() const { return &((*list_it)[array_idx]); }
			inline reference operator++() { next(); return **this; }
			inline reference operator++(int) { auto &res = **this;  next(); return res; }
			inline bool operator==(const iterator_tmpl &rhs) const { return idx == rhs.idx; }
			inline bool operator!=(const iterator_tmpl &rhs) const { return idx != rhs.idx; }
		};

		using iterator = iterator_tmpl<value_type, ranged_storage_block_list, typename list_type::iterator>;
		using const_iterator = iterator_tmpl<const value_type, const ranged_storage_block_list, typename list_type::const_iterator>;

		// Members
		size_type m_size = 0;
		list_type m_data;
		typename list_type::iterator m_data_it;
		size_type m_array_idx;
		size_type m_capacity;

		// Helpers
		inline void next_array()
		{
			if (m_data_it == m_data.end() || ++m_data_it == m_data.end())
			{
				m_data_it = m_data.emplace(m_data_it);
				m_capacity += array_size;
			}

			m_array_idx = 0;
		}

	public:
		// Constructor, Destructor
		ranged_storage_block_list() :
			m_data_it(m_data.end()),
			m_array_idx(UINT32_MAX)
		{}

		// Iterator
		inline iterator begin() noexcept { return { this }; }
		inline const_iterator begin() const noexcept { return { this }; }
		constexpr iterator end() noexcept { return {}; }
		constexpr const_iterator end() const noexcept { return {}; }

		// Operators
		inline value_type& front()
		{
			AUDIT(!empty());
			return m_data.front()[0];
		}

		inline value_type& back()
		{
			AUDIT(m_data_it != m_data.end() && m_array_idx < array_size);
			return (*m_data_it)[m_array_idx];
		}

		// Other operations on data
		inline size_type size() const { return m_size; }
		inline size_type capacity() const { return m_capacity; }
		inline bool empty() const { return m_size == 0; }

		inline void clear()
		{
			m_size = 0;
			m_array_idx = 0;
			m_data_it = m_data.begin();
		}

		inline void free()
		{
			m_size = 0;
			m_array_idx = 0;
			m_capacity = 0;
			m_data.resize(0);
			m_data_it = m_data.end();
		}

		inline void reserve(size_type new_size)
		{
			if (new_size <= m_capacity) return;
			size_type new_num_arrays = ((new_size - 1) / array_size) + 1;
			m_data.reserve(new_num_arrays);
			m_capacity = new_num_arrays * array_size;
		}

		template <typename ...Args>
		inline value_type& emplace_back(Args&&... args)
		{
			if (m_array_idx >= array_size)
			{
				next_array();
			}

			ASSERT(m_capacity > 0 && m_array_idx < array_size && m_data_it != m_data.end());

			value_type *dest = &((*m_data_it)[m_array_idx++]);
			new (dest) value_type(std::forward<Args>(args)...);
			m_size++;
			return *dest;
		}
	};



	/**
	 * Ranged storage
	 */
	template <typename section_storage_type>
	class texture_cache_base
	{
	public:
		virtual void on_memory_read_flags_changed(section_storage_type &section, rsx::memory_read_flags flags) = 0;
		virtual void on_section_destroyed(section_storage_type & /*section*/) {};
	};


	template <typename _ranged_storage_type>
	class ranged_storage_block
	{
	public:
		using ranged_storage_type = _ranged_storage_type;
		using section_storage_type = typename ranged_storage_type::section_storage_type;
		using texture_cache_type = typename ranged_storage_type::texture_cache_type;

		//using block_container_type = std::list<section_storage_type>;
		using block_container_type = ranged_storage_block_list<section_storage_type, 64>;
		using iterator = typename block_container_type::iterator;
		using const_iterator = typename block_container_type::const_iterator;

		using size_type = typename block_container_type::size_type;

		static constexpr u32 num_blocks = ranged_storage_type::num_blocks;
		static constexpr u32 block_size = ranged_storage_type::block_size;

		using unowned_container_type = std::unordered_set<section_storage_type*>;
		using unowned_iterator = typename unowned_container_type::iterator;
		using unowned_const_iterator = typename unowned_container_type::const_iterator;

	private:
		u32 index = 0;
		address_range range = {};
		block_container_type sections = {};
		unowned_container_type unowned; // pointers to sections from other blocks that overlap this block
		std::atomic<u32> exists_count = 0;
		std::atomic<u32> locked_count = 0;
		std::atomic<u32> unreleased_count = 0;
		ranged_storage_type *m_storage = nullptr;

		inline void add_owned_section_overlaps(section_storage_type &section)
		{
			u32 end = section.get_section_range().end;
			for (auto *block = next_block(); block != nullptr && end >= block->get_start(); block = block->next_block())
			{
				block->add_unowned_section(section);
			}
		}

		inline void remove_owned_section_overlaps(section_storage_type &section)
		{
			u32 end = section.get_section_range().end;
			for (auto *block = next_block(); block != nullptr && end >= block->get_start(); block = block->next_block())
			{
				block->remove_unowned_section(section);
			}
		}

	public:
		// Construction
		ranged_storage_block() = default;

		void initialize(u32 _index, ranged_storage_type *storage)
		{
			verify(HERE), m_storage == nullptr && storage != nullptr;
			AUDIT(index < num_blocks);

			m_storage = storage;
			index = _index;
			range = address_range::start_length(index * block_size, block_size);

			AUDIT(range.is_page_range() && get_start() / block_size == index);
		}

		/**
		 * Wrappers
		 */
		constexpr iterator begin() noexcept { return sections.begin(); }
		constexpr const_iterator begin() const noexcept { return sections.begin(); }
		inline iterator end() noexcept { return sections.end(); }
		inline const_iterator end() const noexcept { return sections.end(); }
		inline iterator at(size_type pos) { return sections.data(pos); }
		inline const_iterator at(size_type pos) const { return sections.data(pos); }
		inline bool empty() const { return sections.empty(); }
		inline size_type size() const { return sections.size(); }
		inline u32 get_exists_count() const { return exists_count; }
		inline u32 get_locked_count() const { return locked_count; }
		inline u32 get_unreleased_count() const { return unreleased_count; }

		/**
		 * Utilities
		 */
		ranged_storage_type& get_storage() const
		{
			AUDIT(m_storage != nullptr);
			return *m_storage;
		}

		texture_cache_type& get_texture_cache() const
		{
			return get_storage().get_texture_cache();
		}

		inline section_storage_type& create_section()
		{
			auto &res = sections.emplace_back(this);
			return res;
		}

		inline void clear()
		{
			for (auto &section : *this)
			{
				section.destroy();
			}

			AUDIT(exists_count == 0);
			AUDIT(unreleased_count == 0);
			AUDIT(locked_count == 0);
			sections.clear();
		}

		inline bool is_first_block() const
		{
			return index == 0;
		}

		inline bool is_last_block() const
		{
			return index == num_blocks - 1;
		}

		inline ranged_storage_block* prev_block() const
		{
			if (is_first_block()) return nullptr;
			return &get_storage()[index - 1];
		}

		inline ranged_storage_block* next_block() const
		{
			if (is_last_block()) return nullptr;
			return &get_storage()[index + 1];
		}

		// Address range
		inline const address_range& get_range() const { return range; }
		inline u32 get_start() const { return range.start; }
		inline u32 get_end() const { return range.end; }
		inline u32 get_index() const { return index; }
		inline bool overlaps(const section_storage_type& section, section_bounds bounds = full_range) const { return section.overlaps(range, bounds); }
		inline bool overlaps(const address_range& _range) const { return range.overlaps(_range); }

		/**
		 * Section callbacks
		 */
		inline void on_section_protected(const section_storage_type &section)
		{
			(void)section; // silence unused warning without _AUDIT
			AUDIT(section.is_locked());
			locked_count++;
		}

		inline void on_section_unprotected(const section_storage_type &section)
		{
			(void)section; // silence unused warning without _AUDIT
			AUDIT(!section.is_locked());
			u32 prev_locked = locked_count--;
			ASSERT(prev_locked > 0);
		}

		inline void on_section_range_valid(section_storage_type &section)
		{
			AUDIT(section.valid_range());
			AUDIT(range.overlaps(section.get_section_base()));
			add_owned_section_overlaps(section);
		}

		inline void on_section_range_invalid(section_storage_type &section)
		{
			AUDIT(section.valid_range());
			AUDIT(range.overlaps(section.get_section_base()));
			remove_owned_section_overlaps(section);
		}

		inline void on_section_resources_created(const section_storage_type &section)
		{
			(void)section; // silence unused warning without _AUDIT
			AUDIT(section.exists());

			u32 prev_exists = exists_count++;

			if (prev_exists == 0)
			{
				m_storage->on_ranged_block_first_section_created(*this);
			}
		}

		inline void on_section_resources_destroyed(const section_storage_type &section)
		{
			(void)section; // silence unused warning without _AUDIT
			AUDIT(!section.exists());

			u32 prev_exists = exists_count--;
			ASSERT(prev_exists > 0);

			if (prev_exists == 1)
			{
				m_storage->on_ranged_block_last_section_destroyed(*this);
			}
		}

		void on_section_released(const section_storage_type &/*section*/)
		{
			u32 prev_unreleased = unreleased_count--;
			ASSERT(prev_unreleased > 0);
		}

		void on_section_unreleased(const section_storage_type &/*section*/)
		{
			unreleased_count++;
		}


		/**
		 * Overlapping sections
		 */
		inline bool contains_unowned(section_storage_type &section) const
		{
			return (unowned.find(&section) != unowned.end());
		}

		inline void add_unowned_section(section_storage_type &section)
		{
			AUDIT(overlaps(section));
			AUDIT(section.get_section_base() < range.start);
			AUDIT(!contains_unowned(section));
			unowned.insert(&section);
		}

		inline void remove_unowned_section(section_storage_type &section)
		{
			AUDIT(overlaps(section));
			AUDIT(section.get_section_base() < range.start);
			AUDIT(contains_unowned(section));
			unowned.erase(&section);
		}

		inline unowned_iterator unowned_begin() { return unowned.begin(); }
		inline unowned_const_iterator unowned_begin() const { return unowned.begin(); }
		inline unowned_iterator unowned_end() { return unowned.end(); }
		inline unowned_const_iterator unowned_end() const { return unowned.end(); }
		inline bool unowned_empty() const { return unowned.empty(); }
	};


	template <typename _section_storage_type>
	class ranged_storage
	{
	public:
		static constexpr u32 block_size = 0x100'0000;
		static_assert(block_size % 4096u == 0, "block_size must be a multiple of the page size");
		static constexpr u32 num_blocks = u32{0x1'0000'0000ull / block_size};
		static_assert((num_blocks > 0) && (u64{num_blocks} *block_size == 0x1'0000'0000ull), "Invalid block_size/num_blocks");

		using section_storage_type = _section_storage_type;
		using texture_cache_type = texture_cache_base<section_storage_type>;
		using block_type = ranged_storage_block<ranged_storage>;

	private:
		block_type blocks[num_blocks];
		texture_cache_type *m_tex_cache;
		std::unordered_set<block_type*> m_in_use;
		bool m_purging = false;

	public:
		std::atomic<u32> m_unreleased_texture_objects = { 0 }; //Number of invalidated objects not yet freed from memory
		std::atomic<u32> m_texture_memory_in_use = { 0 };

		// Constructor
		ranged_storage(texture_cache_type *tex_cache) :
			m_tex_cache(tex_cache)
		{
			// Initialize blocks
			for (u32 i = 0; i < num_blocks; i++)
			{
				blocks[i].initialize(i, this);
			}
		}

		/**
		 * Iterators
		 */

		constexpr auto begin() { return std::begin(blocks); }
		constexpr auto begin() const { return std::begin(blocks); }
		constexpr auto end() { return std::end(blocks); }
		constexpr auto end() const { return std::end(blocks); }

		/**
		 * Utilities
		 */
		inline block_type& block_for(u32 address)
		{
			return blocks[address / block_size];
		}

		inline const block_type& block_for(u32 address) const
		{
			return blocks[address / block_size];
		}

		inline block_type& block_for(const address_range &range)
		{
			AUDIT(range.valid());
			return block_for(range.start);
		}

		inline block_type& block_for(const section_storage_type &section)
		{
			return block_for(section.get_section_base());
		}

		inline block_type& operator[](size_t pos)
		{
			AUDIT(pos < num_blocks);
			return blocks[pos];
		}

		inline texture_cache_type& get_texture_cache() const
		{
			AUDIT(m_tex_cache != nullptr);
			return *m_tex_cache;
		}


		/**
		 * Blocks
		 */

		void clear()
		{
			for (auto &block : *this)
			{
				block.clear();
			}

			m_in_use.clear();

			AUDIT(m_unreleased_texture_objects == 0);
			AUDIT(m_texture_memory_in_use == 0);
		}

		void purge_unreleased_sections()
		{
			// We will be iterating through m_in_use
			// do not allow the callbacks to touch m_in_use to avoid invalidating the iterator
			m_purging = true;

			//Reclaims all graphics memory consumed by dirty textures
			for (auto it = m_in_use.begin(); it != m_in_use.end();)
			{
				auto *block = *it;

				if (block->get_unreleased_count() > 0)
				{
					for (auto &tex : *block)
					{
						if (!tex.is_unreleased())
							continue;

						ASSERT(!tex.is_locked());

						tex.destroy();
					}
				}

				if (block->get_exists_count() == 0)
				{
					it = m_in_use.erase(it);
				}
				else
				{
					it++;
				}
			}

			m_purging = false;
			AUDIT(m_unreleased_texture_objects == 0);
		}


		/**
		 * Callbacks
		 */
		void on_section_released(const section_storage_type &/*section*/)
		{
			u32 prev_unreleased = m_unreleased_texture_objects--;
			ASSERT(prev_unreleased > 0);
		}

		void on_section_unreleased(const section_storage_type &/*section*/)
		{
			m_unreleased_texture_objects++;
		}

		void on_section_resources_created(const section_storage_type &section)
		{
			m_texture_memory_in_use += section.get_section_size();
		}

		void on_section_resources_destroyed(const section_storage_type &section)
		{
			u32 size = section.get_section_size();
			u32 prev_size = m_texture_memory_in_use.fetch_sub(size);
			ASSERT(prev_size >= size);
		}

		void on_ranged_block_first_section_created(block_type& block)
		{
			AUDIT(!m_purging);
			AUDIT(m_in_use.find(&block) == m_in_use.end());
			m_in_use.insert(&block);
		}

		void on_ranged_block_last_section_destroyed(block_type& block)
		{
			if (m_purging)
				return;

			AUDIT(m_in_use.find(&block) != m_in_use.end());
			m_in_use.erase(&block);
		}

		/**
		 * Ranged Iterator
		 */
		 // Iterator
		template <typename T, typename unowned_iterator, typename section_iterator, typename block_type, typename parent_type>
		class range_iterator_tmpl
		{
		public:
			// Traits
			using value_type = T;
			using pointer = T * ;
			using difference_type = int;
			using reference = T & ;
			using iterator_category = std::forward_iterator_tag;

			// Constructors
			range_iterator_tmpl() = default; // end iterator
			explicit range_iterator_tmpl(parent_type &storage, const address_range &_range, section_bounds _bounds, bool _locked_only) :
				range(_range),
				bounds(_bounds),
				block(&storage.block_for(range.start)),
				unowned_it(block->unowned_begin()),
				unowned_remaining(true),
				cur_block_it(block->begin()),
				locked_only(_locked_only)
			{
				// do a "fake" iteration to ensure the internal state is consistent
				next(false);
			}

		private:
			// Members
			address_range range;
			section_bounds bounds;

			block_type *block = nullptr;
			bool needs_overlap_check = true;
			bool unowned_remaining = false;
			unowned_iterator unowned_it = {};
			section_iterator cur_block_it = {};
			pointer obj = nullptr;
			bool locked_only = false;

			inline void next(bool iterate = true)
			{
				AUDIT(block != nullptr);

				if (unowned_remaining)
				{
					do
					{
						// Still have "unowned" sections from blocks before the range to loop through
						auto blk_end = block->unowned_end();
						if (iterate && unowned_it != blk_end)
						{
							unowned_it++;
						}

						if (unowned_it != blk_end)
						{
							obj = *unowned_it;
							if (obj->valid_range() && (!locked_only || obj->is_locked()) && obj->overlaps(range, bounds))
								return;

							iterate = true;
							continue;
						}

						// No more unowned sections remaining
						unowned_remaining = false;
						iterate = false;
						break;

					} while (true);
				}

				// Go to next block
				do
				{
					// Iterate current block
					do
					{
						auto blk_end = block->end();
						if (iterate && cur_block_it != blk_end)
						{
							cur_block_it++;
						}

						if (cur_block_it != blk_end)
						{
							obj = &(*cur_block_it);
							if (obj->valid_range() && (!locked_only || obj->is_locked()) && (!needs_overlap_check || obj->overlaps(range, bounds)))
								return;

							iterate = true;
							continue;
						}
						break;

					} while (true);

					// Move to next block(s)
					do
					{
						block = block->next_block();
						if (block == nullptr || block->get_start() > range.end) // Reached end
						{
							block = nullptr;
							obj = nullptr;
							return;
						}

						needs_overlap_check = (block->get_end() > range.end);
						cur_block_it = block->begin();
						iterate = false;
					} while (locked_only && block->get_locked_count() == 0); // find a block with locked sections

				} while (true);
			}

		public:
			inline reference operator*() const { return *obj; }
			inline pointer operator->() const { return obj; }
			inline reference operator++() { next(); return *obj; }
			inline reference operator++(int) { auto *ptr = obj; next(); return *ptr; }
			inline bool operator==(const range_iterator_tmpl &rhs) const { return obj == rhs.obj && unowned_remaining == rhs.unowned_remaining; }
			inline bool operator!=(const range_iterator_tmpl &rhs) const { return !operator==(rhs); }

			inline void set_end(u32 new_end)
			{
				range.end = new_end;

				// If we've exceeded the new end, invalidate iterator
				if (block->get_start() > range.end)
				{
					block = nullptr;
				}
			}

			inline block_type& get_block() const
			{
				AUDIT(block != nullptr);
				return *block;
			}

			inline section_bounds get_bounds() const
			{
				return bounds;
			}
		};

		using range_iterator = range_iterator_tmpl<section_storage_type, typename block_type::unowned_iterator, typename block_type::iterator, block_type, ranged_storage>;
		using range_const_iterator = range_iterator_tmpl<const section_storage_type, typename block_type::unowned_const_iterator, typename block_type::const_iterator, const block_type, const ranged_storage>;

		inline range_iterator range_begin(const address_range &range, section_bounds bounds, bool locked_only = false) {
			return range_iterator(*this, range, bounds, locked_only);
		}

		inline range_const_iterator range_begin(const address_range &range, section_bounds bounds, bool locked_only = false) const {
			return range_const_iterator(*this, range, bounds, locked_only);
		}

		inline range_const_iterator range_begin(u32 address, section_bounds bounds, bool locked_only = false) const {
			return range_const_iterator(*this, address_range::start_length(address, 1), bounds, locked_only);
		}

		constexpr range_iterator range_end()
		{
			return range_iterator();
		}

		constexpr range_const_iterator range_end() const
		{
			return range_const_iterator();
		}

		/**
		 * Debug
		 */
#ifdef TEXTURE_CACHE_DEBUG
		void verify_protection(bool recount = false)
		{
			if (recount)
			{
				// Reset calculated part of the page_info struct
				tex_cache_checker.reset_refcount();

				// Go through all blocks and update calculated values
				for (auto &block : *this)
				{
					for (auto &tex : block)
					{
						if (tex.is_locked())
						{
							tex_cache_checker.add(tex.get_locked_range(), tex.get_protection());
						}
					}
				}
			}

			// Verify
			tex_cache_checker.verify();
		}
#endif //TEXTURE_CACHE_DEBUG

	};



	/**
	 * Cached Texture Section
	 */
	template <typename derived_type>
	class cached_texture_section : public rsx::buffered_section
	{
	public:
		using ranged_storage_type = ranged_storage<derived_type>;
		using ranged_storage_block_type = ranged_storage_block<ranged_storage_type>;
		using texture_cache_type = typename ranged_storage_type::texture_cache_type;

	protected:
		ranged_storage_type *m_storage = nullptr;
		ranged_storage_block_type *m_block = nullptr;
		texture_cache_type *m_tex_cache = nullptr;

	private:
		constexpr derived_type* derived()
		{
			return static_cast<derived_type*>(this);
		}

		constexpr const derived_type* derived() const
		{
			return static_cast<const derived_type*>(this);
		}

		bool dirty = true;
		bool triggered_exists_callbacks = false;
		bool triggered_unreleased_callbacks = false;

	protected:

		u16 width;
		u16 height;
		u16 depth;
		u16 mipmaps;

		u16 real_pitch;
		u16 rsx_pitch;

		u32 gcm_format = 0;
		bool pack_unpack_swap_bytes = false;

		u64 sync_timestamp = 0;
		bool synchronized = false;
		bool flushed = false;

		u32 num_writes = 0;
		std::deque<u32> read_history;

		rsx::memory_read_flags readback_behaviour = rsx::memory_read_flags::flush_once;
		rsx::texture_create_flags view_flags = rsx::texture_create_flags::default_component_order;
		rsx::texture_upload_context context = rsx::texture_upload_context::shader_read;
		rsx::texture_dimension_extended image_type = rsx::texture_dimension_extended::texture_dimension_2d;

	public:
		u64 cache_tag = 0;
		u64 last_write_tag = 0;

		~cached_texture_section()
		{
			AUDIT(!exists());
		}

		cached_texture_section() = default;
		cached_texture_section(ranged_storage_block_type *block) : m_block(block), m_storage(&block->get_storage()), m_tex_cache(&block->get_texture_cache())
		{
			update_unreleased();
		}

		void initialize(ranged_storage_block_type *block)
		{
			verify(HERE), m_block == nullptr && m_tex_cache == nullptr && m_storage == nullptr;
			m_block = block;
			m_storage = &block->get_storage();
			m_tex_cache = &block->get_texture_cache();

			update_unreleased();
		}


		/**
		 * Reset
		 */
		void reset(const address_range &memory_range)
		{
			AUDIT(memory_range.valid());
			AUDIT(!is_locked());

			// Invalidate if necessary
			invalidate_range();

			// Superclass
			rsx::buffered_section::reset(memory_range);

			// Reset member variables to the default
			width = 0;
			height = 0;
			depth = 0;
			mipmaps = 0;

			real_pitch = 0;
			rsx_pitch = 0;

			gcm_format = 0;
			pack_unpack_swap_bytes = false;

			sync_timestamp = 0ull;
			synchronized = false;
			flushed = false;

			cache_tag = 0ull;
			last_write_tag = 0ull;

			num_writes = 0;
			read_history.clear();

			readback_behaviour = rsx::memory_read_flags::flush_once;
			view_flags = rsx::texture_create_flags::default_component_order;
			context = rsx::texture_upload_context::shader_read;
			image_type = rsx::texture_dimension_extended::texture_dimension_2d;

			// Set to dirty
			set_dirty(true);

			// Notify that our CPU range is now valid
			notify_range_valid();
		}



		/**
		 * Destroyed Flag
		 */
		inline bool is_destroyed() const { return !exists(); } // this section is currently destroyed

		inline bool can_destroy() const {
			return !is_destroyed() && is_tracked();
		} // This section may be destroyed

	protected:
		void on_section_resources_created()
		{
			AUDIT(exists());
			AUDIT(valid_range());

			if (triggered_exists_callbacks) return;
			triggered_exists_callbacks = true;

			// Callbacks
			m_block->on_section_resources_created(*derived());
			m_storage->on_section_resources_created(*derived());
		}

		void on_section_resources_destroyed()
		{
			if (!triggered_exists_callbacks) return;
			triggered_exists_callbacks = false;

			AUDIT(valid_range());

			// Set dirty
			set_dirty(true);

			// Unlock
			if (is_locked())
			{
				unprotect();
			}

			// Trigger callbacks
			m_block->on_section_resources_destroyed(*derived());
			m_storage->on_section_resources_destroyed(*derived());

			// Invalidate range
			invalidate_range();
		}

	public:
		/**
		 * Dirty/Unreleased Flag
		 */
		inline bool is_dirty() const { return dirty; } // this section is dirty and will need to be reuploaded

		void set_dirty(bool new_dirty)
		{
			if (new_dirty == false && !is_locked() && context == texture_upload_context::shader_read)
				return;

			dirty = new_dirty;

			AUDIT(dirty || (!dirty && exists()));

			update_unreleased();
		}

	private:
		void update_unreleased()
		{
			bool unreleased = is_unreleased();

			if (unreleased && !triggered_unreleased_callbacks)
			{
				triggered_unreleased_callbacks = true;
				m_block->on_section_unreleased(*derived());
				m_storage->on_section_unreleased(*derived());
			}
			else if (!unreleased && triggered_unreleased_callbacks)
			{
				triggered_unreleased_callbacks = false;
				m_block->on_section_released(*derived());
				m_storage->on_section_released(*derived());
			}
		}


		/**
		 * Valid Range
		 */

		void notify_range_valid()
		{
			AUDIT(valid_range());

			// Callbacks
			m_block->on_section_range_valid(*derived());
			//m_storage->on_section_range_valid(*derived());

			// Reset texture_cache m_flush_always_cache
			if (readback_behaviour == memory_read_flags::flush_always)
			{
				m_tex_cache->on_memory_read_flags_changed(*derived(), memory_read_flags::flush_always);
			}
		}

		void invalidate_range()
		{
			if (!valid_range())
				return;

			// Reset texture_cache m_flush_always_cache
			if (readback_behaviour == memory_read_flags::flush_always)
			{
				m_tex_cache->on_memory_read_flags_changed(*derived(), memory_read_flags::flush_once);
			}

			// Notify the storage block that we are now invalid
			m_block->on_section_range_invalid(*derived());
			//m_storage->on_section_range_invalid(*derived());

			buffered_section::invalidate_range();
		}

	public:
		/**
		 * Misc.
		 */
		bool is_tracked() const
		{
			return !exists() || (get_context() != framebuffer_storage);
		}

		bool is_unreleased() const
		{
			return is_tracked() && exists() && is_dirty() && !is_locked();
		}

		bool can_be_reused() const
		{
			return !exists() || (is_dirty() && !is_locked());
		}

		bool is_flushable() const
		{
			//This section is active and can be flushed to cpu
			return (get_protection() == utils::protection::no);
		}


	private:
		/**
		 * Protection
		 */
		void post_protect(utils::protection old_prot, utils::protection prot)
		{
			if (old_prot != utils::protection::rw && prot == utils::protection::rw)
			{
				AUDIT(!is_locked());

				m_block->on_section_unprotected(*derived());

				// Blit and framebuffers may be unprotected and clean
				if (context == texture_upload_context::shader_read)
				{
					set_dirty(true);
				}
			}
			else if (old_prot == utils::protection::rw && prot != utils::protection::rw)
			{
				AUDIT(is_locked());

				m_block->on_section_protected(*derived());

				set_dirty(false);
			}
		}

	public:
		inline void protect(utils::protection prot)
		{
			utils::protection old_prot = get_protection();
			rsx::buffered_section::protect(prot);
			post_protect(old_prot, prot);
		}

		inline void protect(utils::protection prot, const std::pair<u32, u32>& range_confirm)
		{
			utils::protection old_prot = get_protection();
			rsx::buffered_section::protect(prot, range_confirm);
			post_protect(old_prot, prot);
		}

		inline void unprotect()
		{
			utils::protection old_prot = get_protection();
			rsx::buffered_section::unprotect();
			post_protect(old_prot, utils::protection::rw);
		}

		inline void discard(bool set_dirty = true)
		{
			utils::protection old_prot = get_protection();
			rsx::buffered_section::discard();
			post_protect(old_prot, utils::protection::rw);

			if (set_dirty)
			{
				this->set_dirty(true);
			}
		}

		void reprotect(const utils::protection prot)
		{
			//Reset properties and protect again
			flushed = false;
			synchronized = false;
			sync_timestamp = 0ull;

			protect(prot);
		}

		void reprotect(const utils::protection prot, const std::pair<u32, u32>& range)
		{
			//Reset properties and protect again
			flushed = false;
			synchronized = false;
			sync_timestamp = 0ull;

			protect(prot, range);
		}


		/**
		 * Misc
		 */
		void touch(u64 tag)
		{
			num_writes++;
			last_write_tag = tag;
		}

		void reset_write_statistics()
		{
			if (read_history.size() == 16)
			{
				read_history.pop_back();
			}

			read_history.push_front(num_writes);
			num_writes = 0;
		}

		void set_view_flags(rsx::texture_create_flags flags)
		{
			view_flags = flags;
		}

		void set_context(rsx::texture_upload_context upload_context)
		{
			AUDIT(!exists() || !is_locked() || context == upload_context);
			context = upload_context;
		}

		void set_image_type(rsx::texture_dimension_extended type)
		{
			image_type = type;
		}

		void set_gcm_format(u32 format)
		{
			gcm_format = format;
		}

		void set_memory_read_flags(memory_read_flags flags, bool notify_texture_cache = true)
		{
			const bool changed = (flags != readback_behaviour);
			readback_behaviour = flags;

			if (notify_texture_cache && changed && valid_range())
			{
				m_tex_cache->on_memory_read_flags_changed(*derived(), flags);
			}
		}

		u16 get_width() const
		{
			return width;
		}

		u16 get_height() const
		{
			return height;
		}

		u16 get_depth() const
		{
			return depth;
		}

		u16 get_mipmaps() const
		{
			return mipmaps;
		}

		u16 get_rsx_pitch() const
		{
			return rsx_pitch;
		}

		rsx::texture_create_flags get_view_flags() const
		{
			return view_flags;
		}

		rsx::texture_upload_context get_context() const
		{
			return context;
		}

		rsx::section_bounds get_overlap_test_bounds() const
		{
			if (guard_policy == protection_policy::protect_policy_full_range)
				return rsx::section_bounds::locked_range;

			const bool strict_range_check = g_cfg.video.write_color_buffers || g_cfg.video.write_depth_buffer;
			return (strict_range_check || get_context() == rsx::texture_upload_context::blit_engine_dst) ?
				rsx::section_bounds::confirmed_range :
				rsx::section_bounds::locked_range;
		}

		rsx::texture_dimension_extended get_image_type() const
		{
			return image_type;
		}

		u32 get_gcm_format() const
		{
			return gcm_format;
		}

		memory_read_flags get_memory_read_flags() const
		{
			return readback_behaviour;
		}

		bool writes_likely_completed() const
		{
			// TODO: Move this to the miss statistics block
			const auto num_records = read_history.size();

			if (num_records == 0)
			{
				return false;
			}
			else if (num_records == 1)
			{
				return num_writes >= read_history.front();
			}
			else
			{
				const u32 last = read_history.front();
				const u32 prev_last = read_history[1];

				if (last == prev_last && num_records <= 3)
				{
					return num_writes >= last;
				}

				u32 compare = UINT32_MAX;
				for (u32 n = 1; n < num_records; n++)
				{
					if (read_history[n] == last)
					{
						// Uncertain, but possible
						compare = read_history[n - 1];

						if (num_records > (n + 1))
						{
							if (read_history[n + 1] == prev_last)
							{
								// Confirmed with 2 values
								break;
							}
						}
					}
				}

				return num_writes >= compare;
			}
		}

		u64 get_sync_timestamp() const
		{
			return sync_timestamp;
		}

		/**
		 * Comparison
		 */
		inline bool matches(const address_range &memory_range)
		{
			return valid_range() && rsx::buffered_section::matches(memory_range);
		}

		bool matches_dimensions(u32 width, u32 height, u32 depth, u32 mipmaps)
		{
			if (!valid_range())
				return false;

			if (!width && !height && !depth && !mipmaps)
				return true;

			if (width && width != this->width)
				return false;

			if (height && height != this->height)
				return false;

			if (depth && depth != this->depth)
				return false;

			if (mipmaps && mipmaps > this->mipmaps)
				return false;

			return true;
		}

		bool matches(u32 rsx_address, u32 width, u32 height, u32 depth, u32 mipmaps)
		{
			if (!valid_range())
				return false;

			if (rsx_address != get_section_base())
				return false;

			return matches_dimensions(width, height, depth, mipmaps);
		}

		bool matches(const address_range& memory_range, u32 width, u32 height, u32 depth, u32 mipmaps)
		{
			if (!valid_range())
				return false;

			if (!rsx::buffered_section::matches(memory_range))
				return false;

			return matches_dimensions(width, height, depth, mipmaps);
		}


		/**
		 * Derived wrappers
		 */
		inline void destroy()
		{
			derived()->destroy();
		}

		inline bool exists() const
		{
			return derived()->exists();
		}
	};

} // namespace rsx