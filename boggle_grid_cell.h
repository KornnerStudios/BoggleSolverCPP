#pragma once

#include <inttypes.h>

#include <boggle_grid_cell_neighbor.h>
#include <boggle_grid_char.h>
#include <utilities.h>

class c_boggle_grid;

typedef uint8_t boggle_grid_cell_axis_index_t;
typedef uint16_t boggle_grid_cell_index_t;

extern const boggle_grid_cell_axis_index_t k_invalid_boggle_grid_cell_axis_index;
extern const boggle_grid_cell_index_t k_invalid_boggle_grid_cell_index;

struct s_boggle_grid_cell
{
	enum
	{
		k_axis_max_count = 255,
		k_axis_index_min = 0,
		k_axis_index_max = k_axis_max_count - 1,

		k_max_count = k_axis_max_count * k_axis_max_count,
		k_min_index = 0,
		k_max_index = k_max_count - 1,
	};

	boggle_grid_cell_axis_index_t row, column;
	boggle_grid_char_t grid_char;
	boggle_grid_cell_neighbor_flags_t valid_neighbor_flags;
	boggle_grid_char_flags_t neighbor_grid_chars_flags;

	void build_neighbor_data(
		const c_boggle_grid* grid);

	void build_neighbor_chars_data(
		const c_boggle_grid* grid);

	// Gets the character, as a string, found at this cell
	const char* to_string() const
	{
		return boggle_grid_char_to_string(grid_char);
	}

	s_point2d get_position() const
	{
		return s_point2d{ row, column };
	}

	boggle_grid_cell_index_t get_neighbor_cell_index(
		const c_boggle_grid* grid,
		const e_boggle_grid_cell_neighbor neighbor) const;

	const s_boggle_grid_cell* get_neighbor_cell(
		const c_boggle_grid* grid,
		const e_boggle_grid_cell_neighbor neighbor) const;
};

