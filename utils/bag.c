#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "bag.h"

#ifndef DEBUG
#define NDEBUG
#endif
#include <assert.h>


#ifdef DEBUG
	#define BAG_INVARIANTS(bag) \
		do { \
			if (bag != NULL) { \
				assert(bag->entries != NULL); \
				assert(bag->used != NULL); \
				assert(bag->size <= bag->allocated_size); \
			} \
		} while (false)
#else
	#define BAG_INVARIANTS(bag) do {} while (false)
#endif


// how many entries to allocate space for initially
#define BAG_ALLOC_START 16

typedef uint_fast8_t bitmap_entry_t;

struct _Bag {
	bool thread_safe; // NOTE: this must not be changed once Bag_new returns
	void ** entries; // sparse array of pointers to entries
	bitmap_entry_t * used; // bitmap of which pointers in entries are filled in
	bool last_realloc_automatic; // whether the last realloc was automatic or manual (Bag_reserve)
	size_t allocated_size;
	size_t size;
	pthread_rwlock_t lock;
};


inline static size_t bitmap_size(size_t num_entries)
{
	return (num_entries / (8*sizeof(bitmap_entry_t))) +
		((num_entries % (8*sizeof(bitmap_entry_t))) == 0 ? 0 : 1);
}

inline static size_t _bitmap_index(size_t index)
{
	return index / (8*sizeof(bitmap_entry_t));
}

inline static bitmap_entry_t _bitmap_mask(size_t index)
{
	return (1 << (index % (8*sizeof(bitmap_entry_t))));
}

inline static bool bitmap_get(bitmap_entry_t * bitmap, size_t index)
{
	assert(bitmap != NULL);
	return bitmap[_bitmap_index(index)] & _bitmap_mask(index);
}

inline static void bitmap_set(bitmap_entry_t * bitmap, size_t index)
{
	assert(bitmap != NULL);
	bitmap[_bitmap_index(index)] |= _bitmap_mask(index);
}

inline static void bitmap_clear(bitmap_entry_t * bitmap, size_t index)
{
	assert(bitmap != NULL);
	bitmap[_bitmap_index(index)] &= ~_bitmap_mask(index);
}


Bag * Bag_new(bool thread_safe)
{
	Bag * bag = calloc(1, sizeof(Bag));
	if (bag == NULL)
		return NULL;

	bag->thread_safe = thread_safe;

	bag->entries = calloc(BAG_ALLOC_START, sizeof(void *));
	if (bag->entries == NULL)
	{
		free(bag);
		return NULL;
	}

	bag->used = calloc(bitmap_size(BAG_ALLOC_START), sizeof(bitmap_entry_t));
	if (bag->used == NULL)
	{
		free(bag->entries);
		free(bag);
		return NULL;
	}

	bag->last_realloc_automatic = true;
	bag->allocated_size = BAG_ALLOC_START;
	bag->size = 0;

	if (bag->thread_safe)
	{
		if (pthread_rwlock_init(&bag->lock, NULL) != 0)
		{
			free(bag->used);
			free(bag->entries);
			free(bag);
			return NULL;
		}
	}

	BAG_INVARIANTS(bag);

	return bag;
}

void Bag_free(Bag * bag)
{
	if (bag == NULL)
		return;

	BAG_INVARIANTS(bag);

	assert(bag->size == 0);

	free(bag->entries);

	free(bag->used);

	if (bag->thread_safe)
		pthread_rwlock_destroy(&bag->lock); // TODO: check return code, maybe

	free(bag);
}

static void Bag_rdlock(Bag * bag)
{
	assert(bag != NULL);

	if (bag->thread_safe)
		pthread_rwlock_rdlock(&bag->lock); // TODO: check return code

	BAG_INVARIANTS(bag);
}

static void Bag_wrlock(Bag * bag)
{
	assert(bag != NULL);

	if (bag->thread_safe)
		pthread_rwlock_wrlock(&bag->lock); // TODO: check return code

	BAG_INVARIANTS(bag);
}

