#include "stdafx.h"
#include <intrin.h>
#include <nmmintrin.h>
#include "IndexSet.h"
#include "CompareToVector.h"

#pragma unmanaged

int CountN(unsigned __int64* matchVector, int length)
{
	__int64 count = 0;

	for (int i = 0; i < length; ++i)
	{
		count += _mm_popcnt_u64(matchVector[i]);
	}

	return (int)count;
}

unsigned int __inline ctz(unsigned __int64 value)
{
	unsigned long trailingZero = 0;
	return (_BitScanForward64(&trailingZero, value) ? trailingZero : 64);
}

int PageN(unsigned __int64* matchVector, int length, int start, int* result, int resultLength, int* countSet)
{
	*countSet = 0;
	if (length == 0) return -1;

	// Get pointers to the next index, the last place we can scan a block without checking bounds, and the end of the array
	int* resultNext = result;
	int* resultLastFull = result + resultLength - 64;
	int* resultEnd = result + resultLength;

	// Get the index of the block and position within the block to start at
	int blockIndex = start >> 6;
	int matchWithinBlock = start & 63;

	// Get the first block
	int base = blockIndex << 6;
	unsigned __int64 block = matchVector[blockIndex];

	// If we're resuming within this block, clear already checked bits
	if (matchWithinBlock > 0) block &= (~0x0ULL << matchWithinBlock);

	// While we have room for 64+ matches, find next set bits without checking if we're out of room
	while(resultNext < resultLastFull)
	{
		while (block != 0)
		{
			matchWithinBlock = ctz(block);
			*(resultNext++) = base + matchWithinBlock;
			block &= block - 1;
		}

		++blockIndex;
		base = blockIndex << 6;
		matchWithinBlock = 0;
		if (blockIndex >= length) break;

		block = matchVector[blockIndex];
	}

	// Once the array is almost full, find matches only until the array is full
	if (blockIndex < length)
	{
		while (resultNext < resultEnd)
		{
			while (block != 0 && resultNext != resultEnd)
			{
				matchWithinBlock = ctz(block);
				*(resultNext++) = base + matchWithinBlock;
				block &= block - 1;
			}

			if (resultNext == resultEnd) break;

			++blockIndex;
			base = blockIndex << 6;
			matchWithinBlock = 0;
			if (blockIndex >= length) break;

			block = matchVector[blockIndex];
		}
	}

	// Record the match count found
	*countSet = (int)(resultNext - result);

	// Return -1 if we finished scanning, the next start index otherwise
	int lastIndex = base + matchWithinBlock;
	return (lastIndex >= length << 6 ? -1 : lastIndex + 1);
}

#pragma managed

namespace V5
{
	namespace Collections
	{
		IndexSet::IndexSet()
		{ }

		IndexSet::IndexSet(UInt32 length)
		{
			this->bitVector = gcnew array<UInt64>((length + 63) >> 6);
		}

		Boolean IndexSet::default::get(Int32 index)
		{
			return this->bitVector[index >> 6] & (0x1ULL << (index & 63));
		}

		void IndexSet::default::set(Int32 index, Boolean value)
		{
			if (value)
			{
				this->bitVector[index >> 6] |= (0x1ULL << (index & 63));
			}
			else
			{
				this->bitVector[index >> 6] &= ~(0x1ULL << (index & 63));
			}
		}

		Int32 IndexSet::Count::get()
		{
			pin_ptr<UInt64> pVector = &(this->bitVector)[0];
			return CountN(pVector, this->bitVector->Length);
		}

		Int32 IndexSet::Capacity::get()
		{
			return this->bitVector->Length << 6;
		}

		Boolean IndexSet::Equals(Object^ o)
		{
			if (o == nullptr || GetType() != o->GetType()) return false;
			IndexSet^ other = dynamic_cast<IndexSet^>(o);

			if (this->bitVector->Length != other->bitVector->Length) return false;

			for (int i = 0; i < this->bitVector->Length; ++i)
			{
				if (this->bitVector[i] != other->bitVector[i])
				{
					return false;
				}
			}

			return true;
		}

		Int32 IndexSet::Page(Span<Int32>% page, Int32 fromIndex)
		{
			array<Int32>^ array = page.Array;

			pin_ptr<UInt64> pVector = &(this->bitVector)[0];
			pin_ptr<Int32> pPage = &(array[page.Index]);
			int countSet = 0;
			
			int nextIndex = PageN(pVector, this->bitVector->Length, fromIndex, pPage, page.Capacity, &countSet);
			page.Length = countSet;

			return nextIndex;

			//// Clear the page
			//page.Length = 0;

			//// Find set bits until we scan all bits or fill the page
			//int count = 0;
			//int capacity = page.Capacity;

			//int i = fromIndex;
			//for (; i < this->Capacity; ++i)
			//{
			//	if (this[i])
			//	{
			//		page[count] = i;
			//		count++;
			//		if (count == capacity) break;
			//	}
			//}

			//// Set the Page Length to the count found
			//page.Length = count;

			//// Return -1 if we finished scanning, the next start index otherwise
			//return (i == this->Capacity ? -1 : i + 1);
		}

		IndexSet^ IndexSet::None()
		{
			System::Array::Clear(this->bitVector, 0, this->bitVector->Length);
			return this;
		}

		IndexSet^ IndexSet::All(UInt32 length)
		{
			for (int i = 0; i < this->bitVector->Length; ++i)
			{
				this->bitVector[i] = ~0x0ULL;
			}

			if ((length & 63) > 0)
			{
				this->bitVector[this->bitVector->Length - 1] = (~0x0ULL) >> (64 - (length & 63));
			}

			return this;
		}

		IndexSet^ IndexSet::And(IndexSet^ other)
		{
			if (this->bitVector->Length != other->bitVector->Length) throw gcnew InvalidOperationException();

			for (int i = 0; i < this->bitVector->Length; ++i)
			{
				this->bitVector[i] &= other->bitVector[i];
			}

			return this;
		}

		IndexSet^ IndexSet::AndNot(IndexSet^ other)
		{
			if (this->bitVector->Length != other->bitVector->Length) throw gcnew InvalidOperationException();

			for (int i = 0; i < this->bitVector->Length; ++i)
			{
				this->bitVector[i] &= ~other->bitVector[i];
			}

			return this;
		}


		IndexSet^ IndexSet::Or(IndexSet^ other)
		{
			if (this->bitVector->Length != other->bitVector->Length) throw gcnew InvalidOperationException();

			for (int i = 0; i < this->bitVector->Length; ++i)
			{
				this->bitVector[i] |= other->bitVector[i];
			}

			return this;
		}

		generic <typename T>
		IndexSet^ IndexSet::And(array<T>^ values, CompareOperator op, T value)
		{
			pin_ptr<T> pValues = &values[0];
			pin_ptr<UInt64> pVector = &(this->bitVector[0]);

			if (T::typeid == System::Byte::typeid)
			{
				CompareToVector::Where((CompareOperatorN)op, BooleanOperatorN::And, (unsigned __int8*)pValues, values->Length, (unsigned char)value, pVector);
			}

			return this;
		}
	}
}