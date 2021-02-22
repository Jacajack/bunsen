#include "scene_selection.hpp"
using bu::scene_selection;

void scene_selection::clear()
{
	m_nodes.clear();
	m_primary.reset();
}

void scene_selection::select(const std::shared_ptr<bu::scene_node> &node)
{
	m_nodes.insert(node);
	m_primary = node;
}

void scene_selection::deselect(const std::shared_ptr<bu::scene_node> &node)
{
	m_nodes.erase(node);
	if (is_primary(node))
		m_primary.reset();
}

void scene_selection::click(const std::shared_ptr<bu::scene_node> &node, bool append)
{
	if (append)
	{
		if (contains(node))
		{
			if (is_primary(node))
				deselect(node);
			else
				m_primary = node;
		}
		else
			select(node);
	}
	else
	{
		clear();
		select(node);
	}
}

bool scene_selection::is_primary(const std::shared_ptr<bu::scene_node> &node) const
{
	return m_primary == node;
}

bool scene_selection::has_primary() const
{
	return bool(m_primary);
}

bool scene_selection::contains(const std::shared_ptr<bu::scene_node> &node) const
{
	return m_nodes.find(node) != m_nodes.end();
}

bool scene_selection::empty() const
{
	return m_nodes.empty();
}

int scene_selection::size() const
{
	return m_nodes.size();
}

const std::set<std::shared_ptr<bu::scene_node>> scene_selection::get_nodes() const
{
	return m_nodes;
}

std::shared_ptr<bu::scene_node> scene_selection::get_primary() const
{
	return m_primary;
}