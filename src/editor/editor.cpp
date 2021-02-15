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
#if 0
static void ui_scene_graph(const bu::scene &scene)
{
	static bool is_node = false;
	static bool is_mesh = false;
	static const void *selection = nullptr;
	bool selection_valid = false;

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
	ImGui::BeginChild("Scene Tree", ImVec2(ImGui::GetWindowContentRegionWidth(), 200), false, window_flags);

	std::stack<const bu::scene_node*> node_stack;
	node_stack.push(&scene.root_node);
	while (node_stack.size())
	{
		auto node_ptr = node_stack.top();
		node_stack.pop();		

		if (node_ptr)
		{
			auto &node = *node_ptr;
			ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow 
				| ImGuiTreeNodeFlags_OpenOnDoubleClick 
				| ImGuiTreeNodeFlags_SpanFullWidth 
				| ImGuiTreeNodeFlags_SpanAvailWidth;
			if (is_node && selection == node_ptr)
			{
				node_flags |= ImGuiTreeNodeFlags_Selected;
				selection_valid = true;
			}
			bool node_open = ImGui::TreeNodeEx(node_ptr, node_flags, "%s", node.name.c_str());
			if (ImGui::IsItemClicked())
			{
				if (selection != node_ptr)
				{
					is_mesh = false;
					is_node = true;
					selection = node_ptr;
				}
				else
				{
					selection = nullptr;
				}
			}	

			if (node_open)
			{
				node_stack.push(nullptr);
				for (auto &c : node.children)
					node_stack.push(&c);

				for (auto &mesh : node.meshes)
				{
					ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow 
						| ImGuiTreeNodeFlags_OpenOnDoubleClick 
						| ImGuiTreeNodeFlags_SpanAvailWidth 
						| ImGuiTreeNodeFlags_SpanFullWidth 
						| ImGuiTreeNodeFlags_Leaf 
						| ImGuiTreeNodeFlags_NoTreePushOnOpen;
					if (is_mesh && selection == &mesh)
					{
						node_flags |= ImGuiTreeNodeFlags_Selected;
						selection_valid = true;
					}
					ImGui::TreeNodeEx(&mesh, node_flags, "[M] %s", mesh.data->name.c_str());
					if (ImGui::IsItemClicked())
					{
						if (selection != &mesh)
						{
							is_mesh = true;
							is_node = false;
							selection = &mesh;
						}
						else
						{
							selection = nullptr;
						}
					}	
				}
			}
		}
		else
			ImGui::TreePop();
	}

	ImGui::EndChild();
}
#endif

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
		
		// if (ed.scene)
		// 	ui_scene_graph(*ed.scene);
	
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