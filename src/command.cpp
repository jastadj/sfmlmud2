#include "command.hpp"

#include <iostream>
#include <sstream>
#include "mud.hpp"
#include "tools.hpp"


bool CommandManager::m_Initialized = false;

CommandManager::CommandManager()
{
    if(m_Initialized)
    {
        std::cout << "Commands already initialized!\n";
        return;
    }
    m_Initialized = true;

    // iniitalize all commands and aliases
    addNewCommand("quit", "disconnect from server", commandQuit);
    addNewCommand("look", "look around or at object", commandLook);
    addAlias("l", "look");
    addNewCommand("help", "show command help", commandHelp);
    // add directions
    for(int i = 0; i < DIR_COUNT; i++)
    {
        addNewCommand(dirs[i][0], "move " + dirs[i][0], commandMoveDirection);
        addAlias(dirs[i][4], dirs[i][0]);
    }

    std::cout << m_Commands.size() << " commands and " << m_Aliases.size() << " aliases initialized.\n";
}

CommandManager::~CommandManager()
{

}

bool CommandManager::addNewCommand(std::string cmd, std::string help, int (*func)(Client *tclient, std::string cmd, std::string args))
{
    if(cmd.empty() || func == NULL) {std::cout << "Error creating command " << cmd << ": cmd str empty or func is null\n"; return false; }
    if(help.empty()) help = "no_help";

    // check/format cmd string
    cmd = toLower(cmd);
    if(hasSpaces(cmd)) {std::cout << "Error creating command " << cmd << ": cmd str has spaces\n"; return false; }

    // check that command doesn't already exist
    if(isCommand(cmd)) {std::cout << "Error creating command " << cmd << ": cmd/alias already exists\n"; return false; }

    // create new command
    Command *newcommand = new Command;
    newcommand->cmd = cmd;
    newcommand->help = help;
    newcommand->func = func;
    m_Commands.push_back(newcommand);

    return true;
}

bool CommandManager::addAlias(std::string alias, std::string cmd, std::string args)
{
    std::string alias_error = "Error creating alias '" + alias + "', "; // help string
    Command *tcmd = NULL;

    // check/format alias and command string
    alias = toLower(alias);
    if(hasSpaces(alias)) {std::cout << alias_error << "has spaces\n"; return false; }
    cmd = toLower(cmd);

    // check that alias isn't already a command / alias
    if(isCommand(alias)) {std::cout << alias_error << "is already command/alias '" << cmd << "'\n"; return false;}

    // find command
    for(int i = 0; i < int(m_Commands.size()); i++)
    {
        if( m_Commands[i]->cmd == cmd)
        {
            tcmd = m_Commands[i];
            break;
        }
    }
    if(!tcmd) { std::cout << alias_error << "unable to find cmd '" << cmd << "'\n"; return false;}

    // create new alias
    Alias *newalias = new Alias;
    newalias->alias = alias;
    newalias->cmd = tcmd;
    newalias->args = args;
    m_Aliases.push_back(newalias);

    return true;
}

bool CommandManager::addCommandToCommandList(std::string cmd, CommandList *cmdlist)
{
    if(!cmdlist || cmd.empty()) return false;
    cmd = toLower(cmd);
    Command *tcmd = NULL;

    // find command
    for(int i = 0; int(m_Commands.size()); i++)
    {
        if(m_Commands[i]->cmd == cmd)
        {
            tcmd = m_Commands[i];
            break;
        }
    }
    if(!tcmd) return false;

    // find aliases
    for(int i = 0; i < int(m_Aliases.size()); i++)
    {
        if(m_Aliases[i]->cmd == tcmd)
        {
            cmdlist->m_Aliases.push_back(m_Aliases[i]);
        }
    }

    cmdlist->m_Commands.push_back(tcmd);
    return true;
}

bool CommandManager::showHelp(Client *tclient, CommandList *cmdlist, std::string str)
{
    if(!tclient) return false;

    std::stringstream ss;

    // if general help
    if(str.empty())
    {
        for(int i = 0; i < int(cmdlist->m_Commands.size()); i++)
        {
            ss << cmdlist->m_Commands[i]->cmd << " - " << cmdlist->m_Commands[i]->help << std::endl;
        }
        tclient->send(ss.str());
        return true;
    }
    // specific help on a command, show long help (for now show short help until implemented)
    else
    {
        for(int i = 0; i < int(cmdlist->m_Commands.size()); i++)
        {
            if(cmdlist->m_Commands[i]->cmd == toLower(str))
            {
                ss << cmdlist->m_Commands[i]->cmd << " - " << cmdlist->m_Commands[i]->help << std::endl;
                tclient->send(ss.str());
                return true;
            }
        }
    }

    return false;
}

