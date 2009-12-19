
#pragma once

// HashBucket is a container which uses a built-in hash function
// to perform quick searches.
// T is a struct data type.
// hSize determines the number of buckets HashBucket will use for sorting.
// cmpSize is the size of data to consider 2 structs equal (see find())
// The hash function is determined by taking the first bytes of data and
// performing a modulus the size of hSize. So the most diverse-data should
// be in the first bytes of the struct. (hence why nVifBlock is specifically sorted)
template<typename T, int hSize, int cmpSize>
class HashBucket {
private:
	T*  mChain[hSize];
	int mSize [hSize];
public:
	HashBucket() { 
		for (int i = 0; i < hSize; i++) {
			mChain[i] = NULL;
			mSize [i] = 0;
		}
	}
	~HashBucket() { clear(); }
	int quickFind(u32 data) {
		int o = data % hSize;
		return mSize[o];
	}
	T* find(T* dataPtr) {
		u32 d = *((u32*)dataPtr);
		int o = d % hSize;
		int s = mSize[o];
		T*  c = mChain[o];
		for (int i = 0; i < s; i++) {
			if (!memcmp(&c[i], dataPtr, cmpSize)) return &c[i];
		}
		return NULL;
	}
	void add(T* dataPtr) {
		u32 d = *(u32*)dataPtr;
		int o = d % hSize;
		int s = mSize[o]++;
		T*  c = mChain[o];
		T*  n = new T[s+1];
		if (s) { 
			memcpy(n, c, sizeof(T) * s);
			delete[]  c;
		}
		memcpy(&n[s], dataPtr, sizeof(T));
		mChain[o] = n;
	}
	void clear() {
		for (int i = 0; i < hSize; i++) {
			safe_delete_array(mChain[i]);
			mSize[i] = 0;
		}
	}
};
