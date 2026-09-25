#include <View/UserConfig.h>

#include <filesystem>
#include <fstream>

#ifndef __EMSCRIPTEN__
	#include <ranges>
	#include <sago/platform_folders.h>
#endif

#include <nlohmann/json.hpp>
#include <fmt/core.h>

namespace adp {

bool UserConfig::Init() {
#ifdef __EMSCRIPTEN__
  return true; // no filesystem in WASM, static defaults are used
#else
  configDir = std::filesystem::path(sago::getConfigHome()) / "adp_tool";

  if (!std::filesystem::exists(configDir)) {
    std::filesystem::create_directories(configDir);
  }

  configPath = configDir /  "config.json";
  return UserConfig::LoadFromDisk();
#endif
}

RgbColorf ReadColor(const nlohmann::json& j, const std::string& key, const RgbColorf& defaultColor) {
  if (j.contains(key)) {
    auto value = j[key];
    if (value.is_array() && value.size() == 3) {
      float r = value[0];
      float g = value[1];
      float b = value[2];
      return RgbColorf(r, g, b);
    }
  }
  return defaultColor;
}

void WriteColor(nlohmann::json& j, const std::string& key, const RgbColorf& color) {
  j[key] = { color.rgb[0], color.rgb[1], color.rgb[2] };
}


bool UserConfig::LoadFromDisk() {
#ifdef __EMSCRIPTEN__
  return true; // no filesystem in WASM, static defaults are used
#else
  if (!std::filesystem::exists(configPath)) {
    return false;
  }

  nlohmann::json j;
  try {
    std::ifstream inputStream(configPath);
    inputStream >> j;
  } catch (const std::exception& e) {
    return false;
  }

  UserConfig::SensorOn = ReadColor(j, "SensorOn", UserConfig::SensorOn);
  UserConfig::SensorOff = ReadColor(j, "SensorOff", UserConfig::SensorOff);
  UserConfig::SensorBar = ReadColor(j, "SensorBar", UserConfig::SensorBar);

  UserConfig::WindowWidth = j.value("WindowWidth", UserConfig::WindowWidth);
  UserConfig::WindowHeight = j.value("WindowHeight", UserConfig::WindowHeight);

  return true;
#endif
}

bool UserConfig::SaveToDisk() {
#ifdef __EMSCRIPTEN__
  return false;
#else
  nlohmann::json j;

  WriteColor(j, "SensorOn", UserConfig::SensorOn);
  WriteColor(j, "SensorOff", UserConfig::SensorOff);
  WriteColor(j, "SensorBar", UserConfig::SensorBar);

  j["GraphColors"] = nlohmann::json::object();
  for (size_t i = 0; i < std::size(UserConfig::GraphColors); ++i) {
    auto& color = UserConfig::GraphColors[i];
    WriteColor(j["GraphColors"], fmt::format("{}", i), color);
  }

  j["WindowWidth"] = UserConfig::WindowWidth;
  j["WindowHeight"] = UserConfig::WindowHeight;

  // Open file
  std::ofstream outputStream(configPath);
  if (!outputStream) {
    return false;
  }

  outputStream << j.dump(4);
  return true;
#endif
}

std::filesystem::path UserConfig::configDir = "";
std::filesystem::path UserConfig::configPath = "";

int UserConfig::WindowWidth = 800;
int UserConfig::WindowHeight = 800;

RgbColorf UserConfig::SensorOn = RgbColorf(0.98f, 0.902f, 0.745f);
RgbColorf UserConfig::SensorOff = RgbColorf(0.902f, 0.627f, 0.392f);
RgbColorf UserConfig::SensorBar = RgbColorf(0.098f, 0.098f, 0.098f);
RgbColorf UserConfig::GraphActive = RgbColorf(0.12f, 0.70f, 0.24f);

// SENSOR_COUNT_MAX == 32
// and SENSOR_COUNT_V1 == 12
// For now, we store 8 colors (2 for each direction is enough for most use cases).
// For counts above 8 we will just cycle through the colors.
std::vector<RgbColorf> UserConfig::GraphColors = {
    RgbColorf(0.91f, 0.30f, 0.24f),
    RgbColorf(0.80f, 0.80f, 0.44f),
    RgbColorf(0.20f, 0.60f, 0.86f),
    RgbColorf(0.61f, 0.35f, 0.71f),
    RgbColorf(1.00f, 0.50f, 0.00f),
    RgbColorf(0.56f, 0.56f, 0.56f),
    RgbColorf(0.20f, 0.80f, 0.60f),
    RgbColorf(0.90f, 0.49f, 0.13f),
};

} // namespace adp.
