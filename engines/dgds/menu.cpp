/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/system.h"

#include "audio/mixer.h"
#include "graphics/cursorman.h"
#include "graphics/font.h"
#include "graphics/fontman.h"
#include "graphics/managed_surface.h"
#include "graphics/palette.h"
#include "graphics/surface.h"

#include "dgds/includes.h"
#include "dgds/font.h"
#include "dgds/menu.h"
#include "dgds/music.h"
#include "dgds/request.h"
#include "dgds/sound.h"

namespace Dgds {


// TODO: These are the IDs for Dragon, this code needs updates for China/Beamish/etc
enum MenuButtonIds {
	kMenuMainPlay = 120,
	kMenuMainControls = 20,
	kMenuMainOptions = 121,
	kMenuMainCalibrate = 118,
	kMenuMainFiles = 119,
	kMenuMainQuit = 122,

	kMenuControlsVCR = 127,
	kMenuControlsPlay = 128,

	kMenuSliderControlsDifficulty = 123,
	kMenuSliderControlsTextSpeed = 125,
	kMenuSliderControlsDetailLevel = 131,

	kMenuOptionsJoystickOnOff = 139,
	kMenuOptionsJoystickOnOffHoC = 174,
	kMenuOptionsMouseOnOff = 138,
	kMenuOptionsMouseOnOffHoC = 173,
	kMenuOptionsSoundsOnOff = 137,
	kMenuOptionsMusicOnOff = 140,
	kMenuOptionsSoundsOnOffHoC = 175,
	kMenuOptionsMusicOnOffHoC = 171,
	kMenuOptionsVCR = 135,
	kMenuOptionsPlay = 136,

	kMenuCalibrateJoystickBtn = 145,
	kMenuCalibrateMouseBtn = 146,
	kMenuCalibrateVCR = 144,
	kMenuCalibratePlay = 147,
	kMenuCalibrateVCRHoC = 159,
	kMenuCalibratePlayHoC = 158,

	kMenuFilesSave = 107,
	kMenuFilesRestore = 106,
	kMenuFilesRestart = 105,
	kMenuFilesVCR = 103,
	kMenuFilesPlay = 130,

	kMenuSavePrevious = 58,
	kMenuSaveNext = 59,
	kMenuSaveSave = 53,
	kMenuSaveCancel = 54,
	kMenuSaveChangeDirectory = 55,

	kMenuChangeDirectoryOK = 95,
	kMenuChangeDirectoryCancel = 96,

	kMenuMouseCalibrationCalibrate = 157,
	kMenuMouseCalibrationPlay = 155,

	kMenuJoystickCalibrationOK = 132,

	kMenuQuitYes = 134,
	kMenuQuitNo = 133,

	kMenuMaybeBetterSaveYes = 137,
	kMenuMaybeBetterSaveNo = 138,

	// Intro menu in Rise of the Dragon
	kMenuIntroSkip = 143,
	kMenuIntroPlay = 144,

	// Intro menu in Heart of China / Willy Beamish
	kMenuIntroJumpToIntroduction = 156,
	kMenuIntroJumpToGame = 157,
	kMenuIntroRestore = 150,

	kMenuRestartYes = 163,
	kMenuRestartNo = 164,

