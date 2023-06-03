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
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FirstScriptExample");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    NodeContainer nodesA, nodeB, nodesC;
    nodesA.Create(3);
    nodeB.Create(1);
    nodesC.Create(3);

    //Rede A com nó B = rede AB
    //Rede C com nó B = rede BC
    
    //Cria canal WiFi para a rede AB
    YansWifiChannelHelper channelAB = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phyAB;
    phyAB.SetChannel(channelAB.Create()); //Configura camada física de nós da rede AB

    //Cria canal WiFi para a rede BC
    YansWifiChannelHelper channelBC = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phyBC;
    phyBC.SetChannel(channelBC.Create()); //Configura camada física de nós da rede BC

    WifiHelper wifi;

    WifiMacHelper macAB, macBC;
    Ssid ssidAB = Ssid("ns-3-ssidAB"); //Nome da rede AB
    Ssid ssidBC = Ssid("ns-3-ssidBC"); //Nome da rede BC

    NetDeviceContainer devicesA, deviceAB, deviceBC, devicesC;

    //Configura camada de enlace (mac) em nós da rede AB
    macAB.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssidAB), "ActiveProbing", BooleanValue(false));
    devicesA = wifi.Install(phyAB, macAB, nodesA);
    macAB.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssidAB));
    deviceAB = wifi.Install(phyAB, macAB, nodeB); //Define que nó B será ponto de acesso da rede AB

    //Configura camada de enlace (mac) em nós da rede BC
    macBC.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssidBC), "ActiveProbing", BooleanValue(false));
    devicesC = wifi.Install(phyBC, macBC, nodesC);
    macBC.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssidBC));
    deviceBC = wifi.Install(phyBC, macBC, nodeB); //Define que nó B será ponto de acesso da rede BC

    //Configura pilha de protocolos da internet nos nós
    InternetStackHelper stack;
    stack.Install(nodesA);
    stack.Install(nodeB);
    stack.Install(nodesC);

    Ipv4AddressHelper addressAB, addressBC;
    addressAB.SetBase("10.1.1.0", "255.255.255.0");
    addressBC.SetBase("10.1.2.0", "255.255.255.0");

    Ipv4InterfaceContainer interfacesA = addressAB.Assign(devicesA);
    Ipv4InterfaceContainer interfacesAB = addressAB.Assign(deviceAB); //Atribui endereço IP da rede AB para o nó B

    Ipv4InterfaceContainer interfacesC = addressBC.Assign(devicesC);
    Ipv4InterfaceContainer interfacesBC = addressBC.Assign(deviceBC); //Atribui endereço IP da rede BC para o nó B

    //Define mobilidade nula em todos os nós
    MobilityHelper mobility;

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodesA);
    mobility.Install(nodeB);
    mobility.Install(nodesC);

    UdpEchoServerHelper echoServer(9);

    ApplicationContainer serverApps = echoServer.Install(nodesC.Get(2));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    UdpEchoClientHelper echoClient(interfacesC.GetAddress(2), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(nodesA.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}