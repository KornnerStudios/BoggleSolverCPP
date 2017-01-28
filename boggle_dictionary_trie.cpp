#include <precompile.h>
#include <boggle_dictionary_trie.h>

#include <boggle_dictionary.h>
#include <boggle_dictionary_word.h>

static_assert(sizeof(s_boggle_dictionary_trie_node) == 0x78,
	"Unexpected s_boggle_dictionary_trie_node size");

void s_boggle_dictionary_trie_node::initialize(
	const boggle_grid_char_t grid_char)
{
	this->parent_node_index = -1;
	this->grid_char = grid_char;
	this->completed_word_index = -1;
	this->valid_child_node_indices_flags = 0;
	this->child_node_indices.fill(-1);
}

c_boggle_dictionary_trie::c_boggle_dictionary_trie(
	const c_boggle_dictionary* source_dictionary,
	const boggle_grid_char_flags_t grid_chars_on_grid)
	: m_source_dictionary(source_dictionary)
	, m_grid_chars_on_grid(grid_chars_on_grid)
	, m_root_indices_actual_count(0)
	, m_nodes()
	, m_word_count(0)
{
	m_root_indices.fill(-1);
	m_nodes.reserve(estimated_node_count(source_dictionary->get_words_count(), source_dictionary->get_average_word_length()));
}

size_t c_boggle_dictionary_trie::estimate_total_memory_used() const
{
	size_t estimated_total_memory_used = sizeof(*this);
	// #NOTE not include m_source_dictionary since we don't own its memory
	estimated_total_memory_used += sizeof(m_nodes[0]) * m_nodes.capacity();
	return estimated_total_memory_used;
}

uint32_t c_boggle_dictionary_trie::estimated_node_count(
	const uint32_t dictionary_word_count,
	const uint32_t dictionary_avg_word_length)
{
	// 10x10_Dictionary.txt:
	// word count 385
	// avg word length 4
	// total nodes 650

	// english.txt:
	// word count 415242
	// avg word length 9
	// total nodes 1125511

	// (total nodes / word count) ~= (avg word length / 2)

	uint32_t node_count = dictionary_word_count;
	node_count *= dictionary_avg_word_length / 2;
	return node_count;
}

uint32_t c_boggle_dictionary_trie::get_matching_prefix_string_length(
	const char* prev_word_string,
	const char* curr_word_string)
{
	uint32_t prefix_length = 0;
	for (uint32_t letter_index = 0
		; prev_word_string[letter_index] != '\0' && curr_word_string[letter_index] != '\0'
		; letter_index++)
	{
		if (prev_word_string[letter_index] != curr_word_string[letter_index])
			break;

		prefix_length++;
	}

	return prefix_length;
}

int c_boggle_dictionary_trie::get_nth_parent_node_index(
	const int node_index,
	const uint32_t n) const
{
	int parent_node_index = node_index;
	uint32_t lookups_remaining = n;
	for (; parent_node_index != -1 && lookups_remaining != 0; lookups_remaining--)
	{
		auto node = get_node(parent_node_index);
		parent_node_index = node->parent_node_index;
	}

	assert(lookups_remaining == 0);

	return parent_node_index;
}

void c_boggle_dictionary_trie::add_word(
	s_add_word_state& add_state,
	const s_boggle_dictionary_word& word,
	const size_t word_index)
{
	if (!TEST_FLAGS(m_grid_chars_on_grid, word.grid_chars_in_word_flags))
	{
		// word contains letters not present on the board, skip it
		return;
	}

	int starting_node_index = -1;
	uint32_t letter_index = 0;
	const char* curr_word_string = word.get_string(m_source_dictionary);
#if 1 // #NOTE optimized adding, disable to run in reference mode
	if (add_state.prev_word != nullptr)
	{
		const char* prev_word_string = add_state.prev_word->get_string(m_source_dictionary);
		uint32_t prefix_length = get_matching_prefix_string_length(prev_word_string, curr_word_string);
		if (prefix_length == 0)
		{
		}
		else if (prefix_length == add_state.prev_word->get_length())
		{
			// current word is a continuation of the previous, pick up where we left off
			starting_node_index = add_state.prev_word_last_node_index;
			letter_index = prefix_length;
		}
		else if (prefix_length < add_state.prev_word->get_length())
		{
			// current word is under the same root, but branches further up the trie
			uint32_t n = (add_state.prev_word->get_length() - prefix_length);
			starting_node_index = get_nth_parent_node_index(add_state.prev_word_last_node_index,
				n);
			letter_index = prefix_length;
		}
		else // prefix_length > prev_word->get_length()
		{
			assert(!"unreachable");
			return;
		}
	}
#endif

	int node_index;
	if (starting_node_index == -1)
	{
		auto grid_char = boggle_grid_char_from_character(curr_word_string[letter_index++]);
		node_index = get_or_add_root_node_index(grid_char);
		assert(node_index >= 0);
	}
	else
	{
		node_index = starting_node_index;
		assert(node_index >= 0);
	}
	for (; letter_index < word.get_length(); letter_index++)
	{
		auto grid_char = boggle_grid_char_from_character(curr_word_string[letter_index]);

		node_index = get_or_add_child_node_index(node_index, grid_char);
	}

	s_boggle_dictionary_trie_node* node = get_node(node_index);
	assert(node != nullptr && node->completed_word_index == -1);
	node->completed_word_index = static_cast<int>(word_index);
	m_word_count++;

	add_state.prev_word = &word;
	add_state.prev_word_last_node_index = node_index;
}

