#include <precompile.h>
#include <boggle.h>

#include <boggle_dictionary.h>
#include <boggle_dictionary_compact_trie.h>
#include <boggle_dictionary_trie.h>
#include <boggle_dictionary_word.h>
#include <boggle_grid.h>
#include <boggle_grid_cell.h>
#include <boggle_grid_cell_neighbor.h>
#include <boggle_grid_char.h>
#include <trie_boggle_board_solver.h>

struct s_boggle_dictionary_binary_file_header
{
	enum
	{
		k_signature = 'bdic',
		k_version = 1,
	};

	uint32_t signature;
	uint32_t version;

	struct
	{
		int32_t offset;
		int32_t length;
	}dictionary, dictionary_trie;
};
static_assert(sizeof(s_boggle_dictionary_binary_file_header) == 0x18,
	"Unexpected s_boggle_dictionary_binary_file_header size");

c_boggle::c_boggle()
	: m_dictionary(nullptr)
	, m_dictionary_compact_trie(nullptr)
	, m_currently_solving_board(false)
	, m_filter_dictionary_with_occuring_grid_chars(false)
	, m_estimated_total_memory_used_for_dictionary_trie(0)
	, m_estimated_total_memory_used_for_dictionary_compact_trie(0)
	, m_estimated_total_memory_used_for_grid(0)
	, m_estimated_total_memory_used_for_solver(0)
{
}

c_boggle::~c_boggle()
{
	deallocate_dictionary();
}

void c_boggle::deallocate_dictionary()
{
	deallocate_dictionary_trie();

	if (m_dictionary)
	{
		delete m_dictionary;
		m_dictionary = nullptr;
	}
}

void c_boggle::deallocate_dictionary_trie()
{
	if (m_dictionary_compact_trie)
	{
		delete m_dictionary_compact_trie;
		m_dictionary_compact_trie = nullptr;
	}
}

bool c_boggle::build_dictionary_trie(
	const uint32_t occuring_grid_chars_flags)
{
	static_assert(sizeof(occuring_grid_chars_flags) == sizeof(boggle_grid_char_flags_t),
		"This API needs to be updated");

	bool success = true;

	c_boggle_dictionary_trie dictionary_trie(m_dictionary,
		occuring_grid_chars_flags);

	success = dictionary_trie.build();
	m_estimated_total_memory_used_for_dictionary_trie = dictionary_trie.estimate_total_memory_used();

#if 0 // for figuring out the c_boggle_dictionary::estimated_node_count formula
	auto word_count = m_dictionary->get_number_of_words();
	auto avg_word_length = m_dictionary->get_average_word_length();
	size_t node_count = dictionary_trie.get_node_count();
#endif

	if (success)
	{
		m_dictionary_compact_trie = new c_boggle_dictionary_compact_trie(dictionary_trie);

		success = m_dictionary_compact_trie->build(dictionary_trie);

		m_estimated_total_memory_used_for_dictionary_compact_trie = m_dictionary_compact_trie->estimate_total_memory_used();
	}

	return success;
}

bool c_boggle::build_dictionary_trie(
	const c_boggle_grid& grid)
{
	auto occuring_grid_chars_flags = grid.get_occuring_grid_chars_flags();

	if (m_dictionary_compact_trie != nullptr)
	{
		if (m_filter_dictionary_with_occuring_grid_chars &&
			occuring_grid_chars_flags != m_dictionary_compact_trie->get_occuring_grid_chars_flags())
		{
			deallocate_dictionary_trie();
		}
	}

	bool success = true;

	if (m_dictionary_compact_trie == nullptr)
	{
		if (!m_filter_dictionary_with_occuring_grid_chars)
			occuring_grid_chars_flags = MASK(k_number_of_boggle_grid_characters);

		build_dictionary_trie(occuring_grid_chars_flags);
	}

	return success;
}

void c_boggle::set_legal_words(
	const std::vector<std::string>& all_words)
{
	if (m_currently_solving_board.load())
	{
		output_error("set_legal_words called while we're already solving a board");
		return;
	}

	deallocate_dictionary();
	m_dictionary = new c_boggle_dictionary;
	if (!m_dictionary)
	{
		output_error("set_legal_words failed to allocate dictionary");
		return;
	}

	if (!m_dictionary->set_legal_words(all_words))
	{
		output_error("set_legal_words failed to add all_words to dictionary");
		return;
	}
}

