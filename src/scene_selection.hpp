#pragma once
#include <set>
#include <memory>

namespace bu {

class scene_node;

class scene_selection
{
public:
	void clear();
	void select(const std::shared_ptr<bu::scene_node> &node);
	void deselect(const std::shared_ptr<bu::scene_node> &node);
	void click(const std::shared_ptr<bu::scene_node> &node, bool append = false);
	bool is_primary(const std::shared_ptr<bu::scene_node> &node) const;
	bool has_primary() const;
	bool contains(const std::shared_ptr<bu::scene_node> &node) const;
	bool empty() const;
	int size() const;

	const std::set<std::shared_ptr<bu::scene_node>> get_nodes() const;
	std::shared_ptr<bu::scene_node> get_primary() const;

private:
	std::set<std::shared_ptr<bu::scene_node>> m_nodes;
	std::shared_ptr<bu::scene_node> m_primary;
};

}