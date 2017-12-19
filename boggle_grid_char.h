#pragma once

#include <inttypes.h>
#include <array>

#include <utilities.h>

enum
{
	k_boggle_grid_char_lower_case_letter_start = 'a',
	k_boggle_grid_char_lower_case_letter_end = 'z',
	k_boggle_grid_char_lower_case_letter_count =
		(k_boggle_grid_char_lower_case_letter_end - k_boggle_grid_char_lower_case_letter_start) + 1,

	k_number_of_boggle_grid_characters =
		k_boggle_grid_char_lower_case_letter_count,
};

// Represents the ID of a character that can appear in the grid.
// Internally, this is a zero-based index
typedef int8_t boggle_grid_char_t;
// Since boggle_grid_char_t is internally a zero-based index value mapping to grid characters,
// each bit in this bitvector marks whether that grid character is on or off for the flag's uses
// E.g., we'd want a lightweight bitvector representing which characters actually appear in our
// input grid. So characters that don't appear would have their ID-bit set to zero.
typedef uint32_t boggle_grid_char_flags_t;

static_assert(
	std::numeric_limits<boggle_grid_char_t>::max() >= k_number_of_boggle_grid_characters,
	"boggle_grid_char_t cannot map to all possible grid characters");
static_assert(
	BIT_COUNT(boggle_grid_char_flags_t) >= k_number_of_boggle_grid_characters,
	"boggle_grid_char_flags_t cannot represent a bitvector of all possible grid characters");

// Invalid ID that maps to no character
extern const boggle_grid_char_t k_invalid_boggle_grid_char;
// ID for the 'q' character
extern boggle_grid_char_t k_boggle_grid_char_special_case_q;
// ID for the 'u' character
extern boggle_grid_char_t k_boggle_grid_char_special_case_u;
// Represents the frequency of appearance for each possible boggle_grid_char_t
extern std::array<double, k_number_of_boggle_grid_characters> k_boggle_grid_char_frequencies;

void boggle_grid_char_definitions_initialize();

void boggle_grid_char_verify(
	const boggle_grid_char_t grid_char_index,
	const bool assert_on_invalid = true);

// Is the character a valid element for a Boggle grid?
bool boggle_grid_char_is_valid_character(
	// assumed printable character
	const char character);

const char* boggle_grid_char_to_string(
	const boggle_grid_char_t grid_char_index);

// Transposes a ASCII char to a grid char index (handle)
boggle_grid_char_t boggle_grid_char_from_character(
	const char character);

