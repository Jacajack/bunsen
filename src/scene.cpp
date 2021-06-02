#include "scene.hpp"
#include <stdexcept>
#include "log.hpp"
#include "utils.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

using bu::scene_node;
using bu::transform_node;

bu::scene::scene() :
	root_node(std::make_shared<scene_node>()),
	world(std::make_shared<solid_world>()),
	event_bus(std::make_shared<bu::event_bus>())
{
	root_node->set_name("Scene Root");
}

scene_node &scene_node::operator=(scene_node &&rhs)
{
	if (this == &rhs) return *this;

	// No need to call base class move operators (enable_shader_from_this)

	// Copy trivial members
	m_transform = rhs.m_transform;
	m_transform_origin = rhs.m_transform_origin;
	m_name = std::move(rhs.m_name);
	m_visible = rhs.m_visible;

	// Steal the children
	for (auto it = rhs.m_children.begin(); it != rhs.m_children.end();)
	{
		(*it)->set_parent(shared_from_this());
		it = rhs.m_children.erase(it);
	}
	
	// Assign yourself the parent and remove the old node from its parent
	if (auto p = rhs.get_parent().lock())
		set_parent(p);
	rhs.remove_from_parent();

	return *this;
}

std::shared_ptr<scene_node> bu::scene_node::clone(std::shared_ptr<scene_node> parent) const
{
	auto cl = std::make_shared<scene_node>();

	// Copy trivial members
	cl->m_transform = m_transform;
	cl->m_transform_origin = m_transform_origin;
	cl->m_name = m_name;
	cl->m_visible = m_visible;
	
	// Clone children and parent them to the new node
	for (const auto &c : m_children)
		c->clone(cl);
	
	if (parent)
		cl->set_parent(parent);

	return cl;
}

glm::mat4 scene_node::get_transform() const
{
	return m_transform;
}

glm::mat4 scene_node::get_final_transform() const
{
	return get_transform_relative_to(nullptr);
}

/**
	Nodes transformed relative to WORLD are not affected by their parents
	**unless** the parent is a transform_node and is relative to WORLD as well.
*/
glm::mat4 scene_node::get_transform_relative_to(const scene_node *what) const
{
	glm::mat4 t{1.f};
	const scene_node *n = this;

	while (n && n != what)
	{
		t = n->get_transform() * t;
		auto p = n->get_parent().lock();

		if (n->get_transform_origin() == node_transform_origin::WORLD)
		{
			if (auto ptn = std::dynamic_pointer_cast<bu::transform_node>(p))
			{
				if (ptn->get_transform_origin() == node_transform_origin::WORLD)
				{
					t = ptn->get_raw_transform() * t;
				}
			}

			// Transform nodes need to be taken into account
			// then we cannot stop
			if (!(dynamic_cast<const bu::transform_node*>(n)))
				break;
		}

		n = p.get();
	}
	
	return t;
}

void scene_node::set_transform(const glm::mat4 &t)
{
	m_transform = t;
}

scene_node::node_transform_origin scene_node::get_transform_origin() const
{
	return m_transform_origin;
}

void scene_node::set_transform_origin(node_transform_origin o)
{
	m_transform_origin = o;
}


bool scene_node::has_parent() const
{
	return !m_parent.expired();
}

bool scene_node::is_orphan() const
{
	return !has_parent();
}

std::weak_ptr<scene_node> scene_node::get_parent()
{
	return m_parent;
}

const std::weak_ptr<scene_node> scene_node::get_parent() const
{
	return m_parent;
}

/**
	\note Node is considered not to be its own ancestor
*/
bool scene_node::is_ancestor_of(std::shared_ptr<scene_node> n) const
{
	auto self = shared_from_this();
	for (; n; n = n->get_parent().lock())
	{
		auto parent = n->get_parent().lock();
		if (parent == self)
			return true;
	}
	return false;
}

/**
	\brief Adds a child to this node and sets its parent node
*/
void scene_node::add_child(std::shared_ptr<scene_node> c)
{
	auto childs_parent = c->get_parent().lock();

	// Check if the child is already ours
	if (childs_parent == shared_from_this()) return;
	
	// Add child and change its parent
	if (childs_parent)
		c->remove_from_parent();
	c->m_parent = shared_from_this();
	m_children.push_back(c);
}

/**
	\brief Removes a child and returns a smart pointer to it
*/
std::shared_ptr<scene_node> scene_node::remove_child(scene_node *c)
{
	for (auto i = 0u; i < m_children.size(); i++)
	{
		auto p = m_children[i];
		if (p.get() == c)
		{
			m_children[i] = std::move(m_children.back());
			m_children.pop_back();
			p->m_parent.reset();
			return p;
		}
	}

	throw std::runtime_error("remove_child() - the child does not belong to parent");
}

/**
	\brief Transfers children to parent node and then removes itself from parent
*/
void scene_node::dissolve()
{
	// Give all children to the parent
	if (auto p = m_parent.lock())
	{
		LOG_DEBUG << "dissolving " << this << " (parent " << p << ")";		
		while (m_children.size())
			p->add_child(m_children[0]);
		
		// Remove itself from the parent
		remove_from_parent();
	}
	else
	{
		LOG_ERROR << "dissolve() called on node without parent!";
	}
}

/**
	\brief Removes this node from its parent

	If the node has no parent, nothing happens.
	If the parent didn't own the child an error message is generated.
*/
void scene_node::remove_from_parent() noexcept
{
	auto current_parent = m_parent.lock();
	if (current_parent)
	{
		try
		{
			current_parent->remove_child(shared_from_this().get());
		}
		catch (const std::exception &ex)
		{
			LOG_ERROR << __PRETTY_FUNCTION__ << " - the parent didn't own the child!";
		}
	}
	m_parent.reset();
}

