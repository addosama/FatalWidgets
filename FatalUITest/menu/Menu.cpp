#include "Menu.h"
#include "FatalWidgets.h"

#include <vector>

#include "imgui_internal.h"

extern ImFont* Medium12;
extern ImFont* ExtraBold20;
extern ImFont* IconFont;

namespace 
{
	namespace Client
	{
		struct Category
		{
			char8_t icon[4];
			const char* name;
		};
		const std::vector<Category> FeatureCategories = {
			Category(u8"\ue66f", "RAGE"),
			Category(u8"\ue6df", "LEGIT"),
			Category(u8"\ue620", "VISUALS"),
			Category(u8"\ue665", "MISC")
		};
	}

	namespace Menu
	{
		bool initialized = false;
		std::vector<Client::Category> categories;

		int activeCateIndex = 0;

		void initialize()
		{
			categories = Client::FeatureCategories;
			initialized = true;
		}
	}
}

void Fatal::Menu::draw()
{
	if (!::Menu::initialized)
		::Menu::initialize();
	ImGui::SetNextWindowSize(ImVec2(848, 588));

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());

	ImGui::Begin(
		"FatalMenu", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize
	);
	const auto draws = ImGui::GetWindowDrawList();
	const auto winRectMin = ImGui::GetWindowPos();
	const auto winRectSize = ImGui::GetWindowSize();
	const auto winRectMax = winRectMin + winRectSize;

	// draw background
	draws->AddRectFilled(winRectMin, winRectMax, 0xFF151515, 6);

	// draw titlebar & shadow
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 0));
		ImGui::BeginChild("TitleBar", ImVec2(winRectSize.x, 50), ImGuiChildFlags_AlwaysUseWindowPadding);
		{
			const auto fr = FatalWidgets::Rendering::GetDefaultTextRenderer();
			const auto titleDraws = ImGui::GetWindowDrawList();
			// background
			titleDraws->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), 0xFF1A1A1A, 6, ImDrawFlags_RoundCornersTop);

			// title
			ImGui::PushFont(ExtraBold20);
			auto text = "FUTALITY";
			FatalWidgets::Rendering::DrawFatalTitle(
				text,
				ImGui::GetCursorScreenPos() + ImVec2(0, (ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeight()) / 2),
				fmodf(ImGui::GetTime(), 1.0f), 1,
				0xFFFFFFFF, 0xFF0000FF, 0xFFFF0000
			);
			ImGui::SameLine(0, fr->CalcTextSize(text).x + 20);
			ImGui::PopFont();

			// category buttons
			for (int i = 0; i < ::Menu::categories.size(); ++i)
			{
				if (i > 0) ImGui::SameLine(0, 2);
				const auto& cate = ::Menu::categories.at(i);
				ImGui::SetCursorPosY(11);
				if (FatalWidgets::IconButton(cate.name, reinterpret_cast<const char*>(cate.icon), cate.name, ::Menu::activeCateIndex == i, IconFont, ImVec2(0, 28)))
					::Menu::activeCateIndex = i;
			}

			// user data
			const char* username = "FatalWidgets";
			const char* expireText = "by:";
			const char* expireDays = "addo6544.me";

			const auto usernameSize = fr->CalcTextSize(username) + ImVec2(0, ImGui::GetFontBaked()->Descent / 2);
			const auto expireTextSize = fr->CalcTextSize(expireText);
			const auto expireDaysSize = fr->CalcTextSize(expireDays);
			const auto spaceWidth = ImGui::CalcTextSize(" ").x;
			const auto lineWidthDiff = usernameSize.x - (expireTextSize.x + spaceWidth + expireDaysSize.x);

			ImGui::SetCursorPos(ImVec2(
				winRectSize.x - 20 - (30 + ImMax(usernameSize.x, expireTextSize.x + spaceWidth + expireDaysSize.x)),
				10
			));
			ImGui::BeginGroup();
			{
				// user info
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
				ImGui::BeginGroup();
				{
					ImGui::Dummy(ImVec2(0, (30 - (usernameSize.y + expireTextSize.y)) / 2));
					// username line
					{
						if (lineWidthDiff < 0)
						{
							ImGui::Dummy(ImVec2(-lineWidthDiff, 0));
							ImGui::SameLine(0, 0);
						}
						ImGui::Dummy(usernameSize);
						fr->DrawText(titleDraws, username, ImGui::GetItemRectMin(), 0xFFFAFAFA);
					}
					// expire date line
					{
						if (lineWidthDiff > 0)
						{
							ImGui::Dummy(ImVec2(lineWidthDiff, 0));
							ImGui::SameLine(0, 0);
						}
						ImGui::Dummy(expireTextSize + ImVec2(spaceWidth + expireDaysSize.x, 0));
						fr->DrawText(titleDraws, expireText, ImGui::GetItemRectMin(), 0xFF9E9E9E);
						fr->DrawText(titleDraws, expireDays, ImGui::GetItemRectMin() + ImVec2(expireTextSize.x + spaceWidth, 0), 0xFF6666FF);
					}
				}
				ImGui::EndGroup();
				ImGui::PopStyleVar();
				ImGui::SameLine(0, 4);
				// user avatar
				{
					ImGui::Dummy(ImVec2(30, 30));
					titleDraws->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0xFF666666, 15);
				}
			}
			ImGui::EndGroup();

		}
		ImGui::EndChild();
		ImGui::PopStyleVar();

		// split line
		ImGui::Dummy(ImVec2(winRectSize.x, 1));
		draws->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0xFF262626);

		// shadow
		draws->AddRectFilledMultiColor(
			ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + ImVec2(winRectSize.x, 10),
			0x44000000, 0x44000000, 0, 0
		);
	}

	// draw content
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 14));
		ImGui::BeginChild("Content", ImVec2(winRectSize.x, 508), ImGuiChildFlags_AlwaysUseWindowPadding);

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	// draw bottom bar & shadow
	{
		// split line
		ImGui::Dummy(ImVec2(winRectSize.x, 1));
		draws->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0xFF262626);
		// shadow
		draws->AddRectFilledMultiColor(
			ImGui::GetItemRectMin() + ImVec2(winRectSize.x, -10), ImGui::GetItemRectMin(),
			0, 0, 0x44000000, 0x44000000
		);

		// bar
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 0));
		ImGui::BeginChild("BottomBar", ImVec2(winRectSize.x, 28));

		const auto bottomDraws = ImGui::GetWindowDrawList();
		bottomDraws->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), 0xFF1A1A1A, 6, ImDrawFlags_RoundCornersBottom);

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	ImGui::End();
	ImGui::PopStyleVar(2);
}