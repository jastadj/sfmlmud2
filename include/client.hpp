#ifndef CLASS_CLIENT
#define CLASS_CLIENT

#include <SFML/Network.hpp>

#define CLIENT_RECEIVE_SIZE 100

class Client
{
private:

    sf::TcpSocket *m_Socket;
    bool m_Connected;

public:
    Client(sf::TcpSocket *tsocket);
    ~Client();

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

    // send data to the client
    bool send(std::string str);

};
#endif // CLASS_CLIENT
