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

    NodeContainer Shelf1_Sensors, Shelf2_Sensors, EntranceSensor, ExitSensor, Connector, Server, Robot;
    Shelf1_Sensors.Create(4);
    Shelf2_Sensors.Create(4);
    EntranceSensor.Create(1);
    ExitSensor.Create(1);
    Connector.Create(1);
    Server.Create(1);
    Robot.Create(1);

    //Rede da Estante 1 com nó Itermediário = rede S1C
    //Rede da Estante 2 com nó Itermediário = rede S2C
    //Rede do Sensor da Entrada com nó Itermediário = rede E1C
    //Rede do Sensor da Saída com nó Itermediário = rede E2C
    //Rede do Servidor com nó Itermediário = rede SC
    //Rede do Robo com nó Itermediário = rede RC
    
    //Cria canal WiFi para a rede S1C
    YansWifiChannelHelper channelS1C = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phyS1C;
    phyS1C.SetChannel(channelS1C.Create()); //Configura camada física de nós da rede S1C

    //Cria canal WiFi para a rede S2C
    YansWifiChannelHelper channelS2C = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phyS2C;
    phyS2C.SetChannel(channelS2C.Create()); //Configura camada física de nós da rede S2C

    //Cria canal WiFi para a rede E1C
    YansWifiChannelHelper channelE1C = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phyE1C;
    phyE1C.SetChannel(channelE1C.Create()); //Configura camada física de nós da rede E1C

    //Cria canal WiFi para a rede E2C
    YansWifiChannelHelper channelE2C = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phyE2C;
    phyE2C.SetChannel(channelE2C.Create()); //Configura camada física de nós da rede E2C

    //Cria canal WiFi para a rede SC
    YansWifiChannelHelper channelSC = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phySC;
    phySC.SetChannel(channelSC.Create()); //Configura camada física de nós da rede SC

    //Cria canal WiFi para a rede RC
    YansWifiChannelHelper channelRC = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phyRC;
    phyRC.SetChannel(channelRC.Create()); //Configura camada física de nós da rede RC


    WifiHelper wifi;

    WifiMacHelper macS1C, macS2C, macE1C, macE2C, macSC, macRC;
    Ssid ssidS1C = Ssid("ns-3-ssidS1C"); //Nome da rede S1C
    Ssid ssidS2C = Ssid("ns-3-ssidS2C"); //Nome da rede S2C
    Ssid ssidE1C = Ssid("ns-3-ssidE1C"); //Nome da rede E1C
    Ssid ssidE2C = Ssid("ns-3-ssidE2C"); //Nome da rede E2C
    Ssid ssidSC = Ssid("ns-3-ssidSC"); //Nome da rede SC
    Ssid ssidRC = Ssid("ns-3-ssidRC"); //Nome da rede RC

    NetDeviceContainer devicesS1, deviceS1C, devicesS2, deviceS2C, deviceE1, deviceE1C, deviceE2, deviceE2C, deviceS, deviceSC, deviceR, deviceRC;

    //Configura camada de enlace (mac) em nós da rede S1C
    macS1C.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssidS1C), "ActiveProbing", BooleanValue(false));
    devicesS1 = wifi.Install(phyS1C, macS1C, Shelf1_Sensors);
    macS1C.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssidS1C));
    deviceS1C = wifi.Install(phyS1C, macS1C, Connector); //Define que nó B será ponto de acesso da rede S1C

    //Configura camada de enlace (mac) em nós da rede S2C
    macS2C.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssidS2C), "ActiveProbing", BooleanValue(false));
    devicesS2 = wifi.Install(phyS2C, macS2C, Shelf2_Sensors);
    macS2C.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssidS2C));
    deviceS2C = wifi.Install(phyS2C, macS2C, Connector); //Define que nó B será ponto de acesso da rede S2C

    //Configura camada de enlace (mac) em nós da rede E1C
    macE1C.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssidE1C), "ActiveProbing", BooleanValue(false));
    deviceE1 = wifi.Install(phyE1C, macE1C, EntranceSensor);
    macE1C.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssidE1C));
    deviceE1C = wifi.Install(phyE1C, macE1C, Connector); //Define que nó Itermediário será ponto de acesso da rede E1C

    //Configura camada de enlace (mac) em nós da rede E2C
    macE2C.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssidE2C), "ActiveProbing", BooleanValue(false));
    deviceE2 = wifi.Install(phyE2C, macE2C, ExitSensor);
    macE2C.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssidE2C));
    deviceE2C = wifi.Install(phyE2C, macE2C, Connector); //Define que nó Itermediário será ponto de acesso da rede E2C

    //Configura camada de enlace (mac) em nós da rede SC
    macSC.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssidSC), "ActiveProbing", BooleanValue(false));
    deviceS = wifi.Install(phySC, macSC, Server);
    macSC.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssidSC));
    deviceSC = wifi.Install(phySC, macSC, Connector); //Define que nó Itermediário será ponto de acesso da rede SC

    //Configura camada de enlace (mac) em nós da rede RC
    macRC.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssidRC), "ActiveProbing", BooleanValue(false));
    deviceR = wifi.Install(phyRC, macRC, Robot);
    macRC.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssidRC));
    deviceRC = wifi.Install(phyRC, macRC, Connector); //Define que nó Itermediário será ponto de acesso da rede RC

    //Configura pilha de protocolos da internet nos nós
    InternetStackHelper stack;
    stack.Install(Shelf1_Sensors);
    stack.Install(Shelf2_Sensors);
    stack.Install(EntranceSensor);
    stack.Install(ExitSensor);
    stack.Install(Connector);
    stack.Install(Server);
    stack.Install(Robot);

    Ipv4AddressHelper addressS1C, addressS2C, addressE1C, addressE2C, addressSC, addressRC;
    addressS1C.SetBase("10.1.1.0", "255.255.255.0");
    addressS2C.SetBase("10.1.2.0", "255.255.255.0");
    addressE1C.SetBase("10.1.3.0", "255.255.255.0");
    addressE2C.SetBase("10.1.4.0", "255.255.255.0");
    addressSC.SetBase("10.1.5.0", "255.255.255.0");
    addressRC.SetBase("10.1.6.0", "255.255.255.0");

    Ipv4InterfaceContainer interfacesS1 = addressS1C.Assign(devicesS1);
    Ipv4InterfaceContainer interfacesS1C = addressS1C.Assign(deviceS1C); //Atribui endereço IP da rede S1C para o nó Itermediário

    Ipv4InterfaceContainer interfacesS2 = addressS2C.Assign(devicesS2);
    Ipv4InterfaceContainer interfacesS2C = addressS2C.Assign(deviceS2C); //Atribui endereço IP da rede S2C para o nó Itermediário

    Ipv4InterfaceContainer interfacesE1 = addressE1C.Assign(deviceE1);
    Ipv4InterfaceContainer interfacesE1C = addressE1C.Assign(deviceE1C); //Atribui endereço IP da rede E1C para o nó Itermediário

    Ipv4InterfaceContainer interfacesE2 = addressE2C.Assign(deviceE2);
    Ipv4InterfaceContainer interfacesE2C = addressE2C.Assign(deviceE2C); //Atribui endereço IP da rede E2C para o nó Itermediário

    Ipv4InterfaceContainer interfacesS = addressSC.Assign(deviceS);
    Ipv4InterfaceContainer interfacesSC = addressSC.Assign(deviceSC); //Atribui endereço IP da rede SC para o nó Itermediário

    Ipv4InterfaceContainer interfacesR = addressRC.Assign(deviceR);
    Ipv4InterfaceContainer interfacesRC = addressRC.Assign(deviceRC); //Atribui endereço IP da rede RC para o nó Itermediário

    //Define mobilidade nula em todos os nós
    MobilityHelper mobility;

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(Shelf1_Sensors);
    mobility.Install(Shelf2_Sensors);
    mobility.Install(EntranceSensor);
    mobility.Install(ExitSensor);
    mobility.Install(Server);
    mobility.Install(Robot);

    // UdpEchoServerHelper echoServer(9);

    // ApplicationContainer serverApps = echoServer.Install(nodesC.Get(2));
    // serverApps.Start(Seconds(1.0));
    // serverApps.Stop(Seconds(10.0));

    // UdpEchoClientHelper echoClient(interfacesC.GetAddress(2), 9);
    // echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    // echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    // echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    // ApplicationContainer clientApps = echoClient.Install(nodesA.Get(0));
    // clientApps.Start(Seconds(2.0));
    // clientApps.Stop(Seconds(10.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}