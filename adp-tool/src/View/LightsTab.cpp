#include <Adp.h>

#include <imgui.h>
#include <fmt/core.h>

#include <Model/Reporter.h>
#include <Model/Device.h>

#include <View/Colors.h>
#include <View/LightsTab.h>

namespace adp {

struct LedMappingWithIndex
{
	int mappingIndex;
	LedMapping mapping;
};

static void DrawColorPicker(const char* id, int ruleIndex, LightRule& rule, RgbColor& color)
{
	auto colorf = RgbColorf(color);
	if (ImGui::ColorEdit3(id, colorf.rgb, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
	{
		color = colorf.ToRGB();
		Device::SendLightRule(ruleIndex, rule);
	}
}

static void DrawColorControls(
	const char* label,
	int ruleIndex,
	LightRule& rule,
	RgbColor& baseColor,
	RgbColor& fadeColor,
	bool& fade)
{
	ImGui::TextUnformatted(label);
	ImGui::SameLine(100);

	auto fadeId = fmt::format("Fade##Fade{}LR{}", label, ruleIndex);
	if (ImGui::Checkbox(fadeId.data(), &fade))
		Device::SendLightRule(ruleIndex, rule);

	ImGui::SameLine(200);
	auto baseColId = fmt::format("Base Color##{}BaseColorLR{}", label, ruleIndex);
	DrawColorPicker(baseColId.data(), ruleIndex, rule, baseColor);
	if (fade)
	{
		ImGui::SameLine();
		auto fadeColId = fmt::format("Fade Color##{}FadeColorLR{}", label, ruleIndex);
		DrawColorPicker(fadeColId.data(), ruleIndex, rule, fadeColor);
	}
}

static bool DrawLedMappingControls(
	LedMapping& mapping,
	int mappingIndex,
	const PadState* pad)
{
	string options;
	for (int i = 1; i <= pad->numSensors; ++i)
		options.append(fmt::format("Sensor {}{}", i, '\0'));

	ImGui::TextUnformatted("Map");

	ImGui::SameLine();
	ImGui::PushItemWidth(120);
	string sensorId = fmt::format("to LED##SensorLM{}", mappingIndex);
	if (ImGui::Combo(sensorId.data(), &mapping.sensorIndex, options.data()))
		Device::SendLedMapping(mappingIndex, mapping);

	ImGui::SameLine();
	ImGui::PushItemWidth(120);
	string ledFromId = fmt::format("up to##LedFromLM{}", mappingIndex);
	if (ImGui::InputInt(ledFromId.data(), &mapping.ledIndexBegin, 1, 1))
		Device::SendLedMapping(mappingIndex, mapping);

	ImGui::SameLine();
	ImGui::PushItemWidth(120);
	string ledToId = fmt::format("##LedToLM{}", mappingIndex);
	if (ImGui::InputInt(ledToId.data(), &mapping.ledIndexEnd, 1, 1))
		Device::SendLedMapping(mappingIndex, mapping);

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 100);
	string removeId = fmt::format("Remove##RemoveLM{}", mappingIndex);
	return ImGui::Button(removeId.data(), ImVec2(100, 0));
}

void LightsTab::Render()
{
	auto lights = Device::Lights();
	auto pad = Device::Pad();

	map<int, vector<LedMappingWithIndex>> ledMappings; // key is rule mappingIndex.
	for (auto& [i, mapping] : lights->ledMappings)
		ledMappings[mapping.lightRuleIndex].push_back({ i, mapping });

	vector<int> lightRulesToRemove;
	vector<int> ledMappingsToRemove;
	vector<LedMapping> ledMappingsToAdd;
	for (auto [ruleIndex, rule] : lights->lightRules)
	{
		ImGui::BeginGroup();
		ImGui::Separator();

		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 100);
		auto removeId = fmt::format("Remove##RemoveLR{}", ruleIndex);
		if (ImGui::Button(removeId.data(), ImVec2(100, 0)))
			lightRulesToRemove.push_back(ruleIndex);
		ImGui::SameLine();
		ImGui::SetCursorPosX(8);

		DrawColorControls("Idle color", ruleIndex, rule, rule.offColor, rule.offFadeColor, rule.fadeOff);
		DrawColorControls("Press color", ruleIndex, rule, rule.onColor, rule.onFadeColor, rule.fadeOn);

		for (auto mapping : ledMappings[ruleIndex])
		{
			if (DrawLedMappingControls(mapping.mapping, mapping.mappingIndex, pad))
				ledMappingsToRemove.push_back(mapping.mappingIndex);
		}

		string addMappingId = fmt::format("+ Add LED Mapping##AddMappingLR{}", ruleIndex);
		if (ImGui::Button(addMappingId.data()))
		{
			LedMapping toAdd = {};
			toAdd.lightRuleIndex = ruleIndex;
			ledMappingsToAdd.push_back(toAdd);
		}

		ImGui::EndGroup();
	}

	for (auto& i : lightRulesToRemove)
		Device::DisableLightRule(i);

	for (auto& i : ledMappingsToRemove)
		Device::DisableLedMapping(i);

	ImGui::Separator();
	if (ImGui::Button("+ Add Light Setting"))
	{
		for (int i = 0; i < MAX_LIGHT_RULES; ++i)
		{
			if (lights->lightRules.contains(i)) continue;
			LightRule rule = {};
			rule.onColor = RgbColor(100, 0, 0);
			rule.offColor = RgbColor(0, 0, 100);
			Device::SendLightRule(i, rule);
			break;
		}
	}

	for (auto& mapping : ledMappingsToAdd)
	{
		for (int i = 0; i < MAX_LED_MAPPINGS; ++i)
		{
			if (lights->ledMappings.contains(i)) continue;
			Device::SendLedMapping(i, mapping);
			break;
		}
	}
}

}; // namespace adp.
