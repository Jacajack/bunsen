#include "editor.hpp"
#include <stack>
#include <cstring>
#include "ui/ui.hpp"
#include "../assimp_loader.hpp"
#include "../material.hpp"
#include "../materials/diffuse_material.hpp"
#include "../materials/generic_material.hpp"
#include "../materials/glass_material.hpp"
#include "../materials/generic_volume.hpp"
#include "../log.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuiFileDialog.h>
using bu::bunsen_editor;

/**
	\brief Displays/opens model import dialog window and imports models
*/
static void dialog_import_model(bunsen_editor &ed, bool open = false)
{
	static ImGuiFileDialog dialog;

	if (open && ed.scene)
	{
		static ImVec4 file_highlight(0.8, 0, 0.3, 0.9);
		dialog.SetExtentionInfos(".obj", file_highlight);
		dialog.SetExtentionInfos(".dae", file_highlight);
		dialog.SetExtentionInfos(".gltf", file_highlight);
		dialog.SetExtentionInfos(".glb", file_highlight);

		dialog.OpenDialog("import_model",
			"Import a model from file",
			"OBJ files (*.obj){.obj},"
			"Collada files (*.dae){.dae},"
			"GLTF (*.gltf *.glb){.gltf,.glb}",
			""
		);
	}

	if (dialog.Display("import_model"))
	{
		if (dialog.IsOk())
		{
			std::string path = dialog.GetFilePathName();
			if (ed.scene)
			{
				try
				{
					ed.scene->root_node->add_child(bu::load_mesh_from_file(path));
				}
				catch (const std::exception &ex)
				{
					LOG_ERROR << "Failed to load model '" << path << "' - " << ex.what();
				}
			}
			else
			{
				LOG_ERROR << "Cannot import models - there is no scene!";
			}
		}

		dialog.Close();
	}
}

