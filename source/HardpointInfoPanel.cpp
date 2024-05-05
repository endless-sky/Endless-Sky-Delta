/* HardpointInfoPanel.cpp
Copyright (c) 2024 by Zitchas

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HardpointInfoPanel.h"

#include "text/alignment.hpp"
#include "CategoryList.h"
#include "CategoryTypes.h"
#include "Command.h"
#include "Dialog.h"
#include "text/DisplayText.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "HardpointInfoPanel.h"
#include "Information.h"
#include "Interface.h"
#include "LineShader.h"
#include "LogbookPanel.h"
#include "Messages.h"
#include "MissionPanel.h"
#include "OutlineShader.h"
#include "PlayerInfo.h"
#include "PlayerInfoPanel.h"
#include "Rectangle.h"
#include "Ship.h"
#include "ShipInfoPanel.h"
#include "Sprite.h"
#include "SpriteShader.h"
#include "text/Table.h"
#include "text/truncate.hpp"
#include "UI.h"

#include <algorithm>

using namespace std;

namespace {
	constexpr double WIDTH = 250.;
	constexpr int COLUMN_WIDTH = static_cast<int>(WIDTH) - 20;
}

HardpointInfoPanel::HardpointInfoPanel(PlayerInfo& player)
	: HardpointInfoPanel(player, InfoPanelState(player))
{
}

HardpointInfoPanel::HardpointInfoPanel(PlayerInfo& player, InfoPanelState state)
	: player(player), panelState(std::move(state))
{
	shipIt = this->panelState.Ships().begin();
	SetInterruptible(false);

	// If a valid ship index was given, show that ship.
	if(static_cast<unsigned>(panelState.SelectedIndex()) < player.Ships().size())
		shipIt += panelState.SelectedIndex();
	else if(player.Flagship())
	{
		// Find the player's flagship. It may not be first in the list, if the
		// first item in the list cannot be a flagship.
		while (shipIt != this->panelState.Ships().end() && shipIt->get() != player.Flagship())
			++shipIt;
	}

	UpdateInfo();
}



void HardpointInfoPanel::Step()
{
	DoHelp("hardpoint info");
}



void HardpointInfoPanel::Draw()
{
	// Dim everything behind this panel.
	DrawBackdrop();

	// Fill in the information for how this interface should be drawn.
	Information interfaceInfo;
	interfaceInfo.SetCondition("hardpoint tab");
	if(panelState.CanEdit() && shipIt != panelState.Ships().end()
		&& (shipIt->get() != player.Flagship() || (*shipIt)->IsParked()))
	{
		if(!(*shipIt)->IsDisabled())
			interfaceInfo.SetCondition("can park");
		interfaceInfo.SetCondition((*shipIt)->IsParked() ? "show unpark" : "show park");
		interfaceInfo.SetCondition("show disown");
	}
	else if(!panelState.CanEdit())
	{
		interfaceInfo.SetCondition("show dump");
		if(CanDump())
			interfaceInfo.SetCondition("enable dump");
	}
	if(player.Ships().size() > 1)
		interfaceInfo.SetCondition("five buttons");
	else
		interfaceInfo.SetCondition("three buttons");
	if(player.HasLogs())
		interfaceInfo.SetCondition("enable logbook");

	// Draw the interface.
	const Interface * infoPanelUi = GameData::Interfaces().Get("info panel");
	infoPanelUi->Draw(interfaceInfo, this);
	int infoPanelLine = 0;

	// Draw all the different information sections.
	ClearZones();
	if(shipIt == panelState.Ships().end())
		return;
	// Rectangle cargoBounds = infoPanelUi->GetBox("cargo");
	// DrawShipStats(infoPanelUi->GetBox("stats")); // This line currently displays all the stats calls L355
	DrawShipName(infoPanelUi->GetBox("stats"), infoPanelLine);
	DrawShipModelStats(infoPanelUi->GetBox("stats"), infoPanelLine);
	// DrawShipCosts(infoPanelUi->GetBox("stats"), infoPanelLine);
	infoPanelLine++;
	// DrawShipHealthStats(infoPanelUi->GetBox("stats"), infoPanelLine); // This should pull up shield and hull
	DrawShipCarryingCapacities(infoPanelUi->GetBox("stats"), infoPanelLine); // mass, cargo, bunks, fuel
	infoPanelLine++;
	DrawShipManeuverStats(infoPanelUi->GetBox("stats"), infoPanelLine); // max speed, thrust, reverse thrust, turn, lateral
	infoPanelLine++;
	DrawShipOutfitStat(infoPanelUi->GetBox("stats"), infoPanelLine);
	DrawShipCapacities(infoPanelUi->GetBox("stats"), infoPanelLine);
	DrawShipPropulsionCapacities(infoPanelUi->GetBox("stats"), infoPanelLine);
	DrawShipHardpointStats(infoPanelUi->GetBox("stats"), infoPanelLine);
	DrawShipBayStats(infoPanelUi->GetBox("stats"), infoPanelLine);
	infoPanelLine++;
	DrawShipEnergyHeatStats(infoPanelUi->GetBox("Stats"), infoPanelLine);
	// DrawOutfits(infoPanelUi->GetBox("outfits"), cargoBounds);
	DrawWeapons(infoPanelUi->GetBox("weapons"));
	// DrawCargo(cargoBounds);

	// If the player hovers their mouse over a ship attribute, show its tooltip.
	info.DrawTooltips();
}



bool HardpointInfoPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command& command, bool /* isNewPress */)
{
	bool control = (mod & (KMOD_CTRL | KMOD_GUI));
	bool shift = (mod & KMOD_SHIFT);
	if(key == 'd' || key == SDLK_ESCAPE || (key == 'w' && control))
		GetUI()->Pop(this);
	else if(command.Has(Command::HELP))
		DoHelp("hardpoint info", true);
	else if(!player.Ships().empty() && ((key == 'p' && !shift) || key == SDLK_LEFT || key == SDLK_UP))
	{
		if(shipIt == panelState.Ships().begin())
			shipIt = panelState.Ships().end();
		--shipIt;
		UpdateInfo();
	}
	else if(!panelState.Ships().empty() && (key == 'n' || key == SDLK_RIGHT || key == SDLK_DOWN))
	{
		++shipIt;
		if(shipIt == panelState.Ships().end())
			shipIt = panelState.Ships().begin();
		UpdateInfo();
	}
	else if(key == 'i' || command.Has(Command::INFO) || (control && key == SDLK_TAB))
	{
		GetUI()->Pop(this);
		GetUI()->Push(new PlayerInfoPanel(player, std::move(panelState)));
	}
	else if(key == 's')
	{
		if(!player.Ships().empty())
		{
			GetUI()->Pop(this);
			GetUI()->Push(new ShipInfoPanel(player, std::move(panelState)));
		}
	}
	else if(key == 'R' || (key == 'r' && shift))
		GetUI()->Push(new Dialog(this, &HardpointInfoPanel::Rename, "Change this ship's name?", (*shipIt)->Name()));
	else if(panelState.CanEdit() && (key == 'P' || (key == 'p' && shift) || key == 'k'))
	{
		if(shipIt->get() != player.Flagship() || (*shipIt)->IsParked())
			player.ParkShip(shipIt->get(), !(*shipIt)->IsParked());
	}
	else if(panelState.CanEdit() && key == 'D')
	{
		if(shipIt->get() != player.Flagship())
		{
			map<const Outfit*, int> uniqueOutfits;
			auto AddToUniques = [&uniqueOutfits](const std::map<const Outfit*, int>& outfits)
				{
					for(const auto & it : outfits)
						if(it.first->Attributes().Get("unique"))
							uniqueOutfits[it.first] += it.second;
				};
			AddToUniques(shipIt->get()->Outfits());
			AddToUniques(shipIt->get()->Cargo().Outfits());

			string message = "Are you sure you want to disown \""
				+ shipIt->get()->Name()
				+ "\"? Disowning a ship rather than selling it means you will not get any money for it.";
			if(!uniqueOutfits.empty())
			{
				const int uniquesSize = uniqueOutfits.size();
				const int detailedOutfitSize = (uniquesSize > 20 ? 19 : uniquesSize);
				message += "\nThe following unique items carried by the ship will be lost:";
				auto it = uniqueOutfits.begin();
				for(int i = 0; i < detailedOutfitSize; ++i)
				{
					message += "\n" + to_string(it->second) + " "
						+ (it->second == 1 ? it->first->DisplayName() : it->first->PluralName());
					++it;
				}
				if(it != uniqueOutfits.end())
				{
					int otherUniquesCount = 0;
					for(; it != uniqueOutfits.end(); ++it)
						otherUniquesCount += it->second;
					message += "\nand " + to_string(otherUniquesCount) + " other unique outfits";
				}
			}

			GetUI()->Push(new Dialog(this, &HardpointInfoPanel::Disown, message));
		}
	}
	else if(key == 'c' && CanDump())
	{
		int commodities = (*shipIt)->Cargo().CommoditiesSize();
		int amount = (*shipIt)->Cargo().Get(selectedCommodity);
		int plunderAmount = (*shipIt)->Cargo().Get(selectedPlunder);
		if(amount)
		{
			GetUI()->Push(new Dialog(this, &HardpointInfoPanel::DumpCommodities,
				"How many tons of " + Format::LowerCase(selectedCommodity)
				+ " do you want to jettison?", amount));
		}
		else if(plunderAmount > 0 && selectedPlunder->Get("installable") < 0.)
		{
			GetUI()->Push(new Dialog(this, &HardpointInfoPanel::DumpPlunder,
				"How many tons of " + Format::LowerCase(selectedPlunder->DisplayName())
				+ " do you want to jettison?", plunderAmount));
		}
		else if(plunderAmount == 1)
		{
			GetUI()->Push(new Dialog(this, &HardpointInfoPanel::Dump,
				"Are you sure you want to jettison a " + selectedPlunder->DisplayName() + "?"));
		}
		else if(plunderAmount > 1)
		{
			GetUI()->Push(new Dialog(this, &HardpointInfoPanel::DumpPlunder,
				"How many " + selectedPlunder->PluralName() + " do you want to jettison?",
				plunderAmount));
		}
		else if(commodities)
		{
			GetUI()->Push(new Dialog(this, &HardpointInfoPanel::Dump,
				"Are you sure you want to jettison all of this ship's regular cargo?"));
		}
		else
		{
			GetUI()->Push(new Dialog(this, &HardpointInfoPanel::Dump,
				"Are you sure you want to jettison all of this ship's cargo?"));
		}
	}
	else if(command.Has(Command::MAP) || key == 'm')
		GetUI()->Push(new MissionPanel(player));
	else if(key == 'l' && player.HasLogs())
		GetUI()->Push(new LogbookPanel(player));
	else
		return false;

	return true;
}



