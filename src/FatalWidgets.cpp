#include "FatalWidgets.h"

//
// Created by addo6544 on 2026/6/3.
//

#include "imgui.h"

#include <string>
#include <algorithm>
#include <functional>

#include "imgui_internal.h"

using namespace FatalWidgets;

namespace FatalWidgets::Rendering {
    namespace DefaultTextRendererImpl {
        void drawText(ImDrawList* draws, const char* text, ImVec2 pos, ImU32 color) {
            draws->AddText(pos, color, text);
        }
        ImVec2 calcTextSize(const char* text) {
            return ImGui::CalcTextSize(text);
        }
        float getTextWidth(const char* text) {
            return calcTextSize(text).x;
        }
        float getFontHeight() {
            return ImGui::GetTextLineHeight();
        }
    }
    using namespace DefaultTextRendererImpl;

    auto defaultTextRenderer = TextRenderer(&drawText, &calcTextSize, &getTextWidth, &getFontHeight);
    TextRenderer* GetDefaultTextRenderer() {
        return &defaultTextRenderer;
    }
}

std::string Utils::doubleToText(double value) {
    return std::to_string(value);
}

void Rendering::DrawFatalTitle(const char* text, const ImVec2& mainTextPos, float animation, float animationOffset, ImU32 mainColor, ImU32 colorA, ImU32 colorB, ImDrawList* draws, TextRenderer* textRenderer) {
    if (animation > 0.0f) {
        static const ImVec2 offsetsA[] = {
            { 0,  0},
            {-animationOffset, -animationOffset},
            { animationOffset, -animationOffset},
            {-animationOffset,  animationOffset},
            { animationOffset,  animationOffset}
        };

        static const ImVec2 offsetsB[] = {
            { 0,  0},
            { animationOffset,  animationOffset},
            {-animationOffset,  animationOffset},
            { animationOffset, -animationOffset},
            {-animationOffset, -animationOffset}
        };

        float segmentFloat = animation * 4.0f;
        int segment = static_cast<int>(segmentFloat);

        if (segment >= 4)
            segment = 3;

        float localT = segmentFloat - static_cast<float>(segment);

        // SmoothStep
        localT = localT * localT * (3.0f - 2.0f * localT);

        const ImVec2 aOffset = ImLerp(
            offsetsA[segment],
            offsetsA[segment + 1],
            localT
        );

        const ImVec2 bOffset = ImLerp(
            offsetsB[segment],
            offsetsB[segment + 1],
            localT
        );

        textRenderer->DrawText(
            draws,
            text,
            ImVec2(mainTextPos.x + aOffset.x, mainTextPos.y + aOffset.y),
            colorA
        );

        textRenderer->DrawText(
            draws,
            text,
            ImVec2(mainTextPos.x + bOffset.x, mainTextPos.y + bOffset.y),
            colorB
        );
    }

    textRenderer->DrawText(
        draws,
        text,
        mainTextPos,
        mainColor
    );
}

void Rendering::DrawFatalSlider(double value, const char* valueText, const ImVec2& pos, const ImVec2& size, ImU32 highlightedColor, ImU32 backgroundColor, ImU32 highlightedTextColor, ImU32 textColor, ImU32 textShadowColor, ImDrawList* draws, TextRenderer* textRenderer) {
    ImVec2 rectMax = pos + size;
    draws->PushClipRect(pos, rectMax, true);

    draws->AddRectFilled(pos, rectMax, backgroundColor, 3);

    ImVec2 textSize = textRenderer->CalcTextSize(valueText);
    ImVec2 textRenderPos = ImVec2(
        pos.x + ((size.x - 2) - textSize.x) / 2,
        pos.y + ((size.y - 2) - textRenderer->GetFontHeight()) / 2);

    textRenderer->DrawText(draws, valueText, textRenderPos, textColor);

    ImVec2 valueRectMin = ImVec2(pos.x + 1, pos.y + 1), valueRectMax = ImVec2(pos.x + 1 + (size.x - 2) * value, rectMax.y - 1);
    if ((int) valueRectMax.x > (int) valueRectMin.x)
    {
        draws->PushClipRect(valueRectMin, valueRectMax, true);
        draws->AddRectFilled(valueRectMin, ImVec2(ImMax(valueRectMin.x + 4, valueRectMax.x), valueRectMax.y), highlightedColor, 2);
        if (textShadowColor != 0) {
            textRenderer->DrawText(draws, valueText, ImVec2(textRenderPos.x + 1, textRenderPos.y + 1), textShadowColor);
        }
        textRenderer->DrawText(draws, valueText, textRenderPos, highlightedTextColor);
        draws->PopClipRect();
    }

    draws->PopClipRect();
}

