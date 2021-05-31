#include "material.hpp"
#include "../../material.hpp"
#include "../../materials/diffuse_material.hpp"
#include "../../materials/emissive_material.hpp"

bu::rt::material::material() :
	type(material_type::BASIC_DIFFUSE),
	basic_diffuse({glm::vec3{1.f}})
{
}


bu::rt::material::material(const bu::material_data &mat) :
	material()
{
	if (mat.surface)
	{
		if (auto ptr = dynamic_cast<const bu::diffuse_material*>(mat.surface.get()))
		{
			this->type = material_type::BASIC_DIFFUSE;
			this->basic_diffuse.albedo = ptr->color;
		}

		if (auto ptr = dynamic_cast<const bu::emissive_material*>(mat.surface.get()))
		{
			this->type = material_type::EMISSIVE;
			this->emissive.emission = ptr->color * ptr->strength;
		}
	}
}