bool c_boggle::load_binary_legal_words(
	_iobuf* file,
	const char* filename)
{
	s_boggle_dictionary_binary_file_header header;

	if (1 != fread(&header, sizeof header, 1, file))
		return false;

	if (header.signature != header.k_signature ||
		header.version != header.k_version ||
		header.dictionary.offset <= 0 ||
		header.dictionary.length <= 0 ||
		header.dictionary_trie.offset <= 0 ||
		header.dictionary_trie.length <= 0)
	{
		output_error("load_binary_legal_words file has invalid header: %s",
			filename);
		return false;
	}

	deallocate_dictionary();

	if (0 != fseek(file, header.dictionary.offset, SEEK_SET))
	{
		output_error("load_binary_legal_words failed to seek to dictionary blob: %s",
			filename);
		return false;
	}

	m_dictionary = new c_boggle_dictionary;
	if (!m_dictionary)
	{
		output_error("load_binary_legal_words failed to allocate dictionary: %s",
			filename);
		return false;
	}
	if (!m_dictionary->read_from_file(file))
	{
		output_error("load_binary_legal_words failed to read dictionary blob: %s",
			filename);
		return false;
	}

	if (0 != fseek(file, header.dictionary_trie.offset, SEEK_SET))
	{
		output_error("load_binary_legal_words failed to seek to dictionary trie blob: %s",
			filename);
		return false;
	}

	m_dictionary_compact_trie = new c_boggle_dictionary_compact_trie;
	if (!m_dictionary_compact_trie)
	{
		output_error("load_binary_legal_words failed to allocate dictionary trie: %s",
			filename);
		return false;
	}
	if (!m_dictionary_compact_trie->read_from_file(file, m_dictionary))
	{
		output_error("load_binary_legal_words failed to read dictionary trie blob: %s",
			filename);
		return false;
	}

	return true;
}

bool c_boggle::load_binary_legal_words(
	const char* filename)
{
	if (m_currently_solving_board.load())
	{
		output_error("load_binary_legal_words called while we're already solving a board");
		return false;
	}

	if (!filename || *filename == '\0')
	{
		output_error("load_binary_legal_words passed null or empty filename");
		return false;
	}

	FILE* file;
	if (fopen_s(&file, filename, "rb") != 0)
	{
		output_error("load_binary_legal_words failed to open file: %s",
			filename);
		return false;
	}

	bool success = load_binary_legal_words(file, filename);
	fclose(file);

	return success;
}

bool c_boggle::save_binary_legal_words(
	_iobuf* file,
	const char* filename)
{
	s_boggle_dictionary_binary_file_header header =
	{
		header.k_signature,
		header.k_version,
		{ sizeof header, 0 },
		{ sizeof header, 0 },
	};

	if (1 != fwrite(&header, sizeof header, 1, file))
		return false;

	header.dictionary.offset = ftell(file);
	if (!m_dictionary->write_to_file(file))
	{
		output_error("save_binary_legal_words failed to write dictionary blob: %s",
			filename);
		return false;
	}
	header.dictionary.length = ftell(file) - header.dictionary.offset;

	header.dictionary_trie.offset = ftell(file);
	if (!m_dictionary_compact_trie->write_to_file(file))
	{
		output_error("save_binary_legal_words failed to write dictionary trie blob: %s",
			filename);
		return false;
	}
	header.dictionary_trie.length = ftell(file) - header.dictionary_trie.offset;

	if (0 != fseek(file, 0, SEEK_SET))
	{
		output_error("save_binary_legal_words failed to seek to header for finalization: %s",
			filename);
		return false;
	}

	if (1 != fwrite(&header, sizeof header, 1, file))
		return false;

	return true;
}

