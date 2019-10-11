#ifndef CLASS_COMMAND
#define CLASS_COMMAND

#include <string>
#include <vector>
//#include "client.hpp"

// forward declaration
class Client;
class CommandList;

struct Command
{
    std::string cmd;
    std::string help;
    int (*func)(Client *tclient, std::string str);
};

class CommandManager
{
private:
    CommandManager();
    ~CommandManager();
    static bool m_Initialized;

    // all commands
    std::vector<Command*> m_Commands;
    bool addNewCommand(std::string cmd, std::string help, int (*func)(Client *tclient, std::string str));

public:

    bool isCommand(std::string cmd);

    // command list functions
    bool parseCommand(Client *tclient, CommandList *tlist, std::string str);
    bool addCommandToCommandList(std::string cmd, CommandList *cmdlist);
    bool showHelp(Client *tclient, CommandList *cmdlist, std::string str);

    // some general commands
    static int commandQuit(Client *tclient, std::string str);
    static int commandLook(Client *tclient, std::string str);
    static int commandHelp(Client *tclient, std::string str);

    friend class Mud;
};

class CommandList
{
private:

    std::vector<Command*> m_Commands;

public:
    CommandList();
    ~CommandList();

    bool hasCommand(std::string cmd);
    friend class CommandManager;
};

#endif // CLASS_COMMAND
