#include "editor.hpp"
#include <stack>
#include <imgui.h>
#include <ImGuiFileDialog.h>
#include "../assimp_loader.hpp"
#include "ui/ui.hpp"
#include "../log.hpp"
#include <imgui_internal.h>
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
	}

	ImGui::EndChild();
}

/**
	\brief Property editor for nodes (transforms, names)
*/
static void ui_node_properties(std::shared_ptr<bu::scene_node> node)
{
	bool visible = node->is_visible();
	bool local_transform = node->get_transform_origin() == bu::scene_node::node_transform_origin::PARENT;
	static glm::vec3 translate;
	static glm::vec3 rotation;
	// static float scale;
	

	std::string name = node->get_name();
	std::vector<char> buf(std::max(32u, static_cast<unsigned int>(name.size()) + 1));
	std::copy(name.begin(), name.end(), buf.begin());


	glm::mat4 mat = node->get_transform();
	translate = mat[3];
	mat[3] = glm::vec4{0.f};
	glm::vec3 scale{glm::length(mat[0]), glm::length(mat[1]), glm::length(mat[2])};
	mat[0] /= scale.x;
	mat[1] /= scale.y;
	mat[2] /= scale.z;
	auto quat = glm::quat_cast(mat);

	// if (ImGui::CollapsingHeader("Node properties"))
	{
		// ImGui::Separator();
		// ImGui::Dummy(ImVec2(0.f, 5.f));
		// ImGui::Text("Node property editor");
		ImGui::InputText("Name", buf.data(), 32);
		ImGui::Dummy(ImVec2(0.f, 10.f));
		
		ImGui::DragFloat3("Translation", &translate[0], 0.1f, 0, 0);
		ImGui::DragFloat4("Rotation", &quat[0], 0.1f, 0, 0);
		ImGui::DragFloat3("Scale", &scale[0], 0.1f, 0, 0);
		ImGui::Dummy(ImVec2(0.f, 10.f));

		// ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 30);
		ImGui::Checkbox("Local transform", &local_transform);
		ImGui::Checkbox("Visible", &visible);
	}

	quat = glm::normalize(quat);
	mat = glm::translate(glm::scale(glm::mat4_cast(quat), scale), translate);

	node->set_visible(visible);
	node->set_transform_origin(local_transform ? bu::scene_node::node_transform_origin::PARENT : bu::scene_node::node_transform_origin::WORLD);
	// node->set_transform(mat);
	node->set_name(std::string{buf.begin(), buf.end()});
}

/**
	\brief Node controls (grouping and deleting)
*/
static void ui_node_controls(const bu::scene &scene, std::list<std::weak_ptr<bu::scene_node>> &selection)
{
	bool selection_empty = selection.empty();
	bool multiple_selection = selection.size() > 1;
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
	bool disable_properties = multiple_selection || selection_empty;
	bool disable_duplicate = selection_empty || contains_root_node;

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

	// Delete node
	push_disable(disable_delete);
	if (ImGui::Button("Delete node"))
	{
		for (auto &n : nodes)
			n->remove_from_parent();
		selection.clear();
	}
	pop_disable(disable_delete);

	// Dissolve node
	push_disable(disable_dissolve);
	ImGui::SameLine();
	if (ImGui::Button("Dissolve node"))
	{
		for (auto &n : nodes)
			n->dissolve();
		selection.clear();
	}
	pop_disable(disable_dissolve);

	// Group nodes
	push_disable(disable_group);
	ImGui::SameLine();
	if (ImGui::Button("Group nodes"))
	{
		auto group_node = std::make_shared<bu::scene_node>();
		group_node->set_name("Group Node");
		for (auto &n : nodes)
			n->set_parent(group_node);
		group_node->set_parent(common_parent);
	}
	pop_disable(disable_group);

	// Duplicate nodes
	// push_disable(disable_duplicate);
	// ImGui::SameLine();
	// if (ImGui::Button("Duplicate nodes"))
	// {
	// 	for (auto &n : nodes)
	// 	{
	// 		auto p = n->get_parent().lock();
	// 		auto dup = std::make_shared<bu::scene_node>(*n);
	// 		dup->set_name(dup->get_name() + " (dup)");
	// 	}
	// }
	// pop_disable(disable_duplicate);


	ImGui::Dummy(ImVec2(0.f, 10.f));

	// Node properties (only if a single one is selected)
	if (!disable_properties)
	{
		auto node = *nodes.begin();
		if (ImGui::CollapsingHeader("Node properties"))
			ui_node_properties(node);
	}
}

/**
	\brief Displays mesh info (vertex count, etc.) and list of all meshes in the selected nodes
*/
static void ui_mesh_info(const bu::scene &scene, std::list<std::weak_ptr<bu::scene_node>> &selection)
{
	std::set<std::shared_ptr<bu::scene_node>> nodes;
	std::set<const bu::mesh*> meshes;

	auto extract_meshes = [&meshes](const bu::scene_node &n)
	{
		auto extract_meshes_impl = [&meshes](const bu::scene_node &n, auto &ref)->void
		{
			if (auto mn = dynamic_cast<const bu::mesh_node*>(&n))
				for (auto &mesh : mn->meshes)
					meshes.insert(&mesh);
			
			for (const auto &c : n.get_children())
				ref(*c, ref);
		};

		return extract_meshes_impl(n, extract_meshes_impl);
	};

	for (auto &wp : selection)
		if (auto node = wp.lock())
		{
			nodes.insert(node);
			extract_meshes(*node);
		}

	ImGui::Indent();
	for (auto &mesh_ptr : meshes)
		ImGui::BulletText("%s", mesh_ptr->data->name.c_str());
	ImGui::Unindent();
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
	static bool debug_window_open = false;

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
	// \todo fix selection!
	if (this->scene)
	{
		if (this->preview)
			preview->draw(*this->scene, this->viewport_camera, nullptr);
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