bool HardpointInfoPanel::Click(int x, int y, int /* clicks */)
{
	if(shipIt == panelState.Ships().end())
		return true;

	draggingIndex = -1;
	if(panelState.CanEdit() && hoverIndex >= 0 && (**shipIt).GetSystem() == player.GetSystem() && !(**shipIt).IsDisabled())
		draggingIndex = hoverIndex;

	selectedCommodity.clear();
	selectedPlunder = nullptr;
	Point point(x, y);
	for(const auto & zone : commodityZones)
		if(zone.Contains(point))
			selectedCommodity = zone.Value();
	for(const auto & zone : plunderZones)
		if(zone.Contains(point))
			selectedPlunder = zone.Value();

	return true;
}



bool HardpointInfoPanel::Hover(int x, int y)
{
	Point point(x, y);
	info.Hover(point);
	return Hover(point);
}



bool HardpointInfoPanel::Drag(double dx, double dy)
{
	return Hover(hoverPoint + Point(dx, dy));
}



bool HardpointInfoPanel::Release(int /* x */, int /* y */)
{
	if(draggingIndex >= 0 && hoverIndex >= 0 && hoverIndex != draggingIndex)
		(**shipIt).GetArmament().Swap(hoverIndex, draggingIndex);

	draggingIndex = -1;
	return true;
}



void HardpointInfoPanel::UpdateInfo()
{
	draggingIndex = -1;
	hoverIndex = -1;
	ClearZones();
	if(shipIt == panelState.Ships().end())
		return;

	const Ship & ship = **shipIt;
	info.Update(ship, player);
	if(player.Flagship() && ship.GetSystem() == player.GetSystem() && &ship != player.Flagship())
		player.Flagship()->SetTargetShip(*shipIt);

	outfits.clear();
	for(const auto & it : ship.Outfits())
		outfits[it.first->Category()].push_back(it.first);

	panelState.SelectOnly(shipIt - panelState.Ships().begin());
}



void HardpointInfoPanel::ClearZones()
{
	zones.clear();
	commodityZones.clear();
	plunderZones.clear();
}



void HardpointInfoPanel::DrawShipStats(const Rectangle &bounds)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < WIDTH)
		return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship & ship = **shipIt;
	const Outfit & attributes = ship.Attributes();

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	table.AddColumn(0, { COLUMN_WIDTH, Alignment::LEFT });
	table.AddColumn(COLUMN_WIDTH, { COLUMN_WIDTH, Alignment::RIGHT, Truncate::MIDDLE });
	table.SetUnderline(0, COLUMN_WIDTH);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));

	table.DrawTruncatedPair("ship:", dim, ship.Name(), bright, Truncate::MIDDLE, true);

	double shieldRegen = (attributes.Get("shield generation")
		+ attributes.Get("delayed shield generation"))
		* (1. + attributes.Get("shield generation multiplier"));
	bool hasShieldRegen = shieldRegen > 0.;

	if(hasShieldRegen)
	{
		table.DrawTruncatedPair("shields (charge):", dim, Format::Number(ship.MaxShields())
			+ " (" + Format::Number(60. * shieldRegen) + "/s)", bright, Truncate::MIDDLE, true);
	}
	else
	{
		table.DrawTruncatedPair("shields", dim, Format::Number(ship.MaxShields()), bright, Truncate::MIDDLE, true);
	}
}



