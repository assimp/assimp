
#include <string>
#include <map>

#include "imgui.h"

#include "tinyusdz.hh"
#include "usdShade.hh"

namespace example {

template<typename T>
bool ImGuiComboUI(const std::string &caption, std::string &current_key,
                         const std::map<std::string, T> &items) {
  bool changed = false;

  if (ImGui::BeginCombo(caption.c_str(), current_key.c_str())) {
    for (const auto &item : items) {
      bool is_selected = (current_key == item.first);
      if (ImGui::Selectable(item.first.c_str(), is_selected)) {
        current_key = item.first;
        changed = true;
      }
      if (is_selected) {
        // Set the initial focus when opening the combo (scrolling + for
        // keyboard navigation support in the upcoming navigation branch)
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  return changed;
}


bool material_ui(tinyusdz::UsdPreviewSurface &material);

} // namesapce example
