#pragma once

#include <atomic>
#include <string>
#include <vector>

struct _iobuf;

class c_boggle_dictionary;
class c_boggle_dictionary_compact_trie;
class c_boggle_grid;

// Solves a boggle board using a trie representation of legal ASCII words.
// Given that dictionaries don't often change, the internals support
// saving the dictionary to a binary file which includes the optimized
// trie.

class c_boggle
{
	c_boggle_dictionary* m_dictionary;
	c_boggle_dictionary_compact_trie* m_dictionary_compact_trie;
	std::atomic_bool m_currently_solving_board;
	// should we filter our view of the dictionary using only words
	// that contain only characters that also appear on input grids?
	bool m_filter_dictionary_with_occuring_grid_chars;

public:
	size_t m_estimated_total_memory_used_for_dictionary_trie;
	size_t m_estimated_total_memory_used_for_dictionary_compact_trie;
	size_t m_estimated_total_memory_used_for_grid;
	size_t m_estimated_total_memory_used_for_solver;

private:
	void deallocate_dictionary();

	void deallocate_dictionary_trie();

	bool build_dictionary_trie(
		// really a boggle_grid_char_flags_t, but not trying to force
		// the more private headers to get included just to satisfy private APIs
		const uint32_t occuring_grid_chars_flags);

	bool build_dictionary_trie(
		const c_boggle_grid& grid);

	bool load_binary_legal_words(
		_iobuf* file,
		const char* filename);
	bool save_binary_legal_words(
		_iobuf* file,
		const char* filename);

public:
	c_boggle();
	~c_boggle();

	// prior to solving any board, configure the legal words
	// #NOTE given that this is a public API and I can't guarantee the lifetime of all_words
	// we build our own private representation of all_words (c_boggle_dictionary)
	void set_legal_words_from_alphabetically_sorted_array(
		// alphabetically-sorted array of legal words
		const std::vector<std::string>& all_words);

	// try and load the optimized representation of legal words from an existing binary file
	bool load_binary_legal_words(
		const char* filename);
	// save the optimized representation of legal words to a binary file
	bool save_binary_legal_words(
		const char* filename);

	// find all words on the specified board, returning a list of them
	bool solve_board(
		std::vector<std::string>& found_words,
		// width of the board, e.g. 4 for a retail Boggle game
		const int board_width,
		// height of the board, e.g. 4 for a retail Boggle game
		const int board_height,
		// board_width*board_height characters in row major order
		const char* board_letters);

	// find all words on the specified board, returning a list of them
	std::vector<std::string> solve_board(
		// width of the board, e.g. 4 for a retail Boggle game
		const int board_width,
		// height of the board, e.g. 4 for a retail Boggle game
		const int board_height,
		// board_width*board_height characters in row major order
		const char* board_letters);

	const c_boggle_dictionary* get_dictionary() const
	{
		return m_dictionary;
	}

	const c_boggle_dictionary_compact_trie* get_dictionary_trie() const
	{
		return m_dictionary_compact_trie;
	}

	bool is_filtering_dictionary_with_occuring_grid_chars() const
	{
		return m_filter_dictionary_with_occuring_grid_chars;
	}

	// filter won't actually be respected until the next time solve_board is called
	void set_filter_dictionary_with_occuring_grid_chars(
		const bool filter);
};

