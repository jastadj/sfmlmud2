#include "command.hpp"

#include <iostream>
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

    // iniitalize all commands
    addNewCommand("quit", "disconnect from server", commandQuit );

    std::cout << m_Commands.size() << " commands initialized.\n";
}

CommandManager::~CommandManager()
{

}

bool CommandManager::addNewCommand(std::string cmd, std::string help, int (*func)(Client *tclient, std::string str))
{
    if(cmd.empty() || func == NULL) {std::cout << "Error creating command " << cmd << ": cmd str empty or func is null\n"; return false; }
    if(help.empty()) help = "no_help";

    cmd = toLower(cmd);
    if(hasSpaces(cmd)) {std::cout << "Error creating command " << cmd << ": cmd str has spaces\n"; return false; }

    // check that command doesn't already exist
    if(isCommand(cmd)) {std::cout << "Error creating command " << cmd << ": cmd already exists\n"; return false; }

    Command *newcommand = new Command;
    newcommand->cmd = cmd;
    newcommand->help = help;
    newcommand->func = func;
    m_Commands.push_back(newcommand);

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

    cmdlist->m_Commands.push_back(tcmd);
    return true;
}

bool CommandManager::isCommand(std::string cmd)
{
    if(cmd.empty()) return false;
    cmd = toLower(cmd);

    for(int i = 0; i < int(m_Commands.size()); i++)
    {
        if(m_Commands[i]->cmd == cmd) return true;
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

    //std::cout << "command:'" << cmd << "'\nargs:'" << str << "'\n";

    // check that command is in client's command list
    for(int i = 0; i < int(tlist->m_Commands.size()); i++)
    {
        if(tlist->m_Commands[i]->cmd == cmd)
        {
            command_found = m_Commands[i];
            break;
        }
    }
    if(!command_found)
    {
        tclient->send("Huh?\n");
        return false;
    }

    // execute command
    command_found->func(tclient, str);

    return true;
}

int CommandManager::commandQuit(Client *tclient, std::string str)
{
    tclient->disconnect();
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

    for(int i = 0; i < int(m_Commands.size()); i++)
    {
        if(m_Commands[i]->cmd == cmd) return true;
    }

    return false;
}