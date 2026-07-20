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
		struct FeatureGroup
		{
			const char* name;
			float height;
		};
		struct FeatureSubPage
		{
			const char* name;
			std::vector<FeatureGroup> groups;
		};
		struct FeaturePage
		{
			const char* name;
			std::vector<FeatureSubPage> subpages;
		};
		struct FeatureCategory
		{
			char8_t icon[4];
			const char* name;
			std::vector<FeaturePage> pages;
		};
		const std::vector<FeatureCategory> FeatureCategories = {
			FeatureCategory(
				u8"\ue66f", "RAGE",
				{
					FeaturePage(
						"Aimbot", 
						{
							FeatureSubPage(
								"General",
								{
									FeatureGroup("WEAPON", -1),
									FeatureGroup("EXTRA", -1),
									FeatureGroup("GENERAL", -1)
								}
							),
							FeatureSubPage(
								"Weapon1",
								{
									FeatureGroup("WEAPON", -1),
									FeatureGroup("EXTRA", -1)
								}
							),
							FeatureSubPage(
								"Weapon2",
								{
									FeatureGroup("WEAPON", -1)
								}
							)
						}
					),
					FeaturePage(
						"Anti-aim",
						{
							FeatureSubPage("General")
						}
					)
				}
			),
			FeatureCategory(
				u8"\ue6df", "LEGIT",
				{
					FeaturePage(
						"LegitBot",
						{
							FeatureSubPage("General")
						}
					)
				}
			),
			FeatureCategory(
				u8"\ue620", "VISUALS",
				{
					FeaturePage(
						"General",
						{
							FeatureSubPage(
								"Overlay",
								{
									FeatureGroup("ELEMENT A", 200),
									FeatureGroup("ELEMENT B", 300),
									FeatureGroup("ELEMENT C", 400)
								}
							),
							FeatureSubPage("World")
						}
					)
				}
			),
			FeatureCategory(
				u8"\ue665", "MISC",
				{
					FeaturePage(
						"General",
						{
							FeatureSubPage("General")
						}
					)
				}
			)
		};
	}

	namespace Menu
	{
		bool initialized = false;
		std::vector<Client::FeatureCategory> categories;

		int activeCateIndex = 0;
		int activePageIndex = 0;
		int activeSubPageIndex = 0;

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
				{
					::Menu::activeCateIndex = i;
					::Menu::activePageIndex = 0;
					::Menu::activeSubPageIndex = 0;
				}
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
		{
			const Client::FeatureCategory& selectedCate = ::Menu::categories[::Menu::activeCateIndex];
			if (selectedCate.pages.size() > 1)
			{
				ImGui::BeginGroup();
				for (int i = 0; i < selectedCate.pages.size(); ++i)
				{
					const auto& page = selectedCate.pages[i];
					if (FatalWidgets::RadioButtonB(page.name, page.name, &::Menu::activePageIndex, i))
						::Menu::activeSubPageIndex = 0;
					ImGui::SameLine(0, 4);
				}
				ImGui::EndGroup();
			}
			const Client::FeaturePage& selectedPage = selectedCate.pages[::Menu::activePageIndex];
			ImGui::Dummy(ImVec2(0, 6));
			ImGui::BeginChild("Page", ImGui::GetContentRegionAvail());
			{
				if (selectedPage.subpages.size() > 1)
				{
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 8));
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));
					ImGui::BeginChild("SubPages", ImVec2(110, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_AlwaysUseWindowPadding);
					for (int i = 0; i < selectedPage.subpages.size(); ++i)
					{
						const auto& subPage = selectedPage.subpages[i];
						FatalWidgets::RadioButtonL(subPage.name, subPage.name, &::Menu::activeSubPageIndex, i);
					}
					ImGui::EndChild();
					ImGui::PopStyleVar(2);
				}
				const Client::FeatureSubPage& selectedSubPage = selectedPage.subpages[::Menu::activeSubPageIndex];

				ImGui::SameLine(0, 1);

				ImGui::BeginChild("Groups", ImGui::GetContentRegionAvail());
				const auto avail = ImGui::GetContentRegionAvail();
				const float groupWidth = (avail.x - 28) / 3;
				for (const auto& group : selectedSubPage.groups)
				{
					FatalWidgets::BeginGroup(group.name, ImVec2(groupWidth, group.height < 0 ? avail.y : group.height));
					FatalWidgets::EndGroup(group.name);
					ImGui::SameLine(0, 14);
				}
				ImGui::EndChild();
			}
			ImGui::EndChild();
		}
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