/* PreferencesPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef PREFERENCES_PANEL_H_
#define PREFERENCES_PANEL_H_

#include "Panel.h"

#include "ClickZone.h"
#include "Command.h"
#include "Plugins.h"
#include "Point.h"
#include "text/WrappedText.h"

#include <future>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>



// UI panel for editing preferences, especially the key mappings.
class PreferencesPanel : public Panel {
public:
	PreferencesPanel();

	// Draw this panel.
	virtual void Draw() override;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, int clicks) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Scroll(double dx, double dy) override;

	virtual void EndEditing() override;


private:
	void DrawControls();
	void DrawSettings();
	void DrawPlugins();
	void DrawPluginInstalls();

	void DrawTooltips();

	void Exit();

private:
	int editing;
	int selected;
	int hover;
	int oldSelected;
	int oldHover;
	int latest;
	// Which page of the preferences we're on.
	char page = 'c';

	Point hoverPoint;
	int hoverCount = 0;
	std::string hoverItem;
	std::string tooltip;
	WrappedText hoverText;

	int currentSettingsPage = 0;

	std::string selectedPlugin;

	nlohmann::json pluginInstallList;
	Plugins::InstallData selectedPluginInstall;
	unsigned int pluginInstallPages = 1;
	unsigned int currentPluginInstallPage = 0;
	bool downloadedInfo = false;
	std::vector<std::future<void>> installFeedbacks;

	std::vector<ClickZone<Command>> zones;
	std::vector<ClickZone<std::string>> prefZones;
	std::vector<ClickZone<std::string>> pluginZones;
	std::vector<ClickZone<Plugins::InstallData>> pluginInstallZones;
};



#endif
