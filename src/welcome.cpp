#include "welcome.hpp"

int welcome(Client *tclient)
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
