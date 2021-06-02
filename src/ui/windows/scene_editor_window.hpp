#include "ui/window.hpp"
#include "ui/widgets/light_widget.hpp"
#include "ui/widgets/material_widget.hpp"
#include "ui/widgets/model_import_dialog.hpp"
#include "ui/widgets/model_widget.hpp"
#include "ui/widgets/node_widget.hpp"
#include "ui/widgets/scene_graph_widget.hpp"
#include "ui/widgets/world_widget.hpp"

namespace bu {
class bunsen_editor;
}

namespace bu::ui {

class scene_editor_window : public bu::ui::window
{
public:
	scene_editor_window(bunsen_editor &editor, std::shared_ptr<bu::event_bus> bus = {}) :
		window("Editor", ImGuiWindowFlags_MenuBar, bus),
		m_editor(editor),
		m_light_widget(*this),
		m_material_widget(*this),
		m_model_import_dialog(*this),
		m_model_widget(*this),
		m_node_widget(*this),
		m_scene_graph_widget(*this),
		m_world_widget(*this)
	{}

	void draw() override;

private:
	void draw_buttons(const bu::scene &scene, bu::scene_selection &selection);

	bunsen_editor &m_editor;
	light_widget m_light_widget;
	material_widget m_material_widget;
	model_import_dialog m_model_import_dialog;
	model_widget m_model_widget;
	node_widget m_node_widget;
	scene_graph_widget m_scene_graph_widget;
	world_widget m_world_widget;

	bool m_open_model_import_dialog = false;
};

}