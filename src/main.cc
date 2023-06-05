#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"

#define SENSOR_ADDRESS "10.0.0.0"
#define INTERMEDIATE_ADDRESS "10.0.1.0"
#define SERVER_ADDRESS "10.0.2.0"
#define GATEWAY_ADDRESS "10.0.3.0"

using namespace ns3;
using namespace std;

ns3::Ipv4InterfaceContainer sensorInterfaces, intermediateInterfaces, serverInterfaces, gatewayInterfaces;

typedef struct{
    uint8_t
} messageData;

// Função de callback para o recebimento de dados
void ReceivePacket(ns3::Ptr<ns3::Socket> socket)
{
    ns3::Ptr<ns3::Packet> packet;
    ns3::Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        uint32_t packetSize = packet->GetSize();
        ns3::Ipv4Address senderAddress = ns3::InetSocketAddress::ConvertFrom(from).GetIpv4();

        // Lógica para processar o pacote recebido
        // ...

        // Exemplo de impressão dos dados recebidos
        std::cout << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << std::endl;
    }
}

// Função de callback para o recebimento de dados

void SensorReceivePacket(ns3::Ptr<ns3::Socket> socket)
{
    // Mensagem: source - destino final - comando - dado
    ns3::Ptr<ns3::Packet> packet;
    ns3::Address from;
    bool isSensor = false;
    bool isIntermediate = false;
    bool isServer = false;
    bool isGateway = false;
    while ((packet = socket->RecvFrom(from)))
    {
        uint32_t packetSize = packet->GetSize();
        ns3::Ipv4Address senderAddress = ns3::InetSocketAddress::ConvertFrom(from).GetIpv4();
        for(uint8_t i = 0; i < sensorInterfaces.GetN(); i++){
            if(senderAddress == sensorInterfaces.GetAddress(i)){
                isSensor = true;
            }
        }
        if(!isSensor){
            for(uint8_t i = 0; i  < intermediateInterfaces.GetN(); i++){
                if(senderAddress == intermediateInterfaces.GetAddress(i)){
                    isIntermediate = true;
                }
            }
            if(!isIntermediate){
                if(senderAddress == )
            }
        }

        // Lógica para processar o pacote recebido
        // ...

        // Exemplo de impressão dos dados recebidos
        std::cout << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << std::endl;
    }
}
// Função de callback para o recebimento de dados

void ServerReceivePacket(ns3::Ptr<ns3::Socket> socket)
{
    ns3::Ptr<ns3::Packet> packet;
    ns3::Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        uint32_t packetSize = packet->GetSize();
        ns3::Ipv4Address senderAddress = ns3::InetSocketAddress::ConvertFrom(from).GetIpv4();

        // Lógica para processar o pacote recebido
        // ...

        // Exemplo de impressão dos dados recebidos
        std::cout << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << std::endl;
    }
}// Função de callback para o recebimento de dados


// // Comparar o endereço de origem com os endereços IP dos dispositivos de sensores
// for (uint32_t i = 0; i < sensorInterfaces.GetN(); ++i)
// {
//     ns3::Ipv4Address sensorAddress = sensorInterfaces.GetAddress(i);

//     if (senderAddress == sensorAddress)
//     {
//         // O pacote foi enviado pelo dispositivo de sensor com o endereço IP correspondente
//         std::cout << "Pacote recebido do dispositivo de sensor " << i << std::endl;
//         // Lógica adicional...
//         break;
//     }
// }


void IntermediateReceivePacket(ns3::Ptr<ns3::Socket> socket)
{
    ns3::Ptr<ns3::Packet> packet;
    ns3::Address from;
    Ipv4Address source = InetSocketAddress::ConvertFrom(from).GetIpv4();

    while ((packet = socket->RecvFrom(from)))
    {
        if(source ==(Ipv4Address)SERVER_ADDRESS){
            
        } else if(source == (Ipv4Address)GATEWAY_ADDRESS){

        } else{

        }
        
        uint32_t packetSize = packet->GetSize();
        ns3::Ipv4Address senderAddress = ns3::InetSocketAddress::ConvertFrom(from).GetIpv4();

        // Lógica para processar o pacote recebido
        // ...

        // Exemplo de impressão dos dados recebidos
        std::cout << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << std::endl;
    }
}

