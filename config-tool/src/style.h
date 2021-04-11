#pragma once

#include "wx/wxprec.h"

namespace style {

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

}; // namespace style.