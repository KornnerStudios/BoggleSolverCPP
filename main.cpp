#include <precompile.h>

#include <ransampl.h>

#include <boggle.h>
#include <boggle_dictionary.h>
#include <boggle_grid_char.h>
#include <boggle_text_file_io.h>
#include <utilities.h>

void boggle_board_unit_tests();

int main()
{
	boggle_grid_char_definitions_initialize();

	boggle_board_unit_tests();

    return 0;
}

static void generate_boggle_board(
	const int board_width,
	const int board_height,
	std::string& board_letters)
{
	int board_cell_count = board_width * board_height;
	board_letters.reserve(board_cell_count + 1);

	ransampl_ws* ws = ransampl_alloc(static_cast<int>(k_boggle_grid_char_frequencies.size()));
	ransampl_set(ws, k_boggle_grid_char_frequencies.data());

	std::default_random_engine generator;
	std::uniform_real_distribution<double> distribution(0.0, 1.0);

	for (int x = 0; x < board_cell_count; x++)
	{
		double ran1 = distribution(generator);
		double ran2 = distribution(generator);

		int freq_index = ransampl_draw(ws, ran1, ran2);
		auto grid_char = static_cast<boggle_grid_char_t>(freq_index);
		board_letters.push_back( boggle_grid_char_to_string(grid_char)[0] );
	}

	ransampl_free(ws);
}

static void boggle_board_unit_tests()
{
	boggle_grid_char_definitions_initialize();

	c_boggle boggle;
	static const char* k_boggle_dictionary_filename =
		R"(data\english.boggle_dictionary)";

	std::vector<std::string> all_boggle_words;
	std::vector<char> grid_chars;
	int board_width, board_height;

	const char* board_letters = nullptr;

	output_message("loading precomputed legal words dictionary binary %s",
		k_boggle_dictionary_filename);
	if (!boggle.load_binary_legal_words(k_boggle_dictionary_filename))
	{
		output_message("couldn't open legal words dictionary, loading from txt file instead");

		if (!read_boggle_dictionary_file(
			R"(data\english.txt)",
			//R"(data\10x10_Dictionary.txt)",
			all_boggle_words,
			false,
			false))
		{
			output_error("abandoning test, failed to read legal words txt file");
			return;
		}

		boggle.set_legal_words(all_boggle_words);
		all_boggle_words.clear();
		all_boggle_words.shrink_to_fit();

		if (!boggle.save_binary_legal_words(k_boggle_dictionary_filename))
		{
			output_message("couldn't save legal words dictionary, perf won't be saved next time");
		}
	}

#if 0 // test with a known board
	if (!read_boggle_board_file(
		R"(data\10x10_Grid.txt)",
		grid_chars,
		true))
	{
		output_error("abandoning test, failed to read grid txt file");
		return;
	}

	board_width = board_height = 10;
	board_letters = grid_chars.data();
#endif

#if 1 // generate a very large random grid
	std::string random_grid_chars;
	generate_boggle_board(board_width=255, board_height=255, random_grid_chars);

	board_letters = random_grid_chars.data();
#endif

	auto start_time = std::chrono::high_resolution_clock::now();

	std::vector<std::string> found_words;
	boggle.solve_board(found_words,
		board_width, board_height,
		board_letters);

#if 0 // test resolving
	found_words.clear();
	boggle.solve_board(found_words,
		board_width, board_height,
		board_letters);
#endif

	auto end_time = std::chrono::high_resolution_clock::now();

	printf_s("estimated memory usage:\n");
	printf_s("\tdictionary: %" PRIuPTR " bytes\n",
		boggle.get_dictionary()->estimate_total_memory_used());
	printf_s("\tdictionary trie: %" PRIuPTR " bytes\n",
		boggle.m_estimated_total_memory_used_for_dictionary_trie);
	printf_s("\tcompact dictionary trie: %" PRIuPTR " bytes\n",
		boggle.m_estimated_total_memory_used_for_dictionary_compact_trie);
	printf_s("\tgrid: %" PRIuPTR " bytes\n",
		boggle.m_estimated_total_memory_used_for_grid);
	printf_s("\tgrid solver: %" PRIuPTR " bytes\n",
		boggle.m_estimated_total_memory_used_for_solver);

	long long stopwatch_millis = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
	long long stopwatch_micros = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

	printf_s("total time: %" PRId64 "ms (%" PRId64 "us)\n",
		stopwatch_millis, stopwatch_micros);

	std::sort(found_words.begin(), found_words.end());

	write_boggle_dictionary_file(
		R"(found_words.txt)",
		found_words);
}

