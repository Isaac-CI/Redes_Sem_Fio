#ifndef LIB_REDES_H
#define LIB_REDES_H

#include "ns3/socket.h"
#include "ns3/application.h"
#include "ns3/callback.h"
#include <queue>
#include <fstream>

using namespace ns3;

namespace ns3
{
    class LibRedes
    {   
    public:
        LibRedes();

        std::vector<std::queue<bool>> shelves;
        std::vector<int> gateway_commands;
        std::vector<int> gateway_target;
        typedef struct{
            uint8_t source;
            uint8_t dest;
            uint8_t command;
            uint8_t payload;
        } messageData;

        int loadFile();

    };
    

} // namespace ns3



#endif