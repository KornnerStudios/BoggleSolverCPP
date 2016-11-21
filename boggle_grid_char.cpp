#include <precompile.h>
#include <boggle_grid_char.h>

#include <numeric>

#include <utilities.h>

static_assert(k_number_of_boggle_grid_characters <= std::numeric_limits<boggle_grid_char_t>::max(),
	"Too many grid characters defined to index with boggle_grid_char_t");
static_assert(k_number_of_boggle_grid_characters <= BIT_COUNT(boggle_grid_char_flags_t),
	"Too many grid characters defined to hold as flags in boggle_grid_char_flags_t");

//////////////////////////////////////////////////////////////////////////
// private prototyes
void boggle_grid_char_definitions_dispose();

void set_boggle_grid_char_frequency(
	const char character,
	const double frequency);
void fill_boggle_grid_char_frequencies_with_english_stats();

//////////////////////////////////////////////////////////////////////////
// private globals
static bool boggle_grid_char_definitions_initialized;


// Traits for how characters are represented in our Boggle board models
struct boggle_grid_char_definitions
{
	enum
	{
		k_longest_character_sequence = 1,
		k_character_string_size = k_longest_character_sequence + 1, // + null terminator
	};

	typedef char character_string_t[k_character_string_size];

	static std::array<boggle_grid_char_t, CHAR_MAX>
		k_ascii_char_to_grid_char_table;
	static std::array<character_string_t, k_number_of_boggle_grid_characters>
		k_grid_char_to_ascii_string_table;

	static void add_character(
		size_t& ascii_string_table_index,
		boggle_grid_char_t& grid_char_index_cursor,
		const char character)
	{
		character_string_t& character_string = k_grid_char_to_ascii_string_table[grid_char_index_cursor];

		assert(character_string[0] == '\0');
		character_string[0] = character;
		character_string[1] = '\0';

		k_ascii_char_to_grid_char_table[character] = grid_char_index_cursor;

		ascii_string_table_index++;
		grid_char_index_cursor++;
	}

	static void add_character_range(
		size_t& ascii_string_table_index,
		boggle_grid_char_t& grid_char_index_cursor,
		const char character_start,
		const char character_end)
	{
		for (char c = character_start; c <= character_end; c++)
		{
			add_character(ascii_string_table_index, grid_char_index_cursor, c);
		}
	}
};

std::array<boggle_grid_char_t, CHAR_MAX>
	boggle_grid_char_definitions::k_ascii_char_to_grid_char_table;
std::array<boggle_grid_char_definitions::character_string_t, k_number_of_boggle_grid_characters>
	boggle_grid_char_definitions::k_grid_char_to_ascii_string_table;

const boggle_grid_char_t k_invalid_boggle_grid_char = -1;
boggle_grid_char_t k_boggle_grid_char_special_case_q = k_invalid_boggle_grid_char;
boggle_grid_char_t k_boggle_grid_char_special_case_u = k_invalid_boggle_grid_char;
std::array<double, k_number_of_boggle_grid_characters> k_boggle_grid_char_frequencies;


void boggle_grid_char_definitions_initialize()
{
	if (boggle_grid_char_definitions_initialized)
		return;

	atexit(boggle_grid_char_definitions_dispose);

	boggle_grid_char_definitions::k_ascii_char_to_grid_char_table.fill(k_invalid_boggle_grid_char);

	size_t ascii_string_table_index = 0;
	boggle_grid_char_t grid_char_index_cursor = 0;

	boggle_grid_char_definitions::add_character_range(
		ascii_string_table_index, grid_char_index_cursor,
		k_boggle_grid_char_lower_case_letter_start,
		k_boggle_grid_char_lower_case_letter_end);

	boggle_grid_char_definitions_initialized = true;

	k_boggle_grid_char_special_case_q = boggle_grid_char_from_character('q');
	k_boggle_grid_char_special_case_u = boggle_grid_char_from_character('u');

	fill_boggle_grid_char_frequencies_with_english_stats();

	float total_frequency = static_cast<float>( std::accumulate(k_boggle_grid_char_frequencies.begin(), k_boggle_grid_char_frequencies.end(), 0.0) );
	if (!realcmp(total_frequency, 100.0f))
	{
		assert(!"k_boggle_grid_char_frequencies does not total to 100%");
	}
}

