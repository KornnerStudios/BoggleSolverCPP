#include <precompile.h>
#include <trie_boggle_board_solver.h>

#include <bit_vectors.h>
#include <boggle_dictionary.h>
#include <boggle_dictionary_compact_trie.h>
#include <boggle_grid.h>
#include <utilities.h>

c_trie_boggle_board_root_character_solver::c_trie_boggle_board_root_character_solver(
	c_trie_boggle_board_solver& parent,
	const boggle_grid_char_t root_grid_char)
	: m_parent(parent)
	, m_grid(parent.get_grid())
	, m_trie(parent.get_dictionary_trie())
	, m_root_grid_char(root_grid_char)
	, m_used_cells_flags_length(0)
	, m_used_cells_flags(nullptr)
{
}

size_t c_trie_boggle_board_root_character_solver::estimate_total_memory_needed(
	const c_boggle_grid& m_grid)
{
	size_t estimated_total_memory_used = sizeof c_trie_boggle_board_root_character_solver;
	// m_used_cells_flags
	estimated_total_memory_used += sizeof(uint32_t) * bit_vector_traits_dword::get_size_in_words(m_grid.get_cell_count());
	return estimated_total_memory_used;
}

bool c_trie_boggle_board_root_character_solver::is_cell_in_use(
	const uint32_t cell_index) const
{
	return BIT_VECTOR_TEST_FLAG32(m_used_cells_flags, cell_index);
}

void c_trie_boggle_board_root_character_solver::mark_cell_in_use(
	const uint32_t cell_index,
	const bool in_use)
{
	BIT_VECTOR_SET_FLAG32(m_used_cells_flags, cell_index, in_use);
}

void c_trie_boggle_board_root_character_solver::clear_cells_in_use()
{
	memset(m_used_cells_flags, 0,
		sizeof(m_used_cells_flags[0]) * m_used_cells_flags_length);
}

int32_t c_trie_boggle_board_root_character_solver::get_root_trie_node_index() const
{
	auto& root_indices = m_trie.get_root_indices();

	return root_indices[m_root_grid_char];
}

void c_trie_boggle_board_root_character_solver::solve_recursive(
	const int trie_node_index,
	const boggle_grid_cell_index_t cell_index)
{
	auto trie_node = m_trie.get_node(trie_node_index);
	if (trie_node->completed_word_index >= 0 && !trie_node->is_word_found())
	{
		trie_node->set_word_found(true);
		m_parent.handle_solved_word(trie_node->completed_word_index, cell_index);
	}

	auto trie_node_grid_char = trie_node->get_grid_char();

	auto cell = m_grid.get_cell(cell_index);
	// For each neighbor character surrounding our matching character...
	for (auto neighbor = _boggle_grid_cell_neighbor_iterator_begin_value; neighbor < k_number_of_boggle_grid_cell_neighbors; ++neighbor)
	{
		auto neighbor_cell_index = cell->get_neighbor_cell_index(m_grid, neighbor);
		// neighbor index is invalid, no neighbor there, try the next one...
		// or the element is already in use by a trie prefix we're trying to solve
		if (neighbor_cell_index == k_invalid_boggle_grid_cell_index ||
			is_cell_in_use(neighbor_cell_index))
		{
			continue;
		}

		// look up the next cell and any corresponding trie node
		auto neighbor_cell = m_grid.get_cell(neighbor_cell_index);
		auto trie_node_index_with_neighbor_char = m_trie.get_child_node_index(
			trie_node, neighbor_cell->grid_char);
		if (trie_node_index_with_neighbor_char != -1)
		{
			mark_cell_in_use(neighbor_cell_index, true);
			solve_recursive(trie_node_index_with_neighbor_char, neighbor_cell_index);
			mark_cell_in_use(neighbor_cell_index, false);
		}
		// SPECIAL CASE: when we encounter a 'q', act like we already encountered a 'u' element
		if (trie_node_grid_char == k_boggle_grid_char_special_case_q)
		{
			trie_node_index_with_neighbor_char = m_trie.get_child_node_index(
				trie_node, k_boggle_grid_char_special_case_u);
			if (trie_node_index_with_neighbor_char != -1)
			{
				mark_cell_in_use(neighbor_cell_index, true);
				solve_recursive(trie_node_index_with_neighbor_char, neighbor_cell_index);
				mark_cell_in_use(neighbor_cell_index, false);
			}
		}
	}
}

