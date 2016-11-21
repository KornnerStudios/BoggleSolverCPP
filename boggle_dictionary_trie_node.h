#pragma once

#include <boggle_grid_char.h>
#include <utilities.h>

class c_boggle_dictionary_trie;

struct s_boggle_dictionary_trie_node
{
	typedef int32_t node_index_t;

	node_index_t parent_node_index;
	boggle_grid_char_t grid_char;

	int completed_word_index;
	boggle_grid_char_flags_t valid_child_node_indices_flags;
	std::array<node_index_t, k_number_of_boggle_grid_characters> child_node_indices;

	void initialize(
		const boggle_grid_char_t grid_char = k_invalid_boggle_grid_char);

	bool is_root() const
	{
		return parent_node_index == -1;
	}

	uint32_t get_immediate_child_count() const
	{
		return count_number_of_1s_bits(valid_child_node_indices_flags);
	}

	bool contains_immediate_child_grid_char(
		const boggle_grid_char_t grid_char) const
	{
		return grid_char != k_invalid_boggle_grid_char
			&& TEST_FLAG(valid_child_node_indices_flags, grid_char);
	}

	boggle_grid_char_t get_first_grid_char() const
	{
		return static_cast<boggle_grid_char_t>(index_of_lowest_bit_set(valid_child_node_indices_flags));
	}

	boggle_grid_char_t get_last_grid_char() const
	{
		return static_cast<boggle_grid_char_t>(index_of_highest_bit_set(valid_child_node_indices_flags));
	}
};

