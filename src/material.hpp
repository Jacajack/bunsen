#pragma once
#include <string>
#include <memory>
#include <glm/glm.hpp>

namespace bu {

/**
	\brief Abstract surface material
*/
struct surface_material
{
	virtual ~surface_material() = default;
};

/**
	\brief Abstract volume material
*/
struct volume_material
{
	virtual ~volume_material() = default;
};

/**
	\brief Material data holds and owns shaders for both surface and volume shading
*/
struct material_data
{
	std::string name;
	std::unique_ptr<surface_material> surface;
	std::unique_ptr<volume_material> volume;
};


}