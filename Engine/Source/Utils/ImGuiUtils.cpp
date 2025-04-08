#include "Utils/ImGuiUtils.h"
#include "imgui.h"

ImGuiUtils::ImGuiScope::ImGuiScope(const char* InName)
{
	ImGui::Begin(InName);
}

ImGuiUtils::ImGuiScope::~ImGuiScope()
{
	ImGui::End();
}