int main(){
    NodeContainer sensorNodes;
    sensorNodes.Create(6);

    NodeContainer intermediateNodes;
    intermediateNodes.Create(2);

    NodeContainer serverNode;
    serverNode.Create(1);

    NodeContainer gatewayNode;
    gatewayNode.Create(1);

    //Create WIFI helpers for layers 1 and 2
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    //Create WIFI helpers for MAC addressing
    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

    //Create WIFI helper
    WifiHelper wifi;

    //Create a WIFI device container for the structured network
    NetDeviceContainer sensorDevices, serverDevice, gatewayDevice, intermediateDevices;
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    sensorDevices = wifi.Install(phy, mac, sensorNodes);
    intermediateDevices = wifi.Install(phy, mac, intermediateNodes);
    serverDevice = wifi.Install(phy, mac, serverNode);
    gatewayDevice = wifi.Install(phy, mac, gatewayNode);
 

    // ----------------------- NODE MOBILITY SECTION ------------------------------------------
    
    MobilityHelper sensorMobility, intermediateMobility, serverMobility, gatewayMobility;

    Ptr<ConstantPositionMobilityModel> gatewayMobilityModel = CreateObject<ConstantPositionMobilityModel>();
    double xCoord = 0.0;
    double yCoord = 0.0;
    double zCoord = 0.0;
    gatewayMobilityModel->SetPosition(ns3::Vector(xCoord, yCoord, zCoord));
    gatewayNode.Get(0)->AggregateObject(gatewayMobilityModel);

    Ptr<ConstantPositionMobilityModel> serverMobilityModel = CreateObject<ConstantPositionMobilityModel>();
    xCoord = 100.0;
    yCoord = 100.0;
    serverMobilityModel->SetPosition(ns3::Vector(xCoord, yCoord, zCoord));
    serverNode.Get(0)->AggregateObject(serverMobilityModel);

    Ptr<ConstantPositionMobilityModel> gatewayServerIntermediateMobilityModel = CreateObject<ConstantPositionMobilityModel>();
    xCoord = 50.0;
    yCoord = 50.0;
    gatewayServerIntermediateMobilityModel->SetPosition(ns3::Vector(xCoord, yCoord, zCoord));
    intermediateNodes.Get(0)->AggregateObject(gatewayServerIntermediateMobilityModel);

    Ptr<ConstantPositionMobilityModel> serverShelfIntermediateMobilityModel = CreateObject<ConstantPositionMobilityModel>();
    xCoord = 150.0;
    yCoord = 100.0;
    serverShelfIntermediateMobilityModel->SetPosition(ns3::Vector(xCoord, yCoord, zCoord));
    intermediateNodes.Get(1)->AggregateObject(serverShelfIntermediateMobilityModel);

    std::vector<Ptr<ConstantPositionMobilityModel>> sensorMobilityModels;
    xCoord = 175.0;
    yCoord = 125.0;
    uint shelfGroup = 1;

    for(uint idx = 0; idx < sensorNodes.GetN(); idx++){
        Ptr<ConstantPositionMobilityModel> sensorMobilityModel = CreateObject<ConstantPositionMobilityModel>();
        sensorMobilityModels.push_back(sensorMobilityModel);
        sensorMobilityModels[idx]->SetPosition(Vector(xCoord, yCoord, zCoord));
        sensorNodes.Get(idx)->AggregateObject(sensorMobilityModels[idx]);
    
        zCoord = shelfGroup%2==0 ? 0.0 : zCoord+=10.0 ;
        yCoord = shelfGroup%2==0 ? yCoord : yCoord-=25.0 ;
        shelfGroup = shelfGroup%2==0 ? 1 : shelfGroup+=1 ;
    }


    //Install Internet Stacks on each node
    InternetStackHelper stack;
    stack.Install(serverNode);
    stack.Install(intermediateNodes);
    stack.Install(gatewayNode);
    stack.Install(sensorNodes);

    //Create an Address Helper

    Ipv4AddressHelper address;
    address.SetBase(SENSOR_ADDRESS, "255.255.255.0");
    sensorInterfaces = address.Assign(sensorDevices);

    address.SetBase(INTERMEDIATE_ADDRESS, "255.255.255.0");
    intermediateInterfaces = address.Assign(intermediateDevices);

    address.SetBase(SERVER_ADDRESS, "255.255.255.0");
    serverInterfaces = address.Assign(serverDevice);

    address.SetBase(GATEWAY_ADDRESS, "255.255.255.0");
    gatewayInterfaces = address.Assign(gatewayDevice);

    ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Criar um socket UDP no nó de envio
    ns3::Ptr<ns3::Socket> senderSocket = ns3::Socket::CreateSocket(sensorNodes.Get(0), ns3::UdpSocketFactory::GetTypeId());
    ns3::InetSocketAddress senderAddress = ns3::InetSocketAddress(sensorInterfaces.GetAddress(0), 9); // Endereço IP do nó de servidor e porta

    // Criar um socket UDP no nó de recebimento
    ns3::Ptr<ns3::Socket> receiverSocket = ns3::Socket::CreateSocket(serverNode.Get(0), ns3::UdpSocketFactory::GetTypeId());
    ns3::InetSocketAddress receiverAddress = ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(), 9); // Qualquer endereço IP disponível e porta

    // Associar o socket de recebimento com a função de callback
    receiverSocket->Bind(receiverAddress);
    receiverSocket->SetRecvCallback(ns3::MakeCallback(&ReceivePacket));

    // Lógica para envio de dados
    ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet>(1024); // Tamanho do pacote em bytes
    senderSocket->SendTo(packet, 0, senderAddress);



    // ...
    // Applicação

    ns3::Simulator::Run();
    ns3::Simulator::Destroy();

    return 0;
}
