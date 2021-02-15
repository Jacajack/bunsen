#include "scene.hpp"
#include <stdexcept>
#include "log.hpp"
#include "utils.hpp"

using bu::mesh_gl_buffers;
using bu::mesh;
using bu::mesh_data;
using bu::scene_node;
using bu::transform_node;

void mesh_gl_buffers::buffer_mesh(const mesh_data &m)
{
	auto vertex_count = m.positions.size();
	auto vertex_size = 3 + 3 + 2;
	
	std::vector<float> data(vertex_count * vertex_size);

	for (auto i = 0u; i < vertex_count; i++)
	{
		data[i * vertex_size + 0] = m.positions[i].x;
		data[i * vertex_size + 1] = m.positions[i].y;
		data[i * vertex_size + 2] = m.positions[i].z;

		// Normal (up if missing)
		glm::vec3 N;
		if (i < m.normals.size())
			N = m.normals[i];
		else
			N = glm::vec3{0, 1, 0};

		data[i * vertex_size + 3] = N.x;
		data[i * vertex_size + 4] = N.y;
		data[i * vertex_size + 5] = N.z;

		// UVs
		glm::vec2 uv;
		if (i < m.uvs.size())
			uv = m.uvs[i];
		else
			uv = glm::vec2{0};

		data[i * vertex_size + 6] = uv.x;
		data[i * vertex_size + 7] = uv.y;
	}

	glNamedBufferStorage(vertex_buffer.id(), bu::vector_size(data), data.data(), GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferStorage(index_buffer.id(), bu::vector_size(m.indices), m.indices.data(), GL_DYNAMIC_STORAGE_BIT);
}

void mesh_data::buffer()
{
	gl_buffers = std::make_unique<mesh_gl_buffers>();
	gl_buffers->buffer_mesh(*this);
}

void mesh_data::unbuffer()
{
	gl_buffers.reset();
}

bu::scene::scene() :
	root_node(std::make_shared<scene_node>())
{
	root_node->set_name("root");
}


bu::scene_node::scene_node()
{
}

glm::mat4 scene_node::get_transform() const
{
	return m_transform;
}

glm::mat4 scene_node::get_final_transform() const
{
	return get_transform_relative_to(nullptr);
}

glm::mat4 scene_node::get_transform_relative_to(const scene_node *what) const
{
	glm::mat4 t{1.f};
	const scene_node *n = this;

	while (n && n != what)
	{
		t = n->get_transform() * t;
		if (n->get_transform_origin() == node_transform_origin::WORLD)
			break;
		n = n->get_parent().lock().get();
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
	glm::mat4 t = transform_stack.back();
	node_stack.pop_back();
	transform_stack.pop_back();

	for (const auto &c : n->m_children)
	{
		node_stack.push_back(c.get());
		transform_stack.push_back(t * c->get_transform());
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
}

/**
	\warning Calling this results in a call to dissolve()
*/
void bu::transform_node::apply()
{
	auto p = m_parent.lock();
	auto pt = p->get_final_transform();
	const auto &t = transform_ptr ? *transform_ptr : m_transform;
	auto local_transform = glm::inverse(pt) * t * pt;

	for (auto &c : m_children)
	{
		if (c->get_transform_origin() == node_transform_origin::PARENT)
			c->set_transform(local_transform * c->get_transform());
	}

	dissolve();
}

glm::mat4 transform_node::get_transform() const
{
	auto p = m_parent.lock();
	auto pt = p->get_final_transform();
	const auto &t = transform_ptr ? *transform_ptr : m_transform;
	return glm::inverse(pt) * t * pt;
}