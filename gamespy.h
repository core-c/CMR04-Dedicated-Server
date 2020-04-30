
#include "gsmsalg-old2.h"


#define gsGameName				"cmr4pc"
#define	gsGameKey				"t3F9f1"

#define gsMASTER				"master.gamespy.com"
#define gsMASTER_PORT			28900	//28900

#define gsHOST					"motd.gamespy.com"
#define gsHOST_PORT				80

#define gsHEARTBEAT_PORT		27900 //27900  //30000
#define gsHEARTBEAT_TIME		300000  // 300000 ms = 300 seconds = 5 minutes
#define gsHEARTBEAT_1			"\\heartbeat\\%hu\\gamename\\"gsGameName	// heartbeat-poort en "cmr4pc"
//#define gsHEARTBEAT_2_SECURE	"\\validate\\%s\\final\\\\queryid\\1.1"
//#define gsHEARTBEAT_2			"\\gamename\\"gsGameName"\\final\\\\queryid\\1.1"
#define gsHEARTBEAT_2_SECURE	"\\validate\\%s\\final\\"
#define gsHEARTBEAT_2			"\\gamename\\"gsGameName"\\final\\"




// variabelen..
u_char					gsEnabled		= 1;			// communicatie met gamespy?? (alleen als GlobalGameType==1 online)
u_long					gsLastTimeVal	= 0x00000000;	// de timevalue op het tijdstip van laatste keer naar gamespy gezonden..(tbv. in de lijst worden opgenomen)
u_long					ipMaster;

int						gsSocket;
struct sockaddr_in		gsPeer;
struct sockaddr_in		gsPeerHEARTBEAT;


u_char	gamekey[7];
int		len;
int		psz;
int		enctype			= 0;
u_char*	sec				= NULL;			// tbv. beveiligde toegang..
u_char*	key				= NULL;



// functie declaraties..
void gsSendHeartbeat();





