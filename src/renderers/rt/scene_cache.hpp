#include <memory>
#include <map>
#include <vector>
#include <glm/glm.hpp>
#include "aabb.hpp"
#include "ray.hpp"

namespace bu {
struct material_data;
struct mesh;
struct scene;
struct async_stop_flag;
struct model_node;
}

namespace bu::rt {

struct scene_cache_material
{
	std::weak_ptr<bu::material_data> material_data;
	int index;
	bool visited = false;
};

/**
	\brief Corresponds to one scene model node

	\todo MUTEXES!!!!! - This struct can be modified by cache update and read by
	the BVH draft builder at the same time.
*/
struct scene_cache_mesh
{
	glm::mat4 transform;
	std::vector<std::weak_ptr<bu::mesh>> meshes; // Weak pointers to the original meshes

	rt::aabb aabb;
	std::vector<rt::triangle> triangles;

	bool visited; //!< Has node been visited in this update_from_scene pass
	bool changed; //!< Has node been considered modified during current update pass
};

class bvh_draft;

/**
	\brief Caches transformed models for quicker BVH build
*/
class scene_cache
{
public:
	bool update_from_scene(const bu::scene &scene);

	const auto &get_meshes() const
	{
		return m_meshes;
	}

	std::vector<bu::rt::material> get_materials() const;
	std::vector<rt::aabb> get_mesh_aabbs() const;

private:
	bool update_from_model_node(bu::rt::scene_cache_mesh &cached_mesh, const bu::model_node &node);
	bool update_materials(const bu::scene &scene);
	bool update_meshes(const bu::scene &scene);


	std::map<std::uint64_t, std::shared_ptr<scene_cache_mesh>> m_meshes;
	std::map<std::uint64_t, scene_cache_material> m_materials;
};

}