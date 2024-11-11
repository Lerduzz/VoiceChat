#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <thread>
#include "conio.h"

enum PacketOpcode
{
    CMSG_VCL_PING,
    CMSG_VCL_AUTH,
    CMSG_VCL_AUDIO,
    SMSG_VCL_AUTH_OK,
    SMSG_VCL_AUTH_ERROR_NOT_EXISTS,
    SMSG_VCL_AUTH_ERROR_WRONG_PASS,
    SMSG_VCL_AUDIO
};

sf::IpAddress serverAddress("sking.lerduzz.com");
sf::Uint16 port = 8000;

bool running = true;
bool clientConnected = false;
bool clientLoggedIn = false;

std::string savedUsername = "";
std::string savedPassword = "";

static void playSoundThread(sf::SoundBuffer* buffer) {
    sf::Sound* sound = new sf::Sound(*buffer);
    sound->play();
    while (sound->getStatus() == sf::Sound::Playing)
        sf::sleep(sf::milliseconds(100));
    delete sound;
    delete buffer;
}

static void receiveThread(sf::TcpSocket* client) {
    while (clientConnected && clientLoggedIn) {
        sf::Packet packet;
        sf::Socket::Status st = client->receive(packet);
        if (st == sf::Socket::Done)
        {
            sf::Uint32 opcode;
            packet >> opcode;
            switch (opcode)
            {
                case SMSG_VCL_AUDIO:
                {
                    std::cout << "* ReceiveThread(): Se ha recibido audio." << std::endl;
                    sf::Uint64 sampleCount;
                    unsigned int channelCount;
                    unsigned int sampleRate;
                    packet >> sampleCount >> channelCount >> sampleRate;
                    sf::SoundBuffer* buffer = new sf::SoundBuffer();
                    buffer->loadFromSamples(reinterpret_cast<const sf::Int16*>(packet.getData()), sampleCount, channelCount, sampleRate);
                    std::thread sThread(&playSoundThread, buffer);
                    sThread.detach();
                    break;
                }
                default:
                {
                    std::cout << "* ReceiveThread(): Se ha recibido un opcode desconocido (" << opcode << ")." << std::endl;
                    break;
                }
            }
        }
        else
        {
            if (st == sf::Socket::Disconnected)
            {
                client->disconnect();
                clientConnected = false;
                clientLoggedIn = false;
            }
        }
    }
}

static void pingThread(sf::TcpSocket* client)
{
    while (running)
    {
        sf::sleep(sf::milliseconds(30000));
        if (!clientConnected)
            continue;
        sf::Packet packet;
        packet << CMSG_VCL_PING;
        client->send(packet);
    }
}

int main()
{
    sf::TcpSocket* client = new sf::TcpSocket();

    std::thread pThread(&pingThread, client);
    pThread.detach();

    while (running)
    {
        while (!clientConnected)
        {
            if (client->connect(serverAddress, port) == sf::Socket::Done)
            {
                std::cout << "* Conexion con el servidor establecida." << std::endl;
                clientConnected = true;
            }
            else
            {
                std::cout << "* No se ha podido conectar al servidor. Reintentando..." << std::endl;
                sf::sleep(sf::milliseconds(1000));
            }
        }
        while (!clientLoggedIn)
        {
            if (savedUsername.empty() || savedPassword.empty())
            {
                std::cout << "* Ingrese nombre de usuario: ";
                std::string username;
                std::cin >> username;
                std::cout << "* Ingrese la clave (" << username << "): ";
                std::string password = "";
                char ch = _getch();
                while (ch != 13)
                {
                    password.push_back(ch);
                    std::cout << "*";
                    ch = _getch();
                }
                std::cout << std::endl;
                if (!username.empty() && !password.empty())
                {
                    savedUsername = username;
                    savedPassword = password;
                }
                else
                {
                    std::cout << "* Error: no puede dejar datos en blanco." << std::endl;
                    savedUsername = "";
                    savedPassword = "";
                    continue;
                }
            }
            sf::Packet authPacket;
            authPacket << CMSG_VCL_AUTH << savedUsername << savedPassword;
            sf::Socket::Status stAuth = client->send(authPacket);
            if (stAuth == sf::Socket::Done)
            {
                sf::Packet authRespPacket;
                sf::Socket::Status stAuthResp = client->receive(authRespPacket);
                if (stAuthResp == sf::Socket::Done)
                {
                    sf::Uint32 opcode;
                    authRespPacket >> opcode;
                    switch (opcode)
                    {
                    case SMSG_VCL_AUTH_OK:
                        std::cout << "* Correcto: Presiona CTRL para grabar y sueltalo para enviar." << std::endl;
                        clientLoggedIn = true;
                        break;
                    case SMSG_VCL_AUTH_ERROR_NOT_EXISTS:
                        std::cout << "* Error: el usuario no existe." << std::endl;
                        savedUsername = "";
                        savedPassword = "";
                        break;
                    case SMSG_VCL_AUTH_ERROR_WRONG_PASS:
                        std::cout << "* Error: contraseña incorrecta." << std::endl;
                        savedUsername = "";
                        savedPassword = "";
                        break;
                    default:
                        std::cout << "* Error: el servidor ha enviado un mensaje desconocido (" << opcode << "). Desconectando..." << std::endl;
                        savedUsername = "";
                        savedPassword = "";
                        client->disconnect();
                        clientConnected = false;
                        break;
                    }
                }
                else
                {
                    if (stAuthResp == sf::Socket::Disconnected)
                    {
                        std::cout << "* Error: se ha desconectado del servidor." << std::endl;
                        client->disconnect();
                        clientConnected = false;
                    }
                }
            }
            else
            {
                if (stAuth == sf::Socket::Disconnected)
                {
                    std::cout << "* Error: se ha desconectado del servidor." << std::endl;
                    client->disconnect();
                    clientConnected = false;
                }
            }
            sf::sleep(sf::milliseconds(1000));
        }

        std::thread thread(&receiveThread, client);
        thread.detach();

        while (clientConnected && clientLoggedIn)
        {
            if (!sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
                continue;

            sf::SoundBufferRecorder recorder;
            recorder.start();

            sf::Uint32 timeCount = 0;

            while (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
            {
                if (!clientConnected || !clientLoggedIn)
                    break;
                timeCount++;
                if (timeCount > 600)
                    break;
                sf::sleep(sf::milliseconds(100));
            }

            recorder.stop();

            if (!clientConnected || !clientLoggedIn)
                break;
            if (timeCount < 2)
                continue;

            sf::SoundBuffer buffer;
            buffer = recorder.getBuffer();
            sf::Packet packet;
            packet << CMSG_VCL_AUDIO;
            packet << buffer.getSampleCount() << buffer.getChannelCount() << buffer.getSampleRate();
            packet.append(buffer.getSamples(), buffer.getSampleCount() * sizeof(sf::Int16));
            client->send(packet);
        }
        std::cout << "* Error: se ha desconectado del servidor." << std::endl;
        sf::sleep(sf::milliseconds(30000));
    }
    return 0;
}
