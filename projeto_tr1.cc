#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"
#include "ns3/log.h"


#include <ns3/netanim-module.h>
#include <ns3/bs-net-device.h>
#include <ns3/csma-module.h>
#include <ns3/uan-module.h>

/*
Requesitos do projeto
  *a) - Quatro LANs Ethernet (padrão 802.3) ou duas redes LANs Ethernet (802.3) e duas redes celulares (3G).
  *b) - Duas LANs Wi-Fi sem fio (802.11x).
  *c) - Uma WAN, em qualquer padrão, interligando todas as LANs.
  *d) - Um minimo de 10 clientes em cada uma das redes.
Obs: Deve existir um servidor de aplicacao que precisa ser acessado por nos/clientes das outras redes.
Topologia Estrela Aplicada
        n..  n..  n..                   n.. n.. n..              .
           \ | /                            \ | /                .
            \|/                              \|/                 .
          n1--- n0  ---n......     .....n1--- n0 ---n..          .
            /|\               .   .          /|\                 .
           / | \               . .          / | \                .
      n10 n..  n..              .       n10 n.. n..              .
                            router
        n..  n..  n..           .       n.. n.. n..              .
           \ | /               . .          \ | /                .
            \|/               .   .          \|/                 .
          n1--- n0  ---n.......    .....n1--- n0 ---n..          .
            /|\                              /|\                 .
           / | \                            / | \                .
      n10 n..  n..                      n10 n.. n..              .
    Wifi A (IP)                       Wifi B (IP)
  */

  using namespace ns3;
  NS_LOG_COMPONENT_DEFINE ("Star");

  int main(int argc, char *argv[])
  {

    //
    // Habilitar Logs
    //
    LogComponentEnable("Star",LOG_LEVEL_INFO);
    // LogComponentEnable ("TcpL4Protocol", LOG_LEVEL_ALL);
    // LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
    std::string outputFolder = "output/";
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("500kb/s"));
    //
    //Numero de ligacoes com a LAN   CSMA e WIFI
    //
    uint32_t nSpokes = 10;

    //uint32_t nWifi = 10;

    CommandLine cmd;
    cmd.AddValue ("nAMDP", "Numero de nos da Estrela", nSpokes);
    cmd.Parse (argc, argv);

    NS_LOG_INFO ("Criando Topologia Estrela");
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
    PointToPointStarHelper star1 (nSpokes, pointToPoint);
    PointToPointStarHelper star2 (nSpokes, pointToPoint);
    PointToPointStarHelper star3 (nSpokes, pointToPoint);
    PointToPointStarHelper star4 (nSpokes, pointToPoint);

    NS_LOG_INFO ("Instalando a pilha de internet em todos os nós..");
    InternetStackHelper internet;
    star1.InstallStack (internet);
    star2.InstallStack (internet);
    star3.InstallStack (internet);
    star4.InstallStack (internet);

    NS_LOG_INFO ("Atribuindo endereços IP.");
    star1.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"));
    star2.AssignIpv4Addresses (Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"));
    star3.AssignIpv4Addresses (Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));
    star4.AssignIpv4Addresses (Ipv4AddressHelper ("10.4.1.0", "255.255.255.0"));


    NS_LOG_INFO ("Gerando aplicacao");

    //
    // Cria um coletor de pacotes no "hub" estrela para receber pacotes
    //
    uint16_t port = 50000;
    Address hubLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
    PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", hubLocalAddress);
    ApplicationContainer hubApp1 = packetSinkHelper.Install (star1.GetHub ());
    ApplicationContainer hubApp2 = packetSinkHelper.Install (star2.GetHub ());
    ApplicationContainer hubApp3 = packetSinkHelper.Install (star3.GetHub ());
    ApplicationContainer hubApp4 = packetSinkHelper.Install (star4.GetHub ());
    hubApp1.Start (Seconds (0.0));
    hubApp2.Start (Seconds (0.0));
    hubApp3.Start (Seconds (0.0));
    hubApp4.Start (Seconds (0.0));

    hubApp1.Stop (Seconds (5.0));
    hubApp2.Stop (Seconds (5.0));
    hubApp3.Stop (Seconds (5.0));
    hubApp4.Stop (Seconds (5.0));

    //
    //Cria e envia TCP para o hub, um em cada nó de raio.
    //
    OnOffHelper onOffHelper ("ns3::TcpSocketFactory", Address ());
    onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer spokeApps1;
    ApplicationContainer spokeApps2;
    ApplicationContainer spokeApps3;
    ApplicationContainer spokeApps4;


    for (uint32_t i = 0; i < star1.SpokeCount (); i++)
    {
      AddressValue remoteAddress (InetSocketAddress (star1.GetHubIpv4Address (i), port));
      onOffHelper.SetAttribute ("Remote", remoteAddress);
      spokeApps1.Add (onOffHelper.Install (star1.GetSpokeNode (i)));
    }
    for (uint32_t i = 0; i < star2.SpokeCount (); i++)
    {
      AddressValue remoteAddress (InetSocketAddress (star2.GetHubIpv4Address (i), port));
      onOffHelper.SetAttribute ("Remote", remoteAddress);
      spokeApps2.Add (onOffHelper.Install (star2.GetSpokeNode (i)));
    }
    for (uint32_t i = 0; i < star3.SpokeCount (); i++)
    {
      AddressValue remoteAddress (InetSocketAddress (star3.GetHubIpv4Address (i), port));
      onOffHelper.SetAttribute ("Remote", remoteAddress);
      spokeApps3.Add (onOffHelper.Install (star3.GetSpokeNode (i)));
    }
    for (uint32_t i = 0; i < star4.SpokeCount (); i++)
      {
      AddressValue remoteAddress (InetSocketAddress (star4.GetHubIpv4Address (i), port));
      onOffHelper.SetAttribute ("Remote", remoteAddress);
      spokeApps4.Add (onOffHelper.Install (star4.GetSpokeNode (i)));
    }


     spokeApps1.Start (Seconds (0.0));
     spokeApps2.Start (Seconds (0.0));
     spokeApps3.Start (Seconds (0.0));
     spokeApps4.Start (Seconds (0.0));

     spokeApps1.Stop (Seconds (5.0));
     spokeApps2.Stop (Seconds (5.0));
     spokeApps3.Stop (Seconds (5.0));
     spokeApps4.Stop (Seconds (5.0));

     NS_LOG_INFO ("Ativando roteamente estatico.");
     //
     // Ativa o roteamento lobal para que possa ser encaminhado os dados pela rede.
     //
     Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

     NS_LOG_INFO ("Ativando tracing em pcap");
     //
     // Rastreamento do pcap em todos os dispositivos p2p em todos os nos.
     //
     // AsciiTraceHelper ascii;
     // pointToPoint.EnablePcapAll (outputFolder+"Star");
     // pointToPoint.EnableAsciiAll(ascii.CreateFileStream("projeto_tr1"));
     star1.BoundingBox(1, 1, 50, 25);
     star2.BoundingBox(1, 1, 150, 25);
     star3.BoundingBox(1, 50, 50, 25);
     star4.BoundingBox(1, 50, 150, 25);
     BaseStationNetDevice b;
     SubscriberStationNetDevice s;
     UanNetDevice u;

     AnimationInterface anim(outputFolder+"anim2.xml");
     anim.SetMaxPktsPerTraceFile(0xFFFFFFFF);
     anim.EnablePacketMetadata(true);
     anim.EnableIpv4RouteTracking (outputFolder+"routingtable.xml", Seconds (0), Seconds (9), Seconds (0.25));

     NS_LOG_INFO ("Rodando simulacao aguarde um momento. ...");
     Simulator::Stop(Seconds(10));
     Simulator::Run ();
     Simulator::Destroy ();
     NS_LOG_INFO ("Concluido.");

    return 0;
}
