#include <precompile.h>
#include <utilities.h>

#include <math.h>
#include <stdarg.h>

// #TODO if this code actually needs to support non-VC++ environments
// or non-x86/ARM targets then more work needs to be put into not
// using these intrinsics
#ifdef _MSC_VER
#include <intrin.h>

#pragma intrinsic(__popcnt)
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)

#endif

const float k_real_epsilon = 0.001f;

static const uint8_t k_multiply_de_bruijn_bit_position_highest_bit_set32[BIT_COUNT(uint32_t)] =
{
	0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
	8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
};
static const uint8_t k_multiply_de_bruijn_bit_position_trailing_zeros32[BIT_COUNT(uint32_t)] =
{
	0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
	31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
};

uint32_t count_number_of_1s_bits(
	const uint32_t bits)
{
#ifdef _MSC_VER
	return __popcnt(bits);
#else
	uint32_t x = bits;
	x = x - ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	x = x + (x >> 4) & 0x0F0F0F0F;
	x = (x * 0x01010101) >> 24;
	return x;
#endif
}

int32_t index_of_lowest_bit_set(
	const uint32_t bits)
{
#ifdef _MSC_VER
	unsigned long bit_index;
	return _BitScanForward(&bit_index, bits) > 0
		? static_cast<int>(bit_index)
		: -1;
#else
	return bits != 0
		? trailing_zeros_count(bits)
		: -1;
#endif
}

int32_t index_of_highest_bit_set_unsafe(
	const uint32_t bits)
{
#ifdef _MSC_VER
	unsigned long bit_index;
	_BitScanReverse(&bit_index, bits);
	return static_cast<int>(bit_index);
#else
	uint32_t value = bits;
	value |= value >> 1; // first round down to one less than a power of 2 
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;

	uint32_t index = (value * 0x07C4ACDDU) >> 27;
	return k_multiply_de_bruijn_bit_position_highest_bit_set32[index];
#endif
}

int32_t index_of_highest_bit_set(
	const uint32_t bits)
{
#ifdef _MSC_VER
	unsigned long bit_index;
	return _BitScanReverse(&bit_index, bits) > 0
		? static_cast<int>(bit_index)
		: -1;
#else
	return bits != 0
		? index_of_highest_bit_set_unsafe(bits)
		: -1;
#endif
}

uint32_t leading_zeros_count(
	const uint32_t bits)
{
#ifdef _MSC_VER
	unsigned long bit_index;
	return _BitScanReverse(&bit_index, bits) > 0
		? (BIT_COUNT(bits)-1) - bit_index
		: 0;
#else
	if (bits == 0)
		return BIT_COUNT(bits);

	uint32_t value = bits;
	value |= value >> 1; // first round down to one less than a power of 2 
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;

	// subtract the log base 2 from the number of bits in the integer
	uint32_t index = (value * 0x07C4ACDDU) >> 27;
	return (BIT_COUNT(bits) - (k_multiply_de_bruijn_bit_position_highest_bit_set32[index]+1));
#endif
}

uint32_t trailing_zeros_count(
	const uint32_t bits)
{
#ifdef _MSC_VER
	unsigned long bit_index;
	return _BitScanForward(&bit_index, bits) > 0
		? bit_index
		: 0;
#else
	if (bits == 0)
		return BIT_COUNT(bits);

	uint32_t ls1b = (~bits) + 1; // two's complement
	ls1b = bits & ls1b; // least significant 1 bit
	uint32_t index = (ls1b * 0x077CB531U) >> 27;
	return k_multiply_de_bruijn_bit_position_trailing_zeros32[index];
#endif
}

void __cdecl output_message(
	const char* format,
	...)
{
	va_list vargs;
	va_start(vargs, format);
	vprintf_s(format, vargs);
	va_end(vargs);

	printf_s("\n");
}

void __cdecl output_error(
	const char* format,
	...)
{
	va_list vargs;
	va_start(vargs, format);
	vfprintf_s(stderr, format, vargs);
	va_end(vargs);

	printf_s("\n");
}

bool handle_byte_order_marker(
	_iobuf* file)
{
	uint8_t header[4];
	size_t numread = fread_s(header, sizeof header, sizeof header[0], _countof(header), file);

	long seek_position = 0;

	switch (numread)
	{
	case 4:
		if (header[0] == 0x00 &&
			header[1] == 0x00 &&
			header[2] == 0xFE &&
			header[3] == 0xFF)
		{
			output_error("file looks like UTF32-BE");
			return false;
		}
		if (header[0] == 0xFF &&
			header[1] == 0xFE &&
			header[2] == 0x00 &&
			header[3] == 0x00)
		{
			output_error("file looks like UTF32-LE");
			return false;
		}

	case 3:
		if (header[0] == 0xEF &&
			header[1] == 0xBB &&
			header[2] == 0xBF)
		{
			output_message("file looks like UTF8, will TRY to parse it, no promises");
			seek_position = 3;
			break;
		}

	case 2:
		if (header[0] == 0xFE &&
			header[1] == 0xFF)
		{
			output_error("file looks like UTF16-BE");
			return false;
		}
		if (header[0] == 0xFF &&
			header[1] == 0xFE)
		{
			output_error("file looks like UTF16-LE");
			return false;
		}

		break;
	}

	fseek(file, seek_position, SEEK_SET);

	return true;
}

bool realcmp(
	const float left,
	const float right)
{
	const auto difference = left - right;
	return fabs(difference) < k_real_epsilon;
}