void Rendering::DrawFatalCheckBox(bool value, const ImVec2& pos, const ImVec2& size, ImU32 backgroundColor, ImU32 arrowColor, ImDrawList* draws) {
    const auto rectMax = ImVec2(pos.x + size.x, pos.y + size.y);
    draws->AddRectFilled(pos, rectMax, backgroundColor, 3);
    if (value) {
        const auto p1 = ImVec2(pos.x + 3, pos.y + 7);
        const auto p2 = ImVec2(pos.x + 6, pos.y + 10);
        const auto p3 = ImVec2(pos.x + 12, pos.y + 4);
        draws->PathLineTo(p1);
        draws->PathLineTo(p2);
        draws->PathLineTo(p3);
        draws->PathStroke(arrowColor, 2);
    }
}

void Rendering::DrawFatalComboBox(const char* previewText, const ImVec2& pos, const ImVec2& size, ImU32 backgroundColor, ImU32 textColor, ImU32 arrowColor, ImDrawList* draws, TextRenderer* textRenderer) {
    const auto rectMax = ImVec2(pos.x + size.x, pos.y + size.y);
    draws->AddRectFilled(pos, rectMax, backgroundColor, 3);
    textRenderer->DrawText(
        draws, previewText,
        ImVec2(pos.x + 6, pos.y + (size.y - textRenderer->GetFontHeight()) / 2),
        textColor);

    const auto arrowButtonPos = ImVec2(rectMax.x - size.y, pos.y);
    draws->AddRectFilled(arrowButtonPos, rectMax, backgroundColor, 3, ImDrawFlags_RoundCornersRight);

    const auto bgZeroAlpha = ImGui::GetColorU32(backgroundColor, 0);
    draws->AddRectFilledMultiColor(
        ImVec2(arrowButtonPos.x - 6, arrowButtonPos.y), ImVec2(arrowButtonPos.x, rectMax.y),
        bgZeroAlpha, backgroundColor, backgroundColor, bgZeroAlpha);

    const auto p1 = ImVec2(arrowButtonPos.x + 8, arrowButtonPos.y + 10);
    const auto p2 = ImVec2(arrowButtonPos.x + 11, arrowButtonPos.y + 12.5);
    const auto p3 = ImVec2(arrowButtonPos.x + 14, arrowButtonPos.y + 10);

    draws->PathLineTo(p1);
    draws->PathLineTo(p2);
    draws->PathLineTo(p3);
    draws->PathStroke(arrowColor, 1);
}

void Rendering::DrawRadioButtonB(const char* text, bool highlight, const ImVec2& pos, const ImVec2& size, ImU32 textColor, ImU32 highlightTextColor, ImU32 indicatorColor, ImDrawList* draws, TextRenderer* textRenderer) {
    const ImVec2 textSize = textRenderer->CalcTextSize(text);
    const ImVec2 rectMax = ImVec2(pos.x + size.x, pos.y + size.y);
    textRenderer->DrawText(
        draws, text,
        ImVec2(pos.x + (size.x - textSize.x) / 2, pos.y + ((size.y - 4) - textSize.y) / 2),
        highlight ? highlightTextColor : textColor
    );
    if (highlight) {
        draws->AddRectFilled(
            ImVec2(pos.x + 2, rectMax.y - 4),
            ImVec2(rectMax.x - 2, rectMax.y - 2),
            indicatorColor, 1
        );
    }
}
void Rendering::DrawRadioButtonL(const char* text, bool highlight, const ImVec2& pos, const ImVec2& size, ImU32 textColor, ImU32 highlightTextColor, ImU32 indicatorColor, ImDrawList* draws, TextRenderer* textRenderer) {
    const ImVec2 textSize = textRenderer->CalcTextSize(text);

    textRenderer->DrawText(
        draws,
        text,
        ImVec2(pos.x + 8, pos.y + (size.y - textSize.y) / 2),
        highlight ? highlightTextColor : textColor
    );
    if (highlight) {
        draws->AddRectFilled(
			pos, ImVec2(pos.x + 2, pos.y + size.y),
            indicatorColor, 1
        );
    }
}

