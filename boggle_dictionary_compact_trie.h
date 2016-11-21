#pragma once

#include <array>
#include <string>
#include <vector>

#include <boggle_dictionary_compact_trie_node.h>

struct _iobuf;

class c_boggle_dictionary;
class c_boggle_dictionary_trie;

class c_boggle_dictionary_compact_trie
{
	enum
	{
		k_file_data_signature = 'trie',
		k_file_data_version = 1,
	};

	typedef s_boggle_dictionary_compact_trie_node::relative_node_index_t relative_node_index_t;

	uint32_t m_nodes_count;
	uint32_t m_child_node_indices_count;
	uint32_t m_root_indices_actual_count;
	s_boggle_dictionary_compact_trie_node* m_nodes;
	relative_node_index_t* m_child_node_indices;
	std::array<int, k_number_of_boggle_grid_characters> m_root_indices;

	const c_boggle_dictionary* m_source_dictionary;
	boggle_grid_char_flags_t m_grid_chars_on_grid;
	uint32_t m_word_count;

private:
	static size_t g_debug_largest_relative_parent_node_index;
	relative_node_index_t get_relative_parent_node_index(
		const s_boggle_dictionary_compact_trie_node* this_node,
		const s_boggle_dictionary_compact_trie_node* parent_node);

	static size_t g_debug_largest_relative_child_node_index;
	relative_node_index_t get_relative_child_node_index(
		const s_boggle_dictionary_compact_trie_node* this_node,
		const s_boggle_dictionary_compact_trie_node* child_node);

	void dump(
		std::vector<std::string>& all_words,
		std::vector<char>& chars,
		const s_boggle_dictionary_compact_trie_node* node) const;

	void delete_nodes_memory();

public:
	// for binary file purposes only
	c_boggle_dictionary_compact_trie();

	c_boggle_dictionary_compact_trie(
		const c_boggle_dictionary_trie& source_trie);
	~c_boggle_dictionary_compact_trie();

	size_t estimate_total_memory_used() const;

	int get_index_of_node_unsafe(
		const s_boggle_dictionary_compact_trie_node* node) const;

	int get_index_of_node(
		const s_boggle_dictionary_compact_trie_node* node) const;

	s_boggle_dictionary_compact_trie_node* get_node(
		const int node_index);
	const s_boggle_dictionary_compact_trie_node* get_node(
		const int node_index) const;

	const s_boggle_dictionary_compact_trie_node* try_get_node(
		const int node_index) const
	{
		return node_index != -1
			? get_node(node_index)
			: nullptr;
	}

	bool build(
		const c_boggle_dictionary_trie& source_trie);

	void dump(
		std::vector<std::string>& all_words) const;

	const relative_node_index_t* get_child_nodes_pointer(
		const s_boggle_dictionary_compact_trie_node* node) const;

	int get_child_node_index(
		const s_boggle_dictionary_compact_trie_node* node,
		const boggle_grid_char_t grid_char);

	void revert_runtime_changes();

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

	uint32_t get_node_count() const
	{
		return m_nodes_count;
	}

	uint32_t get_word_count() const
	{
		return m_word_count;
	}

	bool write_to_file(
		_iobuf* file) const;

	bool read_from_file(
		_iobuf* file,
		const c_boggle_dictionary* source_dictionary);
};

class c_boggle_dictionary_compact_trie_node_child_nodes_iterator
{
	const s_boggle_dictionary_compact_trie_node* m_node;
	const s_boggle_dictionary_compact_trie_node::relative_node_index_t* m_child_nodes_indices_pointer;
	boggle_grid_char_t m_first_child_grid_char;
	boggle_grid_char_t m_last_child_grid_char;
	boggle_grid_char_t m_child_grid_char;
	s_boggle_dictionary_compact_trie_node::relative_node_index_t m_child_node_index;

public:
	c_boggle_dictionary_compact_trie_node_child_nodes_iterator(
		const c_boggle_dictionary_compact_trie* trie,
		const s_boggle_dictionary_compact_trie_node* node);

	bool next();

	boggle_grid_char_t get_child_grid_char() const
	{
		return m_child_grid_char;
	}

	const s_boggle_dictionary_compact_trie_node* get_child_node() const
	{
		return m_child_node_index != s_boggle_dictionary_compact_trie_node::k_invalid_relative_node_index
			? (m_node + m_child_node_index)
			: nullptr;
	}
};

