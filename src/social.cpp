#include "social.hpp"
#include <sstream>
#include "mud.hpp"

int say(Client *tclient, std::string cmd, std::string args)
{
    if(!tclient) return 0;
    std::stringstream rss;

    if(args.empty())
    {
        tclient->send("Say what?\n");
        return 0;
    }

    tclient->send("You say \"" + args + "\"\n");

    rss << tclient->getName() << " says \"" << args << "\"";
    Mud::getInstance()->broadcastToRoomExcluding(tclient->getRoom(), rss.str(), tclient);
    return 0;
}
