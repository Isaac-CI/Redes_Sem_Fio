#ifndef NS3_UDP_SERVER_APP_H
#define NS3_UDP_SERVER_APP_H
#include "ns3/socket.h"
#include "../components/libRedes.h"
#include "ns3/application.h"

namespace ns3
{
    class ServerApp : public Application, public LibRedes
    {
        public:
            ServerApp ();
            ServerApp (Ipv4Address, Ipv4Address, Ipv4Address, int, LibRedes);
            virtual ~ServerApp ();

            static TypeId GetTypeId ();
            virtual TypeId GetInstanceTypeId () const;

            void ServerCallback(Ptr<Socket>);

            std::vector<bool> state_table;

            void SendPacket (Ptr<Packet> packet, Ipv4Address destination, uint16_t port);

        private:
        
            void SetupReceiveSocket (Ptr<Socket> socket, Ipv4Address addr);
            virtual void StartApplication ();
            
            ns3::Callback<void, Ptr<Socket>> cb;
            int id; // identificador do nó. Caso seja sensor, vai de 1 até 6. Caso seja servidor, id = 10. Caso seja intermediário entre servidor e gateway, é 11. Se for o intermediário entre servidor e sensores é 12 e por fim, caso seja o gateway, id é 13.
            Ipv4Address addrISS; // endereço do nó intermediário entre servidor e sensores
            Ipv4Address addrIGS; // endereço do nó intermediário entre servidor e gateway
            Ipv4Address m_addr; // endereço do nó cuja aplicação está instalada
            Ptr<Socket> receiver_socket; /**< A socket to receive data */
            Ptr<Socket> sender_socket; /**< A socket to listen data */
    };
}

#endif