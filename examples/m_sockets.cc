/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ThirdScriptExample");

int
main(int argc, char* argv[])
{
    bool verbose = true;
    uint32_t nShelves = 3;
    bool tracing = true;

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);

    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    //  Create p2p Network Nodes between server and intermediate nodes
    NodeContainer p2pNodes;
    p2pNodes.Create(4);

    //Create point-to-point helper
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    //Create a device container to hold net devices installed on each node, connecting the server node with each one of the intermediate nodes
    NetDeviceContainer IntermediateDevices;
    IntermediateDevices = pointToPoint.Install(p2pNodes.Get(0), p2pNodes.Get(1));
    IntermediateDevices.Add(pointToPoint.Install(p2pNodes.Get(0), p2pNodes.Get(2)));
    IntermediateDevices.Add(pointToPoint.Install(p2pNodes.Get(0), p2pNodes.Get(3)));
    
    //  Create WIFI Network Nodes
    NodeContainer nodesP1, nodesP2, nodesP3;
    nodesP1.Create(nShelves);
    nodesP2.Create(nShelves);
    nodesP3.Create(nShelves);

    // Create Access Points to the shelves sub-networks
    NodeContainer ApP1 = p2pNodes.Get(1);
    NodeContainer ApP2 = p2pNodes.Get(2);
    NodeContainer ApP3 = p2pNodes.Get(3);

    //Create WIFI helpers for layers 1 and 2
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    //Create WIFI helpers for MAC addressing
    WifiMacHelper mac;
    Ssid ssidP1 = Ssid("ns-3-ssidP1");
    Ssid ssidP2 = Ssid("ns-3-ssidP2");
    Ssid ssidP3 = Ssid("ns-3-ssidP3");

    //Create WIFI helper
    WifiHelper wifi;

    //Create a WIFI device container for the shelves sub-networks
    NetDeviceContainer P1Devices, P2Devices, P3Devices;
    // Shelf 1
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssidP1), "ActiveProbing", BooleanValue(false));
    P1Devices = wifi.Install(phy, mac, nodesP1);
    // Shelf 2
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssidP2), "ActiveProbing", BooleanValue(false));
    P2Devices = wifi.Install(phy, mac, nodesP2);
    // Shelf 3
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssidP3), "ActiveProbing", BooleanValue(false));
    P3Devices = wifi.Install(phy, mac, nodesP3);

    //Create WIFI device containers for the Access Points
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssidP1));
    IntermediateDevices.Add(wifi.Install(phy, mac, ApP1));
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssidP2));
    IntermediateDevices.Add(wifi.Install(phy, mac, ApP2));
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssidP3));
    IntermediateDevices.Add(wifi.Install(phy, mac, ApP3));

    //Create Mobility helper
    MobilityHelper mobility;

    //Configure Nodes Mobility(Set their position on the grid)
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(5.0),
                                  "DeltaY",
                                  DoubleValue(10.0),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodesP1);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodesP2);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodesP3);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(p2pNodes);

    //Install Internet Stacks on each node
    InternetStackHelper stack;
    stack.Install(p2pNodes);
    stack.Install(nodesP1);
    stack.Install(nodesP2);
    stack.Install(nodesP3);

    //Create an Address Helper
    Ipv4AddressHelper address;

    //Create an interface container to hold the created ipv4 interfaces and assign IP addresses to the interface
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(IntermediateDevices);


    //Create an interface container to hold the created ipv4 interfaces and assign IP addresses to the interface
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer P1Interfaces;
    P1Interfaces = address.Assign(P1Devices);


    //Create an interface container to hold the created ipv4 interfaces and assign IP addresses to the interface
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer P2Interfaces;
    P2Interfaces = address.Assign(P2Devices);

    //Create an interface container to hold the created ipv4 interfaces and assign IP addresses to the interface
    address.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer P3Interfaces;
    P3Interfaces = address.Assign(P3Devices);

    //Application
    Packet::EnablePrinting();

    uint16_t port = 5500;
    
    // Sensor Sockets application
    Ptr<Socket> P1Socket[nodesP1.GetN()];
    for(uint8_t i = 0; i < nodesP1.GetN(); i++){
        P1Socket[i] = Socket::CreateSocket(nodesP1.Get(i), TypeId::LookupByName("ns3::UdpSocketFactory"));
        P1Socket[i]->Bind(InetSocketAddress(P1Interfaces.GetAddress(i), port));
    }
    Ptr<Socket> P2Socket[nodesP2.GetN()];
    for(uint8_t i = 0; i < nodesP2.GetN(); i++){
        P2Socket[i] = Socket::CreateSocket(nodesP2.Get(i), TypeId::LookupByName("ns3::UdpSocketFactory"));
        P2Socket[i]->Bind(InetSocketAddress(P1Interfaces.GetAddress(i), port));
    }
    Ptr<Socket> P3Socket[nodesP3.GetN()];
    for(uint8_t i = 0; i < nodesP3.GetN(); i++){
        P3Socket[i] = Socket::CreateSocket(nodesP3.Get(i), TypeId::LookupByName("ns3::UdpSocketFactory"));
        P3Socket[i]->Bind(InetSocketAddress(P1Interfaces.GetAddress(i), port));
    }
    // Server socket application
    Ptr<Socket> serverSocket = Socket::CreateSocket(p2pNodes.Get(0), TypeId::LookupByName("ns3::UdpSocketFactory"));
    serverSocket->Bind(InetSocketAddress(p2pInterfaces.GetAddress(0), port));

    // Server - Intermediate(AP) socket application
    Ptr<Socket> P1ServerSocket = Socket::CreateSocket(p2pNodes.Get(1), TypeId::LookupByName("ns3::UdpSocketFactory"));
    P1ServerSocket->Bind(InetSocketAddress(p2pInterfaces.GetAddress(1), port));

    Ptr<Socket> P2ServerSocket = Socket::CreateSocket(p2pNodes.Get(2), TypeId::LookupByName("ns3::UdpSocketFactory"));
    P2ServerSocket->Bind(InetSocketAddress(p2pInterfaces.GetAddress(2), port));

    Ptr<Socket> P3ServerSocket = Socket::CreateSocket(p2pNodes.Get(3), TypeId::LookupByName("ns3::UdpSocketFactory"));
    P3ServerSocket->Bind(InetSocketAddress(p2pInterfaces.GetAddress(3), port));

    uint8_t message = 'F';

    Ptr<Packet> teste = Create<Packet>(&message, sizeof(message));

    serverSocket->SendTo(teste, 0, InetSocketAddress(p2pInterfaces.GetAddress(1), port));
    std::cout << "Sending mensage: " <<  static_cast<char>(message)  << " to " << p2pInterfaces.GetAddress(1) << " from Ip " << p2pInterfaces.GetAddress(0) << std::endl;
    P1ServerSocket->SetRecvPktInfo(true); // Enable receiving sender address information
    // Receiving the message from the server
    P1ServerSocket->SetRecvCallback([&](Ptr<Socket> socket) {
        uint8_t receivedMessage;
        Ptr<Packet> receivedPacket;
        Address senderAddress;

        receivedPacket = socket->RecvFrom(senderAddress);
        receivedPacket->CopyData(&receivedMessage, sizeof(receivedMessage));
        for(uint32_t i = 1; i < nodesP1.GetN(); i++){
            P1ServerSocket->SendTo(receivedPacket, 0, InetSocketAddress(P1Interfaces.GetAddress(i), port));
        }
        std::cout << "Receiving mensage: " <<  static_cast<char>(receivedMessage)  << " From " << p2pInterfaces.GetAddress(0) << " from Ip " << p2pInterfaces.GetAddress(1) << std::endl;
    });

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(5.0));

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