bool CommandManager::isCommand(std::string cmd)
{
    if(cmd.empty()) return false;
    cmd = toLower(cmd);

    // check if command
    for(int i = 0; i < int(m_Commands.size()); i++)
    {
        if(m_Commands[i]->cmd == cmd) return true;
    }
    // check if alias
    for(int i = 0; i < int(m_Aliases.size()); i++)
    {
        if(m_Aliases[i]->alias == cmd) return true;
    }

    return false;
}

bool CommandManager::parseCommand(Client *tclient, CommandList *tlist, std::string str)
{
    if(!tclient || !tlist || str.empty()) return false;
    std::vector<std::string> words = csvParse(str, ' ');
    std::string cmd = toLower(words[0]);
    Command *command_found = NULL;
    if( int(words.size()) == 1) str.erase(0, cmd.size());
    else str.erase(0, cmd.size()+1);

    // check that command is in client's command list
    for(int i = 0; i < int(tlist->m_Commands.size()); i++)
    {
        if(tlist->m_Commands[i]->cmd == cmd)
        {
            command_found = tlist->m_Commands[i];
            break;
        }
    }
    // or is an alias
    for(int i = 0; i < int(tlist->m_Aliases.size()); i++)
    {
        if(tlist->m_Aliases[i]->alias == cmd)
        {
            command_found = tlist->m_Aliases[i]->cmd;
            if(!str.empty()) str = m_Aliases[i]->args + " " + str;
            break;
        }
    }
    if(!command_found)
    {
        tclient->send("Huh?\n");
        return false;
    }

    // execute command
    command_found->func(tclient, cmd, str);

    return true;
}

int CommandManager::commandQuit(Client *tclient, std::string cmd, std::string args)
{
    tclient->send("Goodbye!\n");
    tclient->disconnect();
    return 0;
}

int CommandManager::commandLook(Client *tclient, std::string cmd, std::string args)
{
    // if no arguments, do room look
    if(args.empty())
    {
        std::stringstream ss;
        std::vector<std::string> room_desc = Mud::getInstance()->m_ZoneManager->lookRoom(tclient->getRoom());
        for(int i = 0; i < int(room_desc.size()); i++)
        {
            ss << room_desc[i] << "\n";
        }
        tclient->send(ss.str());

    }
    return 0;
}

int CommandManager::commandHelp(Client *tclient, std::string cmd, std::string args)
{
    tclient->showHelp(args);
    return 0;
}

int CommandManager::commandMoveDirection(Client *tclient, std::string cmd, std::string args)
{
    int dir_index = getDirectionIndex(cmd);
    int t_room_id = 0;

    // movement arguments not implemented
    if(!args.empty())
    {
        tclient->send("Unknown movement arguments [not implemented]\n");
        return 0;
    }

    // no valid direction found
    if(dir_index == -1)
    {
        tclient->send("That is not a valid direction!\n");
        return 0;
    }

    // get room id in direction
    t_room_id = Mud::getInstance()->m_ZoneManager->getRoomNumInDirection(tclient->getRoom(), dir_index);
    if(!t_room_id)
    {
        tclient->send("You see no exit " + dirs[dir_index][0] + ".\n");
        return 0;
    }

    // there is a room in that direction, set room and do a room look
    tclient->setRoom(t_room_id);
    tclient->parseCommand("look");

    return 0;
}

////////////////////////////////////////////////////////////////
// COMMAND LIST
CommandList::CommandList()
{

}

CommandList::~CommandList()
{

}

bool CommandList::hasCommand(std::string cmd)
{
    if(cmd.empty()) return false;

    // check commands
    for(int i = 0; i < int(m_Commands.size()); i++)
    {
        if(m_Commands[i]->cmd == cmd) return true;
    }

    // check aliases
    for(int i = 0; i < int(m_Aliases.size()); i++)
    {
        if(m_Aliases[i]->alias == cmd + m_Aliases[i]->args) return true;
    }

    return false;
}
