#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FirstScriptExample");

int main(int argc, char * argv[]){

    //The next line sets the time resolution to one nanosecond
    Time::SetResolution(Time::NS);
    
    //The next two lines of the script are used to enable two logging 
    // components that are built into the Echo Client and Echo Server applications
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    //The next two lines of code in our script will actually create the ns-3 Node
    // objects that will represent the computers in the simulation.
    NodeContainer nodes;
    nodes.Create(2);

    // Instantiates a PointToPointHelper object on the stack.
    PointToPointHelper pointToPoint;

    // tells the PointToPointHelper object to use the value “5Mbps” (five megabits per second) 
    // as the “DataRate” when it creates a PointToPointNetDevice object.
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));

    //tells the PointToPointHelper to use the value “2ms” (two milliseconds) as the value of 
    // the propagation delay of every point to point channel it subsequently creates.
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));


    /*At this point in the script, we have a NodeContainer that contains two nodes. 
    We have a PointToPointHelper that is primed and ready to make PointToPointNetDevices and 
    wire PointToPointChannel objects between them. Just as we used the NodeContainer topology 
    helper object to create the Nodes for our simulation, we will ask the PointToPointHelper to do 
    the work involved in creating, configuring and installing our devices for us. We will need to 
    have a list of all of the NetDevice objects that are created, so we use a NetDeviceContainer to 
    hold them just as we used a NodeContainer to hold the nodes we created. The following two lines 
    of code, will finish configuring the devices and channel.
    NetDeviceContainer devices;*/
    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);


    /*The InternetStackHelper is a topology helper that is to internet stacks what the PointToPointHelper 
    is to point-to-point net devices. The Install method takes a NodeContainer as a parameter. When it 
    is executed, it will install an Internet Stack (TCP, UDP, IP, etc.) on each of the nodes in the node 
    container.*/
    InternetStackHelper stack;
    stack.Install(nodes);

    // Next we need to associate the devices on our nodes with IP addresses. We provide a topology helper to 
    // manage the allocation of IP addresses. The only user-visible API is to set the base IP address and 
    // network mask to use when performing the actual address allocation (which is done at a lower level 
    // inside the helper).
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Now we have a point-to-point network built, with stacks installed and IP addresses assigned. What 
    // we need at this point are applications to generate traffic.

    // Another one of the core abstractions of the ns-3 system is the Application. In this script we use 
    // two specializations of the core ns-3 class Application called UdpEchoServerApplication and 
    // UdpEchoClientApplication. Just as we have in our previous explanations, we use helper objects to 
    // help configure and manage the underlying objects. Here, we use UdpEchoServerHelper and 
    // UdpEchoClientHelper objects to make our lives easier.

    // The following lines of code in our example script, first.cc, are used to set up a UDP echo server 
    // application on one of the nodes we have previously created.

    UdpEchoServerHelper echoServer(9);

    ApplicationContainer serverApps = echoServer.Install(nodes.Get(1));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    
}