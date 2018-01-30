/** Network topology
 *
 *    100Mb/s,0.5ms|                    |100Mb/s,0.5ms
 * n1--------------|                    |--------------n5
 *    100Mb/s, 1ms |                    |100Mb/s,1ms
 * n2--------------|                    |--------------n5
 *                 |       45Mb/s 2ms   |
 *                 nA------------------nB
 *    100Mb/s, 3ms |                    |100Mb/s,5ms
 * n3--------------|                    |--------------n5
 *    100Mb/s, 5ms |                    |100Mb/s,2ms
 * n4--------------|                    |--------------n5
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

std::stringstream filePlotQueueA;
std::stringstream filePlotQueueB;
std::stringstream filePlotQueueAAvg;
std::stringstream filePlotQueueBAvg;
std::stringstream filePlotPacketArriveA;
std::stringstream filePlotPacketArriveB;
std::stringstream filePlotPacketDropA;
std::stringstream filePlotPacketDropB;

// The sequence number is in bytes not packets
void EnqueueAtRedA(Ptr<const QueueDiscItem> item) {
    TcpHeader tcp;
    Ptr<Packet> pkt = item->GetPacket();
    pkt->PeekHeader(tcp);

    std::ofstream fPlotPacketArrive(filePlotPacketArriveA.str().c_str(), std::ios::out | std::ios::app);
    fPlotPacketArrive << Simulator::Now().GetSeconds() << " " << tcp.GetSequenceNumber() << std::endl;
    fPlotPacketArrive.close();
}

void EnqueueAtRedB(Ptr<const QueueDiscItem> item) {
    TcpHeader tcp;
    Ptr<Packet> pkt = item->GetPacket();
    pkt->PeekHeader(tcp);

    std::ofstream fPlotPacketArrive(filePlotPacketArriveB.str().c_str(), std::ios::out | std::ios::app);
    fPlotPacketArrive << Simulator::Now().GetSeconds() << " " << tcp.GetSequenceNumber() << std::endl;
    fPlotPacketArrive.close();
}


void DroppedAtRedA(Ptr<const QueueDiscItem> item) {
    TcpHeader tcp;
    Ptr<Packet> pkt = item->GetPacket();
    pkt->PeekHeader(tcp);

    std::ofstream fPlotPacketDrop(filePlotPacketDropA.str().c_str(), std::ios::out | std::ios::app);
    fPlotPacketDrop << Simulator::Now().GetSeconds() << " " << tcp.GetSequenceNumber() << std::endl;
    fPlotPacketDrop.close();
}

void DroppedAtRedB(Ptr<const QueueDiscItem> item) {
    TcpHeader tcp;
    Ptr<Packet> pkt = item->GetPacket();
    pkt->PeekHeader(tcp);

    std::ofstream fPlotPacketDrop(filePlotPacketDropB.str().c_str(), std::ios::out | std::ios::app);
    fPlotPacketDrop << Simulator::Now().GetSeconds() << " " << tcp.GetSequenceNumber() << std::endl;
    fPlotPacketDrop.close();
}


//This code is fine for printing average and actual queue size
void CheckQueueASize(Ptr<QueueDisc> queue) {
    uint32_t qsize = StaticCast<RedQueueDisc>(queue)->GetQueueSize();
    avgQueueSize += qsize;
    checkTimes++;

    // check queue size every 1/100 of a second
    Simulator::Schedule(Seconds(0.01), &CheckQueueASize, queue);

    std::ofstream fPlotQueue(filePlotQueueA.str().c_str(), std::ios::out | std::ios::app);
    fPlotQueue << Simulator::Now().GetSeconds() << " " << qsize << std::endl;
    fPlotQueue.close();

    std::ofstream fPlotQueueAvg(filePlotQueueAAvg.str().c_str(), std::ios::out | std::ios::app);
    fPlotQueueAvg << Simulator::Now().GetSeconds() << " " << avgQueueSize / checkTimes << std::endl;
    fPlotQueueAvg.close();
}

void CheckQueueBSize(Ptr<QueueDisc> queue) {
    uint32_t qsize = StaticCast<RedQueueDisc>(queue)->GetQueueSize();
    avgQueueSize += qsize;
    checkTimes++;

    Simulator::Schedule(Seconds(0.01), &CheckQueueBSize, queue);

    std::ofstream fPlotQueue(filePlotQueueB.str().c_str(), std::ios::out | std::ios::app);
    fPlotQueue << Simulator::Now().GetSeconds() << " " << qsize << std::endl;
    fPlotQueue.close();

    std::ofstream fPlotQueueAvg(filePlotQueueBAvg.str().c_str(), std::ios::out | std::ios::app);
    fPlotQueueAvg << Simulator::Now().GetSeconds() << " " << avgQueueSize / checkTimes << std::endl;
    fPlotQueueAvg.close();
}

int main (int argc, char *argv[])
{
    LogComponentEnable ("RedQueueDisc", LOG_LEVEL_INFO);

    std::string pathOut = "P2c";;
    bool writeForPlot = true;
    bool writePcap = false;
    bool flowMonitor = false;

    uint32_t runNumber = 0;
    uint32_t maxPackets = 400;
    uint32_t meanPktSize = 500;
    std::string redLinkDataRate = "45Mbps";
    std::string redLinkDelay = "2ms";
    double qw = 0.002;
    double minTh = 5;
    double maxTh = 15;
    double stopTime = 1.0;

    //Will only save in the directory if enable opts below
    CommandLine cmd;
    cmd.AddValue ("runNumber", "run number for random variable generation", runNumber);
    cmd.AddValue ("pathOut", "Path to save results from --writeForPlot/--writePcap/--writeFlowMonitor", pathOut);
    cmd.AddValue ("maxPackets", "Max packets allowed in the device queue", maxPackets);
    cmd.AddValue ("writeForPlot", "<0/1> to write results for plot (gnuplot)", writeForPlot);
    cmd.AddValue ("writePcap", "<0/1> to write results in pcapfile", writePcap);
    cmd.AddValue ("writeFlowMonitor", "<0/1> to enable Flow Monitor and write their results", flowMonitor);

    //RED params
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
    Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (maxPackets));

    cmd.Parse (argc, argv);

    SeedManager::SetSeed(1);
    SeedManager::SetRun(runNumber);

    //Create nodes
    NS_LOG_INFO ("Create nodes");
    NodeContainer c;
    NodeContainer n[9];
    c.Create (10);
    Names::Add ( "N1", c.Get (0));
    Names::Add ( "N2", c.Get (1));
    Names::Add ( "N3", c.Get (2));
    Names::Add ( "N4", c.Get (3));
    Names::Add ( "N5", c.Get (4));
    Names::Add ( "N6", c.Get (5));
    Names::Add ( "N7", c.Get (6));
    Names::Add ( "N8", c.Get (7));
    Names::Add ( "NA", c.Get (8));
    Names::Add ( "NB", c.Get (9));

    for(uint8_t i=0;i<8;i++) {
        if (i < 4)
        n[i] = NodeContainer(c.Get(i), c.Get(8));
        else
        n[i] = NodeContainer(c.Get(i), c.Get(9));
    }
    n[8] = NodeContainer(c.Get(8), c.Get(9));

    //Install internet stack on all nodes
    NS_LOG_INFO ("Install internet stack on all nodes.");
    InternetStackHelper internet;
    internet.Install (c);

    //Create channels and install devices
    NS_LOG_INFO ("Create channels");
    PointToPointHelper p2p;
    std::string delay[9] = {"0.5ms", "1ms", "3ms", "5ms", "0.5ms", "1ms", "5ms", "2ms", "2ms"};
    NetDeviceContainer devn[9];
    for (int i = 0; i < 9; i++) {
        if(i != 8)
            p2p.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
        else
            p2p.SetDeviceAttribute("DataRate", StringValue("45Mbps"));
        p2p.SetChannelAttribute("Delay", StringValue(delay[i]));
        devn[i] = p2p.Install(n[i]);
    }

    //Install traffic controller to Red Link
    TrafficControlHelper tchRed;
    tchRed.SetRootQueueDisc("ns3::RedQueueDisc");
    Ptr<QueueDisc> redQueueA = (tchRed.Install(devn[8].Get(0))).Get(0);
    Ptr<QueueDisc> redQueueB = (tchRed.Install(devn[8].Get(1))).Get(0);

    //Setup traces
    redQueueA->TraceConnectWithoutContext("Enqueue", MakeCallback(&EnqueueAtRedA));
    redQueueA->TraceConnectWithoutContext("Drop", MakeCallback(&DroppedAtRedA));

    redQueueB->TraceConnectWithoutContext("Enqueue", MakeCallback(&EnqueueAtRedB));
    redQueueB->TraceConnectWithoutContext("Drop", MakeCallback(&DroppedAtRedB));


    //Assign IP Address
    NS_LOG_INFO ("Assign IP Addresses");
    Ipv4AddressHelper ipv4;
    Ipv4InterfaceContainer ip4[9];

    for (uint8_t i=0;i<9;i++) {
        std::stringstream ip;
        ip << "10.1." << i+1 << ".0";
        ipv4.SetBase(ip.str().c_str(), "255.255.255.0");
        ip4[i] = ipv4.Assign(devn[i]);
    }

    //Setup routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    ApplicationContainer sources;

    //Install Sources
    OnOffHelper sourceHelper("ns3::TcpSocketFactory", Address());
    sourceHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    sourceHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    sourceHelper.SetAttribute("DataRate", DataRateValue(DataRate("100Mbps")));
    sourceHelper.SetAttribute("PacketSize", UintegerValue(packetSize));
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 4; ++j) {
            AddressValue remote(InetSocketAddress(ip4[i].GetAddress(0), port));
            sourceHelper.SetAttribute("Remote", remote);
            if (i < 4)
                sources.Add(sourceHelper.Install(n[j + 4].Get(0)));
            else
                sources.Add(sourceHelper.Install(n[j].Get(0)));
            sources.Start(Seconds(0));
        }
    }

    sources.Start(Seconds(0));

    //Install Sinks
    ApplicationContainer sinks;

    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), port));
    for (int i = 0; i < 8; ++i)
        sinks.Add(sinkHelper.Install(n[i].Get(0)));

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

    //Write output
    if (writeForPlot) {
        filePlotQueueA << pathOut << "/" << "redQueueA.plot";
        filePlotQueueAAvg << pathOut << "/" << "redQueueAAvg.plot";
        filePlotQueueB << pathOut << "/" << "redQueueB.plot";
        filePlotQueueBAvg << pathOut << "/" << "redQueueBAvg.plot";
        filePlotPacketArriveA << pathOut << "/" << "PacketNumA.plot";
        filePlotPacketArriveB << pathOut << "/" << "PacketNumB.plot";
        filePlotPacketDropA << pathOut << "/" << "PacketDropA.plot";
        filePlotPacketDropB << pathOut << "/" << "PacketDropB.plot";

        remove(filePlotQueueA.str().c_str());
        remove(filePlotQueueB.str().c_str());
        remove(filePlotQueueAAvg.str().c_str());
        remove(filePlotQueueBAvg.str().c_str());
        Simulator::ScheduleNow(&CheckQueueASize, redQueueA);
        Simulator::ScheduleNow(&CheckQueueBSize, redQueueB);
    }

    Simulator::Stop(Seconds(stopTime));
    Simulator::Run();

    uint32_t totalBytes = 0;

    for (uint32_t i = 0; i < sinks.GetN(); ++i) {
        Ptr<Application> app = sinks.Get(i);
        Ptr<PacketSink> pktSink = DynamicCast<PacketSink>(app);
        uint32_t received = pktSink->GetTotalRx();
        std::cout << "\tSink\t" << i << "\tBytes\t" << received << std::endl;
        totalBytes += received;
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