void HardpointInfoPanel::DrawShipName(const Rectangle &bounds, int & infoPanelLine)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < WIDTH)
		return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship & ship = **shipIt;

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	table.AddColumn(0, { COLUMN_WIDTH, Alignment::LEFT });
	table.AddColumn(COLUMN_WIDTH, { COLUMN_WIDTH, Alignment::RIGHT, Truncate::MIDDLE });
	table.SetUnderline(0, COLUMN_WIDTH);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));


	// This allows the section to stack nicely with other info panel sections,
	// But will also allow it to be called on its own in a new box if desire.
	for(int i = 0; i < infoPanelLine; i++)
	{
		table.DrawTruncatedPair(" ", dim, " ", bright, Truncate::MIDDLE, true);
	}

	table.DrawTruncatedPair("ship:", dim, ship.Name(), bright, Truncate::MIDDLE, true);
	infoPanelLine++;
}



void HardpointInfoPanel::DrawShipModelStats(const Rectangle &bounds, int & infoPanelLine)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < WIDTH)
		return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship & ship = **shipIt;

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	table.AddColumn(0, { COLUMN_WIDTH, Alignment::LEFT });
	table.AddColumn(COLUMN_WIDTH, { COLUMN_WIDTH, Alignment::RIGHT, Truncate::MIDDLE });
	table.SetUnderline(0, COLUMN_WIDTH);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));


	// This allows the section to stack nicely with other info panel sections,
	// But will also allow it to be called on its own in a new box if desire.
	for(int i = 0; i < infoPanelLine; i++)
	{
		table.DrawTruncatedPair(" ", dim, " ", bright, Truncate::MIDDLE, true);
	}

	table.DrawTruncatedPair("model:", dim, ship.DisplayModelName(), bright, Truncate::MIDDLE, true);
	infoPanelLine++;
}



void HardpointInfoPanel::DrawShipCosts(const Rectangle &bounds, int & infoPanelLine)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < WIDTH)
		return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship & ship = **shipIt;

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	table.AddColumn(0, { COLUMN_WIDTH, Alignment::LEFT });
	table.AddColumn(COLUMN_WIDTH, { COLUMN_WIDTH, Alignment::RIGHT, Truncate::MIDDLE });
	table.SetUnderline(0, COLUMN_WIDTH);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));


	// This allows the section to stack nicely with other info panel sections,
	// But will also allow it to be called on its own in a new box if desire.
	for(int i = 0; i < infoPanelLine; i++)
	{
		table.DrawTruncatedPair(" ", dim, " ", bright, Truncate::MIDDLE, true);
	}

	// This just displays the ship's cost.
	// Another function should give the hull + outfits breakdown.
	table.DrawTruncatedPair("cost:", dim, Format::Credits(ship.Cost()), bright, Truncate::MIDDLE, true);

	infoPanelLine++;
}



void HardpointInfoPanel::DrawShipHealthStats(const Rectangle &bounds, int & infoPanelLine)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < WIDTH)
		return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship & ship = **shipIt;
	const Outfit & attributes = ship.Attributes();

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	table.AddColumn(0, { COLUMN_WIDTH, Alignment::LEFT });
	table.AddColumn(COLUMN_WIDTH, { COLUMN_WIDTH, Alignment::RIGHT, Truncate::MIDDLE });
	table.SetUnderline(0, COLUMN_WIDTH);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));

	double shieldRegen = (attributes.Get("shield generation")
		+ attributes.Get("delayed shield generation"))
		* (1. + attributes.Get("shield generation multiplier"));
	bool hasShieldRegen = shieldRegen > 0.;
	double hullRegen = (attributes.Get("hull repair rate"))
		* (1. + attributes.Get("hull repair multiplier"));
	bool hasHullRegen = hullRegen > 0.;


	// This allows the section to stack nicely with other info panel sections,
	// But will also allow it to be called on its own in a new box if desire.
	for(int i = 0; i < infoPanelLine; i++)
	{
		table.DrawTruncatedPair(" ", dim, " ", bright, Truncate::MIDDLE, true);
	}

	if(hasShieldRegen)
	{
		table.DrawTruncatedPair("shields (charge):", dim, Format::Number(ship.MaxShields())
			+ " (" + Format::Number(60. * shieldRegen) + "/s)", bright, Truncate::MIDDLE, true);
	}
	else
	{
		table.DrawTruncatedPair("shields", dim, Format::Number(ship.MaxShields()), bright, Truncate::MIDDLE, true);
	}
	if(hasHullRegen)
	{
		table.DrawTruncatedPair("hull (repair):", dim, Format::Number(ship.MaxHull())
			+ " (" + Format::Number(60. * hullRegen) + "/s)", bright, Truncate::MIDDLE, true);
	}
	else
	{
		table.DrawTruncatedPair("hull", dim, Format::Number(ship.MaxHull()), bright, Truncate::MIDDLE, true);
	}
	infoPanelLine = infoPanelLine + 2;
}



void HardpointInfoPanel::DrawShipCarryingCapacities(const Rectangle &bounds, int & infoPanelLine)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < WIDTH)
		return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship & ship = **shipIt;
	const Outfit & attributes = ship.Attributes();

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	table.AddColumn(0, { COLUMN_WIDTH, Alignment::LEFT });
	table.AddColumn(COLUMN_WIDTH, { COLUMN_WIDTH, Alignment::RIGHT, Truncate::MIDDLE });
	table.SetUnderline(0, COLUMN_WIDTH);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));


	// This allows the section to stack nicely with other info panel sections,
	// But will also allow it to be called on its own in a new box if desire.
	for(int i = 0; i < infoPanelLine; i++)
	{
		table.DrawTruncatedPair(" ", dim, " ", bright, Truncate::MIDDLE, true);
	}

	table.DrawTruncatedPair("current mass:", dim, Format::Number(ship.Mass()) + " tons", bright, Truncate::MIDDLE, true);
	table.DrawTruncatedPair("cargo space:", dim, Format::Number(ship.Cargo().Used())
		+ " / " + Format::Number(attributes.Get("cargo space")) + " tons", bright, Truncate::MIDDLE, true);
	table.DrawTruncatedPair("required crew / bunks", dim, Format::Number(ship.Crew())
		+ " / " + Format::Number(attributes.Get("bunks")), bright, Truncate::MIDDLE, true);
	table.DrawTruncatedPair("fuel / fuel capacity:", dim, Format::Number(ship.Fuel() * attributes.Get("fuel capacity"))
		+ " / " + Format::Number(attributes.Get("fuel capacity")), bright, Truncate::MIDDLE, true);

	infoPanelLine = infoPanelLine + 4;
}



