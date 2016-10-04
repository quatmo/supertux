//  SuperTux
//  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmail.com>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "supertux/game_manager.hpp"

#include <sstream>

#include "gui/menu_manager.hpp"
#include "supertux/game_session.hpp"
#include "supertux/gameconfig.hpp"
#include "supertux/globals.hpp"
#include "supertux/levelset_screen.hpp"
#include "supertux/savegame.hpp"
#include "supertux/screen.hpp"
#include "supertux/screen_fade.hpp"
#include "supertux/screen_manager.hpp"
#include "supertux/world.hpp"
#include "supertux/levelsavestate.hpp"
#include "control/input_manager.hpp"
#include "util/file_system.hpp"
#include "util/log.hpp"
#include "util/reader.hpp"
#include "util/reader_document.hpp"
#include "util/reader_mapping.hpp"
#include "worldmap/worldmap.hpp"
#include "worldmap/level.hpp"

GameManager::GameManager() :
  m_world(),
  m_savegame()
{
}

GameManager::~GameManager()
{
}

void
GameManager::start_level(std::unique_ptr<World> world, const std::string& level_filename)
{
  m_world = std::move(world);
  m_savegame.reset(new Savegame(m_world->get_savegame_filename()));
  m_savegame->load();

  std::unique_ptr<Screen> screen(new LevelsetScreen(m_world->get_basedir(),
                                                    level_filename,
                                                    *m_savegame));
  ScreenManager::current()->push_screen(std::move(screen));
}

void
GameManager::start_worldmap(std::unique_ptr<World> world)
{
  try
  {
    m_world = std::move(world);
    m_savegame.reset(new Savegame(m_world->get_savegame_filename()));
    m_savegame->load();
    LevelSaveState::save(LevelSaveState(m_world->get_basedir()));

    auto last_worldmap = m_savegame->get_player_status()->last_worldmap;
    auto worldmap = new worldmap::WorldMap(last_worldmap.length() ? // has last worldmap?
                                           last_worldmap : m_world->get_worldmap_filename(),
                                           *m_savegame);
    ScreenManager::current()->push_screen(std::unique_ptr<Screen>(worldmap));

    if (LevelSaveState::getLoading() && LevelSaveState::get().level != "" && LevelSaveState::get().sector != "")
    {
      worldmap->setup();
      worldmap::LevelTile* level = worldmap->at_level();
      if (!level)
      {
        log_warning << "No level to enter at worldmap" << std::endl;
      }
      else
      {
        std::string levelfile = m_world->get_basedir() + "/" + level->get_name();
        if (LevelSaveState::get().level != levelfile)
        {
          log_warning << "Saved level " << LevelSaveState::get().level << " does not match worlmap level " << levelfile << std::endl;
        }
        else
        {
          // Hack: press a button on a controller
          InputManager::current()->get_controller()->reset();
          InputManager::current()->get_controller()->set_control(Controller::ACTION, true);
          worldmap->update(0.0f);
          InputManager::current()->get_controller()->set_control(Controller::ACTION, false);
        }
      }
    }
  }
  catch(std::exception& e)
  {
    log_fatal << "Couldn't start world: " << e.what() << std::endl;
  }
}

std::string
GameManager::get_level_name(const std::string& filename) const
{
  try
  {
    register_translation_directory(filename);
    auto doc = ReaderDocument::parse(filename);
    auto root = doc.get_root();

    if(root.get_name() != "supertux-level") {
      return "";
    } else {
      auto mapping = root.get_mapping();
      std::string name;
      mapping.get("name", name);
      return name;
    }
  }
  catch(const std::exception& e)
  {
    log_warning << "Problem getting name of '" << filename << "': "
                << e.what() << std::endl;
    return "";
  }
}

/* EOF */
