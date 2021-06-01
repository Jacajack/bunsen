#pragma once
#include <vector>
#include <memory>
#include "ui/widget.hpp"
#include "model.hpp"

namespace bu::ui {

class model_widget : public widget
{
public:
	model_widget(window &win) : 
		widget(win)
	{
	}

	void draw(const std::vector<std::shared_ptr<bu::model>> &models);

private:
	void draw_mesh_info(const std::shared_ptr<bu::mesh> &mesh);

	std::weak_ptr<bu::model> m_selected_model;
	bu::model::mesh_material_pair *m_selected_pair;
};

}