#include <memory>
#include <map>
#include <vector>
#include <mutex>
#include <glm/glm.hpp>
#include "aabb.hpp"
#include "ray.hpp"

namespace bu {
struct material_data;
struct mesh;
struct scene;
class async_stop_flag;
struct model_node;
}

namespace bu::rt {

/**
	\brief Assigns every bu::material different index in material array
*/
struct scene_cache_material
{
	std::weak_ptr<bu::material_data> material_data;
	int index;
	bool visited = false;
};

/**
	\brief Corresponds to single scene model node

	\todo Thread-safety - triangles may be read from another thread
*/
struct scene_cache_mesh
{
	glm::mat4 transform;
	std::vector<std::weak_ptr<bu::mesh>> meshes; // Weak pointers to the original meshes

	rt::aabb aabb;
	std::vector<rt::triangle> triangles;

	bool visited; //!< Has node been visited in this update_from_scene pass
	bool visible;
};

/**
	\brief Caches dissolved meshes, material and light arrays

	\todo Thread-safety - get_materials() and get_mesh_aabbs() can be called from another thread
*/
class scene_cache
{
public:
	std::pair<bool, bool> update_from_scene(const bu::scene &scene);

	const auto &get_meshes() const
	{
		return m_meshes;
	}

	std::vector<bu::rt::material> get_materials() const;
	std::vector<rt::aabb> get_mesh_aabbs() const;

private:
	bool update_from_model_node(
		bu::rt::scene_cache_mesh &cached_mesh,
		const bu::model_node &node,
		bool force_update);
	std::pair<bool, bool> update_materials(const bu::scene &scene);
	bool update_meshes(const bu::scene &scene, bool force_update);


	std::map<std::uint64_t, std::shared_ptr<scene_cache_mesh>> m_meshes;
	std::map<std::uint64_t, scene_cache_material> m_materials;
};

}