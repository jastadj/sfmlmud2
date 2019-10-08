#include "mud.hpp"

#include <iostream>

Mud *Mud::m_Instance = NULL;

Mud::Mud()
{

}

Mud::~Mud()
{
    // close database connection
    sqlite3_close(m_DB);
}

void Mud::start()
{
    static bool started = false;
    if(started) return;
    started = true;
    std::cout << "Starting mud...\n";
    m_ServerState = SERVER_INIT;

    // open connection to sqlite database
    if(sqlite3_open(std::string(DB_FILE).c_str(), &m_DB))
    {
        std::cout << "Error opening DB:" << DB_FILE << ", exiting...\n";
        return;
    }

    // initialize account manager
    std::cout << "Initializing account manager...\n";
    m_AccountManager = new AccountManager(m_DB);

    // start send and receive thread
    m_SendAndReceiveThread = new sf::Thread(Mud::sendAndRecieve, this);
    m_SendAndReceiveThread->launch();

    // wait for server shutdown
    while(m_ServerState != SERVER_SHUTDOWN);

    std::cout << "Shutting down...\n";
}

void Mud::sendAndRecieve()
{
    std::cout << "Initializing server...\n";

    // init server
    m_Port = SERVER_PORT;
    m_Listener.listen(m_Port);
    updateSelector();

    // list of clients ready to be removed in the event of error/disconnect
    std::vector<Client*> m_ClientRemovalQueue;

    // send and receive data until server shutdown
    while(m_ServerState != SERVER_SHUTDOWN)
    {
        // wait for data
        if(m_Selector.wait())
        {
            // incoming connection?
            if(m_Selector.isReady(m_Listener))
            {
                // create and accept new connection
                sf::TcpSocket *newsocket = new sf::TcpSocket;
                m_Listener.accept(*newsocket);

                // create client from accepted connection and add to client list
                Client *newclient = new Client(newsocket);
                if(!addClient(newclient))
                {
                    std::cout << "Error adding new client!\n";
                    delete newclient;
                }
                else std::cout << "Accepted new client.\n";

                // set initial client context (function pointer)
                // show welcome screen
                newclient->func = welcome;
                newclient->func(newclient);
                // show login screen
                newclient->func = AccountManager::loginProcess;
                newclient->func(newclient);

            }
            // check for clients that are ready to send
            else
            {
                m_ClientMutex.lock();
                for(int i = 0; i < int(m_Clients.size()); i++)
                {
                    // receive client data
                    if(m_Clients[i]->isReady(&m_Selector))
                    {
                        m_Clients[i]->receive();
                        // after receiving input from client, give feedback
                        m_Clients[i]->func(m_Clients[i]);
                        if(!m_Clients[i]->isConnected()) m_ClientRemovalQueue.push_back(m_Clients[i]);
                    }
                }
                m_ClientMutex.unlock();
            }

            // clean up any clients that need to be removed
            while(!m_ClientRemovalQueue.empty())
            {
                Client *tclient = m_ClientRemovalQueue.back();
                m_ClientRemovalQueue.pop_back();
                removeClient(tclient);
            }

        }
    }
}

// adds a new client to be managed by server
bool Mud::addClient(Client *tclient)
{
    if(!tclient) return false;
    m_ClientMutex.lock();
    // make sure client isn't already in the list
    for(int i = 0; i < int(m_Clients.size()); i++)
    {
        // if somehow the client was already in the list, remove it
        // and fail
        if(m_Clients[i] == tclient)
        {
            std::cout << "Error adding client, already in clients list!!\n";
            m_Clients.erase(m_Clients.begin() + i);
            m_ClientMutex.unlock();
            return false;
        }
    }
    // add new client to the list
    m_Clients.push_back(tclient);
    m_ClientMutex.unlock();
    // update selector
    updateSelector();
    return true;
}

bool Mud::removeClient(Client *tclient)
{
    bool client_found = false;
    if(!tclient) return false;
    // find target client to be removed
    m_ClientMutex.lock();
    for(int i = 0; i < int(m_Clients.size()); i++)
    {
        // remove client from list
        if(m_Clients[i] == tclient)
        {
            m_Clients.erase(m_Clients.begin() + i);
            client_found = true;
            break;
        }
    }
    // delete client
    if(client_found)
    {
        delete tclient;
        std::cout << "Client disconnected.\n";
    }
    else std::cout << "Error, unable to remove target client, not found!\n";
    m_ClientMutex.unlock();
    // update selector
    updateSelector();
    return client_found;
}

void Mud::updateSelector()
{
    m_Selector.clear();
    m_Selector.add(m_Listener);

    // add client sockets to selector
    m_ClientMutex.lock();
    for(int i = 0; i < int(m_Clients.size()); i++)
    {
        if(!m_Clients[i]->addToSelector(&m_Selector))
        {
            std::cout << "Error updating selector when adding client!!\n";
        }
    }
    m_ClientMutex.unlock();
}

int Mud::mainGame(Client *tclient)
{
    if(!tclient) return 0;
    tclient->send("MAIN GAME");
}