struct s_solver_stack_data
{
	enum e_state : uint8_t
	{
		_state_unused,
		_state_start_iterating_neighbors,
		_state_iterating_neighbors,
		_state_finished,
	};
	enum e_flags : uint8_t
	{
		_unmark_neighbor_cell_in_use_bit,
		_requires_iterating_neighbors_as_qu_bit,
		_iterated_current_neighbor_as_qu_bit,
		_try_current_neighbor_as_qu_bit,
	};

	const s_boggle_grid_cell* cell;
	s_boggle_dictionary_compact_trie_node* trie_node;
	boggle_grid_char_t trie_node_grid_char;
	e_boggle_grid_cell_neighbor neighbor;
	boggle_grid_cell_index_t neighbor_cell_index;
	e_state state;
	uint8_t flags;

	void reset()
	{
		cell = nullptr;
		trie_node = nullptr;
		trie_node_grid_char = k_invalid_boggle_grid_char;
		neighbor = _boggle_grid_cell_neighbor_iterator_begin_value;
		neighbor_cell_index = k_invalid_boggle_grid_cell_index;
		state = _state_unused;
		flags = 0;
	}

	void setup(
		const c_trie_boggle_board_root_character_solver& solver,
		const int trie_node_index,
		const boggle_grid_cell_index_t cell_index)
	{
		assert(trie_node_index != -1);
		assert(cell_index != k_invalid_boggle_grid_cell_index);
		assert(trie_node==nullptr);

		trie_node = solver.m_trie.get_node(trie_node_index);
		if (trie_node->completed_word_index >= 0 && !trie_node->is_word_found())
		{
			trie_node->set_word_found(true);
			solver.m_parent.handle_solved_word(trie_node->completed_word_index, cell_index);
		}

		trie_node_grid_char = trie_node->get_grid_char();

		cell = solver.m_grid.get_cell(cell_index);

		state = s_solver_stack_data::_state_start_iterating_neighbors;
		if (trie_node_grid_char == k_boggle_grid_char_special_case_q)
		{
			SET_FLAG(flags, _requires_iterating_neighbors_as_qu_bit, true);
		}
	}

	bool should_try_neighbor(
		const c_trie_boggle_board_root_character_solver& solver)
	{
		neighbor_cell_index = cell->get_neighbor_cell_index(solver.m_grid, neighbor);
		// neighbor index is invalid, no neighbor there, try the next one...
		// or the element is already in use by a trie prefix we're trying to solve
		if (neighbor_cell_index == k_invalid_boggle_grid_cell_index ||
			solver.is_cell_in_use(neighbor_cell_index))
		{
			return false;
		}

		return true;
	}

	bool requires_iterating_neighbors_as_qu() const
	{
		return test_bit(flags, _requires_iterating_neighbors_as_qu_bit);
	}

	bool has_iterated_current_neighbor_as_qu() const
	{
		return test_bit(flags, _iterated_current_neighbor_as_qu_bit);
	}

	bool try_current_neighbor_as_qu() const
	{
		return test_bit(flags, _try_current_neighbor_as_qu_bit);
	}

	static s_solver_stack_data unused_object()
	{
		s_solver_stack_data data;
		data.reset();

		return data;
	}
};
//static_assert(sizeof(s_solver_stack_data) == 0x18, "unexpected size for 64-bit");