int c_boggle_dictionary_trie::add_node(
	const boggle_grid_char_t grid_char)
{
	int node_index = static_cast<int>(m_nodes.size());
	m_nodes.emplace_back();

	auto node = get_node(node_index);
	node->initialize(grid_char);

	return node_index;
}

s_boggle_dictionary_trie_node* c_boggle_dictionary_trie::get_node(
	const int node_index)
{
	assert(node_index >= 0);
	assert(static_cast<size_t>(node_index)<m_nodes.size());
	return &(m_nodes[node_index]);
}

const s_boggle_dictionary_trie_node* c_boggle_dictionary_trie::get_node(
	const int node_index) const
{
	assert(node_index >= 0);
	assert(static_cast<size_t>(node_index)<m_nodes.size());
	return &(m_nodes[node_index]);
}

bool c_boggle_dictionary_trie::build()
{
	s_add_word_state add_state;
	add_state.prev_word = nullptr;
	add_state.prev_word_last_node_index = -1;

	auto words = m_source_dictionary->begin_words();
	for (uint32_t word_index = 0; word_index < m_source_dictionary->get_words_count(); word_index++)
	{
		add_word(add_state,
			words[word_index], word_index);
	}

	// don't bother doing this anymore, worst case with english dictionary the nodes memory is VERY large
	// and so we'll just end up adding to the app memory high water mark by
	//m_nodes.shrink_to_fit();

	return true;
}

void c_boggle_dictionary_trie::dump(
	std::vector<std::string>& all_words,
	std::vector<char>& chars,
	const int cursor_node_index) const
{
	auto node = get_node(cursor_node_index);
	const char* grid_char_string = boggle_grid_char_to_string(node->grid_char);
	chars.push_back(grid_char_string[0]);

	if (node->completed_word_index != -1)
	{
		chars.push_back('\0');
		std::string word(chars.data());
		all_words.push_back(word);
		chars.pop_back();
	}

	for (int child_node_index : node->child_node_indices)
	{
		if (child_node_index == -1)
			continue;

		dump(all_words, chars, child_node_index);
	}

	chars.pop_back();
}

void c_boggle_dictionary_trie::dump(
	std::vector<std::string>& all_words) const
{
	all_words.reserve(m_source_dictionary->get_words_count());

	std::vector<char> chars;
	for (int root_node_index : m_root_indices)
	{
		if (root_node_index == -1)
			continue;

		dump(all_words, chars, root_node_index);
		chars.clear();
	}
}

int c_boggle_dictionary_trie::get_or_add_root_node_index(
	const boggle_grid_char_t grid_char)
{
	if (grid_char == k_invalid_boggle_grid_char ||
		!test_bit(m_grid_chars_on_grid, grid_char))
	{
		return -1;
	}

	int root_index = m_root_indices[grid_char];
	if (root_index == -1)
	{
		root_index = add_node(grid_char);
		m_root_indices[grid_char] = root_index;
		m_root_indices_actual_count++;
	}

	return root_index;
}

int c_boggle_dictionary_trie::get_index_of_node(
	const s_boggle_dictionary_trie_node* node) const
{
	return node != nullptr
		? static_cast<int>(node - m_nodes.data())
		: -1;
}

s_boggle_dictionary_trie_node* c_boggle_dictionary_trie::get_node_child(
	const s_boggle_dictionary_trie_node* node,
	const boggle_grid_char_t grid_char)
{
	if (node == nullptr || grid_char == k_invalid_boggle_grid_char)
		return nullptr;

	if (!test_bit(node->valid_child_node_indices_flags, grid_char))
		return nullptr;

	int child_node_index = node->child_node_indices[grid_char];
	return get_node(child_node_index);
}

void c_boggle_dictionary_trie::set_node_child(
	const int node_index,
	const boggle_grid_char_t grid_char,
	const int child_node_index)
{
	if (node_index == -1 || grid_char == k_invalid_boggle_grid_char)
		return;

	auto node = get_node(node_index);

	if (child_node_index != -1)
	{
		node->child_node_indices[grid_char] = child_node_index;
		SET_FLAG(node->valid_child_node_indices_flags, grid_char, true);

		auto child_node = get_node(child_node_index);
		assert(child_node->parent_node_index == -1);
		child_node->parent_node_index = node_index;
	}
	else
	{
		node->child_node_indices[grid_char] = -1;
		SET_FLAG(node->valid_child_node_indices_flags, grid_char, false);
	}
}

int c_boggle_dictionary_trie::get_or_add_child_node_index(
	const int node_index,
	const boggle_grid_char_t grid_char)
{
	if (grid_char == k_invalid_boggle_grid_char ||
		!test_bit(m_grid_chars_on_grid, grid_char))
	{
		return -1;
	}

	auto node = get_node(node_index);

	int child_node_index = node->child_node_indices[grid_char];
	if (child_node_index == -1)
	{
		child_node_index = add_node(grid_char);
		assert(child_node_index != -1);

		set_node_child(node_index, grid_char, child_node_index);
		int actual_child_node_index = get_node(node_index)->child_node_indices[grid_char];
		assert(child_node_index == actual_child_node_index);
	}

	return child_node_index;
}

