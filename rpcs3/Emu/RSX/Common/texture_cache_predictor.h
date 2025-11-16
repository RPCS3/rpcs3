#pragma once

#include "../rsx_utils.h"
#include "TextureUtils.h"

#include <unordered_map>

namespace rsx
{
	/**
	 * Predictor Entry History Queue
	 */
	template <u32 buffer_size>
	class texture_cache_predictor_entry_history_queue
	{
		std::array<u32, buffer_size> buffer;
		u32 m_front;
		u32 m_size;

	public:
		texture_cache_predictor_entry_history_queue()
		{
			clear();
		}

		void clear()
		{
			m_front = buffer_size;
			m_size  = 0;
		}

		usz size() const
		{
			return m_size;
		}

		bool empty() const
		{
			return m_size == 0;
		}

		void push(u32 val)
		{
			if (m_size < buffer_size)
			{
				m_size++;
			}

			if (m_front == 0)
			{
				m_front = buffer_size - 1;
			}
			else
			{
				m_front--;
			}

			AUDIT(m_front < buffer_size);
			buffer[m_front] = val;
		}

		u32 operator[](u32 pos) const
		{
			AUDIT(pos < m_size);
			AUDIT(m_front < buffer_size);
			return buffer[(m_front + pos) % buffer_size];
		}
	};

	/**
	 * Predictor key
	 */
	template <typename traits>
	struct texture_cache_predictor_key
	{
		using texture_format       = typename traits::texture_format;
		using section_storage_type = typename traits::section_storage_type;

		address_range32          cpu_range;
		texture_format         format;
		texture_upload_context context;

		// Constructors
		texture_cache_predictor_key() = default;

		texture_cache_predictor_key(const address_range32& _cpu_range, texture_format _format, texture_upload_context _context)
		    : cpu_range(_cpu_range)
		    , format(_format)
			, context(_context)
		{}

		texture_cache_predictor_key(const section_storage_type& section)
		    : cpu_range(section.get_section_range())
		    , format(section.get_format())
			, context(section.get_context())
		{}

		// Methods
		bool operator==(const texture_cache_predictor_key& other) const
		{
			return cpu_range == other.cpu_range && format == other.format && context == other.context;
		}
	};

	/**
	 * Predictor entry
	 */
	template<typename traits>
	class texture_cache_predictor_entry
	{
	public:
		using key_type = texture_cache_predictor_key<traits>;
		using section_storage_type = typename traits::section_storage_type;

		const key_type key;

	private:
		u32 m_guessed_writes;
		u32 m_writes_since_last_flush;

		static const u32 max_write_history_size = 16;
		texture_cache_predictor_entry_history_queue<max_write_history_size> write_history;

		static const u32 max_confidence      = 8; // Cannot be more "confident" than this value
		static const u32 confident_threshold = 6; // We are confident if confidence >= confidence_threshold
		static const u32 starting_confidence = 3;

		static const u32 confidence_guessed_flush    =  2; // Confidence granted when we correctly guess there will be a flush
		static const u32 confidence_guessed_no_flush =  1; // Confidence granted when we correctly guess there won't be a flush
		static const u32 confidence_incorrect_guess  = -2; // Confidence granted when our guess is incorrect
		static const u32 confidence_mispredict       = -4; // Confidence granted when a speculative flush is incorrect

		u32 confidence;

	public:
		texture_cache_predictor_entry(key_type _key)
			: key(std::move(_key))
		{
			reset();
		}
		~texture_cache_predictor_entry() = default;

		u32 get_confidence() const
		{
			return confidence;
		}

		bool is_confident() const
		{
			return confidence >= confident_threshold;
		}

		bool key_matches(const key_type& other_key) const
		{
			return key == other_key;
		}

		bool key_matches(const section_storage_type& section) const
		{
			return key_matches(key_type(section));
		}

		void update_confidence(s32 delta)
		{
			if (delta > 0)
			{
				confidence += delta;

				if (confidence > max_confidence)
				{
					confidence = max_confidence;
				}
			}
			else if (delta < 0)
			{
				u32 neg_delta = static_cast<u32>(-delta);
				if (confidence > neg_delta)
				{
					confidence -= neg_delta;
				}
				else
				{
					confidence = 0;
				}
			}
		}

