#pragma once
#include <string>

namespace bu {

/**
	\brief The configuration read from the INI file
*/
struct bunsen_config
{
	struct
	{
		int resx = 1280; //!< Default window width
		int resy = 720;  //!< Default window height
		int msaa = 0;    //!< Default MSAA setting

		std::string shader_dir = "resources/shaders"; //!< Relative path to shader directory
	} general;

	//! Theme configuration
	struct
	{
		float r = 0.384;
		float g = 0.333;
		float b = 0.449;
	} theme;
};

/**
	\brief Updates configuration from provided INI file
	\returns true on success
*/
bool read_config_from_file(bunsen_config &cfg, const std::string &path);

}