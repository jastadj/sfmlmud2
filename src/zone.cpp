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
    m_NextAvailableRoomID = 1;

    // database reference
    m_DB = db;

    // create buffer room as room 0 to account for rowid 0 being column names
    m_Rooms.push_back(Room());

    // create new table if new
    if(!tableExists(m_DB, "rooms"))
    {
        std::cout << "Creating new rooms table in database...\n";
        std::stringstream ss;
        char *errormsg = 0;
        // note : if this changes, update create, save, and load functions
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
        if(!saveRoom(test_room->room_id)) std::cout << "ERROR SAVING TEST ROOM!\n";


    }
    // if room database already exists, load all rooms from db into memory
    else
    {
        if(!_LoadRooms()) std::cout << "Error, failed to load rooms from database!\n";
    }
}

bool ZoneManager::_LoadRooms()
{
    std::stringstream ss;
    sqlite3_stmt *stmt;
    int error_count = 0;

    // load all rooms from database
    std::cout << "Loading rooms from database...\n";
    ss << "SELECT * FROM rooms;";
    // compile sql statement to binary
    int rc = sqlite3_prepare_v2(m_DB, ss.str().c_str(), -1, &stmt, NULL);
    if( rc != SQLITE_OK)
    {
        std::cout << "Error in sql compile during load rooms:" << sqlite3_errmsg(m_DB) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    // execute sql statement
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        bool failed = false;

        // get zone name of entry
        std::string zone = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt,1)) );
        // if zone doesn't currently exist, create it
        if(!zoneExists(zone)) createZone(zone);
        // create room
        Room *troom = createRoom(zone);
        // if room was successfully created, set room variables from database
        if(troom)
        {
            troom->name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt,2)) );
            troom->description = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt,3)) );
            // if for some reason newly created room is not synced with expected room id from database, squawk
            if( troom->room_id != sqlite3_column_int(stmt,0))
            {
                std::cout << "Loaded new room id does not match database room id!\n";
                failed = true;
            }
        }
        else
        {
            std::cout << "Failed to create new room on load.\n";
            failed = true;
        }

        if(failed) error_count++;
    }
    if(rc != SQLITE_DONE)
    {
        std::cout << "Error in sql during load rooms:" << sqlite3_errmsg(m_DB) << std::endl;
    }
    sqlite3_finalize(stmt);

    std::cout << m_Rooms.size()-1 << " rooms loaded with " << error_count << " errors.\n";
    return true;
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

    // add zone to list
    m_Zones.push_back(Zone());
    tzone = &m_Zones.back();
    tzone->name = zonename;
    m_ZoneMutex.unlock();

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

Room *ZoneManager::createRoom(std::string zonename, bool save_to_database)
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

    // add room to zone
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

    // if saving to database
    if(save_to_database) saveRoom(troom->room_id);

    m_ZoneMutex.unlock();
    m_RoomMutex.unlock();

    if(troom->zone.empty())
    {
        std::cout << "ERROR creating room.  Target zone room not found after passing zone check!!\n";
    }

    return troom;
}

bool ZoneManager::_SaveRooms()
{
    int error_count = 0;
    m_RoomMutex.lock();
    // save all rooms
    std::cout << "Saving all rooms...\n";
    for(int i = 0; i < int(m_Rooms.size()); i++)
    {
        if(!saveRoom(m_Rooms[i].room_id))
        {
            std::cout << "Error saving room id " << m_Rooms[i].room_id << std::endl;
            error_count++;
        }
    }
    std::cout << "Done saving all rooms with " << error_count << " errors.\n";
    m_RoomMutex.unlock();
    if(error_count) return false;
    return true;
}

bool ZoneManager::saveRoom(int room_id)
{

    if(room_id <= 0 || room_id > int(m_Rooms.size()) ) return false;

    Room *troom = &m_Rooms[room_id];
    bool exists_in_db = false;

    // check if room exists in database
    std::stringstream sss;
    sqlite3_stmt *stmt;
    sss << "SELECT * FROM rooms WHERE rowid = " << room_id << ";";
    // compile sql statement to binary
    int rc = sqlite3_prepare_v2(m_DB, sss.str().c_str(), -1, &stmt, NULL);
    if( rc != SQLITE_OK)
    {
        std::cout << "Error in sql compile during load rooms:" << sqlite3_errmsg(m_DB) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    // execute sql statement
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        exists_in_db = true;
    }
    if(rc != SQLITE_DONE) {std::cout << "Error in sql during load rooms:" << sqlite3_errmsg(m_DB) << std::endl;}
    sqlite3_finalize(stmt);

    // update database entry if it already exists in database
    if( exists_in_db)
    {
        std::stringstream ss;
        char *errormsg = 0;
        ss << "UPDATE ";
        ss << "rooms ";
        ss << "SET ";
        ss << "zone = '" << troom->zone << "',";
        ss << "name = '" << troom->name << "',";
        ss << "description = '" << troom->description << "' ";
        ss << "WHERE ";
        ss << "rowid = " << troom->room_id;
        ss << ";";
        if(sqlite3_exec(m_DB, ss.str().c_str(), sqlcallback, NULL, &errormsg) != SQLITE_OK)
        {
            std::cout << "Error saving/updating room in sql:" << errormsg << std::endl;
            sqlite3_free(errormsg);
            return false;
        }
        return true;
    }
    // else create new entry in database
    else
    {
        std::stringstream ss;
        char *errormsg = 0;
        ss << "INSERT INTO ";
        ss << "rooms(";
        ss << "zone,";
        ss << "name,";
        ss << "description";
        ss << ") ";
        ss << "VALUES(";
        ss << "'" << troom->zone << "',";
        ss << "'" << troom->name << "',";
        ss << "'" << troom->description << "'";
        ss << ");";
        if(sqlite3_exec(m_DB, ss.str().c_str(), sqlcallback, NULL, &errormsg) != SQLITE_OK)
        {
            std::cout << "Error inserting new room in sql:" << errormsg << std::endl;
            sqlite3_free(errormsg);
            return false;
        }
        return true;
    }

    return false;
}

std::vector<std::string> ZoneManager::lookRoom(int room_id)
{
    std::vector<std::string> room_look;
    if(room_id < 1 || room_id >= m_Rooms.size()) return room_look;

    Room *troom = &m_Rooms[room_id];

    room_look.push_back(troom->name);
    room_look.push_back(troom->description);

    return room_look;
}
