#include <precompile.h>
#include <boggle_dictionary.h>

#include <boggle_dictionary_word.h>
#include <boggle_grid_char.h>

static_assert(sizeof(s_boggle_dictionary_word) == 0xC,
	"Unexpected s_boggle_grid_cell size");


void s_boggle_dictionary_word::initialize(
	const uint32_t length,
	const uint32_t length_with_qu_as_one_grid_char,
	const uint32_t string_pool_offset,
	const bool contains_qu,
	const boggle_grid_char_flags_t grid_chars_in_word_flags)
{
	assert(length <= UINT16_MAX);
	assert(length_with_qu_as_one_grid_char <= k_length_with_qu_as_one_grid_char_mask);

	this->private_flags = static_cast<uint16_t>(length_with_qu_as_one_grid_char);
	this->private_flags |= contains_qu ? k_contains_qu_flag : 0;
	this->length = static_cast<uint16_t>(length);
	this->string_pool_offset = string_pool_offset;
	this->grid_chars_in_word_flags = grid_chars_in_word_flags;
}

const char* s_boggle_dictionary_word::get_string(
	const c_boggle_dictionary* parent) const
{
	assert(parent);
	return parent->get_string(*this);
}


c_boggle_dictionary::c_boggle_dictionary()
	: m_shortest_word_length(0)
	, m_longest_word_length(0)
	, m_avg_word_length(0)
	, m_string_pool_size(0)
	, m_string_pool(nullptr)
	, m_words_count(0)
	, m_words(nullptr)
	, m_root_chars_word_lengths()
{
	m_root_chars_word_lengths.fill({ UINT16_MAX, 0 });
}

c_boggle_dictionary::~c_boggle_dictionary()
{
	delete_words_memory();
}

size_t c_boggle_dictionary::estimate_total_memory_used() const
{
	size_t estimated_total_memory_used = sizeof(*this);
	estimated_total_memory_used += m_string_pool_size;
	estimated_total_memory_used += sizeof(m_words[0]) + m_words_count;
	return estimated_total_memory_used;
}

uint32_t c_boggle_dictionary::calculate_string_pool_size_for_legal_words(
	const std::vector<std::string>& all_words)
{
	size_t string_pool_size = 0;
	for ( auto iter = all_words.begin(), end = all_words.end()
		; iter != end
		; ++iter)
	{
		size_t word_size = iter->size() + 1;
		string_pool_size += word_size;
	}

	return static_cast<uint32_t>( string_pool_size );
}

void c_boggle_dictionary::allocate_words_array(
	const size_t new_size)
{
	size_t total_new_size = new_size * sizeof(*m_words);
	void* memory;
	if (m_words)
	{
		memory = realloc(m_words, total_new_size);
	}
	else
	{
		memory = malloc(total_new_size);
	}
	m_words = reinterpret_cast<s_boggle_dictionary_word*>(memory);
}

void c_boggle_dictionary::delete_words_memory()
{
	if (m_string_pool)
	{
		delete[] m_string_pool;
		m_string_pool_size = 0;
		m_string_pool = nullptr;
	}
	if (m_words)
	{
		free(m_words);
		m_words_count = 0;
		m_words = nullptr;
	}
}