/**
	\brief Responsible for displaying the scene tree
	\todo refactor
*/
static void ui_scene_graph(const bu::scene &scene, std::list<std::weak_ptr<bu::scene_node>> &selection)
{
	auto selection_remove_invalid = [&selection]()
	{
		std::vector<std::list<std::weak_ptr<bu::scene_node>>::iterator> invalid;
		for (auto it = selection.begin(); it != selection.end(); ++it)
		{
			auto ps = it->lock();
			if (!ps)
				invalid.push_back(it);
		}

		for (auto &it : invalid)
			selection.erase(it);
	};

	auto selection_remove = [&selection](std::shared_ptr<bu::scene_node> &x)
	{
		for (auto it = selection.begin(); it != selection.end(); ++it)
		{
			auto ps = it->lock();
			if (ps && ps.get() == x.get())
			{
				selection.erase(it);
				break;
			}

		}
	};

	auto selection_contains = [&selection](std::shared_ptr<bu::scene_node> &x)
	{
		for (auto it = selection.begin(); it != selection.end(); ++it)
		{
			auto ps = it->lock();
			if (ps && ps.get() == x.get())
					return true;
		}

		return false;
	};

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
	ImGui::BeginChild("Scene Tree", ImVec2(ImGui::GetWindowContentRegionWidth(), 200), false, window_flags);

	// Remove invalid nodes from selection
	selection_remove_invalid();

	std::stack<std::shared_ptr<bu::scene_node>> node_stack;
	node_stack.push(scene.root_node);
	while (node_stack.size())
	{
		auto node_ptr = node_stack.top();
		node_stack.pop();		

		// Going up
		if (!node_ptr)
		{
			ImGui::TreePop();
			continue;
		}

		// Default node flags
		ImGuiTreeNodeFlags node_flags = 
			ImGuiTreeNodeFlags_OpenOnArrow 
			| ImGuiTreeNodeFlags_OpenOnDoubleClick 
			| ImGuiTreeNodeFlags_SpanFullWidth 
			| ImGuiTreeNodeFlags_SpanAvailWidth;

		// Highlight if selected
		if (selection_contains(node_ptr))
			node_flags |= ImGuiTreeNodeFlags_Selected;

		// Assume the node is open (because of temporary transform nodes)
		bool node_open = true;
		bool display_node = true;

		// Do not display transform nodes on the list
		if (dynamic_cast<bu::transform_node*>(node_ptr.get()))
			display_node = false;

		// Check if the node is visible
		bool visible = node_ptr->is_visible();

		// Dim if invisible
		if (!visible)
		{
			auto col = ImGui::GetStyleColorVec4(ImGuiCol_Text);
			col.w *= 0.5;
			ImGui::PushStyleColor(ImGuiCol_Text, col);
		}

		if (display_node)
		{
			std::string name_tags;
			if (dynamic_cast<bu::mesh_node*>(node_ptr.get()))
				name_tags += "[M] ";

			if (dynamic_cast<bu::light_node*>(node_ptr.get()))
				name_tags += "[L] ";

			if (dynamic_cast<bu::camera_node*>(node_ptr.get()))
				name_tags += "[C] ";

			node_open = ImGui::TreeNodeEx(node_ptr.get(), node_flags, "%s%s", name_tags.c_str(), node_ptr->get_name().c_str());
			
			// On click - select or deselect
			// Ctrl + click - toggle
			// If multiple items are selected, and control is not held - select
			if (ImGui::IsItemClicked())
			{
				bool already_selected = selection_contains(node_ptr);
				bool multiple_selection = selection.size() > 1;
				if (ImGui::GetIO().KeyCtrl)
				{
					if (already_selected) selection_remove(node_ptr);
					else selection.push_back(node_ptr);
				}
				else
				{
					selection.clear();
					if (multiple_selection || !already_selected) selection.push_back(node_ptr);
				}
			}
		}

		// If open, draw children
		if (node_open)
		{
			// Push end marker and children on stack
			if (display_node)
				node_stack.push(nullptr);

			for (auto &c : node_ptr->get_children())
				node_stack.push(c);

			// List meshes
			if (auto mesh_node_ptr = dynamic_cast<bu::mesh_node*>(node_ptr.get()))
			{
				ImGuiTreeNodeFlags leaf_flags = 
					ImGuiTreeNodeFlags_SpanAvailWidth 
					| ImGuiTreeNodeFlags_SpanFullWidth 
					| ImGuiTreeNodeFlags_Leaf 
					| ImGuiTreeNodeFlags_NoTreePushOnOpen;

				for (const auto &mesh : mesh_node_ptr->meshes)
				{
					ImGui::TreeNodeEx(&mesh, leaf_flags, "[mesh] %s", mesh.data->name.c_str());
				}
			}
		}

		// Pop dim
		if (!visible)
			ImGui::PopStyleColor();
	}

	ImGui::EndChild();
}

