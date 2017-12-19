#include <precompile.h>
#include <boggle_dictionary_compact_trie.h>

#include <boggle_dictionary.h>
#include <boggle_dictionary_trie.h>

static_assert(alignof(s_boggle_dictionary_compact_trie_node) == 0x4,
	"Unexpected s_boggle_dictionary_compact_trie_node alignment");
static_assert(sizeof(s_boggle_dictionary_compact_trie_node) == 0x10,
	"Unexpected s_boggle_dictionary_compact_trie_node size");

void s_boggle_dictionary_compact_trie_node::initialize(
	const s_boggle_dictionary_trie_node* src)
{
	private_flags = 0;
	child_node_indices_offset = -1;
	valid_child_node_indices_flags = src->valid_child_node_indices_flags;
	completed_word_index = src->completed_word_index;

	set_grid_char(src->grid_char);
	set_parent_node_index(k_invalid_relative_node_index);
}

c_boggle_dictionary_compact_trie::c_boggle_dictionary_compact_trie()
	: m_source_dictionary(nullptr)
	, m_grid_chars_on_grid(0)
	, m_word_count(0)
	, m_nodes_count(0)
	, m_child_node_indices_count(0)
	, m_root_indices_actual_count(0)
	, m_nodes(nullptr)
	, m_child_node_indices(nullptr)
{
	m_root_indices.fill(-1);
}

c_boggle_dictionary_compact_trie::c_boggle_dictionary_compact_trie(
	const c_boggle_dictionary_trie& source_trie)
	: m_source_dictionary(source_trie.get_dictionary())
	, m_grid_chars_on_grid(source_trie.get_occuring_grid_chars_flags())
	, m_word_count(source_trie.get_word_count())
	, m_nodes_count(0)
	, m_child_node_indices_count(0)
	, m_root_indices_actual_count(0)
	, m_nodes(nullptr)
	, m_child_node_indices(nullptr)
{
	m_root_indices.fill(-1);
}

void c_boggle_dictionary_compact_trie::delete_nodes_memory()
{
	if (m_nodes)
	{
		delete[] m_nodes;
		m_nodes_count = 0;
		m_nodes = nullptr;
	}

	if (m_child_node_indices)
	{
		delete[] m_child_node_indices;
		m_child_node_indices_count = 0;
		m_child_node_indices = nullptr;
	}

	if (m_root_indices_actual_count > 0)
	{
		m_root_indices_actual_count = 0;
		m_root_indices.fill(-1);
	}
}

c_boggle_dictionary_compact_trie::~c_boggle_dictionary_compact_trie()
{
	delete_nodes_memory();
}

size_t c_boggle_dictionary_compact_trie::estimate_total_memory_used() const
{
	size_t estimated_total_memory_used = sizeof(*this);
	// #NOTE not include m_source_dictionary since we don't own its memory
	estimated_total_memory_used += sizeof(m_nodes[0]) * m_nodes_count;
	estimated_total_memory_used += sizeof(m_child_node_indices[0]) * m_child_node_indices_count;
	return estimated_total_memory_used;
}

size_t c_boggle_dictionary_compact_trie::g_debug_largest_relative_parent_node_index = 0;
s_boggle_dictionary_compact_trie_node::relative_node_index_t c_boggle_dictionary_compact_trie::get_relative_parent_node_index(
	const s_boggle_dictionary_compact_trie_node* this_node,
	const s_boggle_dictionary_compact_trie_node* parent_node)
{
	if (this_node == nullptr || parent_node == nullptr)
		return s_boggle_dictionary_compact_trie_node::k_invalid_relative_node_index;

	assert(this_node>parent_node);
	size_t raw_relative_index = this_node - parent_node;
	assert(raw_relative_index>s_boggle_dictionary_compact_trie_node::k_invalid_relative_node_index);
	assert(raw_relative_index <= std::numeric_limits<relative_node_index_t>::max());

#if _DEBUG
	g_debug_largest_relative_parent_node_index = std::max(raw_relative_index, g_debug_largest_relative_parent_node_index);
#endif

	return static_cast<relative_node_index_t>(raw_relative_index);
}

