#include "ns3/log.h"
#include "server-app.h"
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
    NS_LOG_COMPONENT_DEFINE("ServerApp");
    NS_OBJECT_ENSURE_REGISTERED(ServerApp);

    TypeId
    ServerApp::GetTypeId()
    {
        static TypeId tid = TypeId("ns3::ServerApp")
                                .AddConstructor<ServerApp>()
                                .SetParent<Application>();
        return tid;
    }

    TypeId
    ServerApp::GetInstanceTypeId() const
    {
        return ServerApp::GetTypeId();
    }

    ServerApp::ServerApp()
    {
        id = 0;
        addrITS = Ipv4Address::GetAny();
        addrIGS = Ipv4Address::GetAny();
        receiver_socket = nullptr;
        sender_socket = nullptr;
    }
    ServerApp::ServerApp(Ipv4Address addr1, Ipv4Address addr2, Ipv4Address addr3, LibRedes handler)
    {
        id = 10;
        m_addr = addr1;
        addrIGS = addr2;
        addrITS = addr3;
        shelves = handler.shelves;
        gateway_commands = handler.gateway_commands;
        gateway_target = handler.gateway_target;
        state_table.clear();
        for(uint8_t i = 0; i < 6; i++){
            state_table.push_back(handler.shelves[i].front());
        }
        receiver_socket = nullptr;
        sender_socket = nullptr;
    }
    ServerApp::~ServerApp()
    {
    }
    void ServerApp::SetupReceiveSocket(Ptr<Socket> socket, Ipv4Address addr)
    { 
        InetSocketAddress local = InetSocketAddress(addr, PORT);
        if (socket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        } 
    }
    void ServerApp::StartApplication()
    {
        //Receiver socket
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        receiver_socket = Socket::CreateSocket(GetNode(), tid);
        SetupReceiveSocket(receiver_socket, m_addr);
        
        std::cout << "Server node, addr: " << m_addr << std::endl;

        receiver_socket->SetRecvCallback(MakeCallback(&ServerApp::ServerCallback, this));
        receiver_socket->SetRecvPktInfo(true);
        receiver_socket->SetAllowBroadcast(true);

        //Sender Socket
        sender_socket = Socket::CreateSocket(GetNode(), tid);
        if(sender_socket->Bind(InetSocketAddress(m_addr, PORT)) == -1)
            NS_FATAL_ERROR("Failed to bind socket");

        sender_socket->SetRecvPktInfo(true);
        sender_socket->SetAllowBroadcast(true);
    }
    
    void ServerApp::ServerCallback(Ptr<Socket> socket) {        
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
            
            NS_LOG_INFO(GREEN_CODE << "Recebido pacote de " << senderAddress << ", tamanho: " << packetSize << " bytes" << END_CODE);
            NS_LOG_INFO(GREEN_CODE << "src: " << data->source << ", dest: " << data->dest << ", command: " << data->command << ", payload" << data->payload << END_CODE);

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
                        SendPacket(packetServer, senderAddress, PORT);
                        //sender_socket->Connect(InetSocketAddress(senderAddress, PORT));
                        //sender_socket->Send(packetServer); // envia de volta para o nó intermediário entre servidor e gateway, indicando que sua solicitação foi inválida

                    } else { // Caso o payload esteja no intervalo permitido
                        bool current_state = state_table[data->payload - 1];
                        if(current_state){ // Se a prateleira estiver cheia, mensagem alcançou seu destino, agora o servidor deve atualizar sua tabela de estados e solicitar a mudança no estado da prateleira
                            state_table[data->payload - 1] = false; // Atualiza a tabela do servidor para o novo estado da prateleira a ser esvaziada
                            msg[0] = 10; // A nova mensagem tem como fonte o servidor
                            msg[1] = data->payload;  // endereço da prateleira que deve ser esvaziada
                            msg[2] = data->command;  // codigo de mensagem de esvaziamento de prateleira
                            msg[3] = 0;  // não importa, deixo em 0.
                            packetServer = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                            SendPacket(packetServer, addrITS, PORT);
                            // sender_socket->Connect(InetSocketAddress(addrITS, PORT));
                            // sender_socket->Send(packetServer); // repassa a mensagem para o nó intermediário entre servidor e sensor

                        } else { // Se a prateleira estiver vazia, envia mensagem de erro, indicando que a solicitação do gateway é inválida
                            errorMsg[0] = 10; // quem manda é o servidor
                            errorMsg[1] = 13; // endereço de gateway
                            errorMsg[2] = 5;  // codigo de mensagem de erro
                            errorMsg[3] = 3;  // codigo que indica que o erro foi de tentativa de esvaziamento de prateleira vazia
                            packetServer = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                            SendPacket(packetServer, Ipv4Address::GetBroadcast(), PORT); // Envia o erro por broadcast
                            //sender_socket->Connect(InetSocketAddress(senderAddress, PORT));
                            //sender_socket->Send(packetServer); // envia de volta parao nó intermediário entre servidor e gateway, indicando que sua solicitação foi inválida
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
                        SendPacket(packetServer, senderAddress, PORT);
                        //sender_socket->Connect(InetSocketAddress(senderAddress, PORT));
                        //sender_socket->Send(packetServer); // envia de volta para o nó intermediário entre servidor e gateway, indicando que sua solicitação foi inválida

                    } else { // Caso o payload esteja no intervalo permitido
                        bool current_state = state_table[data->payload - 1];
                        if(!current_state){ // Se a prateleira estiver vazia, mensagem alcançou seu destino, agora o servidor deve atualizar sua tabela de estados e solicitar a mudança no estado da prateleira
                            state_table[data->payload - 1] = true; // Atualiza a tabela do servidor para o novo estado da prateleira a ser preenchida
                            msg[0] = 10; // A nova mensagem tem como fonte o servidor
                            msg[1] = data->payload;  // endereço da prateleira que deve ser preenchida
                            msg[2] = data->command;  // codigo de mensagem de preenchimento de prateleira
                            msg[3] = 0;  // não importa, deixo em 0.
                            packetServer = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                            SendPacket(packetServer, addrITS, PORT);
                            //sender_socket->Connect(InetSocketAddress(addrITS, PORT));
                            //sender_socket->Send(packetServer);  // repassa a mensagem para o nó intermediário entre servidor e sensores

                        } else { // Se a prateleira estiver cheia, envia mensagem de erro, indicando que a solicitação do gateway é inválida
                            errorMsg[0] = 10; // quem manda é o servidor
                            errorMsg[1] = 13; // endereço de gateway
                            errorMsg[2] = 5;  // codigo de mensagem de erro
                            errorMsg[3] = 4;  // codigo que indica que o erro foi de tentativa de preenchimento de prateleira cheia
                            packetServer = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                            SendPacket(packetServer, Ipv4Address::GetBroadcast(), PORT); // Envia por Boradcast a mensgem de erro
                            // sender_socket->Connect(InetSocketAddress(senderAddress, PORT));
                            // sender_socket->Send(packetServer); // envia de volta para o nó intermediário entre servidor e gateway, indicando que sua solicitação foi inválida
                        }
                    }

                    break;
                default:
                    NS_LOG_INFO("Comando inválido");
                    break;
                }
            } else if(data->source > 0 && data->source <= 6){ // Fonte é um dos sensores
                if(data->payload != state_table[data->source - 1]){ // Caso ocorra inconsistência entre a tabela do servidor e os dados enviados pelo sensor
                    state_table[data->source - 1] = !state_table[data->source - 1]; // Atualiza a tabela do servidor
                    NS_LOG_INFO("Discrepância entre leitura esperada e real dos sensores, enviando mensagem de erro para Gateway.");
                    uint8_t* errorMsg = (uint8_t*)malloc(sizeof(messageData));
                            errorMsg[0] = 10; // quem manda é o servidor
                            errorMsg[1] = 13; // endereço de gateway
                            errorMsg[2] = 5;  // codigo de mensagem de erro
                            errorMsg[3] = 5;  // codigo que indica que o erro foi de tentativa de inconsistência de valores.
                            packetServer = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                            SendPacket(packetServer, addrIGS, PORT);
                            //sender_socket->Connect(InetSocketAddress(addrIGS, PORT));
                            //sender_socket->Send(packetServer); // envia de volta para o nó intermediário entre servidor e gateway, indicando que ocorreu erro
                }else{ // Caso o payload seja consistente com a tabela do servidor
                    uint8_t* msg = (uint8_t*)malloc(sizeof(messageData));
                    if(data->command == 1 || data->command == 2){ // Se o comando vier do gateway(1 ou 2), responde a ele com mensagem de sucesso.
                        msg[0] = 10; // A nova mensagem tem como fonte o servidor
                        msg[1] = data->payload;  // endereço da prateleira que deve ser preenchida
                        msg[2] = data->command;  // codigo de mensagem de preenchimento de prateleira
                        msg[3] = 0;  // não importa, deixo em 0.
                        packetServer = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                        SendPacket(packetServer, addrIGS, PORT);
                        // sender_socket->Connect(InetSocketAddress(addrIGS, PORT));
                        // sender_socket->Send(packetServer); // repassa a mensagem de sucesso para o nó intermediário entre servidor e gateway
                    }else{ // caso o comando seja 0(verificar estado dos sensores), não é necessário mandar nenhuma mensagem, visto que ele partiu do proprio servidor
                        NS_LOG_INFO("Verificação do estado dos sensores concluida com sucesso.");
                    }
                }

            } else if(data->source == 11 || data->source == 12){ // Fonte é um dos nós intermediários, indicando que houve erro
                NS_LOG_INFO("Erro No envio para Nó intermediário. Dados inválidos ou corrompidos.");

            } else { // Fonte inválida
                NS_LOG_INFO("Identificador Fonte Inválido, finalizando comunicação.");
            }
        }
    }
    
    void ServerApp::SendPacket(Ptr<Packet> packet, Ipv4Address destination, uint16_t port)
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