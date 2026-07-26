#pragma once

#include <vector>
#include "property/Properties.h"

namespace Client
{
	struct Feature
	{
		const char* name;
		Properties::PropDesc prop;
	};
	struct FeatureGroup
	{
		const char* name;
		float height;
		std::vector<Feature> features;
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
	Properties::NumberProp   numProp{ 0, 100, 1, 50 };

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
										Feature(
											"Hit-chance", 
											Properties::PropDesc(&boolProp, { Properties::PropDesc(&boolProp1) })
										), 
										Feature("Pointscale", Properties::PropDesc(&numProp))
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