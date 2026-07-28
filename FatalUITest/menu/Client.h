#pragma once

#include <vector>
#include "property/Properties.h"

namespace Client
{
	struct FeatureGroup
	{
		const char* name;
		float height;
		std::vector<Properties::PropDesc> features;
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

	// test props
	Properties::BoolProperty boolProp{ true };
	Properties::BoolProperty boolProp1{ false };
	Properties::NumberProp   hitchance{ 
		0, 1, 0.01, 0, 
		[](double val)->std::string
		{
			if (val == 0)
				return "Auto";
			double percent = val * 100;
			return std::vformat("{:.0f}%", std::make_format_args(percent));
		}
	};
	Properties::NumberProp	 mindamage{
		0, 120, 1, 0,
		[](double val) -> std::string
		{
			if (val == 0)
				return "Lethal";
			if (val > 100)
			{
				double ex = val - 100;
				return std::vformat("HP + {:.0f}", std::make_format_args(ex));
			}
			return std::vformat("{:.0f} HP", std::make_format_args(val));
		}
	};
	Properties::ModeProperty priority{ 0, { "Hit-chance", "Damage" } };

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
								FeatureGroup(
									"WEAPON", -1, 
									{ 
										Properties::PropDesc(
											"Test",
											&boolProp,
											{ Properties::PropDesc("Test", &boolProp1) }
										), 
										Properties::PropDesc("Hit-chance", &hitchance),
										Properties::PropDesc("Min-damage", &mindamage),
										Properties::PropDesc("Priority", &priority)
									}
								),
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
						FeatureSubPage(
							"General",
							{
								FeatureGroup("ELEMENT A", -1),
								FeatureGroup("ELEMENT B", -1),
								FeatureGroup("ELEMENT C", -1)
							}
						)
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
								FeatureGroup(
									"ELEMENT A", 0,
									{
										Properties::PropDesc(
											"Test",
											&boolProp,
											{ Properties::PropDesc("Test", &boolProp1) }
										), 
									}
								),
								FeatureGroup("ELEMENT B", 0),
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