	kMenuGameOverQuit = 169,
	kMenuGameOverRestart = 168,
	kMenuGameOverRestore = 170,
};

Menu::Menu() : _curMenu(kMenuNone), _dragGadget(nullptr) {
	_screenBuffer.create(SCREEN_WIDTH, SCREEN_HEIGHT, Graphics::PixelFormat::createFormatCLUT8());
}

Menu::~Menu() {
	_screenBuffer.free();
}

void Menu::setRequestData(const REQFileData &data) {
	for (auto &req : data._requests) {
		_menuRequests[req._fileNum] = req;
	}
}

void Menu::setScreenBuffer() {
	Graphics::Surface *dst = g_system->lockScreen();
	_screenBuffer.copyFrom(*dst);
	g_system->unlockScreen();
}

bool Menu::updateOptionsGadget(Gadget *gadget) {
	DgdsEngine *engine = static_cast<DgdsEngine *>(g_engine);
	Audio::Mixer *mixer = engine->_mixer;

	switch (gadget->_gadgetNo) {
	case kMenuOptionsJoystickOnOff:
	case kMenuOptionsJoystickOnOffHoC:
		gadget->_buttonName = "JOYSTICK ON";
		return false;
	case kMenuOptionsMouseOnOff:
	case kMenuOptionsMouseOnOffHoC:
		gadget->_buttonName = "MOUSE ON";
		return false;
	case kMenuOptionsSoundsOnOff: // same id as kMenuMaybeBetterSaveYes
	case kMenuOptionsSoundsOnOffHoC:
		gadget->_buttonName = (!mixer->isSoundTypeMuted(Audio::Mixer::kSFXSoundType)) ? "SOUNDS ON" : "SOUNDS OFF";
		return true;
	case kMenuOptionsMusicOnOff:
	case kMenuOptionsMusicOnOffHoC:
		gadget->_buttonName = (!mixer->isSoundTypeMuted(Audio::Mixer::kMusicSoundType)) ? "MUSIC ON" : "MUSIC OFF";
		return true;
	default:
		return false;
	}
}

void Menu::configureGadget(MenuId menu, Gadget *gadget) {
	DgdsEngine *engine = static_cast<DgdsEngine *>(g_engine);

	// a bit of a hack - set up the gadget with the correct value before we draw it.
	if (menu == kMenuControls) {
		SliderGadget *slider = dynamic_cast<SliderGadget *>(gadget);
		switch (gadget->_gadgetNo) {
		case kMenuSliderControlsDifficulty:
			assert(slider);
			slider->setSteps(3, false);
			slider->setValue(engine->getDifficulty()); // TODO: set a difficulty value
			break;
		case kMenuSliderControlsTextSpeed:
			assert(slider);
			slider->setSteps(10, false);
			slider->setValue(9 - engine->getTextSpeed());
			break;
		case kMenuSliderControlsDetailLevel:
			assert(slider);
			slider->setSteps(2, true);
			slider->setValue(engine->getDetailLevel());
			break;
		default:
			break;
			// do nothing.
		}
	} else if (menu == kMenuOptions) {
		updateOptionsGadget(gadget);
	}
}

void Menu::drawMenu(MenuId menu) {
	bool firstDraw = (_curMenu != menu);
	_curMenu = menu;

	Common::Array<Common::SharedPtr<Gadget> > gadgets = _menuRequests[_curMenu]._gadgets;

	// Restore background when drawing submenus
	g_system->copyRectToScreen(_screenBuffer.getPixels(), _screenBuffer.pitch, 0, 0, _screenBuffer.w, _screenBuffer.h);

	// This is not very efficient, but it only happens once when the menu is opened.
	Graphics::Surface *screen = g_system->lockScreen();
	Graphics::ManagedSurface managed(screen->w, screen->h, screen->format);
	managed.blitFrom(*screen);
	_menuRequests[_curMenu].drawBg(&managed);

	for (Common::SharedPtr<Gadget> &gptr : gadgets) {
		Gadget *gadget = gptr.get();
		if (gadget->_gadgetType == kGadgetButton || gadget->_gadgetType == kGadgetSlider) {
			if (firstDraw)
				configureGadget(menu, gadget);
			gadget->draw(&managed);
		}
	}

	drawMenuText(managed);

	// Can't use transparent blit here as the font is often color 0.
	screen->copyRectToSurface(*managed.surfacePtr(), 0, 0, Common::Rect(screen->w, screen->h));

	g_system->unlockScreen();
	g_system->updateScreen();
}

void Menu::drawMenuText(Graphics::ManagedSurface &dst) {
	Common::Array<Common::SharedPtr<Gadget> > gadgets = _menuRequests[_curMenu]._gadgets;
	Common::Array<TextItem> textItems = _menuRequests[_curMenu]._textItemList;

	if (gadgets.empty())
		return;

	// TODO: Get the parent coordinates properly
	uint16 parentX = gadgets[0].get()->_parentX;
	uint16 parentY = gadgets[0].get()->_parentY;
	uint16 pos = 0;

	for (TextItem &textItem : textItems) {
		// HACK: Skip the first entry, which corresponds to the header
		if (pos == 0) {
			pos++;
			continue;
		}

		const DgdsFont *font = RequestData::getMenuFont();
		int w = font->getStringWidth(textItem._txt);
		font->drawString(dst.surfacePtr(), textItem._txt, parentX + textItem._x, parentY + textItem._y, w, 0);
		pos++;
	}
}

Gadget *Menu::getClickedMenuItem(const Common::Point &mouseClick) {
	if (_curMenu == kMenuNone)
		return nullptr;

	Common::Array<Common::SharedPtr<Gadget> > gadgets = _menuRequests[_curMenu]._gadgets;

	for (Common::SharedPtr<Gadget> &gptr : gadgets) {
		Gadget *gadget = gptr.get();
		if (gadget->_gadgetType == kGadgetButton || gadget->_gadgetType == kGadgetSlider) {
			if (gadget->containsPoint(mouseClick)) {
				return gadget;
			}
		}
	}

	return nullptr;
}

void Menu::onMouseLDown(const Common::Point &mouse) {
	SliderGadget *slider = dynamic_cast<SliderGadget *>(getClickedMenuItem(mouse));
	if (slider) {
		_dragGadget = slider;
		_dragStartPt = mouse;
	}
}

void Menu::onMouseMove(const Common::Point &mouse) {
	if (!_dragGadget)
		return;
	_dragGadget->onDrag(mouse);
	drawMenu(_curMenu);
}

void Menu::onMouseLUp(const Common::Point &mouse) {
	DgdsEngine *engine = static_cast<DgdsEngine *>(g_engine);
	if (_dragGadget && mouse != _dragStartPt) {
		int16 setting = _dragGadget->onDragFinish(mouse);
		switch (_dragGadget->_gadgetNo) {
		case kMenuSliderControlsDifficulty:
			engine->setDifficulty(setting);
			break;
		case kMenuSliderControlsTextSpeed:
			engine->setTextSpeed(9 - setting);
			break;
		case kMenuSliderControlsDetailLevel:
			engine->setDetailLevel(static_cast<DgdsDetailLevel>(setting));
			break;
		}
		drawMenu(_curMenu);
		_dragGadget = nullptr;
		_dragStartPt = Common::Point();
		return;
	}
	_dragGadget = nullptr;

	Gadget *gadget = getClickedMenuItem(mouse);
	bool isToggle = false;
	if (!gadget)
		return;

	// Click animation
	if (dynamic_cast<ButtonGadget *>(gadget)) {
		gadget->toggle(false);
		isToggle = updateOptionsGadget(gadget);
		drawMenu(_curMenu);
		g_system->delayMillis(500);
		gadget->toggle(true);
	}

	if (_curMenu == kMenuOptions)
		handleClickOptionsMenu(mouse);
	else if (_curMenu == kMenuSkipPlayIntro)
		handleClickSkipPlayIntroMenu(mouse);
	else
		handleClick(mouse);

	if (isToggle)
		drawMenu(_curMenu);
}

void Menu::handleClick(const Common::Point &mouse) {
	DgdsEngine *engine = static_cast<DgdsEngine *>(g_engine);
	Gadget *gadget = getClickedMenuItem(mouse);
	int16 clickedMenuItem = gadget->_gadgetNo;

	switch (clickedMenuItem) {
	case kMenuMainPlay:
	case kMenuControlsPlay:
	case kMenuOptionsPlay:
	case kMenuCalibratePlay:
	case kMenuCalibratePlayHoC:
	case kMenuFilesPlay:
	case kMenuMouseCalibrationPlay:
	case kMenuMaybeBetterSaveNo:
		_curMenu = kMenuNone;
		CursorMan.showMouse(false);
		break;
	case kMenuMainControls:
		drawMenu(kMenuControls);
		break;
	case kMenuMainOptions:
		drawMenu(kMenuOptions);
		break;
	case kMenuMainCalibrate:
	case kMenuJoystickCalibrationOK:
	case kMenuMouseCalibrationCalibrate: // NOTE: same ID as kMenuJumpToGame (for HOC)
		if (_curMenu == kMenuSkipPlayIntro) {
			hideMenu();
			engine->setShowClock(true);
			engine->changeScene(24);
		} else {
			drawMenu(kMenuCalibrate);
		}
		break;
	case kMenuMainFiles:
	case kMenuSaveCancel:
		drawMenu(kMenuFiles);
		break;
	case kMenuMainQuit:
		drawMenu(kMenuReallyQuit);
		break;
	case kMenuCalibrateVCR: // NOTE: same ID as kMenuIntroPlay
		drawMenu(kMenuMain);
		break;
	case kMenuControlsVCR:
	case kMenuOptionsVCR:
	case kMenuCalibrateVCRHoC:
	case kMenuFilesVCR:
	case kMenuQuitNo:
	case kMenuRestartNo:
		drawMenu(kMenuMain);
		break;
	case kMenuCalibrateJoystickBtn:
		drawMenu(kMenuCalibrateJoystick);
		break;
	case kMenuCalibrateMouseBtn:
		drawMenu(kMenuCalibrateMouse);
		break;
	case kMenuChangeDirectoryCancel:
		drawMenu(kMenuSaveDlg);
		break;
	case kMenuFilesRestore:
	case kMenuGameOverRestore:
	case kMenuIntroRestore:
		if (g_engine->loadGameDialog())
			hideMenu();
		else
			drawMenu(_curMenu);
		break;
	case kMenuFilesRestart:
		drawMenu(kMenuRestart);
		break;
	case kMenuFilesSave: // TODO: Add an option to support original save/load dialogs?
	case kMenuSavePrevious:
	case kMenuSaveNext:
	case kMenuSaveSave:
	case kMenuMaybeBetterSaveYes:
		if (g_engine->saveGameDialog())
			hideMenu();
		else
			drawMenu(_curMenu);
		break;
	case kMenuSaveChangeDirectory:
		drawMenu(kMenuChangeDir);
		break;
	case kMenuChangeDirectoryOK:
		// TODO
		debug("Clicked change directory - %d", clickedMenuItem);
		break;
	case kMenuQuitYes:
		g_engine->quitGame();
		break;
	case kMenuRestartYes:
		engine->restartGame();
		break;
	case kMenuGameOverQuit:
		drawMenu(kMenuReallyQuit);
		break;
	case kMenuGameOverRestart:
		drawMenu(kMenuRestart);
		break;
	case kMenuSliderControlsDifficulty: {
		int16 setting = dynamic_cast<SliderGadget *>(gadget)->onClick(mouse);
		engine->setDifficulty(setting);
		// redraw for update.
		drawMenu(_curMenu);
		break;
	}
	case kMenuSliderControlsTextSpeed: {
		int16 setting = dynamic_cast<SliderGadget *>(gadget)->onClick(mouse);
		engine->setTextSpeed(9 - setting);
		drawMenu(_curMenu);
		break;
	}
	case kMenuSliderControlsDetailLevel: {
		int16 setting = dynamic_cast<SliderGadget *>(gadget)->onClick(mouse);
		engine->setDetailLevel(static_cast<DgdsDetailLevel>(setting));
		drawMenu(_curMenu);
		break;
	}
	default:
		debug("Clicked ID %d", clickedMenuItem);
		break;
	}
}

void Menu::handleClickOptionsMenu(const Common::Point &mouse) {
	DgdsEngine *engine = static_cast<DgdsEngine *>(g_engine);
	Audio::Mixer *mixer = engine->_mixer;
	Gadget *gadget = getClickedMenuItem(mouse);
	int16 clickedMenuItem = gadget->_gadgetNo;
	Audio::Mixer::SoundType soundType = Audio::Mixer::kMusicSoundType;
	DgdsMidiPlayer *midiPlayer = engine->_soundPlayer->getMidiPlayer();

	switch (clickedMenuItem) {
	case kMenuOptionsJoystickOnOff:
	case kMenuOptionsJoystickOnOffHoC:
	case kMenuOptionsMouseOnOff:  // same id as kMenuMaybeBetterSaveNo
	case kMenuOptionsMouseOnOffHoC:
		// Do nothing - we don't toggle joystick or mouse functionality
		break;
	case kMenuOptionsSoundsOnOff: // same id as kMenuMaybeBetterSaveYes
	case kMenuOptionsSoundsOnOffHoC:
		soundType = Audio::Mixer::kSFXSoundType;
		// fall through
	case kMenuOptionsMusicOnOff:
	case kMenuOptionsMusicOnOffHoC:
		if (!mixer->isSoundTypeMuted(soundType)) {
			mixer->muteSoundType(soundType, true);
			midiPlayer->syncVolume();
			midiPlayer->pause();
		} else {
			mixer->muteSoundType(soundType, false);
			midiPlayer->syncVolume();
			midiPlayer->resume();
		}

		updateOptionsGadget(gadget);
		break;
	default:
		handleClick(mouse);
		break;
	}
}

void Menu::handleClickSkipPlayIntroMenu(const Common::Point &mouse) {
	DgdsEngine *engine = static_cast<DgdsEngine *>(g_engine);
	Gadget *gadget = getClickedMenuItem(mouse);
	int16 clickedMenuItem = gadget->_gadgetNo;

	switch (clickedMenuItem) {
	case kMenuIntroPlay:
		hideMenu();
		break;
	case kMenuIntroSkip:
		hideMenu();
		engine->setShowClock(true);
		engine->changeScene(5);
		break;
	case kMenuIntroJumpToIntroduction:
		hideMenu();
		if (engine->getGameId() == GID_HOC)
			engine->changeScene(98);
		else if (engine->getGameId() == GID_WILLY)
			engine->changeScene(24);
		break;
	case kMenuIntroJumpToGame:
		hideMenu();
		if (engine->getGameId() == GID_HOC)
			engine->changeScene(24);
		else if (engine->getGameId() == GID_WILLY)
			warning("TODO: Jump to game");
		break;
	default:
		handleClick(mouse);
		break;
	}
}

void Menu::toggleGadget(int16 gadgetId, bool enable) {
	Common::Array<Common::SharedPtr<Gadget> > gadgets = _menuRequests[_curMenu]._gadgets;

	for (Common::SharedPtr<Gadget> &gptr : gadgets) {
		Gadget *gadget = gptr.get();
		if (gadget->_gadgetNo == gadgetId) {
			gadget->toggle(enable);
			return;
		}
	}
}

} // End of namespace Dgds