static void boggle_grid_char_definitions_dispose()
{
	if (!boggle_grid_char_definitions_initialized)
		return;
}

void boggle_grid_char_verify(
	const boggle_grid_char_t grid_char_index,
	const bool assert_on_invalid)
{
	assert(!assert_on_invalid || grid_char_index != k_invalid_boggle_grid_char);
	assert(grid_char_index >= 0 && grid_char_index < k_number_of_boggle_grid_characters);
}

bool boggle_grid_char_is_valid_character(
	const char character)
{
	if (character >= k_boggle_grid_char_lower_case_letter_start &&
		character <= k_boggle_grid_char_lower_case_letter_end)
	{
		return true;
	}

	char char_lower_case = tolower(character);

	if (char_lower_case >= k_boggle_grid_char_lower_case_letter_start &&
		char_lower_case <= k_boggle_grid_char_lower_case_letter_end)
	{
		return true;
	}

	return false;
}

const char* boggle_grid_char_to_string(
	const boggle_grid_char_t grid_char_index)
{
	assert(boggle_grid_char_definitions_initialized);

	boggle_grid_char_verify(grid_char_index, false);
	if (grid_char_index == k_invalid_boggle_grid_char)
		return nullptr;

	auto character_string = boggle_grid_char_definitions::k_grid_char_to_ascii_string_table[grid_char_index];
	return character_string;
}

boggle_grid_char_t boggle_grid_char_from_character(
	const char character)
{
	assert(boggle_grid_char_definitions_initialized);

	char lower_case_char = tolower(character);

	boggle_grid_char_t grid_char_index = boggle_grid_char_definitions::k_ascii_char_to_grid_char_table[character];

	return grid_char_index;
}

static void set_boggle_grid_char_frequency(
	const char character,
	const double frequency)
{
	auto grid_char = boggle_grid_char_from_character(character);
	assert(grid_char != k_invalid_boggle_grid_char);

	k_boggle_grid_char_frequencies[grid_char] = frequency;
}

// based on https://en.wikipedia.org/wiki/Letter_frequency#Relative_frequencies_of_letters_in_the_English_language
// Which cites Robert Lewand's Cryptological Mathematics: http://en.algoritmy.net/article/40379/Letter-frequency-English
static void fill_boggle_grid_char_frequencies_with_english_stats()
{
	// #NOTE these actually only add up to 99.999, so add a 0.001 somewhere
	set_boggle_grid_char_frequency('a', 8.167);
	set_boggle_grid_char_frequency('b', 1.492);
	set_boggle_grid_char_frequency('c', 2.782);
	set_boggle_grid_char_frequency('d', 4.253);
	// e is the most common letter, so why not make it slightly more common?
	set_boggle_grid_char_frequency('e', 12.702 + 0.001);
	set_boggle_grid_char_frequency('f', 2.228);
	set_boggle_grid_char_frequency('g', 2.015);
	set_boggle_grid_char_frequency('h', 6.094);
	set_boggle_grid_char_frequency('i', 6.966);
	set_boggle_grid_char_frequency('j', 0.153);
	set_boggle_grid_char_frequency('k', 0.772);
	set_boggle_grid_char_frequency('l', 4.025);
	set_boggle_grid_char_frequency('m', 2.406);
	set_boggle_grid_char_frequency('n', 6.749);
	set_boggle_grid_char_frequency('o', 7.507);
	set_boggle_grid_char_frequency('p', 1.929);
	set_boggle_grid_char_frequency('q', 0.095);
	set_boggle_grid_char_frequency('r', 5.987);
	set_boggle_grid_char_frequency('s', 6.327);
	set_boggle_grid_char_frequency('t', 9.056);
	set_boggle_grid_char_frequency('u', 2.758);
	set_boggle_grid_char_frequency('v', 0.978);
	set_boggle_grid_char_frequency('w', 2.360);
	set_boggle_grid_char_frequency('x', 0.150);
	set_boggle_grid_char_frequency('y', 1.974);
	set_boggle_grid_char_frequency('z', 0.074);
}

