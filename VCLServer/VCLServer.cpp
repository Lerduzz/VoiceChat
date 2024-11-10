#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <list>
#include <map>
#include <thread>

typedef std::map<sf::Uint32, sf::TcpSocket*> ClientList;

sf::SocketSelector selector;
sf::TcpListener listener;
ClientList clients;

// static void receiveThread(sf::TcpSocket* client) {
//     std::cout << "* ReceiveThread(): Inicializando para: " << client->getRemoteAddress() << ":" << client->getRemotePort() << "." << std::endl;
//     while (true) {
//         sf::Packet* packet = new sf::Packet();
//         sf::Socket::Status st = client->receive(*packet);
//         if (st == sf::Socket::Disconnected || st == sf::Socket::Error)
//         {
//             clients.remove(client);
//             delete client;
//             break;
//         }
//         if (st == sf::Socket::NotReady)
//             continue;
//         std::cout << "* Audio recibido de: " << client->getRemoteAddress() << ":" << client->getRemotePort() << "." << std::endl;
//         for (ClientList::iterator it = clients.begin(); it != clients.end(); ++it) {
//             if ((*it)->getRemotePort() != client->getRemotePort()) {
//                 std::cout << "Enviando audio a: " << (*it)->getRemoteAddress() << ":" << (*it)->getRemotePort() << "." << std::endl;
//                 (*it)->send(*packet);
//             }
//         }
//         delete packet;
//     }
//     std::cout << "* ReceiveThread(): Cliente desconectado." << std::endl;
// }

int main() {
    // sf::IpAddress serverAddress("192.168.10.153");
    // sf::Uint16 port = 8000;
    // listener.listen(port, serverAddress);
    // 
    // std::cout << "* Aceptando conexiones de los clientes." << std::endl;
    // while (true) {
    //     sf::TcpSocket* client = new sf::TcpSocket();
    //     if (listener.accept(*client) == sf::Socket::Done) {
    //         clients.push_back(client);
    //         std::cout << "* Cliente aceptado: " << client->getRemoteAddress() << ":" << client->getRemotePort() << "." << std::endl;
    //         
    //         std::thread thread(&receiveThread, client);
    //         thread.detach();
    //     }
    // }

    sf::Uint32 tempAccID = 1;

    sf::IpAddress serverAddress("127.0.0.1");
    sf::Uint16 port = 8000;
    listener.listen(port, serverAddress);

    selector.add(listener);

    bool serverRunning = true;
    while (serverRunning)
    {
        // Clientes que deben ser desconectados.
        std::list<sf::Uint32> offlineList;

        // Make the selector wait for data on any socket
        if (selector.wait())
        {
            // Test the listener
            if (selector.isReady(listener))
            {
                sf::TcpSocket* client = new sf::TcpSocket();
                if (listener.accept(*client) == sf::Socket::Done)
                {
                    // Add the new client to the clients list
                    clients[tempAccID++] = client;

                    // Add the new client to the selector so that we will
                    // be notified when he sends something
                    selector.add(*client);
                }
                else
                {
                    // Error, we won't get a new connection, delete the socket
                    delete client;
                }
            }
            else
            {
                // The listener socket is not ready, test all other sockets (the clients)
                for (ClientList::iterator it = clients.begin(); it != clients.end(); ++it)
                {
                    sf::TcpSocket& client = *it->second;
                    if (selector.isReady(client))
                    {
                        // The client has sent some data, we can receive it
                        sf::Packet packet;
                        sf::Socket::Status receiveStatus = client.receive(packet);
                        if (receiveStatus == sf::Socket::Done)
                        {
                            for (ClientList::iterator itS = clients.begin(); itS != clients.end(); ++itS)
                            {
                                if (it->first != itS->first)
                                {
                                    sf::Socket::Status sendStatus = itS->second->send(packet);
                                    if (sendStatus == sf::Socket::Disconnected)
                                        offlineList.push_back(itS->first);
                                }
                            }
                        }
                        else
                        {
                            if (receiveStatus == sf::Socket::Disconnected)
                                offlineList.push_back(it->first);
                        }
                    }
                }
            }
        }
        offlineList.unique();
        while (!offlineList.empty())
        {
            if (clients.find(offlineList.front()) != clients.end())
            {
                sf::TcpSocket* client = clients[offlineList.front()];
                selector.remove(*client);
                clients.erase(offlineList.front());
                delete client;
            }
            offlineList.pop_front();
        }
    }

    return 0;
}