/**
	\brief Property editor for nodes (transforms, names)
	\todo Make transform UI affect the objects
*/
static void ui_node_properties(const bu::scene &scene, std::list<std::weak_ptr<bu::scene_node>> &selection)
{
	std::shared_ptr<bu::scene_node> node;
	if (!selection.empty()) node = selection.front().lock();
	if (selection.size() != 1 || !node)
	{
		ImGui::TextWrapped("Nothing to see here, buddy...");
		return;
	}

	bool visible = node->is_visible();
	bool local_transform = node->get_transform_origin() == bu::scene_node::node_transform_origin::PARENT;
	
	// Name buffer
	const auto name_max_len = 32u;
	std::string name = node->get_name();
	std::vector<char> buf(std::max(name_max_len, static_cast<unsigned int>(name.size()) + 1));
	std::copy(name.begin(), name.end(), buf.begin());

	// Decompose transform
	glm::vec3 translate;
	glm::mat4 mat = node->get_transform();
	translate = mat[3];
	mat[3] = glm::vec4{0.f};
	glm::vec3 scale{glm::length(mat[0]), glm::length(mat[1]), glm::length(mat[2])};
	mat[0] /= scale.x;
	mat[1] /= scale.y;
	mat[2] /= scale.z;
	auto quat = glm::quat_cast(mat);

	// Reassemble matrix
	// quat = glm::normalize(quat);
	// mat = glm::translate(glm::scale(glm::mat4_cast(quat), scale), translate);

	// Name 
	ImGui::InputText("Name", buf.data(), name_max_len);
	ImGui::Dummy(ImVec2(0.f, 5.f));
	
	// Transforms (currently disabled)
	ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	ImGui::DragFloat3("Translation", &translate[0], 0.1f, 0, 0);
	ImGui::DragFloat4("Rotation", &quat[0], 0.1f, 0, 0);
	ImGui::DragFloat3("Scale", &scale[0], 0.1f, 0, 0);
	ImGui::PopItemFlag();
	ImGui::PopStyleVar();
	ImGui::Dummy(ImVec2(0.f, 5.f));

	// Other attributes
	bool changed_local_transform = ImGui::Checkbox("Local transform", &local_transform);
	bool changed_visible = ImGui::Checkbox("Visible", &visible);
	
	// Write back to the node
	if (changed_visible) node->set_visible(visible);
	if (changed_local_transform) node->set_transform_origin(local_transform ? bu::scene_node::node_transform_origin::PARENT : bu::scene_node::node_transform_origin::WORLD);
	// node->set_transform(mat);

	// Do not pass entire buffer to std::string{} because it contains terminating NULs
	auto len = std::strlen(buf.data());
	node->set_name(std::string{buf.begin(), buf.begin() + len});
}

/**
	\brief Node controls (grouping and deleting)
*/
static void ui_node_controls(const bu::scene &scene, std::list<std::weak_ptr<bu::scene_node>> &selection)
{
	bool selection_empty = selection.empty();
	bool contains_root_node = false;
	bool have_common_parent = true;
	std::shared_ptr<bu::scene_node> common_parent;
	std::set<std::shared_ptr<bu::scene_node>> nodes;

	for (auto &wp : selection)
	{
		if (auto node = wp.lock())
		{
			nodes.insert(node);
			if (node == scene.root_node) contains_root_node = true;
			if (!common_parent)
				common_parent = node->get_parent().lock();
			else if (common_parent != node->get_parent().lock())
				have_common_parent = false;
		}
	}

	bool disable_delete = selection_empty || contains_root_node;
	bool disable_dissolve = selection_empty || contains_root_node;
	bool disable_group = selection_empty || contains_root_node || !have_common_parent || !common_parent;
	bool disable_duplicate = selection_empty || contains_root_node;
	bool clicked_delete = false;
	bool clicked_dissolve = false;
	bool clicked_group = false;
	bool clicked_duplicate = false;

	auto push_disable = [](bool disable)
	{
		if (!disable) return;
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	};

	auto pop_disable = [](bool disable)
	{
		if (!disable) return;
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	};

	ImGui::Dummy(ImVec2(0.f, 10.f));

	if (ImGui::BeginTable("split", 2))
	{
		auto button_size = [](){return ImVec2(ImGui::GetContentRegionAvailWidth(), 0);};

		ImGui::TableNextColumn();
		push_disable(disable_delete);
		clicked_delete = ImGui::Button("Delete", button_size());
		pop_disable(disable_delete);

		ImGui::TableNextColumn();	
		push_disable(disable_dissolve);
		clicked_dissolve = ImGui::Button("Dissolve", button_size());
		pop_disable(disable_dissolve);

		ImGui::TableNextColumn();
		push_disable(disable_group);
		clicked_group = ImGui::Button("Group", button_size());
		pop_disable(disable_group);

		ImGui::TableNextColumn();
		push_disable(disable_duplicate);
		clicked_duplicate = ImGui::Button("Duplicate", button_size());
		pop_disable(disable_duplicate);

		ImGui::EndTable();
	}

	// Delete node
	if (clicked_delete)
	{
		for (auto &n : nodes)
			n->remove_from_parent();
		selection.clear();
	}

	// Dissolve node
	if (clicked_dissolve)
	{
		for (auto &n : nodes)
			n->dissolve();
		selection.clear();
	}

	// Group nodes
	if (clicked_group)
	{
		auto group_node = std::make_shared<bu::scene_node>();
		group_node->set_name("Group Node");
		for (auto &n : nodes)
			n->set_parent(group_node);
		group_node->set_parent(common_parent);
	}

	// Duplicate nodes
	if (clicked_duplicate)
	{
		for (auto &n : nodes)
		{
			auto p = n->get_parent().lock();
			auto dup = n->clone(p);
			dup->set_name(dup->get_name() + " (dup)");
			selection.clear();
			selection.push_back(dup);
		}
	}
}

