#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/applications-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/netanim-module.h"
#include "components/libRedes.h"
#include <fstream>
#include <vector>

#define AP_ADDRESS "10.1.1.0"
// #define INTERMEDIATE_ADDRESS "10.1.2.0"
// #define SERVER_ADDRESS "10.1.3.0"
// #define GATEWAY_ADDRESS "10.1.4.0"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("main");

//leitura de sensores
std::queue<bool> shelf1;
std::queue<bool> shelf2;
std::queue<bool> shelf3;
std::queue<bool> shelf4;
std::queue<bool> shelf5;
std::queue<bool> shelf6;
std::queue<int> gateway_commands;
std::queue<int> gateway_target;

// Estado que o servidor tem dos sensores
std::vector<bool> server_state_table;

// Modificado e lido apenas pelos sensores: indica que o sensor serve como atuador
std::vector<bool> sensor_state_vector;

int loadFile(void){
    std::ifstream file("/home/mai/Documents/Redes_sem_Fio/src/data/instance.txt");
    std::cout << "opening file" << std::endl;

    if (file.fail()) {
        std::cout << "Erro ao abrir o arquivo. Detalhes: " << std::strerror(errno) << std::endl;
    }


    if(file.is_open()) {
        std::cout << "file is open" << std::endl;
        int number;
        int count = 0;
        int nCols = 10;
        while (file >> number) {
            if (count < nCols)
            {
                shelf1.push(number);
            }else if (count < 2 * nCols)
            {
                shelf2.push(number);
            }else if (count < 3 * nCols)
            {
                shelf3.push(number);
            }else if (count < 4 * nCols)
            {
                shelf4.push(number);
            }else if (count < 5 * nCols)
            {
                shelf5.push(number);
            }else if (count < 6 * nCols)
            {
                shelf6.push(number);
            }else if (count < 7 * nCols)
            {
                gateway_commands.push(number);
            }else if (count < 8 * nCols)
            {
                gateway_target.push(number);
            }
            count++;
        }
        file.close();
        std::cout << "file is closed" << std::endl;
        server_state_table.push_back(shelf1.front());
        server_state_table.push_back(shelf2.front());
        server_state_table.push_back(shelf3.front());
        server_state_table.push_back(shelf4.front());
        server_state_table.push_back(shelf5.front());
        server_state_table.push_back(shelf6.front());
        for(uint i = 0; i < server_state_table.size(); i++){
            sensor_state_vector.push_back(server_state_table[i]);
        }
    } else {
        std::cout << "Unable to open the file." << std::endl;
        return 1; // Return an error code
    }
    return 0;
}

typedef struct{
    uint8_t source;
    uint8_t dest;
    uint8_t command;
    uint8_t payload;
} messageData;

void gatewayEvent(Ptr<Socket> skt, Ipv4Address dest){
    uint8_t* buffer = (uint8_t*)malloc(sizeof(messageData));
    buffer[0] = 13; // Gateway
    buffer[1] = 10; // Server
    buffer[2] = gateway_commands.front(); // Primeiro elemento da fila das leituras de gateway[0], iindicando o comando a ser executado
    gateway_commands.pop();
    buffer[3] = gateway_target.front(); // Primeiro elemento da fila das leituras de gateway[1], indicando qual sensor é o alvo do comando
    gateway_target.pop();
    Ptr<Packet> pkt = Create<Packet>(buffer, sizeof(messageData));
    skt->SendTo(pkt, 0, InetSocketAddress(dest, 5500));
}

void verify(Ptr<Socket> skt, Ipv4Address dest){
    uint8_t* buffer = (uint8_t*)malloc(sizeof(messageData));
    buffer[0] = 10; // Server
    buffer[1] = 0; // Broadcast
    buffer[2] = 0; // Verifica status dos sensores
    buffer[3] = 0; // não importa, fica em 0
    Ptr<Packet> pkt = Create<Packet>(buffer, sizeof(messageData));
    skt->SendTo(pkt, 0, InetSocketAddress(dest, 5500));
    std::cout << InetSocketAddress(dest, 5500) <<std::endl;
}

