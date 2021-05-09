#include "Assets/Assets.h"

namespace adp {

// ====================================================================================================================
// Brushes.
// ====================================================================================================================

#define BRUSH(name) const wxBrush& Brushes::name() { return brushes->name; }

struct BrushData
{
	wxBrush SensorOff = wxBrush(wxColor(230, 160, 100), wxBRUSHSTYLE_SOLID);
	wxBrush SensorOn = wxBrush(wxColor(250, 230, 190), wxBRUSHSTYLE_SOLID);
	wxBrush SensorBar = wxBrush(wxColor(25, 25, 25), wxBRUSHSTYLE_SOLID);
	wxBrush ReleaseMargin = wxBrush(wxColor(50, 50, 50), wxBRUSHSTYLE_SOLID);
	wxBrush DarkGray = wxBrush(wxColor(50, 50, 50), wxBRUSHSTYLE_SOLID);
};
static BrushData* brushes;

BRUSH(SensorOff)
BRUSH(SensorOn)
BRUSH(SensorBar)
BRUSH(ReleaseMargin)
BRUSH(DarkGray)

// ====================================================================================================================
// Pens.
// ====================================================================================================================

#define PEN(name) const wxPen& Pens::name() { return pens->name; }

struct PenData
{
	wxPen Black1px = wxPen(*wxBLACK, 1);
};
static PenData* pens;

PEN(Black1px)

// ====================================================================================================================
// Files.
// ====================================================================================================================

#define FILE(name) wxMemoryInputStream Files::name() { return OpenStream(FileData::name, sizeof(FileData::name));}

wxMemoryInputStream OpenStream(const unsigned char* data, size_t size)
{
	return wxMemoryInputStream(data, size);
}

namespace FileData
{
	#include "Assets/icon16.png.inl"
	#include "Assets/icon32.png.inl"
	#include "Assets/icon64.png.inl"
};

FILE(Icon16)
FILE(Icon32)
FILE(Icon64)

// ====================================================================================================================
// Assets.
// ====================================================================================================================

void Assets::Init()
{
	brushes = new BrushData();
	pens = new PenData();
}

void Assets::Shutdown()
{
	delete brushes;
	brushes = nullptr;

	delete pens;
	pens = nullptr;
}

}; // namespace adp.