void HardpointInfoPanel::DrawShipManeuverStats(const Rectangle &bounds, int & infoPanelLine)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < WIDTH)
		return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship & ship = **shipIt;
	const Outfit & attributes = ship.Attributes();

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	table.AddColumn(0, { COLUMN_WIDTH, Alignment::LEFT });
	table.AddColumn(COLUMN_WIDTH, { COLUMN_WIDTH, Alignment::RIGHT, Truncate::MIDDLE });
	table.SetUnderline(0, COLUMN_WIDTH);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));

	// This allows the section to stack nicely with other info panel sections,
	// But will also allow it to be called on its own in a new box if desire.
	for(int i = 0; i < infoPanelLine; i++)
	{
		table.DrawTruncatedPair(" ", dim, " ", bright, Truncate::MIDDLE, true);
	}

	// Get the full mass of the ship
	double emptyMass = attributes.Mass();
	// double currentMass = ship.Mass();
	double fullMass = emptyMass + attributes.Get("cargo space");

	// Movement stats are influenced by inertia reduction.
	double reduction = 1. + attributes.Get("inertia reduction");
	emptyMass /= reduction;
	// currentMass /= reduction;
	fullMass /= reduction;

	double baseAccel = 3600. * attributes.Get("thrust") * (1. + attributes.Get("acceleration multiplier"));
	double afterburnerAccel = 3600. * attributes.Get("afterburner thrust") * (1. +
		attributes.Get("acceleration multiplier"));
	double reverseAccel = 3600. * attributes.Get("reverse thrust") * (1. + attributes.Get("acceleration multiplier"));
	double lateralAccel = 3600. * attributes.Get("lateral thrust") * (1. + attributes.Get("acceleration multiplier"));

	double baseTurn = (60. * attributes.Get("turn") * (1. + attributes.Get("turn multiplier"))) / emptyMass;
	double minTurn = (60. * attributes.Get("turn") * (1. + attributes.Get("turn multiplier"))) / fullMass;

	table.DrawTruncatedPair("max speed (w/AB):", dim, Format::Number(60. * attributes.Get("thrust") / ship.Drag()) + " (" +
		Format::Number(60. * attributes.Get("afterburner thrust") / ship.Drag()) + ")", bright, Truncate::MIDDLE, true);
	table.DrawTruncatedPair("thrust (full - no cargo):", dim, " ", bright, Truncate::MIDDLE, true);
	table.DrawTruncatedPair("   forward:", dim, Format::Number(baseAccel / fullMass) +
		" - " + Format::Number(baseAccel / emptyMass), bright, Truncate::MIDDLE, true);
	table.DrawTruncatedPair("   Afterburner:", dim, Format::Number(afterburnerAccel / fullMass) +
		" - " + Format::Number(afterburnerAccel / emptyMass), bright, Truncate::MIDDLE, true);
	table.DrawTruncatedPair("   reverse:", dim, Format::Number(reverseAccel / fullMass) +
		" - " + Format::Number(reverseAccel / emptyMass), bright, Truncate::MIDDLE, true);
	table.DrawTruncatedPair("   lateral:", dim, Format::Number(lateralAccel / fullMass) +
		" - " + Format::Number(lateralAccel / emptyMass), bright, Truncate::MIDDLE, true); // currently doesn't work
	table.DrawTruncatedPair("turning:", dim, Format::Number(minTurn) + " - " + Format::Number(baseTurn), bright,
		Truncate::MIDDLE, true);

	infoPanelLine = infoPanelLine + 7;
}



void HardpointInfoPanel::DrawShipOutfitStat(const Rectangle &bounds, int & infoPanelLine)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < WIDTH)
		return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship & ship = **shipIt;
	const Outfit & attributes = ship.Attributes();

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	table.AddColumn(0, { COLUMN_WIDTH, Alignment::LEFT });
	table.AddColumn(COLUMN_WIDTH, { COLUMN_WIDTH, Alignment::RIGHT, Truncate::MIDDLE });
	table.SetUnderline(0, COLUMN_WIDTH);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));

	// This allows the section to stack nicely with other info panel sections,
	// But will also allow it to be called on its own in a new box if desire.
	for(int i = 0; i < infoPanelLine; i++)
	{
		table.DrawTruncatedPair(" ", dim, " ", bright, Truncate::MIDDLE, true);
	}

	// Find out how much outfit space the chassis has.
	map<string, double> chassis;
	static const vector<string> NAMES = {
		"outfit space free:", "outfit space",
	};

	for(unsigned i = 1; i < NAMES.size(); i += 2)
		chassis[NAMES[i]] = attributes.Get(NAMES[i]);
	for(const auto & it : ship.Outfits())
		for(auto & cit : chassis)
			cit.second -= min(0., it.second * it.first->Get(cit.first));

	for(unsigned i = 0; i < NAMES.size(); i += 2)
	{
		table.DrawTruncatedPair((NAMES[i]), dim, Format::Number(attributes.Get(NAMES[i + 1]))
			+ " / " + Format::Number(chassis[NAMES[i + 1]]), bright, Truncate::MIDDLE, true);
		infoPanelLine++;
	}
}



void HardpointInfoPanel::DrawShipCapacities(const Rectangle &bounds, int & infoPanelLine)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < WIDTH)
		return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship & ship = **shipIt;
	const Outfit & attributes = ship.Attributes();

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	table.AddColumn(0, { COLUMN_WIDTH, Alignment::LEFT });
	table.AddColumn(COLUMN_WIDTH, { COLUMN_WIDTH, Alignment::RIGHT, Truncate::MIDDLE });
	table.SetUnderline(0, COLUMN_WIDTH);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));

	// This allows the section to stack nicely with other info panel sections,
	// But will also allow it to be called on its own in a new box if desire.
	for(int i = 0; i < infoPanelLine; i++)
	{
		table.DrawTruncatedPair(" ", dim, " ", bright, Truncate::MIDDLE, true);
	}

	// Find out how much outfit, engine, and weapon space the chassis has.
	map<string, double> chassis;
	static const vector<string> NAMES = {
		"    weapon capacity:", "weapon capacity",
		"    engine capacity:", "engine capacity",
	};

	for(unsigned i = 1; i < NAMES.size(); i += 2)
		chassis[NAMES[i]] = attributes.Get(NAMES[i]);
	for(const auto & it : ship.Outfits())
		for(auto & cit : chassis)
			cit.second -= min(0., it.second * it.first->Get(cit.first));

	for(unsigned i = 0; i < NAMES.size(); i += 2)
	{
		table.DrawTruncatedPair((NAMES[i]), dim, Format::Number(attributes.Get(NAMES[i + 1]))
			+ " / " + Format::Number(chassis[NAMES[i + 1]]), bright, Truncate::MIDDLE, true);
		infoPanelLine++;
	}
}



