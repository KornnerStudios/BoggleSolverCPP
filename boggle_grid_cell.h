#pragma once

#include <inttypes.h>

#include <boggle_grid_cell_neighbor.h>
#include <boggle_grid_char.h>
#include <utilities.h>

class c_boggle_grid;

// Integer type that can address the maximum X or Y index value on the grid
// Internally, this is unsigned and we use the underlying type's MAX value for invalid indices
typedef uint8_t boggle_grid_cell_axis_index_t;
// Integer type that can address the maximum index value of a (X,Y) point
// Internally, this is unsigned and we use the underlying type's MAX value for invalid indices
typedef uint16_t boggle_grid_cell_index_t;

extern const boggle_grid_cell_axis_index_t k_invalid_boggle_grid_cell_axis_index;
extern const boggle_grid_cell_index_t k_invalid_boggle_grid_cell_index;

struct s_boggle_grid_cell
{
	enum
	{
		k_axis_max_count = std::numeric_limits<boggle_grid_cell_axis_index_t>::max(),
		k_axis_index_min = 0,
		k_axis_index_max = k_axis_max_count - 1,

		k_max_count = k_axis_max_count * k_axis_max_count,
		k_min_index = 0,
		k_max_index = k_max_count - 1,
	};

	static_assert(
		std::numeric_limits<boggle_grid_cell_index_t>::max() >= k_max_index,
		"boggle_grid_cell_index_t cannot represent an index for all possible axis values");

	static_assert(
		std::numeric_limits<s_point2d::component_t>::max() >= std::numeric_limits<boggle_grid_cell_axis_index_t>::max(),
		"s_point2d too small to represent (x,y) values from boggle_grid_cell_axis_index_t");

	// (X,Y) location of this cell
	boggle_grid_cell_axis_index_t row, column;
	// ID of the grid character in this cell
	boggle_grid_char_t grid_char;
	// bitvector of neighbors which are valid to address.
	boggle_grid_cell_neighbor_flags_t valid_neighbor_flags;
	// bitvector of grid character IDs that neighbor this cell
	boggle_grid_char_flags_t neighbor_grid_chars_flags;

	void build_neighbor_data(
		const c_boggle_grid& grid);

	void build_neighbor_chars_data(
		const c_boggle_grid& grid);

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
		const c_boggle_grid& grid,
		const e_boggle_grid_cell_neighbor neighbor) const;

	const s_boggle_grid_cell* get_neighbor_cell(
		const c_boggle_grid& grid,
		const e_boggle_grid_cell_neighbor neighbor) const;
};

