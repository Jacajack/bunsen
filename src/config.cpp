#include "config.hpp"
#include <memory>
#include <INIReader.h>

bool bu::read_config_from_file(bunsen_config &cfg, const std::string &path)
{
	auto ini = std::make_unique<INIReader>(path);
	if (ini->ParseError() < 0)
		return false;

	std::string section;
	auto get_int = [&section, &ini](int &value, const std::string &name)
	{
		value = ini->GetInteger(section, name, value);
	};

	auto get_str = [&section, &ini](std::string &value, const std::string &name)
	{
		value = ini->GetString(section, name, value);
	};

	auto get_flt = [&section, &ini](float &value, const std::string &name)
	{
		value = ini->GetReal(section, name, value);
	};

	// [general]
	section = "general";
	get_int(cfg.general.resx, "resx");
	get_int(cfg.general.resy, "resy");
	get_int(cfg.general.msaa, "msaa");
	get_str(cfg.general.shader_dir, "shader_dir");

	// [theme]
	section = "theme";
	get_flt(cfg.theme.r, "r");
	get_flt(cfg.theme.g, "g");
	get_flt(cfg.theme.b, "b");

	return true;
}