void c_trie_boggle_board_root_character_solver::solve_nonrecursive(
	const int start_trie_node_index,
	const boggle_grid_cell_index_t start_cell_index)
{
	std::array<s_solver_stack_data, c_boggle_dictionary::k_longest_acceptable_word_length> stack_data;
	int stack_index = 0;

	stack_data.fill(s_solver_stack_data::unused_object());

	int next_trie_node_index = start_trie_node_index;
	boggle_grid_cell_index_t next_cell_index = start_cell_index;

	while (stack_index != -1)
	{
		if (stack_index < stack_data.max_size() - 1)
		{
			assert(stack_data[stack_index + 1].state == s_solver_stack_data::_state_unused);
		}

		auto& cursor = stack_data[stack_index];
		if (cursor.state == s_solver_stack_data::_state_unused)
		{
			cursor.setup(*this, next_trie_node_index, next_cell_index);
			next_trie_node_index = -1;
			next_cell_index = k_invalid_boggle_grid_cell_index;
		}

		// next handle how the neighbor iteration should begin or continue
		if (cursor.state == s_solver_stack_data::_state_start_iterating_neighbors)
		{
			cursor.state = s_solver_stack_data::_state_iterating_neighbors;
		}
		else if (cursor.state == s_solver_stack_data::_state_iterating_neighbors)
		{
			if (test_bit(cursor.flags, cursor._unmark_neighbor_cell_in_use_bit))
			{
				SET_FLAG(cursor.flags, cursor._unmark_neighbor_cell_in_use_bit, false);
				mark_cell_in_use(cursor.neighbor_cell_index, false);
			}

			if (cursor.requires_iterating_neighbors_as_qu() &&
				cursor.neighbor_cell_index != k_invalid_boggle_grid_cell_index)
			{
				if (cursor.has_iterated_current_neighbor_as_qu())
					++cursor.neighbor;
				else
					SET_FLAG(cursor.flags, cursor._try_current_neighbor_as_qu_bit, true);
			}
			else
			{
				++cursor.neighbor;
			}

			if (cursor.neighbor == k_number_of_boggle_grid_cell_neighbors)
			{
				cursor.state = s_solver_stack_data::_state_finished;
				cursor.reset();
				stack_index--;
				continue;
			}
		}

		assert(cursor.state == s_solver_stack_data::_state_iterating_neighbors);
		if (cursor.try_current_neighbor_as_qu() &&
			!cursor.has_iterated_current_neighbor_as_qu())
		{
		}
		else if (!cursor.should_try_neighbor(*this))
		{
			continue;
		}

		// finally, try to process the current neighbor
		if (cursor.state == s_solver_stack_data::_state_iterating_neighbors)
		{
			auto neighbor_cell = m_grid.get_cell(cursor.neighbor_cell_index);
			assert(neighbor_cell);
			int trie_node_index_with_neighbor_char;

			if (!cursor.try_current_neighbor_as_qu())
			{
				trie_node_index_with_neighbor_char = m_trie.get_child_node_index(
					cursor.trie_node, neighbor_cell->grid_char);
			}
			else
			{
				// SPECIAL CASE: when we encounter a 'q', act like we already encountered a 'u' element
				trie_node_index_with_neighbor_char = m_trie.get_child_node_index(
					cursor.trie_node, k_boggle_grid_char_special_case_u);

				// set the flag now so we don't have to do any extra handling when there's
				// no 'u' neighbors
				SET_FLAG(cursor.flags, cursor._iterated_current_neighbor_as_qu_bit, true);
				SET_FLAG(cursor.flags, cursor._try_current_neighbor_as_qu_bit, false);
			}

			if (trie_node_index_with_neighbor_char != -1)
			{
				mark_cell_in_use(cursor.neighbor_cell_index, true);
				SET_FLAG(cursor.flags, cursor._unmark_neighbor_cell_in_use_bit, true);
				next_trie_node_index = trie_node_index_with_neighbor_char;
				next_cell_index = cursor.neighbor_cell_index;
				stack_index++;
				continue;
			}
		}
	}
}

void c_trie_boggle_board_root_character_solver::solve_words()
{
	m_used_cells_flags_length = static_cast<uint32_t>( bit_vector_traits_dword::get_size_in_words(m_grid.get_cell_count()) );
	m_used_cells_flags = new uint32_t[m_used_cells_flags_length];

	clear_cells_in_use();

	auto root_trie_node_index = get_root_trie_node_index();

	boggle_grid_cell_index_t cell_index_with_root = k_invalid_boggle_grid_cell_index;
	for ( uint32_t cell_cursor = 0
		; (cell_index_with_root = m_grid.cell_index_of(m_root_grid_char, cell_cursor)) != k_invalid_boggle_grid_cell_index
		; cell_cursor = static_cast<uint32_t>(cell_index_with_root) + 1)
	{
		mark_cell_in_use(cell_index_with_root, true);

		// profiling, in x64, has shown that the non-recursive solver on avg takes longer time to complete,
		// quite possibly due to all the branching used in an effort to keep the state machine at least
		// somewhat understandable. Since I haven't yet blown the default stack limits with the
		// english dictionary, I favor the recursive implementation
#if 1
		solve_recursive(root_trie_node_index, cell_index_with_root);
#else
		solve_nonrecursive(root_trie_node_index, cell_index_with_root);
#endif

		mark_cell_in_use(cell_index_with_root, false);
	}

	delete[] m_used_cells_flags;
}