bool c_boggle::save_binary_legal_words(
	const char* filename)
{
	if (m_currently_solving_board.load())
	{
		output_error("save_binary_legal_words called while we're already solving a board");
		return false;
	}

	if (!filename || *filename == '\0')
	{
		output_error("save_binary_legal_words passed null or empty filename");
		return false;
	}

	if (!m_dictionary)
	{
		output_error("save_binary_legal_words called before set_legal_words");
		return false;
	}
	if (!m_dictionary_compact_trie &&
		!build_dictionary_trie(MASK(k_number_of_boggle_grid_characters)))
	{
		output_error("save_binary_legal_words failed to build dictionary trie");
		return false;
	}

	FILE* file;
	if (fopen_s(&file, filename, "w+b") != 0)
	{
		output_error("save_binary_legal_words failed to open/create file: %s",
			filename);
		return false;
	}

	bool success = save_binary_legal_words(file, filename);
	fclose(file);

	return success;
}

bool c_boggle::solve_board(
	std::vector<std::string>& found_words,
	const int board_width,
	const int board_height,
	const char* board_letters)
{
	if (m_currently_solving_board.load())
	{
		output_error("set_legal_words called while we're already solving a board");
		return false;
	}

	m_estimated_total_memory_used_for_grid = 0;
	m_estimated_total_memory_used_for_solver = 0;

	bool success = false;
	m_currently_solving_board.store(true);

	do
	{
		if (!c_boggle_grid::is_valid_board_size(board_width, board_height))
		{
			output_error("called solve_board with invalid or too large board size: %d by %d",
				board_width, board_height);
			break;
		}
		if (!board_letters || *board_letters == '\0')
		{
			output_error("called solve_board with a null or empty board_letters string");
			break;
		}
		if (!m_dictionary)
		{
			output_error("called solve_board without first calling set_legal_words");
			break;
		}

		c_boggle_grid grid(board_width, board_height);
		if (!grid.set_grid_characters(board_letters))
		{
			output_error("solve_board couldn't use board_letters data");
			break;
		}

		m_estimated_total_memory_used_for_grid = grid.estimate_total_memory_used();

		// based on the English dictionary I unearthed, there are indeed words that are just one letter, so this could be a legitimate board to solve
#if 0
		if (count_number_of_1s_bits(grid.get_occuring_grid_chars_flags()) == 1)
		{
			int grid_char_index = index_of_highest_bit_set_unsafe(grid.get_occuring_grid_chars_flags());

			output_message("board_letters consists of a single type of letter (%s), assuming no words...",
				boggle_grid_char_to_string(static_cast<boggle_grid_char_t>( grid_char_index )));

			success = true;
			break;
		}
#endif

		if (!build_dictionary_trie(grid))
		{
			output_error("solve_board couldn't allocate/build dictionary_trie or its compact form");
			break;
		}

		int processor_count = omp_get_num_procs();
		output_message("OMP says there are %d processors available, we'll try to utilize all for solving",
			processor_count);

		c_trie_boggle_board_solver board_solver(
			*m_dictionary_compact_trie, grid, found_words);

		board_solver.solve_board();

		m_estimated_total_memory_used_for_solver = board_solver.estimate_total_memory_used();

		m_dictionary_compact_trie->revert_runtime_changes();

		output_message("finished solving board, found %d words",
			static_cast<int>(found_words.size()));

		long long stopwatch_millis = std::chrono::duration_cast<std::chrono::milliseconds>(board_solver.get_time_spent_solving()).count();
		long long stopwatch_micros = std::chrono::duration_cast<std::chrono::microseconds>(board_solver.get_time_spent_solving()).count();

		output_message("\ttotal time spent solving: %" PRId64 "ms (%" PRId64 "us)",
			stopwatch_millis, stopwatch_micros);

		success = true;
	} while (false);

	m_currently_solving_board.store(false);
	return success;
}

std::vector<std::string> c_boggle::solve_board(
	const int board_width,
	const int board_height,
	const char* board_letters)
{
	std::vector<std::string> found_words;

	solve_board(found_words, board_width, board_height, board_letters);

	return found_words;
}

void c_boggle::set_filter_dictionary_with_occuring_grid_chars(
	const bool filter)
{
	if (m_currently_solving_board.load())
	{
		output_error("set_filter_dictionary_with_occuring_grid_chars called while we're already solving a board");
		return;
	}

	m_filter_dictionary_with_occuring_grid_chars = filter;
}