size_t c_boggle_dictionary_compact_trie::g_debug_largest_relative_child_node_index = 0;
s_boggle_dictionary_compact_trie_node::relative_node_index_t c_boggle_dictionary_compact_trie::get_relative_child_node_index(
	const s_boggle_dictionary_compact_trie_node* this_node,
	const s_boggle_dictionary_compact_trie_node* child_node)
{
	if (this_node == nullptr || child_node == nullptr)
		return s_boggle_dictionary_compact_trie_node::k_invalid_relative_node_index;

	assert(child_node>this_node);
	size_t raw_relative_index = child_node - this_node;
	assert(raw_relative_index>s_boggle_dictionary_compact_trie_node::k_invalid_relative_node_index);
	assert(raw_relative_index <= std::numeric_limits<relative_node_index_t>::max());

#if _DEBUG
	g_debug_largest_relative_child_node_index = std::max(raw_relative_index, g_debug_largest_relative_child_node_index);
#endif

	return static_cast<relative_node_index_t>(raw_relative_index);
}

int c_boggle_dictionary_compact_trie::get_index_of_node_unsafe(
	const s_boggle_dictionary_compact_trie_node* node) const
{
	return static_cast<int>(node - m_nodes);
}

int c_boggle_dictionary_compact_trie::get_index_of_node(
	const s_boggle_dictionary_compact_trie_node* node) const
{
	return node != nullptr
		? get_index_of_node_unsafe(node)
		: -1;
}

s_boggle_dictionary_compact_trie_node* c_boggle_dictionary_compact_trie::get_node(
	const int node_index)
{
	assert(node_index >= 0);
	assert(static_cast<uint32_t>(node_index)<m_nodes_count);
	return &(m_nodes[node_index]);
}

const s_boggle_dictionary_compact_trie_node* c_boggle_dictionary_compact_trie::get_node(
	const int node_index) const
{
	assert(node_index >= 0);
	assert(static_cast<uint32_t>(node_index)<m_nodes_count);
	return &(m_nodes[node_index]);
}

bool c_boggle_dictionary_compact_trie::build(
	const c_boggle_dictionary_trie& source_trie)
{
	auto nodes_count_limit = static_cast<size_t>(std::numeric_limits<s_boggle_dictionary_compact_trie_node::child_node_indices_offset_t>::max());
	if (source_trie.get_node_count() > nodes_count_limit)
	{
		output_error("dictionary_trie has too many nodes than compact_trie supports: %" PRIuPTR " > %" PRIu64,
			source_trie.get_node_count(),
			nodes_count_limit);
		return false;
	}

	m_nodes_count = static_cast<uint32_t>(source_trie.get_node_count());
	m_nodes = new s_boggle_dictionary_compact_trie_node[m_nodes_count];
	if (!m_nodes)
	{
		output_error("c_boggle_dictionary_compact_trie failed to allocate nodes");
		return false;
	}

	m_root_indices = source_trie.get_root_indices();

	std::vector<s_boggle_dictionary_compact_trie_node::relative_node_index_t> temp_child_node_indices(m_nodes_count);

	for (int node_index = 0, nodes_count = static_cast<int>(m_nodes_count)
		; node_index < nodes_count
		; node_index++)
	{
		auto src_node = source_trie.get_node(node_index);
		auto node = get_node(node_index);
		node->initialize(src_node);

		auto parent_node = try_get_node(src_node->parent_node_index);
		node->set_parent_node_index(get_relative_parent_node_index(node, parent_node));

		int child_count = static_cast<int>(src_node->get_immediate_child_count());
		if (child_count == 0)
			continue;

		auto first_child_grid_char = src_node->get_first_grid_char();
		auto last_child_grid_char = src_node->get_last_grid_char();

		node->child_node_indices_offset = static_cast<s_boggle_dictionary_compact_trie_node::child_node_indices_offset_t>(temp_child_node_indices.size());
		assert(static_cast<size_t>(node->child_node_indices_offset) == temp_child_node_indices.size());

		for (int x = 0, child_indices_to_copy = (last_child_grid_char - first_child_grid_char) + 1
			; x < child_indices_to_copy
			; x++)
		{
			int child_node_index = src_node->child_node_indices[first_child_grid_char + x];
			auto child_node = try_get_node(child_node_index);

			temp_child_node_indices.push_back(get_relative_child_node_index(node, child_node));
		}
	}

	m_child_node_indices_count = static_cast<uint32_t>(temp_child_node_indices.size());
	m_child_node_indices = new s_boggle_dictionary_compact_trie_node::relative_node_index_t[m_child_node_indices_count];
	if (!m_child_node_indices)
	{
		output_error("c_boggle_dictionary_compact_trie failed to allocate child node indices");
		return false;
	}

	memcpy(m_child_node_indices, temp_child_node_indices.data(),
		sizeof(m_child_node_indices[0]) * m_child_node_indices_count);

	return true;
}

