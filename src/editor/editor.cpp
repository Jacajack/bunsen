#include "editor.hpp"
#include <stack>
#include <imgui.h>
#include <ImGuiFileDialog.h>
#include "../assimp_loader.hpp"
#include "ui/ui.hpp"
#include "../log.hpp"
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
			"."
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
			node_open = ImGui::TreeNodeEx(node_ptr.get(), node_flags, "%s", node_ptr->get_name().c_str());
			
			// On normal click - select or deselect
			// Ctrl + click - toggle
			if (ImGui::IsItemClicked())
			{
				bool already_selected = selection_contains(node_ptr);
				if (ImGui::GetIO().KeyCtrl)
				{
					if (already_selected) selection_remove(node_ptr);
					else selection.push_back(node_ptr);
				}
				else
				{
					selection.clear();
					if (!already_selected) selection.push_back(node_ptr);
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
					ImGui::TreeNodeEx(&mesh, leaf_flags, "[M] %s", mesh.data->name.c_str());
				}
			}
		}
	}

	ImGui::EndChild();
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
			ui_scene_graph(*ed.scene, ed.selected_nodes);
	
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

/**
	\brief Draws a 3D line in an ImGui window
*/
static void imgui_3d_line(glm::vec4 a, glm::vec4 b, const glm::vec4 &col = glm::vec4{1.f})
{
	a /= a.w;
	b /= b.w;
	auto ws = ImGui::GetWindowSize();
	glm::vec2 A = (glm::vec2(a.x, -a.y) * 0.5f + 0.5f) * glm::vec2(ws.x, ws.y);
	glm::vec2 B = (glm::vec2(b.x, -b.y) * 0.5f + 0.5f) * glm::vec2(ws.x, ws.y);
	ImU32 c = 0;
	c |= int(col.a * 255) << 24;
	c |= int(col.b * 255) << 16;
	c |= int(col.g * 255) << 8;
	c |= int(col.r * 255) << 0;
	ImGui::GetWindowDrawList()->AddLine(ImVec2(A.x, A.y), ImVec2(B.x, B.y), c);
}

static void abort_scene_transform(bu::bunsen_editor &ed)
{
	for (const auto &n : ed.transform_nodes)
		n->dissolve();
	ed.transform_nodes.clear();
	ed.is_transform_pending = false;
}

static void apply_scene_transform(bu::bunsen_editor &ed)
{
	for (const auto &n : ed.transform_nodes)
		n->apply();
	ed.transform_nodes.clear();
	ed.is_transform_pending = false;
}

/**
	Starts transform on selected nodes
*/
static void start_scene_transform(bu::bunsen_editor &ed)
{
	if (ed.is_transform_pending)
		abort_scene_transform(ed);
	ed.is_transform_pending = true;

	// TODO compute LCA nodes

	ed.transform_matrix = glm::mat4{1.f};

	// Insert transform nodes
	try
	{
		for (auto &sn : ed.selected_nodes)
		{
			if (auto node = sn.lock())
			{
				std::shared_ptr<bu::scene_node> parent{node->get_parent()};
				auto tn = std::make_shared<bu::transform_node>(&ed.transform_matrix);
				parent->add_child(tn);
				tn->add_child(node);
				ed.transform_nodes.push_back(std::move(tn));
			}
		}
	}
	catch (const std::exception &ex)
	{
		LOG_WARNING << "Failed to initate scene transform! (" << ex.what() << ")";
		abort_scene_transform(ed);
	}
}

static void test(bu::bunsen_editor &ed)
{
	ImGui::Begin("Transform test");

	if (ImGui::Button("Start transform"))
		start_scene_transform(ed);

	if (ImGui::Button("Apply transform"))
		apply_scene_transform(ed);

	if (ImGui::Button("Abort transform"))
		abort_scene_transform(ed);

	static glm::vec3 T = glm::vec3{0.f};
	static glm::vec3 R = glm::vec3{0.f};
	ImGui::SliderFloat3("Translate", &T[0], -10, 10);
	ImGui::SliderFloat3("Rotate", &R[0], -4, 4);

	ed.transform_matrix = glm::translate(glm::orientate4(R), T);

	ImGui::End();
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
	bu::update_camera_orbiter_from_mouse(this->orbit_manipulator, main_state.user_input);
	this->orbit_manipulator.update_camera(this->viewport_camera);

	// Draw scene in preview mode
	// \todo fix selection!
	if (this->scene)
	{
		if (this->preview)
			preview->draw(*this->scene, this->viewport_camera, nullptr);
	}

	// The UI
	editor_ui(*this, main_state.debug || main_state.gl_debug);
	test(*this);

	// The overlay
	ImGui::Begin("overlay", nullptr,
		ImGuiWindowFlags_NoBackground
		| ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoInputs);
	ImGui::SetWindowPos(ImVec2(0, 0));
	ImGui::SetWindowSize(ImVec2(window_size.x, window_size.y));
	// ImGui::GetWindowDrawList()->AddLine(ImVec2(A.x, A.y), ImVec2(B.x, B.y), 0xffffffff);
	// imgui_3d_line(a, b, glm::vec4(1, 1, 0, 1));
	// ImGui::GetWindowDrawList()->AddLine(ImVec2(window_size.x - 100, window_size.y - 100), ImVec2(window_size.x, window_size.y), 0xffffffff);
	ImGui::End();

}