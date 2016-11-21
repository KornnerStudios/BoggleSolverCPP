#pragma once

#include <inttypes.h>
#include <string>
#include <vector>

#include <boggle_grid_char.h>

struct _iobuf;

struct s_boggle_dictionary_word;

class c_boggle_dictionary
{
public:
	enum
	{
		k_shortest_acceptable_word_length = 3,
		k_longest_acceptable_word_length = INT8_MAX,

		k_file_data_signature = 'dict',
		k_file_data_version = 1,
	};

	struct s_root_char_word_lengths
	{
		uint16_t shortest_word_length;
		uint16_t longest_word_length;
	};

private:
	uint16_t m_shortest_word_length;
	uint16_t m_longest_word_length;
	uint32_t m_avg_word_length;

	uint32_t m_string_pool_size;
	char* m_string_pool;

	uint32_t m_words_count;
	s_boggle_dictionary_word* m_words;

	std::array<s_root_char_word_lengths, k_number_of_boggle_grid_characters>
		m_root_chars_word_lengths;

private:
	static uint32_t calculate_string_pool_size_for_legal_words(
		const std::vector<std::string>& all_words);

	// allocates or re-allocates the words array, without setting any count values
	void allocate_words_array(
		const size_t new_size);

	void delete_words_memory();

	bool add_word(
		const std::string& word_string,
		uint32_t& word_string_pool_offset);

public:
	c_boggle_dictionary();
	~c_boggle_dictionary();

	size_t estimate_total_memory_used() const;

	bool set_legal_words(
		// alphabetically-sorted array of legal words
		const std::vector<std::string>& all_words);

	const char* get_string(
		const s_boggle_dictionary_word& word) const;

	bool valid_word_pointer(
		const s_boggle_dictionary_word* word) const;

	const s_boggle_dictionary_word* get_word(
		const int word_index) const;

	int get_word_index(
		const s_boggle_dictionary_word* word) const;

	const s_boggle_dictionary_word* begin_words() const;

	const s_boggle_dictionary_word* end_words() const;

	uint32_t get_shortest_word_length() const
	{
		return m_shortest_word_length;
	}

	uint32_t get_shortest_word_length(
		const boggle_grid_char_t grid_char) const
	{
		return grid_char != k_invalid_boggle_grid_char
			? m_root_chars_word_lengths[grid_char].shortest_word_length
			: 0;
	}

	uint32_t get_longest_word_length() const
	{
		return m_longest_word_length;
	}

	uint32_t get_longest_word_length(
		const boggle_grid_char_t grid_char) const
	{
		return grid_char != k_invalid_boggle_grid_char
			? m_root_chars_word_lengths[grid_char].longest_word_length
			: 0;
	}

	uint32_t get_average_word_length() const
	{
		return m_avg_word_length;
	}

	uint32_t get_words_count() const
	{
		return m_words_count;
	}

	bool write_to_file(
		_iobuf* file) const;

	bool read_from_file(
		_iobuf* file);
};

