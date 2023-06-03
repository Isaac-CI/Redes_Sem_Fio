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
#include "simple-udp-application.h"

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
    uint32_t nCsma = 3;
    uint32_t nWifi = 3;
    bool tracing = true;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);

    cmd.Parse(argc, argv);

    // The underlying restriction of 18 is due to the grid position
    // allocator's configuration; the grid layout will exceed the
    // bounding box if more than 18 nodes are provided.
    if (nWifi > 18)
    {
        std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box"
                  << std::endl;
        return 1;
    }

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    //  Create p2p Network Nodes
    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    //Create point-to-point helper
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    //Create a device container to hold net devices installed on each node
    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    //  Create CSMA Network Nodes
    NodeContainer csmaNodes;
    csmaNodes.Add(p2pNodes.Get(1));
    csmaNodes.Create(nCsma);

    //Create CSMA helper
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    //Create a CSMA device container to hold net devices installed on each node
    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install(csmaNodes);

    //  Create WIFI Network Nodes
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWifi);
    NodeContainer wifiApNode = p2pNodes.Get(0);

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
    NetDeviceContainer staDevices;
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    //Create a WIFI device container for the Access Point
    NetDeviceContainer apDevices;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    apDevices = wifi.Install(phy, mac, wifiApNode);

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

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds",
                              RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.Install(wifiStaNodes);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);

    //Install Internet Stacks on each node
    InternetStackHelper stack;
    stack.Install(csmaNodes);
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    //Create an Address Helper
    Ipv4AddressHelper address;

    //Create an interface container to hold the created ipv4 interfaces and assign IP addresses to the interface
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(p2pDevices);

    //Create an interface container to hold the created ipv4 interfaces and assign IP addresses to the interface
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces;
    csmaInterfaces = address.Assign(csmaDevices);

    //Create an interface container to hold the created ipv4 interfaces and assign IP addresses to the interface
    address.SetBase("10.1.3.0", "255.255.255.0");
    address.Assign(staDevices);
    address.Assign(apDevices);

    //Application
    Packet::EnablePrinting();

    //Install SimpleUdpApplication in all the nodes
    ApplicationContainer apps;
    for(uint32_t i = 0; i < NodeList::GetNNodes(); i++) {
        Ptr<SimpleUdpApplication> udp_app = Create<SimpleUdpApplication> ();
        NodeList::GetNode(i)->AddApplication(udp_app);
        apps.Add(udp_app);
    }

    apps.Start(Seconds(0));
    apps.Stop(Seconds(5.0));
    
    Ptr<SimpleUdpApplication> udp0 = DynamicCast <SimpleUdpApplication> (apps.Get(0));
    Ipv4Address dest_ip ("10.1.1.2");

    Ptr<Packet> packet1 = Create <Packet> (400);
    Simulator::Schedule(Seconds(1), &SimpleUdpApplication::SendPacket, udp0, packet1, dest_ip, 5500);

    Ptr<Packet> packet2 = Create <Packet> (800);
    Simulator::Schedule(Seconds(1), &SimpleUdpApplication::SendPacket, udp0, packet2, dest_ip, 5000);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(5.0));

    if (tracing)
    {
        phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        pointToPoint.EnablePcapAll("third");
        phy.EnablePcap("third", apDevices.Get(0));
        csma.EnablePcap("third", csmaDevices.Get(0), true);
    }

    LogComponentEnable("SimpleUdpApplication", LOG_LEVEL_INFO);

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