bool c_boggle_dictionary::add_word(
	const std::string& word_string,
	uint32_t& word_string_pool_offset)
{
	uint32_t word_length = static_cast<uint32_t>( word_string.length() );
	if (word_length < k_shortest_acceptable_word_length)
	{
		output_error("encountered legal word that is shorter than the accepted word length (%d): %s",
			k_shortest_acceptable_word_length,
			word_string.c_str());
		return false;
	}
	if (word_length > k_longest_acceptable_word_length)
	{
		output_error("encountered legal word that is longer than the accepted word length (%d): %s",
			k_longest_acceptable_word_length,
			word_string.c_str());
		return false;
	}

	char prev_char = CHAR_MIN;
	char curr_char;
	bool word_contains_qu = false;
	uint32_t word_length_with_qu_as_one_grid_char = 0;
	boggle_grid_char_flags_t grid_chars_in_word_flags = 0;
	for (size_t char_index = 0
		; char_index < word_string.length()
		; char_index++, word_length_with_qu_as_one_grid_char++)
	{
		curr_char = word_string.at(char_index);
		if (!boggle_grid_char_is_valid_character(curr_char))
		{
			// undo the modifications we've made to the string pool
			memset(&m_string_pool[word_string_pool_offset], 0, char_index);

			output_error("encountered legal word with an invalid character: %s",
				word_string.c_str());
			return false;
		}

		curr_char = tolower(curr_char);
		m_string_pool[word_string_pool_offset + char_index] = curr_char;

		if (curr_char == 'u' && prev_char == 'q')
		{
			// this will always be >= 1, so we don't have to worry about underflow
			word_length_with_qu_as_one_grid_char--;
			word_contains_qu = true;
		}
		else
		{
			auto curr_grid_char = boggle_grid_char_from_character(curr_char);
			SET_FLAG(grid_chars_in_word_flags, curr_grid_char, true);
		}
	}

	m_string_pool[word_string_pool_offset + word_length] = '\0';

	auto& word = m_words[m_words_count++];
	word.initialize(word_length, word_length_with_qu_as_one_grid_char,
		word_string_pool_offset, word_contains_qu,
		grid_chars_in_word_flags);

	word_string_pool_offset += word_length + 1;

	auto first_grid_char = boggle_grid_char_from_character(word_string.at(0));
	auto& root_char_word_lengths = m_root_chars_word_lengths[first_grid_char];

	root_char_word_lengths.shortest_word_length =
		std::min(root_char_word_lengths.shortest_word_length, static_cast<uint16_t>(word_length));
	root_char_word_lengths.longest_word_length =
		std::max(root_char_word_lengths.longest_word_length, static_cast<uint16_t>(word_length));

	m_shortest_word_length =
		std::min(m_shortest_word_length, root_char_word_lengths.shortest_word_length);
	m_longest_word_length =
		std::max(m_longest_word_length, root_char_word_lengths.longest_word_length);
	m_avg_word_length += word_length;

	return true;
}

bool c_boggle_dictionary::set_legal_words(
	const std::vector<std::string>& all_words)
{
	m_string_pool_size = calculate_string_pool_size_for_legal_words(all_words);
	m_string_pool = new char[m_string_pool_size];
	if (!m_string_pool)
	{
		output_error("Failed to allocate enough memory for dictionary string pool");
		return false;
	}

	size_t words_capacity = all_words.size();
	m_words_count = 0;
	allocate_words_array(words_capacity);
	if (!m_words)
	{
		output_error("Failed to allocate enough memory for dictionary words");
		return false;
	}

	bool failed = false;

	m_shortest_word_length = k_longest_acceptable_word_length;
	m_longest_word_length = 0;

	uint32_t word_string_pool_offset = 0;
	uint32_t iter_index = 0;
	const char* prev_word = nullptr;
	for ( auto iter = all_words.begin(), end = all_words.end()
		; iter != end
		; prev_word = iter->c_str(), ++iter, iter_index++)
	{
		if (prev_word != nullptr)
		{
			if (iter->compare(prev_word) < 0)
			{
				failed = true;
				output_error("set_legal_words was not actually given an alphabetically-sorted array: prev(%s) > curr(%s)",
					prev_word,
					iter->c_str());
				break;
			}
		}

		if (add_word(*iter, word_string_pool_offset))
		{
		}
	}

	m_avg_word_length /= m_words_count;

	if (failed)
	{
		delete_words_memory();
		return false;
	}

	assert(m_words_count <= words_capacity);
	if (m_words_count < words_capacity)
	{
		allocate_words_array(m_words_count);
		if (!m_words)
		{
			output_error("Failed to re-allocate memory for dictionary words");
			return false;
		}
	}

	return true;
}

const char* c_boggle_dictionary::get_string(
	const s_boggle_dictionary_word& word) const
{
	assert(valid_word_pointer(&word));

	uint32_t string_offset = word.string_pool_offset;
	assert(string_offset < m_string_pool_size);

	return &m_string_pool[string_offset];
}