void HardpointInfoPanel::DrawShipPropulsionCapacities(const Rectangle &bounds, int & infoPanelLine)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < WIDTH)
		return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship & ship = **shipIt;
	const Outfit & attributes = ship.Attributes();

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	table.AddColumn(0, { COLUMN_WIDTH, Alignment::LEFT });
	table.AddColumn(COLUMN_WIDTH, { COLUMN_WIDTH, Alignment::RIGHT, Truncate::MIDDLE });
	table.SetUnderline(0, COLUMN_WIDTH);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));

	// This allows the section to stack nicely with other info panel sections,
	// But will also allow it to be called on its own in a new box if desire.
	for(int i = 0; i < infoPanelLine; i++)
	{
		table.DrawTruncatedPair(" ", dim, " ", bright, Truncate::MIDDLE, true);
	}

	// Find out how much outfit, engine, and weapon space the chassis has.
	map<string, double> chassis;
	static const vector<string> NAMES = {
		"engine mod space free:", "engine mod space",
		"reverse thruster slots free:", "reverse thruster slot",
		"steering slots free:", "steering slot",
		"thruster slots free:", "thruster slot",
	};

	for(unsigned i = 1; i < NAMES.size(); i += 2)
		chassis[NAMES[i]] = attributes.Get(NAMES[i]);
	for(const auto & it : ship.Outfits())
		for(auto & cit : chassis)
			cit.second -= min(0., it.second * it.first->Get(cit.first));

	for(unsigned i = 0; i < NAMES.size(); i += 2)
	{
		table.DrawTruncatedPair((NAMES[i]), dim, Format::Number(attributes.Get(NAMES[i + 1]))
			+ " / " + Format::Number(chassis[NAMES[i + 1]]), bright, Truncate::MIDDLE, true);
		infoPanelLine++;
	}
}



void HardpointInfoPanel::DrawShipHardpointStats(const Rectangle &bounds, int & infoPanelLine)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < WIDTH)
		return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship & ship = **shipIt;
	const Outfit & attributes = ship.Attributes();

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	table.AddColumn(0, { COLUMN_WIDTH, Alignment::LEFT });
	table.AddColumn(COLUMN_WIDTH, { COLUMN_WIDTH, Alignment::RIGHT, Truncate::MIDDLE });
	table.SetUnderline(0, COLUMN_WIDTH);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));

	// This allows the section to stack nicely with other info panel sections,
	// But will also allow it to be called on its own in a new box if desire.
	for(int i = 0; i < infoPanelLine; i++)
	{
		table.DrawTruncatedPair(" ", dim, " ", bright, Truncate::MIDDLE, true);
	}

	// Find out how much outfit, engine, and weapon space the chassis has.
	map<string, double> chassis;
	static const vector<string> NAMES = {
		"gun ports free:", "gun ports",
		"turret mounts free:", "turret mounts"
	};

	for(unsigned i = 1; i < NAMES.size(); i += 2)
		chassis[NAMES[i]] = attributes.Get(NAMES[i]);
	for(const auto & it : ship.Outfits())
		for(auto & cit : chassis)
			cit.second -= min(0., it.second * it.first->Get(cit.first));

	for(unsigned i = 0; i < NAMES.size(); i += 2)
	{
		table.DrawTruncatedPair((NAMES[i]), dim, Format::Number(attributes.Get(NAMES[i + 1]))
			+ " / " + Format::Number(chassis[NAMES[i + 1]]), bright, Truncate::MIDDLE, true);
		infoPanelLine++;
	}
}



void HardpointInfoPanel::DrawShipBayStats(const Rectangle &bounds, int &infoPanelLine)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < WIDTH)
		return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship & ship = **shipIt;
	int BayCategoryCount = 0;

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	table.AddColumn(0, { COLUMN_WIDTH, Alignment::LEFT });
	table.AddColumn(COLUMN_WIDTH, { COLUMN_WIDTH, Alignment::RIGHT, Truncate::MIDDLE });
	table.SetUnderline(0, COLUMN_WIDTH);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));

	// This allows the section to stack nicely with other info panel sections,
	// But will also allow it to be called on its own in a new box if desire.
	for(int i = 0; i < infoPanelLine; i++)
	{
		table.DrawTruncatedPair(" ", dim, " ", bright, Truncate::MIDDLE, true);
	}

	// Print the number of bays for each bay-type we have
	for(const auto & category : GameData::GetCategory(CategoryType::BAY))
	{
		const string & bayType = category.Name();
		int totalBays = ship.BaysTotal(bayType);
		if(totalBays)
		{
			// Count  how many types of bays are displayed
			BayCategoryCount = BayCategoryCount + 1;
			// make sure the label is printed in lower case
			string bayLabel = bayType;
			transform(bayLabel.begin(), bayLabel.end(), bayLabel.begin(), ::tolower);

			table.DrawTruncatedPair(bayLabel + " bays:", dim, Format::Number(totalBays), bright, Truncate::MIDDLE, true);
		}
	}

	infoPanelLine = infoPanelLine + BayCategoryCount;
}



