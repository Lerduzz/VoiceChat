#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <thread>

enum PacketOpcode
{
    CMSG_VCL_PING,
    CMSG_VCL_AUDIO,
    SMSG_VCL_AUDIO
};

sf::IpAddress serverAddress("sking.lerduzz.com");
sf::Uint16 port = 8000;

static void playSoundThread(sf::SoundBuffer* buffer) {
    sf::Sound* sound = new sf::Sound(*buffer);
    sound->play();
    while (sound->getStatus() == sf::Sound::Playing)
        sf::sleep(sf::milliseconds(100));
    delete sound;
    delete buffer;
}

static void receiveThread(sf::TcpSocket* client) {
    std::cout << "* ReceiveThread(): Inicializando para escuchar." << std::endl;
    while (true) {
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
                while (client->connect(serverAddress, port) != sf::Socket::Done)
                {
                    std::cout << "* Intentando conectar al servidor." << std::endl;
                    sf::sleep(sf::milliseconds(1000));
                }
                std::cout << "* Conectado correctamente al servidor." << std::endl;
            }
        }
    }
}

static void pingThread(sf::TcpSocket* client)
{
    while (true)
    {
        sf::sleep(sf::milliseconds(60000));
        std::cout << "* Enviando ping para mantener la conexion activa." << std::endl;
        sf::Packet packet;
        packet << CMSG_VCL_PING;
        client->send(packet);
    }
}

int main()
{
    sf::TcpSocket* client = new sf::TcpSocket();

    while (client->connect(serverAddress, port) != sf::Socket::Done)
    {
        std::cout << "* Intentando conectar al servidor." << std::endl;
        sf::sleep(sf::milliseconds(1000));
    }

    std::cout << "* Conectado correctamente al servidor." << std::endl;

    std::thread thread(&receiveThread, client);
    thread.detach();

    std::thread pThread(&pingThread, client);
    pThread.detach();

    while (true)
    {
        if (!sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
            continue;

        sf::SoundBufferRecorder recorder;
        recorder.start();

        sf::Uint32 timeCount = 0;
       
        while (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
        {
            timeCount++;
            if (timeCount > 600)
                 break;
            sf::sleep(sf::milliseconds(100));
        }
        
        recorder.stop();
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
    return 0;
}