int main(){

    loadFile();
    LogComponentEnable("main", LOG_LEVEL_ALL);
    NodeContainer sensorNodes;
    sensorNodes.Create(6);

    NodeContainer intermediateNodes;
    intermediateNodes.Create(2);

    NodeContainer serverNode;
    serverNode.Create(1);

    NodeContainer gatewayNode;
    gatewayNode.Create(1);

    //Create WIFI helper
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("VhtMcs9"),
                                 "ControlMode", StringValue("VhtMcs0"));
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue(20));

    //Create WIFI helpers for layers 1 and 2
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    //Create WIFI helpers for MAC addressing
    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

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
    
        zCoord = shelfGroup%2==0 ? 0.0 : zCoord + 10.0 ;
        yCoord = shelfGroup%2==0 ? yCoord : yCoord - 25.0;
        shelfGroup = shelfGroup%2==0 ? 1 : shelfGroup + 1 ;
    }


    //Install Internet Stacks on each node
    InternetStackHelper stack;
    stack.Install(serverNode);
    stack.Install(intermediateNodes);
    stack.Install(gatewayNode);
    stack.Install(sensorNodes);

    //Create an Address Helper

    Ipv4AddressHelper address;
    address.SetBase(AP_ADDRESS, "255.255.255.0");
    
    Ipv4InterfaceContainer sensorInterfaces = address.Assign(sensorDevices);
    Ipv4InterfaceContainer intermediateInterfaces = address.Assign(intermediateDevices);
    Ipv4InterfaceContainer serverInterface = address.Assign(serverDevice);
    Ipv4InterfaceContainer gatewayInterface = address.Assign(gatewayDevice);

    //ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Aplicação
    std::cout << "\n--------Aplicação--------\n" <<std::endl;
    uint32_t port = 5500;
    // Lógica para envio de dados
    uint8_t* buffer = (uint8_t*)malloc(sizeof(messageData));
    buffer[0] = 10;
    buffer[1] = 11;
    buffer[2] = 0;
    buffer[3] = 0;

    ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet>(buffer, sizeof(messageData)); // Tamanho do pacote em bytes
    Ptr<Socket> sensorSocket[sensorNodes.GetN()];

    for (uint32_t i = 0; i < sensorNodes.GetN(); i++){
        sensorSocket[i] = Socket::CreateSocket(sensorNodes.Get(i), TypeId::LookupByName("ns3::UdpSocketFactory"));
        sensorSocket[i]->Bind(InetSocketAddress(sensorInterfaces.GetAddress(i), port));
    }

    // Server socket application
    Ptr<Socket> serverSocket = Socket::CreateSocket(serverNode.Get(0), TypeId::LookupByName("ns3::UdpSocketFactory"));
    serverSocket->Bind(InetSocketAddress(serverInterface.GetAddress(0), port));

    // Intermediate socket applications
    Ptr<Socket> intermediateSocketG = Socket::CreateSocket(intermediateNodes.Get(0), TypeId::LookupByName("ns3::UdpSocketFactory"));
    intermediateSocketG->Bind(InetSocketAddress(intermediateInterfaces.GetAddress(0), port));

    Ptr<Socket> intermediateSocketS = Socket::CreateSocket(intermediateNodes.Get(1), TypeId::LookupByName("ns3::UdpSocketFactory"));
    intermediateSocketS->Bind(InetSocketAddress(intermediateInterfaces.GetAddress(1), port));

    Ptr<Socket> gatewaySocket = Socket::CreateSocket(gatewayNode.Get(0), TypeId::LookupByName("ns3::UdpSocketFactory"));
    gatewaySocket->Bind(InetSocketAddress(gatewayInterface.GetAddress(0), port));

    // Intermediário entre server e gateway recebe a mensagem
    intermediateSocketG->SetRecvCallback([&](Ptr<Socket> socket) {
        ns3::Ptr<ns3::Packet> packetG;
        ns3::Address from;
        std::cout << "Here" << std::endl;
        while ((packetG = socket->RecvFrom(from)))
        {
            uint32_t packetSize = packetG->GetSize();
            ns3::Ipv4Address senderAddress = ns3::InetSocketAddress::ConvertFrom(from).GetIpv4();

            // Lógica para processar o pacote recebido
            // ...
            uint8_t buffer[packetSize];
            packetG->CopyData(buffer, packetSize);
            messageData* data = (messageData*)malloc(sizeof(messageData));
            data->source = buffer[0];
            data->dest = buffer[1];
            data->command = buffer[2];
            data->payload = buffer[3];

            switch (data->source)
            {
            case 10: // Veio do servidor
                // Repassa mensagem recebida do servidor para o gateway.
                intermediateSocketG->SendTo(packetG, 0, InetSocketAddress(gatewayInterface.GetAddress(0), port));
                
                break;
            case 13: // Veio do gateway
                // Repassa mensagem recebida do gateway para o servidor.
                intermediateSocketG->SendTo(packetG, 0, InetSocketAddress(serverInterface.GetAddress(0), port));

                break;
            default: // Inconsistência na mensagem
                uint8_t* errorMsg = (uint8_t*)malloc(sizeof(messageData));
                errorMsg[0] = 11; // quem manda é o intermediário entre gateway e servidor
                errorMsg[1] = data->source; // endereço de quem enviou a mensagem
                errorMsg[2] = 5;  // codigo de mensagem de erro
                errorMsg[3] = 0;  // codigo que indica que o erro foi de fonte não-suportada
                packetG = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                intermediateSocketG->SendTo(packetG, 0, InetSocketAddress(senderAddress, port)); // envia de volta para quem enviou a mensagem, indicando erro na comunicação
                break;
            }

            std::cout << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << std::endl;
            std::cout << "src: " << data->source << ", dest: " << data->dest << ", command: " << data->command << ", payload" << data->payload << std::endl;
        }
    });
    intermediateSocketG->SetRecvPktInfo(true); // Enable receiving sender address information
    // Intermediário entre sensores e server recebe a mensagem
    intermediateSocketS->SetRecvCallback([&](Ptr<Socket> socket) {
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
            uint8_t* msg = (uint8_t*)malloc(sizeof(messageData));

            if(data->source == 10){ // Veio do servidor
            // Repassa mensagem recebida do servidor para os sensores que a interessam.
                switch (data->command)
                {
                    case 0: // servidor deseja descobrir estado atual dos sensores
                        for(uint8_t i = 0; i < 6; i++){ // repassa a mensagem para cada um dos sensores solicitando seus valores atuais
                            intermediateSocketS->SendTo(packetS, 0, InetSocketAddress(sensorInterfaces.GetAddress(i), port));
                        }
                        break;
                    case 1: // servidor deseja esvaziar uma das prateleiras
                        if(data->dest > 6 || data->dest < 0){ // caso o destino esteja fora do intervalo permitido, isto é, não seja o identificador de algum sensor
                            errorMsg[0] = 12; // quem manda é o intermediário entre sensores e servidor
                            errorMsg[1] = data->source;  // endereço de quem enviou a mensagem
                            errorMsg[2] = 5;  // codigo de mensagem de erro
                            errorMsg[3] = 1;  // codigo que indica que o erro foi de destino inválido
                            packetS = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                            intermediateSocketS->SendTo(packetS, 0, InetSocketAddress(senderAddress, port)); // envia de volta para quem enviou a mensagem, indicando erro na comunicação
                        }else{
                            intermediateSocketS->SendTo(packetS, 0, InetSocketAddress(sensorInterfaces.GetAddress(data->dest - 1), port)); // repassa a mensagem para o sensor a ser esvaziado
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
                            intermediateSocketS->SendTo(packetS, 0, InetSocketAddress(senderAddress, port)); // envia de volta para quem enviou a mensagem, indicando erro na comunicação
                            
                            break;
                        }
                        msg[0] = data->source; // A fonte da mensagem permanece inalterada
                        msg[1] = data->dest;  // endereço da prateleira que deve ser preenchida
                        msg[2] = data->command;  // codigo de mensagem de preenchimento de prateleira
                        msg[3] = 0;  // não importa, deixo em 0.
                        packetS = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                        intermediateSocketS->SendTo(packetS, 0, InetSocketAddress(sensorInterfaces.GetAddress(data->dest - 1), port)); // repassa a mensagem para o sensor a ser preenchido

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
                        intermediateSocketS->SendTo(packetS, 0, InetSocketAddress(senderAddress, port)); // envia de volta para quem enviou a mensagem, indicando erro na comunicação

                        break;
                }
            } else {
                if(data->source > 0 && data->source <= 6){ // é algum dos sensores
                    intermediateSocketS->SendTo(packetS, 0, InetSocketAddress(serverInterface.GetAddress(0), port)); // repassa a mensagem para o servidor
                } else { // Inconsistência na mensagem
                       // uint8_t* errorMsg = (uint8_t*)malloc(sizeof(messageData));
                        errorMsg[0] = 12; // quem manda é o intermediário entre sensores e servidor
                        errorMsg[1] = data->source; // endereço de quem enviou a mensagem
                        errorMsg[2] = 5;  // codigo de mensagem de erro
                        errorMsg[3] = 0;  // codigo que indica que o erro foi de fonte inválida
                        packetS = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                        intermediateSocketS->SendTo(packetS, 0, InetSocketAddress(senderAddress, port)); // envia de volta para quem enviou a mensagem, indicando erro na comunicação
                    }

                }

            std::cout << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << std::endl;
            std::cout << "src: " << data->source << ", dest: " << data->dest << ", command: " << data->command << ", payload" << data->payload << std::endl;
        }
    });
    intermediateSocketS->SetRecvPktInfo(true); // Enable receiving sender address information

    serverSocket->SetRecvCallback([&](Ptr<Socket> socket){
        ns3::Ptr<ns3::Packet> packetServer;
        ns3::Address from;
        std::cout << "Here" << std::endl;
        while ((packetServer = socket->RecvFrom(from)))
        {
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

            if(data->source == 13){ // Fonte é o gateway

                uint8_t* errorMsg = (uint8_t*)malloc(sizeof(messageData));
                uint8_t* msg = (uint8_t*)malloc(sizeof(messageData));
                switch (data->command)
                {
                case 0: // Comando recebido é não fazer nada.
                    break;
                case 1: // Comando recebido é esvaziar uma prateleira
                    if(data->payload > 6 || data->payload < 0){ // caso o payload esteja fora do intervalo permitido, isto é, não seja o identificador de alguma prateleira
                        errorMsg[0] = 10; // quem manda é o servidor
                        errorMsg[1] = 13; // endereço de gateway
                        errorMsg[2] = 5;  // codigo de mensagem de erro
                        errorMsg[3] = 1;  // codigo que indica que o erro foi de destino inválido
                        packetServer = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                        serverSocket->SendTo(packetServer, 0, InetSocketAddress(senderAddress, port)); // envia de volta para o nó intermediário entre servidor e gateway, indicando que sua solicitação foi inválida

                    } else { // Caso o payload esteja no intervalo permitido
                        bool current_state = server_state_table[data->payload - 1];
                        if(current_state){ // Se a prateleira estiver cheia, mensagem alcançou seu destino, agora o servidor deve atualizar sua tabela de estados e solicitar a mudança no estado da prateleira
                            server_state_table[data->payload - 1] = false; // Atualiza a tabela do servidor para o novo estado da prateleira a ser esvaziada
                            msg[0] = 10; // A nova mensagem tem como fonte o servidor
                            msg[1] = data->payload;  // endereço da prateleira que deve ser esvaziada
                            msg[2] = data->command;  // codigo de mensagem de esvaziamento de prateleira
                            msg[3] = 0;  // não importa, deixo em 0.
                            packetServer = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                            serverSocket->SendTo(packetServer, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensor

                        } else { // Se a prateleira estiver vazia, envia mensagem de erro, indicando que a solicitação do gateway é inválida
                            errorMsg[0] = 10; // quem manda é o servidor
                            errorMsg[1] = 13; // endereço de gateway
                            errorMsg[2] = 5;  // codigo de mensagem de erro
                            errorMsg[3] = 3;  // codigo que indica que o erro foi de tentativa de esvaziamento de prateleira vazia
                            packetServer = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                            serverSocket->SendTo(packetServer, 0, InetSocketAddress(senderAddress, port)); // envia de volta parao nó intermediário entre servidor e gateway, indicando que sua solicitação foi inválida
                        }
                    }
                    break;
                case 2: // Comando recebido é preencher uma prateleira
                    if(data->payload > 6 || data->payload < 0){ // caso o payload esteja fora do intervalo permitido, isto é, não seja o identificador de alguma prateleira
                        errorMsg[0] = 10; // quem manda é o servidor
                        errorMsg[1] = 13; // endereço de gateway
                        errorMsg[2] = 5;  // codigo de mensagem de erro
                        errorMsg[3] = 1;  // codigo que indica que o erro foi de destino inválido
                        packetServer = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                        serverSocket->SendTo(packetServer, 0, InetSocketAddress(senderAddress, port)); // envia de volta para o nó intermediário entre servidor e gateway, indicando que sua solicitação foi inválida

                    } else { // Caso o payload esteja no intervalo permitido
                        bool current_state = server_state_table[data->payload - 1];
                        if(!current_state){ // Se a prateleira estiver vazia, mensagem alcançou seu destino, agora o servidor deve atualizar sua tabela de estados e solicitar a mudança no estado da prateleira
                            server_state_table[data->payload - 1] = true; // Atualiza a tabela do servidor para o novo estado da prateleira a ser preenchida
                            msg[0] = 10; // A nova mensagem tem como fonte o servidor
                            msg[1] = data->payload;  // endereço da prateleira que deve ser preenchida
                            msg[2] = data->command;  // codigo de mensagem de preenchimento de prateleira
                            msg[3] = 0;  // não importa, deixo em 0.
                            packetServer = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                            serverSocket->SendTo(packetServer, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores

                        } else { // Se a prateleira estiver cheia, envia mensagem de erro, indicando que a solicitação do gateway é inválida
                            errorMsg[0] = 10; // quem manda é o servidor
                            errorMsg[1] = 13; // endereço de gateway
                            errorMsg[2] = 5;  // codigo de mensagem de erro
                            errorMsg[3] = 4;  // codigo que indica que o erro foi de tentativa de preenchimento de prateleira cheia
                            packetServer = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                            serverSocket->SendTo(packetServer, 0, InetSocketAddress(senderAddress, port)); // envia de volta para o nó intermediário entre servidor e gateway, indicando que sua solicitação foi inválida
                        }
                    }

                    break;
                default:
                    NS_LOG_INFO("Comando inválido");
                    break;
                }
            } else if(data->source > 0 && data->source <= 6){ // Fonte é um dos sensores
                if(data->payload != server_state_table[data->source - 1]){ // Caso ocorra inconsistência entre a tabela do servidor e os dados enviados pelo sensor
                    server_state_table[data->source - 1] = !server_state_table[data->source - 1]; // Atualiza a tabela do servidor
                    NS_LOG_INFO("Discrepância entre leitura esperada e real dos sensores, enviando mensagem de erro para Gateway.");
                    uint8_t* errorMsg = (uint8_t*)malloc(sizeof(messageData));
                            errorMsg[0] = 10; // quem manda é o servidor
                            errorMsg[1] = 13; // endereço de gateway
                            errorMsg[2] = 5;  // codigo de mensagem de erro
                            errorMsg[3] = 5;  // codigo que indica que o erro foi de tentativa de inconsistência de valores.
                            packetServer = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                            serverSocket->SendTo(packetServer, 0, InetSocketAddress(intermediateInterfaces.GetAddress(0), port)); // envia de volta para o nó intermediário entre servidor e gateway, indicando que ocorreu erro
                }else{ // Caso o payload seja consistente com a tabela do servidor
                    uint8_t* msg = (uint8_t*)malloc(sizeof(messageData));
                    if(data->command == 1 || data->command == 2){ // Se o comando vier do gateway(1 ou 2), responde a ele com mensagem de sucesso.
                        msg[0] = 10; // A nova mensagem tem como fonte o servidor
                        msg[1] = data->payload;  // endereço da prateleira que deve ser preenchida
                        msg[2] = data->command;  // codigo de mensagem de preenchimento de prateleira
                        msg[3] = 0;  // não importa, deixo em 0.
                        packetServer = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                        serverSocket->SendTo(packetServer, 0, InetSocketAddress(intermediateInterfaces.GetAddress(0), port)); // repassa a mensagem de sucesso para o nó intermediário entre servidor e gateway
                    }else{ // caso o comando seja 0(verificar estado dos sensores), não é necessário mandar nenhuma mensagem, visto que ele partiu do proprio servidor
                        NS_LOG_INFO("Verificação do estado dos sensores concluida com sucesso.");
                    }
                }

            } else if(data->source == 11 || data->source == 12){ // Fonte é um dos nós intermediários, indicando que houve erro
                NS_LOG_INFO("Erro No envio para Nó intermediário. Dados inválidos ou corrompidos.");

            } else { // Fonte inválida
                NS_LOG_INFO("Identificador Fonte Inválido, finalizando comunicação.");
            }

            std::cout << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << std::endl;
            std::cout << "src: " << data->source << ", dest: " << data->dest << ", command: " << data->command << ", payload" << data->payload << std::endl;
        }
    });
    serverSocket->SetRecvPktInfo(true); // Enable receiving sender address information

    sensorSocket[0]->SetRecvCallback([&](Ptr<Socket>socket){
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

            switch (data->command)
            {
            case 0: // Verificar estado dos sensores
            if(shelf1.size() > 0){ // Verifica se a fila não está vazia
                    sensor_state_vector[0] = shelf1.front(); // Lê do vetor do log das leituras do sensor da prateleira 1 o próximo valor e atualiza a tabela de sensores.
                    shelf1.pop(); // elimina o valor da fila.
                    msg[0] = 1; // A nova mensagem tem como fonte o sensor da prateleira 1
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = sensor_state_vector[0];  // Payload assume o valor da leitura do sensor
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[0]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores

                } else { // caso esteja vazia, avisa que não há mais leituras do sensor
                    NS_LOG_INFO("Log esvaziado. Não Há mais leituras do sensor da prateleira 1.");
                }
                
                break;
            case 1: // Esvaziar prateleria
            if(shelf1.size() > 0){ // Verifica se a fila não está vazia
                    sensor_state_vector[0] = false; //Esvazia a prateleira.
                    msg[0] = 1; // A nova mensagem tem como fonte o sensor da prateleira 1
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = false;  // Payload assume o valor 0, indicando que a prateleira foi esvaziada
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[0]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores

                } else { // caso esteja vazia, avisa que não há mais leituras do sensor
                    NS_LOG_INFO("Log esvaziado. Não Há mais leituras do sensor da prateleira 1.");
                }

                break;
            case 2: // Preencher prateleira
                    sensor_state_vector[0] = true; //Preenche a prateleira.
                    msg[0] = 1; // A nova mensagem tem como fonte o sensor da prateleira 1
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = true;  // Payload assume o valor 0, indicando que a prateleira foi preenchida
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[0]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores
                break;
            default: // Inconsistência na mensagem(Não deve entrar aqui)
                NS_LOG_INFO("O sensor não consegue processar o comando enviado.");
                break;
            }

            std::cout << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << std::endl;
            std::cout << "src: " << data->source << ", dest: " << data->dest << ", command: " << data->command << ", payload" << data->payload << std::endl;
        }
    });
    sensorSocket[0]->SetRecvPktInfo(true); // Enable receiving sender address information

    sensorSocket[1]->SetRecvCallback([&](Ptr<Socket>socket){
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

            switch (data->command)
            {
            case 0: // Verificar estado dos sensores
            if(shelf2.size() > 0){ // Verifica se a fila não está vazia
                    sensor_state_vector[0] = shelf2.front(); // Lê do vetor do log das leituras do sensor da prateleira 1 o próximo valor e atualiza a tabela de sensores.
                    shelf2.pop(); // elimina o valor da fila.
                    msg[0] = 2; // A nova mensagem tem como fonte o sensor da prateleira 2
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = sensor_state_vector[1];  // Payload assume o valor da leitura do sensor
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[1]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores

                } else { // caso esteja vazia, avisa que não há mais leituras do sensor
                    NS_LOG_INFO("Log esvaziado. Não Há mais leituras do sensor da prateleira 2.");
                }
                
                break;
            case 1: // Esvaziar prateleria
            if(shelf2.size() > 0){ // Verifica se a fila não está vazia
                    sensor_state_vector[1] = false; //Esvazia a prateleira.
                    msg[0] = 2; // A nova mensagem tem como fonte o sensor da prateleira 2
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = false;  // Payload assume o valor 0, indicando que a prateleira foi esvaziada
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[1]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores

                } else { // caso esteja vazia, avisa que não há mais leituras do sensor
                    NS_LOG_INFO("Log esvaziado. Não Há mais leituras do sensor da prateleira 2.");
                }

                break;
            case 2: // Preencher prateleira
                if(shelf2.size() > 0){
                    sensor_state_vector[1] = true; //Preenche a prateleira.
                    msg[0] = 2; // A nova mensagem tem como fonte o sensor da prateleira 2
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = true;  // Payload assume o valor 0, indicando que a prateleira foi preenchida
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[1]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores
                } else { // caso esteja vazia, avisa que não há mais leituras do sensor
                    NS_LOG_INFO("Log esvaziado. Não Há mais leituras do sensor da prateleira 2.");
                }
                break;
            default: // Inconsistência na mensagem(Não deve entrar aqui)
                NS_LOG_INFO("O sensor não consegue processar o comando enviado.");
                break;
            }

            std::cout << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << std::endl;
            std::cout << "src: " << data->source << ", dest: " << data->dest << ", command: " << data->command << ", payload" << data->payload << std::endl;
        }
    });
    sensorSocket[1]->SetRecvPktInfo(true); // Enable receiving sender address information

    sensorSocket[2]->SetRecvCallback([&](Ptr<Socket>socket){
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

            switch (data->command)
            {
            case 0: // Verificar estado dos sensores
            if(shelf3.size() > 0){ // Verifica se a fila não está vazia
                    sensor_state_vector[2] = shelf3.front(); // Lê do vetor do log das leituras do sensor da prateleira 1 o próximo valor e atualiza a tabela de sensores.
                    shelf3.pop(); // elimina o valor da fila.
                    msg[0] = 3; // A nova mensagem tem como fonte o sensor da prateleira 3
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = sensor_state_vector[2];  // Payload assume o valor da leitura do sensor
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[1]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores

                } else { // caso esteja vazia, avisa que não há mais leituras do sensor
                    NS_LOG_INFO("Log esvaziado. Não Há mais leituras do sensor da prateleira 3.");
                }
                
                break;
            case 1: // Esvaziar prateleria
            if(shelf3.size() > 0){ // Verifica se a fila não está vazia
                    sensor_state_vector[2] = false; //Esvazia a prateleira.
                    msg[0] = 3; // A nova mensagem tem como fonte o sensor da prateleira 3
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = false;  // Payload assume o valor 0, indicando que a prateleira foi esvaziada
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[1]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores

                } else { // caso esteja vazia, avisa que não há mais leituras do sensor
                    NS_LOG_INFO("Log esvaziado. Não Há mais leituras do sensor da prateleira 1.");
                }

                break;
            case 2: // Preencher prateleira
            if(shelf3.size() > 0){// Verifica se a fila não está vazia
                    sensor_state_vector[2] = true; //Preenche a prateleira.
                    msg[0] = 3; // A nova mensagem tem como fonte o sensor da prateleira 3
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = true;  // Payload assume o valor 0, indicando que a prateleira foi preenchida
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[1]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores
                }
                break;
            default: // Inconsistência na mensagem(Não deve entrar aqui)
                NS_LOG_INFO("O sensor não consegue processar o comando enviado.");
                break;
            }

            std::cout << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << std::endl;
            std::cout << "src: " << data->source << ", dest: " << data->dest << ", command: " << data->command << ", payload" << data->payload << std::endl;
        }
    });
    sensorSocket[2]->SetRecvPktInfo(true); // Enable receiving sender address information

    sensorSocket[3]->SetRecvCallback([&](Ptr<Socket>socket){
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

            switch (data->command)
            {
            case 0: // Verificar estado dos sensores
            if(shelf4.size() > 0){ // Verifica se a fila não está vazia
                    sensor_state_vector[3] = shelf4.front(); // Lê do vetor do log das leituras do sensor da prateleira 4 o próximo valor e atualiza a tabela de sensores.
                    shelf4.pop(); // elimina o valor da fila.
                    msg[0] = 4; // A nova mensagem tem como fonte o sensor da prateleira 4
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = sensor_state_vector[3];  // Payload assume o valor da leitura do sensor
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[1]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores

                } else { // caso esteja vazia, avisa que não há mais leituras do sensor
                    NS_LOG_INFO("Log esvaziado. Não Há mais leituras do sensor da prateleira 4.");
                }
                
                break;
            case 1: // Esvaziar prateleria
            if(shelf4.size() > 0){ // Verifica se a fila não está vazia
                    sensor_state_vector[3] = false; //Esvazia a prateleira.
                    msg[0] = 4; // A nova mensagem tem como fonte o sensor da prateleira 4
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = false;  // Payload assume o valor 0, indicando que a prateleira foi esvaziada
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[1]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores

                } else { // caso esteja vazia, avisa que não há mais leituras do sensor
                    NS_LOG_INFO("Log esvaziado. Não Há mais leituras do sensor da prateleira 4.");
                }

                break;
            case 2: // Preencher prateleira
            if(shelf4.size() > 0){// Verifica se a fila não está vazia
                    sensor_state_vector[3] = true; //Preenche a prateleira.
                    msg[0] = 4; // A nova mensagem tem como fonte o sensor da prateleira 4
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = true;  // Payload assume o valor 0, indicando que a prateleira foi preenchida
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[1]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores
                }
                break;
            default: // Inconsistência na mensagem(Não deve entrar aqui)
                NS_LOG_INFO("O sensor não consegue processar o comando enviado.");
                break;
            }

            std::cout << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << std::endl;
            std::cout << "src: " << data->source << ", dest: " << data->dest << ", command: " << data->command << ", payload" << data->payload << std::endl;
        }
    });
    sensorSocket[3]->SetRecvPktInfo(true); // Enable receiving sender address information

    sensorSocket[4]->SetRecvCallback([&](Ptr<Socket>socket){
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

            switch (data->command)
            {
            case 0: // Verificar estado dos sensores
            if(shelf5.size() > 0){ // Verifica se a fila não está vazia
                    sensor_state_vector[4] = shelf5.front(); // Lê do vetor do log das leituras do sensor da prateleira 5 o próximo valor e atualiza a tabela de sensores.
                    shelf5.pop(); // elimina o valor da fila.
                    msg[0] = 5; // A nova mensagem tem como fonte o sensor da prateleira 5
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = sensor_state_vector[4];  // Payload assume o valor da leitura do sensor
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[1]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores

                } else { // caso esteja vazia, avisa que não há mais leituras do sensor
                    NS_LOG_INFO("Log esvaziado. Não Há mais leituras do sensor da prateleira 5.");
                }
                
                break;
            case 1: // Esvaziar prateleria
            if(shelf5.size() > 0){ // Verifica se a fila não está vazia
                    sensor_state_vector[4] = false; //Esvazia a prateleira.
                    msg[0] = 5; // A nova mensagem tem como fonte o sensor da prateleira 5
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = false;  // Payload assume o valor 0, indicando que a prateleira foi esvaziada
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[1]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores

                } else { // caso esteja vazia, avisa que não há mais leituras do sensor
                    NS_LOG_INFO("Log esvaziado. Não Há mais leituras do sensor da prateleira 5.");
                }

                break;
            case 2: // Preencher prateleira
            if(shelf5.size() > 0){// Verifica se a fila não está vazia
                    sensor_state_vector[4] = true; //Preenche a prateleira.
                    msg[0] = 5; // A nova mensagem tem como fonte o sensor da prateleira 5
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = true;  // Payload assume o valor 0, indicando que a prateleira foi preenchida
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[1]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores
                }
                break;
            default: // Inconsistência na mensagem(Não deve entrar aqui)
                NS_LOG_INFO("O sensor não consegue processar o comando enviado.");
                break;
            }

            std::cout << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << std::endl;
            std::cout << "src: " << data->source << ", dest: " << data->dest << ", command: " << data->command << ", payload" << data->payload << std::endl;
        }
    });
    sensorSocket[4]->SetRecvPktInfo(true); // Enable receiving sender address information

    sensorSocket[4]->SetRecvCallback([&](Ptr<Socket>socket){
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

            switch (data->command)
            {
            case 0: // Verificar estado dos sensores
            if(shelf5.size() > 0){ // Verifica se a fila não está vazia
                    sensor_state_vector[5] = shelf6.front(); // Lê do vetor do log das leituras do sensor da prateleira 6 o próximo valor e atualiza a tabela de sensores.
                    shelf6.pop(); // elimina o valor da fila.
                    msg[0] = 5; // A nova mensagem tem como fonte o sensor da prateleira 6
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = sensor_state_vector[5];  // Payload assume o valor da leitura do sensor
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[1]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores

                } else { // caso esteja vazia, avisa que não há mais leituras do sensor
                    NS_LOG_INFO("Log esvaziado. Não Há mais leituras do sensor da prateleira 6.");
                }
                
                break;
            case 1: // Esvaziar prateleria
            if(shelf6.size() > 0){ // Verifica se a fila não está vazia
                    sensor_state_vector[5] = false; //Esvazia a prateleira.
                    msg[0] = 6; // A nova mensagem tem como fonte o sensor da prateleira 6
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = false;  // Payload assume o valor 0, indicando que a prateleira foi esvaziada
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[1]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores

                } else { // caso esteja vazia, avisa que não há mais leituras do sensor
                    NS_LOG_INFO("Log esvaziado. Não Há mais leituras do sensor da prateleira 6.");
                }

                break;
            case 2: // Preencher prateleira
            if(shelf6.size() > 0){// Verifica se a fila não está vazia
                    sensor_state_vector[5] = true; //Preenche a prateleira.
                    msg[0] = 6; // A nova mensagem tem como fonte o sensor da prateleira 6
                    msg[1] = 10;  // Identificador do servidor
                    msg[2] = data->command;  // codigo de mensagem de verificação de estado da prateleira
                    msg[3] = true;  // Payload assume o valor 0, indicando que a prateleira foi preenchida
                    packetSensor = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                    sensorSocket[1]->SendTo(packetSensor, 0, InetSocketAddress(intermediateInterfaces.GetAddress(1), port)); // repassa a mensagem para o nó intermediário entre servidor e sensores
                }
                break;
            default: // Inconsistência na mensagem(Não deve entrar aqui)
                NS_LOG_INFO("O sensor não consegue processar o comando enviado.");
                break;
            }

            std::cout << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << std::endl;
            std::cout << "src: " << data->source << ", dest: " << data->dest << ", command: " << data->command << ", payload" << data->payload << std::endl;
        }
    });
    sensorSocket[5]->SetRecvPktInfo(true); // Enable receiving sender address information

    gatewaySocket->SetRecvCallback([&](Ptr<Socket> socket){
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

            switch (data->command)
            {
            case 1: // Esvaziamento finalizado com sucesso
                NS_LOG_INFO("Produto dispachado com sucesso");
                break;
            case 2: // Preenchimendo finalizado com sucesso
                NS_LOG_INFO("Produto armazenado com sucesso");
                break;
            case 5: // erro
                if(data->payload == 5)
                    NS_LOG_INFO("Inconsistência de valores, alertando central");
                else if(data->payload == 4)
                    NS_LOG_INFO("Tentativa de armazenar produto em prateleira ocupada");
                else if(data->payload == 3)
                    NS_LOG_INFO("Tentativa de retirar produto de prateleira vazia");

            default:
                break;
            }

            std::cout << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << std::endl;
            std::cout << "src: " << data->source << ", dest: " << data->dest << ", command: " << data->command << ", payload" << data->payload << std::endl;
        }
    });
    for(uint8_t i = 0; i < 10; i++){
        Simulator::Schedule(Seconds(i + 0.5), &gatewayEvent, gatewaySocket, intermediateInterfaces.GetAddress(0));
        Simulator::Schedule(Seconds(i + 1.0), &verify, serverSocket, intermediateInterfaces.GetAddress(1));
    }

    Simulator::Stop(Seconds(11.0));
    ns3::Simulator::Run();
    ns3::Simulator::Destroy();

    return 0;
}