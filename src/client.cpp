#include "client.hpp"

#include <iostream> // debug

Client::Client(sf::TcpSocket *tsocket)
{
    m_Socket = tsocket;
    m_Connected = true;

    // init storage
    m_StrRegisters.resize(3);
    m_IntRegisters.resize(3);
    clearStorage();
}

Client::~Client()
{
    m_Socket->disconnect();
    delete m_Socket;
}

void Client::disconnect()
{
    m_Socket->disconnect();
    m_Connected = false;
}

void Client::clearStorage()
{
    for(int i = 0; i < int(m_StrRegisters.size()); i++) m_StrRegisters[i] = std::string();
    for(int i = 0; i < int(m_IntRegisters.size()); i++) m_IntRegisters[i] = 0;
}

bool Client::addToSelector(sf::SocketSelector *tselector)
{
    if(!tselector) return false;
    tselector->add(*m_Socket);
    return true;
}

bool Client::isReady(sf::SocketSelector *tselector)
{
    if(!tselector) return false;
    return tselector->isReady(*m_Socket);
}

bool Client::receive()
{
    char data[CLIENT_RECEIVE_SIZE];
    size_t received = 0;
    if(m_Socket->receive(data, CLIENT_RECEIVE_SIZE, received) == sf::Socket::Status::Disconnected) disconnect();
    // terminate received data
    data[received-2] = '\0';
    // store received data as last input
    m_LastInput = std::string(data);

    return m_Connected;
}

bool Client::send(std::string str)
{
    if(m_Socket->send(str.c_str(), str.size()) == sf::Socket::Status::Disconnected) disconnect();
    return m_Connected;
}
