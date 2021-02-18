#include "material_editor.hpp"

#include <typeinfo>
#include <set>
#include <vector>
#include <map>
#include <string>
#include <cstring>

#include <imgui.h>

#include "../../material.hpp"
#include "../../materials/generic_material.hpp"
#include "../../materials/diffuse_material.hpp"
#include "../../materials/glass_material.hpp"
#include "../../materials/generic_volume.hpp"

/**
	\brief Displays UI for editing materials in place
*/
void bu::ui::material_editor(std::shared_ptr<bu::material_data> mat)
{
	// Name buffer
	const auto name_max_len = 32u;
	std::string name = mat->name;
	std::vector<char> buf(std::max(name_max_len, static_cast<unsigned int>(name.size()) + 1));
	std::copy(name.begin(), name.end(), buf.begin());

	if (ImGui::InputText("Material name", buf.data(), name_max_len))
	{
		auto len = std::strlen(buf.data());
		mat->name = std::string{buf.begin(), buf.begin() + len};
	}

	// Volume and surface type names
	static const std::map<std::uint64_t, std::string> material_type_names{
		{typeid(bu::diffuse_material).hash_code(), "Diffuse material"},
		{typeid(bu::generic_material).hash_code(), "Generic material"},
		{typeid(bu::glass_material).hash_code(), "Glass material"},
		{typeid(bu::generic_volume).hash_code(), "Generic volume"},
	};

	// Surface type
	std::string surface_name = "None";
	if (mat->surface)
		surface_name = material_type_names.at(typeid(*mat->surface).hash_code());
	if (ImGui::BeginCombo("Surface", surface_name.c_str()))
	{
		if (ImGui::Selectable("None"))
			mat->surface.reset();

		if (ImGui::Selectable("Diffuse material"))
			mat->surface = std::make_unique<bu::diffuse_material>();

		if (ImGui::Selectable("Generic material"))
			mat->surface = std::make_unique<bu::generic_material>();

		if (ImGui::Selectable("Glass material"))
			mat->surface = std::make_unique<bu::glass_material>();

		ImGui::EndCombo();
	}

	// Volume type
	std::string volume_name = "None";
	if (mat->volume)
		volume_name = material_type_names.at(typeid(*mat->volume).hash_code());
	if (ImGui::BeginCombo("Volume", volume_name.c_str()))
	{
		if (ImGui::Selectable("None"))
			mat->volume.reset();

		if (ImGui::Selectable("Generic volume"))
			mat->volume = std::make_unique<bu::generic_volume>();

		ImGui::EndCombo();
	}

	ImGui::Dummy(ImVec2(0.f, 5.f));

	if (mat->surface && ImGui::CollapsingHeader("Surface"))
	{
		// Diffuse material
		if (auto surf = dynamic_cast<bu::diffuse_material*>(mat->surface.get()))
		{
			ImGui::ColorEdit3("Base color", &surf->color[0]);
		}

		// Generic material
		if (auto surf = dynamic_cast<bu::generic_material*>(mat->surface.get()))
		{
			ImGui::ColorEdit3("Base color", &surf->color[0]);
			ImGui::ColorEdit3("Emission", &surf->emission[0]);
			ImGui::SliderFloat("Roughness", &surf->roughness, 0, 1);
			ImGui::SliderFloat("Metallic", &surf->metallic, 0, 1);
			ImGui::SliderFloat("Transmission", &surf->transmission, 0, 1);
			ImGui::SliderFloat("IOR", &surf->ior, 0, 2);
		}
	}

	if (mat->volume && ImGui::CollapsingHeader("Volume"))
	{
	}
}