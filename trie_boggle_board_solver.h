#pragma once

#include <inttypes.h>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>

#include <boggle_grid_cell.h>
#include <boggle_grid_char.h>

class c_boggle_dictionary;
class c_boggle_dictionary_compact_trie;
class c_boggle_grid;

class c_trie_boggle_board_solver;

class c_trie_boggle_board_root_character_solver
{
	friend struct s_solver_stack_data;

	c_trie_boggle_board_solver& m_parent;
	const c_boggle_grid& m_grid;
	c_boggle_dictionary_compact_trie& m_trie;

	boggle_grid_char_t m_root_grid_char;
	uint32_t m_used_cells_flags_length;
	uint32_t* m_used_cells_flags;

private:
	bool is_cell_in_use(
		const uint32_t cell_index) const;

	void mark_cell_in_use(
		const uint32_t cell_index,
		const bool in_use);

	void clear_cells_in_use();

	int32_t get_root_trie_node_index() const;

	void solve_recursive(
		const int trie_node_index,
		const boggle_grid_cell_index_t cell_index);

	void solve_nonrecursive(
		const int start_trie_node_index,
		const boggle_grid_cell_index_t start_cell_index);

public:
	c_trie_boggle_board_root_character_solver(
		c_trie_boggle_board_solver& parent,
		const boggle_grid_char_t root_grid_char);

	static size_t estimate_total_memory_needed(
		const c_boggle_grid& m_grid);

	void solve_words();
};

class c_trie_boggle_board_solver
{
	struct s_root_info
	{
		boggle_grid_char_t root_grid_char;
	};

	c_boggle_dictionary_compact_trie& m_trie;
	const c_boggle_grid& m_grid;
	std::vector<std::string>& m_found_words;
	std::atomic_bool m_adding_to_found_words;
	int32_t m_roots_info_count;
	std::array<s_root_info, k_number_of_boggle_grid_characters> m_roots_info;
	uint32_t* m_found_words_flags;
	std::chrono::duration<int64_t, std::nano> m_total_time;

private:
	void solve_root_character(
		const boggle_grid_char_t root_grid_char);

	void solve_root_characters_threaded();

	void solve_root_characters_nonthreaded();

public:
	c_trie_boggle_board_solver(
		c_boggle_dictionary_compact_trie& trie,
		const c_boggle_grid& grid,
		std::vector<std::string>& found_words);

	size_t estimate_total_memory_used() const;

	void solve_board();

	void handle_solved_word(
		const int word_index,
		const int grid_cell_index);

	c_boggle_dictionary_compact_trie& get_dictionary_trie()
	{
		return m_trie;
	}

	const c_boggle_dictionary* get_dictionary() const;

	const c_boggle_grid& get_grid() const
	{
		return m_grid;
	}

	std::chrono::duration<int64_t, std::nano> get_time_spent_solving()
	{
		return m_total_time;
	}
};