	private:
		// Returns how many writes we think there will be this time (i.e. between the last flush and the next flush)
		// Returning UINT32_MAX means no guess is possible
		u32 guess_number_of_writes() const
		{
			const auto history_size = write_history.size();

			if (history_size == 0)
			{
				// We need some history to be able to take a guess
				return -1;
			}
			else if (history_size == 1)
			{
				// If we have one history entry, we assume it will repeat
				return write_history[0];
			}
			else
			{
				// For more than one entry, we try and find a pattern, and assume it holds

				const u32 stop_when_found_matches = 4;
				u32 matches_found                 = 0;
				u32 guess                         = -1;

				for (u32 i = 0; i < history_size; i++)
				{
					// If we are past the number of writes, it's not the same as this time
					if (write_history[i] < m_writes_since_last_flush)
						continue;

					u32 cur_matches_found = 1;

					// Try to find more matches
					for (u32 j = 0; i + j + 1 < history_size; j++)
					{
						if (write_history[i + j + 1] != write_history[j])
							break;

						// We found another matching value
						cur_matches_found++;

						if (cur_matches_found >= stop_when_found_matches)
							break;
					}

					// If we found more matches than all other comparisons, we take a guess
					if (cur_matches_found > matches_found)
					{
						guess         = write_history[i];
						matches_found = cur_matches_found;
					}

					if (matches_found >= stop_when_found_matches)
						break;
				}

				return guess;
			}
		}

		void calculate_next_guess(bool reset)
		{
			if (reset || m_guessed_writes == umax || m_writes_since_last_flush > m_guessed_writes)
			{
				m_guessed_writes = guess_number_of_writes();
			}
		}

	public:
		void reset()
		{
			confidence                = starting_confidence;
			m_writes_since_last_flush = 0;
			m_guessed_writes          = -1;
			write_history.clear();
		}

		void on_flush()
		{
			update_confidence(is_flush_likely() ? confidence_guessed_flush : confidence_incorrect_guess);

			// Update history
			write_history.push(m_writes_since_last_flush);
			m_writes_since_last_flush = 0;

			calculate_next_guess(true);
		}

		void on_write(bool mispredict)
		{
			if (mispredict || is_flush_likely())
			{
				update_confidence(mispredict ? confidence_mispredict : confidence_incorrect_guess);
			}
			else
			{
				update_confidence(confidence_guessed_no_flush);
			}

			m_writes_since_last_flush++;

			calculate_next_guess(false);
		}

		bool is_flush_likely() const
		{
			return m_writes_since_last_flush >= m_guessed_writes;
		}

		// Returns true if we believe that the next operation on this memory range will be a flush
		bool predict() const
		{
			// Disable prediction if we have a low confidence in our predictions
			if (!is_confident())
				return false;

			return is_flush_likely();
		}
	};

	/**
	 * Predictor
	 */
	template <typename traits>
	class texture_cache_predictor
	{
	public:
		// Traits
		using section_storage_type = typename traits::section_storage_type;
		using texture_cache_type   = typename traits::texture_cache_base_type;

		using key_type    = texture_cache_predictor_key<traits>;
		using mapped_type = texture_cache_predictor_entry<traits>;
		using map_type    = std::unordered_map<key_type, mapped_type>;

		using value_type     = typename map_type::value_type;
		using size_type      = typename map_type::size_type;
		using iterator       = typename map_type::iterator;
		using const_iterator = typename map_type::const_iterator;

	private:
		// Member variables
		map_type m_entries;
		texture_cache_type* m_tex_cache;

	public:
		// Per-frame statistics
		atomic_t<u32> m_mispredictions_this_frame = {0};

		// Constructors
		texture_cache_predictor(texture_cache_type* tex_cache)
		    : m_tex_cache(tex_cache) {}
		~texture_cache_predictor() = default;

		// Trait wrappers
		constexpr iterator begin() noexcept { return m_entries.begin(); }
		constexpr const_iterator begin() const noexcept { return m_entries.begin(); }
		inline iterator end() noexcept { return m_entries.end(); }
		inline const_iterator end() const noexcept { return m_entries.end(); }
		bool empty() const noexcept { return m_entries.empty(); }
		size_type size() const noexcept { return m_entries.size(); }
		void clear() { m_entries.clear(); }

		mapped_type& operator[](const key_type& key)
		{
			auto ret = m_entries.try_emplace(key, key);
			AUDIT(ret.first != m_entries.end());
			return ret.first->second;
		}
		mapped_type& operator[](const section_storage_type& section)
		{
			return (*this)[key_type(section)];
		}

		// Callbacks
		void on_frame_end()
		{
			m_mispredictions_this_frame = 0;
		}

		void on_misprediction()
		{
			m_mispredictions_this_frame++;
		}

		// Returns true if the next operation is likely to be a read
		bool predict(const key_type& key) const
		{
			// Use "find" to avoid allocating entries if they do not exist
			const_iterator entry_it = m_entries.find(key);
			if (entry_it == m_entries.end())
			{
				return false;
			}
			else
			{
				return entry_it->second.predict();
			}
		}

		bool predict(const section_storage_type& section) const
		{
			return predict(key_type(section));
		}
	};
} // namespace rsx

template <typename Traits>
struct std::hash<rsx::texture_cache_predictor_key<Traits>>
{
	usz operator()(const rsx::texture_cache_predictor_key<Traits>& k) const
	{
		usz result = std::hash<utils::address_range32>{}(k.cpu_range);
		result ^= static_cast<usz>(k.format);
		result ^= (static_cast<usz>(k.context) << 16);
		return result;
	}
};
