//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2008-2013 Joerg Henrichs
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "race/grand_prix_manager.hpp"

#include "config/user_config.hpp"
#include "grand_prix_data.hpp"
#include "io/file_manager.hpp"
#include "utils/string_utils.hpp"

#include <algorithm>
#include <set>
#include <sstream>

GrandPrixManager *grand_prix_manager = NULL;

const char* GrandPrixManager::SUFFIX = ".grandprix";

// ----------------------------------------------------------------------------
void GrandPrixManager::loadFiles()
{
    std::set<std::string> dirs;

    //Add all the directories to a set to avoid duplicates
    dirs.insert(file_manager->getAsset(FileManager::GRANDPRIX, ""));
    dirs.insert(file_manager->getGPDir());
    dirs.insert(UserConfigParams::m_additional_gp_directory);

    for (std::set<std::string>::const_iterator it = dirs.begin(); it != dirs.end(); ++it)
    {
        std::string dir = *it;
        if (!dir.empty() && dir[dir.size() - 1] == '/')
            loadDir(dir);
    }
}

// ----------------------------------------------------------------------------
void GrandPrixManager::loadDir(const std::string& dir)
{
    Log::info("GrandPrixManager", "Loading Grand Prix files from %s", dir.c_str());
    assert(!dir.empty() && dir[dir.size() - 1] == '/');

    // Findout which grand prixs are available and load them
    std::set<std::string> result;
    file_manager->listFiles(result, dir);
    for(std::set<std::string>::iterator i = result.begin(); i != result.end(); i++)
    {
        if (StringUtils::hasSuffix(*i, SUFFIX))
            load(dir + *i);
    }   // for i
}   // loadDir

// ----------------------------------------------------------------------------
void GrandPrixManager::load(const std::string& filename)
{
    GrandPrixData* gp;

    try
    {
        gp = new GrandPrixData(filename);
        m_gp_data.push_back(gp);
        Log::debug("GrandPrixManager", "Grand Prix '%s' loaded from %s",
            gp->getId().c_str(), filename.c_str());
    }
    catch (std::logic_error& er)
    {
        Log::error("GrandPrixManager", "Ignoring GP %s (%s)\n",
            filename.c_str(), er.what());
    }
}   // load

// ----------------------------------------------------------------------------
void GrandPrixManager::reload()
{
    for(unsigned int i=0; i<m_gp_data.size(); i++)
        delete m_gp_data[i];
    m_gp_data.clear();

    loadFiles();
}

// ----------------------------------------------------------------------------
std::string GrandPrixManager::generateId()
{
    std::stringstream s;

    do
    {
        s.clear();
        s << "usr_gp_" << ((rand() % 90000000) + 10000000);
    } while (existsId(s.str()));

    return s.str();
}

// ----------------------------------------------------------------------------
bool GrandPrixManager::existsId(const std::string& id) const
{
    bool exists;

    exists = false;
    for (unsigned int i = 0; !exists && i < m_gp_data.size(); i++)
        exists = (m_gp_data[i]->getId() == id);

    return exists;
}

// ----------------------------------------------------------------------------
bool GrandPrixManager::existsName(const irr::core::stringw& name) const
{
    bool exists;

    exists = false;
    for (unsigned int i = 0; !exists && i < m_gp_data.size(); i++)
        exists = (m_gp_data[i]->getName() == name);

    return exists;
}

// ----------------------------------------------------------------------------
GrandPrixManager::GrandPrixManager()
{
    loadFiles();
}   // GrandPrixManager

// ----------------------------------------------------------------------------
GrandPrixManager::~GrandPrixManager()
{
    for(unsigned int i=0; i<m_gp_data.size(); i++)
    {
        delete m_gp_data[i];
    }   // for i

}   // ~GrandPrixManager

// ----------------------------------------------------------------------------
const GrandPrixData* GrandPrixManager::getGrandPrix(const std::string& s) const
{
    return editGrandPrix(s);
}   // getGrandPrix

// ----------------------------------------------------------------------------
GrandPrixData* GrandPrixManager::editGrandPrix(const std::string& s) const
{
    for(unsigned int i=0; i<m_gp_data.size(); i++)
        if(m_gp_data[i]->getId()==s) return m_gp_data[i];
    return NULL;
}

// ----------------------------------------------------------------------------
void GrandPrixManager::checkConsistency()
{
    for(unsigned int i=0; i<m_gp_data.size(); i++)
    {
        if(!m_gp_data[i]->checkConsistency())
        {
            // delete this GP, since a track is missing
            delete *(m_gp_data.erase(m_gp_data.begin()+i));
            i--;
        }
    }
}   // checkConsistency

// ----------------------------------------------------------------------------
GrandPrixData* GrandPrixManager::createNew(const irr::core::stringw& newName)
{
    if (existsName(newName))
        return NULL;

    std::string newID = generateId();

    GrandPrixData* gp = new GrandPrixData;
    gp->setId(newID);
    gp->setName(newName);
    gp->setFilename(file_manager->getGPDir() + newID + SUFFIX);
    gp->setEditable(true);
    gp->writeToFile();
    m_gp_data.push_back(gp);

    return gp;
}

// ----------------------------------------------------------------------------
GrandPrixData* GrandPrixManager::copy(const std::string& id,
    const irr::core::stringw& newName)
{
    if (existsName(newName))
        return NULL;

    std::string newID = generateId();

    GrandPrixData* gp = new GrandPrixData(*getGrandPrix(id));
    gp->setId(newID);
    gp->setName(newName);
    gp->setFilename(file_manager->getGPDir() + newID + SUFFIX);
    gp->setEditable(true);
    gp->writeToFile();
    m_gp_data.push_back(gp);

    return gp;
}

// ----------------------------------------------------------------------------
void GrandPrixManager::remove(const std::string& id)
{
    const GrandPrixData* gp = getGrandPrix(id);
    assert(gp != NULL);

    if (gp->isEditable())
    {
        file_manager->removeFile(gp->getFilename());
        reload();
    }
    else
    {
        Log::warn("GrandPrixManager", "Grand Prix '%s' cannot be removed\n", gp->getId().c_str());
    }
}
