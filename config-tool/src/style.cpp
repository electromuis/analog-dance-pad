#include "style.h"

namespace style {

struct StyleData
{
	wxBrush brushSensorOff = wxBrush(wxColour(230, 160, 100), wxBRUSHSTYLE_SOLID);
	wxBrush brushSensorOn  = wxBrush(wxColour(250, 230, 190), wxBRUSHSTYLE_SOLID);
	wxBrush brushSensorBar = wxBrush(wxColour(25, 25, 25), wxBRUSHSTYLE_SOLID);
	wxBrush brushDarkGray = wxBrush(wxColor(50, 50, 50), wxBRUSHSTYLE_SOLID);

	wxPen penBlack1px = wxPen(*wxBLACK, 1);
};

static StyleData* styleData = nullptr;

void Style::Init()
{
	styleData = new StyleData();
}

void Style::Shutdown()
{
	delete styleData;
	styleData = nullptr;
}

const wxBrush& Brushes::SensorOff() { return styleData->brushSensorOff; }
const wxBrush& Brushes::SensorOn()  { return styleData->brushSensorOn; }
const wxBrush& Brushes::SensorBar() { return styleData->brushSensorBar; }
const wxBrush& Brushes::DarkGray() { return styleData->brushDarkGray; }

const wxPen& Pens::Black1px() { return styleData->penBlack1px; }

}; // namespace style.