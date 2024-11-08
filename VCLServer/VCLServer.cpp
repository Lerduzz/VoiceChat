#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <list>
#include <thread>

typedef std::list<sf::TcpSocket*> ClientList;

sf::TcpListener listener;
ClientList clients;

static void receiveThread(sf::TcpSocket* client) {
    std::cout << "* ReceiveThread(): Inicializando para: " << client->getRemoteAddress() << ":" << client->getRemotePort() << "." << std::endl;
    while (true) {
        sf::Packet* packet = new sf::Packet();
        sf::Socket::Status st = client->receive(*packet);
        if (st == sf::Socket::Disconnected || st == sf::Socket::Error)
        {
            clients.remove(client);
            delete client;
            break;
        }
        if (st == sf::Socket::NotReady)
            continue;
        std::cout << "* Audio recibido de: " << client->getRemoteAddress() << ":" << client->getRemotePort() << "." << std::endl;
        for (ClientList::iterator it = clients.begin(); it != clients.end(); ++it) {
            if ((*it)->getRemotePort() != client->getRemotePort()) {
                std::cout << "Enviando audio a: " << (*it)->getRemoteAddress() << ":" << (*it)->getRemotePort() << "." << std::endl;
                (*it)->send(*packet);
            }
        }
        delete packet;
    }
    std::cout << "* ReceiveThread(): Cliente desconectado." << std::endl;
}

int main() {
    sf::IpAddress serverAddress("192.168.10.153");
    sf::Uint16 port = 8000;
    listener.listen(port, serverAddress);

    std::cout << "* Aceptando conexiones de los clientes." << std::endl;
    while (true) {
        sf::TcpSocket* client = new sf::TcpSocket();
        if (listener.accept(*client) == sf::Socket::Done) {
            clients.push_back(client);
            std::cout << "* Cliente aceptado: " << client->getRemoteAddress() << ":" << client->getRemotePort() << "." << std::endl;
            
            std::thread thread(&receiveThread, client);
            thread.detach();
        }
    }

    return 0;
}
