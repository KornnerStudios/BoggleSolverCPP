#pragma once

#include <cassert>

#include <boggle_grid_char.h>
#include <utilities.h>

struct s_boggle_dictionary_trie_node;
class c_boggle_dictionary_compact_trie;

struct s_boggle_dictionary_compact_trie_node
{
	enum
	{
		_private_flags_word_found_bit,
		k_number_of_private_flags,

		// english.txt contains 400+ words that result in 112878 nodes the last time
		// this comment was updated. Meaning we only need 17 bits to represent an
		// unsigned relative index. Using 18 just to try and handle anything crazy Bungie throws at this
		k_relative_node_index_bit_count = 18,
		k_relative_node_index_shift = k_number_of_private_flags,
		k_relative_node_index_bit_mask = MASK(k_relative_node_index_bit_count),

		k_private_flags_grid_char_shift = 24,
		k_private_flags_grid_char_mask = MASK(k_number_of_boggle_grid_characters),
	};

	// parent is always >=1 more nodes behind 'this'
	// children are always >=1 more nodes beyond 'this'
	typedef uint32_t relative_node_index_t;
	typedef uint32_t raw_relative_node_index_t;
	typedef raw_relative_node_index_t private_flags_t;
	static const relative_node_index_t k_invalid_relative_node_index = 0;

	typedef int32_t child_node_indices_offset_t;

	raw_relative_node_index_t private_flags;
	child_node_indices_offset_t child_node_indices_offset;
	boggle_grid_char_flags_t valid_child_node_indices_flags;
	int completed_word_index;

	void initialize(
		const s_boggle_dictionary_trie_node* src);

	bool is_word_found() const
	{
		return TEST_FLAG(private_flags, _private_flags_word_found_bit);
	}

	void set_word_found(
		const bool found)
	{
		if (completed_word_index >= 0)
		{
			SET_FLAG(private_flags, _private_flags_word_found_bit, found);
		}
	}

	raw_relative_node_index_t get_parent_node_index() const
	{
		private_flags_t parent_node_index_bits = private_flags;
		parent_node_index_bits >>= k_relative_node_index_shift;
		parent_node_index_bits &= k_relative_node_index_bit_mask;
		return static_cast<raw_relative_node_index_t>(parent_node_index_bits);
	}

	void set_parent_node_index(
		const raw_relative_node_index_t parent_node_index)
	{
		// less than OR equal to as 0 is the invalid sentinel, not the max value
		assert(parent_node_index<=k_relative_node_index_bit_mask ||
			!"try to increase k_relative_node_index_bit_count");

		private_flags_t parent_node_index_bits = parent_node_index;
		parent_node_index_bits &= k_relative_node_index_bit_mask;
		parent_node_index_bits <<= k_relative_node_index_shift;
		this->private_flags |= parent_node_index_bits;
	}

	boggle_grid_char_t get_grid_char() const
	{
		private_flags_t grid_char_bits = private_flags;
		grid_char_bits >>= k_private_flags_grid_char_shift;
		grid_char_bits &= k_private_flags_grid_char_mask;
		return static_cast<boggle_grid_char_t>(grid_char_bits);
	}

	void set_grid_char(
		const boggle_grid_char_t grid_char)
	{
		private_flags_t grid_char_bits = grid_char;
		grid_char_bits &= k_private_flags_grid_char_mask;
		grid_char_bits <<= k_private_flags_grid_char_shift;
		this->private_flags |= grid_char_bits;
	}

	// parent is always >=1 more nodes behind 'this'
	s_boggle_dictionary_compact_trie_node* get_parent_node()
	{
		auto parent_node_index = get_parent_node_index();
		return parent_node_index != k_invalid_relative_node_index
			? (this - parent_node_index)
			: nullptr;
	}

	const s_boggle_dictionary_compact_trie_node* get_parent_node() const
	{
		auto parent_node_index = get_parent_node_index();
		return parent_node_index != k_invalid_relative_node_index
			? (this - parent_node_index)
			: nullptr;
	}

	bool is_root() const
	{
		return get_parent_node_index() == k_invalid_relative_node_index;
	}

	bool is_leaf() const
	{
		return valid_child_node_indices_flags == 0;
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

