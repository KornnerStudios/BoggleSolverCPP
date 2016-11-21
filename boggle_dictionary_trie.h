#pragma once

#include <array>
#include <string>
#include <vector>

#include <boggle_dictionary_trie_node.h>

struct _iobuf;

struct s_boggle_dictionary_word;
class c_boggle_dictionary;
class c_boggle_dictionary_compact_trie;

class c_boggle_dictionary_trie
{
	friend class c_boggle_dictionary_compact_trie;

	struct s_add_word_state
	{
		const s_boggle_dictionary_word* prev_word;
		int prev_word_last_node_index;
	};

	const c_boggle_dictionary* m_source_dictionary;
	const boggle_grid_char_flags_t m_grid_chars_on_grid;
	uint32_t m_root_indices_actual_count;
	std::array<int, k_number_of_boggle_grid_characters> m_root_indices;
	std::vector<s_boggle_dictionary_trie_node> m_nodes;
	uint32_t m_word_count;

private:
	static uint32_t get_matching_prefix_string_length(
		const char* prev_word_string,
		const char* curr_word_string);

	int get_nth_parent_node_index(
		const int node_index,
		const uint32_t n) const;

	void add_word(
		s_add_word_state& add_state,
		const s_boggle_dictionary_word& word,
		const size_t word_index);

	int add_node(
		const boggle_grid_char_t grid_char);

	s_boggle_dictionary_trie_node* get_node(
		const int node_index);
	const s_boggle_dictionary_trie_node* get_node(
		const int node_index) const;

	const s_boggle_dictionary_trie_node* try_get_node(
		const int node_index) const
	{
		return node_index != -1
			? get_node(node_index)
			: nullptr;
	}

	void dump(
		std::vector<std::string>& all_words,
		std::vector<char>& chars,
		const int cursor_node_index) const;

public:
	c_boggle_dictionary_trie(
		const c_boggle_dictionary* source_dictionary,
		const boggle_grid_char_flags_t grid_chars_on_grid);

	size_t estimate_total_memory_used() const;

	static uint32_t estimated_node_count(
		const uint32_t dictionary_word_count,
		const uint32_t dictionary_avg_word_length);

	bool build();

	void dump(
		std::vector<std::string>& all_words) const;

	int get_or_add_root_node_index(
		const boggle_grid_char_t grid_char);

	int get_index_of_node(
		const s_boggle_dictionary_trie_node* node) const;

	s_boggle_dictionary_trie_node* get_node_child(
		const s_boggle_dictionary_trie_node* node,
		const boggle_grid_char_t grid_char);

	void set_node_child(
		const int node_index,
		const boggle_grid_char_t grid_char,
		const int child_node_index);

	int get_or_add_child_node_index(
		const int node_index,
		const boggle_grid_char_t grid_char);

	const c_boggle_dictionary* get_dictionary() const
	{
		return m_source_dictionary;
	}

	boggle_grid_char_flags_t get_occuring_grid_chars_flags() const
	{
		return m_grid_chars_on_grid;
	}

	uint32_t get_root_indices_actual_count() const
	{
		return m_root_indices_actual_count;
	}

	const std::array<int, k_number_of_boggle_grid_characters>& get_root_indices() const
	{
		return m_root_indices;
	}

	size_t get_node_count() const
	{
		return m_nodes.size();
	}

	uint32_t get_word_count() const
	{
		return m_word_count;
	}
};

