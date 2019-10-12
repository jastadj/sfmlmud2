#ifndef CLASS_ZONE
#define CLASS_ZONE

#include <string>
#include <vector>
#include <SFML/System.hpp>
#include "sqlite3.h"
#include "direction.hpp"

struct Room
{
    int room_id;                // room number
    std::string zone;           // what zone room belongs to

    std::string name;           // room name, first line of room description
    std::string description;    // long room description

    // exits - number links to other room numbers
    std::vector<int> exits;
};

struct Zone
{
    std::string name;
    std::vector<Room*> rooms; // list of pointers to rooms belonging to this zone
};

class ZoneManager
{
private:
    static bool m_Initialized;

    ZoneManager(sqlite3 *db);
    ~ZoneManager();

    sqlite3 *m_DB;

    // save/load rooms in database
    bool _LoadRooms();              // only happens once - on init
    bool _SaveRooms();              // only happens once - on shutdown

    // room
    sf::Mutex m_RoomMutex;
    int m_NextAvailableRoomID;
    std::vector<Room> m_Rooms;

    // zone
    sf::Mutex m_ZoneMutex;
    std::vector<Zone> m_Zones;

public:

    // public zone functions
    std::vector<std::string> getZones();
    Zone *createZone(std::string zonename);
    bool zoneExists(std::string zonename);

    // public room functions
    Room *createRoom(std::string zonename, bool save_to_database = true);
    bool linkRooms(int room_a, int room_b, int dir_index);
    bool saveRoom(int room_id);
    bool roomExists(int room_id);
    std::vector<std::string> lookRoom(int room_id);
    std::vector<std::string> getExits(int room_id);
    int getRoomNumInDirection(int room_id, int dir_index);

    friend class Mud;
};
#endif // CLASS_ZONE
