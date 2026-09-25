#pragma once

#include "View/Colors.h"

namespace adp {

constexpr int GRAPH_COLORS_SIZE = 8;

class UserConfig {
  public:
    static bool Init();

    static bool SaveToDisk();

    static bool LoadFromDisk();

    static RgbColorf SensorOn;
    static RgbColorf SensorOff;
    static RgbColorf SensorBar;
    static RgbColorf GraphActive;

    static int WindowWidth;
    static int WindowHeight;

    static RgbColorf& GetGraphColor(size_t index) {
      return GraphColors[index % GraphColors.size()];
    }

  private:
    static std::vector<RgbColorf> GraphColors;
    static std::filesystem::path configDir;
    static std::filesystem::path configPath;
};

} // namespace adp
