#include <precompile.h>
#include <boggle_grid.h>

#include <boggle_grid_cell.h>

static_assert(sizeof(s_boggle_grid_cell) == 0x8,
	"Unexpected s_boggle_grid_cell size");

const boggle_grid_cell_axis_index_t k_invalid_boggle_grid_cell_axis_index = std::numeric_limits<boggle_grid_cell_axis_index_t>::max();
const boggle_grid_cell_index_t k_invalid_boggle_grid_cell_index = std::numeric_limits<boggle_grid_cell_index_t>::max();

//////////////////////////////////////////////////////////////////////////
// s_boggle_grid_cell

void s_boggle_grid_cell::build_neighbor_data(
	const c_boggle_grid& grid)
{
	valid_neighbor_flags = 0;
	neighbor_grid_chars_flags = 0;

	for (auto neighbor = _boggle_grid_cell_neighbor_iterator_begin_value; neighbor < k_number_of_boggle_grid_cell_neighbors; ++neighbor)
	{
		auto neighbor_cell_index = grid.get_neighbor_cell_index(this->row, this->column, neighbor);
		if (neighbor_cell_index != k_invalid_boggle_grid_cell_index)
		{
			SET_FLAG(valid_neighbor_flags, neighbor, true);
		}
	}
}

void s_boggle_grid_cell::build_neighbor_chars_data(
	const c_boggle_grid& grid)
{
	neighbor_grid_chars_flags = 0;

	for (auto neighbor = _boggle_grid_cell_neighbor_iterator_begin_value; neighbor < k_number_of_boggle_grid_cell_neighbors; ++neighbor)
	{
		auto neighbor_cell = get_neighbor_cell(grid, neighbor);
		if (neighbor_cell == nullptr)
			continue;

		SET_FLAG(neighbor_grid_chars_flags, neighbor_cell->grid_char, true);
	}
}

boggle_grid_cell_index_t s_boggle_grid_cell::get_neighbor_cell_index(
	const c_boggle_grid& grid,
	const e_boggle_grid_cell_neighbor neighbor) const
{
	if (!test_bit(valid_neighbor_flags, neighbor))
		return k_invalid_boggle_grid_cell_index;

	return grid.get_neighbor_index_cell_unsafe(this->row, this->column, neighbor);
}

const s_boggle_grid_cell* s_boggle_grid_cell::get_neighbor_cell(
	const c_boggle_grid& grid,
	const e_boggle_grid_cell_neighbor neighbor) const
{
	auto neighbor_cell_index = get_neighbor_cell_index(grid, neighbor);
	auto neighbor_cell = grid.get_cell(neighbor_cell_index);
	return neighbor_cell;
}


//////////////////////////////////////////////////////////////////////////
// c_boggle_grid

c_boggle_grid::c_boggle_grid(
	const int width,
	const int height)
	: m_dimensions({width, height})
	, m_occuring_grid_chars_flags(0)
	, m_cell_count(0)
	, m_cells(nullptr)
{
}

c_boggle_grid::~c_boggle_grid()
{
	if (m_cells)
	{
		delete[] m_cells;
		m_cell_count = 0;
		m_cells = nullptr;
	}
}

size_t c_boggle_grid::estimate_total_memory_used() const
{
	size_t estimated_total_memory_used = sizeof(*this);
	estimated_total_memory_used += sizeof(m_cells[0]) + m_cell_count;
	return estimated_total_memory_used;
}

bool c_boggle_grid::build_cells_for_row_major_order()
{
	m_cell_count = static_cast<size_t>(m_dimensions.x * m_dimensions.y);
	m_cells = new s_boggle_grid_cell[m_cell_count];
	if (!m_cells)
	{
		output_error("Failed to allocate enough memory for grid cells");
		return false;
	}

	uint32_t index = 0;
	for (int x = 0; x < m_dimensions.x; x++)
	{
		for (int y = 0; y < m_dimensions.y; y++, index++)
		{
			auto& cell = m_cells[index];

			cell.grid_char = k_invalid_boggle_grid_char;
			cell.row = static_cast<boggle_grid_cell_axis_index_t>(x);
			cell.column = static_cast<boggle_grid_cell_axis_index_t>(y);
			cell.build_neighbor_data(*this);
		}
	}

	return true;
}

