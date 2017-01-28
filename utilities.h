#pragma once

#include <inttypes.h>

struct _iobuf;

struct s_point2d
{
	int32_t x;
	int32_t y;
};

#define FLAG(b) (1U<<(b))

#define TEST_FLAG(f, b) (((f)&FLAG(b)) > 0)
#define SET_FLAG(f, b, v) ((v) ? ((f)|=FLAG(b)) : ((f)&=~FLAG(b)))

// checks that all test bits are set in the given flags
#define TEST_FLAGS(f, test) (((f)&(test)) == (test))

#define MASK(count) ( (1U << (count)) - 1U )

#define BIT_COUNT(type) ( sizeof(type) * 8 )

constexpr uint32_t compile_time_log2(
	const uint32_t n,
	const uint32_t p = 0)
{
	return n <= 1
		? p
		: compile_time_log2(n / 2, p + 1);
}

template<
	typename TFlags,
	typename TBit>
inline
bool test_bit(
		const TFlags flags,
		const TBit bit)
{
	static_assert(sizeof(TFlags) <= sizeof(uint32_t),
		"test_bit not yet written to support >32-bit flags");

	return (static_cast<uint32_t>(flags) & (1U << static_cast<uint32_t>(bit))) > 0;
}

template<
	typename TFlags>
inline
	bool test_flag(
		const TFlags flags,
		const uint32_t flag)
{
	static_assert(sizeof(TFlags) <= sizeof(uint32_t),
		"test_flag not yet written to support >32-bit flags");

	return (static_cast<uint32_t>(flags) & flag) != 0;
}

uint32_t count_number_of_1s_bits(
	const uint32_t bits);

// if bits==0, returns -1
int32_t index_of_lowest_bit_set(
	const uint32_t bits);

// doesn't handle the bits==0 case
int32_t index_of_highest_bit_set_unsafe(
	const uint32_t bits);

// if bits==0, returns -1
int32_t index_of_highest_bit_set(
	const uint32_t bits);

// Count the "leftmost" consecutive zero bits (leading)
uint32_t leading_zeros_count(
	const uint32_t bits);

// Count the "rightmost" consecutive zero bits (trailing)
uint32_t trailing_zeros_count(
	const uint32_t bits);

void __cdecl output_message(
	const char* format,
	...);
void __cdecl output_error(
	const char* format,
	...);

bool handle_byte_order_marker(
	_iobuf* file);

bool realcmp(
	const float left,
	const float right);

