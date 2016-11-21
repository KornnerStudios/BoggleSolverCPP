#pragma once

#include <inttypes.h>
#include <cassert>

#include <utilities.h>

extern const uint32_t k_invalid_bit_vector_index;

template<typename TWord>
struct bit_vector_traits
{
	enum : uint32_t
	{
		k_word_size = sizeof TWord,
		k_bit_count = BIT_COUNT(TWord),
		k_bit_count_log2 = compile_time_log2(k_bit_count),
	};

	template<size_t k_bit_length>
	struct size_in_words 
	{ enum {
		value = (k_bit_length + (k_bit_count-1)) >> k_bit_count_log2
	}; };

	static size_t get_size_in_words(
		const size_t bit_length)
	{
		return (bit_length + (k_bit_count-1)) >> k_bit_count_log2;
	}

	static uint32_t bit_index_to_word_index(
		const uint32_t bit_index)
	{
		return bit_index >> k_bit_count_log2;
	}

	static void bit_cursors_from_bit_index(
		const uint32_t bit_index,
		uint32_t& out_word_index,
		uint32_t& out_bit_offset)
	{
		out_word_index = bit_index_to_word_index(bit_index);
		out_bit_offset = bit_index & (k_bit_count-1);
	}

	static bool test_flag(
		const TWord* vector,
		const uint32_t bit_index)
	{
		uint32_t word_index, bit_offset;
		bit_cursors_from_bit_index(bit_index, word_index, bit_offset);

		TWord flag = static_cast<TWord>(1U) << bit_offset;

		return (vector[word_index] & flag) != 0;
	}

	static bool set_flag(
		TWord* vector,
		const uint32_t bit_index,
		const bool value)
	{
		uint32_t word_index, bit_offset;
		bit_cursors_from_bit_index(bit_index, word_index, bit_offset);

		TWord flag = static_cast<TWord>(1U) << bit_offset;

		value
			? vector[word_index] |=  flag
			: vector[word_index] &= ~flag;

		return value;
	}
};

typedef bit_vector_traits<uint32_t> bit_vector_traits_dword;

// How many 32 bit integers it takes to hold a bit vector with [size] bits
#define BIT_VECTOR_SIZE_IN_DWORDS(size)			( bit_vector_traits_dword::size_in_words<size>::value )
// How many total bits are in the supplied bit vector size and type container
#define BIT_VECTOR_SIZE_IN_BITS(size, type)		( (size) * BIT_COUNT(type) )
#define BIT_VECTOR_TEST_FLAG32(vector, bit)			bit_vector_traits_dword::test_flag(reinterpret_cast<const uint32_t*>(vector), bit)
#define BIT_VECTOR_SET_FLAG32(vector, bit, value)	bit_vector_traits_dword::set_flag (reinterpret_cast<      uint32_t*>(vector), bit, value)

uint32_t bit_vector_dword_next_bit_index(
	const uint32_t* bit_vector,
	const uint32_t bit_length,
	const uint32_t start_bit_index,
	const bool state_filter);

struct c_bit_vector_dword_bit_iterator
{
	const uint32_t* m_bit_vector;
	const uint32_t m_bit_vector_length_minus_one;
	uint32_t m_bit_index;
public:
	c_bit_vector_dword_bit_iterator(
		const uint32_t* bit_vector,
		const uint32_t bit_vector_length);

	bool next();

	bool get_bit() const
	{
		assert(m_bit_index != k_invalid_bit_vector_index);
		assert(m_bit_index <= m_bit_vector_length_minus_one);
		return BIT_VECTOR_TEST_FLAG32(m_bit_vector, m_bit_index);
	}

	uint32_t get_bit_index() const
	{
		assert(m_bit_index != k_invalid_bit_vector_index);
		return m_bit_index;
	}
};

struct c_bit_vector_dword_bit_filtered_iterator
{
	const uint32_t* m_bit_vector;
	const uint32_t m_bit_vector_length_minus_one;
	uint32_t m_bit_index;
	const uint32_t m_start_bit_index;
	const bool m_state_filter;
public:
	c_bit_vector_dword_bit_filtered_iterator(
		const uint32_t* bit_vector,
		const uint32_t bit_vector_length,
		const uint32_t start_bit_index = 0,
		const bool state_filter = true);

	bool next();

	bool get_bit() const
	{
		return m_state_filter;
	}

	uint32_t get_bit_index() const
	{
		assert(m_bit_index != k_invalid_bit_vector_index);
		return m_bit_index;
	}
};

