#pragma once
#include <memory>
#include "ui/widget.hpp"
#include <ImGuiFileDialog.h>

namespace bu {
struct scene;
}

namespace bu::ui {

class model_import_dialog : public widget
{
public:
	model_import_dialog(window &win) : 
		widget(win)
	{
	}

	void draw(std::shared_ptr<bu::scene> scene, bool open = false);

private:
	ImGuiFileDialog m_dialog;
	std::weak_ptr<bu::scene> m_scene;
};

}