void gsSendHeartbeat() {
	int		len;
	int		psz;
	u_char	gamekey[7];
	int		enctype			= 0;
	u_char*	sec				= NULL;			// tbv. beveiligde toegang..
	u_char*	key				= NULL;
	char	*log[100];




		// de peer met de master
		gsPeer.sin_addr.s_addr = ipMaster;// = resolv(gsMASTER);


		//sprintf( log, "master @ %s %s", gsMASTER, inet_ntoa( (*(struct in_addr *)&ipMaster ) ) );
		//Log( log );


//		gsPeer.sin_addr.s_addr = inet_addr( "207.38.11.34" );
//		gsPeer.sin_addr.s_addr = resolv(gsMASTER);
//		gsPeer.sin_port        = htons(gsMASTER_PORT);
		gsPeer.sin_port        = htons(gsMASTER_PORT);
		gsPeer.sin_family      = AF_INET;
		// de verbinding met de gamespy-heartbeat server
//		gsPeerHEARTBEAT.sin_addr.s_addr = inet_addr( "192.168.200.201" );
		gsPeerHEARTBEAT.sin_addr.s_addr = INADDR_ANY;
		gsPeerHEARTBEAT.sin_port        = htons(gsHEARTBEAT_PORT);
		gsPeerHEARTBEAT.sin_family      = AF_INET;

		// socket openen..
		gsSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (gsSocket < 0)  {
			Log2( "gsSocket socket error", "debug" );
			std_err();
		}
//if (setsockopt(gsSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&on, sizeof(on)) < 0) std_err();

		if (setsockopt(gsSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
			Log2( "gsSocket setsockopt error", "debug" );
			std_err();
		}

		// luisteren op de heartbeat poort..
		if (bind(gsSocket, (struct sockaddr *)&gsPeerHEARTBEAT, sizeof(gsPeerHEARTBEAT)) < 0) {
			Log2( "gsSocket bind error", "debug" );
			std_err();
		}


		MsgprintyIP(inet_ntoa(gsPeer.sin_addr), ntohs(gsPeer.sin_port), "<contact gamespy>");




	// Een heartbeat zenden naar de gamespy master..
	//len = snprintf( TXPacket, BUFFSZ, gsHEARTBEAT_1, gsHEARTBEAT_PORT);
	len = snprintf( TXPacket, BUFFSZ, gsHEARTBEAT_1, port);
	

	//sprintf( log, "packet is %d bytes : %s", len, TXPacket );
	//Log ( log );

	if ( len<0 || len>BUFFSZ ) return; //buffer is te klein..
	//if ( sendto(gsSocket, TXPacket, len, 0, (struct sockaddr *)&gsPeer, sizeof(gsPeer)) < 0 ) {

	if ( sendto( gsSocket, TXPacket, len, 0, (struct sockaddr *)&gsPeer, sizeof(gsPeer)) < 0 ) {
			Log2( "gsSocket sendto error", "debug" );
			std_err();
		}
    sleep(0);   // needed

		
	if ( MyTimeout( gsSocket ) < 0 ) {
		MsgprintyIP(inet_ntoa(gsPeer.sin_addr), ntohs(gsPeer.sin_port), "<gamespy firewall error>");
//		fputs("\n"
//			"Error: socket timeout, probably your firewall blocks the UDP packets to or from the master server\n"
//			"\n", stderr);
		//Log( "gsSocket timeout, probably your firewall blocks the UDP packets to or from the master server" );
		// beeindig deze function();

		gsPeer.sin_port        = htons( 27900 );

		len = snprintf( TXPacket, BUFFSZ, gsHEARTBEAT_1, port );
		//len = snprintf( TXPacket, BUFFSZ, gsHEARTBEAT_1, gsHEARTBEAT_PORT );

		//sprintf( log, "RETRY on port 27900, packet is %d bytes : %s", len, TXPacket );
		//Log ( log );

		if ( len<0 || len>BUFFSZ ) return; //buffer is te klein..

		if ( sendto( gsSocket, TXPacket, len, 0, (struct sockaddr *)&gsPeer, sizeof(gsPeer)) < 0 ) {
			Log2( "gsSocket sendto error", "debug" );
			std_err();
		}
		sleep(0);   // needed
		if ( MyTimeout( gsSocket ) < 0 ) {
			MsgprintyIP(inet_ntoa(gsPeer.sin_addr), ntohs(gsPeer.sin_port), "<gamespy firewall error>");
//			fputs("\n"
//				"Error: socket timeout, probably your firewall blocks the UDP packets to or from the master server\n"
//				"\n", stderr);
			//Log( "gsSocket timeout, probably your firewall blocks the UDP packets to or from the master server" );
			return;
		}

	}


	// Het antwoord van de master-server opnemen..
	psz = sizeof(gsPeer);
	len = recvfrom(gsSocket, TXPacket, BUFFSZ, 0, (struct sockaddr *)&gsPeer, &psz );
	if (len <= 0) {
		Log2( "no answer from GameSpy", "debug" );	
		return;
	} else {
		//sprintf(log, "answer from GameSpy: %d bytes", len);
		//Log( log );
	}
	// here we check if the source IP is the same or 
	// is in the same B subnet as the master server
	if ( (gsPeer.sin_addr.s_addr & 0xFFFF) != (ipMaster & 0xFFFF) ) return;
	//
	TXPacket[len] = 0x00;


	// Heartbeat afhandelen, secure of niet??
	strncpy( gamekey, gsGameKey, 6 );
	sec = strstr( TXPacket, "\\secure\\" );
	if( sec ) {
		sec += 8;
		key = gsseckey( sec, gamekey, enctype );
		//printf("\n- Secure: %s  Key:    %s\n", sec, key);
		len = snprintf( TXPacket, BUFFSZ, gsHEARTBEAT_2_SECURE, key );
		//sprintf(log, "secure heartbeat GameSpy: %d bytes", len);
		//Log( log );
	} else {
		len = snprintf( TXPacket, BUFFSZ, gsHEARTBEAT_2 );
		//sprintf(log, "heartbeat GameSpy: %d bytes: %s", len, gsHEARTBEAT_2 );
		//Log( log );
	}
	if ( len<0 || len>BUFFSZ ) return; //buffer is te klein..
	if ( sendto(gsSocket, TXPacket, len, 0, (struct sockaddr *)&gsPeer, sizeof(gsPeer)) < 0 ) std_err();
//	if ( sendto(gsSocket, TXPacket, len, 0, (struct sockaddr *)&gsPeer, sizeof(gsPeer)) < 0 ) std_err();

//
//	// needed to avoid that the sockaddr_in structure
//	// is overwritten with a different host and port
	gsPeer.sin_addr.s_addr = ipMaster;
	gsPeer.sin_port        = htons(gsMASTER_PORT);
	gsPeer.sin_family      = AF_INET;
//

	close( gsSocket );
}

/*

void gsSendHeartbeat() {
	char	*log[100];

	// de peer met de master
	gsPeer.sin_addr.s_addr = ipMaster = resolv(gsMASTER);

	sprintf( log, "master @ %s %s", gsMASTER, inet_ntoa( (*(struct in_addr *)&ipMaster ) ) );
	Log( log );


//		gsPeer.sin_addr.s_addr = inet_addr( "207.38.11.34" );
//		gsPeer.sin_addr.s_addr = resolv(gsMASTER);

	gsPeer.sin_port        = htons(gsMASTER_PORT);
	gsPeer.sin_family      = AF_INET;

	// de verbinding met de gamespy-heartbeat server
//		gsPeerHEARTBEAT.sin_addr.s_addr = inet_addr( "192.168.200.201" );

	gsPeerHEARTBEAT.sin_addr.s_addr = INADDR_ANY;
	gsPeerHEARTBEAT.sin_port        = htons(gsHEARTBEAT_PORT);
	gsPeerHEARTBEAT.sin_family      = AF_INET;

	// Een heartbeat zenden naar de gamespy master..
	//len = snprintf( TXPacket, BUFFSZ, gsHEARTBEAT_1, gsHEARTBEAT_PORT);
	len = snprintf( TXPacket, BUFFSZ, gsHEARTBEAT_1, gsHEARTBEAT_PORT);

	if ( len<0 || len>BUFFSZ ) return; //buffer is te klein..
	//if ( sendto(sd, TXPacket, len, 0, (struct sockaddr *)&gsPeer, sizeof(gsPeer)) < 0 ) {

	sprintf( log, "packet to send is %d bytes: %s", len, TXPacket );
	Log ( log );

	if ( sendto( sd, TXPacket, len, 0, (struct sockaddr *)&gsPeer, sizeof(gsPeer)) < 0 ) {
		Log( "sd sendto error" );
		//std_err();
		return;
	}
	sleep(0);
/*
	if ( timeout( sd ) < 0 ) {
		fputs("\n"
			"Error: sd socket timeout, probably your firewall blocks the UDP packets to or from the master server\n"
			"\n", stderr);
		Log( "sd timeout, probably your firewall blocks the UDP packets to or from the master server" );
		// beeindig deze function();
		return;
	}


	// Het antwoord van de master-server opnemen..
	psz = sizeof(gsPeer);
	len = recvfrom(sd, TXPacket, BUFFSZ, 0, (struct sockaddr *)&gsPeer, &psz );
	if (len <= 0) {
		Log( "no answer from GameSpy" );	
		return;
	} else {
		sprintf(log, "answer from GameSpy: %d bytes", len);
		Log( log );
	}
	// here we check if the source IP is the same or 
	// is in the same B subnet as the master server
	if ( (gsPeer.sin_addr.s_addr & 0xFFFF) != (ipMaster & 0xFFFF) ) return;
	//
	TXPacket[len] = 0x00;


	// Heartbeat afhandelen, secure of niet??
	strncpy( gamekey, gsGameKey, 6 );
	sec = strstr( TXPacket, "\\secure\\" );
	if( sec ) {
		sec += 8;
		key = gsseckey( sec, gamekey, enctype );
		//printf("\n- Secure: %s  Key:    %s\n", sec, key);
		len = snprintf( TXPacket, BUFFSZ, gsHEARTBEAT_2_SECURE, key );
	} else {
		len = snprintf( TXPacket, BUFFSZ, gsHEARTBEAT_2 );
	}
	if ( len<0 || len>BUFFSZ ) return; //buffer is te klein..
	if ( sendto(sd, TXPacket, len, 0, (struct sockaddr *)&gsPeer, sizeof(gsPeer)) < 0 ) std_err();


}
*/





// Een overzicht van alle spellen:
//
// http://motd.gamespy.com/software/services/index.aspx?mode=modified
// met in de lijst:
//		cmr4pc - 1080776890 1078230339 1086913147
//		cmr4pcd - 1080776873 1078230367 1078166257


// info van een spel:
// http://motd.gamespy.com/software/services/index.aspx?mode=full&services=cmr4pc
//
/*
[cmr4pc]
fpmt=1078230339
handoff=GutO3YFZ9Ufs1Z
peerqr=1
peermangle=1
chatchannel=#gsp!cmr4pc
engine=cengine
fullname=Colin McRae Rally 4
maxplayers=8
minplayers=2
lateentry=3
genre=Sim
newsurl=http://games.gamespy.com/6223/
planeturl=http://www.sportplanet.com
hosttemplate=#EXEPATH# -host -hostname "#ROOMNAME#" -name "#PLAYERNAME#"  [$SERVERPW$ -password "#SERVERPW#"]
jointemplate=#EXEPATH# -join #SERVERIP# -name "#PLAYERNAME#" -port #SERVERPORT# [$SERVERPW$ -password "#SERVERPW#"]
monotemplate=#EXEPATH#
gametype=CommandLine
*/



/*
de entry in Luigi's gslist.cfg van CMR04:

		118)  cmr4pc          Colin McRae Rally 4     t3F9f1



gslist -N cmr4pc:

		Gamename:    cmr4pc    Key: t3F9f1
		Filter:
		Enctype:     0
		Resolving    master.gamespy.com ...
		Server:      207.38.11.34:28900
		Secure:      VRISJD
		Validate:    EkUzE/M7
		Receiving:   .. 55 bytes

		-----------------------
		  217.217.30.87   30000
		  201.24.140.55   30000
		 172.211.75.183   30000
		 220.238.178.55   63568
			 84.98.2.19   30000
		 212.205.251.81   62909
		 24.202.228.246   30000
			80.99.94.57   30000


		Online there are 8 servers



zoeken naar de string: "Secure"
inien gevonden:  
	string erna overnemen naar var "sec"

	key = gsseckey(sec, gamekey, enctype);

*/



/*
handoff=GutO3YFZ9Ufs1Z    wordt key  t3F9f1
          | | | | | |
*/




/*
server naar 207.38.8.34 gamespy:

                                     03 85 ce 4a 3d 6c  ."u0l..9.....J=l
0030   6f 63 61 6c 69 70 30 00 31 39 32 2e 31 36 38 2e  ocalip0.192.168.
0040   32 30 30 2e 31 30 30 00 6c 6f 63 61 6c 69 70 31  200.100.localip1
0050   00 31 39 32 2e 31 36 38 2e 31 32 33 2e 31 30 30  .192.168.123.100
0060   00 6c 6f 63 61 6c 69 70 32 00 31 39 32 2e 31 36  .localip2.192.16
0070   38 2e 30 2e 39 39 00 6c 6f 63 61 6c 70 6f 72 74  8.0.99.localport
0080   00 33 30 30 30 30 00 6e 61 74 6e 65 67 00 30 00  .30000.natneg.0.
0090   73 74 61 74 65 63 68 61 6e 67 65 64 00 33 00 67  statechanged.3.g
00a0   61 6d 65 6e 61 6d 65 00 63 6d 72 34 70 63 00 68  amename.cmr4pc.h
00b0   6f 73 74 6e 61 6d 65 00 55 4a 45 20 73 65 72 76  ostname.UJE serv
00c0   65 72 74 00 67 61 6d 65 76 65 72 00 31 2e 30 00  ert.gamever.1.0.
00d0   68 6f 73 74 70 6f 72 74 00 33 30 30 30 30 00 70  hostport.30000.p
00e0   61 73 73 77 6f 72 64 00 30 00 67 61 6d 65 74 79  assword.0.gamety
00f0   70 65 00 31 00 67 61 6d 65 6d 6f 64 65 00 6f 70  pe.1.gamemode.op
0100   65 6e 77 61 69 74 69 6e 67 00 6e 75 6d 70 6c 61  enwaiting.numpla
0110   79 65 72 73 00 31 00 6d 61 78 70 6c 61 79 65 72  yers.1.maxplayer
0120   73 00 38 00 72 61 6c 6c 79 00 30 00 73 74 61 67  s.8.rally.0.stag
0130   65 73 00 31 30 30 00 64 61 6d 61 67 65 00 30 00  es.100.damage.0.
0140   72 61 6e 6b 69 6e 67 00 30 00 63 61 72 74 79 70  ranking.0.cartyp
0150   65 00 30 00 00 00 00 00 00 00 00                 e.0........


207.38.8.34 terug:
                                     fe fd 01 85 ce 4a  .dl.u0."}-.....J
0030   3d 34 39 36 6b 6c 4a 35 33 36 32 46 35 39 45 37  =496klJ5362F59E7
0040   35 33 30 00                                      530.


207.38.8.28 terug:  !!!! let op ander ip
                                     fe fd 02 85 ce 4a  .d.xu0.-.Y.....J
0030   3d 47 61 6d 65 53 70 79 20 46 69 72 65 77 61 6c  =GameSpy Firewal
0040   6c 20 50 72 6f 62 65 20 50 61 63 6b 65 74 2e     l Probe Packet.


server naar 207.38.8.34:
                                     01 85 ce 4a 3d 64  ."u0l..&r....J=d
0030   45 63 33 63 65 68 36 79 34 75 5a 79 4f 71 38 35  Ec3ceh6y4uZyOq85
0040   34 2b 38 49 30 6d 75 00                          4+8I0mu.


server naar 207.38.8.38 terug: !!!let op ander ip
                                     05 85 ce 4a 3d 47  ..u0.x.+.[...J=G
0030   61 6d 65 53 70 79 20 46 69 72 65 77 61 6c 6c 20  ameSpy Firewall 
0040   50 72 6f 62 65 20 50 61 63 6b 65 74 2e           Probe Packet.


207.38.8.34 terug:
                                     fe fd 01 85 ce 4a  .dl.u0."}-.....J
0030   3d 34 39 36 6b 6c 4a 35 33 36 32 46 35 39 45 37  =496klJ5362F59E7
0040   35 33 30 00                                      530.


server naar 207.38.8.34:
                                     01 85 ce 4a 3d 64  ."u0l..&r....J=d
0030   45 63 33 63 65 68 36 79 34 75 5a 79 4f 71 38 35  Ec3ceh6y4uZyOq85
0040   34 2b 38 49 30 6d 75 00                          4+8I0mu.




tijdje later, om de 20 seconden contact maken met gamespy:
server naar 207.38.8.34:

#122		08 85 ce 4a 3d

#661		08 85 ce 4a 3d

#1090
                                     03 85 ce 4a 3d 6c  ."u0l..+.)...J=l
0030   6f 63 61 6c 69 70 30 00 31 39 32 2e 31 36 38 2e  ocalip0.192.168.
0040   32 30 30 2e 31 30 30 00 6c 6f 63 61 6c 69 70 31  200.100.localip1
0050   00 31 39 32 2e 31 36 38 2e 31 32 33 2e 31 30 30  .192.168.123.100
0060   00 6c 6f 63 61 6c 69 70 32 00 31 39 32 2e 31 36  .localip2.192.16
0070   38 2e 30 2e 39 39 00 6c 6f 63 61 6c 70 6f 72 74  8.0.99.localport
0080   00 33 30 30 30 30 00 6e 61 74 6e 65 67 00 30 00  .30000.natneg.0.
0090   67 61 6d 65 6e 61 6d 65 00 63 6d 72 34 70 63 00  gamename.cmr4pc.
00a0   68 6f 73 74 6e 61 6d 65 00 55 4a 45 20 73 65 72  hostname.UJE ser
00b0   76 65 72 74 00 67 61 6d 65 76 65 72 00 31 2e 30  vert.gamever.1.0
00c0   00 68 6f 73 74 70 6f 72 74 00 33 30 30 30 30 00  .hostport.30000.
00d0   70 61 73 73 77 6f 72 64 00 30 00 67 61 6d 65 74  password.0.gamet
00e0   79 70 65 00 31 00 67 61 6d 65 6d 6f 64 65 00 6f  ype.1.gamemode.o
00f0   70 65 6e 77 61 69 74 69 6e 67 00 6e 75 6d 70 6c  penwaiting.numpl
0100   61 79 65 72 73 00 36 00 6d 61 78 70 6c 61 79 65  ayers.6.maxplaye
0110   72 73 00 38 00 72 61 6c 6c 79 00 31 34 00 73 74  rs.8.rally.14.st
0120   61 67 65 73 00 31 33 34 00 64 61 6d 61 67 65 00  ages.134.damage.
0130   30 00 72 61 6e 6b 69 6e 67 00 30 00 63 61 72 74  0.ranking.0.cart
0140   79 70 65 00 30 00 00 00 00 00 00 00 00           ype.0........

#1421		08 85 ce 4a 3d

#1698		08 85 ce 4a 3d

#2321
                                     03 85 ce 4a 3d 6c  ."u0l..+.)...J=l
0030   6f 63 61 6c 69 70 30 00 31 39 32 2e 31 36 38 2e  ocalip0.192.168.
0040   32 30 30 2e 31 30 30 00 6c 6f 63 61 6c 69 70 31  200.100.localip1
0050   00 31 39 32 2e 31 36 38 2e 31 32 33 2e 31 30 30  .192.168.123.100
0060   00 6c 6f 63 61 6c 69 70 32 00 31 39 32 2e 31 36  .localip2.192.16
0070   38 2e 30 2e 39 39 00 6c 6f 63 61 6c 70 6f 72 74  8.0.99.localport
0080   00 33 30 30 30 30 00 6e 61 74 6e 65 67 00 30 00  .30000.natneg.0.
0090   67 61 6d 65 6e 61 6d 65 00 63 6d 72 34 70 63 00  gamename.cmr4pc.
00a0   68 6f 73 74 6e 61 6d 65 00 55 4a 45 20 73 65 72  hostname.UJE ser
00b0   76 65 72 74 00 67 61 6d 65 76 65 72 00 31 2e 30  vert.gamever.1.0
00c0   00 68 6f 73 74 70 6f 72 74 00 33 30 30 30 30 00  .hostport.30000.
00d0   70 61 73 73 77 6f 72 64 00 30 00 67 61 6d 65 74  password.0.gamet
00e0   79 70 65 00 31 00 67 61 6d 65 6d 6f 64 65 00 6f  ype.1.gamemode.o
00f0   70 65 6e 77 61 69 74 69 6e 67 00 6e 75 6d 70 6c  penwaiting.numpl
0100   61 79 65 72 73 00 38 00 6d 61 78 70 6c 61 79 65  ayers.8.maxplaye
0110   72 73 00 38 00 72 61 6c 6c 79 00 31 34 00 73 74  rs.8.rally.14.st
0120   61 67 65 73 00 31 33 34 00 64 61 6d 61 67 65 00  ages.134.damage.
0130   30 00 72 61 6e 6b 69 6e 67 00 30 00 63 61 72 74  0.ranking.0.cart
0140   79 70 65 00 30 00 00 00 00 00 00 00 00           ype.0........

etc etc enz....


Als het spel is afgelopen, wordt dit gestuurd naar gamespy:
#37792
                                     03 85 ce 4a 3d 6c  ."u0l...@>...J=l
0030   6f 63 61 6c 69 70 30 00 31 39 32 2e 31 36 38 2e  ocalip0.192.168.
0040   32 30 30 2e 31 30 30 00 6c 6f 63 61 6c 69 70 31  200.100.localip1
0050   00 31 39 32 2e 31 36 38 2e 31 32 33 2e 31 30 30  .192.168.123.100
0060   00 6c 6f 63 61 6c 69 70 32 00 31 39 32 2e 31 36  .localip2.192.16
0070   38 2e 30 2e 39 39 00 6c 6f 63 61 6c 70 6f 72 74  8.0.99.localport
0080   00 33 30 30 30 30 00 6e 61 74 6e 65 67 00 30 00  .30000.natneg.0.
0090   73 74 61 74 65 63 68 61 6e 67 65 64 00 32 00 67  statechanged.2.g
00a0   61 6d 65 6e 61 6d 65 00 63 6d 72 34 70 63 00 00  amename.cmr4pc..
00b0  

*/
