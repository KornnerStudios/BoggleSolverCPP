#pragma once

#include <boggle_grid_char.h>
#include <utilities.h>

class c_boggle_dictionary;

struct s_boggle_dictionary_word
{
	enum
	{
		k_contains_qu_flag = 0x8000,
		k_length_with_qu_as_one_grid_char_mask = 0x7FFF,
	};

	uint16_t private_flags;
	uint16_t length;
	uint32_t string_pool_offset;
	boggle_grid_char_flags_t grid_chars_in_word_flags;

	void initialize(
		const uint32_t length,
		const uint32_t length_with_qu_as_one_grid_char,
		const uint32_t string_pool_offset,
		const bool contains_qu,
		const boggle_grid_char_flags_t grid_chars_in_word_flags);

	const char* get_string(
		const c_boggle_dictionary* parent) const;

	uint32_t get_length() const
	{
		return length;
	}

	uint32_t get_length_with_qu_as_one_grid_char() const
	{
		return private_flags & k_length_with_qu_as_one_grid_char_mask;
	}

	bool contains_qu() const
	{
		return (private_flags & k_contains_qu_flag) != 0;
	}

	bool contains_grid_char(
		const boggle_grid_char_t grid_char) const
	{
		return grid_char != k_invalid_boggle_grid_char
			&& test_bit(grid_chars_in_word_flags, grid_char);
	}
};

