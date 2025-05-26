#pragma once

#include "Emu/System.h"
#include "texture_cache_types.h"
#include "texture_cache_predictor.h"
#include "TextureUtils.h"

#include "Emu/Memory/vm.h"
#include "Emu/RSX/Host/MM.h"
#include "util/vm.hpp"

#include <list>
#include <unordered_set>

namespace rsx
{
	enum section_bounds
	{
		full_range,
		locked_range,
		confirmed_range
	};

	enum section_protection_strategy
	{
		lock,
		hash
	};

	static inline void memory_protect(const address_range32& range, utils::protection prot)
	{
		ensure(range.is_page_range());

		rsx::mm_protect(vm::base(range.start), range.length(), prot);

#ifdef TEXTURE_CACHE_DEBUG
		tex_cache_checker.set_protection(range, prot);
#endif
	}

	/**
	 * List structure used in Ranged Storage Blocks
	 * List of Arrays
	 * (avoids reallocation without the significant disadvantages of slow iteration through a list)
	 */
	template <typename section_storage_type, usz array_size>
	class ranged_storage_block_list
	{
		static_assert(array_size > 0, "array_elements must be positive non-zero");

	public:
		using value_type = section_storage_type;
		using array_type = std::array<value_type, array_size>;
		using list_type = std::list<array_type>;
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
					idx = u32{umax};
			}

		private:
			// Members
			block_list *block;
			list_iterator list_it = {};
			size_type idx = u32{umax};
			size_type array_idx = 0;

