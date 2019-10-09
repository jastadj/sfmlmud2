#ifndef CLASS_MUD
#define CLASS_MUD

#include <vector>
#include <SFML/Network.hpp>
#include "sqlite3.h"

#include "client.hpp"
#include "welcome.hpp"
#include "account.hpp"
#include "zone.hpp"

#define SERVER_PORT 1212
#define DB_FILE "mud.db"

class Mud
{
private:
    // singleton
    Mud();
    ~Mud();
    static Mud *m_Instance;

    // server i/o
    enum SERVER_STATE{SERVER_INIT, SERVER_RUNNING, SERVER_SHUTDOWN};
    int m_ServerState;
    unsigned short m_Port;
    sf::TcpListener m_Listener;
    sf::SocketSelector m_Selector;
    sf::Thread *m_SendAndReceiveThread;
    void sendAndRecieve();
    void updateSelector();
    std::vector<Client*> m_Clients;
    sf::Mutex m_ClientMutex;
    bool addClient(Client *tclient);
    bool removeClient(Client *tclient);

    // sqlite database
    sqlite3 *m_DB;


public:
    // get singleton
    static Mud *getInstance()
    {
        if(!m_Instance) m_Instance = new Mud();
        return m_Instance;
    }

    void start();

    static int mainGame(Client *tclient);

    // database managers
    AccountManager *m_AccountManager;
    ZoneManager *m_ZoneManager;
};
#endif // CLASS_MUD
