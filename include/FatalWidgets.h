#pragma once

#include <string>
#include "imgui.h"

namespace FatalWidgets {
    struct TextRenderer {
        using DrawTextFn = void(*)(ImDrawList* draws, const char* text, ImVec2 pos, ImU32 color);
        using CalcTextSizeFn = ImVec2(*)(const char* text);
        using GetTextWidthFn = float(*)(const char* text);
        using GetFontHeightFn = float(*)();

        DrawTextFn DrawText;
        CalcTextSizeFn CalcTextSize;
        GetTextWidthFn GetTextWidth;
        GetFontHeightFn GetFontHeight;
    };
    namespace Utils {
        std::string doubleToText(double value);
    }

    namespace Rendering {
        TextRenderer* GetDefaultTextRenderer();

        void DrawFatalTitle(
	        const char* text, const ImVec2& mainTextPos,
	        float animation, float animationOffset,
	        ImU32 mainColor, ImU32 colorA, ImU32 colorB,
	        ImDrawList* draws = ImGui::GetWindowDrawList(), TextRenderer* textRenderer = GetDefaultTextRenderer()
		);

        void DrawFatalSlider(
	        double value, const char* valueText,
	        const ImVec2& pos, const ImVec2& size,
	        ImU32 highlightedColor, ImU32 backgroundColor,
	        ImU32 highlightedTextColor, ImU32 textColor, ImU32 textShadowColor,
	        ImDrawList* draws = ImGui::GetWindowDrawList(), TextRenderer* textRenderer = GetDefaultTextRenderer()
		);

        void DrawFatalCheckBox(
	        bool value,
	        const ImVec2& pos, const ImVec2& size,
	        ImU32 backgroundColor, ImU32 arrowColor,
	        ImDrawList* draws = ImGui::GetWindowDrawList()
		);

        void DrawFatalComboBox(
	        const char* previewText,
	        const ImVec2& pos, const ImVec2& size,
	        ImU32 backgroundColor, ImU32 textColor, ImU32 arrowColor,
	        ImDrawList* draws = ImGui::GetWindowDrawList(), TextRenderer* textRenderer = GetDefaultTextRenderer()
		);

        void DrawRadioButtonB(
	        const char* text, bool highlight,
	        const ImVec2& pos, const ImVec2& size,
	        ImU32 textColor, ImU32 highlightTextColor, ImU32 indicatorColor,
	        ImDrawList* draws = ImGui::GetWindowDrawList(), TextRenderer* textRenderer = GetDefaultTextRenderer()
		);
        void DrawRadioButtonL(
	        const char* text, bool highlight,
	        const ImVec2& pos, const ImVec2& size,
	        ImU32 textColor, ImU32 highlightTextColor, ImU32 indicatorColor,
	        ImDrawList* draws = ImGui::GetWindowDrawList(), TextRenderer* textRenderer = GetDefaultTextRenderer()
		);

		void DrawIconButton(
			const char* icon, const char* text, bool activated,
			const ImVec2& pos, const ImVec2& size,
			ImFont* iconFont,
			ImU32 iconColor, ImU32 textColor,
			ImU32 iconHighlightColor, ImU32 textHighlightColor, ImU32 backgroundHighlightColor,
			ImDrawList* draws = ImGui::GetWindowDrawList(), TextRenderer* textRenderer = GetDefaultTextRenderer()
		);

        void DrawGroupOverlay(
	        const char* title,
	        const ImVec2& rectMin, const ImVec2& rectMax,
	        ImU32 backgroundColor, ImU32 borderColor, ImU32 outlineColor, ImU32 textColor,
	        ImDrawList* draws = ImGui::GetWindowDrawList(), TextRenderer* textRenderer = GetDefaultTextRenderer()
		);
    }

    bool DoubleSlider(
	    const char* label,
	    double minValue, double maxValue, double* value,
	    const ImVec2& size,
	    std::string(*toTextFn)(double) = &Utils::doubleToText,
	    ImU32 highlightedColor = 0xFF6666FF, ImU32 backgroundColor = 0xFF141414,
	    ImU32 highlightedTextColor = 0xFFFFFFFF, ImU32 textColor = 0xFF4D4D4D, ImU32 textShadowColor = 0xFF4D4D4D,
	    TextRenderer* textRenderer = Rendering::GetDefaultTextRenderer()
	);

    bool CheckBox(
	    const char* label,
	    bool* value, const ImVec2& size,
	    ImU32 backgroundColor = 0xFF141414, ImU32 arrowColor = 0xFFFFFFFF
	);

    bool ComboBox(
	    const char* label, const char* popupLabel,
	    const char* previewText,
	    const ImVec2& size,
	    void(*onPreBeginPopup)() = nullptr,
	    ImU32 backgroundColor = 0xFF141414, ImU32 textColor = 0xFF666666, ImU32 arrowColor = 0xFFCCCCCC,
	    TextRenderer* textRenderer = Rendering::GetDefaultTextRenderer()
	);

    bool RadioButtonB(
	    const char* label, const char* text,
	    int* selected, int index,
	    const ImVec2& size = ImVec2(0, 0),
	    ImU32 textColor = 0xFF666666, ImU32 highlightTextColor = 0xFFFFFFFF, ImU32 indicatorColor = 0xFF6666FF,
	    TextRenderer* textRenderer = Rendering::GetDefaultTextRenderer()
	);
    bool RadioButtonL(
	    const char* label, const char* text,
	    int* selected, int index,
	    const ImVec2& size = ImVec2(0, 0),
	    ImU32 textColor = 0xFF666666, ImU32 highlightTextColor = 0xFFFFFFFF, ImU32 indicatorColor = 0xFF6666FF,
	    TextRenderer* textRenderer = Rendering::GetDefaultTextRenderer()
	);

    bool IconButton(
        const char* label,
        const char* icon, const char* text,
        bool activated,
		ImFont* iconFont,
        const ImVec2& size = ImVec2(0, 0),
        ImU32 iconColor = 0xFF666666, ImU32 textColor = 0xFF666666,
        ImU32 iconHighlightColor = 0xFF6666FF, ImU32 textHighlightColor = 0xFFFAFAFA, ImU32 backgroundHighlightColor = 0xFF242424,
        TextRenderer* textRenderer = Rendering::GetDefaultTextRenderer()
    );

    void BeginGroup(const char* childLabel, const ImVec2& size, TextRenderer* textRenderer = Rendering::GetDefaultTextRenderer());
    void EndGroup(
        const char* title,
        ImU32 backgroundColor = 0xFF1A1A1A, ImU32 borderColor = 0xFF262626, ImU32 textColor = 0xFFFAFAFA,
        TextRenderer* textRenderer = Rendering::GetDefaultTextRenderer()
	);

}