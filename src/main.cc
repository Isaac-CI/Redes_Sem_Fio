#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"

using namespace ns3;
using namespace std;

int main(){
    NodeContainer sensorNodes;
    sensorNodes.Create(9);

    NodeContainer intermediateNodes;
    intermediateNodes.Create(4);

    NodeContainer serverNode;
    serverNode.Create(1);
 
    MobilityHelper sensorMobility, intermediateMobility, serverMobility;

    Ptr<ConstantPositionMobilityModel> serverMobilityModel = CreateObject<ConstantPositionMobilityModel>();

    double xCoord = 0.0;
    double yCoord = 0.0;
    double zCoord = 0.0;

    serverMobilityModel->SetPosition(ns3::Vector(xCoord, yCoord, zCoord));
    serverNode.Get(0)->AggregateObject(serverMobilityModel);

    std::vector<Ptr<ConstantPositionMobilityModel>> sensorMobilityModels;
    std::vector<Ptr<ConstantPositionMobilityModel>> intermediateMobilityModels;

    for(uint idx = 0; idx < sensorNodes.GetN(); idx++){
        Ptr<ConstantPositionMobilityModel> sensorMobilityModel = CreateObject<ConstantPositionMobilityModel>();
        sensorMobilityModels.push_back(sensorMobilityModel);
        sensorMobilityModels[idx]->SetPosition(Vector(xCoord, yCoord, zCoord));
        sensorNodes.Get(idx)->AggregateObject(sensorMobilityModels[idx]);
        xCoord += 10.0;
        yCoord += 10.0;
    }



    // ns3::Ptr<ns3::ConstantPositionMobilityModel> mobilityModel1 = ns3::CreateObject<ns3::ConstantPositionMobilityModel>();
    // mobilityModel1->SetPosition(ns3::Vector(0.0, 0.0, 0.0)); // Define a posição (0.0, 0.0, 0.0) para o nó 1
    // nodes.Get(0)->AggregateObject(mobilityModel1);

    // ns3::Ptr<ns3::ConstantPositionMobilityModel> mobilityModel2 = ns3::CreateObject<ns3::ConstantPositionMobilityModel>();
    // mobilityModel2->SetPosition(ns3::Vector(100.0, 100.0, 0.0)); // Define a posição (100.0, 100.0, 0.0) para o nó 2
    // nodes.Get(1)->AggregateObject(mobilityModel2);


    InternetStackHelper internet;
    internet.Install(sensorNodes);
    internet.Install(intermediateNodes);
    internet.Install(serverNode);

}


