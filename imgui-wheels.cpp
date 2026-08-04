#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <math.h>

#include "imgui-wheels.h"

namespace ImGuiWheels {

namespace detail {

struct drag_state {
    ImGuiID active_id = 0;
    float start_mouse = 0.0f;
    float start_value = 0.0f;
};

struct palette_state {
    ImVec4 base_bg = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 shadow = ImVec4(0.02f, 0.02f, 0.02f, 0.75f);
    ImVec4 highlight = ImVec4(1.0f, 1.0f, 1.0f, 0.25f);
};

struct wheel_state {
    ImGuiWindow* window = nullptr;
    ImGuiContext* context = nullptr;
    ImGuiID id = 0;
    ImRect wheel_bb;
    ImRect total_bb;
    ImVec2 label_size;
    ImVec2 pos;
    float width = 0.0f;
    float height = 0.0f;
    bool is_horizontal = true;
    bool hovered = false;
    bool held = false;
    bool value_changed = false;

    wheel_state(const char* label, ImVec2 size, WheelOrientation orientation) {
        window = ImGui::GetCurrentWindow();
        context = GImGui;
        if (window == nullptr || context == nullptr || window->SkipItems) {
            return;
        }

        width = size.x;
        height = size.y;
        is_horizontal = (orientation == Horizontal);
        id = window->GetID(label);
        label_size = ImGui::CalcTextSize(label, NULL, true);
        pos = window->DC.CursorPos;

        const ImGuiStyle& style = context->Style;
        total_bb = ImRect(pos, ImVec2(pos.x + width + (label_size.x > 0 ? style.ItemInnerSpacing.x + label_size.x : 0), pos.y + height));
        ImGui::ItemSize(total_bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(total_bb, id)) {
            window = nullptr;
            context = nullptr;
            return;
        }

        wheel_bb = ImRect(pos, ImVec2(pos.x + width, pos.y + height));
        value_changed = ImGui::ButtonBehavior(wheel_bb, id, &hovered, &held);
    }

