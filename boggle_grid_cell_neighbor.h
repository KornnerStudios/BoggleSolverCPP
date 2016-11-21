#pragma once

#include <inttypes.h>

// these are ordered in the same way as how the actual neighbors are in linear memory
enum e_boggle_grid_cell_neighbor : uint8_t
{
	_boggle_grid_cell_neighbor_north_west,
	_boggle_grid_cell_neighbor_north,
	_boggle_grid_cell_neighbor_north_east,
	_boggle_grid_cell_neighbor_west,
	_boggle_grid_cell_neighbor_east,
	_boggle_grid_cell_neighbor_south_west,
	_boggle_grid_cell_neighbor_south,
	_boggle_grid_cell_neighbor_south_east,

	k_number_of_boggle_grid_cell_neighbors,

	_boggle_grid_cell_neighbor_iterator_begin_value = _boggle_grid_cell_neighbor_north_west,
};

inline
e_boggle_grid_cell_neighbor& operator++(
	e_boggle_grid_cell_neighbor& neighbor)
{
	neighbor = static_cast<e_boggle_grid_cell_neighbor>(neighbor+1);
	return neighbor;
}

typedef uint8_t boggle_grid_cell_neighbor_flags_t;