void Rendering::DrawGroupOverlay(const char* title, const ImVec2& rectMin, const ImVec2& rectMax, ImU32 backgroundColor, ImU32 borderColor, ImU32 outlineColor, ImU32 textColor, ImDrawList* draws, TextRenderer* textRenderer) {
    const auto titleSize = textRenderer->CalcTextSize(title);
    const auto borderMin = ImVec2(rectMin.x, rectMin.y + titleSize.y / 2);
    
    // border
    {
        bool antialised = draws->Flags & ImDrawListFlags_AntiAliasedLines;
        float offset = antialised ? 0.5f : 0;

        const auto roundCenterRT = ImVec2(rectMax.x - 4 - offset, borderMin.y + 4 + offset);
        const auto roundCenterRB = rectMax - ImVec2(4 + offset, 4 + offset);
        const auto roundCenterLB = ImVec2(borderMin.x + 4 + offset, rectMax.y - 4 - offset);
        const auto roundCenterLT = borderMin + ImVec2(4 + offset, 4 + offset);

        // top r-part to right
        draws->PathLineTo(borderMin + ImVec2(20 + titleSize.x, offset));
        draws->PathLineTo(ImVec2(roundCenterRT.x - 1, borderMin.y + offset));
        draws->PathArcToFast(roundCenterRT, 4, 9, 12);

        // right to bottom
        draws->PathLineTo(ImVec2(rectMax.x - offset, roundCenterRB.y - 1));
        draws->PathArcToFast(roundCenterRB, 4, 0, 3);

        // bottom to left
        draws->PathLineTo(ImVec2(roundCenterLB.x + 1, rectMax.y - offset));
        draws->PathArcToFast(roundCenterLB, 4, 3, 6);

        // left to top
        draws->PathLineTo(ImVec2(borderMin.x + offset, roundCenterLT.y + 1));
        draws->PathArcToFast(roundCenterLT, 4, 6, 9);

        // top l-part
        draws->PathLineTo(borderMin + ImVec2(10, offset));

        draws->PathStroke(borderColor);
    }

    const auto lineFilterStart = ImVec2(rectMin.x + 11, rectMin.y + titleSize.y / 2);
    draws->AddRectFilled(lineFilterStart, ImVec2(lineFilterStart.x + titleSize.x + 8, lineFilterStart.y + 1), backgroundColor);
    textRenderer->DrawText(draws, title, ImVec2(rectMin.x + 15, rectMin.y), textColor);
}

void Rendering::DrawIconButton(const char* icon, const char* text, bool activated, const ImVec2& pos, const ImVec2& size, ImFont* iconFont, ImU32 iconColor, ImU32 textColor, ImU32 iconHighlightColor, ImU32 textHighlightColor, ImU32 backgroundHighlightColor, ImDrawList* draws, TextRenderer* textRenderer)
{
    if (activated)
        draws->AddRectFilled(pos, pos + size, backgroundHighlightColor, 4);
    const auto textSize = textRenderer->CalcTextSize(text);
    ImGui::PushFont(iconFont);
    const auto iconSize = textRenderer->CalcTextSize(icon);
    const auto contentSize = ImVec2(iconSize.x + 4 + textSize.x, ImMax(iconSize.y, textSize.y));
    const auto iconPos = pos + ImVec2((size.x - contentSize.x) / 2, (size.y - iconSize.y) / 2);
    textRenderer->DrawText(
        draws, 
        icon, iconPos,
        activated ? iconHighlightColor : iconColor
    );
    ImGui::PopFont();
    textRenderer->DrawText(
        draws, text, 
        ImVec2(iconPos.x + iconSize.x + 4, pos.y + (size.y - textSize.y) / 2),
        activated ? textHighlightColor : textColor
    );
    //draws->AddRect(ImVec2(iconPos.x + iconSize.x + 4, pos.y + (size.y - textSize.y) / 2), ImVec2(iconPos.x + iconSize.x + 4, pos.y + (size.y - textSize.y) / 2) + textSize, 0xFF0000FF);
}