			inline void next()
			{
				++idx;
				if (idx >= block->size())
				{
					idx = u32{umax};
					return;
				}

				++array_idx;
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
			m_array_idx(-1),
			m_capacity(0)
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
			m_data.clear();
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

			ensure(m_capacity > 0 && m_array_idx < array_size && m_data_it != m_data.end());

			value_type *dest = &((*m_data_it)[m_array_idx++]);
			new (dest) value_type(std::forward<Args>(args)...);
			++m_size;
			return *dest;
		}
	};


	/**
	 * Ranged storage
	 */
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
		address_range32 range = {};
		block_container_type sections = {};
		unowned_container_type unowned; // pointers to sections from other blocks that overlap this block
		atomic_t<u32> exists_count = 0;
		atomic_t<u32> locked_count = 0;
		atomic_t<u32> unreleased_count = 0;
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
			ensure(m_storage == nullptr && storage != nullptr);
			AUDIT(index < num_blocks);

			m_storage = storage;
			index = _index;
			range = address_range32::start_length(index * block_size, block_size);

			AUDIT(range.is_page_range() && get_start() / block_size == index);
		}

		/**
		 * Wrappers
		 */
		constexpr iterator begin() noexcept { return sections.begin(); }
		constexpr const_iterator begin() const noexcept { return sections.begin(); }
		inline iterator end() noexcept { return sections.end(); }
		inline const_iterator end() const noexcept { return sections.end(); }
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
				if (section.is_locked())
					section.unprotect();

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
		inline const address_range32& get_range() const { return range; }
		inline u32 get_start() const { return range.start; }
		inline u32 get_end() const { return range.end; }
		inline u32 get_index() const { return index; }
		inline bool overlaps(const section_storage_type& section, section_bounds bounds = full_range) const { return section.overlaps(range, bounds); }
		inline bool overlaps(const address_range32& _range) const { return range.overlaps(_range); }

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
			ensure(prev_locked > 0);
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
			ensure(prev_exists > 0);

			if (prev_exists == 1)
			{
				m_storage->on_ranged_block_last_section_destroyed(*this);
			}
		}

		void on_section_released(const section_storage_type &/*section*/)
		{
			u32 prev_unreleased = unreleased_count--;
			ensure(prev_unreleased > 0);
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


	template <typename traits>
	class ranged_storage
	{
	public:
		static constexpr u32 block_size = 0x100000;
		static_assert(block_size % 4096u == 0, "block_size must be a multiple of the page size");
		static constexpr u32 num_blocks = u32{0x100000000ull / block_size};
		static_assert((num_blocks > 0) && (u64{num_blocks} *block_size == 0x100000000ull), "Invalid block_size/num_blocks");

		using section_storage_type = typename traits::section_storage_type;
		using texture_cache_type   = typename traits::texture_cache_base_type;
		using block_type           = ranged_storage_block<ranged_storage>;

	private:
		block_type blocks[num_blocks];
		texture_cache_type *m_tex_cache;
		std::unordered_set<block_type*> m_in_use;

	public:
		atomic_t<u32> m_unreleased_texture_objects = { 0 }; //Number of invalidated objects not yet freed from memory
		atomic_t<u64> m_texture_memory_in_use = { 0 };

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

		inline block_type& block_for(const address_range32 &range)
		{
			AUDIT(range.valid());
			return block_for(range.start);
		}

		inline block_type& block_for(const section_storage_type &section)
		{
			return block_for(section.get_section_base());
		}

		inline block_type& operator[](usz pos)
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
			std::vector<section_storage_type*> textures_to_remove;

			// Reclaims all graphics memory consumed by dirty textures
			// Do not destroy anything while iterating or you will end up with stale iterators
			for (auto& block : m_in_use)
			{
				if (block->get_unreleased_count() > 0)
				{
					for (auto& tex : *block)
					{
						if (!tex.is_unreleased())
							continue;

						ensure(!tex.is_locked());
						textures_to_remove.push_back(&tex);
					}
				}
			}

			for (auto& tex : textures_to_remove)
			{
				tex->destroy();
			}

			AUDIT(m_unreleased_texture_objects == 0);
		}

		bool purge_unlocked_sections()
		{
			// Reclaims all graphics memory consumed by unlocked textures
			// Do not destroy anything while iterating or you will end up with stale iterators
			std::vector<section_storage_type*> textures_to_remove;

			for (auto& block : m_in_use)
			{
				if (block->get_exists_count() > block->get_locked_count())
				{
					for (auto& tex : *block)
					{
						if (tex.get_context() == rsx::texture_upload_context::framebuffer_storage ||
							tex.is_locked() ||
							!tex.exists())
						{
							continue;
						}

						ensure(!tex.is_locked() && tex.exists());
						textures_to_remove.push_back(&tex);
					}
				}
			}

			for (auto& tex : textures_to_remove)
			{
				tex->destroy();
			}

			return !textures_to_remove.empty();
		}

		void trim_sections()
		{
			for (auto it = m_in_use.begin(); it != m_in_use.end(); it++)
			{
				auto* block = *it;
				if (block->get_locked_count() > 256)
				{
					for (auto& tex : *block)
					{
						if (tex.is_locked() && !tex.is_locked(true))
						{
							tex.sync_protection();
						}
					}
				}
			}
		}


		/**
		 * Callbacks
		 */
		void on_section_released(const section_storage_type &/*section*/)
		{
			u32 prev_unreleased = m_unreleased_texture_objects--;
			ensure(prev_unreleased > 0);
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
			u64 size = section.get_section_size();
			u64 prev_size = m_texture_memory_in_use.fetch_sub(size);
			ensure(prev_size >= size);
		}

		void on_ranged_block_first_section_created(block_type& block)
		{
			AUDIT(m_in_use.find(&block) == m_in_use.end());
			m_in_use.insert(&block);
		}

		void on_ranged_block_last_section_destroyed(block_type& block)
		{
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

			explicit range_iterator_tmpl(parent_type &storage, const address_range32 &_range, section_bounds _bounds, bool _locked_only)
				: range(_range)
				, bounds(_bounds)
				, block(&storage.block_for(range.start))
				, unowned_remaining(true)
				, unowned_it(block->unowned_begin())
				, cur_block_it(block->begin())
				, locked_only(_locked_only)
			{
				// do a "fake" iteration to ensure the internal state is consistent
				next(false);
			}

		private:
			// Members
			address_range32 range;
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
							++unowned_it;
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
							++cur_block_it;
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

		inline range_iterator range_begin(const address_range32 &range, section_bounds bounds, bool locked_only = false) {
			return range_iterator(*this, range, bounds, locked_only);
		}

		inline range_const_iterator range_begin(const address_range32 &range, section_bounds bounds, bool locked_only = false) const {
			return range_const_iterator(*this, range, bounds, locked_only);
		}

		inline range_const_iterator range_begin(u32 address, section_bounds bounds, bool locked_only = false) const {
			return range_const_iterator(*this, address_range32::start_length(address, 1), bounds, locked_only);
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

	class buffered_section
	{
	private:
		address_range32 locked_range;
		address_range32 cpu_range = {};
		address_range32 confirmed_range;

		utils::protection protection = utils::protection::rw;

		section_protection_strategy protection_strat = section_protection_strategy::lock;
		u64 mem_hash = 0;

		bool locked = false;
		void init_lockable_range(const address_range32& range);
		u64  fast_hash_internal() const;

	public:

		buffered_section() = default;
		~buffered_section() = default;

		void reset(const address_range32& memory_range);

	protected:
		void invalidate_range();

	public:
		void protect(utils::protection new_prot, bool force = false);
		void protect(utils::protection prot, const std::pair<u32, u32>& new_confirm);
		void unprotect();
		bool sync() const;

		void discard();
		const address_range32& get_bounds(section_bounds bounds) const;

		bool is_locked(bool actual_page_flags = false) const;

		/**
		 * Overlapping checks
		 */
		inline bool overlaps(const u32 address, section_bounds bounds) const
		{
			return get_bounds(bounds).overlaps(address);
		}

		inline bool overlaps(const address_range32& other, section_bounds bounds) const
		{
			return get_bounds(bounds).overlaps(other);
		}

		inline bool overlaps(const address_range_vector32& other, section_bounds bounds) const
		{
			return get_bounds(bounds).overlaps(other);
		}

		inline bool overlaps(const buffered_section& other, section_bounds bounds) const
		{
			return get_bounds(bounds).overlaps(other.get_bounds(bounds));
		}

		inline bool inside(const address_range32& other, section_bounds bounds) const
		{
			return get_bounds(bounds).inside(other);
		}

		inline bool inside(const address_range_vector32& other, section_bounds bounds) const
		{
			return get_bounds(bounds).inside(other);
		}

		inline bool inside(const buffered_section& other, section_bounds bounds) const
		{
			return get_bounds(bounds).inside(other.get_bounds(bounds));
		}

		inline s32 signed_distance(const address_range32& other, section_bounds bounds) const
		{
			return get_bounds(bounds).signed_distance(other);
		}

		inline u32 distance(const address_range32& other, section_bounds bounds) const
		{
			return get_bounds(bounds).distance(other);
		}

		/**
		* Utilities
		*/
		inline bool valid_range() const
		{
			return cpu_range.valid();
		}

		inline u32 get_section_base() const
		{
			return cpu_range.start;
		}

		inline u32 get_section_size() const
		{
			return cpu_range.valid() ? cpu_range.length() : 0;
		}

		inline const address_range32& get_locked_range() const
		{
			AUDIT(locked);
			return locked_range;
		}

		inline const address_range32& get_section_range() const
		{
			return cpu_range;
		}

		const address_range32& get_confirmed_range() const
		{
			return confirmed_range.valid() ? confirmed_range : cpu_range;
		}

		const std::pair<u32, u32> get_confirmed_range_delta() const
		{
			if (!confirmed_range.valid())
				return { 0, cpu_range.length() };

			return { confirmed_range.start - cpu_range.start, confirmed_range.length() };
		}

		inline bool matches(const address_range32& range) const
		{
			return cpu_range.valid() && cpu_range == range;
		}

		inline utils::protection get_protection() const
		{
			return protection;
		}

		inline address_range32 get_min_max(const address_range32& current_min_max, section_bounds bounds) const
		{
			return get_bounds(bounds).get_min_max(current_min_max);
		}

		/**
		 * Super Pointer
		 */
		template <typename T = void>
		inline T* get_ptr(u32 address) const
		{
			return reinterpret_cast<T*>(vm::g_sudo_addr + address);
		}
	};

	/**
	 * Cached Texture Section
	 */
	template <typename derived_type, typename traits>
	class cached_texture_section : public rsx::buffered_section, public rsx::ref_counted
	{
	public:
		using ranged_storage_type       = ranged_storage<traits>;
		using ranged_storage_block_type = ranged_storage_block<ranged_storage_type>;
		using texture_cache_type        = typename traits::texture_cache_base_type;
		using predictor_type            = texture_cache_predictor<traits>;
		using predictor_key_type        = typename predictor_type::key_type;
		using predictor_entry_type      = typename predictor_type::mapped_type;

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

		u32 real_pitch;
		u32 rsx_pitch;

		u32 gcm_format = 0;
		bool pack_unpack_swap_bytes = false;
		bool swizzled = false;

		u64 sync_timestamp = 0;
		bool synchronized = false;
		bool flushed = false;
		bool speculatively_flushed = false;

		rsx::memory_read_flags readback_behaviour = rsx::memory_read_flags::flush_once;
		rsx::component_order view_flags = rsx::component_order::default_;
		rsx::texture_upload_context context = rsx::texture_upload_context::shader_read;
		rsx::texture_dimension_extended image_type = rsx::texture_dimension_extended::texture_dimension_2d;

		address_range_vector32 flush_exclusions; // Address ranges that will be skipped during flush

		predictor_type *m_predictor = nullptr;
		usz m_predictor_key_hash = 0;
		predictor_entry_type *m_predictor_entry = nullptr;

	public:
		u64 cache_tag = 0;
		u64 last_write_tag = 0;

		~cached_texture_section()
		{
			AUDIT(!exists());
		}

		cached_texture_section() = default;
		cached_texture_section(ranged_storage_block_type *block)
		{
			initialize(block);
		}

		void initialize(ranged_storage_block_type *block)
		{
			ensure(m_block == nullptr && m_tex_cache == nullptr && m_storage == nullptr);
			m_block = block;
			m_storage = &block->get_storage();
			m_tex_cache = &block->get_texture_cache();
			m_predictor = &m_tex_cache->get_predictor();

			update_unreleased();
		}


		/**
		 * Reset
		 */
		void reset(const address_range32 &memory_range)
		{
			AUDIT(memory_range.valid());
			AUDIT(!is_locked());

			// Destroy if necessary
			destroy();

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
			swizzled = false;

			sync_timestamp = 0ull;
			synchronized = false;
			flushed = false;
			speculatively_flushed = false;

			cache_tag = 0ull;
			last_write_tag = 0ull;

			m_predictor_entry = nullptr;

			readback_behaviour = rsx::memory_read_flags::flush_once;
			view_flags = rsx::component_order::default_;
			context = rsx::texture_upload_context::shader_read;
			image_type = rsx::texture_dimension_extended::texture_dimension_2d;

			flush_exclusions.clear();

			// Set to dirty
			set_dirty(true);

			// Notify that our CPU range is now valid
			notify_range_valid();
		}

		void create_dma_only(u16 width, u16 height, u32 pitch)
		{
			this->width = width;
			this->height = height;
			this->rsx_pitch = pitch;

			set_context(rsx::texture_upload_context::dma);
		}

		/**
		 * Destroyed Flag
		 */
		inline bool is_destroyed() const { return !exists(); } // this section is currently destroyed

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
			ensure(!is_locked());
			ensure(is_managed());

			// Set dirty
			set_dirty(true);

			// Trigger callbacks
			m_block->on_section_resources_destroyed(*derived());
			m_storage->on_section_resources_destroyed(*derived());

			// Invalidate range
			invalidate_range();
		}

		virtual void dma_abort()
		{}

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

			AUDIT(dirty || exists());

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

			m_predictor_entry = nullptr;
			speculatively_flushed = false;
			buffered_section::invalidate_range();
		}

	public:
		/**
		 * Misc.
		 */
		bool is_unreleased() const
		{
			return exists() && is_dirty() && !is_locked();
		}

		bool can_be_reused() const
		{
			if (has_refs()) [[unlikely]]
			{
				return false;
			}

			return !exists() || (is_dirty() && !is_locked());
		}

		bool is_flushable() const
		{
			//This section is active and can be flushed to cpu
			return (get_protection() == utils::protection::no);
		}

		bool is_synchronized() const
		{
			return synchronized;
		}

		bool is_flushed() const
		{
			return flushed;
		}

		bool should_flush() const
		{
			if (context == rsx::texture_upload_context::framebuffer_storage)
			{
				const auto surface = derived()->get_render_target();
				return surface->has_flushable_data();
			}

			return true;
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

			if (context == rsx::texture_upload_context::framebuffer_storage && !Emu.IsStopped())
			{
				// Lock, unlock
				auto surface = derived()->get_render_target();

				if (prot == utils::protection::no && old_prot != utils::protection::no)
				{
					// Locked memory. We have to take ownership of the object in the surface cache as well
					surface->on_lock();
				}
				else if (old_prot == utils::protection::no && prot != utils::protection::no)
				{
					// Release the surface, the cache can remove it if needed
					ensure(prot == utils::protection::rw);
					surface->on_unlock();
				}
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
			if (synchronized && !flushed)
			{
				// Abort enqueued transfer
				dma_abort();
			}

			//Reset properties and protect again
			flushed = false;
			synchronized = false;
			sync_timestamp = 0ull;

			protect(prot);
		}

		void reprotect(const utils::protection prot, const std::pair<u32, u32>& range)
		{
			if (synchronized && !flushed)
			{
				// Abort enqueued transfer
				dma_abort();
			}

			//Reset properties and protect again
			flushed = false;
			synchronized = false;
			sync_timestamp = 0ull;

			protect(prot, range);
		}

		/**
		 * Prediction
		 */
		bool tracked_by_predictor() const
		{
			// We do not update the predictor statistics for flush_always sections
			return get_context() != texture_upload_context::shader_read && get_memory_read_flags() != memory_read_flags::flush_always;
		}

		void on_flush()
		{
			speculatively_flushed = false;

			m_tex_cache->on_flush();

			if (tracked_by_predictor())
			{
				get_predictor_entry().on_flush();
			}

			flush_exclusions.clear();

			if (context == rsx::texture_upload_context::framebuffer_storage)
			{
				derived()->get_render_target()->sync_tag();
			}
		}

		void on_speculative_flush()
		{
			speculatively_flushed = true;

			m_tex_cache->on_speculative_flush();
		}

		void on_miss()
		{
			rsx_log.warning("Cache miss at address 0x%X. This is gonna hurt...", get_section_base());
			m_tex_cache->on_miss(*derived());
		}

		void touch(u64 tag)
		{
			last_write_tag = tag;

			if (tracked_by_predictor())
			{
				get_predictor_entry().on_write(speculatively_flushed);
			}

			if (speculatively_flushed)
			{
				m_tex_cache->on_misprediction();
			}

			flush_exclusions.clear();
		}

		bool sync_protection()
		{
			if (!buffered_section::sync())
			{
				discard(true);
				ensure(is_dirty());
				return false;
			}

			return true;
		}


		/**
		 * Flush
		 */
	private:
		void imp_flush_memcpy(u32 vm_dst, u8* src, u32 len) const
		{
			u8 *dst = get_ptr<u8>(vm_dst);
			address_range32 copy_range = address_range32::start_length(vm_dst, len);

			if (flush_exclusions.empty() || !copy_range.overlaps(flush_exclusions))
			{
				// Normal case = no flush exclusions, or no overlap
				memcpy(dst, src, len);
				return;
			}
			else if (copy_range.inside(flush_exclusions))
			{
				// Nothing to copy
				return;
			}

			// Otherwise, we need to filter the memcpy with our flush exclusions
			// Should be relatively rare
			address_range_vector32 vec;
			vec.merge(copy_range);
			vec.exclude(flush_exclusions);

			for (const auto& rng : vec)
			{
				if (!rng.valid())
					continue;

				AUDIT(rng.inside(copy_range));
				u32 offset = rng.start - vm_dst;
				memcpy(dst + offset, src + offset, rng.length());
			}
		}

		virtual void imp_flush()
		{
			AUDIT(synchronized);

			ensure(real_pitch > 0);

			// Calculate valid range
			const auto valid_range  = get_confirmed_range();
			AUDIT(valid_range.valid());
			const auto valid_length = valid_range.length();
			const auto valid_offset = valid_range.start - get_section_base();
			AUDIT(valid_length > 0);

			// In case of pitch mismatch, match the offset point to the correct point
			u32 mapped_offset, mapped_length;
			if (real_pitch != rsx_pitch)
			{
				if (!valid_offset) [[likely]]
				{
					mapped_offset = 0;
				}
				else
				{
					const u32 offset_in_x = valid_offset % rsx_pitch;
					const u32 offset_in_y = valid_offset / rsx_pitch;
					mapped_offset = (offset_in_y * real_pitch) + offset_in_x;
				}

				const u32 available_vmem = (get_section_size() / rsx_pitch) * real_pitch + std::min<u32>(get_section_size() % rsx_pitch, real_pitch);
				mapped_length = std::min(available_vmem - mapped_offset, valid_length);
			}
			else
			{
				mapped_offset = valid_offset;
				mapped_length = valid_length;
			}

			// Obtain pointers to the source and destination memory regions
			u8 *src = static_cast<u8*>(derived()->map_synchronized(mapped_offset, mapped_length));
			u32 dst = valid_range.start;
			ensure(src != nullptr);

			// Copy from src to dst
			if (real_pitch >= rsx_pitch || valid_length <= rsx_pitch)
			{
				imp_flush_memcpy(dst, src, valid_length);
			}
			else
			{
				u8 *_src = src;
				u32 _dst = dst;

				const auto num_exclusions = flush_exclusions.size();
				if (num_exclusions > 0)
				{
					rsx_log.warning("Slow imp_flush path triggered with non-empty flush_exclusions (%d exclusions, %d bytes), performance might suffer", num_exclusions, valid_length);
				}

				for (s32 remaining = s32(valid_length); remaining > 0; remaining -= rsx_pitch)
				{
					imp_flush_memcpy(_dst, _src, real_pitch);
					_src += real_pitch;
					_dst += rsx_pitch;
				}
			}
		}


	public:
		// Returns false if there was a cache miss
		void flush()
		{
			if (flushed) return;

			// Sanity checks
			ensure(exists());
			AUDIT(is_locked());

			auto cleanup_flush = [&]()
			{
				flushed = true;
				flush_exclusions.clear();
				on_flush();
			};

			// If we are fully inside the flush exclusions regions, we just mark ourselves as flushed and return
			// We apply the same skip if there is nothing new in the surface data
			if (!should_flush() || get_confirmed_range().inside(flush_exclusions))
			{
				cleanup_flush();
				return;
			}

			// NOTE: Hard faults should have been pre-processed beforehand
			ensure(synchronized);

			// Copy flush result to guest memory
			imp_flush();

			// Finish up
			// Its highly likely that this surface will be reused, so we just leave resources in place
			derived()->finish_flush();
			cleanup_flush();
		}

		void add_flush_exclusion(const address_range32& rng)
		{
			AUDIT(is_locked() && is_flushable());
			const auto _rng = rng.get_intersect(get_section_range());
			flush_exclusions.merge(_rng);
		}

		/**
		 * Misc
		 */
	public:
		predictor_entry_type& get_predictor_entry()
		{
			// If we don't have a predictor entry, or the key has changed
			if (m_predictor_entry == nullptr || !m_predictor_entry->key_matches(*derived()))
			{
				m_predictor_entry = &((*m_predictor)[*derived()]);
			}
			return *m_predictor_entry;
		}

		void set_view_flags(rsx::component_order flags)
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

		void set_swizzled(bool is_swizzled)
		{
			swizzled = is_swizzled;
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

		u32 get_rsx_pitch() const
		{
			return rsx_pitch;
		}

		rsx::component_order get_view_flags() const
		{
			return view_flags;
		}

		rsx::texture_upload_context get_context() const
		{
			return context;
		}

		rsx::section_bounds get_overlap_test_bounds() const
		{
			return rsx::section_bounds::locked_range;
		}

		rsx::texture_dimension_extended get_image_type() const
		{
			return image_type;
		}

		u32 get_gcm_format() const
		{
			return gcm_format;
		}

		bool is_swizzled() const
		{
			return swizzled;
		}

		memory_read_flags get_memory_read_flags() const
		{
			return readback_behaviour;
		}

		u64 get_sync_timestamp() const
		{
			return sync_timestamp;
		}

		rsx::format_class get_format_class() const
		{
			return classify_format(gcm_format);
		}

		/**
		 * Comparison
		 */
		inline bool matches(const address_range32 &memory_range) const
		{
			return valid_range() && rsx::buffered_section::matches(memory_range);
		}

		bool matches(u32 format, u32 width, u32 height, u32 depth, u32 mipmaps) const
		{
			if (!valid_range())
				return false;

			if (format && gcm_format != format)
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

		bool matches(u32 rsx_address, u32 format, u32 width, u32 height, u32 depth, u32 mipmaps) const
		{
			if (!valid_range())
				return false;

			if (rsx_address != get_section_base())
				return false;

			return matches(format, width, height, depth, mipmaps);
		}

		bool matches(const address_range32& memory_range, u32 format, u32 width, u32 height, u32 depth, u32 mipmaps) const
		{
			if (!valid_range())
				return false;

			if (!rsx::buffered_section::matches(memory_range))
				return false;

			return matches(format, width, height, depth, mipmaps);
		}


		/**
		 * Derived wrappers
		 */
		void destroy()
		{
			derived()->destroy();
		}

		bool is_managed() const
		{
			return derived()->is_managed();
		}

		bool exists() const
		{
			if (derived()->exists())
			{
				return true;
			}
			else
			{
				return (context == rsx::texture_upload_context::dma && is_locked());
			}
		}
	};

} // namespace rsx
