#pragma once

namespace bu {

class modified_flag
{
public:
	virtual bool is_modified() const {return m_modified;}
	virtual void mark_as_modified() {m_modified = true;}
	virtual void clear_modified() {m_modified = false;}

protected:
	~modified_flag() = default;
	bool m_modified = true;
};

}