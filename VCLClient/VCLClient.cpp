#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <thread>

sf::IpAddress serverAddress("192.168.10.153");
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
        sf::Packet* packet = new sf::Packet();
        sf::Socket::Status st = client->receive(*packet);
        if (st == sf::Socket::Disconnected || st == sf::Socket::Error)
        {
            client->disconnect();
            while (client->connect(serverAddress, port) != sf::Socket::Done)
            {
                std::cout << "* Intentando conectar al servidor." << std::endl;
                sf::sleep(sf::milliseconds(1000));
            }
            std::cout << "* Conectado correctamente al servidor." << std::endl;
            continue;
        }
        if (st == sf::Socket::NotReady)
            continue;
        std::cout << "* ReceiveThread(): Se ha recibido audio." << std::endl;
        sf::Uint64 sampleCount;
        unsigned int channelCount;
        unsigned int sampleRate;
        *packet >> sampleCount >> channelCount >> sampleRate;

        sf::SoundBuffer* buffer = new sf::SoundBuffer();
        buffer->loadFromSamples(reinterpret_cast<const sf::Int16*>(packet->getData()), sampleCount, channelCount, sampleRate);

        std::thread sThread(&playSoundThread, buffer);
        sThread.detach();
    }
}

int main() {
    sf::TcpSocket* client = new sf::TcpSocket();

    while (client->connect(serverAddress, port) != sf::Socket::Done)
    {
        std::cout << "* Intentando conectar al servidor." << std::endl;
        sf::sleep(sf::milliseconds(1000));
    }

    std::cout << "* Conectado correctamente al servidor." << std::endl;

    std::thread thread(&receiveThread, client);
    thread.detach();

    while (true) {
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
        packet << buffer.getSampleCount() << buffer.getChannelCount() << buffer.getSampleRate();
        packet.append(buffer.getSamples(), buffer.getSampleCount() * sizeof(sf::Int16));
        client->send(packet); // TODO: Comprimir paquete antes de enviarlo.
    }

    return 0;
}
