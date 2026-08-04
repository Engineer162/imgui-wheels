#pragma once

#include "imgui.h"
#include "imgui_internal.h"

namespace ImGuiWheels {

enum WheelOrientation_ {
    Horizontal = 0,
    Vertical   = 1,
    ImGuiWheelOrientation_Horizontal = Horizontal,
    ImGuiWheelOrientation_Vertical   = Vertical
};
typedef int WheelOrientation;
typedef WheelOrientation ImGuiWheelOrientation;

/**
 * @brief Renders a 3D barrel-shaped rolling wheel input component.
 * 
 * @param label Unique text identifier and display name.
 * @param v Pointer to the float value to modify. Can be nullptr for testing.
 * @param v_min Minimum bounding value.
 * @param v_max Maximum bounding value.
 * @param size The size of the widget layout box (X and Y dimensions).
 * @param orientation Set to ImGuiWheelOrientation_Horizontal or ImGuiWheelOrientation_Vertical.
 * @param format Printf style precision tracking string (e.g. "%.1f", "%.3f deg"). Default is "%.2f".
 * @param speed Sensitivity multiplier for mouse dragging. Default is 1.0f.
 * @param invert_colors Set to true to invert this wheel's color profile relative to your current theme. Default is false.
 * @return true if the value was actively changed during this frame.
 */
bool WheelFloat(const char* label, float* v, float v_min, float v_max, ImVec2 size, WheelOrientation orientation = Horizontal, const char* format = "%.2f", float speed = 1.0f, bool invert_colors = false);
bool WheelFloatHorizontal(const char* label, float* v, float v_min, float v_max, ImVec2 size, const char* format = "%.2f", float speed = 1.0f, bool invert_colors = false);
bool WheelFloatVertical(const char* label, float* v, float v_min, float v_max, ImVec2 size, const char* format = "%.2f", float speed = 1.0f, bool invert_colors = false);
bool WheelInt(const char* label, int* v, int v_min, int v_max, ImVec2 size, WheelOrientation orientation = Horizontal, const char* format = "%d", float speed = 1.0f, bool invert_colors = false);
bool WheelIntHorizontal(const char* label, int* v, int v_min, int v_max, ImVec2 size, const char* format = "%d", float speed = 1.0f, bool invert_colors = false);
bool WheelIntVertical(const char* label, int* v, int v_min, int v_max, ImVec2 size, const char* format = "%d", float speed = 1.0f, bool invert_colors = false);

} // namespace ImGuiWheels