#pragma once

#ifdef TEXTURE_CACHE_DEBUG

#include "../rsx_utils.h"

namespace rsx {

	class tex_cache_checker_t {
		struct per_page_info_t {
			u8 prot = 0;
			u8 no = 0;
			u8 ro = 0;

			FORCE_INLINE utils::protection get_protection() const
			{
				return static_cast<utils::protection>(prot);
			}

			FORCE_INLINE void set_protection(utils::protection prot)
			{
				this->prot = static_cast<u8>(prot);
			}

			FORCE_INLINE void reset_refcount()
			{
				no = 0;
				ro = 0;
			}

			FORCE_INLINE u16 sum() const
			{
				return u16{ no } + ro;
			}

			FORCE_INLINE bool verify() const
			{
				const utils::protection prot = get_protection();
				switch (prot)
				{
				case utils::protection::no: return no > 0;
				case utils::protection::ro: return no == 0 && ro > 0;
				case utils::protection::rw: return no == 0 && ro == 0;
				default: fmt::throw_exception("Unreachable");
				}
			}

			FORCE_INLINE void add(utils::protection prot)
			{
				switch (prot)
				{
				case utils::protection::no: if (no++ == umax) fmt::throw_exception("add(protection::no) overflow"); return;
				case utils::protection::ro: if (ro++ == umax) fmt::throw_exception("add(protection::ro) overflow"); return;
				default: fmt::throw_exception("Unreachable");
				}
			}

			FORCE_INLINE void remove(utils::protection prot)
			{
				switch (prot)
				{
				case utils::protection::no: if (no-- == 0) fmt::throw_exception("remove(protection::no) overflow with NO==0"); return;
				case utils::protection::ro: if (ro-- == 0) fmt::throw_exception("remove(protection::ro) overflow with RO==0"); return;
				default: fmt::throw_exception("Unreachable");
				}
			}
		};
		static_assert(sizeof(per_page_info_t) <= 4, "page_info_elmnt must be less than 4-bytes in size");


		// 4GB memory space / 4096 bytes per page = 1048576 pages
		// Initialized to utils::protection::rw
		static constexpr usz num_pages = 0x1'0000'0000 / 4096;
		per_page_info_t _info[num_pages]{0};

		static_assert(static_cast<u32>(utils::protection::rw) == 0, "utils::protection::rw must have value 0 for the above constructor to work");

		static constexpr usz rsx_address_to_index(u32 address)
		{
			return (address / 4096);
		}

		static constexpr u32 index_to_rsx_address(usz idx)
		{
			return static_cast<u32>(idx * 4096);
		}

		constexpr per_page_info_t* rsx_address_to_info_pointer(u32 address)
		{
			return &(_info[rsx_address_to_index(address)]);
		}

		constexpr const per_page_info_t* rsx_address_to_info_pointer(u32 address) const
		{
			return &(_info[rsx_address_to_index(address)]);
		}

		constexpr u32 info_pointer_to_address(const per_page_info_t* ptr) const
		{
			return index_to_rsx_address(static_cast<usz>(ptr - _info));
		}

		std::string prot_to_str(utils::protection prot) const
		{
			switch (prot)
			{
			case utils::protection::no: return "NA";
			case utils::protection::ro: return "RO";
			case utils::protection::rw: return "RW";
			default: fmt::throw_exception("Unreachable");
			}
		}

	public:
		void set_protection(const address_range32& range, utils::protection prot)
		{
			AUDIT(range.is_page_range());
			AUDIT(prot == utils::protection::no || prot == utils::protection::ro || prot == utils::protection::rw);

			for (per_page_info_t* ptr = rsx_address_to_info_pointer(range.start); ptr <= rsx_address_to_info_pointer(range.end); ptr++)
			{
				ptr->set_protection(prot);
			}
		}

		void discard(const address_range32& range)
		{
			set_protection(range, utils::protection::rw);
		}

		void reset_refcount()
		{
			for (per_page_info_t* ptr = rsx_address_to_info_pointer(0); ptr <= rsx_address_to_info_pointer(0xFFFFFFFF); ptr++)
			{
				ptr->reset_refcount();
			}
		}

		void add(const address_range32& range, utils::protection prot)
		{
			AUDIT(range.is_page_range());
			AUDIT(prot == utils::protection::no || prot == utils::protection::ro);

			for (per_page_info_t* ptr = rsx_address_to_info_pointer(range.start); ptr <= rsx_address_to_info_pointer(range.end); ptr++)
			{
				ptr->add(prot);
			}
		}

		void remove(const address_range32& range, utils::protection prot)
		{
			AUDIT(range.is_page_range());
			AUDIT(prot == utils::protection::no || prot == utils::protection::ro);

			for (per_page_info_t* ptr = rsx_address_to_info_pointer(range.start); ptr <= rsx_address_to_info_pointer(range.end); ptr++)
			{
				ptr->remove(prot);
			}
		}

		// Returns the a lower bound as to how many locked sections are known to be within the given range with each protection {NA,RO}
		// The assumption here is that the page in the given range with the largest number of refcounted sections represents the lower bound to how many there must be
		std::pair<u8,u8> get_minimum_number_of_sections(const address_range32& range) const
		{
			AUDIT(range.is_page_range());

			u8 no = 0;
			u8 ro = 0;
			for (const per_page_info_t* ptr = rsx_address_to_info_pointer(range.start); ptr <= rsx_address_to_info_pointer(range.end); ptr++)
			{
				no = std::max(no, ptr->no);
				ro = std::max(ro, ptr->ro);
			}

			return { no,ro };
		}

		void check_unprotected(const address_range32& range, bool allow_ro = false, bool must_be_empty = true) const
		{
			AUDIT(range.is_page_range());
			for (const per_page_info_t* ptr = rsx_address_to_info_pointer(range.start); ptr <= rsx_address_to_info_pointer(range.end); ptr++)
			{
				const auto prot = ptr->get_protection();
				if (prot != utils::protection::rw && (!allow_ro || prot != utils::protection::ro))
				{
					const u32 addr = info_pointer_to_address(ptr);
					fmt::throw_exception("Page at addr=0x%8x should be RW%s: Prot=%s, RO=%d, NA=%d", addr, allow_ro ? " or RO" : "", prot_to_str(prot), ptr->ro, ptr->no);
				}

				if (must_be_empty && (
						ptr->no > 0 ||
						(!allow_ro && ptr->ro > 0)
					))
				{
					const u32 addr = info_pointer_to_address(ptr);
					fmt::throw_exception("Page at addr=0x%8x should not have any NA%s sections: Prot=%s, RO=%d, NA=%d", addr, allow_ro ? " or RO" : "", prot_to_str(prot), ptr->ro, ptr->no);
				}
			}
		}

		void verify() const
		{
			for (usz idx = 0; idx < num_pages; idx++)
			{
				auto &elmnt = _info[idx];
				if (!elmnt.verify())
				{
					const u32 addr = index_to_rsx_address(idx);
					const utils::protection prot = elmnt.get_protection();
					fmt::throw_exception("Protection verification failed at addr=0x%x: Prot=%s, RO=%d, NA=%d", addr, prot_to_str(prot), elmnt.ro, elmnt.no);
				}
			}
		}
	};

	extern tex_cache_checker_t tex_cache_checker;
}; // namespace rsx
#endif //TEXTURE_CACHE_DEBUG