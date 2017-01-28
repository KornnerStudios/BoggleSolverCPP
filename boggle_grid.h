#pragma once

#include <cassert>

#include <boggle_grid_cell.h>

class c_boggle_grid
{
	s_point2d m_dimensions;
	boggle_grid_char_flags_t m_occuring_grid_chars_flags;
	uint32_t m_cell_count;
	s_boggle_grid_cell* m_cells;

private:
	bool build_cells_for_row_major_order();

	void get_neighbor_row_and_column(
		int& row,
		int& col,
		const e_boggle_grid_cell_neighbor neighbor) const;

public:
	c_boggle_grid(
		const int width,
		const int height);
	~c_boggle_grid();

	size_t estimate_total_memory_used() const;

	bool set_grid_characters(
		const char* board_letters);

	boggle_grid_cell_index_t cell_position_to_index_unsafe(
		const int row,
		const int column) const;

	boggle_grid_cell_index_t get_neighbor_index_cell_unsafe(
		const int cell_row,
		const int cell_column,
		const e_boggle_grid_cell_neighbor neighbor) const;

	boggle_grid_cell_index_t get_neighbor_cell_index(
		const int cell_row,
		const int cell_column,
		const e_boggle_grid_cell_neighbor neighbor) const;

	// Searches for the first occurrence of a particular character
	boggle_grid_cell_index_t cell_index_of(
		const boggle_grid_char_t grid_char,
		const uint32_t start_index = 0) const;

	const s_point2d& get_dimensions() const
	{
		return m_dimensions;
	}

	boggle_grid_char_flags_t get_occuring_grid_chars_flags() const
	{
		return m_occuring_grid_chars_flags;
	}

	uint32_t get_cell_count() const
	{
		return m_cell_count;
	}

	s_boggle_grid_cell* begin_cells()
	{
		return m_cells;
	}
	const s_boggle_grid_cell* begin_cells() const
	{
		return m_cells;
	}

	s_boggle_grid_cell* end_cells()
	{
		return m_cells + m_cell_count;
	}
	const s_boggle_grid_cell* end_cells() const
	{
		return m_cells + m_cell_count;
	}

	const s_boggle_grid_cell* get_cell(
		const boggle_grid_cell_index_t cell_index) const
	{
		assert(cell_index == k_invalid_boggle_grid_cell_index || cell_index<m_cell_count);

		return cell_index != k_invalid_boggle_grid_cell_index
			? &m_cells[cell_index]
			: nullptr;
	}

	static bool is_valid_board_size(
		const int width,
		const int height);
};

