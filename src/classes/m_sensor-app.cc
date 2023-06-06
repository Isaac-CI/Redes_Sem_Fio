#include "ns3/log.h"
#include "m_sensor-app.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/csma-net-device.h"
#include "ns3/ethernet-header.h"
#include "ns3/arp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"

#define PURPLE_CODE "\033[95m"
#define CYAN_CODE "\033[96m"
#define TEAL_CODE "\033[36m"
#define BLUE_CODE "\033[94m"
#define GREEN_CODE "\033[32m"
#define YELLOW_CODE "\033[33m"
#define LIGHT_YELLOW_CODE "\033[93m"
#define RED_CODE "\033[91m"
#define BOLD_CODE "\033[1m"
#define END_CODE "\033[0m"

#define PORT 5500

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("SensorApp");
    NS_OBJECT_ENSURE_REGISTERED(SensorApp);

    TypeId
    SensorApp::GetTypeId()
    {
        static TypeId tid = TypeId("ns3::SensorApp")
                                .AddConstructor<SensorApp>()
                                .SetParent<Application>();
        return tid;
    }

    TypeId
    SensorApp::GetInstanceTypeId() const
    {
        return SensorApp::GetTypeId();
    }

    SensorApp::SensorApp()
    {
        id = 0;
        addrISS = Ipv4Address::GetAny();
        receiver_socket = nullptr;
        sender_socket = nullptr;
    }
    SensorApp::SensorApp(Ipv4Address addr1, Ipv4Address addr2, int Id, LibRedes handler)
    {
        id = Id;
        m_addr = addr1;
        addrISS = addr2;
        shelves = handler.shelves;
        sensor_state = shelves[id - 1].front();
        gateway_commands = handler.gateway_commands;
        gateway_target = handler.gateway_target;
        receiver_socket = nullptr;
        sender_socket = nullptr;
    }
    SensorApp::~SensorApp()
    {
    }
    void SensorApp::SetupReceiveSocket(Ptr<Socket> socket, Ipv4Address addr)
    { 
        InetSocketAddress local = InetSocketAddress(addr, PORT);
        if (socket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        } 
    }
    void SensorApp::StartApplication()
    {
        //Receiver socket
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        receiver_socket = Socket::CreateSocket(GetNode(), tid);

        SetupReceiveSocket(receiver_socket, m_addr);
        std::cout << "Sensor node, addr: " << m_addr << std::endl;

        receiver_socket->SetRecvCallback(MakeCallback(&SensorApp::SensorCallback, this));
        receiver_socket->SetRecvPktInfo(true);
        receiver_socket->SetAllowBroadcast(true);

        //Sender Socket
        sender_socket = Socket::CreateSocket(GetNode(), tid);
        if(sender_socket->Bind(InetSocketAddress(m_addr, PORT)) == -1)
            NS_FATAL_ERROR("Failed to bind socket");

        sender_socket->SetRecvPktInfo(true);
        sender_socket->SetAllowBroadcast(true);
    }

    void SensorApp::SensorCallback(Ptr<Socket> socket) {
        ns3::Ptr<ns3::Packet> packetSensor;
        ns3::Address from;
        std::cout << "Here" << std::endl;
        while ((packetSensor = socket->RecvFrom(from)))
        {
            uint32_t packetSize = packetSensor->GetSize();
            ns3::Ipv4Address senderAddress = ns3::InetSocketAddress::ConvertFrom(from).GetIpv4();

            // Lógica para processar o pacote recebido
            // ...
            uint8_t buffer[packetSize];
            packetSensor->CopyData(buffer, packetSize);
            messageData* data = (messageData*)malloc(sizeof(messageData));
            data->source = buffer[0];
            data->dest = buffer[1];
            data->command = buffer[2];
            data->payload = buffer[3];
            
            uint8_t* msg = (uint8_t*)malloc(sizeof(messageData));
            NS_LOG_INFO(GREEN_CODE << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << END_CODE);
            NS_LOG_INFO(GREEN_CODE << "src: " << data->source << ", dest: " << data->dest << ", command: " << data->command << ", payload" << data->payload << END_CODE);
            if(id > 0 && id <= 6){ // Se o Sensor Id se referir a uma prateleira existente
                switch (data->command)
                {
                case 0: // Verificar estado dos sensores
                if(shelves[id - 1].size() > 0){ // Verifica se a fila não está vazia
                        sensor_state = shelves[id - 1].front(); // Lê do vetor do log das leituras do sensor da prateleira 1 o próximo valor e atualiza a tabela de sensores.
                        shelves[id - 1].pop(); // elimina o valor da fila.
                        msg[0] = 1; // A nova mensagem tem como fonte o sensor da prateleira 1
                        msg[1] = 10;  // Identificador do servidor
                        msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                        msg[3] = sensor_state;  // Payload assume o valor da leitura do sensor
                        packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                        SendPacket(packetSensor, addrISS, PORT);// repassa a mensagem para o nó intermediário entre servidor e sensores
                        //sender_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(addrISS), PORT));
                        //sender_socket->Send(packetSensor); // repassa a mensagem para o nó intermediário entre servidor e sensores

                    } else { // caso esteja vazia, avisa que não há mais leituras do sensor
                        NS_LOG_INFO(YELLOW_CODE << "Log esvaziado. Não Há mais leituras do sensor da prateleira " << id << END_CODE);
                    }
                    
                    break;
                case 1: // Esvaziar prateleria
                if(shelves[id - 1].size() > 0){ // Verifica se a fila não está vazia
                        sensor_state = false; //Esvazia a prateleira.
                        msg[0] = 1; // A nova mensagem tem como fonte o sensor da prateleira 1
                        msg[1] = 10;  // Identificador do servidor
                        msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                        msg[3] = false;  // Payload assume o valor 0, indicando que a prateleira foi esvaziada
                        packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                        SendPacket(packetSensor, addrISS, PORT);
                        //sender_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(addrISS), PORT));
                        //sender_socket->Send(packetSensor); // repassa a mensagem para o nó intermediário entre servidor e sensores
                    } else { // caso esteja vazia, avisa que não há mais leituras do sensor
                        NS_LOG_INFO(YELLOW_CODE << "Log esvaziado. Não Há mais leituras do sensor da prateleira " << id << END_CODE);
                    }

                    break;
                case 2: // Preencher prateleira
                        sensor_state = true; //Preenche a prateleira.
                        msg[0] = 1; // A nova mensagem tem como fonte o sensor da prateleira 1
                        msg[1] = 10;  // Identificador do servidor
                        msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                        msg[3] = true;  // Payload assume o valor 0, indicando que a prateleira foi preenchida
                        packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                        SendPacket(packetSensor, addrISS, PORT);
                        //sender_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(addrISS), PORT));
                        //sender_socket->Send(packetSensor); // repassa a mensagem para o nó intermediário entre servidor e sensores
                    break;
                default: // Inconsistência na mensagem(Não deve entrar aqui)
                    NS_LOG_INFO(RED_CODE << "O sensor referente a prateleira" << id << "não consegue processar o comando enviado." << END_CODE);
                    break;
                }
            } else { // Se Sensor Id se referir a uma prateleira inexistente.
                    NS_LOG_INFO(RED_CODE << "Prateleira " << id << "não existe no contexto da rede." << END_CODE);
            }
        }
    }

    void SensorApp::SendPacket(Ptr<Packet> packet, Ipv4Address destination, uint16_t port)
    {
        NS_LOG_FUNCTION (this << m_addr << packet->ToString() << destination << port << Simulator::Now());
        sender_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(destination), port));
        if(sender_socket->Send(packet) == -1)
            std::cout << "Failed Transmission" << std::endl;
        else{
            uint8_t* buff  = (uint8_t*)malloc(packet->GetSize());
            packet->CopyData(buff, packet->GetSize());
            int i[4];
            i[0] = buff[0];
            i[1] = buff[1];
            i[2] = buff[2];
            i[3] = buff[3];
            i[4] = buff[4];
            std::cout << "Sending: " << i[0] << " " << i[1] << " " << i[2] << " " << i[3] << " " << " to " << destination << std::endl;
            }

    }
} // namespace ns3