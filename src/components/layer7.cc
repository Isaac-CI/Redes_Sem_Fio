#include "ns3/log.h"
#include "layer7.h"
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

#define PORT 3000

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("Layer7");
    NS_OBJECT_ENSURE_REGISTERED(Layer7);

    TypeId
    Layer7::GetTypeId()
    {
        static TypeId tid = TypeId("ns3::Layer7")
                                .AddConstructor<Layer7>()
                                .SetParent<Application>();
        return tid;
    }

    TypeId
    Layer7::GetInstanceTypeId() const
    {
        return Layer7::GetTypeId();
    }

    Layer7::Layer7()
    {
        id = 0;
        addrGateway = Ipv4Address::GetAny();
        addrITS = Ipv4Address::GetAny();
        addrIGS = Ipv4Address::GetAny();
        addrServer = Ipv4Address::GetAny();
        for(uint8_t i = 0; i < 6; i++)
            addrSensors[i] = Ipv4Address::GetAny();
        receiver_socket = nullptr;
        sender_socket = nullptr;
    }
    Layer7::Layer7(Ipv4Address addr1, Ipv4Address addr2, Ipv4Address addr3, Ipv4Address addr4, Ipv4Address addr5,
                    Ipv4Address addr6, Ipv4Address addr7, Ipv4Address addr8, Ipv4Address addr9, Ipv4Address addr10, int Id, LibRedes handler)
    {
        id = Id;
        addrSensors[0] = addr1;
        addrSensors[1] = addr2;
        addrSensors[2] = addr3;
        addrSensors[3] = addr4;
        addrSensors[4] = addr5;
        addrSensors[5] = addr6;
        addrServer = addr7;
        addrIGS = addr8;
        addrITS = addr9;
        addrGateway = addr10;
        shelves = handler.shelves;
        if(id <= 6 && id > 0) // se for sensor
            sensor_state = shelves[id - 1].front();
        if(id == 13) { // Se for gateway
            gateway_commands = handler.gateway_commands;
            gateway_target = handler.gateway_target;
        }
        receiver_socket = nullptr;
        sender_socket = nullptr;
    }
    Layer7::Layer7(Ipv4Address addr1, Ipv4Address addr2, Ipv4Address addr3, Ipv4Address addr4, Ipv4Address addr5,
                    Ipv4Address addr6, Ipv4Address addr7, Ipv4Address addr8, Ipv4Address addr9, Ipv4Address addr10, int Id, LibRedes handler, std::vector<bool>server_table)
    {
        id = Id;
        addrSensors[0] = addr1;
        addrSensors[1] = addr2;
        addrSensors[2] = addr3;
        addrSensors[3] = addr4;
        addrSensors[4] = addr5;
        addrSensors[5] = addr6;
        addrServer = addr7;
        addrIGS = addr8;
        addrITS = addr9;
        addrGateway = addr10;
        shelves = handler.shelves;
        if(id <= 6 && id > 0) // se for sensor
            sensor_state = shelves[id - 1].front();
        if(id == 13) { // Se for gateway
            gateway_commands = handler.gateway_commands;
            gateway_target = handler.gateway_target;
        }
        if(id == 10) // se for servidor
            server_state_table = server_table;
        receiver_socket = nullptr;
        sender_socket = nullptr;
    }
    Layer7::~Layer7()
    {
    }
    void Layer7::SetupReceiveSocket(Ptr<Socket> socket, uint16_t port)
    {
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);
        if (socket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }
    void Layer7::StartApplication()
    {
        //Receiver socket
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        receiver_socket = Socket::CreateSocket(GetNode(), tid);

        SetupReceiveSocket(receiver_socket, PORT);
        if(id > 0 && id <= 6)
            receiver_socket->SetRecvCallback(MakeCallback(&Layer7::SensorCallback, this));
        else if(id == 10)
            receiver_socket->SetRecvCallback(MakeCallback(&Layer7::ServerCallback, this));
        else if(id == 11)
            receiver_socket->SetRecvCallback(MakeCallback(&Layer7::IGSCallback, this));
        else if(id == 12)
            receiver_socket->SetRecvCallback(MakeCallback(&Layer7::ITSCallback, this));
        else if(id == 13)
            receiver_socket->SetRecvCallback(MakeCallback(&Layer7::GatewayCallback, this));
        else
            NS_LOG_INFO(RED_CODE << "Erro, ID de objeto inválido. Inviável configurar corretamente o callback" << END_CODE);

        //Sender Socket
        sender_socket = Socket::CreateSocket(GetNode(), tid);
    }

    void Layer7::SensorCallback(Ptr<Socket> socket) {
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

                        sender_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(addrITS), PORT));
                        sender_socket->Send(packetSensor); // repassa a mensagem para o nó intermediário entre servidor e sensores

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

                        sender_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(addrITS), PORT));
                        sender_socket->Send(packetSensor); // repassa a mensagem para o nó intermediário entre servidor e sensores
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

                        sender_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(addrITS), PORT));
                        sender_socket->Send(packetSensor); // repassa a mensagem para o nó intermediário entre servidor e sensores
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
    
    void Layer7::ServerCallback(Ptr<Socket> socket) {        
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
                        sender_socket->Connect(InetSocketAddress(senderAddress, PORT));
                        sender_socket->Send(packetServer); // envia de volta para o nó intermediário entre servidor e gateway, indicando que sua solicitação foi inválida

                    } else { // Caso o payload esteja no intervalo permitido
                        bool current_state = server_state_table[data->payload - 1];
                        if(current_state){ // Se a prateleira estiver cheia, mensagem alcançou seu destino, agora o servidor deve atualizar sua tabela de estados e solicitar a mudança no estado da prateleira
                            server_state_table[data->payload - 1] = false; // Atualiza a tabela do servidor para o novo estado da prateleira a ser esvaziada
                            msg[0] = 10; // A nova mensagem tem como fonte o servidor
                            msg[1] = data->payload;  // endereço da prateleira que deve ser esvaziada
                            msg[2] = data->command;  // codigo de mensagem de esvaziamento de prateleira
                            msg[3] = 0;  // não importa, deixo em 0.
                            packetServer = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                            sender_socket->Connect(InetSocketAddress(addrITS, PORT));
                            sender_socket->Send(packetServer); // repassa a mensagem para o nó intermediário entre servidor e sensor

                        } else { // Se a prateleira estiver vazia, envia mensagem de erro, indicando que a solicitação do gateway é inválida
                            errorMsg[0] = 10; // quem manda é o servidor
                            errorMsg[1] = 13; // endereço de gateway
                            errorMsg[2] = 5;  // codigo de mensagem de erro
                            errorMsg[3] = 3;  // codigo que indica que o erro foi de tentativa de esvaziamento de prateleira vazia
                            packetServer = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                            sender_socket->Connect(InetSocketAddress(senderAddress, PORT));
                            sender_socket->Send(packetServer); // envia de volta parao nó intermediário entre servidor e gateway, indicando que sua solicitação foi inválida
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
                        sender_socket->Connect(InetSocketAddress(senderAddress, PORT));
                        sender_socket->Send(packetServer); // envia de volta para o nó intermediário entre servidor e gateway, indicando que sua solicitação foi inválida

                    } else { // Caso o payload esteja no intervalo permitido
                        bool current_state = server_state_table[data->payload - 1];
                        if(!current_state){ // Se a prateleira estiver vazia, mensagem alcançou seu destino, agora o servidor deve atualizar sua tabela de estados e solicitar a mudança no estado da prateleira
                            server_state_table[data->payload - 1] = true; // Atualiza a tabela do servidor para o novo estado da prateleira a ser preenchida
                            msg[0] = 10; // A nova mensagem tem como fonte o servidor
                            msg[1] = data->payload;  // endereço da prateleira que deve ser preenchida
                            msg[2] = data->command;  // codigo de mensagem de preenchimento de prateleira
                            msg[3] = 0;  // não importa, deixo em 0.
                            packetServer = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                            sender_socket->Connect(InetSocketAddress(addrITS, PORT));
                            sender_socket->Send(packetServer);  // repassa a mensagem para o nó intermediário entre servidor e sensores

                        } else { // Se a prateleira estiver cheia, envia mensagem de erro, indicando que a solicitação do gateway é inválida
                            errorMsg[0] = 10; // quem manda é o servidor
                            errorMsg[1] = 13; // endereço de gateway
                            errorMsg[2] = 5;  // codigo de mensagem de erro
                            errorMsg[3] = 4;  // codigo que indica que o erro foi de tentativa de preenchimento de prateleira cheia
                            packetServer = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                            sender_socket->Connect(InetSocketAddress(senderAddress, PORT));
                            sender_socket->Send(packetServer); // envia de volta para o nó intermediário entre servidor e gateway, indicando que sua solicitação foi inválida
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
                            sender_socket->Connect(InetSocketAddress(addrIGS, PORT));
                            sender_socket->Send(packetServer); // envia de volta para o nó intermediário entre servidor e gateway, indicando que ocorreu erro
                }else{ // Caso o payload seja consistente com a tabela do servidor
                    uint8_t* msg = (uint8_t*)malloc(sizeof(messageData));
                    if(data->command == 1 || data->command == 2){ // Se o comando vier do gateway(1 ou 2), responde a ele com mensagem de sucesso.
                        msg[0] = 10; // A nova mensagem tem como fonte o servidor
                        msg[1] = data->payload;  // endereço da prateleira que deve ser preenchida
                        msg[2] = data->command;  // codigo de mensagem de preenchimento de prateleira
                        msg[3] = 0;  // não importa, deixo em 0.
                        packetServer = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                        sender_socket->Connect(InetSocketAddress(addrIGS, PORT));
                        sender_socket->Send(packetServer); // repassa a mensagem de sucesso para o nó intermediário entre servidor e gateway
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

    void Layer7::IGSCallback(Ptr<Socket> socket){
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
                sender_socket->Connect(InetSocketAddress(addrServer, PORT));
                sender_socket->Send(packetServer); // repassa a mensagem para o servidor
            }else if(data->source == 10){ // Servidor->Gateway
                uint8_t* msg = (uint8_t*)malloc(sizeof(messageData));
                msg[0] = data->source;
                msg[1] = data->dest;
                msg[2] = data->command;
                msg[3] = data->payload;  
                packetServer = Create<Packet>(msg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                sender_socket->Connect(InetSocketAddress(addrGateway, PORT));
                sender_socket->Send(packetServer); // repassa a mensagem para o servidor
            }else{
                uint8_t* errorMsg = (uint8_t*)malloc(sizeof(messageData));
                errorMsg[0] = data->source; // A nova mensagem tem como fonte o servidor
                errorMsg[1] = data->source;  // Essa mensagem não deveria ter saído do source
                errorMsg[2] = 5;  // Código de erro
                errorMsg[3] = 0;  // não importa, deixo em 0.
                packetServer = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem a ser repassada
                sender_socket->Connect(InetSocketAddress(senderAddress, PORT));
                sender_socket->Send(packetServer); // repassa a mensagem de sucesso para o nó intermediário entre servidor e gateway
                NS_LOG_ERROR("Erro ao enviar pacote indevido ao nó intermediário 'Gateway-Servidor'. Retornando ao nó de origem.");
            }
        }

    }

    void Layer7::ITSCallback(Ptr<Socket> socket){
        
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
                        for(uint8_t i = 0; i < 8; i++){ // repassa a mensagem para cada um dos sensores solicitando seus valores atuais
                            sender_socket->Connect(InetSocketAddress(addrSensors[i], PORT));
                            sender_socket->Send(packetS);
                        }
                        break;
                    case 1: // servidor deseja esvaziar uma das prateleiras
                        if(data->dest > 6 || data->dest < 0){ // caso o destino esteja fora do intervalo permitido, isto é, não seja o identificador de algum sensor
                            errorMsg[0] = 12; // quem manda é o intermediário entre sensores e servidor
                            errorMsg[1] = data->source;  // endereço de quem enviou a mensagem
                            errorMsg[2] = 5;  // codigo de mensagem de erro
                            errorMsg[3] = 1;  // codigo que indica que o erro foi de destino inválido
                            packetS = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                            sender_socket->Connect(InetSocketAddress(senderAddress, PORT));
                            sender_socket->Send(packetS); // envia de volta para quem enviou a mensagem, indicando erro na comunicação
                        }else{
                            sender_socket->Connect(InetSocketAddress(addrSensors[data->dest - 1], PORT));
                            sender_socket->Send(packetS); // repassa a mensagem para o sensor a ser esvaziado
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
                            sender_socket->Connect(InetSocketAddress(senderAddress, PORT));
                            sender_socket->Send(packetS);// envia de volta para quem enviou a mensagem, indicando erro na comunicação
                            
                            break;
                        }else{
                                sender_socket->Connect(InetSocketAddress(addrSensors[data->dest - 1], PORT));
                                sender_socket->Send(packetS);// repassa a mensagem para o sensor a ser preenchido
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
                    sender_socket->Connect(InetSocketAddress(addrServer, PORT));
                    sender_socket->Send(packetS); // repassa a mensagem para o servidor
                }else{ // Inconsistência na mensagem
                    // uint8_t* errorMsg = (uint8_t*)malloc(sizeof(messageData));
                    errorMsg[0] = 12; // quem manda é o intermediário entre sensores e servidor
                    errorMsg[1] = data->source; // endereço de quem enviou a mensagem
                    errorMsg[2] = 5;  // codigo de mensagem de erro
                    errorMsg[3] = 0;  // codigo que indica que o erro foi de fonte inválida
                    packetS = Create<Packet>(errorMsg, sizeof(messageData)); // cria pacote com mensagem de erro
                    sender_socket->Connect(InetSocketAddress(senderAddress, 550));
                    sender_socket->Send(packetS); // envia de volta para quem enviou a mensagem, indicando erro na comunicação
                }

            }
        }
    }

    void Layer7::GatewayCallback(Ptr<Socket> socket){
        
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

    void Layer7::SendPacket(Ptr<Packet> packet, Ipv4Address destination, uint16_t port)
    {
        NS_LOG_FUNCTION (this << packet << destination << port);
        sender_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(destination), port));
        sender_socket->Send(packet);
    }
} // namespace ns3