/**
	\todo Maybe don't make this a separate window
*/
static void ui_material_editor(std::shared_ptr<bu::material_data> mat)
{
	ImGui::Begin("Material editor");

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

	ImGui::End();
}

/**
	\brief Displays mesh info (vertex count, etc.) and list of all meshes in the selected nodes
*/
static void ui_mesh_info(const bu::scene &scene, std::list<std::weak_ptr<bu::scene_node>> &selection)
{
	std::set<std::shared_ptr<bu::scene_node>> nodes;
	std::set<bu::mesh*> meshes;

	// Pointers to available mesh data and materials
	std::set<std::shared_ptr<bu::mesh_data>> mesh_data_ptrs;
	std::set<std::shared_ptr<bu::material_data>> material_ptrs;

	auto extract_meshes = [&meshes, &mesh_data_ptrs, &material_ptrs](bu::scene_node &n)
	{
		auto extract_meshes_impl = [&meshes,  &mesh_data_ptrs, &material_ptrs](bu::scene_node &n, auto &ref)->void
		{
			if (auto mn = dynamic_cast<bu::mesh_node*>(&n))
				for (auto &mesh : mn->meshes)
				{
					meshes.insert(&mesh);
					if (mesh.data) mesh_data_ptrs.insert(mesh.data);
					if (mesh.mat) material_ptrs.insert(mesh.mat);
				}
			
			for (auto &c : n.get_children())
				ref(*c, ref);
		};

		return extract_meshes_impl(n, extract_meshes_impl);
	};

	// Get all unique nodes and extract meshes from them
	for (auto &wp : selection)
		if (auto node = wp.lock())
		{
			nodes.insert(node);
			extract_meshes(*node);
		}


	static bu::mesh *selected_mesh; //TODO remove static
	bool selected_mesh_valid = false;

	// Show list box with meshes if there's more than one
	if (meshes.size() > 1)
	{
		ImGui::ListBoxHeader("Meshes");
		for (auto &mesh_ptr : meshes)
		{
			std::string text = mesh_ptr->data->name;

			bool selected = mesh_ptr == selected_mesh;
			selected_mesh_valid |= selected;
			if (ImGui::Selectable(text.c_str(), selected))
				selected_mesh = mesh_ptr;
		}
		ImGui::ListBoxFooter();
		ImGui::Separator();
	}
	else if (meshes.size() == 1)
	{
		selected_mesh = *meshes.begin();
		selected_mesh_valid = true;
	}

	// Get mesh data an material pointers
	if (!selected_mesh_valid) return;
	auto mesh_data = selected_mesh->data;
	auto material_data = selected_mesh->mat;

	// Table with mesh info
	ImGui::Text("Basic mesh info:");
	if (ImGui::BeginTable("Mesh info", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
	{
		ImGui::TableNextColumn();
		ImGui::Text("Vertices");
		ImGui::TableNextColumn();
		ImGui::Text("%lu", mesh_data->positions.size());

		ImGui::TableNextColumn();
		ImGui::Text("Normals");
		ImGui::TableNextColumn();
		ImGui::Text("%lu", mesh_data->normals.size());

		ImGui::TableNextColumn();
		ImGui::Text("UVs");
		ImGui::TableNextColumn();
		ImGui::Text("%lu", mesh_data->uvs.size());

		ImGui::TableNextColumn();
		ImGui::Text("Indices");
		ImGui::TableNextColumn();
		ImGui::Text("%lu", mesh_data->indices.size());

		ImGui::TableNextColumn();
		ImGui::Text("Triangles");
		ImGui::TableNextColumn();
		ImGui::Text("%lu", mesh_data->indices.size() / 3);

		ImGui::EndTable();
	}

	ui_material_editor(material_data);
}

/**
	\brief Displays a window with some debugging cheats
*/
static void window_debug_cheats(bunsen_editor &ed)
{
	ImGui::Begin("Evil debug cheats");
	
	if (ImGui::Button("Reload preview renderer"))
	{
		ed.preview.reset();
		try
		{
			ed.preview = std::make_unique<bu::preview_renderer>();
		}
		catch (const std::exception &ex)
		{
			LOG_ERROR << "Failed to reload preview mode! - " << ex.what();
		}
	}

	ImGui::End();
}

static void window_editor(bunsen_editor &ed)
{
	if (ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_MenuBar))
	{
		// Titlebar menu
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Scene"))
			{
				ImGui::MenuItem("Load", "l", nullptr);
				ImGui::MenuItem("Save", "s", nullptr);
			
				if (ImGui::MenuItem("Import model", "i", nullptr))
					dialog_import_model(ed, true);

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}
		
		if (ed.scene)
		{
			ui_scene_graph(*ed.scene, ed.selected_nodes);
			ui_node_controls(*ed.scene, ed.selected_nodes);		

			if (ImGui::CollapsingHeader("Node properties"))
				ui_node_properties(*ed.scene, ed.selected_nodes);

			if (ImGui::CollapsingHeader("Meshes info"))
				ui_mesh_info(*ed.scene, ed.selected_nodes);
		}
	}

	ImGui::End();
}

static void editor_ui(bunsen_editor &ed, bool debug = false)
{
	static bool debug_window_open = true;

	// Main menu bar
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			ImGui::MenuItem("Load scene", "l", nullptr);
			ImGui::MenuItem("Save scene", "s", nullptr);
			if (ImGui::MenuItem("Import model", "i", nullptr))
				dialog_import_model(ed, true);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (debug)
				ImGui::MenuItem("Evil debug cheats", nullptr, &debug_window_open);

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	// Dialogs and windows
	dialog_import_model(ed);
	window_editor(ed);
	if (debug_window_open && debug) window_debug_cheats(ed);
}

void bunsen_editor::draw(const bu::bunsen_state &main_state)
{
	// Set theme
	bu::imgui_cherry_theme();

	// Get current window size
	glm::ivec2 window_size;
	glfwGetWindowSize(main_state.window, &window_size.x, &window_size.y);

	// Update camera
	this->viewport_camera.aspect = float(window_size.x) / window_size.y;
	if (!layout_ed.is_transform_pending())
		bu::update_camera_orbiter_from_mouse(this->orbit_manipulator, main_state.user_input);
	this->orbit_manipulator.update_camera(this->viewport_camera);

	// Update layout editor
	layout_ed.update(main_state.user_input, selected_nodes, viewport_camera, glm::vec2{window_size}, overlay);

	// Draw scene in preview mode
	if (this->scene)
	{
		if (this->preview)
			preview->draw(*this->scene, this->viewport_camera, {selected_nodes.begin(), selected_nodes.end()});
	}

	// The UI
	if (!layout_ed.is_transform_pending())
		editor_ui(*this, main_state.debug || main_state.gl_debug);

	// The overlay
	ImGui::Begin("overlay", nullptr,
		ImGuiWindowFlags_NoBackground
		| ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoInputs);
	ImGui::SetWindowPos(ImVec2(0, 0));
	ImGui::SetWindowSize(ImVec2(window_size.x, window_size.y));
	overlay.draw();
	ImGui::End();
}