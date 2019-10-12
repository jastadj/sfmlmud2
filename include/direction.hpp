#ifndef CLASS_DIRECTION
#define CLASS_DIRECTION

#include <string>
#include <vector>

// if adding or removing directions, update dir count
#define DIR_COUNT 4
static std::string dirs[DIR_COUNT][5] =
    {
        {"north", "south", "left to the north", "entered from the north", "n"},
        {"south", "north", "left to the south", "entered from the south", "s"},
        {"east", "west", "left to the east", "entered from the east", "e"},
        {"west", "east", "left to the west", "entered form the west", "w"}
    };


std::vector<std::string> getDirections();
std::string oppositeDirection(std::string dir);

#endif // CLASS_DIRECTION
