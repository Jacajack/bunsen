#pragma once
#include <string>
#include <memory>
#include "uid_provider.hpp"
#include "modified_flag.hpp"

namespace bu {

/**
	\brief Abstract surface material
*/
struct surface_material : public uid_provider<surface_material>
{
	virtual ~surface_material() = default;
};

/**
	\brief Abstract volume material
*/
struct volume_material : public uid_provider<volume_material>
{
	virtual ~volume_material() = default;
};

/**
	\brief Material data holds and owns shaders for both surface and volume shading
*/
struct material_data : public uid_provider<material_data>, public modified_flag
{
	std::string name;
	std::unique_ptr<surface_material> surface;
	std::unique_ptr<volume_material> volume;
};


}