void HardpointInfoPanel::DrawShipEnergyHeatStats(const Rectangle &bounds, int &infoPanelLine)
{
	// Check that the specified area is big enough.
	// if(bounds.Width() < WIDTH)
		// return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship & ship = **shipIt;
	const Outfit & attributes = ship.Attributes();

	// Three columns are created.
	Table table;
	table.AddColumn(10, {240, Alignment::LEFT });
	table.AddColumn(160, {170, Alignment::RIGHT });
	table.AddColumn(240, {230, Alignment::RIGHT });
	table.SetHighlight(0, 250);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));
	table.DrawGap(10.);

	// This allows the section to stack nicely with other info panel sections,
	// But will also allow it to be called on its own in a new box if desire.
	for(int i = 0; i < infoPanelLine; i++)
	{
		table.DrawTruncatedPair(" ", dim, " ", bright, Truncate::MIDDLE, true);
	}

	table.Advance();
	table.Draw("energy", dim);
	table.Draw("heat", dim);

	tableLabels.clear();
	energyTable.clear();
	heatTable.clear();
	// Skip a spacer and the table header.
	attributesHeight += 30;

	const double idleEnergyPerFrame = attributes.Get("energy generation")
		+ attributes.Get("solar collection")
		+ attributes.Get("fuel energy")
		- attributes.Get("energy consumption")
		- attributes.Get("cooling energy");
	const double idleHeatPerFrame = attributes.Get("heat generation")
		+ attributes.Get("solar heat")
		+ attributes.Get("fuel heat")
		- ship.CoolingEfficiency() * (attributes.Get("cooling") + attributes.Get("active cooling"));
	tableLabels.push_back("idle:");
	energyTable.push_back(Format::Number(60. * idleEnergyPerFrame));
	heatTable.push_back(Format::Number(60. * idleHeatPerFrame));

	// Add energy and heat while moving to the table.
	attributesHeight += 20;
	const double movingEnergyPerFrame =
		max(attributes.Get("thrusting energy"), attributes.Get("reverse thrusting energy"))
		+ attributes.Get("turning energy")
		+ attributes.Get("afterburner energy");
	const double movingHeatPerFrame = max(attributes.Get("thrusting heat"), attributes.Get("reverse thrusting heat"))
		+ attributes.Get("turning heat")
		+ attributes.Get("afterburner heat");
	tableLabels.push_back("moving:");
	energyTable.push_back(Format::Number(-60. * movingEnergyPerFrame));
	heatTable.push_back(Format::Number(60. * movingHeatPerFrame));

	// Add energy and heat while firing to the table.
	attributesHeight += 20;
	double firingEnergy = 0.;
	double firingHeat = 0.;
	for(const auto & it : ship.Outfits())
		if(it.first->IsWeapon() && it.first->Reload())
		{
			firingEnergy += it.second * it.first->FiringEnergy() / it.first->Reload();
			firingHeat += it.second * it.first->FiringHeat() / it.first->Reload();
		}
	tableLabels.push_back("firing:");
	energyTable.push_back(Format::Number(-60. * firingEnergy));
	heatTable.push_back(Format::Number(60. * firingHeat));

	// Add energy and heat when doing shield and hull repair to the table.
	attributesHeight += 20;

	double shieldRegen = (attributes.Get("shield generation")
		+ attributes.Get("delayed shield generation"))
		* (1. + attributes.Get("shield generation multiplier"));
	bool hasShieldRegen = shieldRegen > 0.;
	double shieldEnergy = (hasShieldRegen) ? (attributes.Get("shield energy")
		+ attributes.Get("delayed shield energy"))
		* (1. + attributes.Get("shield energy multiplier")) : 0.;
	double hullRepair = (attributes.Get("hull repair rate")
		+ attributes.Get("delayed hull repair rate"))
		* (1. + attributes.Get("hull repair multiplier"));
	bool hasHullRepair = hullRepair > 0.;
	double hullEnergy = (hasHullRepair) ? (attributes.Get("hull energy")
		+ attributes.Get("delayed hull energy"))
		* (1. + attributes.Get("hull energy multiplier")) : 0.;
	tableLabels.push_back((shieldEnergy && hullEnergy) ? "shields / hull:" :
		hullEnergy ? "repairing hull:" : "charging shields:");
	energyTable.push_back(Format::Number(-60. * (shieldEnergy + hullEnergy)));
	double shieldHeat = (hasShieldRegen) ? (attributes.Get("shield heat")
		+ attributes.Get("delayed shield heat"))
		* (1. + attributes.Get("shield heat multiplier")) : 0.;
	double hullHeat = (hasHullRepair) ? (attributes.Get("hull heat")
		+ attributes.Get("delayed hull heat"))
		* (1. + attributes.Get("hull heat multiplier")) : 0.;
	heatTable.push_back(Format::Number(60. * (shieldHeat + hullHeat)));

	// Add up the maximum possible changes and add the total to the table.
	attributesHeight += 20;
	const double overallEnergy = idleEnergyPerFrame
		- movingEnergyPerFrame
		- firingEnergy
		- shieldEnergy
		- hullEnergy;
	const double overallHeat = idleHeatPerFrame
		+ movingHeatPerFrame
		+ firingHeat
		+ shieldHeat
		+ hullHeat;
	tableLabels.push_back("net change:");
	energyTable.push_back(Format::Number(60. * overallEnergy));
	heatTable.push_back(Format::Number(60. * overallHeat));

	// Add maximum values of energy and heat to the table.
	attributesHeight += 20;
	const double maxEnergy = attributes.Get("energy capacity");
	const double maxHeat = 60. * ship.HeatDissipation() * ship.MaximumHeat();
	tableLabels.push_back("max:");
	energyTable.push_back(Format::Number(maxEnergy));
	heatTable.push_back(Format::Number(maxHeat));
	// Pad by 10 pixels on the top and bottom.
	attributesHeight += 30;

	for(unsigned i = 0; i < tableLabels.size(); ++i)
	{
		// CheckHover(table, tableLabels[i]);
		table.Draw(tableLabels[i], dim);
		table.Draw(energyTable[i], bright);
		table.Draw(heatTable[i], bright);
	}

	infoPanelLine = infoPanelLine + 4;
}



void HardpointInfoPanel::DrawOutfits(const Rectangle &bounds, Rectangle& cargoBounds)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < WIDTH)
		return;

	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship & ship = **shipIt;

	// Two columns of opposite alignment are used to simulate a single visual column.
	Table table;
	table.AddColumn(0, { COLUMN_WIDTH, Alignment::LEFT });
	table.AddColumn(COLUMN_WIDTH, { COLUMN_WIDTH, Alignment::RIGHT });
	table.SetUnderline(0, COLUMN_WIDTH);
	Point start = bounds.TopLeft() + Point(10., 8.);
	table.DrawAt(start);

	// Draw the outfits in the same order used in the outfitter.
	for(const auto & cat : GameData::GetCategory(CategoryType::OUTFIT))
	{
		const string & category = cat.Name();
		auto it = outfits.find(category);
		if(it == outfits.end())
			continue;

		// Skip to the next column if there is no space for this category label
		// plus at least one outfit.
		if(table.GetRowBounds().Bottom() + 40. > bounds.Bottom())
		{
			start += Point(WIDTH, 0.);
			if(start.X() + COLUMN_WIDTH > bounds.Right())
				break;
			table.DrawAt(start);
		}

		// Draw the category label.
		table.Draw(category, bright);
		table.Advance();
		for(const Outfit * outfit : it->second)
		{
			// Check if we've gone below the bottom of the bounds.
			if(table.GetRowBounds().Bottom() > bounds.Bottom())
			{
				start += Point(WIDTH, 0.);
				if(start.X() + COLUMN_WIDTH > bounds.Right())
					break;
				table.DrawAt(start);
				table.Draw(category, bright);
				table.Advance();
			}

			// Draw the outfit name and count.
			table.DrawTruncatedPair(outfit->DisplayName(), dim,
				to_string(ship.OutfitCount(outfit)), bright, Truncate::BACK, false);
		}
		// Add an extra gap in between categories.
		table.DrawGap(10.);
	}

	// Check if this information spilled over into the cargo column.
	if(table.GetPoint().X() >= cargoBounds.Left())
	{
		double startY = table.GetRowBounds().Top() - 8.;
		cargoBounds = Rectangle::WithCorners(
			Point(cargoBounds.Left(), startY),
			Point(cargoBounds.Right(), max(startY, cargoBounds.Bottom())));
	}
}