/**
	\brief Changes this node's parent node
*/
void scene_node::set_parent(std::shared_ptr<scene_node> p)
{
	p->add_child(shared_from_this());
}

const std::vector<std::shared_ptr<scene_node>> &scene_node::get_children() const
{
	return m_children;
}

const std::string &scene_node::get_name() const
{
	return m_name;
}

void scene_node::set_name(const std::string &name)
{
	m_name = name;
}

bool scene_node::is_visible() const
{
	auto p = m_parent.lock();
	return m_visible && (!p || p->is_visible());
}

void scene_node::set_visible(bool v)
{
	m_visible = v;
}

scene_node::~scene_node()
{
	if (auto p = m_parent.lock())
	{
		LOG_ERROR << "node " << this << " is being destructed while still owned by the parent " << p << "!";
		remove_from_parent();
	}
}

scene_node::dfs_iterator scene_node::begin()
{
	return dfs_iterator(*this);
}

scene_node::dfs_iterator scene_node::end()
{
	return dfs_iterator(dfs_iterator::end_tag{});
}

scene_node::dfs_iterator::dfs_iterator(scene_node &n) :
	node_stack({&n}),
	transform_stack({n.get_transform()})
{
}

scene_node::dfs_iterator::dfs_iterator(dfs_iterator::end_tag) :
	counter(-1)
{
}

scene_node &scene_node::dfs_iterator::operator*()
{
	return *node_stack.back();
}

scene_node::dfs_iterator &scene_node::dfs_iterator::operator++()
{
	scene_node *n = node_stack.back();
	auto tn = dynamic_cast<bu::transform_node*>(n);
	glm::mat4 t = transform_stack.back();
	node_stack.pop_back();
	transform_stack.pop_back();

	for (const auto &c : n->m_children)
	{
		bool child_world_transform = c->get_transform_origin() == bu::scene_node::node_transform_origin::WORLD;

		if (child_world_transform)
		{
			if (tn)
			{
				// World-relative transform node with world-relative child (apply transform)
				// Parent-relative transform node with world-relative child (do not apply transform)
				if (tn->get_transform_origin() == bu::scene_node::node_transform_origin::WORLD)
					transform_stack.push_back(tn->get_raw_transform() * c->get_transform());
				else
					transform_stack.push_back(c->get_transform());
			}
			else
			{
				// Regular node with child world-relative transform node (apply transform)
				// Regular node with child world-relative regular node (child overrides)
				if (auto ctn = std::dynamic_pointer_cast<bu::transform_node>(c))
					transform_stack.push_back(t * c->get_transform());
				else
					transform_stack.push_back(c->get_transform());
			}
		}
		else
		{
			// Parent-relative child
			transform_stack.push_back(t * c->get_transform());
		}
		
		node_stack.push_back(c.get());
	}

	counter++;

	return *this;
}

bool scene_node::dfs_iterator::operator==(const dfs_iterator &rhs) const
{
	if (counter == rhs.counter) return true;
	else if (counter == -1 && rhs.node_stack.size() == 0) return true;
	else if (rhs.counter == -1 && node_stack.size() == 0) return true;
	else return false;
}

glm::mat4 scene_node::dfs_iterator::get_transform()
{
	return transform_stack.back();
}

bu::transform_node::transform_node(const glm::mat4 *ext_mat) :
	transform_ptr(ext_mat)
{
	set_transform_origin(node_transform_origin::WORLD);
}

std::shared_ptr<scene_node> bu::transform_node::clone(std::shared_ptr<scene_node> parent) const
{
	auto cl = std::make_shared<transform_node>(transform_ptr);
	*std::dynamic_pointer_cast<scene_node>(cl) = std::move(*scene_node::clone(parent));
	return cl;
}

/**
	\warning Calling this results in a call to dissolve()
*/
void bu::transform_node::apply()
{
	// Transform for children nodes
	glm::mat4 local_transform;

	// Used matrix
	const auto &T = get_raw_transform();

	if (get_transform_origin() == node_transform_origin::WORLD)
	{
		auto p = m_parent.lock();
		auto pt = p->get_final_transform();
		local_transform = glm::inverse(pt) * T * pt;
	}
	else
	{
		local_transform = T;
	}

	for (auto &c : m_children)
	{
		// Here we distinguish between children transformed relative 
		// to the world (where we simply apply the transform) and
		// children transformed relative to the parent where
		// we need to cancel out the parent transforms first
		if (c->get_transform_origin() == node_transform_origin::WORLD)
		{
			if (get_transform_origin() == node_transform_origin::WORLD)
				c->set_transform(T * c->get_transform());
		}
		else
			c->set_transform(local_transform * c->get_transform());			
	}

	dissolve();
}

/**
*/
glm::mat4 transform_node::get_transform() const
{
	auto p = m_parent.lock();
	auto pt = p->get_final_transform();
	const auto &T = get_raw_transform();
	if (get_transform_origin() == node_transform_origin::WORLD)
		return glm::inverse(pt) * T * pt;
	else
		return T;
}

const glm::mat4 &transform_node::get_raw_transform() const
{
	return transform_ptr ? *transform_ptr : m_transform;
}

std::shared_ptr<scene_node> bu::model_node::clone(std::shared_ptr<scene_node> parent) const
{
	auto cl = std::make_shared<model_node>();
	*std::dynamic_pointer_cast<scene_node>(cl) = std::move(*scene_node::clone(parent));
	cl->model = model;
	return cl;
}

std::shared_ptr<scene_node> bu::light_node::clone(std::shared_ptr<scene_node> parent) const
{
	auto cl = std::make_shared<light_node>();
	*std::dynamic_pointer_cast<scene_node>(cl) = std::move(*scene_node::clone(parent));
	cl->light = light;
	return cl;
}