bool FatalWidgets::DoubleSlider(const char* label, double minValue, double maxValue, double* value, const ImVec2& size, const std::function<std::string(double)>& toTextFn, ImU32 highlightedColor, ImU32 backgroundColor, ImU32 highlightedTextColor, ImU32 textColor, ImU32 textShadowColor, TextRenderer* textRenderer) {
    ImGui::InvisibleButton(label, size, 0);
    const bool drag = ImGui::IsItemActive() && ImGui::IsMouseDragging(0);

    const ImVec2 itemPos = ImGui::GetItemRectMin();
    const double valueInterval = maxValue - minValue;
    if (drag) {
        const double relativeX = ImGui::GetMousePos().x - itemPos.x;
        const double mousePercent = relativeX / size.x;

        *value = std::clamp(minValue + mousePercent * valueInterval, minValue, maxValue);
    }

    Rendering::DrawFatalSlider(
	    (*value - minValue) / valueInterval, toTextFn(*value).c_str(),
	    itemPos, size,
	    highlightedColor, backgroundColor, highlightedTextColor, textColor, textShadowColor,
	    ImGui::GetWindowDrawList(), textRenderer);

    return drag;
}

bool FatalWidgets::CheckBox(const char* label, bool* value, const ImVec2& size, ImU32 backgroundColor, ImU32 arrowColor) {
    bool ret;
    if ((ret = ImGui::InvisibleButton(label, size))) {
	    *value = !*value;
    }

    Rendering::DrawFatalCheckBox(*value, ImGui::GetItemRectMin(), size, backgroundColor, arrowColor);
    return ret;
}

bool FatalWidgets::ComboBox(const char* label, const char* popupLabel, const char* previewText, const ImVec2& size, void(*onPreBeginPopup)(), ImU32 backgroundColor, ImU32 textColor, ImU32 arrowColor, TextRenderer* textRenderer) {
    bool actived = ImGui::InvisibleButton(label, size);
    if (actived && popupLabel) {
        ImGui::OpenPopup(popupLabel);
    }
    Rendering::DrawFatalComboBox(previewText, ImGui::GetItemRectMin(), size, backgroundColor, textColor, arrowColor, ImGui::GetWindowDrawList(), textRenderer);

    if (popupLabel) {
        if (ImGui::IsPopupOpen(popupLabel)) {
            if (onPreBeginPopup) onPreBeginPopup();
            return ImGui::BeginPopup(popupLabel);
        }
        return false;
    }
    return actived;
}

bool FatalWidgets::RadioButtonB(const char* label, const char* text, int* selected, int index, const ImVec2& size, ImU32 textColor, ImU32 highlightTextColor, ImU32 indicatorColor, TextRenderer* textRenderer) {
    bool ret = false;
    ImVec2 renderTextSize = textRenderer->CalcTextSize(text);
    ImVec2 itemSize = size;
    if (itemSize.x == 0) {
        itemSize.x = renderTextSize.x + 8;
    }
    if (itemSize.y == 0) {
        itemSize.y = renderTextSize.y + 10;
    }

    if ((ret = ImGui::InvisibleButton(label, itemSize))) {
        *selected = index;
    }

    Rendering::DrawRadioButtonB(
	    text,
	    *selected == index,
	    ImGui::GetItemRectMin(), itemSize,
	    textColor, highlightTextColor, indicatorColor, ImGui::GetWindowDrawList(), textRenderer);

    return ret;
}
bool FatalWidgets::RadioButtonL(const char* label, const char* text, int* selected, int index, const ImVec2& size, ImU32 textColor, ImU32 highlightTextColor, ImU32 indicatorColor, TextRenderer* textRenderer) {
    bool ret = false;
    ImVec2 renderTextSize = textRenderer->CalcTextSize(text);
    ImVec2 itemSize = size;
    if (itemSize.x == 0) {
        itemSize.x = renderTextSize.x + 10;
    }
    if (itemSize.y == 0) {
        itemSize.y = renderTextSize.y + 4;
    }

    if ((ret = ImGui::InvisibleButton(label, itemSize))) {
        *selected = index;
    }

    Rendering::DrawRadioButtonL(
	    text,
	    *selected == index,
	    ImGui::GetItemRectMin(), itemSize,
	    textColor, highlightTextColor, indicatorColor, ImGui::GetWindowDrawList(), textRenderer);

    return ret;
}

