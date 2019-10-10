#ifndef CLASS_CLIENT
#define CLASS_CLIENT

#include <SFML/Network.hpp>
#include "command.hpp"

#define CLIENT_RECEIVE_SIZE 100

// forward dec
class CommandList;

class Client
{
private:

    sf::TcpSocket *m_Socket;
    bool m_Connected;

    std::string m_Username;
    int m_CurrentRoom;

    CommandList m_CommandList;

public:
    Client(sf::TcpSocket *tsocket);
    ~Client();

    std::string getName() { return m_Username;}
    int getRoom() { return m_CurrentRoom;}
    bool setRoom(int room_id);

    // client data storage
    std::string m_LastInput;                // last recieved input
    std::vector<int> m_IntRegisters;        // storage utility
    std::vector<std::string> m_StrRegisters;// storage utility
    void clearStorage();                    // zero out storage utlities

    // connection status
    bool isConnected() { return m_Connected;}
    void disconnect();

    // socket hand shaking
    bool addToSelector(sf::SocketSelector *tselector);
    bool isReady(sf::SocketSelector *tselector);

    // receive data from the client, results are stored in m_LastInput
    bool receive();
    bool parseCommand(std::string str);

    // send data to the client
    bool send(std::string str);
    bool sendPrompt();

    // client function pointer (give client feedback context with the function pointer)
    int (*func)(Client *tclient);

    friend class AccountManager;

};
#endif // CLASS_CLIENT
