#pragma once

#include <inttypes.h>
#include <array>

enum
{
	k_boggle_grid_char_lower_case_letter_start = 'a',
	k_boggle_grid_char_lower_case_letter_end = 'z',
	k_boggle_grid_char_lower_case_letter_count =
		(k_boggle_grid_char_lower_case_letter_end - k_boggle_grid_char_lower_case_letter_start) + 1,

	k_number_of_boggle_grid_characters =
		k_boggle_grid_char_lower_case_letter_count,
};

typedef int8_t boggle_grid_char_t;
typedef uint32_t boggle_grid_char_flags_t;

extern const boggle_grid_char_t k_invalid_boggle_grid_char;
extern boggle_grid_char_t k_boggle_grid_char_special_case_q;
extern boggle_grid_char_t k_boggle_grid_char_special_case_u;
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