void c_boggle_dictionary_compact_trie::dump(
	std::vector<std::string>& all_words,
	std::vector<char>& chars,
	const s_boggle_dictionary_compact_trie_node* node) const
{
	const char* grid_char_string = boggle_grid_char_to_string(node->get_grid_char());
	chars.push_back(grid_char_string[0]);

	if (node->completed_word_index != -1)
	{
		chars.push_back('\0');
		std::string word(chars.data());
		all_words.push_back(word);
		chars.pop_back();
	}

	for (c_boggle_dictionary_compact_trie_node_child_nodes_iterator iter(this, node)
		; iter.next()
		; )
	{
		dump(all_words, chars, iter.get_child_node());
	}

	chars.pop_back();
}

void c_boggle_dictionary_compact_trie::dump(
	std::vector<std::string>& all_words) const
{
	all_words.reserve(m_source_dictionary->get_words_count());

	std::vector<char> chars;
	// We'll at most end up adding the longest_word_length in chars, plus the null terminator,
	// so front load the allocation here to avoid reallocations and maintain more consistent memory usage
	chars.reserve(m_source_dictionary->get_longest_word_length() + 1);
	for (int root_node_index : m_root_indices)
	{
		if (root_node_index == -1)
			continue;

		dump(all_words, chars, get_node(root_node_index));
		chars.clear();
	}
}

const s_boggle_dictionary_compact_trie_node::relative_node_index_t* c_boggle_dictionary_compact_trie::get_child_nodes_pointer(
	const s_boggle_dictionary_compact_trie_node* node) const
{
	assert(node != nullptr);

	if (node->child_node_indices_offset == -1)
		return nullptr;

	return m_child_node_indices + node->child_node_indices_offset;
}

int c_boggle_dictionary_compact_trie::get_child_node_index(
	const s_boggle_dictionary_compact_trie_node* node,
	const boggle_grid_char_t grid_char)
{
	auto child_node_indices_pointer = get_child_nodes_pointer(node);
	if (child_node_indices_pointer == nullptr || !node->contains_immediate_child_grid_char(grid_char))
		return -1;

	auto first_child_grid_char = node->get_first_grid_char();
	int child_node_index_offset = grid_char - first_child_grid_char;

	auto relative_child_node_index = child_node_indices_pointer[child_node_index_offset];

	int node_index = get_index_of_node_unsafe(node);
	int child_node_index = node_index + relative_child_node_index;
	return child_node_index;
}

void c_boggle_dictionary_compact_trie::revert_runtime_changes()
{
	for (uint32_t x = 0; x < m_nodes_count; x++)
	{
		auto& node = m_nodes[x];

		node.set_word_found(false);
	}
}

c_boggle_dictionary_compact_trie_node_child_nodes_iterator::c_boggle_dictionary_compact_trie_node_child_nodes_iterator(
	const c_boggle_dictionary_compact_trie* trie,
	const s_boggle_dictionary_compact_trie_node* node)
	: m_node(node)
{
	assert(trie != nullptr);
	assert(node != nullptr);
	m_child_nodes_indices_pointer = trie->get_child_nodes_pointer(m_node);
	m_first_child_grid_char = node->get_first_grid_char();
	m_last_child_grid_char = node->get_last_grid_char();
	m_child_grid_char = k_invalid_boggle_grid_char;
	m_child_node_index = s_boggle_dictionary_compact_trie_node::k_invalid_relative_node_index;
}

