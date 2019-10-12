#include "direction.hpp"

#include "tools.hpp"

std::vector<std::string> getDirections()
{
    std::vector<std::string> dlist;

    for(int i = 0; i < DIR_COUNT; i++)
    {
        dlist.push_back(dirs[i][0]);
    }
    return dlist;
}

int getDirectionIndex(std::string dir)
{
    for(int i = 0; i < DIR_COUNT; i++)
    {
        if(dirs[i][0] == dir || dirs[i][4] == dir)
        {
            return i;
        }
    }
    return -1;
}

std::string oppositeDir(std::string dir)
{
    std::string dir_error = "dir_error";

    dir = capitalize(toLower(dir));

    // find opposite direction
    for(int i = 0; i < DIR_COUNT; i++)
    {
        if( dirs[i][0] == dir)
        {
            return dirs[i][1];
        }
    }

    // did not find valid direction
    return dir_error;
}