void HardpointInfoPanel::DrawWeapons(const Rectangle &bounds)
{
	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Font & font = FontSet::Get(14);
	const Ship & ship = **shipIt;

	// Figure out how much to scale the sprite by.
	const Sprite * sprite = ship.GetSprite();
	double scale = 0.;
	if(sprite)
		scale = min(1., min((WIDTH - 10) / sprite->Width(), (WIDTH - 10) / sprite->Height()));

	// Figure out the left- and right-most hardpoints on the ship. If they are
	// too far apart, the scale may need to be reduced.
	// Also figure out how many weapons of each type are on each side.
	double maxX = 0.;
	int count[2][2] = { {0, 0}, {0, 0} };
	for(const Hardpoint & hardpoint : ship.Weapons())
	{
		// Multiply hardpoint X by 2 to convert to sprite pixels.
		maxX = max(maxX, fabs(2. * hardpoint.GetPoint().X()));
		++count[hardpoint.GetPoint().X() >= 0.][hardpoint.IsTurret()];
	}
	// If necessary, shrink the sprite to keep the hardpoints inside the labels.
	// The width of this UI block will be 2 * (LABEL_WIDTH + HARDPOINT_DX).
	static const double LABEL_WIDTH = 150.;
	static const double LABEL_DX = 95.;
	static const double LABEL_PAD = 5.;
	if(maxX > (LABEL_DX - LABEL_PAD))
		scale = min(scale, (LABEL_DX - LABEL_PAD) / (2. * maxX));

	// Draw the ship, using the black silhouette swizzle.
	if(sprite)
	{
		SpriteShader::Draw(sprite, bounds.Center(), scale, 28);
		OutlineShader::Draw(sprite, bounds.Center(), scale * Point(sprite->Width(), sprite->Height()), Color(.5f));
	}

	// Figure out how tall each part of the weapon listing will be.
	int gunRows = max(count[0][0], count[1][0]);
	int turretRows = max(count[0][1], count[1][1]);
	// If there are both guns and turrets, add a gap of ten pixels.
	double height = 20. * (gunRows + turretRows) + 10. * (gunRows && turretRows);

	double gunY = bounds.Top() + .5 * (bounds.Height() - height);
	double turretY = gunY + 20. * gunRows + 10. * (gunRows != 0);
	double nextY[2][2] = {
		{gunY + 20. * (gunRows - count[0][0]), turretY + 20. * (turretRows - count[0][1])},
		{gunY + 20. * (gunRows - count[1][0]), turretY + 20. * (turretRows - count[1][1])} };

	int index = 0;
	const double centerX = bounds.Center().X();
	const double labelCenter[2] = { -.5 * LABEL_WIDTH - LABEL_DX, LABEL_DX + .5 * LABEL_WIDTH };
	const double fromX[2] = { -LABEL_DX + LABEL_PAD, LABEL_DX - LABEL_PAD };
	static const double LINE_HEIGHT = 20.;
	static const double TEXT_OFF = .5 * (LINE_HEIGHT - font.Height());
	static const Point LINE_SIZE(LABEL_WIDTH, LINE_HEIGHT);
	Point topFrom;
	Point topTo;
	Color topColor;
	bool hasTop = false;
	auto layout = Layout(static_cast<int>(LABEL_WIDTH), Truncate::BACK);
	for(const Hardpoint & hardpoint : ship.Weapons())
	{
		string name = "[empty]";
		if(hardpoint.GetOutfit())
			name = hardpoint.GetOutfit()->DisplayName();

		bool isRight = (hardpoint.GetPoint().X() >= 0.);
		bool isTurret = hardpoint.IsTurret();

		double & y = nextY[isRight][isTurret];
		double x = centerX + (isRight ? LABEL_DX : -LABEL_DX - LABEL_WIDTH);
		bool isHover = (index == hoverIndex);
		layout.align = isRight ? Alignment::LEFT : Alignment::RIGHT;
		font.Draw({ name, layout }, Point(x, y + TEXT_OFF), isHover ? bright : dim);
		Point zoneCenter(labelCenter[isRight], y + .5 * LINE_HEIGHT);
		zones.emplace_back(zoneCenter, LINE_SIZE, index);

		// Determine what color to use for the line.
		Color color;
		if(isTurret)
			color = *GameData::Colors().Get(isHover ? "player info hardpoint turret hover"
				: "player info hardpoint turret");
		else
			color = *GameData::Colors().Get(isHover ? "player info hardpoint gun hover"
				: "player info hardpoint gun");

		// Draw the line.
		Point from(fromX[isRight], zoneCenter.Y());
		Point to = bounds.Center() + (2. * scale) * hardpoint.GetPoint();
		DrawLine(from, to, color);
		if(isHover)
		{
			topFrom = from;
			topTo = to;
			topColor = color;
			hasTop = true;
		}

		y += LINE_HEIGHT;
		++index;
	}
	// Make sure the line for whatever hardpoint we're hovering is always on top.
	if(hasTop)
		DrawLine(topFrom, topTo, topColor);

	// Re-positioning weapons.
	if(draggingIndex >= 0)
	{
		const Outfit * outfit = ship.Weapons()[draggingIndex].GetOutfit();
		string name = outfit ? outfit->DisplayName() : "[empty]";
		Point pos(hoverPoint.X() - .5 * font.Width(name), hoverPoint.Y());
		font.Draw(name, pos + Point(1., 1.), Color(0., 1.));
		font.Draw(name, pos, bright);
	}
}