c_trie_boggle_board_solver::c_trie_boggle_board_solver(
	c_boggle_dictionary_compact_trie& trie,
	const c_boggle_grid& grid,
	std::vector<std::string>& found_words)
	: m_trie(trie)
	, m_grid(grid)
	, m_found_words(found_words)
	, m_adding_to_found_words(false)
	, m_roots_info_count(0)
	, m_found_words_flags(nullptr)
	, m_total_time()
{
	m_roots_info.fill({ k_invalid_boggle_grid_char });

	boggle_grid_char_t root_grid_char = 0;
	for (int root_trie_node_index : m_trie.get_root_indices())
	{
		if (root_trie_node_index != -1)
		{
			m_roots_info[m_roots_info_count++].root_grid_char = root_grid_char;
		}

		root_grid_char++;
	}
}

size_t c_trie_boggle_board_solver::estimate_total_memory_used() const
{
	size_t estimated_total_memory_used = sizeof(*this);
	estimated_total_memory_used += sizeof(uint32_t) * bit_vector_traits_dword::get_size_in_words(m_trie.get_dictionary()->get_words_count());

	int processor_count = omp_get_num_procs();
	size_t memory_for_one_solver = c_trie_boggle_board_root_character_solver::estimate_total_memory_needed(m_grid);
	if (processor_count > 1)
		estimated_total_memory_used += memory_for_one_solver * (processor_count - 1);
	else
		estimated_total_memory_used += memory_for_one_solver;

	return estimated_total_memory_used;
}

void c_trie_boggle_board_solver::solve_root_character(
	const boggle_grid_char_t root_grid_char)
{
	c_trie_boggle_board_root_character_solver root_solver(
		*this, root_grid_char);

	root_solver.solve_words();
}

void c_trie_boggle_board_solver::solve_root_characters_threaded()
{
#pragma omp parallel for
	for (int root_info_index = 0; root_info_index < m_roots_info_count; root_info_index++)
	{
		solve_root_character(m_roots_info[root_info_index].root_grid_char);
	}
}

void c_trie_boggle_board_solver::solve_root_characters_nonthreaded()
{
	for (int root_info_index = 0; root_info_index < m_roots_info_count; root_info_index++)
	{
		solve_root_character(m_roots_info[root_info_index].root_grid_char);
	}
}

void c_trie_boggle_board_solver::solve_board()
{
	auto dict = get_dictionary();

	uint32_t total_words_count = dict->get_words_count();
	uint32_t found_words_flags_length = static_cast<uint32_t>( bit_vector_traits_dword::get_size_in_words(total_words_count) );
	m_found_words_flags = new uint32_t[found_words_flags_length];
	memset(m_found_words_flags, 0,
		sizeof(m_found_words_flags[0]) * found_words_flags_length);

	auto start_time = std::chrono::high_resolution_clock::now();
	solve_root_characters_threaded();
	auto end_time = std::chrono::high_resolution_clock::now();

	m_total_time = end_time - start_time;

	uint32_t found_words_count = 0;
	for (uint32_t x = 0; x < found_words_flags_length; x++)
	{
		found_words_count += count_number_of_1s_bits(m_found_words_flags[x]);
	}

	m_found_words.reserve(found_words_count);

	for ( c_bit_vector_dword_bit_filtered_iterator iter(m_found_words_flags, total_words_count)
		; iter.next()
		; )
	{
		uint32_t word_index = iter.get_bit_index();
		auto word = dict->get_word(static_cast<int>(word_index));
		assert(word != nullptr);

		std::string word_string(dict->get_string(*word));
		m_found_words.push_back(word_string);
	}

	assert(m_found_words.size() == found_words_count);
	delete[] m_found_words_flags;

	m_found_words.shrink_to_fit();
}

void c_trie_boggle_board_solver::handle_solved_word(
	const int word_index,
	const int grid_cell_index)
{
	while (m_adding_to_found_words.load())
	{
	}

	m_adding_to_found_words.store(true);
	BIT_VECTOR_SET_FLAG32(m_found_words_flags, static_cast<uint32_t>(word_index), true);
	m_adding_to_found_words.store(false);
}

const c_boggle_dictionary* c_trie_boggle_board_solver::get_dictionary() const
{
	return m_trie.get_dictionary();
}

