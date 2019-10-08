#ifndef CLASS_WELCOME
#define CLASS_WELCOME

#include <iostream>
#include <string>
#include <fstream>

#include "client.hpp"

#define WELCOME_FILE "welcome.txt"

static int welcome(Client *tclient)
{
    if(!tclient) return 0;
    static bool loaded = false;
    static std::string m_WelcomeScreen;

    // read in welcome screen from text file
    if(!loaded)
    {
        std::cout << "Loading welcome file:" << WELCOME_FILE << std::endl;
        std::ifstream ifile;
        ifile.open(WELCOME_FILE);
        if(ifile.is_open())
        {
            while(!ifile.eof())
            {
                std::string buf;
                std::getline(ifile, buf);
                m_WelcomeScreen += buf + "\n";
            }
            ifile.close();
        }
        else std::cout << "Error opening welcome file:" << WELCOME_FILE << std::endl;
        loaded = true;
    }

    tclient->send(m_WelcomeScreen);
    return 0;
}

#endif // CLASS_WELCOME
