#include "zone.hpp"

#include <iostream>
#include <sstream>
#include "tools.hpp"

bool ZoneManager::m_Initialized = false;

ZoneManager::ZoneManager(sqlite3 *db)
{
    if(m_Initialized)
    {
        std::cout << "Zone manager has already been initialized!\n";
        return;
    }
    m_Initialized = true;

    // database reference
    m_DB = db;

    // create new table if new
    if(!tableExists(m_DB, "rooms"))
    {
        std::cout << "Creating new rooms table in database...\n";
        std::stringstream ss;
        char *errormsg = 0;
        ss << "CREATE TABLE rooms( ";
        ss << "room_id INTEGER PRIMARY KEY,";
        ss << "zone TEXT NOT NULL,";
        ss << "name TEXT NOT NULL,";
        ss << "description INTEGER";
        ss << ");";

        if(sqlite3_exec(m_DB, ss.str().c_str(), sqlcallback, NULL, &errormsg) != SQLITE_OK)
        {
            std::cout << "Error creating rooms table:" << errormsg << std::endl;
            sqlite3_free(errormsg);
            return;
        }


        // create test zone and rooms
        if(!createZone("testzone")) std::cout << "ERROR CREATING TEST ZONE!\n";
        Room *test_room = createRoom("testzone");
        if(!test_room) std::cout << "ERROR CREATING TEST ROOM!\n";
        test_room->name = "Test Room";
        test_room->description = "This is a test room.";

    }
    // if room database already exists, load all rooms
    else
    {

    }
}

std::vector<std::string> ZoneManager::getZones()
{
    std::vector<std::string> zones;

    m_ZoneMutex.lock();
    for(int i = 0; i < int(m_Zones.size()); i++)
    {
        zones.push_back(m_Zones[i].name);
    }
    m_ZoneMutex.unlock();

    return zones;
}

Zone *ZoneManager::createZone(std::string zonename)
{
    Zone *tzone = NULL;

    if(zonename.empty()) return NULL;
    if(hasSpaces(zonename)) return NULL;

    // check that zone doesn't exist
    m_ZoneMutex.lock();
    for(int i = 0; i < int(m_Zones.size()); i++)
    {
        if(m_Zones[i].name == zonename)
        {
            m_ZoneMutex.unlock();
            return NULL;
        }
    }

    m_Zones.push_back(Zone());
    tzone = &m_Zones.back();
    tzone->name = zonename;
    m_ZoneMutex.unlock();

    std::cout << "Created zone " << tzone->name << std::endl;

    return tzone;
}

bool ZoneManager::zoneExists(std::string zonename)
{
    bool exists = false;

    m_ZoneMutex.lock();
    for(int i = 0; i < int(m_Zones.size()); i++)
    {
        if(m_Zones[i].name == zonename)
        {
            exists = true;
            break;
        }
    }
    m_ZoneMutex.unlock();

    return exists;
}

Room *ZoneManager::createRoom(std::string zonename)
{
    Room *troom = NULL;

    if(!zoneExists(zonename)) return NULL;

    // create new room with next available room id
    // put reference in zone
    // return room
    m_RoomMutex.lock();
    m_Rooms.push_back(Room());
    troom = &m_Rooms.back();
    troom->room_id = m_NextAvailableRoomID;
    m_NextAvailableRoomID++;
    troom->name = "no_name";
    troom->description = "no_description";

    m_ZoneMutex.lock();
    for(int i = 0; i < int(m_Zones.size()); i++)
    {
        if(m_Zones[i].name == zonename)
        {
            troom->zone = zonename;
            m_Zones[i].rooms.push_back(troom);
            break;
        }
    }
    m_ZoneMutex.unlock();
    m_RoomMutex.unlock();

    if(troom->zone.empty())
    {
        std::cout << "ERROR creating room.  Target zone room not found after passing zone check!!\n";
    }

    return troom;
}
