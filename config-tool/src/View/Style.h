#pragma once

#include "wx/brush.h"
#include "wx/pen.h"

namespace mpc {

struct Brushes
{
	static const wxBrush& SensorOff();
	static const wxBrush& SensorOn();
	static const wxBrush& SensorBar();
	static const wxBrush& DarkGray();
};

struct Pens
{
	static const wxPen& Black1px();
};

class Style
{
public:
	static void Init();

	static void Shutdown();
};

}; // namespace mpc.