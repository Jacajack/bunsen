#pragma once

namespace bu::ui {

class window;

/**
	\brief A widget displayed inside of a window
*/
class widget
{
public:
	widget(window &win);
	virtual ~widget() = default;

	virtual void draw();

protected:
	window &get_window() {return *m_window;}

private:
	window *m_window;

};

}