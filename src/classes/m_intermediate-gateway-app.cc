#include "ns3/log.h"
#include "m_intermediate-gateway-app.h"
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
    NS_LOG_COMPONENT_DEFINE("IGS");
    NS_OBJECT_ENSURE_REGISTERED(IGS);

    TypeId
    IGS::GetTypeId()
    {
        static TypeId tid = TypeId("ns3::IGS")
                                .AddConstructor<IGS>()
                                .SetParent<Application>();
        return tid;
    }

    TypeId
    IGS::GetInstanceTypeId() const
    {
        return IGS::GetTypeId();
    }

    IGS::IGS()
    {
        id = 0;
        addrGateway = Ipv4Address::GetAny();
        addrServer = Ipv4Address::GetAny();
        m_addr = Ipv4Address::GetAny();
        receiver_socket = nullptr;
        sender_socket = nullptr;
    }
    IGS::IGS(Ipv4Address addr1, Ipv4Address addr2, Ipv4Address addr3, int Id, LibRedes handler)
    {
        id = Id;
        m_addr = addr1;
        addrServer = addr2;
        addrGateway = addr3;
        shelves = handler.shelves;
        gateway_commands = handler.gateway_commands;
        gateway_target = handler.gateway_target;
        receiver_socket = nullptr;
        sender_socket = nullptr;
    }
    IGS::~IGS()
    {
    }
    void IGS::SetupReceiveSocket(Ptr<Socket> socket, Ipv4Address addr)
    { 
        InetSocketAddress local = InetSocketAddress(addr, PORT);
        if (socket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        } 
    }
    void IGS::StartApplication()
    {
        //Receiver socket
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        receiver_socket = UdpSocket::CreateSocket(GetNode(), tid);

        SetupReceiveSocket(receiver_socket, m_addr);
        std::cout << "Intermediate node(Server - Gateway), addr: " << m_addr << std::endl;

        receiver_socket->SetRecvCallback(MakeCallback(&IGS::IGSCallback, this));
        receiver_socket->SetRecvPktInfo(true);
        receiver_socket->SetAllowBroadcast(true);

        //Sender Socket
        sender_socket = UdpSocket::CreateSocket(GetNode(), tid);
        if(sender_socket->Bind(InetSocketAddress(m_addr, PORT)) == -1)
            NS_FATAL_ERROR("Failed to bind socket");

        sender_socket->SetRecvPktInfo(true);
        sender_socket->SetAllowBroadcast(true);
    }

    void IGS::IGSCallback(Ptr<Socket> socket){
        ns3::Ptr<ns3::Packet> packetServer;
        ns3::Address from;
        std::cout << "Here" << std::endl;
        while ((packetServer = socket->RecvFrom(from))){
            uint32_t packetSize = packetServer->GetSize();
            ns3::Ipv4Address senderAddress = ns3::InetSocketAddress::ConvertFrom(from).GetIpv4();

            // Lógica para processar o pacote recebido
            // ...
            uint8_t buffer[packetSize];
            packetServer->CopyData(buffer, packetSize);
            messageData* data = (messageData*)malloc(sizeof(messageData));
            data->source = buffer[0];
            data->dest = buffer[1];
            data->command = buffer[2];
            data->payload = buffer[3];

            if(data->source == 13){ // Gateway->Servidor
                uint8_t* msg = (uint8_t*)malloc(sizeof(messageData));
                msg[0] = data->source;
                msg[1] = data->dest;
                msg[2] = data->command;
                msg[3] = data->payload;  
                packetServer = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                SendPacket(packetServer, addrServer, PORT);
                // sender_socket->Connect(InetSocketAddress(addrServer, PORT));
                // sender_socket->Send(packetServer); // repassa a mensagem para o servidor
            }else if(data->source == 10){ // Servidor->Gateway
                uint8_t* msg = (uint8_t*)malloc(sizeof(messageData));
                msg[0] = data->source;
                msg[1] = data->dest;
                msg[2] = data->command;
                msg[3] = data->payload;  
                packetServer = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                SendPacket(packetServer, addrGateway, PORT);
                // sender_socket->Connect(InetSocketAddress(addrGateway, PORT));
                // sender_socket->Send(packetServer); // repassa a mensagem para o servidor
            }else{
                uint8_t* errorMsg = (uint8_t*)malloc(sizeof(messageData));
                errorMsg[0] = data->source; // A nova mensagem tem como fonte o servidor
                errorMsg[1] = data->source;  // Essa mensagem não deveria ter saído do source
                errorMsg[2] = 5;  // Código de erro
                errorMsg[3] = 0;  // não importa, deixo em 0.
                packetServer = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                SendPacket(packetServer, senderAddress, PORT);
                // sender_socket->Connect(InetSocketAddress(senderAddress, PORT));
                // sender_socket->Send(packetServer); // repassa a mensagem de sucesso para o nó intermediário entre servidor e gateway
                NS_LOG_ERROR("Erro ao enviar pacote indevido ao nó intermediário 'Gateway-Servidor'. Retornando ao nó de origem.");
            }
        }

    }

    void IGS::SendPacket(Ptr<Packet> packet, Ipv4Address destination, uint16_t port)
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