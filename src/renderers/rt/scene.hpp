#pragma once
#include <vector>
#include <memory>

namespace bu::rt {
struct material;
struct bvh_tree;

/**
	\brief RT scene - BVH + RT material and RT light arrays
*/
struct scene
{
	std::shared_ptr<bu::rt::bvh_tree> bvh;
	std::shared_ptr<std::vector<bu::rt::material>> materials;
	// std::shared_ptr<std::vector<bu::rt::light>> lights;
};

}