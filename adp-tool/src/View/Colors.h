#pragma once

#include <Adp.h>

#include <Model/Device.h>

#include <imgui.h>

namespace adp {

struct RgbColorf
{
	RgbColorf();
	RgbColorf(const RgbColor& color);
	RgbColorf(float r, float g, float b);

	float rgb[3] = { 1,1,1 };
	ImU32 ToU32() const;
	RgbColor ToRGB() const;

	static RgbColorf SensorOn;
	static RgbColorf SensorOff;
	static RgbColorf SensorBar;
};

}; // namespace adp.