bool c_boggle_dictionary_compact_trie_node_child_nodes_iterator::next()
{
	if (!m_child_nodes_indices_pointer)
		return false;

	if (m_child_grid_char == k_invalid_boggle_grid_char)
		m_child_grid_char = m_node->get_first_grid_char();

	while (m_child_grid_char <= m_last_child_grid_char)
	{
		int child_node_index_offset = m_child_grid_char++ - m_first_child_grid_char;

		m_child_node_index = m_child_nodes_indices_pointer[child_node_index_offset];
		if (m_child_node_index != s_boggle_dictionary_compact_trie_node::k_invalid_relative_node_index)
		{
			return true;
		}
	}

	m_child_grid_char = m_last_child_grid_char + 1;
	return false;
}

bool c_boggle_dictionary_compact_trie::write_to_file(
	_iobuf* file) const
{
	if (!file)
		return false;

	uint32_t file_signature = k_file_data_signature;
	if (1 != fwrite(&file_signature, sizeof file_signature, 1, file))
		return false;

	uint32_t file_version = k_file_data_version;
	if (1 != fwrite(&file_version, sizeof file_version, 1, file))
		return false;

	if (1 != fwrite(&m_nodes_count, sizeof m_nodes_count, 1, file))
		return false;
	if (1 != fwrite(&m_child_node_indices_count, sizeof m_child_node_indices_count, 1, file))
		return false;
	if (1 != fwrite(&m_root_indices_actual_count, sizeof m_root_indices_actual_count, 1, file))
		return false;

	if (m_root_indices.size() != fwrite(m_root_indices.data(), sizeof m_root_indices[0], m_root_indices.size(), file))
		return false;

	if (1 != fwrite(&m_grid_chars_on_grid, sizeof m_grid_chars_on_grid, 1, file))
		return false;

	if (1 != fwrite(&m_word_count, sizeof m_word_count, 1, file))
		return false;

	if (m_nodes_count != fwrite(m_nodes, sizeof m_nodes[0], m_nodes_count, file))
		return false;
	if (m_child_node_indices_count != fwrite(m_child_node_indices, sizeof m_child_node_indices[0], m_child_node_indices_count, file))
		return false;

	return true;
}

bool c_boggle_dictionary_compact_trie::read_from_file(
	_iobuf* file,
	const c_boggle_dictionary* source_dictionary)
{
	if (!file)
		return false;

	if (!source_dictionary)
		return false;

	uint32_t file_signature;
	if (1 != fread(&file_signature, sizeof file_signature, 1, file))
		return false;

	uint32_t file_version;
	if (1 != fread(&file_version, sizeof file_version, 1, file))
		return false;

	if (file_signature != k_file_data_signature ||
		file_version != k_file_data_version)
		return false;

	delete_nodes_memory();
	m_source_dictionary = source_dictionary;

	if (1 != fread(&m_nodes_count, sizeof m_nodes_count, 1, file))
		return false;
	if (1 != fread(&m_child_node_indices_count, sizeof m_child_node_indices_count, 1, file))
		return false;
	if (1 != fread(&m_root_indices_actual_count, sizeof m_root_indices_actual_count, 1, file))
		return false;

	if (m_root_indices.size() != fread(m_root_indices.data(), sizeof m_root_indices[0], m_root_indices.size(), file))
		return false;

	if (1 != fread(&m_grid_chars_on_grid, sizeof m_grid_chars_on_grid, 1, file))
		return false;

	if (1 != fread(&m_word_count, sizeof m_word_count, 1, file))
		return false;

	if (m_word_count != m_source_dictionary->get_words_count())
		return false;

	m_nodes = new s_boggle_dictionary_compact_trie_node[m_nodes_count];
	if (!m_nodes)
	{
		return false;
	}

	m_child_node_indices = new s_boggle_dictionary_compact_trie_node::relative_node_index_t[m_child_node_indices_count];
	if (!m_child_node_indices)
	{
		return false;
	}

	if (m_nodes_count != fread(m_nodes, sizeof m_nodes[0], m_nodes_count, file))
		return false;
	if (m_child_node_indices_count != fread(m_child_node_indices, sizeof m_child_node_indices[0], m_child_node_indices_count, file))
		return false;

	return true;
}

