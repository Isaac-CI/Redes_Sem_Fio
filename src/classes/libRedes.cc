#include "libRedes.h"

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

namespace ns3
{
    LibRedes::LibRedes()
    {
        shelves.resize(6);
    }
    
    LibRedes::~LibRedes()
    {
    }
    

    int LibRedes::loadFile()
    {
        std::ifstream file("/home/isaac/Documents/Estudos/redes_sem_fio/codigos-ns3/src/data/instance.txt");
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
                shelves[0].push((bool)number);
            }else if (count < 2 * nCols)
            {
                shelves[1].push(number);
            }else if (count < 3 * nCols)
            {
                shelves[2].push(number);
            }else if (count < 4 * nCols)
            {
                shelves[3].push(number);
            }else if (count < 5 * nCols)
            {
                shelves[4].push(number);
            }else if (count < 6 * nCols)
            {
                shelves[5].push(number);
            }else if (count < 7 * nCols)
            {
                gateway_commands.push_back(number);
            }else if (count < 8 * nCols)
            {
                gateway_target.push_back(number);
            }
            count++;
        }
        file.close();
        } else {
            std::cout << "Unable to open the file." << std::endl;
            return 1; // Return an error code
        }
        return 0;
    }
}