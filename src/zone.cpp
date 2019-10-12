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
        ss << "description INTEGER,";
        for(int i = 0; i < DIR_COUNT; i++)
        {
            ss << "exit_" << dirs[i][0];
            if( i < DIR_COUNT-1 ) ss << ",";
        }
        ss << ");";

        if(sqlite3_exec(m_DB, ss.str().c_str(), sqlcallback, NULL, &errormsg) != SQLITE_OK)
        {
            std::cout << "Error creating rooms table:" << errormsg << std::endl;
            sqlite3_free(errormsg);
            return;
        }

        // create test zone and rooms
        if(!createZone("testzone")) std::cout << "ERROR CREATING TEST ZONE!\n";

        // create room 1
        {
            Room *test_room = createRoom("testzone");
            if(!test_room) std::cout << "ERROR CREATING TEST ROOM!\n";
            test_room->name = "Main room of Cabin";
            test_room->description = "This cabin has long been abandoned.  The floor is covered in a thick layer of dust.  Cobwebs have taken up all corners of the room.  A fireplace is built into the southern wall.";
            if(!saveRoom(test_room->room_id)) std::cout << "ERROR SAVING TEST ROOM!\n";
        }
        // create room 2
        {
            Room *test_room = createRoom("testzone");
            if(!test_room) std::cout << "ERROR CREATING TEST ROOM!\n";
            test_room->name = "Cabin Storage Room";
            test_room->description = "This is a small cramped storage room.  Sheleves are lined against the wall containg various odds and ends.";
            if(!saveRoom(test_room->room_id)) std::cout << "ERROR SAVING TEST ROOM!\n";
            linkRooms(1, 2, getDirectionIndex("west"));
        }


    }
    // if room database already exists, load all rooms from db into memory
    else
    {
        std::cout << "Loading rooms from database...\n";
        if(!_LoadRooms()) std::cout << "Error, failed to load rooms from database!\n";
    }
}

bool ZoneManager::_LoadRooms()
{
    std::stringstream ss;
    sqlite3_stmt *stmt;
    int error_count = 0;

    // load all rooms from database
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
        Room *troom = createRoom(zone, false);
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
            for(int n = 0; n < DIR_COUNT; n++) troom->exits.push_back(0);
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

bool ZoneManager::linkRooms(int room_a, int room_b, int dir_index)
{
    int room_b_dir = -1;
    std::stringstream link_error_ss;
    link_error_ss << "Error linking rooms " << room_a << "," << room_b << ": ";

    // valid direction?
    if(dir_index < 0 || dir_index >= DIR_COUNT)
    {
        std::cout << link_error_ss.str() << "direction index " << dir_index << " is not a valid direction!\n";
        return false;
    }

    // get opposite room direction index
    room_b_dir = getDirectionIndex(dirs[dir_index][1]);
    if(room_b_dir == -1)
    {
        std::cout << link_error_ss.str() << "unable to determine opposite direction for dir_index " << dir_index << std::endl;
        return false;
    }

    // rooms exist?
    if(!roomExists(room_a) || !roomExists(room_b))
    {
        std::cout << link_error_ss.str() << "one of these rooms does not exist!\n";
        return false;
    }

    // check bi-directional availability
    if(m_Rooms[room_a].exits[dir_index] || m_Rooms[room_b].exits[room_b_dir])
    {
        std::cout << link_error_ss.str() << "one of these rooms is already linked!\n";
        return false;
    }

    // link rooms
    m_Rooms[room_a].exits[dir_index] = room_b;
    m_Rooms[room_b].exits[room_b_dir] = room_a;
    return true;
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
        ss << "description = '" << troom->description << "',";
        for(int i = 0; i < DIR_COUNT; i++)
        {
            ss << "exit_" << dirs[i][0] << " = " << troom->exits[i];
            if(i < DIR_COUNT - 1) ss << ",";
            else ss << " ";
        }
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
        ss << "description,";
        for(int i = 0; i < DIR_COUNT; i++)
        {
            ss << "exit_" << dirs[i][0];
            if(i < DIR_COUNT-1) ss << ",";
        }
        ss << ") ";
        ss << "VALUES(";
        ss << "'" << troom->zone << "',";
        ss << "'" << troom->name << "',";
        ss << "'" << troom->description << "',";
        for(int i = 0; i < DIR_COUNT; i++)
        {
            ss << troom->exits[i];
            if(i < DIR_COUNT-1) ss << ",";
        }
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

bool ZoneManager::roomExists(int room_id)
{
    return (room_id > 0 && room_id < int(m_Rooms.size()) );
}

std::vector<std::string> ZoneManager::lookRoom(int room_id)
{
    std::vector<std::string> room_look;
    std::vector<std::string> exits = getExits(room_id);
    if(room_id < 1 || room_id >= int(m_Rooms.size()) ) return room_look;

    Room *troom = &m_Rooms[room_id];

    room_look.push_back(troom->name);
    room_look.push_back(troom->description);

    // get exits
    {
        std::stringstream ss;
        ss << "[ ";

        if(!exits.empty())
        {
            for(int i = 0; i < int(exits.size()); i++)
            {
                ss << exits[i];
                if(i != int(exits.size()-1) ) ss << " - ";
            }
        }
        else ss << "You see no obvious exits";
        ss << " ]";
        room_look.push_back(ss.str());
    }


    return room_look;
}

std::vector<std::string> ZoneManager::getExits(int room_id)
{
    std::vector<std::string> exits;
    if(!roomExists(room_id)) return exits;

    Room *troom = &m_Rooms[room_id];

    for(int i = 0; i < DIR_COUNT; i++)
    {
        if(troom->exits[i] != 0) exits.push_back(dirs[i][0]);
    }

    return exits;
}

int ZoneManager::getRoomNumInDirection(int room_id, int dir_index)
{
    if(dir_index < 0 || dir_index >= DIR_COUNT) return 0;
    if(room_id < 1 || room_id >= int(m_Rooms.size()) ) return 0;
    return m_Rooms[room_id].exits[dir_index];

}
