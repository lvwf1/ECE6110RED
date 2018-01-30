/** Network topology
 *
 *    100Mb/s, 1ms |
 * n3--------------|
 *                 |       45Mb/s 20ms
 *                 n5------------------n6
 *    100Mb/s, 1ms |
 * n4--------------|
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Red(a)");

uint32_t checkTimes;
double avgQueueSize;
uint32_t port = 8888;
constexpr uint32_t packetSize = 1000 - 42;

NodeContainer n1n3;
NodeContainer n2n3;
NodeContainer n3n4;

Ipv4InterfaceContainer i1i3;
Ipv4InterfaceContainer i2i3;
Ipv4InterfaceContainer i3i4;

std::stringstream filePlotQueue;
std::stringstream filePlotQueueAvg;
std::stringstream filePlotPacketArrive;
std::stringstream filePlotPacketDrop;

// The sequence number is in bytes not packets
void EnqueueAtRed(Ptr<const QueueDiscItem> item) {
    TcpHeader tcp;
    Ptr<Packet> pkt = item->GetPacket();
    pkt->PeekHeader(tcp);

    std::ofstream fPlotPacketArrive(filePlotPacketArrive.str().c_str(), std::ios::out | std::ios::app);
    fPlotPacketArrive << Simulator::Now().GetSeconds() << " " << tcp.GetSequenceNumber() << " " << tcp.GetDestinationPort() << std::endl;
    fPlotPacketArrive.close();
}

void DequeueAtRed(Ptr<const QueueDiscItem> item) {
    TcpHeader tcp;
    Ptr<Packet> pkt = item->GetPacket();
    pkt->PeekHeader(tcp);
}

void DroppedAtRed(Ptr<const QueueDiscItem> item) {
    TcpHeader tcp;
    Ptr<Packet> pkt = item->GetPacket();
    pkt->PeekHeader(tcp);

    std::ofstream fPlotPacketDrop(filePlotPacketDrop.str().c_str(), std::ios::out | std::ios::app);
    fPlotPacketDrop << Simulator::Now().GetSeconds() << std::endl;
    fPlotPacketDrop.close();
}

void CheckQueueSize (Ptr<QueueDisc> queue)
{
    uint32_t qSize = StaticCast<RedQueueDisc> (queue)->GetQueueSize ();

    avgQueueSize += qSize;
    checkTimes++;

    // check queue size every 1/100 of a second
    Simulator::Schedule (Seconds (0.01), &CheckQueueSize, queue);

    std::ofstream fPlotQueue (filePlotQueue.str ().c_str (), std::ios::out|std::ios::app);
    fPlotQueue << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
    fPlotQueue.close ();

    std::ofstream fPlotQueueAvg (filePlotQueueAvg.str ().c_str (), std::ios::out|std::ios::app);
    fPlotQueueAvg << Simulator::Now ().GetSeconds () << " " << avgQueueSize / checkTimes << std::endl;
    fPlotQueueAvg.close ();
}

int
main (int argc, char *argv[])
{
    LogComponentEnable ("RedQueueDisc", LOG_LEVEL_INFO);

    std::string pathOut = "P2b";;
    bool writeForPlot = true;
    bool writePcap = false;
    bool flowMonitor = false;

    uint32_t runNumber = 0;
    uint32_t maxPackets = 40;
    uint32_t meanPktSize = 500;
    std::string redLinkDataRate = "45Mbps";
    std::string redLinkDelay = "20ms";
    double qw = 0.002;
    double minTh = 15;
    double maxTh = 140;
    double stopTime = 1.0;
    double queueLimit = 1000;

    // Will only save in the directory if enable opts below
    CommandLine cmd;
    cmd.AddValue ("runNumber", "run number for random variable generation", runNumber);
    cmd.AddValue ("pathOut", "Path to save results from --writeForPlot/--writePcap/--writeFlowMonitor", pathOut);
    cmd.AddValue ("maxPackets", "Max packets allowed in the device queue", maxPackets);
    cmd.AddValue ("writeForPlot", "<0/1> to write results for plot (gnuplot)", writeForPlot);
    cmd.AddValue ("writePcap", "<0/1> to write results in pcapfile", writePcap);
    cmd.AddValue ("writeFlowMonitor", "<0/1> to enable Flow Monitor and write their results", flowMonitor);

    // RED params
    NS_LOG_INFO ("Set RED params");
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(packetSize));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(1));
    Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_DISC_MODE_PACKETS"));
    Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (meanPktSize));
    Config::SetDefault ("ns3::RedQueueDisc::Wait", BooleanValue (true));
    Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (true));
    Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (qw));
    Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (minTh));
    Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (maxTh));
    Config::SetDefault ("ns3::RedQueueDisc::LinkBandwidth", StringValue (redLinkDataRate));
    Config::SetDefault ("ns3::RedQueueDisc::LinkDelay", StringValue (redLinkDelay));
    Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (queueLimit));

    cmd.Parse (argc, argv);

    NS_LOG_INFO ("Create nodes");
    NodeContainer c;
    c.Create (4);
    Names::Add ( "N1", c.Get (0));
    Names::Add ( "N2", c.Get (1));
    Names::Add ( "N3", c.Get (2));
    Names::Add ( "N4", c.Get (3));
    n1n3 = NodeContainer (c.Get (0), c.Get (2));
    n2n3 = NodeContainer (c.Get (1), c.Get (2));
    n3n4 = NodeContainer (c.Get (2), c.Get (3));

    NS_LOG_INFO ("Install internet stack on all nodes.");
    InternetStackHelper internet;
    internet.Install (c);

    TrafficControlHelper tchRed;
    tchRed.SetRootQueueDisc("ns3::RedQueueDisc");

    NS_LOG_INFO ("Create channels");
    PointToPointHelper p2p;

    p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
    NetDeviceContainer devn1n3 = p2p.Install (n1n3);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
    NetDeviceContainer devn2n3 = p2p.Install (n2n3);

    p2p.SetDeviceAttribute ("DataRate", StringValue (redLinkDataRate));
    p2p.SetChannelAttribute ("Delay", StringValue (redLinkDelay));
    NetDeviceContainer devn3n4 = p2p.Install (n3n4);
    Ptr<QueueDisc> redQueue = (tchRed.Install(devn3n4)).Get(0);

    //setup traces
    redQueue->TraceConnectWithoutContext("Enqueue", MakeCallback(&EnqueueAtRed));
    redQueue->TraceConnectWithoutContext("Dequeue", MakeCallback(&DequeueAtRed));
    redQueue->TraceConnectWithoutContext("Drop", MakeCallback(&DroppedAtRed));

    NS_LOG_INFO ("Assign IP Addresses");
    Ipv4AddressHelper ipv4;

    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    i1i3 = ipv4.Assign (devn1n3);

    ipv4.SetBase ("10.1.2.0", "255.255.255.0");
    i2i3 = ipv4.Assign (devn2n3);

    ipv4.SetBase ("10.1.3.0", "255.255.255.0");
    i3i4 = ipv4.Assign (devn3n4);

    // Set up the routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    ApplicationContainer sources0;
    ApplicationContainer sources1;

    //Install Sources
    OnOffHelper sourceHelper("ns3::TcpSocketFactory", Address());
    sourceHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    sourceHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    sourceHelper.SetAttribute("DataRate", DataRateValue(DataRate("100Mbps")));
    sourceHelper.SetAttribute("PacketSize", UintegerValue(packetSize));

    AddressValue remote1(InetSocketAddress(i3i4.GetAddress(1), 8081));
    sourceHelper.SetAttribute("Remote", remote1);
    sources0.Add(sourceHelper.Install(n1n3.Get(0)));
    sources0.Start(Seconds(0));
    AddressValue remote2(InetSocketAddress(i3i4.GetAddress(1), 8082));
    sourceHelper.SetAttribute("Remote", remote2);
    sources1.Add(sourceHelper.Install(n2n3.Get(0)));
    sources1.Start(Seconds(0.2));

    ApplicationContainer sinks;

    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", Address());
    sinkHelper.SetAttribute("Local", remote1);
    sinks.Add(sinkHelper.Install(n3n4.Get(1)));
    sinkHelper.SetAttribute("Local", remote2);
    sinks.Add(sinkHelper.Install(n3n4.Get(1)));

    sinks.Start(Seconds(0));

    if (writePcap)
    {
        PointToPointHelper ptp;
        std::stringstream stmp;
        stmp << pathOut << "/red";
        ptp.EnablePcapAll (stmp.str ().c_str ());
    }

    Ptr<FlowMonitor> flowmon;
    if (flowMonitor)
    {
        FlowMonitorHelper flowmonHelper;
        flowmon = flowmonHelper.InstallAll ();
    }

    if (writeForPlot)
    {
        filePlotQueue << pathOut << "/" << "redQueue.plot";
        filePlotQueueAvg << pathOut << "/" << "redQueueAvg.plot";
        filePlotPacketArrive << pathOut << "/" << "PacketNum.plot";
        filePlotPacketDrop << pathOut << "/" << "PacketDrop.plot";

        remove (filePlotQueue.str ().c_str ());
        remove (filePlotQueueAvg.str ().c_str ());
        Simulator::ScheduleNow(&CheckQueueSize, redQueue);
    }

    Simulator::Stop(Seconds(stopTime));
    Simulator::Run();

    uint32_t totalBytes = 0;

    for (uint32_t i = 0; i < sinks.GetN(); ++i) {
        Ptr<Application> app = sinks.Get(i);
        Ptr<PacketSink> pktSink = DynamicCast<PacketSink>(app);
        uint32_t recieved = pktSink->GetTotalRx();
        std::cout << "\tSink\t" << i << "\tBytes\t" << recieved << std::endl;
        totalBytes += recieved;
    }

    std::cout << std::endl << "\tTotal\t\tBytes\t" << totalBytes << std::endl;

    std::cout << "Done" << std::endl;

    if (flowMonitor)
    {
        std::stringstream stmp;
        stmp << pathOut << "/red.flowmon";

        flowmon->SerializeToXmlFile (stmp.str ().c_str (), false, false);
    }

    Simulator::Destroy ();

    return 0;
}
