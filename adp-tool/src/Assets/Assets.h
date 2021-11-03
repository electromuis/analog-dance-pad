#pragma once

#include "wx/brush.h"
#include "wx/pen.h"
#include "wx/mstream.h"
#include "wx/colour.h"

namespace adp {

struct Brushes
{
	static const wxBrush& SensorOff();
	static const wxBrush& SensorOn();
	static const wxBrush& SensorBar();
	static const wxBrush& ReleaseMargin();
	static const wxBrush& DarkGray();
};

struct Pens
{
	static const wxPen& Black1px();
	static const wxPen& White1px();
	static const wxPen& Transparent1px();
};

struct Files
{
	static wxMemoryInputStream Icon16();
	static wxMemoryInputStream Icon32();
	static wxMemoryInputStream Icon64();
};

class Assets
{
public:
	static void Init();

	static void Shutdown();
};

}; // namespace adp.
