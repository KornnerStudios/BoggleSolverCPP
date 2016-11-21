#include <precompile.h>
#include <bit_vectors.h>

const uint32_t k_invalid_bit_vector_index = UINT32_MAX;


static uint32_t bit_vector_dword_section_bitmask_little_endian(
	const uint32_t start_bit_index)
{
	return UINT32_MAX << start_bit_index;
}

static uint32_t bit_vector_dword_count_zeros_for_next_bit_little_endian(
	const uint32_t bits)
{
	return trailing_zeros_count(bits);
}

uint32_t bit_vector_dword_next_bit_index(
	const uint32_t* bit_vector,
	const uint32_t bit_length,
	const uint32_t start_bit_index,
	const bool state_filter)
{
	assert(start_bit_index != k_invalid_bit_vector_index);

	uint32_t word_index, bit_offset;
	bit_vector_traits_dword::bit_cursors_from_bit_index(start_bit_index, word_index, bit_offset);

	// get a mask for the the bits that start at bit_offset, thus ignoring bits that came before startBitIndex
	auto bitmask = bit_vector_dword_section_bitmask_little_endian(bit_offset);

	uint32_t result_bit_index = k_invalid_bit_vector_index;
	auto word = bit_vector[word_index];
	for ( word = (state_filter == false ? ~word : word) & bitmask
		; result_bit_index==k_invalid_bit_vector_index
		; word = state_filter == false ? ~bit_vector[word_index] : bit_vector[word_index])
	{
		// word will be 0 if it contains bits that are NOT stateFilter, thus we want to ignore such elements.
		// count the number of zeros (representing bits in the undesired state) leading up to the bit with 
		// the desired state, then add the the index in which it appears at within the overall BitSet
		if (word != 0)
			result_bit_index = bit_vector_dword_count_zeros_for_next_bit_little_endian(word) + (word_index * BIT_COUNT(word));

		// I perform the increment and loop condition here to keep the for() statement simple
		if (++word_index == bit_length)
			break;
	}

	// If we didn't find a next bit, result will be UINT32_MAX and thus greater than Length, which is desired behavior
	// else, the result is a valid index of the next bit with the desired state
	return result_bit_index >= bit_length
		? k_invalid_bit_vector_index
		: result_bit_index;
}

c_bit_vector_dword_bit_iterator::c_bit_vector_dword_bit_iterator(
	const uint32_t* bit_vector,
	const uint32_t bit_vector_length)
	: m_bit_vector(bit_vector)
	, m_bit_vector_length_minus_one(bit_vector_length == 0 ? 0 : bit_vector_length-1)
	, m_bit_index(k_invalid_bit_vector_index)
{
	assert(bit_vector_length != k_invalid_bit_vector_index);
}

bool c_bit_vector_dword_bit_iterator::next()
{
	if (m_bit_index < m_bit_vector_length_minus_one)
	{
		++m_bit_index;
		return true;
	}

	m_bit_index = m_bit_vector_length_minus_one + 1;
	return false;
}

c_bit_vector_dword_bit_filtered_iterator::c_bit_vector_dword_bit_filtered_iterator(
	const uint32_t* bit_vector,
	const uint32_t bit_vector_length,
	const uint32_t start_bit_index,
	const bool state_filter)
	: m_bit_vector(bit_vector)
	, m_bit_vector_length_minus_one(bit_vector_length == 0 ? 0 : bit_vector_length - 1)
	, m_bit_index(k_invalid_bit_vector_index)
	, m_start_bit_index(start_bit_index-1)
	, m_state_filter(state_filter)
{
	assert(bit_vector_length != k_invalid_bit_vector_index);
}

bool c_bit_vector_dword_bit_filtered_iterator::next()
{
	if (m_bit_index == k_invalid_bit_vector_index)
		m_bit_index = m_start_bit_index;

	if (m_bit_index < m_bit_vector_length_minus_one ||
		m_bit_index == k_invalid_bit_vector_index)
	{
		uint32_t next_index = bit_vector_dword_next_bit_index(m_bit_vector, m_bit_vector_length_minus_one + 1, ++m_bit_index, m_state_filter);

		if (next_index != k_invalid_bit_vector_index)
		{
			m_bit_index = next_index;
			return true;
		}
	}

	m_bit_index = m_bit_vector_length_minus_one;
	return false;
}

