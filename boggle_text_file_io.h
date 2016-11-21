#pragma once

#include <string>
#include <vector>

bool read_boggle_board_file(
	const char* filename,
	std::vector<char>& grid_chars,
	const bool consume_u_after_q);

bool read_boggle_dictionary_file(
	const char* filename,
	std::vector<std::string>& all_words,
	const bool sort_all_words_after_loading = true,
	const bool find_and_remove_duplicates = false);

bool write_boggle_dictionary_file(
	const char* filename,
	const std::vector<std::string>& all_words);

