#pragma once
#include <thread>
#include <array>
#include <mutex>
#include <algorithm>

//Simple non-resizable FIFO Ringbuffer that can be simultaneously be read from and written to
//if we ever get to use boost please replace this with boost::circular_buffer, there's no reason
//why we would have to keep this amateur attempt at such a fundamental data-structure around
template< typename T, unsigned int MAX_MTRINGBUFFER_BUFFER_SIZE>
class MTRingbuffer{
	std::array<T, MAX_MTRINGBUFFER_BUFFER_SIZE> mBuffer;
	//this is a recursive mutex because the get methods lock it but the only
	//way to be sure that they do not block is to check the size and the only
	//way to check the size and use get atomically is to lock this mutex,
	//so it goes:
	//lock get mutex-->check size-->call get-->lock get mutex-->unlock get mutex-->return from get-->unlock get mutex
	std::recursive_mutex mMutGet;
	std::mutex mMutPut;

	size_t mGet;
	size_t mPut;
	size_t moveGet(size_t by = 1){ return (mGet + by) % MAX_MTRINGBUFFER_BUFFER_SIZE; }
	size_t movePut(size_t by = 1){ return (mPut + by) % MAX_MTRINGBUFFER_BUFFER_SIZE; }
public:
	MTRingbuffer() : mGet(0), mPut(0){}

	//blocks until there's something to get, so check "spaceLeft()" if you want to avoid blocking
	//also lock the get mutex around the spaceLeft() check and the pop if you want to avoid racing
	T pop()
	{
		std::lock_guard<std::recursive_mutex> lock(mMutGet);
		while (mGet == mPut)
		{
			//wait until there's actually something to get
			//throwing an exception might be better, blocking here is a little awkward
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		size_t ret = mGet;
		mGet = moveGet();
		return mBuffer[ret];
	}

	//blocks if the buffer is full until there's enough room
	void push(T &putEle)
	{
		std::lock_guard<std::mutex> lock(mMutPut);
		while (movePut() == mGet)
		{
			//if this is reached a lot it's time to increase the buffer size
			//or implement dynamic re-sizing
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		mBuffer[mPut] = std::forward(putEle);
		mPut = movePut();
	}

	bool empty()
	{
		return mGet == mPut;
	}

	//returns the amount of free places, this is the amount of actual free spaces-1
	//since mGet==mPut signals an empty buffer we can't actually use the last free
	//space, so we shouldn't report it as free.
	size_t spaceLeft() //apparently free() is a macro definition in msvc in some conditions
	{
		if (mGet < mPut)
		{
			return mBuffer.size() - (mPut - mGet) - 1;
		}
		else if (mGet > mPut)
		{
			return mGet - mPut - 1;
		}
		else
		{
			return mBuffer.size() - 1;
		}
	}

	size_t size()
	{
		//the magic -1 is the same magic 1 that is explained in the spaceLeft() function
		return mBuffer.size() - spaceLeft() - 1;
	}

	//takes random access iterator to T
	template<typename IteratorType>
	void pushRange(IteratorType from, IteratorType until)
	{
		std::lock_guard<std::mutex> lock(mMutPut);
		size_t length = until - from;

		//if whatever we're trying to store is greater than the entire buffer the following loop will be infinite
		assert(mBuffer.size() > length);
		while (spaceLeft() < length)
		{
			//if this is reached a lot it's time to increase the buffer size
			//or implement dynamic re-sizing
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		if (mPut + length <= mBuffer.size())
		{
			std::copy(from, until, mBuffer.begin() + mPut);
		}
		else
		{
			size_t tillEnd = mBuffer.size() - mPut;
			std::copy(from, from + tillEnd, mBuffer.begin() + mPut);
			std::copy(from + tillEnd, until, mBuffer.begin());
		}
		mPut = movePut(length);

	}

	//takes output iterator to T
	template<typename IteratorType>
	void popN(IteratorType output, size_t n)
	{
		std::lock_guard<std::recursive_mutex> lock(mMutGet);
		//make sure we're not trying to retrieve more than is in
		assert(n <= size());
		peekN<IteratorType>(output, n);
		mGet = moveGet(n);
	}

	//takes output iterator to T
	template<typename IteratorType>
	void peekN(IteratorType output, size_t n)
	{
		size_t lGet = mGet;
		if (lGet + n <= mBuffer.size())
		{
			std::copy_n(mBuffer.begin() + lGet, n, output);
		}
		else
		{
			auto next = std::copy(mBuffer.begin() + lGet, mBuffer.end(), output);
			std::copy_n(mBuffer.begin(), n - (mBuffer.size() - lGet), next);
		}
	}

	//well this is just asking for trouble
	//but the comment above the declaration of mMutGet explains why it's there
	//if there's a better way please remove this
	void lockGet()
	{
		mMutGet.lock();
	}

	//well this is just asking for trouble
	//but the comment above the declaration of mMutGet explains why it's there
	//if there's a better way please remove this
	void unlockGet()
	{
		mMutGet.unlock();
	}
};
