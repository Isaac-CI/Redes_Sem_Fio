#include "ns3/log.h"
#include "intermediate-sensor-app.h"
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
    NS_LOG_COMPONENT_DEFINE("ISS");
    NS_OBJECT_ENSURE_REGISTERED(ISS);

    TypeId
    ISS::GetTypeId()
    {
        static TypeId tid = TypeId("ns3::ISS")
                                .AddConstructor<ISS>()
                                .SetParent<Application>();
        return tid;
    }

    TypeId
    ISS::GetInstanceTypeId() const
    {
        return ISS::GetTypeId();
    }

    ISS::ISS()
    {
        id = 0;
        m_addr = Ipv4Address::GetAny();
        addrServer = Ipv4Address::GetAny();
        for(uint8_t i = 0; i < 6; i++)
            addrSensors[i] = Ipv4Address::GetAny();
        receiver_socket = nullptr;
        sender_socket = nullptr;
    }
    ISS::ISS(Ipv4Address addr1, Ipv4Address addr2, Ipv4Address addr3, Ipv4Address addr4, Ipv4Address addr5,
                    Ipv4Address addr6, Ipv4Address addr7, Ipv4Address addr8, int Id)
    {
        id = Id;
        m_addr = addr1;
        addrServer = addr2;
        addrSensors[0] = addr3;
        addrSensors[1] = addr4;
        addrSensors[2] = addr5;
        addrSensors[3] = addr6;
        addrSensors[4] = addr7;
        addrSensors[5] = addr8;
        receiver_socket = nullptr;
        sender_socket = nullptr;
    }
    ISS::~ISS()
    {
    }
    void ISS::SetupReceiveSocket(Ptr<Socket> socket, Ipv4Address addr)
    { 
        InetSocketAddress local = InetSocketAddress(addr, PORT);
        if (socket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        } 
    }
    void ISS::StartApplication()
    {
        //Receiver socket
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        receiver_socket = UdpSocket::CreateSocket(GetNode(), tid);

        SetupReceiveSocket(receiver_socket, m_addr);
        std::cout << "Intermediate node(Server - Sensors), addr: " << m_addr << std::endl;

        receiver_socket->SetRecvCallback(MakeCallback(&ISS::ISSCallback, this));
        receiver_socket->SetRecvPktInfo(true);
        receiver_socket->SetAllowBroadcast(true);

        //Sender Socket
        sender_socket = UdpSocket::CreateSocket(GetNode(), tid);
        if(sender_socket->Bind(InetSocketAddress(m_addr, PORT)) == -1)
            NS_FATAL_ERROR("Failed to bind socket");

        sender_socket->SetRecvPktInfo(true);
        sender_socket->SetAllowBroadcast(true);
    }

    void ISS::ISSCallback(Ptr<Socket> socket){
        
        ns3::Ptr<ns3::Packet> packetS;
        ns3::Address from; 
        std::cout << "Here" << std::endl;
        while ((packetS = socket->RecvFrom(from)))
        {
            uint32_t packetSize = packetS->GetSize();
            ns3::Ipv4Address senderAddress = ns3::InetSocketAddress::ConvertFrom(from).GetIpv4();

            // Lógica para processar o pacote recebido
            // ...
            uint8_t buffer[packetSize];
            packetS->CopyData(buffer, packetSize);
            messageData* data = (messageData*)malloc(sizeof(messageData));
            data->source = buffer[0];
            data->dest = buffer[1];
            data->command = buffer[2];
            data->payload = buffer[3];
            uint8_t* errorMsg = (uint8_t*)malloc(sizeof(messageData));

            if(data->source == 10){ // Veio do servidor
            // Repassa mensagem recebida do servidor para os sensores que a interessam.
                switch (data->command)
                {
                    case 0: // servidor deseja descobrir estado atual dos sensores
                        for(uint8_t i = 0; i < 8; i++){ // repassa a mensagem para cada um dos sensores solicitando seus valores atuais
                            SendPacket(packetS, addrSensors[i], PORT);
                            // sender_socket->Connect(InetSocketAddress(addrSensors[i], PORT));
                            // sender_socket->Send(packetS);
                        }
                        break;
                    case 1: // servidor deseja esvaziar uma das prateleiras
                        if(data->dest > 6 || data->dest < 0){ // caso o destino esteja fora do intervalo permitido, isto é, não seja o identificador de algum sensor
                            errorMsg[0] = 12; // quem manda é o intermediário entre sensores e servidor
                            errorMsg[1] = data->source;  // endereço de quem enviou a mensagem
                            errorMsg[2] = 5;  // codigo de mensagem de erro
                            errorMsg[3] = 1;  // codigo que indica que o erro foi de destino inválido
                            packetS = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                            SendPacket(packetS, senderAddress, PORT);
                            // sender_socket->Connect(InetSocketAddress(senderAddress, PORT));
                            // sender_socket->Send(packetS); // envia de volta para quem enviou a mensagem, indicando erro na comunicação
                        }else{
                            SendPacket(packetS, addrSensors[data->dest - 1], PORT);
                            // sender_socket->Connect(InetSocketAddress(addrSensors[data->dest - 1], PORT));
                            // sender_socket->Send(packetS); // repassa a mensagem para o sensor a ser esvaziado
                        }

                        break;
                    case 2: // servidor deseja preencher uma prateleira
                        if(data->dest > 6 || data->dest < 0){ // caso o destino esteja fora do intervalo permitido, isto é, seja o identificador de algum sensor
                            //uint8_t* errorMsg = (uint8_t*)malloc(sizeof(messageData));
                            errorMsg[0] = 12; // quem manda é o intermediário entre sensores e servidor
                            errorMsg[1] = data->source;  // endereço de quem enviou a mensagem
                            errorMsg[2] = 5;  // codigo de mensagem de erro
                            errorMsg[3] = 1;  // codigo que indica que o erro foi de destino inválido
                            packetS = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                            SendPacket(packetS, senderAddress, PORT);
                            //sender_socket->Connect(InetSocketAddress(senderAddress, PORT));
                            //sender_socket->Send(packetS);// envia de volta para quem enviou a mensagem, indicando erro na comunicação
                            
                            break;
                        }else{
                                SendPacket(packetS, addrSensors[data->dest - 1], PORT);
                                // sender_socket->Connect(InetSocketAddress(addrSensors[data->dest - 1], PORT));
                                // sender_socket->Send(packetS);// repassa a mensagem para o sensor a ser preenchido
                            }

                        break;
                    case 5: // Ocorreu Erro
                        break;
                    default: // instrução inválida, envia mensagem de erro de volta para o nó que enviou a mensagem original.
                        //uint8_t* errorMsg = (uint8_t*)malloc(sizeof(messageData));
                        errorMsg[0] = 12; // quem manda é o intermediário entre sensores e servidor
                        errorMsg[1] = data->source;  // endereço de quem enviou a mensagem
                        errorMsg[2] = 5;  // codigo de mensagem de erro
                        errorMsg[3] = 2;  // codigo que indica que o erro foi de comando inválido
                        packetS = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                        sender_socket->SendTo(packetS, 0, InetSocketAddress(senderAddress, PORT)); // envia de volta para quem enviou a mensagem, indicando erro na comunicação

                        break;
                }
            } else {
                if(data->source > 0 && data->source <= 6){ // é algum dos sensores
                    SendPacket(packetS, addrServer, PORT);
                    // sender_socket->Connect(InetSocketAddress(addrServer, PORT));
                    // sender_socket->Send(packetS); // repassa a mensagem para o servidor
                }else{ // Inconsistência na mensagem
                    // uint8_t* errorMsg = (uint8_t*)malloc(sizeof(messageData));
                    errorMsg[0] = 12; // quem manda é o intermediário entre sensores e servidor
                    errorMsg[1] = data->source; // endereço de quem enviou a mensagem
                    errorMsg[2] = 5;  // codigo de mensagem de erro
                    errorMsg[3] = 0;  // codigo que indica que o erro foi de fonte inválida
                    packetS = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                    SendPacket(packetS, senderAddress, PORT);
                    // sender_socket->Connect(InetSocketAddress(senderAddress, 550));
                    // sender_socket->Send(packetS); // envia de volta para quem enviou a mensagem, indicando erro na comunicação
                }
            }
        }
    }

    void ISS::SendPacket(Ptr<Packet> packet, Ipv4Address destination, uint16_t port)
    {
        NS_LOG_FUNCTION (this << m_addr << packet->ToString() << destination << port << Simulator::Now());
        sender_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(destination), port));
        sender_socket->SetAllowBroadcast(true);
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