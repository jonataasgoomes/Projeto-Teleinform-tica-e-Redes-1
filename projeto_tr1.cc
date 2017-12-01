//bliotecas básicas contendo o núcleo do simulador
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>
#include <sstream>

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
 */


using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("simulacao");
int main(int argc,char *argv[]){
	bool verbose = true;
	bool tracing = true;


  // Numero de clientes para cada tipo de dispositivo
  // Variável para atribuir a quantidade mínima de nós (multiponto)
  uint32_t nCsma = 10;
  uint32_t nWifi = 10;
  std::string outputFolder = "output/";
  /** CMD **/
  // Tipo de log definido para a aplicação
  // Variável para receber os parâmetros da linha de comando
  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Disable pcap tracing", tracing);
  cmd.Parse (argc,argv);
  // Verificar se o log será habilitado
  LogComponentEnable("simulacao",LOG_LEVEL_INFO);
  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  // Define e cria nós
	int nP2p = 5;
	NodeContainer p2pNodes[nP2p];

	for(int i=0; i < nP2p-3 ;i++)
		p2pNodes[i].Create(1);
	  p2pNodes[2].Create(4);

	for(int i=0;i< nP2p-3 ;i++)
		p2pNodes[i].Add(p2pNodes[i+1].Get(0));

  // Cria o enlace ponto a ponto entre dois nós
  NS_LOG_INFO ("Instalando a pilha de internet em todos os nós..");
  PointToPointHelper pointToPoint;
  // Configura a taxa da conexão do enlace
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  // Configura o atraso do enlace
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  // Configura o dispositivo de rede
	NetDeviceContainer p2pDevices[nP2p];
	for(int i=0;i<nP2p;i++)
		p2pDevices[i] = pointToPoint.Install(p2pNodes[i]);



  NodeContainer csmaNodes[4];
	CsmaHelper csma[4];
	NetDeviceContainer csmaDevices[4];

	for(int i=0;i<4;i++){
		csmaNodes[i].Add(p2pNodes[0].Get(i));
		csmaNodes[i].Create(nCsma);
  // Configura a taxa da conexão do enlace multiponto
		csma[i].SetChannelAttribute ("DataRate", StringValue("100Mbps"));
  // Configura o atraso do enlace multiponto
		csma[i].SetChannelAttribute ("Delay", TimeValue(NanoSeconds(6560)));

		csmaDevices[i] = csma[i].Install(csmaNodes[i]);
	}

  // Define e cria a quantidade mínima de nós para o enlace
  // sem fio e configura o primeiro nó para ser o AP
	NodeContainer wifiStaNodes[2];
	NodeContainer wifiApNode[2];
	for(int i=0;i<1;i++){
		wifiStaNodes[i].Create(nWifi);
		wifiApNode[i] = p2pNodes[i+1].Get(0);
	}
	wifiStaNodes[1].Create(nWifi);
	wifiApNode[1] = p2pNodes[2].Get(1);

  // Cria o canal e associa-o a camada física
  // da rede sem fio
	YansWifiChannelHelper channel[2];
	YansWifiPhyHelper phy[2];

  // Define a camada de enlace da rede sem fio
  // básica, sem QoS e usando AARF para controlar
  // a taxa do canal de dados
	WifiHelper wifi[2];

  WifiMacHelper macW, macA;

  // Configura o SSID da rede sem fio, com o tipo do MAC
 // e desabilita a sondagem nas STA (estações)
  Ssid ssid = Ssid ("ns-3-ssid");
  macW.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  // Configura o dispositivo de rede no enlace sem fio
	NetDeviceContainer staDevices[4];

  // Configura o SSID da rede sem fio, como o tipo do MAC
  // no AP (ponto de acesso)
	macA.SetType("ns3::ApWifiMac","Ssid",SsidValue(ssid));

	// Configura o dispositivo de rede no enlace sem fio
	NetDeviceContainer apDevices[2];


  // Configura o modelo de mobilidade para as estações,
  // definindo as dimenssões e o layout da grade
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (sqrt(nWifi)),
                                 "LayoutType", StringValue ("RowFirst"));

  // Define e instala o modelo de mobilidade aleatória, tanto
  // para direção quanto para velocidade nas estações
 	mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                           "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));

	for(int i=0;i<2;i++){
		channel[i] = YansWifiChannelHelper::Default();
		phy[i] = YansWifiPhyHelper::Default();

		phy[i].SetChannel(channel[i].Create());
		wifi[i].SetRemoteStationManager("ns3::AarfWifiManager");

		staDevices[i] = wifi[i].Install(phy[i],macW,wifiStaNodes[i]);
		apDevices[i] = wifi[i].Install(phy[i],macA,wifiApNode[i]);

		mobility.Install(wifiStaNodes[i]);

		// Define a posição constante (sem mobilidade) para o AP
		mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
		mobility.Install (wifiApNode[i]);

	}

	// Configurar a pilha de protocolo IP nos nós
	InternetStackHelper stack;
	for(int i=0;i<4;i++)
		stack.Install(csmaNodes[i]);

	for(int i=0;i<2;i++){
		stack.Install(wifiApNode[i]);
		stack.Install(wifiStaNodes[i]);
	}

	// Atribui os endereços IP as interfaces de rede
  // Configura o endereço IP da rede e sua máscara de subrede
  // para os dispositivos dos enlaces ponto a ponto,
  // multiponto e sem fio
	Ipv4AddressHelper address;
	Ipv4InterfaceContainer p2pInterfaces[nP2p];
	Ipv4InterfaceContainer csmaInterfaces[4];
  // Configura o endereço IP da rede e sua máscara de subrede
 NS_LOG_INFO ("Atribuindo endereços IP.");
	for(int i=0;i<nP2p;i++){
		stringstream ss;
		ss << "10.1." << i+1 << ".0";
		address.SetBase(ss.str().c_str(),"255.255.255.0");
		p2pInterfaces[i] = address.Assign(p2pDevices[i]);
	}

	for(int i=0;i<4;i++){
		stringstream ss;
		ss << "10.1." << i+6 << ".0";
		address.SetBase(ss.str().c_str(),"255.255.255.0");
		csmaInterfaces[i] = address.Assign(csmaDevices[i]);
	}

	for(int i=0;i<2;i++){
		stringstream ss;
		ss << "10.1." << i+8 << ".0";
		address.SetBase(ss.str().c_str(),"255.255.255.0");
		address.Assign(staDevices[i]);
		address.Assign(apDevices[i]);
	}

  // Cria o servidor da aplicação de Echo na porta 9
   NS_LOG_INFO ("Gerando aplicacao");
  UdpEchoServerHelper echoServer (9);
  // Configura o nó como servidor da aplicação de Echo
  ApplicationContainer serverApps = echoServer.Install (csmaNodes[0].Get (nCsma));
  // Define o tempo de início e final da aplicação no servidor
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (120.0));


  // SERVIDOR CLIENTE
  // Configura o endereço IP do servidor e a porta
  // no cliente da aplicação de Echo
  UdpEchoClientHelper echoClient (csmaInterfaces[0].GetAddress (nCsma), 9);
  // Configura o número máximo de pacotes enviados pelo cliente
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  // Intervalo de tempo entre pacotes no cliente
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  // Tamanho máximo do pacote no cliente
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));


  // CLIENTS => 10 para cada servidor
  // Configura o cliente da aplicação do enlace sem fio apontando para o servidor no
  // enlace multiponto
  ApplicationContainer clientApps = echoClient.Install (wifiStaNodes[0]);
	for(int i=1;i<2;i++)
		clientApps.Add(echoClient.Install (wifiStaNodes[i]));
	for(int i=0;i<4;i++)
		clientApps.Add(echoClient.Install (csmaNodes[i]));
  // Define o tempo de início e final da aplicação no cliente
  clientApps.Start (Seconds (1.0));
  clientApps.Stop (Seconds (120.0));
  // Configura o roteamento global na rede,
  // cada nó funciona como se fosse um roteador OSPF
  NS_LOG_INFO ("Ativando roteamente estatico.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Habilita o PCAP/Sniffer no enlace ponto a ponto
  NS_LOG_INFO ("Ativando tracing em pcap");
  if (tracing == true){
      pointToPoint.EnablePcapAll (outputFolder+"p2p_projeto1");
			for(int i=0;i<2;i++){
				stringstream ss;
				ss << outputFolder+"p2p_projeto1" << i+8;
				phy[i].EnablePcap (ss.str(), apDevices[i].Get (0));
			}
			for(int i=0;i<4;i++){
				stringstream ss;
				ss << outputFolder+"p2p_projeto1" << i+6;
				csma[i].EnablePcap (ss.str(), csmaDevices[i].Get (0), true);
			}
	}


  //Exportar simulação para netanim
  BaseStationNetDevice b;
  SubscriberStationNetDevice s;
  CsmaNetDevice c;
  UanNetDevice u;

  AnimationInterface anim(outputFolder+"simulacao_tr1.xml");
  anim.SetMaxPktsPerTraceFile(0xFFFFFFFF);
  anim.EnablePacketMetadata(true);
  anim.EnableIpv4RouteTracking (outputFolder+"routingtable-wireless.xml", Seconds (0), Seconds (9), Seconds (0.25));
	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
  NS_LOG_UNCOND("Simulando\n");
  // Rodando simulacao por 120 segundos
   NS_LOG_INFO ("Rodando simulacao aguarde um momento. ...");
  Simulator::Stop (Seconds (120));
  // Carrega a simulação após todas as definições
  Simulator::Run ();

	flowmon.SerializeToXmlFile("statistics.xml", true, true);
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  NS_LOG_UNCOND("\n Dados:");
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter){
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);
    NS_LOG_UNCOND("\nFluxo ID (" << iter->first << ") " << t.sourceAddress << " => " << t.destinationAddress);
    NS_LOG_UNCOND("Taxa de Bytes = " << iter->second.txBytes);
    NS_LOG_UNCOND("Taxa de Pacotes = " << iter->second.txPackets);
    NS_LOG_UNCOND("Taxa de transferencia = " << iter->second.txBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024 / 1024 << " Mbps");
    NS_LOG_UNCOND("Rx Pacotes = " << iter->second.rxPackets);
    NS_LOG_UNCOND("Rx Pacotes = " << iter->second.rxBytes);
    NS_LOG_UNCOND("Taxa de transferencia: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024 / 1024 << " Mbps");
  }

  // Encerra a simulação no final
  Simulator::Destroy ();
  NS_LOG_INFO ("Concluido.");

  return 0;
}
