#include "model_import_dialog.hpp"
#include "assimp_loader.hpp"
#include "log.hpp"

using bu::ui::model_import_dialog;

void model_import_dialog::draw(std::shared_ptr<bu::scene> scene, bool open)
{
	if (open)
	{
		m_scene = scene;
		static ImVec4 file_highlight(0.8, 0, 0.3, 0.9);
		m_dialog.SetExtentionInfos(".obj", file_highlight);
		m_dialog.SetExtentionInfos(".dae", file_highlight);
		m_dialog.SetExtentionInfos(".gltf", file_highlight);
		m_dialog.SetExtentionInfos(".glb", file_highlight);

		m_dialog.OpenDialog("import_model",
			"Import a model from file",
			"GLTF (*.gltf *.glb){.gltf,.glb}"
			"OBJ files (*.obj){.obj},"
			"Collada files (*.dae){.dae},",
			""
		);
	}

	if (m_dialog.Display("import_model"))
	{
		if (m_dialog.IsOk())
		{
			std::string path = m_dialog.GetFilePathName();
			if (auto scene = m_scene.lock())
			{
				try
				{
					scene->root_node->add_child(bu::load_mesh_from_file(path));
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

		m_dialog.Close();
	}
}