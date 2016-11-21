#include <precompile.h>
#include <boggle_text_file_io.h>

#include <boggle_dictionary.h>
#include <boggle_grid_char.h>
#include <utilities.h>

#include <set>

bool read_boggle_board_file(
	const char* filename,
	std::vector<char>& grid_chars,
	const bool consume_u_after_q)
{
	auto start_time = std::chrono::high_resolution_clock::now();

	FILE* file;
	if (fopen_s(&file, filename, "r") != 0)
	{
		output_error("failed to read boggle board file: %s",
			filename);
		return false;
	}

	output_message("reading boggle board file %s...",
		filename);

	if (!handle_byte_order_marker(file))
	{
		output_error("\tfile is not text or has unacceptable BOM, not processing");
		fclose(file);
		return false;
	}

	size_t byte_offset = 0;
	for (int c, prev_c = -1; (c = fgetc(file)) != EOF; byte_offset++)
	{
		if (iswspace(c))
			continue;

		if (consume_u_after_q && c == 'u' && prev_c == 'q')
		{
			continue;
		}

		auto test_grid_char = boggle_grid_char_from_character(c);
		if (test_grid_char == k_invalid_boggle_grid_char)
		{
			output_error("\tskipping non-playable character reading boggle board at byte offset #%d",
				static_cast<int>(byte_offset));
			continue;
		}

		grid_chars.push_back(c);
		prev_c = c;
	}

	bool success = true;
	if (!feof(file))
	{
		output_error("\tfailed reading boggle board at byte offset #%d",
			static_cast<int>(byte_offset));
		success = false;
	}

	fclose(file);

	grid_chars.push_back('\0');
	grid_chars.shrink_to_fit();

	auto end_time = std::chrono::high_resolution_clock::now();

	if (!success)
	{
		return false;
	}

	output_message("\tfinished reading boggle board file (#%d cells)",
		static_cast<int>(grid_chars.size() - 1));

	long long stopwatch_millis = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
	long long stopwatch_micros = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

	output_message("\ttotal time: %" PRId64 "ms (%" PRId64 "us)",
		stopwatch_millis, stopwatch_micros);

	return true;
}

bool read_boggle_dictionary_file(
	const char* filename,
	std::vector<std::string>& all_words,
	const bool sort_all_words_after_loading,
	const bool find_and_remove_duplicates)
{
	// if we get more than a million words, the file may be bad
	static const int k_suspect_word_count = 1000000;
	static const size_t k_word_length_padding_for_newlines = 2;
	static const size_t k_word_length = c_boggle_dictionary::k_longest_acceptable_word_length + k_word_length_padding_for_newlines;

	auto start_time = std::chrono::high_resolution_clock::now();

	FILE* file;
	if (fopen_s(&file, filename, "r") != 0)
	{
		output_error("failed to read dictionary file: %s",
			filename);
		return false;
	}

	output_message("reading dictionary file %s...",
		filename);

	if (!handle_byte_order_marker(file))
	{
		output_error("\tfile is not text or has unacceptable BOM, not processing");
		fclose(file);
		return false;
	}

	bool success = true;

	char word[k_word_length+1];
	for (int line = 1, word_count = 0; word_count <= k_suspect_word_count; line++)
	{
		memset(word, 0, sizeof word);
		
		if (!fgets(word, _countof(word), file))
		{
			if (feof(file))
			{
				break;
			}
			else
			{
				output_error("\tfailed reading dictionary at line #%d",
					line);
				success = false;
			}

			break;
		}

		size_t word_length = strlen(word);
		if (word_length == 0)
		{
			continue;
		}
		
		char* newline_char = strchr(word, '\n');
		if (newline_char)
		{
			newline_char[0] = '\0';
			word_length--;
		}

		newline_char = strchr(word, '\r');
		if (newline_char)
		{
			newline_char[0] = '\0';
			word_length--;
		}

		if (word_length == 0)
		{
			// empty line
			continue;
		}
		else if (word_length < c_boggle_dictionary::k_shortest_acceptable_word_length)
		{
			output_error("\tword at line #%d is too short (<%d), skipping: %s",
				line,
				static_cast<int>(c_boggle_dictionary::k_shortest_acceptable_word_length),
				word);
			continue;
		}
		else if (word_length > c_boggle_dictionary::k_longest_acceptable_word_length)
		{
			output_error("\tword at line #%d is too long (>%d), skipping: %s",
				line,
				static_cast<int>(c_boggle_dictionary::k_longest_acceptable_word_length),
				word);
			continue;
		}

		int invalid_character_column = -1;
		for (size_t letter_index = 0; letter_index < word_length; letter_index++)
		{
			word[letter_index] = tolower(word[letter_index]);

			auto grid_char = boggle_grid_char_from_character(word[letter_index]);
			if (grid_char == k_invalid_boggle_grid_char)
			{
				invalid_character_column = static_cast<int>(letter_index+1);
				break;
			}
		}
		if (invalid_character_column != -1)
		{
			output_error("\tword at line #%d contains unacceptable characters (starting at column #%d), skipping: %s",
				line,
				invalid_character_column,
				word);
			continue;
		}

		std::string word_string(word);
		all_words.push_back(word_string);
	}

	fclose(file);

	all_words.shrink_to_fit();

	auto end_time = std::chrono::high_resolution_clock::now();

	if (!success)
	{
		return false;
	}

	output_message("\tfinished reading dictionary file (#%d words)",
		static_cast<int>(all_words.size()));

	if (sort_all_words_after_loading)
	{
		std::sort(all_words.begin(), all_words.end());
	}
	if (find_and_remove_duplicates)
	{
		std::set<std::string> all_unique_words(all_words.begin(), all_words.end());
		if (all_unique_words.size() != all_words.size())
		{
			all_words.assign(all_unique_words.begin(), all_unique_words.end());
			all_words.shrink_to_fit();
		}
	}

	long long stopwatch_millis = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
	long long stopwatch_micros = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

	output_message("\ttotal time: %" PRId64 "ms (%" PRId64 "us)",
		stopwatch_millis, stopwatch_micros);

	return true;
}

bool write_boggle_dictionary_file(
	const char* filename,
	const std::vector<std::string>& all_words)
{
	auto start_time = std::chrono::high_resolution_clock::now();

	FILE* file;
	if (fopen_s(&file, filename, "w+") != 0)
	{
		output_error("failed to open/create dictionary file: %s",
			filename);
		return false;
	}

	output_message("saving dictionary file %s with %d words...",
		filename, static_cast<int>(all_words.size()));

	for (auto& word : all_words)
	{
		fprintf_s(file, "%s\n", word.c_str());
	}

	fclose(file);

	auto end_time = std::chrono::high_resolution_clock::now();

	output_message("\tfinished writing dictionary file");

	long long stopwatch_millis = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
	long long stopwatch_micros = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

	output_message("\ttotal time: %" PRId64 "ms (%" PRId64 "us)",
		stopwatch_millis, stopwatch_micros);

	return true;
}