bool FatalWidgets::IconButton(const char* label, const char* icon, const char* text, bool activated, ImFont* iconFont, const ImVec2& size, ImU32 iconColor, ImU32 textColor, ImU32 iconHighlightColor, ImU32 textHighlightColor, ImU32 backgroundHighlightColor, TextRenderer* textRenderer)
{
    ImVec2 itemSize;
    if (bool resizeX = size.x == 0, resizeY = size.y == 0; resizeX || resizeY)
    {
        const auto textSize = textRenderer->CalcTextSize(text);
        ImGui::PushFont(iconFont);
        const auto iconSize = textRenderer->CalcTextSize(icon);
        ImGui::PopFont();
        itemSize = ImVec2(
            resizeX ? iconSize.x + 4 + textSize.x + 12 : size.x,
            resizeY ? ImMax(iconSize.y, textSize.y) + 12 : size.y
        );
    }
    else itemSize = size;

    const bool ret = ImGui::InvisibleButton(label, itemSize);
    Rendering::DrawIconButton(
        icon, text, activated,
        ImGui::GetItemRectMin(), itemSize,
        iconFont, iconColor, textColor, 
        iconHighlightColor, textHighlightColor, backgroundHighlightColor, 
        ImGui::GetWindowDrawList(), textRenderer
    );
    return ret;
}

void FatalWidgets::BeginGroup(const char* childLabel, const ImVec2& size, TextRenderer* textRenderer) {
    ImGui::BeginGroup();
    const auto groupPos = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(groupPos.x, groupPos.y));
    const auto dummySize = ImVec2(0, textRenderer->GetFontHeight() / 2);
    ImGui::Dummy(dummySize);
    ImGui::SetCursorScreenPos(ImVec2(groupPos.x, groupPos.y + ImGui::GetItemRectSize().y));

    const auto windowDraws = ImGui::GetWindowDrawList();
    windowDraws->ChannelsSplit(3);
    windowDraws->ChannelsSetCurrent(1);

    ImGuiChildFlags flags = ImGuiChildFlags_AlwaysUseWindowPadding;
    if (size.x == 0) flags |= ImGuiChildFlags_AutoResizeX;
    if (size.y == 0) flags |= ImGuiChildFlags_AutoResizeY;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
    ImGui::BeginChild(childLabel, ImMax(size - dummySize, ImVec2{}), flags);
    ImGui::PopStyleVar();
}
void FatalWidgets::EndGroup(const char* title, ImU32 backgroundColor, ImU32 borderColor, ImU32 textColor, TextRenderer* textRenderer) {
    ImGui::EndChild();
    ImGui::EndGroup();

    const auto windowDraws = ImGui::GetWindowDrawList();
    const auto rectMin = ImGui::GetItemRectMin(), rectMax = ImGui::GetItemRectMax();
    const float fontHeight = textRenderer->GetFontHeight();

    windowDraws->ChannelsSetCurrent(0);
    windowDraws->AddRectFilled(ImVec2(rectMin.x, rectMin.y + fontHeight / 2), rectMax, backgroundColor, 4);

    windowDraws->ChannelsSetCurrent(2);
    Rendering::DrawGroupOverlay(title, rectMin, rectMax, backgroundColor, borderColor, backgroundColor - 0x00101010, textColor, windowDraws, textRenderer);
    ImGui::GetWindowDrawList()->ChannelsMerge();
}