static void Bag_unlock(Bag * bag)
{
	assert(bag != NULL);

	BAG_INVARIANTS(bag);

	if (bag->thread_safe)
		pthread_rwlock_unlock(&bag->lock); // TODO: check return code
}

size_t Bag_size(Bag * bag)
{
	size_t size;

	assert(bag != NULL);

	Bag_rdlock(bag);

	size = bag->size;

	Bag_unlock(bag);

	return size;
}

/**
	Reallocate the bag to support num_entries entries.

	NOTE: you MUST hold the lock for writing when calling this.
*/
static bool Bag_realloc(Bag * bag, size_t num_entries, bool automatic)
{
	assert(bag != NULL);

	BAG_INVARIANTS(bag);

	assert(num_entries >= bag->size);

	if (!bag->last_realloc_automatic && num_entries <= bag->allocated_size)
	{
		BAG_INVARIANTS(bag);
		return true; // don't clobber manual calls to bag_reserve
	}

	if (num_entries < bag->allocated_size / 4 && bag->allocated_size > BAG_ALLOC_START)
	{
		size_t shrink_to = bag->allocated_size / 2;
		if (shrink_to < BAG_ALLOC_START) shrink_to = BAG_ALLOC_START;

		assert(shrink_to < bag->allocated_size);
		assert(shrink_to >= BAG_ALLOC_START);
		assert(shrink_to > num_entries);
		assert(shrink_to > bag->size);

		void ** new_entries = malloc(shrink_to * sizeof(void*));
		if (new_entries == NULL)
		{
			BAG_INVARIANTS(bag);
			return false;
		}

		bitmap_entry_t * new_used = calloc(bitmap_size(shrink_to), sizeof(bitmap_entry_t));
		if (new_used == NULL)
		{
			free(new_entries);
			BAG_INVARIANTS(bag);
			return false;
		}

		for (size_t new_index = 0, index = 0; index < bag->allocated_size; ++index)
		{
			if (bitmap_get(bag->used, index))
			{
				new_entries[new_index] = bag->entries[index];
				bitmap_set(new_used, new_index);
				++new_index;
			}
		}

		free(bag->entries);
		free(bag->used);
		bag->entries = new_entries;
		bag->used = new_used;
		bag->last_realloc_automatic = automatic;
		bag->allocated_size = shrink_to;

		BAG_INVARIANTS(bag);
		return true;
	}
	else if (num_entries > bag->allocated_size)
	{
		size_t grow_to = bag->allocated_size;
		while (grow_to < num_entries) grow_to *= 2;

		assert(grow_to > bag->allocated_size);
		assert(grow_to > BAG_ALLOC_START);
		assert(grow_to >= num_entries);
		assert(grow_to > bag->size);

		void ** new_entries = realloc(bag->entries, grow_to * sizeof(void*));
		if (new_entries == NULL)
		{
			BAG_INVARIANTS(bag);
			return false;
		}
		else
		{
			bag->entries = new_entries;
		}

		assert(bitmap_size(grow_to) >= bitmap_size(bag->allocated_size));
		if (bitmap_size(grow_to) > bitmap_size(bag->allocated_size))
		{
			bitmap_entry_t * new_used = realloc(bag->used, bitmap_size(grow_to) * sizeof(bitmap_entry_t));
			if (new_used == NULL)
			{
				BAG_INVARIANTS(bag);
				return false;
			}
			else
			{
				bag->used = new_used;
				memset((void *)(bag->used + bitmap_size(bag->allocated_size)),
					0,
					sizeof(bitmap_entry_t) * (bitmap_size(grow_to) - bitmap_size(bag->allocated_size)));
			}
		}

		bag->last_realloc_automatic = automatic;
		bag->allocated_size = grow_to;

		BAG_INVARIANTS(bag);
		return true;
	}

	BAG_INVARIANTS(bag);
	return true;
}

bool Bag_reserve(Bag * bag, size_t num_entries)
{
	assert(bag != NULL);

	Bag_wrlock(bag);

	if (num_entries <= bag->allocated_size)
	{
		Bag_unlock(bag);
		return true;
	}

	bool ret = Bag_realloc(bag, num_entries, false);

	Bag_unlock(bag);

	return ret;
}
