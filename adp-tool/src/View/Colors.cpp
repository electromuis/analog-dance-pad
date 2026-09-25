#include <Adp.h>

#include <View/Colors.h>

using namespace std;

namespace adp {

RgbColorf::RgbColorf()
{
	rgb[0] = rgb[1] = rgb[2] = 0.f;
}

RgbColorf::RgbColorf(const RgbColor& color)
{
	rgb[0] = color.red * (1.f / 255);
	rgb[1] = color.green * (1.f / 255);
	rgb[2] = color.blue * (1.f / 255);
}

RgbColorf::RgbColorf(float r, float g, float b)
{
	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

ImU32 RgbColorf::ToU32() const
{
	return ImGui::ColorConvertFloat4ToU32(ImVec4(rgb[0], rgb[1], rgb[2], 1.0f));
}

RgbColor RgbColorf::ToRGB() const
{
	return RgbColor(
		uint8_t(clamp(int(rgb[0] * 255.f), 0, 255)),
		uint8_t(clamp(int(rgb[1] * 255.f), 0, 255)),
		uint8_t(clamp(int(rgb[2] * 255.f), 0, 255)));
}

}; // namespace adp.
