#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"
#include "ns3/log.h"

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
    LogComponentEnable("UdpEchoClientApplication",LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication",LOG_LEVEL_INFO);

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
    PointToPointStarHelper star (nSpokes, pointToPoint);

    NS_LOG_INFO ("Instalando a pilha de internet em todos os nós..");
    InternetStackHelper internet;
    star.InstallStack (internet);

    NS_LOG_INFO ("Atribuindo endereços IP.");
    star.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"));


    NS_LOG_INFO ("Gerando aplicacao");

    //
    // Cria um coletor de pacotes no "hub" estrela para receber pacotes
    //
    uint16_t port = 50000;
    Address hubLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
    PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", hubLocalAddress);
    ApplicationContainer hubApp = packetSinkHelper.Install (star.GetHub ());
    hubApp.Start (Seconds (0.0));
    hubApp.Stop (Seconds (10.0));

    //
    //Cria e envia TCP para o hub, um em cada nó de raio.
    //
    OnOffHelper onOffHelper ("ns3::TcpSocketFactory", Address ());
    onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer spokeApps;

    for (uint32_t i = 0; i < star.SpokeCount (); i++)
     {
       AddressValue remoteAddress (InetSocketAddress (star.GetHubIpv4Address (i), port));
       onOffHelper.SetAttribute ("Remote", remoteAddress);
       spokeApps.Add (onOffHelper.Install (star.GetSpokeNode (i)));
     }
     spokeApps.Start (Seconds (0.0));
     spokeApps.Stop (Seconds (10.0));

     NS_LOG_INFO ("Ativando roteamente estatico.");
     //
     // Ativa o roteamento lobal para que possa ser encaminhado os dados pela rede.
     //
     Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

     NS_LOG_INFO ("Ativando tracing em pcap");
     //
     // Rastreamento do pcap em todos os dispositivos p2p em todos os nos.
     //
     AsciiTraceHelper ascii
     pointToPoint.EnablePcapAll ("star");
     pointToPoint.EnableAsciiAll(ascii.CreateFileStream("projeto_tr1"));

     NS_LOG_INFO ("Rodando Simulacao.");
     Simulator::Run ();
     Simulator::Destroy ();
     NS_LOG_INFO ("Done.");

    return 0;
  }