void c_boggle_grid::get_neighbor_row_and_column(
	int& row,
	int& col,
	const e_boggle_grid_cell_neighbor neighbor) const
{
	static const uint8_t k_neighbor_mask_decrement_row =
		FLAG(_boggle_grid_cell_neighbor_north_west) |
		FLAG(_boggle_grid_cell_neighbor_north) |
		FLAG(_boggle_grid_cell_neighbor_north_east);
	static const uint8_t k_neighbor_mask_increment_row =
		FLAG(_boggle_grid_cell_neighbor_south_west) |
		FLAG(_boggle_grid_cell_neighbor_south) |
		FLAG(_boggle_grid_cell_neighbor_south_east);
	static const uint8_t k_neighbor_mask_decrement_col =
		FLAG(_boggle_grid_cell_neighbor_north_west) |
		FLAG(_boggle_grid_cell_neighbor_west) |
		FLAG(_boggle_grid_cell_neighbor_south_west);
	static const uint8_t k_neighbor_mask_increment_col =
		FLAG(_boggle_grid_cell_neighbor_north_east) |
		FLAG(_boggle_grid_cell_neighbor_east) |
		FLAG(_boggle_grid_cell_neighbor_south_east);

	if (test_bit(k_neighbor_mask_decrement_row, neighbor))
	{
		row--;
	}
	if (test_bit(k_neighbor_mask_decrement_col, neighbor))
	{
		col--;
	}
	if (test_bit(k_neighbor_mask_increment_row, neighbor))
	{
		row++;
	}
	if (test_bit(k_neighbor_mask_increment_col, neighbor))
	{
		col++;
	}
}

bool c_boggle_grid::set_grid_characters(
	const char* board_letters)
{
	if (board_letters == nullptr)
		return false;

	if (!build_cells_for_row_major_order())
		return false;

	size_t board_letters_size = 0;
	for ( uint32_t letter_index = 0, cell_index = 0
		; board_letters[letter_index] != '\0' && board_letters_size <= m_cell_count
		; letter_index++, cell_index++, board_letters_size++)
	{
		if (cell_index >= m_cell_count)
		{
			output_error("board_letters contains too many characters to fit this grid");
			return false;
		}

		auto& cell = m_cells[cell_index];
		auto grid_char = boggle_grid_char_from_character(board_letters[letter_index]);
		if (grid_char == k_invalid_boggle_grid_char)
		{
			output_error("board_letters contains invalid character at index #%u (%u, %u)",
				cell_index,
				cell.row, cell.column);
			return false;
		}

		cell.grid_char = grid_char;
		SET_FLAG(m_occuring_grid_chars_flags, grid_char, true);
	}

	if (board_letters_size != m_cell_count)
	{
		output_error("board_letters doesn't contain enough characters to fit the grid");
		return false;
	}

	for (auto cell = begin_cells(), end = end_cells(); cell != end; ++cell)
	{
		cell->build_neighbor_chars_data(*this);
	}

	return true;
}

boggle_grid_cell_index_t c_boggle_grid::cell_position_to_index_unsafe(
	const int row,
	const int column) const
{
	int index = (row * m_dimensions.y) + column;

	return static_cast<boggle_grid_cell_index_t>(index);
}

boggle_grid_cell_index_t c_boggle_grid::get_neighbor_index_cell_unsafe(
	const int cell_row,
	const int cell_column,
	const e_boggle_grid_cell_neighbor neighbor) const
{
	int row = cell_row;
	int col = cell_column;

	get_neighbor_row_and_column(row, col, neighbor);

	boggle_grid_cell_index_t cell_index = cell_position_to_index_unsafe(
		row,
		col);

	return cell_index;
}

boggle_grid_cell_index_t c_boggle_grid::get_neighbor_cell_index(
	const int cell_row,
	const int cell_column,
	const e_boggle_grid_cell_neighbor neighbor) const
{
	int row = cell_row;
	int col = cell_column;

	get_neighbor_row_and_column(row, col, neighbor);

	boggle_grid_cell_index_t cell_index = k_invalid_boggle_grid_cell_index;

	if (row >= 0 && row < m_dimensions.x &&
		col >= 0 && col < m_dimensions.y)
	{
		cell_index = cell_position_to_index_unsafe(
			row,
			col);
	}

	return cell_index;
}

boggle_grid_cell_index_t c_boggle_grid::cell_index_of(
	const boggle_grid_char_t grid_char,
	const uint32_t start_index) const
{
	if (start_index >= m_cell_count)
		return k_invalid_boggle_grid_char;

	for (uint32_t x = start_index; x < m_cell_count; x++)
	{
		if (grid_char == m_cells[x].grid_char)
			return static_cast<boggle_grid_cell_index_t>(x);
	}

	return k_invalid_boggle_grid_char;
}

bool c_boggle_grid::is_valid_board_size(
	const int width,
	const int height)
{
	if (width < 0 || width > s_boggle_grid_cell::k_axis_max_count)
		return false;

	if (height < 0 || height > s_boggle_grid_cell::k_axis_max_count)
		return false;

	int total_cells = width * height;
	if (total_cells > s_boggle_grid_cell::k_max_count)
		return false;

	return true;
}

