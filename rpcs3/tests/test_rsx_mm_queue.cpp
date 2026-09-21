#include <gtest/gtest.h>
#include "Emu/RSX/Host/MM.cpp"

namespace rsx::MM
{
	class MMQueue : public ::testing::Test
	{
	protected:
		static constexpr u32 s_pages = 16;

		u64 m_page_size = 0;
		u8* m_base = nullptr;

		void SetUp() override
		{
			g_cfg.video.disable_async_host_memory_manager.set(false);

			m_page_size = static_cast<u64>(utils::get_page_size());
			m_base = static_cast<u8*>(utils::memory_reserve(m_page_size * s_pages));
			utils::memory_commit(m_base, m_page_size * s_pages);

			g_deferred_mprotect_queue.clear();
		}

		void TearDown() override
		{
			g_deferred_mprotect_queue.clear();
			utils::memory_release(m_base, m_page_size * s_pages);
		}

		void* page(u32 index) const
		{
			return m_base + (index * m_page_size);
		}

		utils::address_range64 pages(u32 first, u32 count) const
		{
			return utils::address_range64::start_length(reinterpret_cast<u64>(page(first)), count * m_page_size);
		}

		void protect(u32 first, u32 count, utils::protection prot)
		{
			mm_protect(page(first), count * m_page_size, prot);
		}
	};

	TEST_F(MMQueue, DeferredProtectIsQueuedUntilFlush)
	{
		protect(0, 1, utils::protection::no);

		ASSERT_EQ(g_deferred_mprotect_queue.size(), 1u);
		EXPECT_EQ(g_deferred_mprotect_queue[0].range, pages(0, 1));
		EXPECT_EQ(g_deferred_mprotect_queue[0].prot, utils::protection::no);

		mm_flush();
		EXPECT_TRUE(g_deferred_mprotect_queue.empty());
	}

	TEST_F(MMQueue, MergesContiguousBlocks)
	{
		for (u32 i = 0; i < 4; i++)
		{
			protect(i, 1, utils::protection::ro);
		}

		ASSERT_EQ(g_deferred_mprotect_queue.size(), 1u);
		EXPECT_EQ(g_deferred_mprotect_queue[0].range, pages(0, 4));
	}

	TEST_F(MMQueue, MergesOverlappingBlocks)
	{
		protect(0, 2, utils::protection::ro);
		protect(1, 2, utils::protection::ro);

		ASSERT_EQ(g_deferred_mprotect_queue.size(), 1u);
		EXPECT_EQ(g_deferred_mprotect_queue[0].range, pages(0, 3));
	}

	TEST_F(MMQueue, DoesNotMergeAcrossProtections)
	{
		protect(0, 1, utils::protection::ro);
		protect(1, 1, utils::protection::no);

		ASSERT_EQ(g_deferred_mprotect_queue.size(), 2u);
		EXPECT_EQ(g_deferred_mprotect_queue[0].prot, utils::protection::ro);
		EXPECT_EQ(g_deferred_mprotect_queue[1].prot, utils::protection::no);
	}

	TEST_F(MMQueue, SwallowedBlockIsReplaced)
	{
		protect(2, 1, utils::protection::ro);
		protect(1, 3, utils::protection::no);

		ASSERT_EQ(g_deferred_mprotect_queue.size(), 1u);
		EXPECT_EQ(g_deferred_mprotect_queue[0].range, pages(1, 3));
		EXPECT_EQ(g_deferred_mprotect_queue[0].prot, utils::protection::no);
	}

	TEST_F(MMQueue, UnlockFlushesPrefixAndKeepsTail)
	{
		protect(0, 1, utils::protection::ro);
		protect(3, 2, utils::protection::no);
		protect(8, 1, utils::protection::ro);
		ASSERT_EQ(g_deferred_mprotect_queue.size(), 3u);

		protect(4, 1, utils::protection::rw);

		ASSERT_EQ(g_deferred_mprotect_queue.size(), 1u);
		EXPECT_EQ(g_deferred_mprotect_queue[0].range, pages(8, 1));
		EXPECT_EQ(g_deferred_mprotect_queue[0].prot, utils::protection::ro);
	}

	TEST_F(MMQueue, UnlockSwallowingWholeQueueLeavesNoResidue)
	{
		protect(1, 1, utils::protection::ro);
		protect(3, 1, utils::protection::no);
		ASSERT_EQ(g_deferred_mprotect_queue.size(), 2u);

		protect(0, 8, utils::protection::rw);

		ASSERT_TRUE(g_deferred_mprotect_queue.empty());

		mm_flush();
		EXPECT_TRUE(g_deferred_mprotect_queue.empty());
	}

	TEST_F(MMQueue, FlushByRangeFlushesPrefixOnly)
	{
		protect(0, 1, utils::protection::ro);
		protect(4, 1, utils::protection::no);
		protect(8, 1, utils::protection::ro);

		mm_flush(rsx::simple_array<utils::address_range64>{ pages(4, 1) });

		ASSERT_EQ(g_deferred_mprotect_queue.size(), 1u);
		EXPECT_EQ(g_deferred_mprotect_queue[0].range, pages(8, 1));
	}
}