bool  c_boggle_dictionary::valid_word_pointer(
	const s_boggle_dictionary_word* word) const
{
	return word != nullptr
		&& word >= m_words && word < (m_words + m_words_count);
}

const s_boggle_dictionary_word* c_boggle_dictionary::get_word(
	const int word_index) const
{
	return static_cast<uint32_t>(word_index) < m_words_count
		? m_words + word_index
		: nullptr;
}

int c_boggle_dictionary::get_word_index(
	const s_boggle_dictionary_word* word) const
{
	return valid_word_pointer(word)
		? static_cast<int>(word - m_words)
		: -1;
}

const s_boggle_dictionary_word* c_boggle_dictionary::begin_words() const
{
	return m_words;
}

const s_boggle_dictionary_word* c_boggle_dictionary::end_words() const
{
	return m_words + m_words_count;
}

bool c_boggle_dictionary::write_to_file(
	_iobuf* file) const
{
	if (!file)
		return false;

	uint32_t file_signature = k_file_data_signature;
	if (1 != fwrite(&file_signature, sizeof file_signature, 1, file))
		return false;

	uint32_t file_version = k_file_data_version;
	if (1 != fwrite(&file_version, sizeof file_version, 1, file))
		return false;

	if (1 != fwrite(&m_shortest_word_length, sizeof m_shortest_word_length, 1, file))
		return false;
	if (1 != fwrite(&m_longest_word_length, sizeof m_longest_word_length, 1, file))
		return false;
	if (1 != fwrite(&m_avg_word_length, sizeof m_avg_word_length, 1, file))
		return false;

	if (1 != fwrite(&m_string_pool_size, sizeof m_string_pool_size, 1, file))
		return false;
	if (m_string_pool_size != fwrite(m_string_pool, sizeof m_string_pool[0], m_string_pool_size, file))
		return false;

	if (1 != fwrite(&m_words_count, sizeof m_words_count, 1, file))
		return false;
	if (m_words_count != fwrite(m_words, sizeof m_words[0], m_words_count, file))
		return false;

	if (m_root_chars_word_lengths.size() != fwrite(m_root_chars_word_lengths.data(), sizeof m_root_chars_word_lengths[0], m_root_chars_word_lengths.size(), file))
		return false;

	return true;
}

bool c_boggle_dictionary::read_from_file(
	_iobuf* file)
{
	if (!file)
		return false;

	uint32_t file_signature;
	if (1 != fread(&file_signature, sizeof file_signature, 1, file))
		return false;

	uint32_t file_version;
	if (1 != fread(&file_version, sizeof file_version, 1, file))
		return false;

	if (file_signature != k_file_data_signature ||
		file_version != k_file_data_version)
		return false;

	delete_words_memory();

	if (1 != fread(&m_shortest_word_length, sizeof m_shortest_word_length, 1, file))
		return false;
	if (1 != fread(&m_longest_word_length, sizeof m_longest_word_length, 1, file))
		return false;
	if (1 != fread(&m_avg_word_length, sizeof m_avg_word_length, 1, file))
		return false;

	if (1 != fread(&m_string_pool_size, sizeof m_string_pool_size, 1, file))
		return false;

	m_string_pool = new char[m_string_pool_size];
	if (!m_string_pool)
	{
		return false;
	}

	if (m_string_pool_size != fread(m_string_pool, sizeof m_string_pool[0], m_string_pool_size, file))
		return false;

	if (1 != fread(&m_words_count, sizeof m_words_count, 1, file))
		return false;

	allocate_words_array(m_words_count);
	if (!m_words)
	{
		return false;
	}

	if (m_words_count != fread(m_words, sizeof m_words[0], m_words_count, file))
		return false;

	if (m_root_chars_word_lengths.size() != fread(m_root_chars_word_lengths.data(), sizeof m_root_chars_word_lengths[0], m_root_chars_word_lengths.size(), file))
		return false;

	return true;
}

