#include "ns3/log.h"
#include "m_gateway-app.h"
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

#define RECV_PORT 5500
#define SEND_PORT 80

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("GatewayApp");
    NS_OBJECT_ENSURE_REGISTERED(GatewayApp);

    TypeId
    GatewayApp::GetTypeId()
    {
        static TypeId tid = TypeId("ns3::GatewayApp")
                                .AddConstructor<GatewayApp>()
                                .SetParent<Application>();
        return tid;
    }

    TypeId
    GatewayApp::GetInstanceTypeId() const
    {
        return GatewayApp::GetTypeId();
    }

    GatewayApp::GatewayApp()
    {
        id = 0;
        m_addr = Ipv4Address::GetAny();
        addrIGS = Ipv4Address::GetAny();
        receiver_socket = nullptr;
        sender_socket = nullptr;
    }
    GatewayApp::GatewayApp(Ipv4Address addr1, Ipv4Address addr2, int Id, LibRedes handler)
    {
        id = Id;
        m_addr = addr1; 
        addrIGS = addr2;
        shelves = handler.shelves;
        gateway_commands = handler.gateway_commands;
        gateway_target = handler.gateway_target;
        receiver_socket = nullptr;
        sender_socket = nullptr;
    }
    GatewayApp::~GatewayApp()
    {
    }
    void GatewayApp::SetupReceiveSocket(Ptr<Socket> socket, Ipv4Address addr)
    { 
        InetSocketAddress local = InetSocketAddress(addr, RECV_PORT);
        if (socket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        } 
    }
    void GatewayApp::StartApplication()
    {
        //Receiver socket
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        receiver_socket = Socket::CreateSocket(GetNode(), tid);
        SetupReceiveSocket(receiver_socket, m_addr);
        std::cout << "Gateway node, addr: " << m_addr << std::endl;

        receiver_socket->SetRecvCallback(MakeCallback(&GatewayApp::GatewayCallback, this));
        receiver_socket->SetRecvPktInfo(true);
        receiver_socket->SetAllowBroadcast(true);

        //Sender Socket
        sender_socket = Socket::CreateSocket(GetNode(), tid);
        if(sender_socket->Bind(InetSocketAddress(m_addr, SEND_PORT)) == -1)
            NS_FATAL_ERROR("Failed to bind socket");

        sender_socket->SetRecvPktInfo(true);
        sender_socket->SetAllowBroadcast(true);
    }

    void GatewayApp::GatewayCallback(Ptr<Socket> socket){
        
        ns3::Ptr<ns3::Packet> packetGateway;
        ns3::Address from;
        std::cout << "Here" << std::endl;
        while ((packetGateway = socket->RecvFrom(from)))
        {
            uint32_t packetSize = packetGateway->GetSize();
            ns3::Ipv4Address senderAddress = ns3::InetSocketAddress::ConvertFrom(from).GetIpv4();

            // Lógica para processar o pacote recebido
            // ...
            uint8_t buffer[packetSize];
            packetGateway->CopyData(buffer, packetSize);
            messageData* data = (messageData*)malloc(sizeof(messageData));
            data->source = buffer[0];
            data->dest = buffer[1];
            data->command = buffer[2];
            data->payload = buffer[3];
            
            NS_LOG_INFO(GREEN_CODE << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << END_CODE);
            NS_LOG_INFO(GREEN_CODE << "src: " << data->source << ", dest: " << data->dest << ", command: " << data->command << ", payload" << data->payload << END_CODE);

            switch (data->command)
            {
            case 1: // Esvaziamento finalizado com sucesso
                NS_LOG_INFO(PURPLE_CODE << "Produto dispachado com sucesso" << END_CODE);
                break;
            case 2: // Preenchimendo finalizado com sucesso
                NS_LOG_INFO(PURPLE_CODE << "Produto armazenado com sucesso" << END_CODE);
                break;
            case 5: // erro
                if(data->payload == 5)
                    NS_LOG_INFO(PURPLE_CODE << "Inconsistência de valores, alertando central" << END_CODE);
                else if(data->payload == 4)
                    NS_LOG_INFO(PURPLE_CODE << "Tentativa de armazenar produto em prateleira ocupada" << END_CODE);
                else if(data->payload == 3)
                    NS_LOG_INFO(PURPLE_CODE << "Tentativa de retirar produto de prateleira vazia" << END_CODE);

            default:
                break;
            }
        }
    }

    void GatewayApp::SendPacket(Ptr<Packet> packet, Ipv4Address destination, uint16_t port)
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