    bool valid() const {
        return window != nullptr && context != nullptr;
    }
};

static ImVec4 get_reference_style_color(bool use_dark_reference, ImGuiCol color_index) {
    ImGuiStyle reference_style;
    if (use_dark_reference) {
        ImGui::StyleColorsDark(&reference_style);
    } else {
        ImGui::StyleColorsLight(&reference_style);
    }
    return reference_style.Colors[color_index];
}

static float apply_drag_sensitivity_curve(float normalized_delta) {
    const float magnitude = fabsf(normalized_delta);
    const float curved = powf(magnitude, 1.35f);
    return (normalized_delta < 0.0f) ? -curved : curved;
}

static bool update_drag(const wheel_state& state, ImGuiID id, float speed, float v_min, float v_max, float* value) {
    if (value == nullptr) {
        return false;
    }

    static drag_state drag;
    if (ImGui::IsItemActivated()) {
        drag.active_id = id;
        drag.start_mouse = state.is_horizontal ? state.context->IO.MousePos.x : state.context->IO.MousePos.y;
        drag.start_value = *value;
    }

    bool value_changed = false;
    if (state.held) {
        const float track_length = state.is_horizontal ? state.width : state.height;
        const float mouse_position = state.is_horizontal ? state.context->IO.MousePos.x : state.context->IO.MousePos.y;
        const float mouse_distance = mouse_position - drag.start_mouse;
        const float normalized_distance = (track_length > 0.0f && speed > 0.0f)
            ? (mouse_distance / (track_length * speed))
            : 0.0f;
        const float curved_distance = apply_drag_sensitivity_curve(normalized_distance);
        const float value_delta = curved_distance * (v_max - v_min);
        const float dragged_value = std::clamp(drag.start_value + value_delta, v_min, v_max);
        if (*value != dragged_value) {
            *value = dragged_value;
            value_changed = true;
        }
    }

    if (!state.held && drag.active_id == id) {
        drag.active_id = 0;
    }

    return value_changed;
}

static palette_state resolve_palette(const wheel_state& state, bool invert_colors) {
    palette_state palette;
    const ImGuiCol background_color_index = state.held ? ImGuiCol_FrameBgActive : (state.hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    const ImVec4 current_frame_bg = ImGui::GetStyle().Colors[ImGuiCol_FrameBg];
    const float current_luminance = (0.2126f * current_frame_bg.x) + (0.7152f * current_frame_bg.y) + (0.0722f * current_frame_bg.z);
    const bool current_is_light = current_luminance >= 0.5f;
    palette.base_bg = ImGui::GetStyle().Colors[background_color_index];
    palette.border = ImGui::GetStyle().Colors[ImGuiCol_Border];

    if (invert_colors) {
        const bool use_dark_reference = current_is_light;
        palette.base_bg = get_reference_style_color(use_dark_reference, background_color_index);
        palette.border = get_reference_style_color(use_dark_reference, ImGuiCol_Border);
    }

    const float resolved_luminance = (0.2126f * palette.base_bg.x) + (0.7152f * palette.base_bg.y) + (0.0722f * palette.base_bg.z);
    if (resolved_luminance > 0.5f) {
        palette.shadow = ImVec4(0.0f, 0.0f, 0.0f, 0.70f);
        palette.highlight = ImVec4(0.0f, 0.0f, 0.0f, 0.20f);
    } else {
        palette.shadow = ImVec4(1.0f, 1.0f, 1.0f, 0.65f);
        palette.highlight = ImVec4(1.0f, 1.0f, 1.0f, 0.20f);
    }

    return palette;
}

static void draw_barrel(const wheel_state& state, const palette_state& palette, float value, float v_min, float v_max) {
    ImGuiWindow* window = state.window;
    const ImRect& wheel_bb = state.wheel_bb;
    const float width = state.width;
    const float height = state.height;
    const bool is_horizontal = state.is_horizontal;

    const int segments = 32;
    ImVec2 min_points[segments + 1];
    ImVec2 max_points[segments + 1];

    const float main_axis_length = is_horizontal ? width : height;
    const float cross_axis_center = is_horizontal ? (wheel_bb.Min.y + height * 0.5f) : (wheel_bb.Min.x + width * 0.5f);
    const float cross_axis_max_height = is_horizontal ? height : width;

    for (int px = 0; px <= static_cast<int>(main_axis_length); px++) {
        const float rel_p = px / main_axis_length;
        const float cos_a = rel_p * 2.0f - 1.0f;
        const float sin_a = sqrtf(ImMax(0.0f, 1.0f - cos_a * cos_a));

        float base_half_thick = (cross_axis_max_height * 0.5f) - 2.0f;
        const float dynamic_half_thick = base_half_thick * (0.4f + 0.6f * sin_a);
        const float thick_min = cross_axis_center - dynamic_half_thick - 2.0f;
        const float thick_max = cross_axis_center + dynamic_half_thick + 2.0f;

        const float shadow_factor = powf(sin_a, 0.4f);
        ImVec4 shaded_v4 = palette.base_bg;
        const float darken_floor = 0.35f;
        const float blend_mult = 1.0f - darken_floor;
        shaded_v4.x *= (darken_floor + blend_mult * shadow_factor);
        shaded_v4.y *= (darken_floor + blend_mult * shadow_factor);
        shaded_v4.z *= (darken_floor + blend_mult * shadow_factor);
        const ImU32 column_color = ImGui::ColorConvertFloat4ToU32(shaded_v4);

        const ImVec2 line_start = is_horizontal ? ImVec2(wheel_bb.Min.x + px, thick_min) : ImVec2(thick_min, wheel_bb.Min.y + px);
        const ImVec2 line_end = is_horizontal ? ImVec2(wheel_bb.Min.x + px, thick_max) : ImVec2(thick_max, wheel_bb.Min.y + px);
        window->DrawList->AddLine(line_start, line_end, column_color, 1.0f);

        for (int s = 0; s <= segments; s++) {
            if (px == static_cast<int>(main_axis_length * (static_cast<float>(s) / segments))) {
                min_points[s] = line_start;
                max_points[s] = line_end;
            }
        }
    }

    const float range = v_max - v_min;
    const float percent = (range != 0.0f) ? ((value - v_min) / range) : 0.0f;
    const float rotation_offset = -percent * static_cast<float>(M_PI) * 4.0f;
    const int num_grooves = 20;

    for (int i = 0; i < num_grooves; i++) {
        const float angle = (static_cast<float>(i) / static_cast<float>(num_grooves)) * static_cast<float>(M_PI) * 2.0f + rotation_offset;
        const float cos_a = cosf(angle);
        const float sin_a = sinf(angle);

        if (sin_a > 0.0f) {
            const float relative_p = (cos_a + 1.0f) * 0.5f;
            const float groove_p = (is_horizontal ? wheel_bb.Min.x : wheel_bb.Min.y) + relative_p * main_axis_length;

            float base_half_thick = (cross_axis_max_height * 0.5f) - 2.0f;
            if (i % 2 != 0) {
                base_half_thick *= 0.6f;
            }

            const float dynamic_half_thick = base_half_thick * (0.4f + 0.6f * sin_a);
            const float thick_min = cross_axis_center - dynamic_half_thick;
            const float thick_max = cross_axis_center + dynamic_half_thick;

            ImVec4 shadow_faded = palette.shadow;
            ImVec4 highlight_faded = palette.highlight;
            const float line_shading = powf(sin_a, 1.5f);
            shadow_faded.w *= line_shading;
            highlight_faded.w *= line_shading;

            const ImU32 current_shadow = ImGui::ColorConvertFloat4ToU32(shadow_faded);
            const ImU32 current_highlight = ImGui::ColorConvertFloat4ToU32(highlight_faded);

            const ImVec2 grv_start = is_horizontal ? ImVec2(groove_p, thick_min) : ImVec2(thick_min, groove_p);
            const ImVec2 grv_end = is_horizontal ? ImVec2(groove_p, thick_max) : ImVec2(thick_max, groove_p);
            const ImVec2 hlt_start = is_horizontal ? ImVec2(groove_p + 1.0f, thick_min) : ImVec2(thick_min, groove_p + 1.0f);
            const ImVec2 hlt_end = is_horizontal ? ImVec2(groove_p + 1.0f, thick_max) : ImVec2(thick_max, groove_p + 1.0f);

            window->DrawList->AddLine(grv_start, grv_end, current_shadow, 2.0f);
            window->DrawList->AddLine(hlt_start, hlt_end, current_highlight, 1.0f);
        }
    }

    const ImU32 border_color = ImGui::ColorConvertFloat4ToU32(palette.border);
    for (int i = 0; i < segments; ++i) {
        window->DrawList->AddLine(min_points[i], min_points[i + 1], border_color, 1.0f);
        window->DrawList->AddLine(max_points[i], max_points[i + 1], border_color, 1.0f);
    }
    window->DrawList->AddLine(min_points[0], max_points[0], border_color, 1.0f);
    window->DrawList->AddLine(min_points[segments], max_points[segments], border_color, 1.0f);
}

static void draw_label(const wheel_state& state, const ImGuiStyle& style, const char* label, const char* format, float value, bool is_test_value, bool is_integer_value = false) {
    if (state.label_size.x <= 0.0f) {
        return;
    }

    char val_buf[64];
    if (is_integer_value) {
        ImFormatString(val_buf, IM_ARRAYSIZE(val_buf), format, static_cast<int>(std::round(value)));
    } else {
        ImFormatString(val_buf, IM_ARRAYSIZE(val_buf), format, value);
    }

    char text_buf[256];
    if (is_test_value) {
        ImFormatString(text_buf, IM_ARRAYSIZE(text_buf), "%s (TEST): %s", label, val_buf);
    } else {
        ImFormatString(text_buf, IM_ARRAYSIZE(text_buf), "%s: %s", label, val_buf);
    }

    ImGui::RenderText(ImVec2(state.wheel_bb.Max.x + style.ItemInnerSpacing.x, state.wheel_bb.Min.y + (state.height - state.label_size.y) * 0.5f), text_buf);
}

} // namespace detail

bool WheelFloat(const char* label, float* v, float v_min, float v_max, ImVec2 size, WheelOrientation orientation, const char* format, float speed, bool invert_colors) {
    detail::wheel_state state(label, size, orientation);
    if (!state.valid()) {
        return false;
    }

    ImGuiWindow* window = state.window;
    ImGuiContext& g = *state.context;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = state.id;

    // Testing Fallback
    static float fallback_testing_value = 0.0f;
    float* target_v = (v != nullptr) ? v : &fallback_testing_value;
    bool value_changed = state.value_changed;
    value_changed = detail::update_drag(state, id, speed, v_min, v_max, target_v) || value_changed;
    const detail::palette_state palette = detail::resolve_palette(state, invert_colors);
    detail::draw_barrel(state, palette, *target_v, v_min, v_max);
    detail::draw_label(state, style, label, format, *target_v, v == nullptr, false);

    return value_changed;
}

bool WheelFloatHorizontal(const char* label, float* v, float v_min, float v_max, ImVec2 size, const char* format, float speed, bool invert_colors) {
    return WheelFloat(label, v, v_min, v_max, size, Horizontal, format, speed, invert_colors);
}

bool WheelFloatVertical(const char* label, float* v, float v_min, float v_max, ImVec2 size, const char* format, float speed, bool invert_colors) {
    return WheelFloat(label, v, v_min, v_max, size, Vertical, format, speed, invert_colors);
}

bool WheelInt(const char* label, int* v, int v_min, int v_max, ImVec2 size, WheelOrientation orientation, const char* format, float speed, bool invert_colors) {
    if (v == nullptr) {
        return WheelFloat(label, nullptr, static_cast<float>(v_min), static_cast<float>(v_max), size, orientation, "%.0f", speed, invert_colors);
    }

    detail::wheel_state state(label, size, orientation);
    if (!state.valid()) {
        return false;
    }

    ImGuiContext& g = *state.context;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = state.id;

    float float_value = static_cast<float>(*v);
    bool value_changed = state.value_changed;
    value_changed = detail::update_drag(state, id, speed, static_cast<float>(v_min), static_cast<float>(v_max), &float_value) || value_changed;
    if (value_changed) {
        *v = static_cast<int>(std::round(float_value));
        *v = std::clamp(*v, v_min, v_max);
        float_value = static_cast<float>(*v);
    }

    const detail::palette_state palette = detail::resolve_palette(state, invert_colors);
    detail::draw_barrel(state, palette, float_value, static_cast<float>(v_min), static_cast<float>(v_max));
    detail::draw_label(state, style, label, format, float_value, false, true);
    return value_changed;
}

bool WheelIntHorizontal(const char* label, int* v, int v_min, int v_max, ImVec2 size, const char* format, float speed, bool invert_colors) {
    return WheelInt(label, v, v_min, v_max, size, Horizontal, format, speed, invert_colors);
}

bool WheelIntVertical(const char* label, int* v, int v_min, int v_max, ImVec2 size, const char* format, float speed, bool invert_colors) {
    return WheelInt(label, v, v_min, v_max, size, Vertical, format, speed, invert_colors);
}

} // namespace ImGuiWheels