void HardpointInfoPanel::DrawCargo(const Rectangle &bounds)
{
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	Color backColor = *GameData::Colors().Get("faint");
	const Ship & ship = **shipIt;

	// Cargo list.
	const CargoHold & cargo = (player.Cargo().Used() ? player.Cargo() : ship.Cargo());
	Table table;
	table.AddColumn(0, { COLUMN_WIDTH, Alignment::LEFT });
	table.AddColumn(COLUMN_WIDTH, { COLUMN_WIDTH, Alignment::RIGHT });
	table.SetUnderline(-5, COLUMN_WIDTH + 5);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));

	double endY = bounds.Bottom() - 30. * (cargo.Passengers() != 0);
	bool hasSpace = (table.GetRowBounds().Bottom() < endY);
	if((cargo.CommoditiesSize() || cargo.HasOutfits() || cargo.MissionCargoSize()) && hasSpace)
	{
		table.Draw("Cargo", bright);
		table.Advance();
		hasSpace = (table.GetRowBounds().Bottom() < endY);
	}
	if(cargo.CommoditiesSize() && hasSpace)
	{
		for(const auto & it : cargo.Commodities())
		{
			if(!it.second)
				continue;

			commodityZones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), it.first);
			if(it.first == selectedCommodity)
				table.DrawHighlight(backColor);

			table.Draw(it.first, dim);
			table.Draw(to_string(it.second), bright);

			// Truncate the list if there is not enough space.
			if(table.GetRowBounds().Bottom() >= endY)
			{
				hasSpace = false;
				break;
			}
		}
		table.DrawGap(10.);
	}
	if(cargo.HasOutfits() && hasSpace)
	{
		for(const auto & it : cargo.Outfits())
		{
			if(!it.second)
				continue;

			plunderZones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), it.first);
			if(it.first == selectedPlunder)
				table.DrawHighlight(backColor);

			// For outfits, show how many of them you have and their total mass.
			bool isSingular = (it.second == 1 || it.first->Get("installable") < 0.);
			string name = (isSingular ? it.first->DisplayName() : it.first->PluralName());
			if(!isSingular)
				name += " (" + to_string(it.second) + "x)";
			table.Draw(name, dim);

			double mass = it.first->Mass() * it.second;
			table.Draw(Format::Number(mass), bright);

			// Truncate the list if there is not enough space.
			if(table.GetRowBounds().Bottom() >= endY)
			{
				hasSpace = false;
				break;
			}
		}
		table.DrawGap(10.);
	}
	if(cargo.HasMissionCargo() && hasSpace)
	{
		for(const auto & it : cargo.MissionCargo())
		{
			// Capitalize the name of the cargo.
			table.Draw(Format::Capitalize(it.first->Cargo()), dim);
			table.Draw(to_string(it.second), bright);

			// Truncate the list if there is not enough space.
			if(table.GetRowBounds().Bottom() >= endY)
				break;
		}
		table.DrawGap(10.);
	}
	if(cargo.Passengers() && endY >= bounds.Top())
	{
		table.DrawAt(Point(bounds.Left(), endY) + Point(10., 8.));
		table.Draw("passengers:", dim);
		table.Draw(to_string(cargo.Passengers()), bright);
	}
}



void HardpointInfoPanel::DrawLine(const Point& from, const Point& to, const Color& color) const
{
	Color black(0.f, 1.f);
	Point mid(to.X(), from.Y());

	LineShader::Draw(from, mid, 3.5f, black);
	LineShader::Draw(mid, to, 3.5f, black);
	LineShader::Draw(from, mid, 1.5f, color);
	LineShader::Draw(mid, to, 1.5f, color);
}



bool HardpointInfoPanel::Hover(const Point& point)
{
	if(shipIt == panelState.Ships().end())
		return true;

	hoverPoint = point;

	hoverIndex = -1;
	const vector<Hardpoint>& weapons = (**shipIt).Weapons();
	bool dragIsTurret = (draggingIndex >= 0 && weapons[draggingIndex].IsTurret());
	for(const auto & zone : zones)
	{
		bool isTurret = weapons[zone.Value()].IsTurret();
		if(zone.Contains(hoverPoint) && (draggingIndex == -1 || isTurret == dragIsTurret))
			hoverIndex = zone.Value();
	}

	return true;
}



void HardpointInfoPanel::Rename(const string& name)
{
	if(shipIt != panelState.Ships().end() && !name.empty())
	{
		player.RenameShip(shipIt->get(), name);
		UpdateInfo();
	}
}



bool HardpointInfoPanel::CanDump() const
{
	if(panelState.CanEdit() || shipIt == panelState.Ships().end())
		return false;

	CargoHold & cargo = (*shipIt)->Cargo();
	return (selectedPlunder && cargo.Get(selectedPlunder) > 0) || cargo.CommoditiesSize() || cargo.OutfitsSize();
}



void HardpointInfoPanel::Dump()
{
	if(!CanDump())
		return;

	CargoHold & cargo = (*shipIt)->Cargo();
	int commodities = (*shipIt)->Cargo().CommoditiesSize();
	int amount = cargo.Get(selectedCommodity);
	int plunderAmount = cargo.Get(selectedPlunder);
	int64_t loss = 0;
	if(amount)
	{
		int64_t basis = player.GetBasis(selectedCommodity, amount);
		loss += basis;
		player.AdjustBasis(selectedCommodity, -basis);
		(*shipIt)->Jettison(selectedCommodity, amount);
	}
	else if(plunderAmount > 0)
	{
		loss += plunderAmount * selectedPlunder->Cost();
		(*shipIt)->Jettison(selectedPlunder, plunderAmount);
	}
	else if(commodities)
	{
		for(const auto & it : cargo.Commodities())
		{
			int64_t basis = player.GetBasis(it.first, it.second);
			loss += basis;
			player.AdjustBasis(it.first, -basis);
			(*shipIt)->Jettison(it.first, it.second);
		}
	}
	else
	{
		for(const auto & it : cargo.Outfits())
		{
			loss += it.first->Cost() * max(0, it.second);
			(*shipIt)->Jettison(it.first, it.second);
		}
	}
	selectedCommodity.clear();
	selectedPlunder = nullptr;

	info.Update(**shipIt, player);
	if(loss)
		Messages::Add("You jettisoned " + Format::CreditString(loss) + " worth of cargo."
			, Messages::Importance::High);
}



void HardpointInfoPanel::DumpPlunder(int count)
{
	int64_t loss = 0;
	count = min(count, (*shipIt)->Cargo().Get(selectedPlunder));
	if(count > 0)
	{
		loss += count * selectedPlunder->Cost();
		(*shipIt)->Jettison(selectedPlunder, count);
		info.Update(**shipIt, player);

		if(loss)
			Messages::Add("You jettisoned " + Format::CreditString(loss) + " worth of cargo."
				, Messages::Importance::High);
	}
}



void HardpointInfoPanel::DumpCommodities(int count)
{
	int64_t loss = 0;
	count = min(count, (*shipIt)->Cargo().Get(selectedCommodity));
	if(count > 0)
	{
		int64_t basis = player.GetBasis(selectedCommodity, count);
		loss += basis;
		player.AdjustBasis(selectedCommodity, -basis);
		(*shipIt)->Jettison(selectedCommodity, count);
		info.Update(**shipIt, player);

		if(loss)
			Messages::Add("You jettisoned " + Format::CreditString(loss) + " worth of cargo."
				, Messages::Importance::High);
	}
}



void HardpointInfoPanel::Disown()
{
	// Make sure a ship really is selected.
	if(shipIt == panelState.Ships().end() || shipIt->get() == player.Flagship())
		return;

	const auto ship = shipIt;
	if(shipIt != panelState.Ships().begin())
		--shipIt;

	player.DisownShip(ship->get());
	panelState.Disown(ship);
	UpdateInfo();
}
