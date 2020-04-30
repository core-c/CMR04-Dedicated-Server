/*

  CMR04 DS Dedicated Server
  by Ron & Frank
  thanks to Luigi Auriemma
  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mysql.h>
#include "rdcksum.h"
#include <curses.h>
#include <fcntl.h>

//#include <iostream.h>
//using namespace std;


#ifdef WIN32
#include <winsock.h>
#include "winerr.h"

#define close   closesocket
#define ONESEC  1000
#define sleep   Sleep
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>


#define ONESEC  1
#endif


#define VER				"0.9"						// our prog. version
#define MAXBUFF			99							// aantal packet-buffers per speler
#define BUFFSZ			2048						// packet buffersize (for input/receive and output/transmit)
#define HTTPBUFFSZ		16384						// buffersize for HTTP input
#define PORT			30000						// cmr4 host port
#define TIMEOUT			200							// socket timeout in seconden
#define TIMEOUTTCP		1000						// socket timeout in microseconden
#define MAXRESEND		100							// maximaal x een packet verzenden, zonder ACK te krijgen.. op de 100 // dus 9x
#define CMRVER			"\x00\x00"					// default game version ??

#define MODE_LAN		"\x00"						// game mode
#define MODE_ONLINE		"\x01"						//
#define MODE_FIXED		"\x02"						// 2? test this..

#define CHATLINE		"\x00\x00\x00\x00"			// lines chatted (0..)
#define CLIENTLINE		"\x00\x00"					// client msg.linenumber
#define CLIENTHANDLE	"\x00\x00\x00\x00"			// client request handle ??

#define MAXHOST			23							// max length server/hostname
#define CMR4HOSTNAME	"UJE Dedicated Linux"		// the name of the server
#define MAXNICK			15							// max length player nickname
#define CMR4HOSTNICK	"VOTE-BOT"					// the player-name of the host
#define VOTESYSTEM		"VOTE-BOT"					// meldingen van het UJE VOTING SYSTEM komen van deze "speler"
#define FILTERSYSTEM	"VOTE-BOT"					// meldingen van het UJE FILTER SYSTEM komen van deze "speler"
#define MAXPWD			15							// max length password
#define CMR4PASSWORD	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //""	 an empty password by default
#define	MAXPLAYERS		6							// maximaal aantal spelers (8)
#define MINPLAYERS		2							// Minimum aantal spelers nodig om een game te starten
#define MINWAITTIME		45							// tenminste zoveel seconden wachten in de lobby (tussen 2 stages)

#define MAXLASTPOS		100							// de laatste MAXLASTPOS posities per speler onthouden..

#define STATUSLINE		2							// extra info
#define LISTTOP			4							// De regels voor de playerlist
#define LISTBOTTOM		LISTTOP+9
#define TIMINGTOP		LISTBOTTOM
#define TIMINGBOTTOM	TIMINGTOP+0
#define MSGTOP			TIMINGBOTTOM+1				// De regels voor meldingen
#define MSGBOTTOM		MSGTOP+60

#define SCREEN 			"\33[1;1H\33[0;30;47m      UJE CMR04 Dedicated Server v"VER" - by Ron and Frank - special thanks to Luigi Auriemma - \33[1mRide For Fun !!          \33[0m\n"


#define SR(x,y)			rdcksum(x, y); \
	for(i = 3; i; i--) { \
	SEND(x,y); \
	if(!MyTimeout(sd)) break; \
	} \
	if(!i) { \
	fputs("\nError: socket timeout, no reply received\n\n", stdout); \
	exit(1); \
	} \
RECV;
#define SEND(x,y)		if( TrafficOut( sendto(sd, x, y, 0, (struct sockaddr *)&peer2, (socklen_t *) sizeof(peer2)) ) < 0) std_err(); 
#define RECV			len = recvfrom(sd, RXPacket, BUFFSZ, 0, NULL, NULL); \
	if(len < 0) std_err(); \
fputc('.', stdout);



// de grootte van Player.PosRec..een opname van een hele race
#define BLOB_LEN		491520		//1048576						//1024*1024;//65535;


u_long	GlobalEndPos;
u_long	GlobalCurrentCut;


/////////////////////
#include "vars.0.9.h"	//global variables
#include "packets.h"	//pakketjes
#include "screen.h"		//scherm routines
#include "player.h"		//speler definities
#include "database.h"	//MySQL functions
#include "gamespy.h"	//GameSpy
#include "logging.h"	//Logging to file
/////////////////////


#include "sys/signal.h"



// het "on close event"
void OnCloseApplication();

// spel-status controleren..
void CheckGlobalState();
// spel-status ophogen..
void NextGlobalState( int ThisState );


// zend een TXPacket naar een andere speler (dwz. niet naar de zender zelf)
void SendToPlayer( short playerNumber, int PacketLen );
// zend een TXPacket terug naar de zender zelf
void ReplyToPlayer( int PacketLen );
void Send1ToPlayer( short playerNumber, int PacketLen );
void Reply1ToPlayer( int PacketLen );

void DisplayBuffers(); // aantal gebruikte buffers afbeelden
void DisplayList();
void DisplayTiming();
void DisplayStatus();
void DisplayProgress();

// TXPacket overnemen naar ptr_players buffers
void BufferTXPacket( short PlayerNr, int PacketLen, u_short CMD );
// Alle gebufferde packets nog een keer verzenden naar iedere speler..
void BuffersResend();
//Een packet zoeken met een opgegeven CMD en playerNumber..
//het resultaat is de index in Packet[index][BUFFSZ]
int FindBufferedPacket( short PlayerNr, u_short CMD );
// een gebufferd packet bevestigen/verwijderen uit de "opnieuw verzenden"-lijst
u_char AckBufferedPacket( short PlayerNr, u_short CMD );
// de buffers controleren of ze leeg zijn. Allemaal leeg? dan resultaat == -1
int BuffersEmpty();


void MakeIngamePlayerList( short playerNumber );
// zend een playerlist aan iedereen (func. zoekt zelf uit C6/D6 nav. het opgegeven playerNumber)
// Als playerNumber == -1 dan wordt er geen D6 gestuurd (omdat er geen zender is na deletelayer bv.)
void SendAllPlayerList( short playerNumber );
// een serverInfo packet maken, 
// resultaat is de PacketLen van de serverInfo..
int Answer_ServerInfo(u_char *Packet, u_long Handle,
					  u_char *hostname, u_char *gamever, u_char *hostport, u_char *password, u_char *gametype,
					  u_char *gamemode, u_char *numplayers, u_char *maxplayers, u_char *rally, u_char *stages,
					  u_char *damage, u_char *ranking, u_char *cartype);
int Answer_MiniServerInfo(u_char *Packet, u_long Handle,
						  u_char *hostname, u_char *gamever, 
						  u_char *ranking, u_char *damage,
						  u_char *gamemode, u_char *numplayers, u_char *maxplayers);
void Answer_LeaveLobby();
void Server_NumberThing( u_char Nr );
void Server_JoinNr( short PlayerNr );
void Server_StartCountdown( short PlayerNr );
void Server_PingList( short PlayerNr, int withTimestamp );
void Server_GridPresent1();
void Server_GridReady();
void Server_ReplyTimestamp();
void Server_SendPos();
void KickPlayer( short PlayerNr, u_char* aMessage );
void Server_RetireDriver( u_char PlayerNr );
void Server_RetireDrivers();
void Server_Retire();
void CheckRetireHost();
void ServerChat( short PlayerNr, u_char* aSpeaker, u_char* aMessage );
void HostChat( short PlayerNr, u_char *aMessage );
u_char IsStage( u_char aStage );
u_char StrToStageValue( u_char *aStage );
u_char StrToGhostValue( u_char *aGhost );
u_char StrToCarValue( u_char *aCar );
u_char StrToDamageValue( u_char *aDamage );
u_char isClientVoteStatus( short PlayerNr, u_char* aMessage );
u_char isClientVoteKick( short PlayerNr, u_char* aMessage );
u_char isClientVoteGhost( short PlayerNr, u_char* aMessage );
u_char isClientVoteChat( short PlayerNr, u_char* aMessage );
u_char isClientVoteCar( short PlayerNr, u_char* aMessage );
u_char isClientVoteDamage( short PlayerNr, u_char* aMessage );
u_char ClientVoting( short PlayerNr, u_char* aMessage );
u_char ChatForbidden( u_char* aMessage );
void ClientNoNonsense( short PlayerNr, u_char* aMessage );
void ServerChangeStage( u_char aStage );
// Een speler verwijderen. (+ alle arrays/variabelen bijwerken)
void RemovePlayer( short PlayerNr, u_char sendPL );

u_long minisec();
u_char timerPassed( u_long msec );
u_char timerPassedSendBuffers( u_long msec );

u_char CheckNoCut( short PlayerNr, float aPosX, float aPosY, float aPosZ );
u_char LineSegmentIntersection( float P1x,float P1y, float P2x,float P2y, float P3x,float P3y, float P4x,float P4y );


u_char *getCountryStr(short PlayerNr);
u_char *getCarStr(short PlayerNr);
u_char *getCarTypeStr(short PlayerNr);
u_char *getGearboxStr(short PlayerNr);
u_char *getPingStr(short PlayerNr);

char	*log[1024];

// function declarations
int MyTimeout(int sock);
u_long resolv(char *host);
void std_err(void);

// 
u_long	startTimer;
u_long	lastServerTimeStamp;
u_long	lastDisplayTimeStamp;
u_long	GlobalLobbyEnter;
u_long	RemainingWaitTime;
u_long	RaceStartTimeStamp;

u_long	FastestTime;
u_char	FastestNick[16]="\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

u_char	FastestCar;
u_char	timeString[256];



//--- netwerk-verkeer doorvoer meting
int TrafficOut( int PacketLen );
int TrafficIn( int PacketLen );

u_long	TotalBytesOut;
u_long	TotalBytesIn;
u_long	BytesInSec;			// doorvoer per seconde
u_long	BytesOutSec;
//u_long	BytesInMin;			// per minuut
//u_long	BytesOutMin;
//u_long	BytesInHour;		// per uur
//u_long	BytesOutHour;

u_long	TotalBytesOutSec;
//u_long	TotalBytesOutMin;
//u_long	TotalBytesOutHour;
u_long	TotalBytesInSec;
//u_long	TotalBytesInMin;
//u_long	TotalBytesInHour;

u_long	TimerSec;
//u_long	TimerMin;
//u_long	TimerHour;

// ring-buffers tbv. meting over de afgelopen minuut
u_long	BytesInLastMin[60] = { 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0 };
u_long	BytesInLastMinPtr = 0;	// de ptr in de ring-buffer BytesInLastMin
u_long	BytesInLastMinAvg = 0;	// het gemiddeld aantal bytes per seconde, gemeten over de afgelopen minuut
u_long	BytesOutLastMin[60] = { 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0 };
u_long	BytesOutLastMinPtr = 0;	// de ptr in de ring-buffer BytesOutLastMin
u_long	BytesOutLastMinAvg = 0;	// het gemiddeld aantal bytes per seconde, gemeten over de afgelopen minuut
// ring-buffers tbv. meting over het afgelopen uur
u_long	BytesInLastHour[60] = { 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0 };
u_long	BytesInLastHourPtr = 0;
u_long	BytesInLastHourAvg = 0;
u_long	BytesOutLastHour[60] = { 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0 };
u_long	BytesOutLastHourPtr = 0;
u_long	BytesOutLastHourAvg = 0;

u_char	aNumPlayers[300];	// voor 5 minuten bijhouden
u_long	IndexNumPlayers;
float	AvgNumPlayers5min;

u_long	BytesInPeak;
u_long	BytesOutPeak;

#define	KB				1024
#define	MINUTE			60
#define	HOUR			3600
#define	KB_PER_SECOND	KB
#define	KB_PER_MINUTE	(KB * MINUTE)
#define	KB_PER_HOUR		(KB * HOUR)
//---






//application
int main(int argc, char *argv[]) {
	

	for ( i=0; i<300 ; i++ ) {
		aNumPlayers[i]=0;
	}

	//	char	*log[100];
	struct	sigaction sig_act;
	
	sigset_t mask;
    sigemptyset(&mask);
    sig_act.sa_handler = (void *)OnCloseApplication;
    sig_act.sa_flags = 0;
    sigemptyset(&sig_act.sa_mask);
	
	
	GlobalState = 0x00; // bezig met opstarten..
	//GlobalState = 10; // bezig met opstarten..
	
	sigaction(SIGINT, &sig_act, NULL);      /* catch crtl-c */
	
	// het scherm intialiseren
    setbuf(stdout, NULL);
	initdisplay;
    cls();
    fputs( SCREEN, stdout );
	
	//	sprintf( StrReplaceGogogoD6, "minumum players: %d       vote FIN5 to change stage            vote kick \"playername\" to kick a player", MINPLAYERS ) ;	// dit ziet de persoon zelf
	//	sprintf( StrReplaceGogogoD6, "beta server.. http://cmr04.fastfrank.speedxs.nl/          vote like \"vote usa5\""
	sprintf( StrReplaceGogogoD6, "cut=retire http://come.to/cmr04                        %d players minimum" , MINPLAYERS ) ;	// dit ziet de persoon zelf
	//memcpy( StrReplaceGogogoD6, log, strlen( log ) ) ;

	// command prompt argumenten
    if(argc < 2) {
        printf("\n"
            "Usage: %s <hostname> [port( default %d )]\n"
            "\n", argv[0], port);
        exit(1);
    }
	if(argc == 2 ){
		cmr4hostname = argv[1];
	} 
	if(argc == 3) {
		port = atoi(argv[2]);
		cmr4hostname = argv[1];
	}
/*//---------------------
	if (argc==1) {
        printf("\nUsage: %s [iniFilename], ...\n", argv[0]);
        exit(1);
	} else {
		for (i=1; i<argc; i++) {
printf("i = %d", i);
			strcpy( &tmpStr, &argv[i] );
printf("tmpStr = %s", tmpStr);
			ReadIniFile( tmpStr );
			break;//woot..meer servers starten dan..
		}
	}
//---------------------*/

	printf(" Preparing %s on port %d ", cmr4hostname, port );
	
	sprintf( log, "Preparing %s on port %d ", cmr4hostname, port );
	Log2( log, "game" );
	
	ipMaster = resolv(gsMASTER);
	
	// sockets
#ifdef WIN32
    WSADATA    wsadata;
    WSAStartup(MAKEWORD(1,0), &wsadata);
#endif
	
    peer.sin_addr.s_addr  = INADDR_ANY; // resolv();
    peer.sin_port         = htons(port);
    peer.sin_family       = AF_INET;
	
    peer2.sin_addr.s_addr = INADDR_ANY; //htonl("255.255.255.255"); //INADDR_ANY
    peer2.sin_port        = htons(0); //	htons(port)
    peer2.sin_family      = AF_INET; //         AF_INET
	
    sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sd < 0) std_err();
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) std_err();
	//    if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&on, sizeof(on)) < 0) std_err();
	//    peerl.sin_port++;
    if (bind(sd, (struct sockaddr *)&peer, sizeof(peer)) < 0) std_err();
	

	
	if ( GlobalHTTPEnabled==1 ) {
		peerRA.sin_addr.s_addr  = INADDR_ANY; // resolv();
		peerRA.sin_port         = htons(port);
		peerRA.sin_family       = AF_INET;
		
		sdRA = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sdRA < 0) std_err();
		if (setsockopt(sdRA, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) std_err();
	//	if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&on, sizeof(on)) < 0) std_err();
		//    peerl.sin_port++;
		if (bind(sdRA, (struct sockaddr *)&peerRA, sizeof(peerRA)) < 0) std_err();
		if (listen(sdRA, 64) < 0) std_err(); //10
	}    
	


	//--- RELAY eventueel naar een opgegeven IP:Poort
	if ( RelayEnabled != 0 ) {
		// adres instellen
		RelayClient.sin_addr.s_addr  = inet_addr( StrRelayIP );
		RelayClient.sin_port         = htons( RelayPort );
		RelayClient.sin_family       = AF_INET;
		// socket openen..
		RelaySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (RelaySocket < 0) std_err();
		if (setsockopt(RelaySocket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) std_err();
		//if (bind(RelaySocket, (struct sockaddr *)&RelayClient, sizeof(RelayClient)) < 0) std_err();
	}
	//---

	
//	if(fork() != 0)	return 0; /* parent returns OK to shell */

//	(void)signal(SIGCLD, SIG_IGN); /* ignore child death */

//	(void)signal(SIGHUP, SIG_IGN); /* ignore terminal hangups */

//	for(i=0;i<32;i++) (void)close(i);      /* close open files */

//	(void)setpgrp();             /* break away from process group */


	//--- GameSpy aanmelding, alleen als de server online host..
	//	if ( gsEnabled!=0 && GlobalGameType==1) {
	//		Log( "connecting to GameSpy" );
	//	}
	//---
	
	
	
	//--- MySQL database
	sqlTABLES();
	//---
	
	Log2( "SQL init done", "debug" );
	
	// de highscores array in database.h
	//	HighScores = calloc( 4, sizeof(struct THighScore) );
	
	// geheugen alloceren/reserveren voor de lijst met spelers..1
	// + 4 extra spelers[8..11] tbv. de highscore-races
	// +  MAXPLAYERS extra spelers[12..18] voor PersonalRecords van elke speler
	ptr_players = calloc( 8+4+8, sizeof(struct tPlayer) ); 
	//ptr_players = calloc( 8, sizeof(struct tPlayer) );
	Log2( "alloc mem for players OK", "debug" );
	
	/*
	// geheugen alloceren/reserveren voor de lijst met spelers..2
	HostConfig = calloc( 1, sizeof(struct tHostConfig) );
	HostConfig[0].ptr_Players = calloc( 8, sizeof(struct tPlayer) );
	HostConfig[0].Password = cmr4password;
	HostConfig[0].ChatLine = 2;								//aantal regels ge-chat
	free( HostConfig[0].ptr_Players );
	free( HostConfig );
	*/
	
	// Poorten van alle spelers wissen ten teken: "lege entry"
	for ( i=1; i < 8; i++ ) {
		changePortNr( i, 0 );							// peer niet in gebruik
	}
	
	// de eerste speler aanmaken..
	NumPlayers = 1;										// aantal spelers verhogen
	// (1e speler) host record invullen..
	PlayerIndex[0] = 0;									// index in de array ptr_players
	changeNick( 0, cmr4hostnick );
	changeCountry( 0, 0x27 );							// United Nations
	changeCar( 0, 0x0D );								// 205
	changeCarType( 0, 0x02 );							// group B
	changeGearbox( 0, 0x02 );							// manual
	changeChatLine( 0, 0x0006 );						// CMD / ChatLine
	changeStartNr(0, 0xFF);								// ongeldig startnummer..
	changeFinished( 0, 0);								// niet gefinished..
	changeRetired( 0, 0);								// niet retired..
	changeQuit( 0, 0);									// niet gequit..
	changePingCount( 0, 0x00000001 );						// Ping
	changePing( 0, 0x00000001 );						// Ping
	changeAvgPing( 0, 0 );								// gemiddelde ping
	changeID( 0, GlobalJoinCount++ );					// ID (meteen ook ophogen achteraf)
	ptr_players[0].IP		= 0; // "0.0.0.0\x00";
	changePortNr( 0, 0 );								// peer niet in gebruik
	ptr_players[0].BufferedPackets = 0;
	for ( i=0; i < MAXBUFF; i++ ) {
		ptr_players[0].BufferLen[i] = -1;				// buffer niet in gebruik
		ptr_players[0].BufferCMD[i] = 0x0000;			// "leeg" commando
		ptr_players[0].BufferCountdown[i] = 0;
	}
	memcpy( ptr_players[0].LastPos, PosUSA5+3, 27);		// de host 3D-positie op USA5 instellen
	
	if ( GlobalConfigMode == 1 ) {
		// de eerste speler aanmaken..
		NumPlayers = 3;										// aantal spelers verhogen
		changeReady( 0, 1 );
		// (1e speler) host record invullen..
		PlayerIndex[1] = 1;									// index in de array ptr_players
		changeReady( 1, 1 );
		changeNick( 1, "CUTPOINT-1" );
		changeCountry( 1, 0x27 );							// United Nations
		changeCar( 1, 0x0D );								// 205
		changeCarType( 1, 0x02 );							// group B
		changeGearbox( 1, 0x02 );							// manual
		changeChatLine( 1, 0x0006 );						// CMD / ChatLine
		changeStartNr(1, 0xFF);								// ongeldig startnummer..
		changeFinished( 1, 0);								// niet gefinished..
		changeRetired( 1, 0);								// niet retired..
		changeQuit( 1, 0);									// niet gequit..
		changePingCount( 1, 0x00000001 );						// Ping
		changePing( 1, 0x00000010 );						// Ping
		changeAvgPing( 1, 16 );								// gemiddelde ping
		changeID( 1, GlobalJoinCount++ );					// ID (meteen ook ophogen achteraf)
		ptr_players[1].IP		= 0; // "0.0.0.0\x00";
		changePortNr( 1, 0 );								// peer niet in gebruik
		ptr_players[1].BufferedPackets = 0;
		for ( i=0; i < MAXBUFF; i++ ) {
			ptr_players[1].BufferLen[i] = -1;				// buffer niet in gebruik
			ptr_players[1].BufferCMD[i] = 0x0000;			// "leeg" commando
			ptr_players[1].BufferCountdown[i] = 0;
		}
		memcpy( ptr_players[1].LastPos, PosUSA5+3, 27);		// de host 3D-positie op USA5 instellen
		// (1e speler) host record invullen..
		PlayerIndex[2] = 2;									// index in de array ptr_players
		changeReady( 2, 1 );
		changeNick( 2, "CUTPOINT-2" );
		changeCountry( 2, 0x27 );							// United Nations
		changeCar( 2, 0x0D );								// 205
		changeCarType( 2, 0x02 );							// group B
		changeGearbox( 2, 0x02 );							// manual
		changeChatLine( 2, 0x0006 );						// CMD / ChatLine
		changeStartNr(2, 0xFF);								// ongeldig startnummer..
		changeFinished( 2, 0);								// niet gefinished..
		changeRetired( 2, 0);								// niet retired..
		changeQuit( 2, 0);									// niet gequit..
		changePingCount( 2, 0x00000001 );						// Ping
		changePing( 2, 0x00000010 );						// Ping
		changeAvgPing( 2, 16 );								// gemiddelde ping
		changeID( 2, GlobalJoinCount++ );					// ID (meteen ook ophogen achteraf)
		ptr_players[2].IP		= 0; // "0.0.0.0\x00";
		changePortNr( 2, 0 );								// peer niet in gebruik
		ptr_players[2].BufferedPackets = 0;
		for ( i=0; i < MAXBUFF; i++ ) {
			ptr_players[2].BufferLen[i] = -1;				// buffer niet in gebruik
			ptr_players[2].BufferCMD[i] = 0x0000;			// "leeg" commando
			ptr_players[2].BufferCountdown[i] = 0;
		}
		memcpy( ptr_players[2].LastPos, PosUSA5+3, 27);		// de host 3D-positie op USA5 instellen
	}
	
    sleep(1);
    fputs(".", stdout);
	//    sleep(1);
    fputs(".", stdout);
	//    sleep(1);
    fputs(".", stdout);
	//    sleep(1);
    fputs(".", stdout);
	
	// de host is er klaar voor..
	//GlobalState = 0x01;
	GlobalState = stateInit;//init data..
	
    cls();
    fputs( SCREEN, stdout );	// titel-kop
	// informatie afbeelden
	DisplayStatus();	// host info
	DisplayList();		// spelerlijst afbeelden
	//	DisplayTiming();	// race timing
	DisplayProgress();	// propellor
	gotoxy(1,MSGBOTTOM+2);
	
	// druk ff op enter
	//	putchar(13);
	srandom( minisec() );
	
	LastTimeValue = minisec();
	LastTimeValue2 = minisec();
	LastTimeValue3 = minisec()-2500;
	LastTimeValueSendBuf = minisec();
	
	//--- netwerk-verkeer doorvoer meting
	TotalBytesOut=0;
	TotalBytesIn=0;
	BytesInSec=0;
	BytesOutSec=0;
	TotalBytesOutSec = 0;
	TotalBytesInSec = 0;
	
	TimerSec=minisec();
	
	// de buffers tbv. meting over de afgelopen minuut
	for (i=0; i<60; i++) BytesInLastMin[i] = 0;
	BytesInLastMinPtr = 0;						// de ptr in de ring-buffer BytesInLastMin
	BytesInLastMinAvg = 0;						// het gemiddeld aantal bytes per seconde, gemeten over de afgelopen minuut
	for (i=0; i<60; i++) BytesOutLastMin[i] = 0;
	BytesOutLastMinPtr = 0;						// de ptr in de ring-buffer BytesOutLastMin
	BytesOutLastMinAvg = 0;						// het gemiddeld aantal bytes per seconde, gemeten over de afgelopen minuut
	
	// de buffers tbv. meting over het afgelopen uur
	for (i=0; i<60; i++) BytesInLastHour[i] = 0;
	BytesInLastHourPtr = 0;
	BytesInLastHourAvg = 0;
	for (i=0; i<60; i++) BytesOutLastHour[i] = 0;
	BytesOutLastHourPtr = 0;
	BytesOutLastHourAvg = 0;
	
	BytesInPeak = 0;
	BytesOutPeak = 0;
	//---
	
	
	
	Log2( "timers set...", "debug" );
	
	//initscr();
	//cbreak();
	//raw();
	//timeout(0);
	//halfdelay(1);
	
    for(;;) {
		
		
		//tmpULong = initscr();
		//set_term( tmpULong );
		// toets gedrukt??
		//cbreak();
		//raw();
		//timeout(0);
		//qiflush();
		//endwin();


		// TCP
		if ( GlobalHTTPEnabled==1 ) {
			if ( !MyTimeoutTCP(sdRA) ) {
				int sdCN = accept(sdRA, NULL, NULL);
MsgprintyIP(inet_ntoa(peerRA.sin_addr), ntohs(peerRA.sin_port), "<remote admin> accepted" );
 				if ( sdCN > 0 ) {

					
MsgprintyIP(inet_ntoa(peerRA.sin_addr), ntohs(peerRA.sin_port), "<remote admin> voor FORK" );
//					if ( (pid = fork())<0 )	{
					if ( (pid=0)<0 )	{//fop
						//stderr();
					} else {
//MsgprintyIP1(inet_ntoa(peerRA.sin_addr), ntohs(peerRA.sin_port), "<remote admin> na FORK, pid=", pid );
						if(pid == 0) {  /* child */
/*
							(void)signal(SIGCLD, SIG_IGN); // ignore child death 
							(void)signal(SIGHUP, SIG_IGN); // ignore terminal hangups 

							(void)setpgrp();             // break away from process group 
*/

							(void)close(sdRA);

							if ( GlobalHTTPEnabled==1 ) {
								peerRA.sin_addr.s_addr  = INADDR_ANY; // resolv();
								peerRA.sin_port         = htons(port);
								peerRA.sin_family       = AF_INET;
								
								sdRA = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
								if (sdRA < 0) std_err();
								if (setsockopt(sdRA, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) std_err();
							//	if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&on, sizeof(on)) < 0) std_err();
								//    peerl.sin_port++;
								if (bind(sdRA, (struct sockaddr *)&peerRA, sizeof(peerRA)) < 0) std_err();
								if (listen(sdRA, 64) < 0) std_err(); //10
							}    

							// perform read write operations ...
							memset( &BufHTTP, 0x00, HTTPBUFFSZ );
							tmpULong = read( sdCN, BufHTTP, sizeof( BufHTTP ) );
							//MsgprintyIP1(inet_ntoa(peerRA.sin_addr), ntohs(peerRA.sin_port), "<remote admin>", BufHTTP );

							// test welke vars zijn opgegeven
							u_char	*PtrVar;
							u_char	*PtrEOL;
							u_long	Len;

							// de array: let op! "players" heeft een leading underscore omdat anders de zoekactie de tekst eerder vindt in "maxplayers=x"
							#define qvCOUNT 15
							u_char	*QueryVar[qvCOUNT] = {"restart","action","hostname","maxplayers","numplayers",
														"_players0", "_players1", "_players2", "_players3", "_players4", "_players5", "_players6", "_players7",
														"setmaxplayers", "chatout" };
							u_char	*QueryVal[qvCOUNT][256] = {"","","","","","","","","","","","","","",""};
							u_char	*QueryStr[qvCOUNT][256] = {"","","","","","","","","","","","","","",""};

							int		qv;

							memset( &QueryVal, 0x00, sizeof(QueryVal) );
							memset( &QueryStr, 0x00, sizeof(QueryStr) );

        sleep(0);       /* to allow socket to drain */

							// zoek het begin van de querystring..
							// Van achteraan beginnen, en naar voor zoeken op een EOL.
							PtrVar = memrchr( BufHTTP, 13, tmpULong );
							if ( PtrVar!=NULL) {
								// iets gevonden.. 1 byte verder begint de qs.
								PtrVar++;
								PtrVar++;
								Len = strlen(PtrVar);
								memset( &tmpString, 0x00, sizeof(tmpString) );
								//strncpy( &tmpString, PtrVar, Len );
								//strcpy( &tmpString, PtrVar );
								memcpy( &tmpString, PtrVar, Len );
							}

							// de querystring urldecoden..
							long val;
							for (i=0;i<Len;i++) {
								switch( tmpString[i] ) {
								case '\%':
									val = strtol(tmpString+i+1,(char**)NULL,16);
									tmpString[i] = val;
									// de rest aanschuiven..
									memmove( tmpString+i+1, tmpString+i+3, Len-i-2 );
									Len -= 2;
									memset( tmpString+Len, 0, 2 );
									break;
								case '+':
									tmpString[i] = ' ';
									break;
								}
							}


							qv = 0;
							while (qv<qvCOUNT) {
//								QueryVal[qv] = cgi->Fetch(QueryVar[qv]);

								if ( (PtrVar=strstr( tmpString, QueryVar[qv] ))!=NULL ) {
									PtrVar += strlen(QueryVar[qv])+1;
			//MsgprintyIP2(inet_ntoa(peerRA.sin_addr), ntohs(peerRA.sin_port), "<remote admin restart PtrVar=>", (u_long)PtrVar );

									memset( &tmpStr, 0x00, sizeof(tmpStr) );
									if ( (PtrEOL=memchr( PtrVar, '&', tmpULong ))!=NULL ) {
										// ..var..
										Len = (u_long)(PtrEOL-PtrVar);
										memcpy( &tmpStr, PtrVar, Len );
									} else {
										// laatste var
										strcpy( &tmpStr, PtrVar );
										//tmpStr[Len] = 0x00;

									}

									// waarde onthouden..
									strcpy( QueryVal[qv], tmpStr );//er is genoeg geheugen gealloceerd [256]... anders pof!..

			//sprintf( log, "Var:Value = %s:%s", QueryVar[qv], tmpStr );
			//MsgprintyIP1(inet_ntoa(peerRA.sin_addr), ntohs(peerRA.sin_port), "<remote admin> ", log );
			//MsgprintyIP1(inet_ntoa(peerRA.sin_addr), ntohs(peerRA.sin_port), "<remote admin> ", tmpStr );
								} 
								
								
								if (QueryVal[qv]) {

									if ( strlen(QueryVal[qv])==0 ) {
										// readonly
										if (QueryVar[qv]=="hostname") {
											memcpy( &QueryStr[qv], cmr4hostname, strlen(cmr4hostname) );
										} else
										if (QueryVar[qv]=="maxplayers") {
											sprintf(log,"%d", MaxPlayers);
											memcpy( &QueryStr[qv], log, strlen(log) );
										}  else
										if (QueryVar[qv]=="numplayers") {
											sprintf(log,"%d",NumPlayers);
											memcpy( &QueryStr[qv], log, strlen(log) );
										} else
										if (QueryVar[qv]=="_players0") {
											sprintf(log,"%s",getNick(0));
											memcpy( &QueryStr[qv], log, strlen(log) );
										} else
										if (QueryVar[qv]=="_players1") {
											if (NumPlayers>=1 && 1<=MaxPlayers) {
												sprintf(log,"%s",getNick(1));
												memcpy( &QueryStr[qv], log, strlen(log) );
											}
										} else
										if (QueryVar[qv]=="_players2") {
											if (NumPlayers>=2 && 2<=MaxPlayers) {
												sprintf(log,"%s",getNick(2));
												memcpy( &QueryStr[qv], log, strlen(log) );
											}
										} else
										if (QueryVar[qv]=="_players3") {
											if (NumPlayers>=3 && 3<=MaxPlayers) {
												sprintf(log,"%s",getNick(3));
												memcpy( &QueryStr[qv], log, strlen(log) );
											}
										} else
										if (QueryVar[qv]=="_players4") {
											if (NumPlayers>=4 && 4<=MaxPlayers) {
												sprintf(log,"%s",getNick(4));
												memcpy( &QueryStr[qv], log, strlen(log) );
											}
										} else
										if (QueryVar[qv]=="_players5") {
											if (NumPlayers>=5 && 5<=MaxPlayers) {
												sprintf(log,"%s",getNick(5));
												memcpy( &QueryStr[qv], log, strlen(log) );
											}
										} else
										if (QueryVar[qv]=="_players6") {
											if (NumPlayers>=6 && 6<=MaxPlayers) {
												sprintf(log,"%s",getNick(6));
												memcpy( &QueryStr[qv], log, strlen(log) );
											}
										} else
										if (QueryVar[qv]=="_players7") {
											if (NumPlayers>=7 && 7<=MaxPlayers) {
												sprintf(log,"%s",getNick(7));
												memcpy( &QueryStr[qv], log, strlen(log) );
											}
										}
									} else {
							//						memcpy( &QueryStr[qv], QueryVal[qv], strlen(QueryVal[qv]) );
										if (QueryVar[qv]!="hostname" &&
											QueryVar[qv]!="maxplayers" &&
											QueryVar[qv]!="numplayers" &&
											QueryVar[qv]!="_players0" &&
											QueryVar[qv]!="_players1" &&
											QueryVar[qv]!="_players2" &&
											QueryVar[qv]!="_players3" &&
											QueryVar[qv]!="_players4" &&
											QueryVar[qv]!="_players5" &&
											QueryVar[qv]!="_players6" &&
											QueryVar[qv]!="_players7") {

											// editable
											tmpUChar = 0; //flag
											if (QueryVar[qv]=="setmaxplayers") {
												if ( strlen(QueryVal[qv])>0 ) {
													tmpUShort = (u_short)atol(QueryVal[qv]);
													if ( GlobalConfigMode==0 ) {
														if ( tmpUShort>1 && tmpUShort<=7 ) {
															MaxPlayers = tmpUShort;
															tmpUChar = 1; //flag
														} else {
															tmpUShort = MaxPlayers;
														}
													} else {
														if ( tmpUShort>3 && tmpUShort<=7 ) {
															MaxPlayers = tmpUShort;
															tmpUChar = 1; //flag
														} else {
															tmpUShort = MaxPlayers;
														}

													}
													if ( tmpUChar == 0 ) {
														sprintf(log,"%d", tmpUShort);
														memcpy( &QueryStr[qv], log, strlen(log) );
													} else {
														strcpy(QueryStr[qv],"");
													}
												} else {
													strcpy(QueryStr[qv],"");
												}
											} else
											if (QueryVar[qv]=="chatout") {
												if ( strlen(QueryVal[qv])>0 ) {
													// zend een chat naar de spelers..
													ServerChat( -1, "ADMIN", QueryVal[qv] );
												}
												strcpy(QueryStr[qv],"");
											}
										}
									}

								}
							
								qv++;
							}
					//
			//				sprintf( tmpStr, "200 OK HTTP/1.0\r\n\r\n<html><head><title>UJE CMR04 DS remote admin</title><link href=\"http://cmr04.fastfrank.speedxs.nl:81/cmr04/normal.css\" rel=\"stylesheet\" type=\"text/css\"></head><body><h1>CMR04 server: %s</h1><hr><form action=\"/\" name=F1 method=POST><input type=submit value=restart name=restart></form></body></html>", cmr4hostname ) ;
							sprintf( tmpStr, "<html>
								<head>
								<title>UJE CMR04 DS remote admin</title>
								<link href=\"http://cmr04.fastfrank.speedxs.nl:81/cmr04/normal.css\" rel=\"stylesheet\" type=\"text/css\">
								</head>
								<body>
								<h1>CMR04 server: %s</h1>
								<hr><form action=\"/\" name=F1 method=POST>
								<table><tr>
								<td valign=top style=\"border-width:1px;border-style:solid;border-color:#000000;\"><input type=text value=start name=action><br>
												<input type=submit value=restart name=restart></td>
								<td valign=top>Hostname:<br><input type=text value=\"%s\" name=hostname readonly></td>
								<td valign=top>MaxPlayers:<br><input type=text value=\"%s\" name=maxplayers readonly>
														<br><input type=text value=\"%s\" name=setmaxplayers></td>
								<td valign=top>NumPlayers:<br><input type=text value=\"%s\" name=numplayers readonly></td>
								<td valign=top>Players:<br>
												<input type=text value=\"%s\" name=_players0 readonly><br>
												<input type=text value=\"%s\" name=_players1 readonly><br>
												<input type=text value=\"%s\" name=_players2 readonly><br>
												<input type=text value=\"%s\" name=_players3 readonly><br>
												<input type=text value=\"%s\" name=_players4 readonly><br>
												<input type=text value=\"%s\" name=_players5 readonly><br>
												<input type=text value=\"%s\" name=_players6 readonly><br>
												<input type=text value=\"%s\" name=_players7 readonly></td>
								</tr><tr>
								<td colspan=5>chat:<input type=text value=\"%s\" name=chatout size=64 maxlength=64></td>
								</tr></table>
								</form>
								</body>
								</html>", 
								cmr4hostname, QueryStr[2], QueryStr[3], QueryStr[13], QueryStr[4], 
								QueryStr[5], QueryStr[6], QueryStr[7], QueryStr[8], QueryStr[9], QueryStr[10], QueryStr[11], QueryStr[12],
								QueryStr[14] ) ;
							tmpULong = strlen( tmpStr );
							write( sdCN, tmpStr, tmpULong );
							(void)close(sdCN);

						} else {        /* parent */

							(void)close(sdCN);

						}
					}

					
/*
					// perform read write operations ...
					memset( &BufHTTP, 0x00, HTTPBUFFSZ );
					tmpULong = read( sdCN, BufHTTP, sizeof( BufHTTP ) );
					MsgprintyIP1(inet_ntoa(peerRA.sin_addr), ntohs(peerRA.sin_port), "<remote admin>", BufHTTP );

					// test welke vars zijn opgegeven
					u_char	*PtrVar;
					u_char	*PtrEOL;
					u_long	Len;

					// de array: let op! "players" heeft een leading underscore omdat anders de zoekactie de tekst eerder vindt in "maxplayers=x"
					#define qvCOUNT 15
					u_char	*QueryVar[qvCOUNT] = {"restart","action","hostname","maxplayers","numplayers",
												"_players0", "_players1", "_players2", "_players3", "_players4", "_players5", "_players6", "_players7",
												"setmaxplayers", "chatout" };
					u_char	*QueryVal[qvCOUNT][256] = {"","","","","","","","","","","","","","",""};
					u_char	*QueryStr[qvCOUNT][256] = {"","","","","","","","","","","","","","",""};

					int		qv;

					memset( &QueryVal, 0x00, sizeof(QueryVal) );
					memset( &QueryStr, 0x00, sizeof(QueryStr) );



					// zoek het begin van de querystring..
					// Van achteraan beginnen, en naar voor zoeken op een EOL.
					PtrVar = memrchr( BufHTTP, 13, tmpULong );
					if ( PtrVar!=NULL) {
						// iets gevonden.. 1 byte verder begint de qs.
						PtrVar++;
						PtrVar++;
						Len = strlen(PtrVar);
						memset( &tmpString, 0x00, sizeof(tmpString) );
						//strncpy( &tmpString, PtrVar, Len );
						//strcpy( &tmpString, PtrVar );
						memcpy( &tmpString, PtrVar, Len );
					}
	MsgprintyIP1(inet_ntoa(peerRA.sin_addr), ntohs(peerRA.sin_port), "<remote admin QueryStr>", tmpString );











					// doorzoek de querystring
					qv = 0;
					while (qv<qvCOUNT) {
						if ( (PtrVar=strstr( tmpString, QueryVar[qv] ))!=NULL ) {
							PtrVar += strlen(QueryVar[qv])+1;
	//MsgprintyIP2(inet_ntoa(peerRA.sin_addr), ntohs(peerRA.sin_port), "<remote admin restart PtrVar=>", (u_long)PtrVar );

							memset( &tmpStr, 0x00, sizeof(tmpStr) );
							if ( (PtrEOL=memchr( PtrVar, '&', tmpULong ))!=NULL ) {
								// ..var..
								Len = (u_long)(PtrEOL-PtrVar);
								memcpy( &tmpStr, PtrVar, Len );
							} else {
								// laatste var
								strcpy( &tmpStr, PtrVar );
								//tmpStr[Len] = 0x00;

							}

							// waarde onthouden..
							strcpy( QueryVal[qv], tmpStr );//er is genoeg geheugen gealloceerd [256]... anders pof!..

	sprintf( log, "Var:Value = %s:%s", QueryVar[qv], tmpStr );
	MsgprintyIP1(inet_ntoa(peerRA.sin_addr), ntohs(peerRA.sin_port), "<remote admin> ", log );
	//MsgprintyIP1(inet_ntoa(peerRA.sin_addr), ntohs(peerRA.sin_port), "<remote admin> ", tmpStr );
						} 
						qv++;
					}


					// test of een leeg formulier-veld is gepost..
					qv = 0;
					while (qv<qvCOUNT) {
						if ( strlen(QueryVal[qv])==0 ) {
							// readonly
							if (QueryVar[qv]=="hostname") {
								memcpy( &QueryStr[qv], cmr4hostname, strlen(cmr4hostname) );
							} else
							if (QueryVar[qv]=="maxplayers") {
								sprintf(log,"%d", MaxPlayers);
								memcpy( &QueryStr[qv], log, strlen(log) );
							}  else
							if (QueryVar[qv]=="numplayers") {
								sprintf(log,"%d",NumPlayers);
								memcpy( &QueryStr[qv], log, strlen(log) );
							} else
							if (QueryVar[qv]=="_players0") {
								sprintf(log,"%s",getNick(0));
								memcpy( &QueryStr[qv], log, strlen(log) );
							} else
							if (QueryVar[qv]=="_players1") {
								if (NumPlayers>=1 && 1<=MaxPlayers) {
									sprintf(log,"%s",getNick(1));
									memcpy( &QueryStr[qv], log, strlen(log) );
								}
							} else
							if (QueryVar[qv]=="_players2") {
								if (NumPlayers>=2 && 2<=MaxPlayers) {
									sprintf(log,"%s",getNick(2));
									memcpy( &QueryStr[qv], log, strlen(log) );
								}
							} else
							if (QueryVar[qv]=="_players3") {
								if (NumPlayers>=3 && 3<=MaxPlayers) {
									sprintf(log,"%s",getNick(3));
									memcpy( &QueryStr[qv], log, strlen(log) );
								}
							} else
							if (QueryVar[qv]=="_players4") {
								if (NumPlayers>=4 && 4<=MaxPlayers) {
									sprintf(log,"%s",getNick(4));
									memcpy( &QueryStr[qv], log, strlen(log) );
								}
							} else
							if (QueryVar[qv]=="_players5") {
								if (NumPlayers>=5 && 5<=MaxPlayers) {
									sprintf(log,"%s",getNick(5));
									memcpy( &QueryStr[qv], log, strlen(log) );
								}
							} else
							if (QueryVar[qv]=="_players6") {
								if (NumPlayers>=6 && 6<=MaxPlayers) {
									sprintf(log,"%s",getNick(6));
									memcpy( &QueryStr[qv], log, strlen(log) );
								}
							} else
							if (QueryVar[qv]=="_players7") {
								if (NumPlayers>=7 && 7<=MaxPlayers) {
									sprintf(log,"%s",getNick(7));
									memcpy( &QueryStr[qv], log, strlen(log) );
								}
							}
						} else {
	//						memcpy( &QueryStr[qv], QueryVal[qv], strlen(QueryVal[qv]) );
							if (QueryVar[qv]!="hostname" &&
								QueryVar[qv]!="maxplayers" &&
								QueryVar[qv]!="numplayers" &&
								QueryVar[qv]!="_players0" &&
								QueryVar[qv]!="_players1" &&
								QueryVar[qv]!="_players2" &&
								QueryVar[qv]!="_players3" &&
								QueryVar[qv]!="_players4" &&
								QueryVar[qv]!="_players5" &&
								QueryVar[qv]!="_players6" &&
								QueryVar[qv]!="_players7") {

								// editable
								tmpUChar = 0; //flag
								if (QueryVar[qv]=="setmaxplayers") {
									if ( strlen(QueryVal[qv])>0 ) {
										tmpUShort = (u_short)atol(QueryVal[qv]);
										if ( GlobalConfigMode==0 ) {
											if ( tmpUShort>1 && tmpUShort<=7 ) {
												MaxPlayers = tmpUShort;
												tmpUChar = 1; //flag
											} else {
												tmpUShort = MaxPlayers;
											}
										} else {
											if ( tmpUShort>3 && tmpUShort<=7 ) {
												MaxPlayers = tmpUShort;
												tmpUChar = 1; //flag
											} else {
												tmpUShort = MaxPlayers;
											}

										}
										if ( tmpUChar == 0 ) {
											sprintf(log,"%d", tmpUShort);
											memcpy( &QueryStr[qv], log, strlen(log) );
										} else {
											strcpy(QueryStr[qv],"");
										}
									} else {
										strcpy(QueryStr[qv],"");
									}
								} else
								if (QueryVar[qv]=="chatout") {
									if ( strlen(QueryVal[qv])>0 ) {
										// zend een chat naar de spelers..
										ServerChat( -1, "ADMIN", QueryVal[qv] );
									}
									strcpy(QueryStr[qv],"");
								}
							}
						}
						qv++;
					}
*/


/*
					//
	//				sprintf( tmpStr, "200 OK HTTP/1.0\r\n\r\n<html><head><title>UJE CMR04 DS remote admin</title><link href=\"http://cmr04.fastfrank.speedxs.nl:81/cmr04/normal.css\" rel=\"stylesheet\" type=\"text/css\"></head><body><h1>CMR04 server: %s</h1><hr><form action=\"/\" name=F1 method=POST><input type=submit value=restart name=restart></form></body></html>", cmr4hostname ) ;
					sprintf( tmpStr, "<html>
						<head>
						<title>UJE CMR04 DS remote admin</title>
						<link href=\"http://cmr04.fastfrank.speedxs.nl:81/cmr04/normal.css\" rel=\"stylesheet\" type=\"text/css\">
						</head>
						<body>
						<h1>CMR04 server: %s</h1>
						<hr><form action=\"/\" name=F1 method=POST>
						<table><tr>
						<td valign=top style=\"border-width:1px;border-style:solid;border-color:#000000;\"><input type=text value=start name=action><br>
										<input type=submit value=restart name=restart></td>
						<td valign=top>Hostname:<br><input type=text value=\"%s\" name=hostname readonly></td>
						<td valign=top>MaxPlayers:<br><input type=text value=\"%s\" name=maxplayers readonly>
												<br><input type=text value=\"%s\" name=setmaxplayers></td>
						<td valign=top>NumPlayers:<br><input type=text value=\"%s\" name=numplayers readonly></td>
						<td valign=top>Players:<br>
										<input type=text value=\"%s\" name=_players0 readonly><br>
										<input type=text value=\"%s\" name=_players1 readonly><br>
										<input type=text value=\"%s\" name=_players2 readonly><br>
										<input type=text value=\"%s\" name=_players3 readonly><br>
										<input type=text value=\"%s\" name=_players4 readonly><br>
										<input type=text value=\"%s\" name=_players5 readonly><br>
										<input type=text value=\"%s\" name=_players6 readonly><br>
										<input type=text value=\"%s\" name=_players7 readonly></td>
						</tr><tr>
						<td colspan=5>chat:<input type=text value=\"%s\" name=chatout size=64 maxlength=64></td>
						</tr></table>
						</form>
						</body>
						</html>", 
						cmr4hostname, QueryStr[2], QueryStr[3], QueryStr[13], QueryStr[4], 
						QueryStr[5], QueryStr[6], QueryStr[7], QueryStr[8], QueryStr[9], QueryStr[10], QueryStr[11], QueryStr[12],
						QueryStr[14] ) ;
					tmpULong = strlen( tmpStr );
					write( sdCN, tmpStr, tmpULong );

					//shutdown(sdCN, 2);
					close(sdCN);
*/
				}

//				continue;//main-loop
MsgprintyIP(inet_ntoa(peerRA.sin_addr), ntohs(peerRA.sin_port), "<remote admin> acc" );
/*
		sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sd < 0) std_err();
		if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) std_err();
		peer.sin_addr.s_addr  = INADDR_ANY; // resolv();
		peer.sin_port         = htons(port);
		peer.sin_family       = AF_INET;
		
		//		if (bind(sd, (struct sockaddr *)&peer, sizeof(peer)) < 0) std_err();
		bind(sd, (struct sockaddr *)&peer, sizeof(peer));
*/

			}
		}		
		

		if ( GlobalConfigMode == 1 ) {
			char c;
			int n, tem;
			//tmpInt = getc(stdin);
			//			if ( tmpInt==81 ) {
			//				OnCloseApplication();
			//			}
			tem = fcntl(0, F_GETFL, 0);
			fcntl (0, F_SETFL, (tem | O_NDELAY));
			n = read(0, &c, 1);
			if (n > 0) {
				//tmpInt = getc(stdin);
				sprintf( log, "%d" , c );
				MsgprintyIP1("", 0, "<key>", log);
				switch(c) {
				case 81:
					OnCloseApplication();
					break;
				case 0x31: //"1"
					// lastpos van speler[3] naar speler[1] overnemen
					memcpy(&ptr_players[1].LastPos, &ptr_players[3].LastPos, 27);
					break;
				case 0x32: //"2"
					// lastpos van speler[3] naar speler[2] overnemen
					memcpy(&ptr_players[2].LastPos, &ptr_players[3].LastPos, 27);
					break;
					
				case 0x33: //"3"
					// lastpos van speler[3] naar speler[1] overnemen
					memcpy(&ptr_players[1].LastPos, &ptr_players[4].LastPos, 27);
					break;
				case 0x34: //"4"
					// lastpos van speler[3] naar speler[2] overnemen
					memcpy(&ptr_players[2].LastPos, &ptr_players[4].LastPos, 27);
					break;
					
				case 99: //"c"  toggle NO-CUTS detection
					if ( GlobalNoCutsEnabled == 1 ) {
						GlobalNoCutsEnabled = 0;
					} else {
						GlobalNoCutsEnabled = 1;
					}
					break;
					
				case 115: //"s"  save current CUTS-points
					memcpy(&aID, &GlobalCuts[GlobalCurrentCut].ID, 2);
					memcpy(&aPX1, &ptr_players[1].LastPos[12], 4);
					memcpy(&aPY1, &ptr_players[1].LastPos[16], 4);
					memcpy(&aPZ1, &ptr_players[1].LastPos[20], 4);
					memcpy(&aPX2, &ptr_players[2].LastPos[12], 4);
					memcpy(&aPY2, &ptr_players[2].LastPos[16], 4);
					memcpy(&aPZ2, &ptr_players[2].LastPos[20], 4);
					memcpy(&aPercentage1, &ptr_players[1].LastPos[24], 2);
					memcpy(&aPercentage2, &ptr_players[2].LastPos[24], 2);
					
					sqlUpdateCut( aID, GlobalStages, aPercentage1, aPercentage2, aPX1, aPY1, aPZ1, aPX2, aPY2, aPZ2 );
					break;
				case 97: //"a"
					// opslaan/toevoegen in de DB	
					//             000102030405060708090011220304050607080900010203 456789012345678901234567890
					//				                   11111111111111111112222         2          3      
					//0500d9ca0000dafde7fe3cfe8ceb2645 337ecd43 71756245 9f00 02 
					memcpy(&aPX1, &ptr_players[1].LastPos[12], 4);
					memcpy(&aPY1, &ptr_players[1].LastPos[16], 4);
					memcpy(&aPZ1, &ptr_players[1].LastPos[20], 4);
					memcpy(&aPX2, &ptr_players[2].LastPos[12], 4);
					memcpy(&aPY2, &ptr_players[2].LastPos[16], 4);
					memcpy(&aPZ2, &ptr_players[2].LastPos[20], 4);
					memcpy(&aPercentage1, &ptr_players[1].LastPos[24], 2);
					memcpy(&aPercentage2, &ptr_players[2].LastPos[24], 2);
					GlobalCurrentCut = GlobalCutsCount++;
					GlobalCuts[GlobalCurrentCut].ID = sqlAddCut( GlobalStages, aPercentage1, aPercentage2, aPX1, aPY1, aPZ1, aPX2, aPY2, aPZ2 );
					break;
				case 110: //  n   next cut laten zien
					
					memcpy(&ptr_players[1].LastPos[12], &GlobalCuts[GlobalCurrentCut].X1, 4);
					memcpy(&ptr_players[1].LastPos[16], &GlobalCuts[GlobalCurrentCut].Y1, 4);
					memcpy(&ptr_players[1].LastPos[20], &GlobalCuts[GlobalCurrentCut].Z1, 4);
					memcpy(&ptr_players[2].LastPos[12], &GlobalCuts[GlobalCurrentCut].X2, 4);
					memcpy(&ptr_players[2].LastPos[16], &GlobalCuts[GlobalCurrentCut].Y2, 4);
					memcpy(&ptr_players[2].LastPos[20], &GlobalCuts[GlobalCurrentCut].Z2, 4);
					memcpy(&ptr_players[1].LastPos[24], &GlobalCuts[GlobalCurrentCut].Percentage1, 2);
					memcpy(&ptr_players[2].LastPos[24], &GlobalCuts[GlobalCurrentCut].Percentage2, 2);
					
					if ( ++GlobalCurrentCut > GlobalCutsCount ) GlobalCurrentCut=0;
					
					break;
				case 112: //  p   positie dinges iets..

					Toggle_ReplayMode();

					break;
				}
				//			if ( c==81 ) {
				//				OnCloseApplication();
				//			}
			}
			fcntl(0, F_SETFL, tem);
		}
		
		
		
		//		if ( fgets( &tmpChar, 1, stdin ) != NULL ) {
		//			if ( toupper(tmpChar)=="Q" ) {
		//tmpInt = getch();
		//getch();
		//char ch;
		//cin.get(ch);
		//if ( tmpInt != EOF ) {
		//			switch( tmpInt ) {
		//			}
		//		if ( tmpInt==81 ) {
		//			OnCloseApplication();
		//			}
		//		}
		//
		
		
		//--- netwerk-verkeer doorvoer meting..
		if ( minisec() - TimerSec > 1000  ) {  // seconde

			if ( ++IndexNumPlayers >= 300 ) IndexNumPlayers = 0;
			aNumPlayers[IndexNumPlayers]= (NumPlayers-1);	// voor 5 minuten bijhouden

			// uitvoer
			BytesOutSec = TotalBytesOut - TotalBytesOutSec;
			TotalBytesOutSec = TotalBytesOut;
			// invoer
			BytesInSec = TotalBytesIn - TotalBytesInSec;
			TotalBytesInSec = TotalBytesIn;
			// reset timer voor volgende meting per seconde
			TimerSec = minisec();
			
			// de piek-waarden
			if ( BytesInSec > BytesInPeak ) BytesInPeak = BytesInSec;
			if ( BytesOutSec > BytesOutPeak ) BytesOutPeak = BytesOutSec;
			
			// de ring-buffers tbv. afgelopen minuut:
			// invoer
			BytesInLastMin[BytesInLastMinPtr] = BytesInSec;
			BytesInLastMinPtr++;
			if (BytesInLastMinPtr>=60) BytesInLastMinPtr=0;
			BytesInLastMinAvg = BytesInLastMinAvg + BytesInSec - BytesInLastMin[BytesInLastMinPtr];
			// uitvoer
			BytesOutLastMin[BytesOutLastMinPtr] = BytesOutSec;
			BytesOutLastMinPtr++;
			if (BytesOutLastMinPtr>=60) BytesOutLastMinPtr=0;
			BytesOutLastMinAvg = BytesOutLastMinAvg + BytesOutSec - BytesOutLastMin[BytesOutLastMinPtr];
			
			// de ring-buffers tbv. afgelopen uur:
			// Er is telkens een minuut voorbij, als BytesInLastMinPtr==0
			if ( BytesInLastMinPtr==0 ) {
				// invoer
				BytesInLastHour[BytesInLastHourPtr] = BytesInLastMinAvg;
				BytesInLastHourPtr++;
				if (BytesInLastHourPtr>=60) BytesInLastHourPtr=0;
				BytesInLastHourAvg = BytesInLastHourAvg + BytesInLastMinAvg - BytesInLastHour[BytesInLastHourPtr];
				// uitvoer
				BytesOutLastHour[BytesOutLastHourPtr] = BytesOutLastMinAvg;
				BytesOutLastHourPtr++;
				if (BytesOutLastHourPtr>=60) BytesOutLastHourPtr=0;
				BytesOutLastHourAvg = BytesOutLastHourAvg + BytesOutLastMinAvg - BytesOutLastHour[BytesOutLastHourPtr];
			}
		}
		//---
		
		
		
		// nog een keer non-ACK-ed packets versturen..
		CheckRetireHost();
		
		
		//		if ( timerPassed(1000)==1 ) { //om de hele seconde
		//		if ( timerPassed(500)==1 ) { //om de halve seconde
		//		if ( timerPassed(250)==1 ) { //om de kwart seconde
		//		if ( timerPassed(100)==1 ) { //10x per seconde
		//		if ( timerPassed(50)==1 ) { //20x per seconde
		if ( timerPassed(33)==1 ) { //30x per seconde
			
			
			//!!!!		BuffersResend();/// zo heb ik ook wel eens gedaan.. nu weet je iig dat ie om de 1/30e sec. stuurt   okun je iets met de ping erbij doen ok
			
			if ( GlobalState >= stateRacing ) {   // was 7 later 9
				// de 3D-posities zenden.
				Server_SendPos();
				PosSentCount++;
			}
			
			// om de 10x wat extras verzenden..
			//			if ( (int)(PosSentCount % 20) == 0 ) {
			// om de 2 seconden een pinglist verzenden..
			//			if ( PosSentCount % 40 == 0 ) {
			// antwoord deze speler met een ping-lijst van alle spelers..
			//@				Server_PingList( -1, 1 ); //-1=naar iedereen, 1=list met timestamp
			//			}
		}
		
		
		// controleren of de 60 seconden timeout, na eerste die finisht, is verlopen...
		// in dat geval de nog rijdende spelers retiren..
		//
		// is er al iemand over de finish??
		if ( GlobalState == stateRacing ) {   // was 7 later 9
			if ( GlobalFinishTimeVal != 0 ) {
				if ( GlobalPlayersFinished > 0 ) {
					// GlobalFinishTimeVal bevat het tijdstip van 1e finish..
					if ( minisec() > GlobalFinishTimeVal + 61500 ) {
						// timed out..de resterende, rijdende spelers nu retiren..
						// en retire daarna ook de host.
						Server_RetireDrivers();
					}
				}
			}
		}
		
		if ( timerPassed2(5000)==1 ) { //om de 6 seconde(n)
			if ( GlobalState >= stateRacing ) { //was 9
				sprintf( ptr_players[0].Nick, "%s", FastestNick );
				GlobalShowTiming	= 1;
				//SendAllPlayerList( -1 );
			} else {
				Server_PingList( -1, 1 ); //-1=naar iedereen, 1=list met timestamp
				SendAllPlayerList( -1 );
			}
		}	
		
		
		if ( timerPassed3(5000)==1 ) { //om de 6 seconde(n)
			//Server_PingList( -1, 1 ); //-1=naar iedereen, 1=list met timestamp
			if ( GlobalState >= stateRacing ) { //was 9
				MStoTimeString(FastestTime , &ptr_players[0].Nick );   //zie niet veranderen naam van speler tijdens rally...
				GlobalShowTiming	= 0;
				//SendAllPlayerList( -1 );
			} else {
				Server_PingList( -1, 1 ); //-1=naar iedereen, 1=list met timestamp
				SendAllPlayerList( -1 );
			}
		}	
		
		// GameSpy communicatie..
		if ( gsEnabled != 0 ) {
			// is er nog geen contact gelegd??   of..
			// is de GameSpy-timeout al verlopen??
			if ( gsLastTimeVal==0 || minisec()>gsLastTimeVal+gsHEARTBEAT_TIME ) {
				gsLastTimeVal = minisec();
				// deze server in de gamespy-list opnemen..
				gsSendHeartbeat();
			}
		}


		// iets TCP
		//MsgprintyIP(inet_ntoa(peerRA.sin_addr), ntohs(peerRA.sin_port), "<check UDP>" );

		// Wachten op een UDP-packet..
		if ( MyTimeout(sd) ) {		
			//fputs( ".", stdout );
			//DisplayProgress();
			// main_loop begin
			continue;
		}
		
		// Een UDP ontvangen/inlezen
		Offset = sizeof(peer2);
		len = recvfrom(sd, RXPacket, BUFFSZ, 0, (struct sockaddr_in *)&peer2, (socklen_t *) &Offset);
		if (len < 0) {
			std_err();
		} else {
			TrafficIn( len );
		}
		
		if (len == 0) continue;
		
		
		// Gamespy afhandelen ?
		//
		if ( gsEnabled != 0 ) {
			if ( (peer2.sin_addr.s_addr & 0xFFFF) == (ipMaster & 0xFFFF) ) {
				
				RXPacket[len] = 0x00;
				
				
				// Heartbeat afhandelen, secure of niet??
				strncpy( gamekey, gsGameKey, 6 );
				sec = strstr( RXPacket, "\\secure\\" );
				if( sec ) {
					sec += 8;
					key = gsseckey( sec, gamekey, enctype );
					//printf("\n- Secure: %s  Key:    %s\n", sec, key);
					len = snprintf( RXPacket, BUFFSZ, gsHEARTBEAT_2_SECURE, key );
				} else {
					len = snprintf( RXPacket, BUFFSZ, gsHEARTBEAT_2 );
				}
				if ( len<0 || len>BUFFSZ ) return; //buffer is te klein..
				if ( TrafficOut( sendto(sd, RXPacket, len, 0, (struct sockaddr *)&peer2, sizeof(peer2)) ) < 0 ) std_err();
				
				continue;
			}
		}
		
		/*
		// Relay / Doorsturen ??
		if ( RelayEnabled != 0 ) {
		//	if( sendto(RelaySocket, RXPacket, len, 0, (struct sockaddr *)&RelayClient, (socklen_t *)sizeof(RelayClient) ) < 0) std_err();
		sendto(RelaySocket, RXPacket, len, 0, (struct sockaddr *)&RelayClient, (socklen_t *)sizeof(RelayClient) );
		}
		*/
		
		
		// De Peer gegevens overnemen naar wat variabelen 4EZXS
		playerAddrIn = peer2.sin_addr;
		playerIP = ntohl( peer2.sin_addr.s_addr );			// ntohl:  -> u_int32 (long)
		playerPort = ntohs(peer2.sin_port);					// ntohs:  -> u_int16 (short)
		playerNumber = FindPlayer( playerIP, playerPort );	// het spelernummer bepalen
		playerCMD = 0;
		
		
		/*
		// stuur een timestamp van de host
		if ( playerNumber != -1 ) Server_ReplyTimestamp();
		*/
		// Er is net een packet ontvangen,
		// aanduiden als "nog niet verwerkt"
		PacketProcessed = 0;
		// aanduiden als "nog niet afgebeeld"
		PacketPrinted = 0;
		
		// totaal aantal packets ophogen..
		TotalPacketCount++;
		
		// de inhoud van het ontvangen pakketje afbeelden
		// behalve bij oa. "ping" packets.
		if (GlobalShowPackets) {
			//			if ( RXPacket[0] != 0x40 && RXPacket[0] != 0x50 && RXPacket[0] != 0xFE )	  { 
			//			if ( RXPacket[0] != 0x40 && RXPacket[0] != 0xFE )	  { 
			packetCount ++;
			//if ( packetCount % 1 == 0 ) {
			eraseDown( MSGBOTTOM+1 );
			gotoxy( 2, MSGBOTTOM+2 );		
			//}
			if ( packetCount==0xFFFFFFFF ) packetCount = 0;
			fprintf( stdout, "\n Receving packet:  %d ", packetCount);
			fprintf( stdout, "\n Peer-address   :  %s port: %d \n", inet_ntoa( peer2.sin_addr ), ntohs(peer2.sin_port) );
			fputs( " Packet Received: ", stdout );
			for ( i=0; i<len && i<30; i++ ) fprintf( stdout, " %x", RXPacket[i]);
			fputs( "\n", stdout );
			//			} else {
			//fputs( "!", stdout );
			//			}
		}
		
		
		
		// CLIENT ACK met PING in ms
		// Packet == "\x60".."\x00";
		if ( len == 5 ) {
			if ( RXPacket[0] == 0x60 )	  { 
				
				//MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client ping ack>");
				// aanduiden als "afgebeeld"
				PacketPrinted = 1;
				
				// alleen een client ping beantwoorden als de speler is gevonden
				if (playerNumber!=-1) { // speler gevonden??
					// aanduiden als verwerkt
					PacketProcessed = 1;
					tmpULong = (RXPacket[4]<<8) + RXPacket[3];
					sprintf( log, "packet:%d, cl:%d, cmd:%d", tmpULong, getChatLine(playerNumber), getLastCMD(playerNumber) );
					MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client ping ack>", log);
					
					// de client pingt, en geeft een timestamp (4 bytes).
					// Deze timestamp bewaren voor de ping meting..
					//tmpPing = RXPacket[3] + (RXPacket[4]<<8);
					//changePing( playerNumber, tmpPing );
					//SendAllPlayerList( playerNumber );
				}
			}
		}
		
		
		// CLIENT PING
		// Packet == "\x40".."\x01";
		if ( len == 8 ) {
			if ( RXPacket[0] == 0x40 && (RXPacket[3] == 0x01) )	  { 
				
				//MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client ping1>");
				// aanduiden als "afgebeeld"
				PacketPrinted = 1;
				
				// alleen een client ping beantwoorden als de speler is gevonden
				if (playerNumber!=-1) { // speler gevonden??
					// aanduiden als verwerkt
					PacketProcessed = 1;
					
					// de client pingt, en geeft een timestamp (4 bytes).
					// Deze timestamp bewaren voor de ping meting..
					tmpPing = RXPacket[4] + (RXPacket[5]<<8) + (RXPacket[6]<<16) + (RXPacket[7]<<24);
					//sprintf( log, "%x", tmpPing );
					//MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client ping1>", log);
					
					// de ping in ms. berekenen..  tmpPing-Ping1 (dwz. de huidige Ping1 - de vorige)
					// De tijd is in microseconden, dus eerst delen door 1000 voor millisec.
					//tmpULong = (tmpPing - ptr_players[PlayerIndex[playerNumber]].Ping1) / 1000;
					//changePing( playerNumber, tmpULong );
					/*
					// Direct een "\x40" "\x00\x00" "\x02" "\x00\x00\x00\x00" terug-zenden,
					// dan antwoordt de client ook weer met een 40-02 (+timestamp)
					PacketLen = sizeof(ServerPong);				
					memcpy( TXPacket, ServerPong, PacketLen);
					TXPacket[4] = RXPacket[4]; // handle/timestamp erin plakken
					TXPacket[5] = RXPacket[5];
					TXPacket[6] = RXPacket[6];
					TXPacket[7] = RXPacket[7];
					rdcksum( TXPacket,PacketLen );
					Reply1ToPlayer( PacketLen );
					*/
					// de timestamp in dit pakketje bewaren..
					ptr_players[PlayerIndex[playerNumber]].Ping1 = tmpPing;
					//ptr_players[PlayerIndex[playerNumber]].LastTimeValue = tmpPing;
				}
			}
		}
		
		
		// CLIENT PING + timestamp ??
		// Packet == "\x40".."\x02";
		if ( len == 8 ) {
			if ( RXPacket[0] == 0x40 && RXPacket[3] == 0x02 ) {
				
				//MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client ping2>");
				// aanduiden als "afgebeeld"
				PacketPrinted = 1;
				
				if (playerNumber!=-1) { // speler gevonden??
					
					
					// aanduiden als verwerkt
					PacketProcessed = 1;
					
					// Deze timestamp bewaren
					tmpPing = RXPacket[4] + (RXPacket[5]<<8) + (RXPacket[6]<<16) + (RXPacket[7]<<24);
					
					// de ping in ms. berekenen..  tmpPing-Ping2 (dwz. de huidige Ping2 - de vorige)
					// De tijd is in microseconden, dus eerst delen door 1000 voor millisec.
					// tmpULong = (tmpPing - ptr_players[PlayerIndex[playerNumber]].Ping2) / 1000;
					// tmpULong = tmpPing - ptr_players[PlayerIndex[playerNumber]].LastTimeValue;
					
					tmpULong = minisec() - tmpPing;
					
					//sprintf( log, "%x %x %d", tmpPing, ptr_players[PlayerIndex[playerNumber]].LastTimeValue, tmpULong );
					//MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client ping2>", log );
					
					changePing( playerNumber, tmpULong );
					
					//ptr_players[PlayerIndex[playerNumber]].Ping2 = tmpPing;
					
					
					//	Server_ReplyTimestamp();
				}
			}
		}
		
		
		
		// UJE HTTP PLAYERLIST ??
		// Packet == "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F" ??
		// ja: dan een playerlist terugzenden
		if ( playerNumber < 0 ) {
			// CACTI 
			if ( len==3 ) {
				if (memchr("\xCA\xCA\x00", RXPacket[0], 3)!=NULL ) {
					AvgNumPlayers5min = 0;
					for ( i=0; i<300 ; i++ ) {
						AvgNumPlayers5min += aNumPlayers[i];
					}
					AvgNumPlayers5min = AvgNumPlayers5min / 300;
					sprintf( TXPacket, "maxplayers:%d nowplaying:%f ", (MaxPlayers-1), AvgNumPlayers5min );
					PacketLen = strlen( TXPacket );
					Reply1ToPlayer( PacketLen );
				}
			}

			if (len==16) {
				if (memchr("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F", RXPacket[0], 16)!=NULL) {
					
					MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<UJE HTTP PlayerList>");
					// aanduiden als "afgebeeld"
					PacketPrinted = 1;
					
					// aanduiden als verwerkt
					PacketProcessed = 1;
					
					
					// Playerlists maken en terugzenden naar de web-server
					PacketLen = sizeof(ServerPlayListC6);
					memcpy( TXPacket, ServerPlayListC6, PacketLen );
					MakeIngamePlayerList( 0 );
					rdcksum( TXPacket, PacketLen );
					Reply1ToPlayer( PacketLen );
				}
			}
		}
		
		
		// CLIENT REFRESH ??
		// Packet == "\xFE\xFD\x02\x00\x00\x00\x00\x00" ??
		// ja: dan 3x terugzenden: "\x05\x00\x00\x00\x00"
		if (len==8) {
			if (memchr("\xFE\xFD\x02\x00\x00\x00\x00\x00", RXPacket[0], 8)!=NULL) {
				
				MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client refresh>");
				// aanduiden als "afgebeeld"
				PacketPrinted = 1;
				
				// aanduiden als verwerkt
				PacketProcessed = 1;
				
				memcpy( TXPacket, clRefreshAnswer, sizeof(clRefreshAnswer));
				// 3x zenden..
				//ReplyToPlayer( 5 ); //lengte 5
				Reply1ToPlayer( 5 );
				
			}
		}

		
		// CLIENT MINI SERVER LIST ??
		// Packet == "\xFE\xFD\x00" "\x00\x00\x00\x00" ... ??
		if ( len>=17 ) {
			if (memchr("\xFE\xFD\x00", RXPacket[0], 3)!=NULL ) {
				
				//MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client mini serverInfo>");
				// aanduiden als "afgebeeld"
				PacketPrinted = 1;
				
				// aanduiden als verwerkt
				PacketProcessed = 1;
				
				clHandle = *(u_long *)(RXPacket+3);
				
				// Num- & MaxPlayers omzetten naar strings..
				// sprintf==0, niets geprint
				// sprintf>0, zoveel characters geprint
				// snprintf, maximaal het opgegeven aantal characters printen..
				snprintf( &strNumPlayers,5, "%d", NumPlayers-1 );
				snprintf( &strMaxPlayers,5, "%d", MaxPlayers-1 );
				
				PacketLen = Answer_MiniServerInfo(TXPacket, clHandle, 
					cmr4hostname, "1.0", 
					
					"1", //???  StrRankingValue[GlobalRanking]
					//"1",
					"1", //rally=0, stages=1 ??
					
					
					StrGameMode[GlobalGameMode],
					strNumPlayers, strMaxPlayers);
				Reply1ToPlayer( PacketLen );
			}
		}
		
		
		// CLIENT SERVER LIST ??
		// Packet == "\xFE\xFD\x00" "\x00\x00\x00\x00" "\xFF\xFF\xFF" ??
		if ( len==10 ) {
			if (memchr("\xFE\xFD\x00", RXPacket[0], 3)!=NULL &&
				memchr("\xFF\xFF\xFF", RXPacket[7], 3)!=NULL) {
				
				//MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client serverInfo>");
				// aanduiden als "afgebeeld"
				PacketPrinted = 1;
				
				// aanduiden als verwerkt
				PacketProcessed = 1;
				
				// client handle eruit halen..
				clHandle = *(u_long *)(RXPacket+3); //clHandle = RXPacket[6]*256*256*256 + RXPacket[5]*256*256 + RXPacket[4]*256 + RXPacket[3];
				
				// Num- & MaxPlayers omzetten naar strings..
				snprintf( &strNumPlayers,5, "%d", NumPlayers-1 );
				snprintf( &strMaxPlayers,5, "%d", MaxPlayers-1 );
				snprintf( &strPort,5, "%d", PORT );
				/*
				// is dit een packet van een speler?? of van iemand buiten het spel?..
				if ( playerNumber == -1 ) {
				// anonymous access..
				
				  // een sever info packet samenstellen..
				  memcpy( TXPacket, ServerInfo, sizeof(ServerInfo));
				  PacketLen = Answer_ServerInfo(TXPacket, clHandle, 
				  cmr4hostname, "1.0", strPort, 
				  StrPasswordValue[GlobalPassword],
				  StrGameTypeValue[GlobalGameType], StrGameMode[GlobalGameMode],
				  strNumPlayers , strMaxPlayers,
				  "0", "0", //"134134134134134134"
				  StrDamageValue[GlobalDamage], StrRankingValue[GlobalRanking], 
				  StrCarTypeValue[GlobalCarType]);
				  // send the packet..
				  Reply1ToPlayer( PacketLen );
				  
					}
				*/
				// een sever info packet samenstellen..
				memcpy( TXPacket, ServerInfo, sizeof(ServerInfo));
				PacketLen = Answer_ServerInfo(TXPacket, clHandle, 
					cmr4hostname, "1.0", strPort, 
					StrPasswordValue[GlobalPassword],
					StrGameTypeValue[GlobalGameType], StrGameMode[GlobalGameMode],
					strNumPlayers , strMaxPlayers,
					StrRalliesValue[GlobalRally], StrStagesValues[GlobalStages_Land][GlobalStages_Stage], //"134134134134134134"
					StrDamageValue[GlobalDamage], StrRankingValue[GlobalRanking], 
					StrCarTypeValue[GlobalCarType]);
				// send the packet..
				Reply1ToPlayer( PacketLen );
				
			}
		}
		
		
		
		
		
		
		
		
		
		
		// JOIN REQUEST ??
		// Packet == "\xC8\x0D\x89\x00\x00\x07\x00\x00\x00\x00\xa0\x0f\x00\x00\xa0\x0f\x00\x00\x01";
		if (playerNumber<0) {
			if ( len>=19  &&  len<=19+MAXPWD ) {
				if ( RXPacket[0]==0xC8 && RXPacket[5]==0x07 ) {   
					
					//MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client joinrequest>");
					// aanduiden als "afgebeeld"
					PacketPrinted = 1;
					
					
					// aanduiden als verwerkt
					PacketProcessed = 1;
					
					// geen wachtwoord nodig?? of wel nodig en een correct wachtwoord opgegeven??..
					if ( (GlobalPassword == 1 && RXPacket[17]>0 && (memchr(cmr4password, RXPacket+19, RXPacket[17])!=NULL)) ||
						(GlobalPassword == 0) ) {
						// wachtwoord is goed, of is niet nodig
						// \xD8......07
						PacketLen = sizeof(ServerJoinReply1);
						memcpy( TXPacket, ServerJoinReply1, PacketLen);
						ReplyToPlayer( PacketLen );
						
						// \x50\xE1\xE5\x00\x00
						PacketLen = sizeof(ServerJoinReply2);
						memcpy( TXPacket, ServerJoinReply2, PacketLen);
						ReplyToPlayer( PacketLen );
					} else {
						// wachtwoord is nodig, maar is fout opgegeven
						//fputs( "Wrong Password!", stdout );
						PacketLen = sizeof(ServerWrongPassword);
						memcpy( TXPacket, ServerWrongPassword, PacketLen);
						ReplyToPlayer( PacketLen );
					}
				}
				/*
				if ( RXPacket[17]==0  ||  
				(RXPacket[17]>0  &&  (memchr(cmr4password, RXPacket+19, RXPacket[17])!=NULL)) ) {
				
				  // goed wachtwoord opgegeven..(of geen wachtwoord nodig)
				  // \xD8......07
				  PacketLen = sizeof(ServerJoinReply1);
				  memcpy( TXPacket, ServerJoinReply1, PacketLen);
				  ReplyToPlayer( PacketLen );
				  
					// \x50\xE1\xE5\x00\x00
					PacketLen = sizeof(ServerJoinReply2);
					memcpy( TXPacket, ServerJoinReply2, PacketLen);
					ReplyToPlayer( PacketLen );
					} else {
					// fout wachtwoord opgegeven..
					// 10x terugzenden packet: ServerWrongPassword
					fputs( "Wrong Password!", stdout );
					
					  PacketLen = sizeof(ServerWrongPassword);
					  memcpy( TXPacket, ServerWrongPassword, PacketLen);
					  ReplyToPlayer( PacketLen );
					  }
				*/
			}
		}
		
		
		// ECHTE JOIN REQUEST ??
		// Packet == "\x50".."\x00\x00" "\x01"....;
		if ( len==10 ) {
			if ( RXPacket[0]==0x50 && RXPacket[3]==0x00 && RXPacket[4]==0x00 && RXPacket[5]==0x01) {   
				
				MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client real joinrequest>");
				// aanduiden als "afgebeeld"
				PacketPrinted = 1;
				
				// alleen mogelijk indien "openwaiting"..
				if ( GlobalGameMode == 0 ) {
					
					if (playerNumber<0) {
						// niet gevonden, dus nieuwe speler maken..
						playerNumber = AddPlayer( playerAddrIn, playerPort);

						// gelukt??
						if (playerNumber!=-1) {
							
							changeReady(playerNumber, 0 ); // op niet ready ivm vorige waardes...
							
							// aanduiden als verwerkt
							PacketProcessed = 1;
							
							// nummer uit client-pakketje overnemen..
							clHandle = (u_long *)(RXPacket+6); //clHandle = RXPacket[9]*256*256*256 + RXPacket[8]*256*256 + RXPacket[7]*256 + RXPacket[6];
							//fprintf( stdout, " clHandle: 0x%x \n ", clHandle );
							
							//joiner beantwoorden..
							// \xC9\x34\xDC\x01\x00
							PacketLen = sizeof(ServerJoinReply3);
							memcpy( TXPacket, ServerJoinReply3, PacketLen);
							ReplyToPlayer( PacketLen );
							
							
							// \xC6..\x02\x00\x80...
							PacketLen = sizeof(ServerJoinReply5);
							memcpy( TXPacket, ServerJoinReply5, PacketLen);
							// de GameSpy-ID erin plakken ??
							//						tmpUShort = ;
							//						TXPacket[6]  = tmpUShort & 0xFF;
							//						TXPacket[7] = (tmpUShort>>8) & 0xFF;
							// het speler JoinNr erin plakken..
							tmpULong = getID(playerNumber);
							TXPacket[9]  = tmpULong & 0xFF;
							TXPacket[10] = (tmpULong>>8) & 0xFF;
							TXPacket[11] = (tmpULong>>16) & 0xFF;
							TXPacket[12] = (tmpULong>>24) & 0xFF;
							// checksum
							rdcksum( TXPacket,PacketLen );
							ReplyToPlayer( PacketLen );
							
							//-1=naar iedereen, 1=list met timestamp
						}
					} else {
						// de speler bestaat al/nog??!!....
					}
					
				}
				
			}
		}
		
		
		// NICK DOORGEVEN
		// Packet == "\xC6";
		if ( ( RXPacket[0] == 0xC6 && RXPacket[5] == 0x81 ) || 
			 ( RXPacket[0] == 0xD6 && RXPacket[7] == 0x81 ) )    { 
			
//			MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client name>");
			// aanduiden als "afgebeeld"
//			PacketPrinted = 1;
			
			if (playerNumber>0) { // speler gevonden??
				Offset = ( RXPacket[0] == 0xD6 )? 2 : 0;

				// deze chat nog niet gehad??
				playerCMD = RXPacket[4+Offset]*256+RXPacket[3+Offset];
				if ( getLastCMD(playerNumber)+1 == playerCMD ) {
					//				if ( playerCMD == 1 ) {
					changeLastCMD( playerNumber, playerCMD ); //laatste waarde onthouden..
					// aanduiden als verwerkt
					PacketProcessed = 1;
					
					sprintf( log, "\"%s\"", (u_char *) RXPacket + 13 + Offset );
//strcpy( log, RXPacket + 13 + Offset );
					MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client name> ", log);  // stond MsgprintyIP2
					PacketPrinted = 1;
					

					// onthouden of deze speler met een C6 of D6 heeft aangemeld..
					ptr_players[PlayerIndex[playerNumber]].NickC6D6 = RXPacket[0];
					
					
					changeChatLine( playerNumber, 2 ); //2 tmpUShort+1
					increaseChatLine(0);

					
					// Namen filteren waarin een wazig teken is opgenomen..zo'n teken als in "Pawe¦".. (die ik typ op mijn keyboard is een "|"..dus)
					// ¦ 
					i = 0;
					while ( i<16 && (u_char)RXPacket[13+Offset+i]!=0 ) {
						if ( (u_char)RXPacket[13+Offset+i]>=127 ) (u_char)RXPacket[13+Offset+i] = 32; //" ";
						i++;
					}
/*
					// Namen weigeren die bestaan uit slechts spaties..
					i = 0;
					while ( i<16 && (u_char)RXPacket[13+Offset+i]!=0 ) {
						if ( (u_char)RXPacket[13+Offset+i]!=32 ) break;
						i++;
					}
					if ( strlen((u_char *) RXPacket+13+Offset) == i ) {
						// weigeren..
					}
*/

					// op (RXPacket + 9 + Offset) staat het JoinNr
					
					// geen lege naam opgeven ja..
					//	if ( (u_char *) RXPacket + 13 + Offset != "" )
					changeReady(playerNumber, 0 ); // op niet ready ivm vorige waardes...
					changeNick(playerNumber, (u_char *) RXPacket + 13 + Offset);
					changeCountry(playerNumber, RXPacket[29 + Offset]);
					// RXPacket[30] lijkt constant een waarde 0x02 te hebben..
//ptr_players[PlayerIndex[playerNumber]].RXPacket30 = RXPacket[30];
changeConnectionSpeed( playerNumber, RXPacket[30] );
					changeCar(playerNumber, RXPacket[31 + Offset]);
					changeCarType(playerNumber, RXPacket[32 + Offset]);
					changeGearbox(playerNumber, RXPacket[33 + Offset]);
					//fprintf( stdout, " Nick=%s\n", getNick(playerNumber) );
					
					
					// een ACK sturen naar deze speler..
					// \x50\x5E\x66\x01\x00
					PacketLen = sizeof(ServerJoinReply4);
					memcpy( TXPacket, ServerJoinReply4, PacketLen);
					ReplyToPlayer( PacketLen );
					
/* changed: 2008mei11					
					// Playerlists maken en verzenden
					SendAllPlayerList( playerNumber );
*/					
					Server_ReplyTimestamp();
					
					
					// Een C6-83 zenden naar deze speler..
					// Game settings doorgeven.
					PacketLen = sizeof(ServerGameChange);				
					memcpy( TXPacket, ServerGameChange, PacketLen);
					// rally of stages  (0 of 1)
					TXPacket[13] = 1;
					// damage
					TXPacket[14] = GlobalDamage;
					// ranking: points of time
					TXPacket[15] = GlobalRanking;
					// CarType
					TXPacket[16] = GlobalCarType;
					// land
					TXPacket[17] = GlobalStages_Land;
					// stage
					TXPacket[18] = GlobalStages_Stage;
					// ChatLine
					increaseChatLine( playerNumber );
					playerCMD = getChatLine(playerNumber);
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					//
					rdcksum( TXPacket, PacketLen );
					BufferPacket( playerNumber, TXPacket, PacketLen, playerCMD );

					
					
					// Playerlists maken en verzenden
					SendAllPlayerList( playerNumber );


					//test
					// Relay / Doorsturen ??
					if ( RelayEnabled != 0 ) {
						sendto(RelaySocket, TXPacket, PacketLen, 0, (struct sockaddr *)&RelayClient, (socklen_t *)sizeof(RelayClient) );
						//sprintf(log, "%d", sendto(RelaySocket, TXPacket, PacketLen, 0, (struct sockaddr *)&RelayClient, (socklen_t *)sizeof(RelayClient) ));
						//Log( log );
					}
					///test
					//BuffersResend();
/*					
					//sprintf( Nick, "%s", getNick(playerNumber) ); //changed: 2008mei1
					//						sprintf( tmpStr, "%s", trim(Nick) );//changed: 2008mei1
					sprintf(log, "<client name> \"%s\"",  getNick(playerNumber) ); //changed: 2008mei1
					//Log( log );
					MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), log );
*/					
/*
					// Namen weigeren waarin een piping-teken is opgenomen..zo'n teken als in "Pawe¦".. (die ik typ op mijn keyboard is een "|"..dus)
					// ¦ 
					Nick = getNick(playerNumber);
					for (i=0;i<strlen(Nick);i++) {
						if (Nick[i]>127) Nick[i] = " ";
					}
					changeNick(playerNumber, Nick);
					sprintf(log, "<client name> \"%s\"", Nick  ); //
*/
					//						if ( (strchr( Nick, 0xA6 )!=NULL) ||
					//							 (strlen(tmpStr)==0 ) ) {
					if (strcasecmp( log, "<client name> \"\"" )==0 || 
						strcasecmp( log, "<client name> \" \"" )==0 || 
						strcasecmp( log, "<client name> \"  \"" )==0 ||
						strcasecmp( log, "<client name> \"   \"" )==0 ||
						strcasecmp( log, "<client name> \"    \"" )==0 ||
						strcasecmp( log, "<client name> \"     \"" )==0 ||
						strcasecmp( log, "<client name> \"      \"" )==0 ||
						strcasecmp( log, "<client name> \"       \"" )==0 ||
						strcasecmp( log, "<client name> \"        \"" )==0 ||
						strcasecmp( log, "<client name> \"         \"" )==0 ||
						strcasecmp( log, "<client name> \"          \"" )==0 ||
						strcasecmp( log, "<client name> \"           \"" )==0 ||
						strcasecmp( log, "<client name> \"            \"" )==0 ||
						strcasecmp( log, "<client name> \"             \"" )==0 ||
						strcasecmp( log, "<client name> \"              \"" )==0 ||
						strcasecmp( log, "<client name> \"               \"" )==0 
						//|| strchr( log, 0xB1 )!=NULL
						) {
						
						
						//sleep(0);
//						MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), log );
						
						//--- een C6-8B naar de rest..
						PacketLen = sizeof(ChatFromHostToAny);
						memcpy( TXPacket, ChatFromHostToAny, PacketLen);
						
						// nickname opzoeken en plakken
						strcpy( (u_char *) ChatFromHostToAny_Nick, "KICKED" );//string
						//strncpy( (u_char *) ChatFromHostToAny_Nick,3, (u_char *)"UJE" );
						
						// de chat tekst uit het ontvangen pakketje overnemen
						//L = strlen((u_char *) ChatFromHostToAny_Nick);
						//LMsg = strlen((u_char *) aMessage);
						strcpy( ChatFromHostToAny_Nick + 6 +1, "YOU HAVE NO NAME !" ); //adres
						// de pakket-lengte berekenen..(na chatText+0 afkappen)
						PacketLen = 37 + 6 + 1 + 18 +1;
						
						
						if ( playerNumber > 0 ) {
							increaseChatLine( playerNumber );
							
							tmpUShort = getChatLine(playerNumber);
							TXPacket[3] = tmpUShort & 0xFF;
							TXPacket[4] = (tmpUShort >> 8) & 0xFF;
							
							tmpULong =  getID(0);		// wie zei het ?
							TXPacket[9] = tmpULong & 0xFF;
							TXPacket[10] = (tmpULong >> 8) & 0xFF;
							TXPacket[11] = (tmpULong >> 16) & 0xFF;
							TXPacket[12] = (tmpULong >> 24) & 0xFF;
							
							rdcksum( TXPacket, PacketLen );
							SendToPlayer( playerNumber, PacketLen );
							//BufferPacket( playerNumber, TXPacket, PacketLen, tmpUShort );
						}
						
						increaseChatLine( 0 ); //host
						//BuffersResend();
						
						
						//-------------
						// de andere spelers een C6-08 sturen..
						PacketLen = sizeof(ServerQuit);
						memcpy( TXPacket, ServerQuit, PacketLen);
						
						// playerNumber van deze speler
						if ( GlobalState < stateSwitch  ) { //changed: 2008apr30
							//							if ( GlobalState <= stateNumbering  ) {								
							TXPacket[6] = playerNumber;
						} else {
							TXPacket[6] = getStartPlayerNr(playerNumber);
						}
						
						for ( i=1; i < NumPlayers; i++ ) {
							if ( playerNumber != i ) {
								// Chatline
								increaseChatLine( i );
								tmpUShort = getChatLine(i);
								TXPacket[3] = tmpUShort & 0xFF;
								TXPacket[4] = (tmpUShort >> 8) & 0xFF;
								// checksum
								rdcksum( TXPacket,PacketLen );
								// volgens mij mag ie packets naar anderen best bufferen..
								BufferPacket( i, TXPacket, PacketLen, tmpUShort );
								//SendToPlayer( i, PacketLen );//changed: 2008mei1  terug veranderd
							}
						}
						DisplayBuffers();
						//BuffersResend();
						//-------------

						changeState( playerNumber, 0 );
						changeQuit( playerNumber, 1);
						GlobalPlayersQuit++;
						
						RemovePlayer( playerNumber, (GlobalState<stateSwitch)?1:0 );
						//KickPlayer( playerNumber, "you have no name !" );
						
					} else {
						
						// de host zelf..
						increaseChatLine(0);
						
						// een UJE welcome tekst sturen naar deze nieuwe speler..
						//ServerChat( playerNumber, "Server", "BETA server.. sorry for disconnect.. \"vote FIN5\" to change stage..." );
						ServerChat( playerNumber, CMR4HOSTNICK, StrReplaceGogogoD6 );
						//					ServerChat( playerNumber, "Server", StrWelcome );
						//						}
						// de game-status controleren, na een speler-join..
						CheckGlobalState();

						IP = getIP(playerNumber);
						Nick = getNick(playerNumber);
						Country = StrCountry[getCountry(playerNumber)][0];
						// zoek het MemberID op in de DB.
						changeMemberID( playerNumber, sqlGetMember( IP, Nick, Country ) );

					}
				}
				
			}
		}
		
		
		// CAR DOORGEVEN
		// Packet == "\xC6"?? clCarChange
		if ( RXPacket[0] == 0xC6 &&  RXPacket[5] == 0x82  ) { 
			
			//MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client state>");
			// aanduiden als "afgebeeld"
			PacketPrinted = 1;
			
			if (playerNumber!=-1) { // speler gevonden??
				playerCMD = RXPacket[4]*256+RXPacket[3];
				
				// ack terugsturen aan client..
				PacketLen = sizeof(Acknowledge);
				memcpy( TXPacket, Acknowledge, PacketLen);
				TXPacket[3] = playerCMD & 0xFF;
				TXPacket[4] = (playerCMD >> 8) & 0xFF;
				rdcksum( TXPacket,PacketLen );
				ReplyToPlayer( PacketLen );
				
				if ( getLastCMD(playerNumber)+1 == playerCMD ) {
					
					MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client state>");
					// aanduiden als verwerkt
					PacketProcessed = 1;
					
					changeLastCMD( playerNumber, playerCMD ); //laatste waarde onthouden..
					
					//  READY in RXPacket[13]     1 = ready,    0 of 2 = not ready
					if ( (u_long*)*(RXPacket+13) != 1 ) {
						ptr_players[PlayerIndex[playerNumber]].LastTimeReady = minisec();
						ptr_players[PlayerIndex[playerNumber]].ReadyWarning = 0;
					}
					if ( getCar( playerNumber ) != RXPacket[17] ) changeCar( playerNumber, RXPacket[17] );
					if ( getCarType( playerNumber ) != RXPacket[18] ) changeCarType( playerNumber, RXPacket[18] );
					if ( getGearbox( playerNumber ) != RXPacket[19] ) changeGearbox( playerNumber, RXPacket[19] );
					
					//if ( getReady( playerNumber ) != 1) {  // je moet eigenlijk weten of ie ghost heeft gechat of nie...
					// nu doe je weer niet ready en dan plopt ie terug
					//}
					changeReady(playerNumber, (u_long*)*(RXPacket+13) );
					
					// Playerlists verzenden
					SendAllPlayerList( playerNumber );
					
					// de game-status controleren, na een speler-ready verandering..
					CheckGlobalState();

					//LoadHighScores( GlobalStages );
					i = 11 + PlayerIndex[playerNumber];
					LoadPersonalRecord_CarType( playerNumber, GlobalStages, getCarTypeStr(playerNumber), i );
					sprintf( log, "PlayerNr:%d, Loading TimeID: %d", playerNumber, ptr_players[i].TimeID );
					MsgprintyIP1( "",0, "<PersonalRecord HS>", log );
/*
					// reset de index naar de laatst-verzonden-PosRec..
					// de speler PlayerNr heeft nog geen PosRecs ontvangen.
					// ..dan begint de host weer vooraan de race, ipv ergens achteraan in stage (al meteen bij start race)..
					for (j=0; j<MAXPLAYERS; j++) ptr_players[PlayerIndex[PlayerNr]].LastPosRecSent[j] = 0;
*/
				} else if (playerCMD  <= getLastCMD(playerNumber) ) {
					
					// toch een ack terugsturen aan client..
					PacketLen = sizeof(Acknowledge);
					memcpy( TXPacket, Acknowledge, PacketLen);
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
					
				}
				
			}
		}
		
		
		// CHAT ONTVANGEN
		// Packet == "\xC6"....."\x8A";
		if ( RXPacket[0] == 0xC6 && RXPacket[5] == 0x8A ) { 
			
			//MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client chat>", clChat_Text);
			// aanduiden als "afgebeeld"
			PacketPrinted = 1;
			
			if (playerNumber!=-1) { // speler gevonden??
				
				// deze chat nog niet gehad??
				playerCMD = RXPacket[4]*256+RXPacket[3];
				if ( getLastCMD(playerNumber)+1 == playerCMD ) {
					
					// aanduiden als verwerkt
					PacketProcessed = 1;
					
					changeLastCMD( playerNumber, playerCMD ); //laatste waarde onthouden..
					
					
					sprintf( log, "<chat> %s: %s", getNick(playerNumber), clChat_Text );
					
					MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), log);
					//Log( log );
					
					
					// normale- & vote-chats altijd afbeelden..
					// het regelnummer van de host ophogen..
					increaseChatLine(0);
					
					// 1 melding naar de zender terug (D6......8B), de rest een melding (C6....8B)
					//--- Eerst de D6...
					PacketLen = sizeof(ChatRelayToSpeaker);
					memcpy( TXPacket, ChatRelayToSpeaker, PacketLen);
					TXPacket[3] = RXPacket[3];	// hetzelfde nummer plakken (een word 16b),
					TXPacket[4] = RXPacket[4];	// als de client zelf zendt.
					
					increaseChatLine(playerNumber);
					tmpUShort =  getChatLine(playerNumber);
					TXPacket[5] = tmpUShort & 0xFF;
					TXPacket[6] = (tmpUShort >> 8) & 0xFF;
					
					tmpULong =  getID(playerNumber);		// wie zei het ?
					TXPacket[11] = tmpULong & 0xFF;
					TXPacket[12] = (tmpULong >> 8) & 0xFF;
					TXPacket[13] = (tmpULong >> 16) & 0xFF;
					TXPacket[14] = (tmpULong >> 24) & 0xFF;
					
					// nickname opzoeken en plakken
					strcpy( (u_char *)ChatRelayToSpeaker_Nick, getNick(playerNumber) );
					// de chat tekst uit het ontvangen pakketje overnemen
					strcpy( ChatRelayToSpeaker_Nick + strlen((u_char *)ChatRelayToSpeaker_Nick) + 1, (u_char *)clChat_Text );
					// de pakket-lengte berekenen..(na chatText+0 afkappen) (D6 is 2 bytes langer)
					PacketLen = 2+37 + strlen((u_char *)ChatRelayToSpeaker_Nick)+1 + strlen((u_char *)clChat_Text)+1;
					rdcksum( TXPacket,PacketLen );
					//ReplyToPlayer( PacketLen );
					BufferPacket( playerNumber, TXPacket, PacketLen, tmpUShort );
					
					
					//--- nu een C6 naar de rest..
					// maar alleen als het geen verboden tekst is..
					// en indien het een "vote-kick" is, alleen zenden als "vote-kick"s niet anoniem zijn..
					if ( (isClientVoteKick(playerNumber, (u_char *)clChat_Text)==0 && ChatForbidden( clChat_Text )==0) ||
						(isClientVoteKick(playerNumber, (u_char *)clChat_Text)!=0 && GlobalShowVoteKicks!=0) ) {
						PacketLen = sizeof(ChatRelay);
						memcpy( TXPacket, ChatRelay, PacketLen);
						// nickname opzoeken en plakken
						strcpy( (u_char *) ChatRelay_Nick, getNick(playerNumber) );
						// de chat tekst uit het ontvangen pakketje overnemen
						strcpy( ChatRelay_Nick + strlen((u_char *) ChatRelay_Nick) + 1, (u_char *) clChat_Text );
						// de pakket-lengte berekenen..(na chatText+0 afkappen)
						PacketLen = 37 + strlen((u_char *)ChatRelay_Nick)+1 + strlen((u_char *)clChat_Text)+1;
						for ( i=1; i < NumPlayers; i++ ) {
							if ( playerNumber != i ) {
								
								increaseChatLine( i );
								
								tmpUShort = getChatLine(i);
								TXPacket[3] = tmpUShort & 0xFF;
								TXPacket[4] = (tmpUShort >> 8) & 0xFF;
								
								tmpULong =  getID(playerNumber);		// wie zei het ?
								TXPacket[9] = tmpULong & 0xFF;
								TXPacket[10] = (tmpULong >> 8) & 0xFF;
								TXPacket[11] = (tmpULong >> 16) & 0xFF;
								TXPacket[12] = (tmpULong >> 24) & 0xFF;
								
								rdcksum( TXPacket, PacketLen );
								//SendToPlayer( i, PacketLen );
								BufferPacket( i, TXPacket, PacketLen, tmpUShort );
							}
						}
					}
					//BuffersResend();
					
					//--- UJE CHAT CHECK ------
					ClientNoNonsense( playerNumber, clChat_Text );
					//-------------------------
					
					//--- UJE VOTING SYSTEM ---
					// test of deze chat een "vote"-chat is voor een stage of een normale chat..
					//					if ( isClientVoteChat( playerNumber, (u_char *)clChat_Text ) != 0 ||
					//						 isClientVoteStatus( playerNumber, (u_char *)clChat_Text ) != 0 ||
					//						 isClientVoteKick( playerNumber, (u_char *)clChat_Text ) != 0 ) {
					// deze chat is een vote..
					if ( ClientVoting( playerNumber, (u_char *)clChat_Text ) != 0 ) {
						// deze chat is een stage-vote..of een ghost-vote
					}
					//					}
					//-------------------------
					
				}
				
			}
		}
		
		
		
		
		
		
		
		
		
		// CLIENT ACK ??
		if (len==5 || len==10) {
			if ( RXPacket[0] == 0x50 ) {
				
				// ack-nummer
				playerCMD = RXPacket[4]*256+RXPacket[3];
				
				//MsgprintyIP2(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client ack>", playerCMD);
				// aanduiden als "afgebeeld"
				PacketPrinted = 1;
				
				// het client nummer in het ontvangen lange 50-packet overnemen..
				clHandle = (u_long *)(RXPacket+6); //clHandle = RXPacket[9]*256*256*256 + RXPacket[8]*256*256 + RXPacket[7]*256 + RXPacket[6];
				
				//				if ( playerCMD == getChatLine(playerNumber) ) {
				
				if ( playerCMD != 0x0000 ) {	// geen "leeg" commando verwerken..
					// aanduiden als verwerkt
					PacketProcessed = 1;
					
					// ACK bevestigd/afgehandeld..
					tmpUChar = AckBufferedPacket( playerNumber, playerCMD );
					// PlayerNr = tmpUChar >> 4
					// BufferNr = tmpUChar & 0x0F
					// 0xFF     = niet gevonden
//					if ( tmpUChar != 0xFF ) {
//						// gevonden!..en verwijderd.
//						//MsgprintyIP2(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<Buffer Deleted>", tmpUChar );
//						//MsgprintyIP4( inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<Buffer Deleted>", (tmpUChar>>4), (tmpUChar & 0x0F), playerCMD );
//					} else {
//						//packet niet gevonden in een buffer!..
//						//MsgprintyIP2(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<Buffer unknown>", getChatLine(playerNumber) );
//						//MsgprintyIP3( inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<Buffer empty>", playerNumber, playerCMD );
//					}
				}
				
				if ( len == 10 ) {
					// als het een lange 50 was (10 bytes),
					// dan altijd een 40 packet antwoorden van 8 bytes (zonder checksum),
					// met daarin de 4 laatste packet-bytes overgenomen uit de 50,
					// naar de 4 laatste bytes van de terug te sturen 40. (genaamd clHandle)
					PacketLen = sizeof(ServerLong50ACK);
					memcpy( TXPacket, ServerLong50ACK, PacketLen);
					// handle erin plakken
					*(u_long *)(TXPacket+4) = clHandle;
					ReplyToPlayer( PacketLen );
				}
				
				// de game-status controleren, na een speler-ACK..
				CheckGlobalState();
				
				//				}
			}
		}
		
		
		
		
		
		// CLIENT clGridPresent1 (C6....0E)
		if ( len==37 ) {
			if ( RXPacket[0] == 0xC6 && RXPacket[5] == 0x0E ) {
				
				// aanduiden als "afgebeeld"
				PacketPrinted = 1;
				// ack-nummer
				playerCMD = RXPacket[4]*256+RXPacket[3];
				
				// spelerstartnummer
				tmpPlayerNr = RXPacket[6];
				
				if ( getLastCMD(playerNumber)+1 == playerCMD ) {
					changeLastCMD( playerNumber, playerCMD ); //laatste waarde onthouden..
					//}
					
					MsgprintyIP2(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client gridPresent1: playerIndex>", tmpPlayerNr);
					
					// aanduiden als verwerkt
					PacketProcessed = 1;
					
					// mark
					ptr_players[PlayerIndex[playerNumber]].ReceivedC60E = 1;
					
					/*
					// tellen hoeveel spelers al een C6-0E hebben gestuurd..
					tmpInt=0; for (i=1;i<NumPlayers;i++) if (ptr_players[PlayerIndex[playerNumber]].ReceivedC60E!=0) tmpInt++;
					*/
					
					// ack terugsturen aan client..
					PacketLen = sizeof(Acknowledge);
					memcpy( TXPacket, Acknowledge, PacketLen);
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
					ReplyToPlayer( PacketLen );
					
					
					// de anderen een C6-0E terugsturen..
					for ( i=1; i < NumPlayers; i++ ) {
						if ( getStartPlayerNr(i) != tmpPlayerNr ) {
							//!if (tmpInt==1||tmpInt==2) {
							PacketLen = sizeof(ServerGridPresent1);
							memcpy( TXPacket, ServerGridPresent1, PacketLen);
							//!
							/*
							} else
							if (tmpInt==3||tmpInt==4) {
							PacketLen = sizeof(ServerGridPresent1a);
							memcpy( TXPacket, ServerGridPresent1a, PacketLen);
							} else 
							if (tmpInt==5||tmpInt==6) {
							PacketLen = sizeof(ServerGridPresent1b);
							memcpy( TXPacket, ServerGridPresent1b, PacketLen);
							} else 
							if (tmpInt==7||tmpInt==8) {
							PacketLen = sizeof(ServerGridPresent1c);
							memcpy( TXPacket, ServerGridPresent1c, PacketLen);
							}
							*/
							
							//Server_NumberThing(i);   //// deze nog ertussen ? //////////////////////////////////
							
							// ChatLine
							increaseChatLine( i );
							tmpUShort = getChatLine(i);
							TXPacket[3] = tmpUShort & 0xFF;
							TXPacket[4] = (tmpUShort >> 8) & 0xFF;
							// client-info uit ontvangen packet overnemen..
							// PlayerNr
							TXPacket[6] = tmpPlayerNr;
							// ??
							memcpy( TXPacket+21, RXPacket+21, 16);
							// checksum
							rdcksum( TXPacket,PacketLen );
							BufferPacket( i, TXPacket, PacketLen, tmpUShort );
							
							//SendAllPlayerList( -1 );   /// eigenlijk i ////////
						}
					}
					//BuffersResend();
					
					// antwoorden "de host is ook present"
					PacketLen = sizeof(ServerGridPresent1);
					memcpy( TXPacket, ServerGridPresent1, PacketLen);
					// ChatLine
					increaseChatLine( playerNumber );
					tmpUShort = getChatLine(playerNumber);
					TXPacket[3] = tmpUShort & 0xFF;
					TXPacket[4] = (tmpUShort >> 8) & 0xFF;
					// playernumber
					TXPacket[6] = 0;
					memcpy( TXPacket+21, RXPacket+21, 16);
					// checksum
					rdcksum( TXPacket,PacketLen );
					BufferPacket( playerNumber, TXPacket, PacketLen, tmpUShort );
					
					if ( GlobalConfigMode == 1 ) {
						// Replay2player 
						
						PacketLen = sizeof(ServerGridPresent1);
						memcpy( TXPacket, ServerGridPresent1, PacketLen);
						
						// ChatLine
						increaseChatLine( playerNumber );
						tmpUShort = getChatLine(playerNumber);
						TXPacket[3] = tmpUShort & 0xFF;
						TXPacket[4] = (tmpUShort >> 8) & 0xFF;
						// playernumber
						TXPacket[6] = 1;
						memcpy( TXPacket+21, RXPacket+21, 16);
						// checksum
						rdcksum( TXPacket,PacketLen );
						BufferPacket( playerNumber, TXPacket, PacketLen, tmpUShort );
						
						// ChatLine
						increaseChatLine( playerNumber );
						tmpUShort = getChatLine(playerNumber);
						TXPacket[3] = tmpUShort & 0xFF;
						TXPacket[4] = (tmpUShort >> 8) & 0xFF;
						// playernumber
						TXPacket[6] = 2;
						memcpy( TXPacket+21, RXPacket+21, 16);
						// checksum
						rdcksum( TXPacket,PacketLen );
						BufferPacket( playerNumber, TXPacket, PacketLen, tmpUShort );
						
					}
					
					
				} else if (playerCMD  <= getLastCMD(playerNumber) ) {
					// toch een ack terugsturen aan client..
					PacketLen = sizeof(Acknowledge);
					memcpy( TXPacket, Acknowledge, PacketLen);
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
				}
			}
		}
		
		
		// CLIENT clGridPresent2 (C6....0D)
		if ( len==7 ) {
			if ( RXPacket[0] == 0xC6 && RXPacket[5] == 0x0D ) {
				
				// aanduiden als "afgebeeld"
				PacketPrinted = 1;
				
				// ack-nummer
				playerCMD = RXPacket[4]*256+RXPacket[3];
				
				if ( getLastCMD(playerNumber)+1 == playerCMD ) {
					
					changeLastCMD( playerNumber, playerCMD ); //laatste waarde onthouden..
					
					// aanduiden als verwerkt
					PacketProcessed = 1;
					
					MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client gridPresent2>");
					
					// ack terugsturen aan client..
					PacketLen = sizeof(Acknowledge);
					memcpy( TXPacket, Acknowledge, PacketLen);
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
					
				} else if (playerCMD  <= getLastCMD(playerNumber) ) {
					
					// toch een ack terugsturen aan client..
					PacketLen = sizeof(Acknowledge);
					memcpy( TXPacket, Acknowledge, PacketLen);
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
				}
				
			}
		}
		
		
		// CLIENT GridReady (C6....03)
		// Een speler is ready aan de startlijn.
		if ( (len==13 && RXPacket[0] == 0xC6 && RXPacket[5] == 0x03) ||
			(len>=15 && RXPacket[0] == 0xD6 && RXPacket[7] == 0x03) ) {
			
			Offset = ( RXPacket[0] == 0xD6 )? 2 : 0;
			
			//MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client gridReady>");
			// aanduiden als "afgebeeld"
			PacketPrinted = 1;
			
			// aanduiden als verwerkt
			PacketProcessed = 1;
			
			// ack-nummer
			playerCMD = RXPacket[4+Offset]*256+RXPacket[3+Offset];
			
			if ( getLastCMD(playerNumber)+1 == playerCMD ) {
				changeLastCMD( playerNumber, playerCMD ); //laatste waarde onthouden..
				
				
				MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client gridReady>");
				
				
				
				// Een C6-03 terugsturen naar de rest..
				// met de ID van deze speler erin..
				for ( i=1; i < NumPlayers; i++ ) {
					//@					if ( playerNumber != i ) {			////////////////////////////////////////////////
					//changed 2008apr30
					//					if ( (	( GlobalState <= stateNumbering && playerNumber != i ) ||
					//							( GlobalState > stateNumbering && getStartPlayerNr(playerNumber) != getStartPlayerNr(i) ) ) //changed 2008apr28
					if ( (	( GlobalState < stateSwitch && playerNumber != i ) ||
					 	 ( GlobalState >= stateSwitch && getStartPlayerNr(playerNumber) != getStartPlayerNr(i) ) ) 
						&& getPortNr(i) != 0 ) { //was 3
						PacketLen = sizeof(GridReadyC6);
						memcpy( TXPacket, GridReadyC6, PacketLen);
						// ChatLine
						increaseChatLine( i );
						tmpUShort = getChatLine(i);
						TXPacket[3] = tmpUShort & 0xFF;
						TXPacket[4] = (tmpUShort >> 8) & 0xFF;
						/*
						// ???
						TXPacket[6] = 0x00;
						TXPacket[7] = 0x00;
						TXPacket[8] = 0x00;
						*/
						// speler nummer (0..7)
						TXPacket[9] = (u_char)getStartPlayerNr(playerNumber);
						//	TXPacket[9] = PlayerIndex[playerNumber];
						
						// checksum
						rdcksum( TXPacket,PacketLen );
						SendToPlayer( i, PacketLen );
					}
				}
				// de host regel# ophogen..
				increaseChatLine(0);
				
				// ACK terugsturen aan client..
				PacketLen = sizeof(Acknowledge);//LongAcknowledge
				memcpy( TXPacket, Acknowledge, PacketLen);//LongAcknowledge
				// CMD
				TXPacket[3] = playerCMD & 0xFF;
				TXPacket[4] = (playerCMD >> 8) & 0xFF;
				// checksum
				rdcksum( TXPacket,PacketLen );
				ReplyToPlayer( PacketLen );
				
				
				// de State van deze speler aanpassen..  GlobalState is nu 5..
				changeState( playerNumber, 6 ); //GlobalState+1
				
				LastPlayerNr = playerNumber; // PlayerNr onthouden..	//obsolete ??
				LastPlayerRX3 = RXPacket[3+Offset]; // CMD onthouden..	//
				LastPlayerRX4 = RXPacket[4+Offset];						//
				
				// Als iedereen klaar is, kan de rally beginnen..
				// de game-status controleren..
				CheckGlobalState();
				
			} else if (playerCMD  <= getLastCMD(playerNumber) ) {
				
				// toch een ack terugsturen aan client..
				PacketLen = sizeof(Acknowledge);
				memcpy( TXPacket, Acknowledge, PacketLen);
				TXPacket[3] = playerCMD & 0xFF;
				TXPacket[4] = (playerCMD >> 8) & 0xFF;
				rdcksum( TXPacket,PacketLen );
				ReplyToPlayer( PacketLen );
			}
		}
		
		
		// CLIENT Positie (& TimeStamp) (42)(43)
		if ( ((len==30 && RXPacket[0]==0x42 && RXPacket[3]==0x05) || 
			(len==35 && RXPacket[0]==0x43 && RXPacket[3]==0x01)) 
			) {
			//			 && (GlobalState==stateRacing && playerNumber!=-1 && getFinished(playerNumber)==0 && getRetired(playerNumber)==0)
			//!				MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client 3D-position+timecode>");
			// aanduiden als "afgebeeld"
			PacketPrinted = 1;
			
			if ( playerNumber != -1 ) {
				
				// aanduiden als verwerkt
				PacketProcessed = 1;
				
				
				if ( RXPacket[0] == 0x42 ) {
					// deze positie ed.. bewaren in de player LastPos buffer.
					memcpy( &ptr_players[PlayerIndex[playerNumber]].LastPos, RXPacket+3, 27);
					
					// bewaren deze hele race
					//if ( ptr_players[i].PosCount < 100 ) {
					//	memcpy( &ptr_players[PlayerIndex[playerNumber]].PosRec[ptr_players[i].PosCount++], RXPacket+3, 27); 
					//}
					
					// toevoegen aan de positie-buffer..
					AddLastPos( playerNumber );
					
					if ( ptr_players[PlayerIndex[playerNumber]].PosCount < 18000 ) {
						// let op: PosCount++
						memcpy( &ptr_players[PlayerIndex[playerNumber]].PosRec[27*ptr_players[PlayerIndex[playerNumber]].PosCount++], &ptr_players[PlayerIndex[playerNumber]].LastPos, 27 );
					}
					
					// indien nog niet gefinished..de positie/tijd ed. overnemen
					if ( getFinished(playerNumber) == 0 ) {
						// de actuele race-tijd overnemen,
						// alleen als de tijd in dit packet > RaceTime
						tmpULong = RXPacket[5]+(RXPacket[6]<<8)+(RXPacket[7]<<16)+(RXPacket[8]<<24);
						if ( tmpULong>getRaceTime(playerNumber) ) {
							changeRaceTime( playerNumber, tmpULong );
							// laatst ontvangen Position-packet al verstuurd aan spelers?  reset
							//%							for ( tmpInt=0; tmpInt < MaxPlayers; tmpInt++ ) {
							//%								ptr_players[PlayerIndex[playerNumber]].PosSent[tmpInt] = 0; //nog niet verzonden naar andere speler(j)
							//%							}
						}
						// het percentage tot aan de split0
						changePercToSplit0( playerNumber, RXPacket[24] );
					}
					
					/////////////////////////////////////////////////////
					// NO-CUTS controle:
					
					/*					if ( GlobalNoCutsEnabled == 1 ) {
					float PosX=0.0f;
					float PosY=0.0f;
					float PosZ=0.0f;
					memcpy(&PosX, RXPacket+15, 4);//4 bytes vanaf offset 12 in LastPos
					memcpy(&PosY, RXPacket+19, 4);
					memcpy(&PosZ, RXPacket+23, 4);
					if ( CheckNoCut(playerNumber, PosX,PosY,PosZ) ) {
					sprintf( log, "%s", getNick(playerNumber) );
					MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<cut>", log);
					sprintf( log, "<cut1> %s", getNick(playerNumber) );
					Log( log );
					}
					changeLastPosX( playerNumber, PosX );
					changeLastPosY( playerNumber, PosY );
					changeLastPosZ( playerNumber, PosZ );
					}
					*/
					if ( GlobalNoCutsEnabled == 1 ) {
						float PosX;
						float PosY;
						float PosZ;
						memcpy(&PosX, RXPacket+15, 4);//4 bytes vanaf offset 12 in LastPos
						memcpy(&PosY, RXPacket+19, 4);
						memcpy(&PosZ, RXPacket+23, 4);
						float LPosX = getLastPosX( playerNumber );
						float LPosY = getLastPosY( playerNumber );
						float LPosZ = getLastPosZ( playerNumber );
						
						if ( ptr_players[PlayerIndex[playerNumber]].RaceTime>5000 ) {//changed 2008apr28
							for (i=0; i<GlobalCutsCount; i++ ) {
								float NoCutX1 = GlobalCuts[i].X1;
								float NoCutY1 = GlobalCuts[i].Y1;
								float NoCutZ1 = GlobalCuts[i].Z1;
								float NoCutX2 = GlobalCuts[i].X2;
								float NoCutY2 = GlobalCuts[i].Y2;
								float NoCutZ2 = GlobalCuts[i].Z2;
								
								
								if ( LineSegmentIntersection(PosX,PosZ, LPosX,LPosZ, NoCutX1,NoCutZ1, NoCutX2,NoCutZ2) ) {
									//sprintf( log, "<cut>: XYZ(%f, %f, %f) <-> XYZ(%f, %f, %f)", NoCutX1, NoCutY1, NoCutZ1, NoCutX2, NoCutY2, NoCutZ2 );
									//sprintf( log, "%s", getNick(playerNumber) );
									sprintf( log, "%s: Pos XYZ(%f, %f, %f) ", getNick(playerNumber), PosX,PosY,PosZ );
									MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<cut>", log);
									//Log( log );
									// de speler is aan het cutten..
									Server_RetireDriver( playerNumber );
								}
							}
						}
						
						changeLastPosX( playerNumber, PosX );
						changeLastPosY( playerNumber, PosY );
						changeLastPosZ( playerNumber, PosZ );
					}
					
					/////////////////////////////////////////////////////
					
				} else {
					
					//offsets[4..7] = timestamp
					/*					
					// Deze timestamp bewaren
					tmpPing = RXPacket[4] + (RXPacket[5]<<8) + (RXPacket[6]<<16) + (RXPacket[7]<<24);
					//					ptr_players[PlayerIndex[playerNumber]].Ping2 = tmpPing;
					//					tmpULong = minisec() - tmpPing;
					changePing( playerNumber, minisec() - tmpPing );
					*/
					
					memcpy( &ptr_players[PlayerIndex[playerNumber]].LastPos, RXPacket+8, 27);
					// toevoegen aan de positie-buffer..
					AddLastPos( playerNumber );
					
					// indien nog niet gefinished..de positie/tijd ed. overnemen
					// alleen als de tijd in dit packet > RaceTime
					if ( getFinished(playerNumber) == 0 ) {

						tmpULong = RXPacket[10]+(RXPacket[11]<<8)+(RXPacket[12]<<16)+(RXPacket[13]<<24);

						if ( tmpULong>getRaceTime(playerNumber) ) {

							changeRaceTime( playerNumber, tmpULong );
							// laatst ontvangen Position-packet al verstuurd aan spelers?  reset
							//%							for ( tmpInt=0; tmpInt < MaxPlayers; tmpInt++ ) {
							//%								ptr_players[PlayerIndex[playerNumber]].PosSent[tmpInt] = 0; //nog niet verzonden naar andere speler(j)
							//%							}
						}
						changePercToSplit0( playerNumber, RXPacket[29] );
					}
					/////////////////////////////////////////////////////
					// NO-CUTS controle:
					/*
					if ( GlobalNoCutsEnabled == 1 ) {
					float PosX;
					float PosY;
					float PosZ;
					memcpy(&PosX, RXPacket+20, 4);//4 bytes vanaf offset 12 in LastPos
					memcpy(&PosY, RXPacket+24, 4);
					memcpy(&PosZ, RXPacket+28, 4);
					if ( CheckNoCut(playerNumber, PosX,PosY,PosZ) == 1 ) {
					sprintf( log, "%s", getNick(playerNumber) );
					MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<cut>", log);
					sprintf( log, "<cut2> %s", getNick(playerNumber) );
					Log( log );
					}
					changeLastPosX( playerNumber, PosX );
					changeLastPosY( playerNumber, PosY );
					changeLastPosZ( playerNumber, PosZ );
					}
					*/
					if ( GlobalNoCutsEnabled == 1 ) {
						float PosX;
						float PosY;
						float PosZ;
						memcpy(&PosX, RXPacket+20, 4);//4 bytes vanaf offset 12 in LastPos
						memcpy(&PosY, RXPacket+24, 4);
						memcpy(&PosZ, RXPacket+28, 4);
						float LPosX = getLastPosX( playerNumber);
						float LPosY = getLastPosY( playerNumber);
						float LPosZ = getLastPosZ( playerNumber);
						
						if ( ptr_players[PlayerIndex[playerNumber]].RaceTime>5000 ) {//changed 2008apr28
							for (i=0; i<GlobalCutsCount; i++ ) {
								float NoCutX1 = GlobalCuts[i].X1;
								float NoCutY1 = GlobalCuts[i].Y1;
								float NoCutZ1 = GlobalCuts[i].Z1;
								float NoCutX2 = GlobalCuts[i].X2;
								float NoCutY2 = GlobalCuts[i].Y2;
								float NoCutZ2 = GlobalCuts[i].Z2;
								
								
								if ( LineSegmentIntersection(PosX,PosZ, LPosX,LPosZ, NoCutX1,NoCutZ1, NoCutX2,NoCutZ2) ) {
									//sprintf( log, "%s", getNick(playerNumber) );
									sprintf( log, "%s: Pos XYZ(%f, %f, %f) ", getNick(playerNumber), PosX,PosY,PosZ );
									MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<cut>", log);
									//Log( log );
									// de speler is aan het cutten..
									Server_RetireDriver( playerNumber );
								}
							}
						}
						
						changeLastPosX( playerNumber, PosX );
						changeLastPosY( playerNumber, PosY );
						changeLastPosZ( playerNumber, PosZ );
					}
					
					/////////////////////////////////////////////////////
					
				}
				/* //%
				//test, als deze byte == 0, dan dit packet niet verzenden naar anderen..
				//want anders is de auto onzichtbaar (of als een streep zichtbaar) bij diegene die het fout ziet (fout positie)
				if ( ptr_players[PlayerIndex[playerNumber]].LastPos[10] == 0x00 ) {
				//als "reeds verzonden" merken..dan worden ze niet eens verzonden naar anderen..
				for ( tmpInt=0; tmpInt < MaxPlayers; tmpInt++ ) ptr_players[PlayerIndex[playerNumber]].PosSent[tmpInt] = 1;
				}
				*/
				// de host heeft dus nu van deze speler een positie ontvangen..
				// Is dit het eerste Pos-packet van deze client, na de start van de race??
				if ( ptr_players[PlayerIndex[playerNumber]].PosReceived == 0 ) {
					// de timestamp op tijdstip eerst ontvangen pos-packet van speler(playerNumber)
					ptr_players[PlayerIndex[playerNumber]].StartTime = getRaceTime(playerNumber);
				}
				ptr_players[PlayerIndex[playerNumber]].PosReceived++;
				
				
				if ( RXPacket[0] == 0x43 ) {
					// de client pingt, en geeft een timestamp (4 bytes).
					// Deze timestamp bewaren voor de ping meting..
					tmpPing = RXPacket[4] + (RXPacket[5]<<8) + (RXPacket[6]<<16) + (RXPacket[7]<<24);
					
					// De clients laatst gestuurde timeval onthouden..
					ptr_players[PlayerIndex[playerNumber]].LastTimeValue = tmpPing;
					
					// Direct een "\x40" "\x00\x00" "\x02" "\x00\x00\x00\x00" terug-zenden,
					// dan antwoordt de client ook weer met een 40-02 (+timestamp)
					PacketLen = sizeof(ServerPing);				
					memcpy( TXPacket, ServerPing, PacketLen);
					//PacketLen = sizeof(ServerPing);				
					//memcpy( TXPacket, ServerPing, PacketLen);
					if ( GlobalState == stateRacing ) { //was 9
						tmpULong = minisec();
						TXPacket[4] = tmpULong & 0xFF; // LastTimeValue overnemen..
						TXPacket[5] = (tmpULong >> 8) & 0xFF;
						TXPacket[6] = (tmpULong >> 16) & 0xFF;
						TXPacket[7] = (tmpULong >> 24) & 0xFF;
						// timestamp erin plakken.. die krijg je terug als ze aan het racen zijn...
					} else {
						TXPacket[4] = RXPacket[4]; // LastTimeValue overnemen..
						TXPacket[5] = RXPacket[5];
						TXPacket[6] = RXPacket[6];
						TXPacket[7] = RXPacket[7];
					}
					rdcksum( TXPacket,PacketLen );
					Reply1ToPlayer( PacketLen );
					
					// de ping in ms. berekenen..  tmpPing-Ping1 (dwz. de huidige Ping1 - de vorige)
					// De tijd is in microseconden, dus eerst delen door 1000 voor millisec.
					//	tmpULong = (tmpPing - ptr_players[PlayerIndex[playerNumber]].Ping1) / 1000;
					//	changePing( playerNumber, tmpULong );
					//test
					//tmpULong = minisec() - tmpPing;
					//changePing( playerNumber, tmpULong );
					///test
					
					
					// de timestamp in dit pakketje bewaren..
					ptr_players[PlayerIndex[playerNumber]].Ping1 = tmpPing;
					
					//test!!!!!DEBUG!!!!!
					//Server_SendPos();
					//Server_PingList( -1, 0 ); //-1=naar iedereen, 1=list met timestamp
					//PosSentCount++;
					///test
					
				} else {
					// 42
				}
				
				//Server_SendPos();
				//PosSentCount++;
				
				
				//hi-lo goeie
				/*
				if ( RXPacket[0]==0x42 ) {
				changeRotation( playerNumber,	(RXPacket[6]<<24)+(RXPacket[7]<<16)+(RXPacket[8]<<8)+RXPacket[9], \
				(RXPacket[10]<<24)+(RXPacket[11]<<16)+(RXPacket[12]<<8)+RXPacket[13], \
				(RXPacket[14]<<24)+(RXPacket[15]<<16)+(RXPacket[16]<<8)+RXPacket[17] );
				changePosition( playerNumber,	(RXPacket[18]<<24)+(RXPacket[19]<<16)+(RXPacket[20]<<8)+RXPacket[21], \
				(RXPacket[22]<<24)+(RXPacket[23]<<16)+(RXPacket[24]<<8)+RXPacket[25], \
				(RXPacket[26]<<24)+(RXPacket[27]<<16)+(RXPacket[28]<<8)+RXPacket[29] );
				} else {
				changeRotation( playerNumber,	(RXPacket[6+5]<<24)+(RXPacket[7+5]<<16)+(RXPacket[8+5]<<8)+RXPacket[9+5], \
				(RXPacket[10+5]<<24)+(RXPacket[11+5]<<16)+(RXPacket[12+5]<<8)+RXPacket[13+5], \
				(RXPacket[14+5]<<24)+(RXPacket[15+5]<<16)+(RXPacket[16+5]<<8)+RXPacket[17+5] );
				changePosition( playerNumber,	(RXPacket[18+5]<<24)+(RXPacket[19+5]<<16)+(RXPacket[20+5]<<8)+RXPacket[21+5], \
				(RXPacket[22+5]<<24)+(RXPacket[23+5]<<16)+(RXPacket[24+5]<<8)+RXPacket[25+5], \
				(RXPacket[26+5]<<24)+(RXPacket[27+5]<<16)+(RXPacket[28+5]<<8)+RXPacket[29+5] );
				}
				*/
			}
		}
		
		
		// CLIENT INTERMEDIATE ??
		// Packet == "\xC6"...."\x0C" ?? 
		if (len==21) {
			if ( RXPacket[0]==0xC6 && RXPacket[5]==0x0C ) { 
				
				// aanduiden als "afgebeeld"
				PacketPrinted = 1;
				
				// dit CMD nog niet gehad??
				// ack-nummer
				playerCMD = RXPacket[4]*256+RXPacket[3];
				
				if ( getLastCMD(playerNumber)+1 == playerCMD ) {
					
					changeLastCMD( playerNumber, playerCMD ); //laatste waarde onthouden..
					
					// aanduiden als verwerkt
					PacketProcessed = 1;
					
					// positie/ranking van de speler
					tmpPlayerNr = RXPacket[6];
					/*
					//fail safe
					if ( tmpPlayerNr == 0 ) {
					// oops
					}
					*/
					// split#
					tmpSplit = RXPacket[7];
					
					// de speler race split-tijd overnemen..
					tmpTime = RXPacket[13]+(RXPacket[14]<<8)+(RXPacket[15]<<16)+(RXPacket[16]<<24);
					changeSplitTime( playerNumber, tmpSplit, tmpTime );
					changeRaceTime( playerNumber, tmpTime );
					increaseSplitsDone( playerNumber );
					MStoTimeString( tmpTime, &timeString );
					sprintf( log, "%d, %d, %s", tmpPlayerNr, tmpSplit, timeString );
					MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client intermediate: playerNr,split,time>", log);
					//MsgprintyIP3(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client intermediate: playerNr,split>", tmpPlayerNr, tmpSplit);
					
					// Een C6....0C sturen naar iedereen behalve speler playerNumber (en de host)
					PacketLen = sizeof(ServerIntermediate);
					memcpy( TXPacket, ServerIntermediate, PacketLen);
					for ( i=1; i < NumPlayers; i++ ) {
						//@						if ( playerNumber != i ) { ////////////////////////////////////////////////////////////
						if ( (	( GlobalState <= stateSwitch && playerNumber != i ) ||
							( GlobalState > stateSwitch && getStartPlayerNr(i) != tmpPlayerNr ) ) 
							&& getPortNr(i) != 0 ) { //was 3
							// CMD
							increaseChatLine(i);
							tmpUShort = getChatLine(i);
							TXPacket[3] = tmpUShort & 0xFF;
							TXPacket[4] = (tmpUShort >> 8) & 0xFF;
							// PlayerNr van deze speler(playerNumber)
							TXPacket[6] = tmpPlayerNr;
							// split
							TXPacket[7] = RXPacket[7];
							
							// "\xD0\xD6\x53\x00" constant ??
							
							// totaal tijd tot dusver gereden..+ de rest van het packet..
							memcpy( TXPacket+13, RXPacket+13, 8);
							// checksum
							rdcksum( TXPacket,PacketLen );
							//							SendToPlayer( i, PacketLen );
							BufferPacket( i, TXPacket, PacketLen, tmpUShort );
						}
					}
					//BuffersResend();
					// host chatline verhogen..
					increaseChatLine(0);
					
					
					// ACK terugsturen aan client..
					PacketLen = sizeof(Acknowledge);
					memcpy( TXPacket, Acknowledge, PacketLen);
					// CMD
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					// checksum
					rdcksum( TXPacket,PacketLen );
					//Reply1ToPlayer( PacketLen );
					ReplyToPlayer( PacketLen );
					ReplyToPlayer( PacketLen );
					
				} else if (playerCMD  <= getLastCMD(playerNumber) ) {
					// toch een ack terugsturen aan client..
					PacketLen = sizeof(Acknowledge);
					memcpy( TXPacket, Acknowledge, PacketLen);
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
				}
				
			}
			
		}
		
		
		// CLIENT FINISH ??
		// Packet == "\xC6"...."\x0B" ?? 
		if (len==21) {
			if ( RXPacket[0]==0xC6 && RXPacket[5]==0x0B ) { 

//waar?				if ( GlobalState == stateRacing ) {

				MsgprintyIP2(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client finish: playerNr>", RXPacket[6]);
				// aanduiden als "afgebeeld"
				PacketPrinted = 1;
				
				// dit CMD nog niet gehad??
				// ack-nummer
				playerCMD = RXPacket[4]*256+RXPacket[3];
				if ( getLastCMD(playerNumber)+1 == playerCMD ) {
					
					changeLastCMD( playerNumber, playerCMD ); //laatste waarde onthouden..
					
					// aanduiden als verwerkt
					PacketProcessed = 1;
					
					// player
					tmpPlayerNr = RXPacket[6];
					/*
					//fail safe
					if ( tmpPlayerNr == 0 ) {
					// oops
					}
					*/
					// RXPacket[7] == 0x4f.
					// De race split-tijd
					tmpTime = RXPacket[13]+(RXPacket[14]<<8)+(RXPacket[15]<<16)+(RXPacket[16]<<24);
					// de race eind-tijd
					changeRaceTime( playerNumber, tmpTime );
					
					// teller voor het aantal spelers dat reeds de finish heeft bereikt
					GlobalPlayersFinished++;
					// de race-positie voor deze speler
					changeFinished( playerNumber, GlobalPlayersFinished);
					
					// Het tijstip bewaren van 1e finish..
					// ivm. het af en toe niet retiren van een speler,
					// 60 seconden na de eerste die finisht.
					// In het geval van deze timeout, zelf de speler retiren..
					if ( GlobalPlayersFinished == 1 ) {
						GlobalFinishTimeVal = minisec();
					}
					
					// De State van deze speler wordt nu verhoogd.. 
					//changeState( playerNumber, 8 );
					
					//---
					u_long	FastestTime;
					u_long	MemberID;
					u_char	Car;
					u_char	Gearbox;
					u_long	Split0;
					u_long	Split1;
					u_long	Split2;
					u_long	RaceTime;//tmpTime
					u_char*	IP;
					u_char*	Nick;
					u_char*	Country;
					
					IP = getIP(playerNumber);
					Nick = getNick(playerNumber);
					Country = StrCountry[getCountry(playerNumber)][0];
					// zoek het MemberID op in de DB.
					MemberID = sqlGetMember( IP, Nick, Country );
					
					// de speler in TABLE CMR04_Times
					if ( MemberID != 0 ) {
						
						//Log( "einde race" );
						Car = getCar(playerNumber);
						Gearbox = getGearbox(playerNumber);
						Split0 = getSplitTime(playerNumber, 0);
						Split1 = getSplitTime(playerNumber, 1);
						Split2 = getSplitTime(playerNumber, 2);
						RaceTime = getRaceTime(playerNumber);//tmpTime
//	sprintf( log, "Adding Time for member: %d %d %d %d %d %d %d %d %d", MemberID, GlobalDamage, Car, Gearbox, GlobalStages, Split0, Split1, Split2, RaceTime );
//	MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), log );
//							
						//sqlAddTime( MemberID, GlobalDamage, Car, Gearbox, GlobalStages, Split0, Split1, Split2, RaceTime, ptr_players[PlayerIndex[playerNumber]].PosCount, &ptr_players[PlayerIndex[playerNumber]].PosRec[0] );
						sqlAddTime_CarType( MemberID, GlobalDamage, Car, Gearbox, GlobalStages, Split0, Split1, Split2, RaceTime, getCarTypeStr(playerNumber), ptr_players[PlayerIndex[playerNumber]].PosCount, &ptr_players[PlayerIndex[playerNumber]].PosRec[0] );
					}
					//---
					
					// Een C6....0B sturen naar iedereen behalve speler playerNumber (en de host)
					PacketLen = sizeof(ServerFinish);
					memcpy( TXPacket, ServerFinish, PacketLen);
					for ( i=1; i < NumPlayers; i++ ) {
						//@						if ( playerNumber != i ) { ///////////////////////////////////////////////////////
						if ( (	( GlobalState < stateSwitch && playerNumber != i ) ||
							( GlobalState >= stateSwitch && getStartPlayerNr(i) != tmpPlayerNr ) )  
//						if ( (	( GlobalState <= stateLobbyLeave && playerNumber != i ) ||
//							( GlobalState > stateLobbyLeave && getStartPlayerNr(i) != tmpPlayerNr ) )  
							&& getPortNr(i) != 0 ) { //was 3
							// CMD
							increaseChatLine(i);
							tmpUShort = getChatLine(i);
							TXPacket[3] = tmpUShort & 0xFF;
							TXPacket[4] = (tmpUShort >> 8) & 0xFF;
							// PlayerNr van deze speler(playerNumber)
							TXPacket[6] = tmpPlayerNr;
							
							// is deze speler de 1e die over de finish gaat?
							if ( getFinished(playerNumber) == 1 ) {
								// de 1e over de finish, dan waarde 0x00. 
								// vanaf de 2e finisher, waarde 0x13
								TXPacket[7] = 0x00;
								//test
								TXPacket[9] = 0x40; 
								TXPacket[10] = 0x93;
								TXPacket[11] = 0x17;
								TXPacket[12] = 0x00;
								///test
							} else {
								// vanaf de 2e finisher, waarde 0x13
								TXPacket[7] = 0x13;
								// constant??
								//TXPacket[8] = 0x00; 
								TXPacket[9] = 0x16; 
								TXPacket[10] = 0x00;
								TXPacket[11] = 0x00;
								TXPacket[12] = 0x00;
							}
							// totaal tijd tot dusver gereden..+ de rest van het packet..
							memcpy( TXPacket+13, RXPacket+13, 8);
							
							// checksum
							rdcksum( TXPacket,PacketLen );
							//						SendToPlayer( i, PacketLen );
							BufferPacket( i, TXPacket, PacketLen, tmpUShort );
						}
					}
					//BuffersResend();
					// host chatline verhogen..
					increaseChatLine(0);
					
					
					// ACK terugsturen aan client..
					PacketLen = sizeof(Acknowledge);
					memcpy( TXPacket, Acknowledge, PacketLen);
					// CMD
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					// checksum
					rdcksum( TXPacket,PacketLen );
					Reply1ToPlayer( PacketLen );
					//					ReplyToPlayer( PacketLen );
					
					
					// Allemaal over de finish??
					// dan "terug naar Lobby" mogelijk maken..
					//CheckRetireHost();
					
				} else if (playerCMD  <= getLastCMD(playerNumber) ) {
					// toch een ack terugsturen aan client..
					PacketLen = sizeof(Acknowledge);
					memcpy( TXPacket, Acknowledge, PacketLen);
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
				}
			}
		}
		
		
		
		// CLIENT RETIRE ?? 
		// Packet == "\xC6"...."\x07"
		if ( (len==7 && RXPacket[0]==0xC6 && RXPacket[5]==0x07) || 
			(len==9 && RXPacket[0]==0xD6 && RXPacket[7]==0x07) ) {
			//d6 5c fe 65 0 a 0 7 2
			// de anderen een C6-07 sturen,
			// daarna deze speler een ACK.
			
			// ack-nummer
			playerCMD = RXPacket[4]*256+RXPacket[3];
			
			Offset = ( RXPacket[0] == 0xD6 )? 2 : 0;
			
			// player
			tmpPlayerNr = RXPacket[6+Offset];
			/*
			//fail safe
			if ( tmpPlayerNr == 0 ) {
			// oops
			}
			*/
			
			//MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client retire>");
			//MsgprintyIP2(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client retire: playerNr>", RXPacket[6+Offset]);
			// aanduiden als "afgebeeld"
			PacketPrinted = 1;
			
			if ( playerNumber != -1 ) {
				if ( getLastCMD(playerNumber)+1 == playerCMD ) {
					changeLastCMD( playerNumber, playerCMD ); //laatste waarde onthouden..
					//}
					// aanduiden als verwerkt
					PacketProcessed = 1;
					
					
					// C6-07 naar de anderen sturen..
					PacketLen = sizeof(ServerRetire);
					memcpy( TXPacket, ServerRetire, PacketLen);
					for ( i=1; i < NumPlayers; i++ ) {
						//@						if ( playerNumber != i ) { //////////////////////////////////////////////////////////
						if ( (	( GlobalState < stateSwitch && playerNumber != i ) ||
							( GlobalState >= stateSwitch && getStartPlayerNr(i) != tmpPlayerNr ) ) 
							&& getPortNr(i) != 0 ) { //was 3
							//						if ( (	( GlobalState <= stateNumbering && playerNumber != i ) ||
							//								( GlobalState > stateNumbering && getStartPlayerNr(i) != tmpPlayerNr ) )  
							// CMD
							//changeLastCMD( playerNumber, getLastCMD(playerNumber)+1 ); 
							sprintf( log, "%d %d",  RXPacket[6+Offset], getStartPlayerNr(playerNumber) );
							MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client retire: playerNr, playerstartnr>", log);
							//sprintf( log, "client retire: %s", getNick(playerNumber));
							//Log( log );
							
							increaseChatLine( i );
							tmpUShort = getChatLine(i);
							TXPacket[3] = tmpUShort & 0xFF;
							TXPacket[4] = (tmpUShort >> 8) & 0xFF;
							// playerNumber van deze speler
							TXPacket[6] = tmpPlayerNr;
							//
							rdcksum( TXPacket,PacketLen );
							//ReplyToPlayer( PacketLen );
							BufferPacket( i, TXPacket, PacketLen, tmpUShort );
						}
					}
					//BuffersResend();
					increaseChatLine(0);//changed 2008apr28
					
					
					// ack terugsturen aan client..
					PacketLen = sizeof(Acknowledge);
					memcpy( TXPacket, Acknowledge, PacketLen);
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
					ReplyToPlayer( PacketLen );
					
					
					// indien nog niet retired..
					if ( getRetired(playerNumber)==0 ) {
						// deze speler .hasRetired bijwerken..
						changeRetired( playerNumber, 1 );
						// het aantal spelers retired tot dusver..
						GlobalPlayersRetired++;
						// evt. "terug naar Lobby" mogelijk maken..
						//CheckRetireHost();
					}
					
				} else if (playerCMD  == getLastCMD(playerNumber) ) {
					// toch een ack terugsturen aan client..
					PacketLen = sizeof(Acknowledge);
					memcpy( TXPacket, Acknowledge, PacketLen);
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
				} else if (playerCMD  < getLastCMD(playerNumber) ) {
					//ongeldige/oude CMD gevonden..
					sprintf( log, "LATE RETIRE CMD:%d  ", getLastCMD(playerNumber)+1 );
					for ( i=0; i<len ; i++ ) {
						sprintf( log,"%s%x ", log, RXPacket[i] );
					}
					//Log( log );
					// toch een ack terugsturen aan client..
					PacketLen = sizeof(Acknowledge);
					memcpy( TXPacket, Acknowledge, PacketLen);
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
				}
				//nieuw
				
				// playerNumber anders dan het nummer in dit pakketje?
				if (getStartPlayerNr(playerNumber) != tmpPlayerNr) {
					// er is een RETIRE blijven hangen...
					sprintf( log, "AGAIN RETIRE CMD:%d  Current:%d", playerCMD, getLastCMD(playerNumber) );
					for ( i=0; i<len ; i++ ) {
						sprintf( log,"%s%x ", log, RXPacket[i] );
					}
					//Log( log );
					
					// toch een ack terugsturen aan client..
					PacketLen = sizeof(Acknowledge);
					memcpy( TXPacket, Acknowledge, PacketLen);
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
					
				}
				
				
			}
			
		}
		
		
		// CLIENT QUIT ??
		// Packet == "\xC6"...."\x08"
		// Op dit pakketje volgt altijd een paar packets: CA
		// Deze quit valt iig. voor als ze aan het racen zijn..
		if (len==13) {
			if ( RXPacket[0]==0xC6 && RXPacket[5]==0x08 ) { 
			/*
			MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client quit>");
			// aanduiden als "afgebeeld"
			PacketPrinted = 1;
				*/
				
				// ack-nummer
				playerCMD = RXPacket[4]*256+RXPacket[3];
				
				
				if ( playerNumber > 0 ) {
					
					if ( getLastCMD(playerNumber)+1 == playerCMD ) {
						changeLastCMD( playerNumber, playerCMD ); //laatste waarde onthouden..
						
						// aanduiden als verwerkt
						PacketProcessed = 1;

						sprintf( log, " %d", RXPacket[6]);
						MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client quit: player>", log);
						//sprintf( log, "client quit: %s", getNick(playerNumber));
						//Log( log );
						// aanduiden als "afgebeeld"
						PacketPrinted = 1;
						
						// player
						tmpPlayerNr = RXPacket[6];
						/*
						//fail safe
						if ( tmpPlayerNr == 0 ) {
						// oops
						}
						*/
						
						// de andere spelers ook een C6-08 sturen..
						PacketLen = sizeof(ServerQuit);
						memcpy( TXPacket, ServerQuit, PacketLen);
						for ( i=1; i < NumPlayers; i++ ) {
							//changed 2008mei1
							if ( (	( GlobalState < stateSwitch && playerNumber != i ) ||
								( GlobalState >= stateSwitch && getStartPlayerNr(i) != tmpPlayerNr ) ) 
								&& getPortNr(i) != 0 ) { //was 3
								
								// CMD
								increaseChatLine( i );
								tmpUShort = getChatLine(i);
								TXPacket[3] = tmpUShort & 0xFF;
								TXPacket[4] = (tmpUShort >> 8) & 0xFF;
								// playerNumber van deze speler
								TXPacket[6] = tmpPlayerNr;
								//
								rdcksum( TXPacket,PacketLen );
								//ReplyToPlayer( PacketLen );
								BufferPacket( i, TXPacket, PacketLen, tmpUShort );
							}
						}
						//BuffersResend();
increaseChatLine(0);//changed 2008mei5
						
						
						// ack terugsturen aan client..
						PacketLen = sizeof(Acknowledge);
						memcpy( TXPacket, Acknowledge, PacketLen);
						TXPacket[3] = playerCMD & 0xFF;
						TXPacket[4] = (playerCMD >> 8) & 0xFF;
						rdcksum( TXPacket,PacketLen );
						ReplyToPlayer( PacketLen );
						ReplyToPlayer( PacketLen );
						
						/*						
						// de speler verwijderen, en alle variabelen bijwerken..
						RemovePlayer( playerNumber );
						*/
						
						changeState( playerNumber, 0 );
						changeQuit( playerNumber, 1);
						GlobalPlayersQuit++;

						RemovePlayer( playerNumber, (GlobalState<stateSwitch)?1:0 );
//						RemovePlayer( playerNumber );   //  20080505   toegevoegd... naar aanleiding van verhaal C.. en ik blijf net in een server staan terwijl ik Quit deed...

					}
					
				} else if (playerCMD  <= getLastCMD(playerNumber) ) {
					
					// toch een ack terugsturen aan client..
					PacketLen = sizeof(Acknowledge);
					memcpy( TXPacket, Acknowledge, PacketLen);
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
				}
			}
		}
		
		
		// CLIENT Goner ??
		if ( len==5 ) {
			if ( RXPacket[0] == 0xCA ) {
				
				// ack-nummer
				playerCMD = RXPacket[4]*256+RXPacket[3];
				
				if ( playerNumber > 0 ) {
					
					//					if ( getLastCMD(playerNumber)+1 == playerCMD ) {
					
					//changeLastCMD( playerNumber, playerCMD ); //laatste waarde onthouden..
					
					// aanduiden als verwerkt
					PacketProcessed = 1;


					MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client: CA>");
					// aanduiden als "afgebeeld"
					PacketPrinted = 1;
					
					
					// ack terugsturen aan client..
					PacketLen = sizeof(ServerACKGoner);
					memcpy( TXPacket, ServerACKGoner, PacketLen);
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
					ReplyToPlayer( PacketLen );
					
					// nieuw
					/*
					// 54
					PacketLen = sizeof(ServerACKGoner);
					memcpy( TXPacket, ServerACKGoner, PacketLen);
					// client-handle overnemen..
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					// en een checksum hier?? zeker vergeten?..
					ReplyToPlayer( PacketLen );
					ReplyToPlayer( PacketLen );
					*/
					// /nieuw

					// verwijder de speler
					changeState( playerNumber, 0 );
					changeQuit(playerNumber,1);
					GlobalPlayersQuit++;

					// de speler verwijderen, en alle variabelen bijwerken..
					RemovePlayer( playerNumber, (GlobalState<stateSwitch)?1:0 );
//					RemovePlayer( playerNumber );
					
					//					}
				} else if (playerCMD  <= getLastCMD(playerNumber) ) {
					
					// toch een ack terugsturen aan client..
					PacketLen = sizeof(Acknowledge);
					memcpy( TXPacket, Acknowledge, PacketLen);
					TXPacket[3] = playerCMD & 0xFF;
					TXPacket[4] = (playerCMD >> 8) & 0xFF;
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
				}
			}
		}
		
		
		// QUIT DOORGEVEN
		// Packet == "\xC6" "\x87"
		// Op dit pakketje volgt altijd een: DA
		// Deze quit komt in de lobby..
		if ( RXPacket[0] == 0xC6 && RXPacket[5] == 0x87 ) { 
			
			playerCMD = RXPacket[4]*256+RXPacket[3];
			
			if (playerNumber > 0 ) { // speler gevonden??

				// een bevestiging sturen naar de client
				if ( getLastCMD(playerNumber)+1 == playerCMD ) {
					changeLastCMD( playerNumber, playerCMD ); //laatste waarde onthouden..

					// aanduiden als verwerkt
					PacketProcessed = 1;

					
					MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client quit: C6-87>");
					// aanduiden als "afgebeeld"
					PacketPrinted = 1;
			
				
					// D6 88
					PacketLen = sizeof(ServerQuitAck);
					memcpy( TXPacket, ServerQuitAck, PacketLen);
					// client-handle overnemen..
					TXPacket[3] = RXPacket[3];
					TXPacket[4] = RXPacket[4];
					// ChatLine overnemen..
					increaseChatLine( playerNumber );
					tmpUShort = getChatLine( playerNumber );
					TXPacket[5] = tmpUShort & 0xFF;
					TXPacket[6] = (tmpUShort >> 8) & 0xFF;
					// spelernummer of JoinNr ??
					TXPacket[11] = RXPacket[9];		//JoinNr is 4 bytes lang
					TXPacket[12] = RXPacket[10];	//.
					TXPacket[13] = RXPacket[11];	//
					TXPacket[14] = RXPacket[12];	//
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
					
					
					// iemand verlaat het spel
					PacketLen = sizeof(Goner);
					memcpy( TXPacket, Goner, PacketLen);
					// ChatLine overnemen..
					increaseChatLine( playerNumber );
					tmpUShort = getChatLine( playerNumber );
					TXPacket[3] = tmpUShort & 0xFF;
					TXPacket[4] = (tmpUShort >> 8) & 0xFF;
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );

					// verwijder de speler
					changeState( playerNumber, 0 );
					changeQuit( playerNumber, 1);
					GlobalPlayersQuit++;

					RemovePlayer( playerNumber, (GlobalState<stateSwitch)?1:0 );
//					RemovePlayer( playerNumber );   //  20080505   toegevoegd... naar aanleiding van verhaal C.. en ik blijf net in een server staan terwijl ik Quit deed...

					// indien in de lobby, playerlist naar de rest

//					if ( GlobalState < stateSwitch ) { //changed: 2008apr30 , zit al in removeplayer
//						SendAllPlayerList( -1 );
//					}

				}
			} else if (playerCMD  <= getLastCMD(playerNumber) ) {
				
				// toch een ack terugsturen aan client..
				PacketLen = sizeof(Acknowledge);
				memcpy( TXPacket, Acknowledge, PacketLen);
				TXPacket[3] = playerCMD & 0xFF;
				TXPacket[4] = (playerCMD >> 8) & 0xFF;
				rdcksum( TXPacket,PacketLen );
				ReplyToPlayer( PacketLen );
			}
			
		}
		
		
		// CLIENT DA
		if ( RXPacket[0] == 0xDA ) { 
			
			if (playerNumber > 0 ) { // speler gevonden??
				// aanduiden als verwerkt
				PacketProcessed = 1;
				
				playerCMD = RXPacket[4]*256+RXPacket[3];
				
				//if ( getLastCMD(playerNumber)+1 == playerCMD ) {
				
				//changeLastCMD( playerNumber, playerCMD ); //laatste waarde onthouden..


				MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client DAag>");
				// aanduiden als "afgebeeld"
				PacketPrinted = 1;
				

				// 54
				PacketLen = sizeof(ServerACKGoner);
				memcpy( TXPacket, ServerACKGoner, PacketLen);
				// client-handle overnemen..
				TXPacket[3] = RXPacket[5];
				TXPacket[4] = RXPacket[6];
				rdcksum( TXPacket,PacketLen );
				ReplyToPlayer( PacketLen );
				
				
				// 44 6F 51
				PacketLen = sizeof(ServerQuitAck2);
				memcpy( TXPacket, ServerQuitAck2, PacketLen);
				ReplyToPlayer( PacketLen );
				ReplyToPlayer( PacketLen );
				
				// verwijder de speler
				changeState( playerNumber, 0 );
				changeQuit(playerNumber,1);
				GlobalPlayersQuit++;
				
				// de speler verwijderen, en alle variabelen bijwerken..
				RemovePlayer( playerNumber, (GlobalState<stateSwitch)?1:0 );
//				RemovePlayer( playerNumber );
				
				// playerlist naar de rest
				//SendAllPlayerList( -1 ); //zit al in removeplayer
				
				//} else if (playerCMD  <= getLastCMD(playerNumber) ) {
				/*
				// toch een ack terugsturen aan client..
				PacketLen = sizeof(Acknowledge);
				memcpy( TXPacket, Acknowledge, PacketLen);
				TXPacket[3] = playerCMD & 0xFF;
				TXPacket[4] = (playerCMD >> 8) & 0xFF;
				rdcksum( TXPacket,PacketLen );
				ReplyToPlayer( PacketLen );
				
				  }
				*/
			}
		}
		
		
		// QUITTER
		if ( len == 3 ) {
			if ( RXPacket[0]==0x41 && RXPacket[1]==0xCC && RXPacket[2]==0xEC ) {
				
				if ( playerNumber > 0 ) {
					
					// aanduiden als verwerkt
					PacketProcessed = 1;
					
				
					MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<client quitter: 41 CC EC>");
					// aanduiden als "afgebeeld"
					PacketPrinted = 1;
			
					
					// 54
					PacketLen = sizeof(ServerACKGoner);
					memcpy( TXPacket, ServerACKGoner, PacketLen);
					// client-handle overnemen..
					TXPacket[3] = RXPacket[5];
					TXPacket[4] = RXPacket[6];
					rdcksum( TXPacket,PacketLen );
					ReplyToPlayer( PacketLen );
					
//changed 2008mei5
					// de andere spelers ook een C6-08 sturen..
					PacketLen = sizeof(ServerQuit);
					memcpy( TXPacket, ServerQuit, PacketLen);
					tmpPlayerNr = (GlobalState<stateSwitch)?playerNumber:getStartPlayerNr(playerNumber);
					for ( i=1; i < NumPlayers; i++ ) {
						//changed 2008mei1
						if ( (	( GlobalState < stateSwitch && tmpPlayerNr != i ) ||
							( GlobalState >= stateSwitch && tmpPlayerNr != getStartPlayerNr(i) ) ) 
							&& getPortNr(i) != 0 ) { //was 3
							
							// CMD
							increaseChatLine( i );
							tmpUShort = getChatLine(i);
							TXPacket[3] = tmpUShort & 0xFF;
							TXPacket[4] = (tmpUShort >> 8) & 0xFF;
							// playerNumber van deze speler
							TXPacket[6] = tmpPlayerNr;
							//
							rdcksum( TXPacket,PacketLen );
							//ReplyToPlayer( PacketLen );
							BufferPacket( i, TXPacket, PacketLen, tmpUShort );
						}
					}
					//BuffersResend();
increaseChatLine(0);///changed 2008mei5


					// verwijder de speler
					changeState( playerNumber, 0 );
					changeQuit(playerNumber,1);
					GlobalPlayersQuit++;

					RemovePlayer( playerNumber, (GlobalState<stateSwitch)?1:0 );
//					RemovePlayer( playerNumber );   //  20080505   toegevoegd... naar aanleiding van verhaal C.. en ik blijf net in een server staan terwijl ik Quit deed...
					
				}
			}
		}
		
		
		
		//buffersResend achteraan in de main-loop, omdat: als er een speler uit wordt geknikkerd, playerNumber niet meer geldig is..
		if ( timerPassedSendBuffers(33)==1 ) { //30x per seconde
			BuffersResend();/// zo heb ik ook wel eens gedaan.. nu weet je iig dat ie om de 1/30e sec. stuurt   okun je iets met de ping erbij doen ok
		}
		//BuffersResend();
		CheckGlobalState();
		
		
		// Afbeelden van alle informatie / de huidige status
		if ( minisec() - lastDisplayTimeStamp > 1000 ) {
			
			lastDisplayTimeStamp = minisec();
			
			gotoxy(1,1);//cls();
			fputs( SCREEN, stdout );
			
			// Afbeelden van alle informatie / de huidige status
			DisplayProgress();
			DisplayStatus();
			DisplayList();
			//DisplayTiming();
			DisplayBuffers();
			gotoxy(1,2);
		}
		
		
		if ( PacketPrinted == 0 ) {
			//sprintf( log, "" );
			//			MsgprintyIP4(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<unknown packet [0][5][7]>", RXPacket[0], RXPacket[5], RXPacket[7]);
			//for ( i=0; i<len ; i++ ) {
			//	sprintf( log,"%s%x ", log, RXPacket[i] );
			//}
			//Log( log );
			sprintf( log, "" );
			if ( len > 16 ) {
				len = 16;
			} 
			for ( i=0; i<len ; i++ ) {
				sprintf( log,"%s %x", log, RXPacket[i] );
			}
			MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "", log );
			//			MsgprintyIP4(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<unknown packet [0][5][7]>", RXPacket[0], RXPacket[5], RXPacket[7]);
		}
		//		if ( RXPacket[0] != 0x40  &&  RXPacket[0] != 0x50  &&  RXPacket[0] != 0xFE ) {
		//			//fputs( "\n", stdout);  
		//		}
		
		
		
		peer.sin_addr.s_addr  = INADDR_ANY; // resolv();
		peer.sin_port         = htons(port);
		peer.sin_family       = AF_INET;
		
		//		if (bind(sd, (struct sockaddr *)&peer, sizeof(peer)) < 0) std_err();
		bind(sd, (struct sockaddr *)&peer, sizeof(peer));
		
		
	}
	
	OnCloseApplication();
	
    return 0;
	
}




// het "on close event"
void OnCloseApplication() {
	// melding afsluiten..
	
	//nocbreak();
	//endwin();
	
	Log2( "Server stopped", "debug" );
	//Log( "" );
	
	//Msgprinty("<Application closing..>");
	
	// De socket sluiten
    close( sd );

	close( RelaySocket );

//	unbind( 
	//if ( GlobalHTTPEnabled==1 ) {
		close( sdRA );
	//}
	//	close( gsSocket );
	
	// het scherm netjes achterlaten..
	closedisplay;
	
	// alle speler structures de-alloceren
	free( ptr_players );
	
	// de highscores array in database.h
	//	free( HighScores );
	
	// Het programma afsluiten..
    exit(0); //error-code die de app. terug-geeft
}







void NextGlobalState( int ThisState ) {
	// scherm opnieuw wissen en tekenen
	//cls();
	//fputs( SCREEN, stdout );
	
	// de volgende stap..
	GlobalState = ThisState;
	MsgprintyIP2( "", 0, "<Next Global State>", GlobalState );
	
}



void CheckGlobalState() {
	int		i;
	int		j;
	int		k;
	int		pos;
	int		AllReady;
	u_long	Time;
	u_char	timeString[256];
	long	tmpLong;
	long	HighPing;
	long	TimePast;
	u_long	NPos,EndPos;
	u_char tmpByte;
	u_char	Country;
	u_char	Stage;
	//	u_long	FastestTime;
	
	
	switch (GlobalState) {
		
	case stateLobby: //was 1
		// Alle spelers zijn in de lobby.
		// Er kunnen maar MAXPLAYERS spelers joinen.
		GlobalGameMode = (NumPlayers >= MaxPlayers)? 1: 0;	// closedplaying, openwaiting
		
		if ( GlobalConfigMode == 1 && NumPlayers > 3 ) {
			changeReady( 1,1 );
			changeReady( 2,1 );
		}
		
		//sprintf( ptr_players[0].Nick, "%s", CMR4HOSTNICK );
		
		// Als nu alle spelers ready zijn, gaat de GlobalState omhoog..
		if ( NumPlayers>1 && NumPlayers>=MINPLAYERS) {
			AllReady = 1;
			for ( i=1; i < NumPlayers; i++ ) {
				if ( getReady(i) != 1 ) {
					AllReady = 0; 
					break;
				}
			}
		} else {
			AllReady = 0;
		}
		
		// kick ass
		if ( GlobalConfigMode != 1 ) {
			for ( i=1; i < NumPlayers; i++ ) {
				if ( getReady(i) != 1  ) {
					if ( NumPlayers > MINPLAYERS && minisec() - ptr_players[PlayerIndex[i]].LastTimeReady > 40000 && ptr_players[PlayerIndex[i]].ReadyWarning == 0 )  {
						HostChat( i, "press READY or get KICKED..." );
						//HostChat( i, "press READY please..." );
						ptr_players[PlayerIndex[i]].ReadyWarning = 1;
					} 
					if ( NumPlayers < MINPLAYERS ) {
						ptr_players[PlayerIndex[i]].LastTimeReady = minisec();
						ptr_players[PlayerIndex[i]].ReadyWarning = 0; // 20080506
					}
					if ( minisec() - ptr_players[PlayerIndex[i]].LastTimeReady > 60000 && ptr_players[PlayerIndex[i]].ReadyWarning == 1 )  {
						//HostChat( i, "i pressed READY for you..." );
						KickPlayer( i , "kicked...");
						//changeReady( i, 1 );
						// Playerlists verzenden
						//SendAllPlayerList( -1 );
						
					}
					break;
				}
			}
		}
		
		
		// was iedereen klaar?
		if ( AllReady == 1 ) {
			// iedereen is ready in de lobby, dus op naar de volgende stap..
			startTimer = minisec();
			
			NextGlobalState( stateLobbyWait ); //was 2
			
			// the lobby must be open long enough for players to be able to join the server..
			if ( minisec() - GlobalLobbyEnter > (MINWAITTIME*1000) ) {
				if ( NumPlayers < MaxPlayers ) {
					RemainingWaitTime = 15 - NumPlayers;
				} else {
					RemainingWaitTime = 1;
				}
			} else {
				if ( NumPlayers == MaxPlayers ) {
					RemainingWaitTime = 1;
				} else {
					RemainingWaitTime = MINWAITTIME - ( (minisec()- GlobalLobbyEnter)/1000 ) + (15 - NumPlayers);
				}
			}
			if ( GlobalConfigMode == 1 ) RemainingWaitTime = 1;
			
			
			if ( RemainingWaitTime == 1 ) {
				sprintf( log, "starting" );
			} else {
				sprintf( log, "starting in %d seconds", RemainingWaitTime );
			}
			HostChat( -1, log );
			
			// closedplaying forceren, er kan/mag nu niemand meer bij komen..ook al zou de server niet vol zitten.
			//GlobalGameMode = 1;
			
			//if ( timer > 10 seconden ) {
			// de volgende stap..
			//}
		}
		break;
		
	case stateLobbyWait://was 2
		// Alle spelers zijn in de lobby.
		// Er kunnen maar MAXPLAYERS spelers joinen.
		GlobalGameMode = (NumPlayers >= MaxPlayers)? 1: 0;	// closedplaying, openwaiting
		
		// Als nu alle spelers ready zijn, gaat de GlobalState omhoog..
		if ( NumPlayers>1 && NumPlayers>=MINPLAYERS) {
			AllReady = 1;
			for ( i=1; i < NumPlayers; i++ ) {
				if ( getReady(i) != 1 ) {
					AllReady = 0; 
					break;
				}
			}
		} else {
			AllReady = 0;
		}
		
		
		
		// was iedereen klaar?
		if ( AllReady == 1 ) {
			// iedereen is ready in de lobby, dus op naar de volgende stap..
			
			//if ( timer > 10 seconden ) {
			// de volgende stap..
			//NextGlobalState( stateLobbyLeave );
			//if ( minisec() % 10 == 0 ) {
			/*			if ( minisec() - startTimer > 1000 && minisec() - startTimer < 1100 ) { 
			sprintf( log, "timer: %d %d", startTimer, minisec() );
			HostChat( -1, log );
			}
			*/
			if ( minisec() - startTimer > (RemainingWaitTime*1000) ) { 
				
				
				// de volgende stap..
				NextGlobalState( stateLobbyLeave ); //was 3
				//				NextGlobalState( stateLobbyCheckStage ); //was 3
				
				//HostChat( -1, "Send players to the grid" );
				//HostChat( -1, StrReplaceGogogoD6 );  // what they see when returning from lobby after stage
				//				HostChat( -1, "press esc ready please" );
				
			}
			
			//}
		} else {
			NextGlobalState( stateLobby ); //was 1
			//			HostChat( -1, "starting aborted" );
		}
		break;
/*		
	case stateLobbyCheckStage:
		
		// was iedereen klaar?

		if ( GlobalStageCycleEnabled == 1 ) {
			tmpByte = GlobalStageCycle[GlobalStageCycleCounter];

			Country = ((tmpByte - 100)-(tmpByte % 10)) / 10;
			Stage = tmpByte % 10;
			
			sprintf( log, "%s", StrStages[Country][Stage] );
			MsgprintyIP1("", 0, "<stage-cycle>", log);
			sprintf( log, "stage-cycle: %s", StrStages[Country][Stage] );
			ServerChat( -1, "CYCLE", log );

			ServerChangeStage( tmpByte ); // willekleurig een andere kiezen
			//startTimer = minisec();
			GlobalStagesPrev = 0;
			NextGlobalState( stateLobbyWait ); //was 3

		} else {

			if ( GlobalStages == GlobalStagesPrev ) {  //  dezelfde baan als vorige keer
				//tmpByte = 0;
				tmpByte = GlobalStages;
				while ( IsStage(tmpByte) == 0 || tmpByte == GlobalStages ) {
					
					tmpByte = random(); //(random()*1000 / RAND_MAX*1000 )*1000;///+ (random()*100 / RAND_MAX*100 )*100 + (random()*100 / RAND_MAX*100 )*10;
					
				}
				
				Country = ((tmpByte - 100)-(tmpByte % 10)) / 10;
				Stage = tmpByte % 10;
				
				
				sprintf( log, "%s", StrStages[Country][Stage] );
				MsgprintyIP1("", 0, "<random stage>", log);
				sprintf( log, "random stage: %s", StrStages[Country][Stage] );
				ServerChat( -1, "RANDOM", log );
				
				ServerChangeStage( tmpByte ); // willekleurig een andere kiezen
				//startTimer = minisec();
				GlobalStagesPrev = 0;
				NextGlobalState( stateLobbyWait ); //was 3
				//}
				//break;
			}
		}
		NextGlobalState( stateLobbyCheckedStage ); //was 3
		break;
		
	case stateLobbyCheckedStage:
		
		sleep(0);
		// wachten op alle ACKs van de spelers..
		if ( BuffersEmpty() == -1 ) {
			
			NextGlobalState( stateLobbyLeave ); //was 3
			
		}
		break;
*/		
	case stateLobbyLeave: //was 3
		// Alle spelers zijn in de lobby.
		// Er kunnen maar MAXPLAYERS spelers joinen.
		GlobalGameMode = (NumPlayers >= MaxPlayers)? 1: 0;	// closedplaying, openwaiting
		
		// Als nu alle spelers ready zijn, gaat de GlobalState omhoog..
		if ( NumPlayers>1 && NumPlayers>=MINPLAYERS) {
			AllReady = 1;
			for ( i=1; i < NumPlayers; i++ ) {
				if ( getReady(i) != 1 ) {
					AllReady = 0; 
					break;
				}
			}
		} else {
			AllReady = 0;
		}
		// was iedereen klaar?
		if ( AllReady == 1 ) {
			// iedereen is ready in de lobby, dus op naar de volgende stap..
			
			// closedplaying forceren, er kan/mag nu niemand meer bij komen..ook al zou de server niet vol zitten.
			GlobalGameMode = 1;
			
			
			// de host op ready zetten
			changeReady(0, 1);
			
			// Playerlists verzenden
			strcpy( FastestNick, sqlGetFastestNick( GlobalStages ) ) ;
			FastestTime = sqlGetFastestTime( GlobalStages );
			MStoTimeString(FastestTime , &ptr_players[0].Nick );
			SendAllPlayerList( -1 );
			
			// de volgende stap..
			NextGlobalState( stateNumbering ); //was 4
			
			//na verlaten lobby, markeren "nog geen C6-0E ontvangen van de speler"
			for (i=0;i<MaxPlayers;i++) {
				ptr_players[i].PosCount = 0;
				ptr_players[i].ReceivedC60E = 0;
			}
			// globale count
			PosSentCount = 0;
			
			// naar de startlijn gaan..
			Answer_LeaveLobby();
			
			sleep(0); // even wachten...
			
			// iedereen een ping-lijst sturen..
			Server_PingList( -1, 1 ); // iedereen, list met timestamp
			
		} else {
			// de host op not-ready zetten
			changeReady(0, 2);
		}
		break;
		
	case stateNumbering: //was 4
		
		
		// wachten op alle ACKs van de spelers..
		if ( BuffersEmpty() == -1 ) {
			
			// iedereen een ping-lijst sturen..
			Server_PingList( -1, 1 ); // iedereen, list met timestamp
			
			// de State van iedereen op 2.. 
			for ( i=0 ; i < NumPlayers ; i++ ) {
				if ( getState(i) < 2 ) { 
					changeState(i,2);
				}
			}
			
			// Vanaf nu komen er C6-0E's binnen van de spelers..
			// Elke op de voet gevolgd door een C6-0D.
			// De C6-0E's 
			
			// 40 01 naar iedereen..
			PacketLen = sizeof(ServerPing);
			memcpy( TXPacket, ServerPing, PacketLen);
			// host timestamp erin plakken
			Time = minisec();
			TXPacket[4] = Time & 0xFF;
			TXPacket[5] = (Time>>8) & 0xFF;
			TXPacket[6] = (Time>>16) & 0xFF;
			TXPacket[7] = (Time>>24) & 0xFF;
			//
			rdcksum( TXPacket, PacketLen );
			for ( i=1; i < NumPlayers; i++ ) {
				Send1ToPlayer( i, PacketLen );
			}
			
			// PingList naar iedereen..zonder timestamp erin.
			Server_PingList( -1, 0 );
			
			
			// C6-10 nummers 0..7 sturen, startnummers..
			for ( i=0 ; i < NumPlayers ; i++ ) Server_NumberThing(i);

			//			for ( i=0 ; i < NumPlayers ; i++ ) Server_NumberThing(i);	//2x zenden??
			// en wachten op ACK's
			/*
			// Stuur een grid ready van de host
			// de host is nu ook present..
			//Server_GridPresent1();
			// de anderen een C6-0E terugsturen..
			for ( i=1; i < MaxPlayers; i++ ) {
			if ( PlayerIndex[i] != -1) {
			//			for ( i=1; i < NumPlayers; i++ ) {
			
			  PacketLen = sizeof(ServerGridPresent1);
			  memcpy( TXPacket, ServerGridPresent1, PacketLen);
			  
				// ChatLine
				increaseChatLine( i );
				tmpUShort = getChatLine(i);
				TXPacket[3] = tmpUShort & 0xFF;
				TXPacket[4] = (tmpUShort >> 8) & 0xFF;
				// client-info uit ontvangen packet overnemen..
				// PlayerNr
				TXPacket[6] = 0;
				// ??
				memcpy( TXPacket+21, RXPacket+21, 16);
				// checksum
				rdcksum( TXPacket,PacketLen );
				BufferPacket( i, TXPacket, PacketLen, tmpUShort );
				
				  // meteen ook een ServerGridPresent2 sturen..
				  // Een C6-0D zenden naar iedereen..
				  PacketLen = sizeof(ServerGridPresent2);
				  memcpy( TXPacket, ServerGridPresent2, PacketLen);
				  // ChatLine
				  increaseChatLine( i );
				  tmpUShort = getChatLine(i);
				  TXPacket[3] = tmpUShort & 0xFF;
				  TXPacket[4] = (tmpUShort >> 8) & 0xFF;
				  // PlayerNr van de host..
				  TXPacket[6] = 0;
				  // checksum
				  rdcksum( TXPacket,PacketLen );
				  BufferPacket( i, TXPacket, PacketLen, tmpUShort );
				  }
				  }
			*/
			// de volgende stap..
			NextGlobalState( stateJoinNr ); //was 5
		}
		break;
		
	case stateJoinNr: //was 5
		// Alle spelers zijn ready in de lobby, en we zijn op weg naar de startlijn in-game
		// Nu iedereen z'n JoinNr toesturen..en wachten op ACK's
		
		// wachten op alle ACKs van de spelers..
		if ( BuffersEmpty() == -1 ) {
			
			// de State van iedereen op 3.. 
			for ( i=0 ; i < NumPlayers ; i++ ) {
				if ( getState(i) < 3 ) { 
					changeState(i,3);
				}
			}
			
			// load stage-record positions
			//oud#		NPos = sqlLoadStageRecord_OLD( GlobalStages );
			
			sqlLoadStageRecord( GlobalStages );
			/*
			if ( GlobalNoCutsEnabled == 1 ) {
			float PosX;
			float PosY;
			float PosZ;
			for ( j=8; j<12; j++)  {
			for ( pos=0; pos<ptr_players[j].PosCount; pos++ ) {
			
			  memcpy(&PosX, &ptr_players[j].PosRec[pos*27+12], 4);//4 bytes vanaf offset 12 in LastPos
			  memcpy(&PosY, &ptr_players[j].PosRec[pos*27+16], 4);
			  memcpy(&PosZ, &ptr_players[j].PosRec[pos*27+20], 4);
			  float LPosX = getLastPosX( j );
			  float LPosY = getLastPosY( j );
			  float LPosZ = getLastPosZ( j );
			  
				//if ( ptr_players[PlayerIndex[playerNumber]].RaceTime>5000 ) {//changed 2008apr28
				for (k=0; k<GlobalCutsCount; k++ ) {
				float NoCutX1 = GlobalCuts[k].X1;
				float NoCutY1 = GlobalCuts[k].Y1;
				float NoCutZ1 = GlobalCuts[k].Z1;
				float NoCutX2 = GlobalCuts[k].X2;
				float NoCutY2 = GlobalCuts[k].Y2;
				float NoCutZ2 = GlobalCuts[k].Z2;
				
				  
					if ( LineSegmentIntersection(PosX,PosZ, LPosX,LPosZ, NoCutX1,NoCutZ1, NoCutX2,NoCutZ2) ) {
					//sprintf( log, "<cut>: XYZ(%f, %f, %f) <-> XYZ(%f, %f, %f)", NoCutX1, NoCutY1, NoCutZ1, NoCutX2, NoCutY2, NoCutZ2 );
					sprintf( log, "%s", getNick(j) );
					sprintf( log, "<cut-record>: Pos XYZ(%f, %f, %f) ", PosX,PosY,PosZ );
					MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<cut-record>", log);
					Log( log );
					//// de speler is aan het cutten..
					//Server_RetireDriver( playerNumber );
					}
					}
					//}
					
					  changeLastPosX( j, PosX );
					  changeLastPosY( j, PosY );
					  changeLastPosZ( j, PosZ );
					  }
					  }
					  }
			*/
			
			// de highscore race record positions laden
			/*
			HighScores[0].PosCount = sqlLoadPositions( HighScores[0].TimeID, &HighScores[0].PosRec );
			HighScores[1].PosCount = sqlLoadPositions( HighScores[1].TimeID, &HighScores[1].PosRec );
			HighScores[2].PosCount = sqlLoadPositions( HighScores[2].TimeID, &HighScores[2].PosRec );
			HighScores[3].PosCount = sqlLoadPositions( HighScores[3].TimeID, &HighScores[3].PosRec );
			*/
			
			
			// CUTs laden voor deze baan
			if ( GlobalNoCutsEnabled == 1 )	sqlLoadCuts( GlobalStages );
			/*oud#
			// zoek het laatste pakketje, en haal de laatste Pos eruit..
			if (NPos<=0) {
			GlobalEndPos = 0; // er is nog geen record opgeslagen met een PosRec
			} else {
			GlobalEndPos = (ptr_players[0].PosRec[NPos*27-2] << 8) + ptr_players[0].PosRec[NPos*27-3];
			}
			*/

			// indien in NOCUTS-mode..
//			if ( GlobalConfigMode == 1 ) {
//				//Server_GridPresent1Player(1);
//				//Server_GridPresent1Player(2);
//			}

			//TEST
			// de host is grid-present..//TEST
			//Server_GridPresent1();
			///TEST
			// Speler Join-nummers toesturen
			//!			Server_JoinNr( LastPlayerNr );
			Server_JoinNr( -1 );
			// Nu ook iedereen een playerlist sturen.
			SendAllPlayerList( -1 ); // naar iedereen..
			//			// PingList naar iedereen..zonder timestamp erin.
			Server_PingList( -1, 1 );
			
			// de volgende stap..
			NextGlobalState( stateSwitch ); //was 6
		}
		break;
		
	case stateSwitch: //was 6
		sleep(0);
		// wachten op alle (JoinNr-)ACKs van de spelers..
		if ( BuffersEmpty() == -1 ) {
			// Alle spelers hebben een JoinNr ontvangen..
			
			// de State van iedereen op 4.. 
			for ( i=0 ; i < NumPlayers ; i++ ) {
				if ( getState(i) < 4 ) { 
					changeState(i, 4);
				}
			}
			
			if ( GlobalConfigMode == 1 ) {
				Server_GridReadyPlayer(1);
				changeState( 1, 6 );
				Server_GridReadyPlayer(2);
				changeState( 2, 6 );
			}
			
			// Wacht daarna op alle ACK's..
			// Dan GAAT een speler naar de startlijn in game.
			startTimer = minisec();
			RaceStartTimeStamp = minisec();  // bijhouden wanneer begonnen voor om 1 seconde een nrthing
			
			// de volgende stap..
			NextGlobalState( stateGridReady ); //was 7
		}
		break;
		
	case stateGridReady: //was 7
		sleep(0);
		// wachten op alle ACKs van de spelers..
		if ( BuffersEmpty() == -1 ) {
/*			
			if ( minisec() - RaceStartTimeStamp > 1000 ) {
				for ( i=0 ; i < NumPlayers ; i++ ) Server_NumberThing(i);
				SendAllPlayerList( -1 ); // naar iedereen..
				RaceStartTimeStamp = minisec(); 
			}
*/			
			// Alle ACK's zijn binnen...
			// Nu STAAT iedereen aan de startlijn in game.
			// Nu wachten op alle C6....03's van iedereen, ten teken ready aan de startlijn.
			// Een speler-C6-03 is binnen en de player.State = 6.
			// Een speler stuurt maar 1x een C6-03...hij kan niet meer terug op not ready.
			AllReady = 1;
			for ( i=1; i < NumPlayers; i++ ) {
				if ( getState(i) != -1 && getState(i) != 6 ) { ///&& getState(i) != 4 ) {  
					//				if ( getState(i) != 6 ) {
					AllReady = 0;
					break;
				}
			}
			// allemaal ready??
			if ( AllReady == 1 ||  ( minisec() - startTimer > 15000 ) ) {
				
				// de State van de host aanpassen op gridReady..
				changeState( 0, 6 );
				LastPlayerNr = 0;
				
				// iedere speler .Finished instellen op "nog niet gefinished"..voordat de race begint.
				for ( i=0; i < NumPlayers; i++ ) {
					//changeState( i, 6 );
					changeRaceTime( i, 0);						// racetijd op 0
					changeSplitsDone( i, 0 );					// aantal splits gehad teller..
					changeFinished( i, 0);						// nog niet over de finish..
					changeRetired( i, 0);						// nog niet retired..
					changeQuit( i, 0);							// nog niet ge-quit..
					ptr_players[PlayerIndex[i]].PosReceived = 0;// "de host heeft nog geen posities ontvangen van deze speler.."
					//test
					for (j=0; j<MaxPlayers; j++) ptr_players[PlayerIndex[i]].LastPosSentTime[j] = 0x00000000;
					///test
				}
				
				// in-race tellers op 0
				GlobalPlayersFinished = 0;			// het aantal spelers finished tot dusver..
				GlobalPlayersRetired = 0;			// het aantal spelers retired tot dusver..
				GlobalPlayersQuit = 0;				// het aantal spelers quit tot dusver..
				
				// De host op ready zetten.
				// Een C6-03 terugsturen naar de rest..
				// met de ID van de host erin..
				Server_GridReady();
				//while ( BuffersEmpty() != -1 ) { 
				//	BuffersResend();
				//	sleep(0); 
				//}
				
				// Playerlists verzenden (geen D6)
				SendAllPlayerList( -1 );
				
				// de volgende stap..
				NextGlobalState( stateGridReadyACK ); //was 8
				
			} else {
				
				// de host-State op not-gridReady zetten
				changeState(0, 5);    ///  5
				
			}
		}
		break;
		
	case stateGridReadyACK: //was 8
		
		sleep(0);
		if ( BuffersEmpty() == -1 ) {
			
			RaceStartTimeStamp = minisec();  // bijhouden wanneer begonnen voor stoppen na 8 minuten
			
			//ptr_players[0].PosReceived = 0;
			NextGlobalState( stateCountdown );
			
		}
		break;
		
	case stateCountdown: //was 8
		// wachten op alle ACKs van de spelers..
		//if ( BuffersEmpty() == -1 ) {
		// Start de rally, het aftellen voorbereiden..
		
		
		// iedereen een ping-lijst sturen..
		//Server_PingList( -1, 0 ); // iedereen, list met timestamp
		
		// Een C6....04 versturen naar alle spelers..het aftellen begint
		//Server_StartCountdown();
		
		// NEW aftellen naar aanleiding van de AvgPing van client
		tmpULong = 0;
		HighPing = 0;
		for ( i=1 ; i<NumPlayers; i++ ) {
			if ( getAvgPing(i) > HighPing ) HighPing = getAvgPing(i);  ///
			//if ( getPing(i) > tmpULong ) tmpULong = getPing(i);
		}
		
		//HighPing = HighPing * 2;
		if ( HighPing > 2000 ) HighPing = 2000;
		if ( HighPing == 0 ) HighPing = 300;
		
		sprintf( log, "highest: %d ms", HighPing );
		MsgprintyIP1("", 0, "<ping>", log);
		AllReady = 0;
		while ( AllReady == 0 ) {
			sleep(0);
			
			
			for ( i=1 ; i<NumPlayers; i++ ) {
				
				if ( getState(i) != 7 ) {
					
					TimePast = minisec() - RaceStartTimeStamp; 
					tmpULong = HighPing - TimePast ;
					
					//sprintf( log, "p:%d avg:%d past:%d trigger:%d", i, getAvgPing(i), TimePast, tmpULong );
					//MsgprintyIP1("", 0, "<check>", log);
					
					if ( getAvgPing(i) >= tmpULong || tmpULong > 10000 || tmpULong < 0 ) {  //
						sprintf( log, "%d: sent %d ms player: %s", i, tmpULong, getNick(i) );
						MsgprintyIP1("", 0, "<start>", log);
						Server_StartCountdown(i);
						if ( getState(i) < 7 ) { 
							changeState(i, 7);
						}
					}
				}
			}
			
			AllReady = 1;
			for ( i=1; i < NumPlayers; i++ ) {
				if ( getState(i) != 7 ) {
					AllReady = 0;
					break;
				}
			}
			
		}
		
		if ( AllReady == 1 ) {
			
			// de stopwatch begintijd (timeval) opnemen..
			// De in-game spreker telt nu eerst 5 seconden af..
			GlobalStartTimeVal = minisec();
			// tbv. de 60sec-timeout..
			GlobalFinishTimeVal = 0; //als nog niet ingesteld..
			
			RaceStartTimeStamp = minisec();  // bijhouden wanneer begonnen voor stoppen na 8 minuten
			LastTimeValue2 = minisec();
			LastTimeValue3 = minisec()-2500;  // zijn nu in de war...
			
			//Server_Retire(); //////////////////////////////////////////////////////////////
			// de volgende stap..
			NextGlobalState( stateRacing );
		}
		//}
		break;
		
	case stateRacing: //was 9:
		
		sleep(0);
		
		// 8 minuten voorbij ?  
		if ( ( minisec()- RaceStartTimeStamp) > (1000 * 60 * 8) && GlobalConfigMode != 1) {  
			Server_RetireDrivers();
			ServerChat( -1, "Hurry", "it's taking too long..." );
		}
		
		// wachten op alle ACKs van de spelers..
//		if ( BuffersEmpty() == -1 ) {
//			// in de race..
//			// GlobalPlayersFinished, GlobalPlayersRetired & GlobalPlayersQuit zijn nu relevant
//		}
		break;
		
	case stateInit: //was 10:
		// wachten op alle ACKs van de spelers..
		if ( BuffersEmpty() == -1 ) {
			//GlobalStagesPrev = GlobalStages;
			if ( GlobalConfigMode != 1 ) {
				
				//while ( IsStage(tmpByte) == 0 || tmpByte == GlobalStages ) {
				/*
				while ( IsStage(tmpByte) == 0 || tmpByte == GlobalStages || 
				!(	tmpByte == 100 || tmpByte == 101 || tmpByte == 102 || tmpByte == 103 || tmpByte == 104 || tmpByte == 105 || 
				tmpByte == 110 || tmpByte == 111 || tmpByte == 112 || tmpByte == 113 || tmpByte == 114 || tmpByte == 115 || tmpByte == 116 ||
				tmpByte == 120 || tmpByte == 121 || tmpByte == 122 || tmpByte == 123 || tmpByte == 124 || tmpByte == 125 || 
				tmpByte == 130 || tmpByte == 131 || tmpByte == 132 || tmpByte == 133 || tmpByte == 134 || tmpByte == 135 || 
				tmpByte == 140 || tmpByte == 141 || tmpByte == 142 || tmpByte == 143 || tmpByte == 144 || tmpByte == 145 ||
				tmpByte == 160 || tmpByte == 161 || tmpByte == 162 || tmpByte == 163 || tmpByte == 164 || tmpByte == 165 || tmpByte == 166 || 
				tmpByte == 170 || tmpByte == 171 || tmpByte == 172 || tmpByte == 173 || tmpByte == 174 || tmpByte == 175  
				) ) {
				*/	

				if ( GlobalStageCycleEnabled == 1 ) {
					tmpByte = GlobalStageCycle[GlobalStageCycleCounter];

					Country = ((tmpByte - 100)-(tmpByte % 10)) / 10;
					Stage = tmpByte % 10;
					
					sprintf( log, "%s", StrStages[Country][Stage] );
					MsgprintyIP1("", 0, "<stage-cycle>", log);
					sprintf( log, "next stage: %s", StrStages[Country][Stage] );
					ServerChat( -1, "CYCLE", log );
				} else {
					tmpByte = sqlNextStage();

					Country = ((tmpByte - 100)-(tmpByte % 10)) / 10;
					Stage = tmpByte % 10;
					
					sprintf( log, "%s", StrStages[Country][Stage] );
					MsgprintyIP1("", 0, "<next stage>", log);
					sprintf( log, "next stage: %s", StrStages[Country][Stage] );
					ServerChat( -1, CMR4HOSTNICK, log );
				}
				//tmpByte = random(); //(random()*1000 / RAND_MAX*1000 )*1000;///+ (random()*100 / RAND_MAX*100 )*100 + (random()*100 / RAND_MAX*100 )*10;
				
				//}
				
				
				ServerChangeStage( tmpByte ); // willekleurig een andere kiezen
			}
			//startTimer = minisec();
			GlobalStagesPrev = 0;
			
			GlobalCurrentCut=0;
			LastTimeValue2 = minisec();
			LastTimeValue3 = minisec()-2500;  // zijn nu in de war...
			changeNick( 0, CMR4HOSTNICK );
			//sprintf( ptr_players[0].Nick, "%s", CMR4HOSTNICK );
			SendAllPlayerList(-1);
			// in de race, en al over de finish
			// nog niet in de lobby..
			// GlobalPlayersFinished, GlobalPlayersRetired & GlobalPlayersQuit zijn nu relevant
			
			// Tijd onthouden van naar lobby gaan...
			GlobalLobbyEnter = minisec();
			GlobalShowTiming = 1;

			if ( GlobalStageCycleEnabled==1 ) {
				GlobalStageCycleLastCounter = GlobalStageCycleCounter;
				GlobalStageCycleCounter++;
				//if ( GlobalStageCycleCounter>=GlobalStageCycleCount ) GlobalStageCycleCount = 0;
				if ( GlobalStageCycleCounter>=GlobalStageCycleCount ) GlobalStageCycleCounter = 0; //  20080513
			}

			// spelers.Ready weer op 0
			// spelers.State weer op 1
			for ( i=0; i< MAXPLAYERS; i++ ) {
				//!for ( i=0; i<NumPlayers; i++ ) {
				//!changeReady( i, 0 );
				//!changeState( i, 1 );
				ptr_players[i].Ready = 0;		// niet klaar..
				ptr_players[i].State = 1;		
				ptr_players[i].VoteStage = 0;	// blanco stem
				ptr_players[i].VoteKick = 0x00000000;			// geen vote tegen player ID
				ptr_players[i].RaceTime = 0x00000000;	// tijd in ms.
				for (j=0; j<4; j++) ptr_players[i].SplitTime[j] = 0x00000000;
				ptr_players[i].SplitsDone = 0;
				ptr_players[i].StartTime = 0x00000000;	// tijd in ms.
				ptr_players[i].hasFinished = 0;
				ptr_players[i].hasRetired = 0;
				ptr_players[i].hasQuit = 0;
				ptr_players[i].PercToSplit0 = 0x00;
				ptr_players[i].Percentage = 0x00;
				ptr_players[i].PosCount = 0;
				ptr_players[i].PosReceived = 0;
				ptr_players[i].LastTimeReady = minisec();
				ptr_players[i].LastTimeValue = 0x00000000;
				ptr_players[i].ReadyWarning = 0x00;
				//for (j=0; j<27; j++) ptr_players[i].LastPos[j] = 0;
				memset( &ptr_players[i].LastPos[0], 0x00, 27 );
				//for (j=0;j<BLOB_LEN;j++) ptr_players[i].PosRec[j] = 0; //memset
				memset( &ptr_players[i].PosRec[0], 0x00, BLOB_LEN );
//				for (j=8; j<12; j++) ptr_players[j].LastPosRecSent[i] = 0;
//				for (j=0; j<19; j++) ptr_players[j].LastPosRecSent[i] = 0;  // spelers[8..11] = highscores, spelers[12..18] = PR
				for (j=0; j<8+4+MAXPLAYERS; j++) ptr_players[j].LastPosRecSent[i] = 0;  // spelers[8..11] = highscores, spelers[12..18] = PR
				for (j=0; j<MAXPLAYERS; j++) ptr_players[i].LastPosSentTime[j] = 0x00000000;
				for (j=0;j<4;j++) ptr_players[i].PosSplitSent[j] = 0; //1 == intermediate[j] verzonden naar speler
			}
			NextGlobalState( stateLobby ); //was 1
/* changed: 2008mei11			
			// Playerlists verzenden
			SendAllPlayerList( -1 );
*/
		}
		break;
	}
	
	// scherm opnieuw tekenen..
	// scherm opnieuw wissen en tekenen
	//gotoxy(1,1);//cls();
	//fputs( SCREEN, stdout );
	//DisplayProgress();
	//DisplayStatus();
	//DisplayList();
	//DisplayTiming();
}






int TrafficOut( int PacketLen ) {
	
	if ( PacketLen > 0 ) {
		TotalBytesOut += PacketLen;
		//BytesOutSec += PacketLen;
		//BytesOutMin += PacketLen;
		//BytesOutHour += PacketLen;
	}
	return PacketLen;
}

int TrafficIn( int PacketLen ) {
	
	if ( PacketLen > 0 ) {
		TotalBytesIn += PacketLen;
		//BytesInSec += PacketLen;
		//BytesInMin += PacketLen;
		//BytesInHour += PacketLen;
	}
	return PacketLen;
}



void SendToPlayer( short playerNumber, int PacketLen ) {
	// zit de speler er nog in ??..
	if ( getPortNr(playerNumber)==0) return;
	peerOut.sin_addr.s_addr = ptr_players[PlayerIndex[playerNumber]].AddrIn.s_addr;
	//	peerOut.sin_addr.s_addr = htonl(ptr_players[PlayerIndex[playerNumber]].IP);
	peerOut.sin_port        = htons(ptr_players[PlayerIndex[playerNumber]].PortNr);
	peerOut.sin_family      = AF_INET;
	if( TrafficOut( sendto(sd, TXPacket, PacketLen, 0, (struct sockaddr *)&peerOut, (socklen_t *) sizeof(peerOut)) ) < 0) std_err();
	if( TrafficOut( sendto(sd, TXPacket, PacketLen, 0, (struct sockaddr *)&peerOut, (socklen_t *) sizeof(peerOut)) ) < 0) std_err();
	
	peerOut.sin_addr.s_addr  = INADDR_ANY;
	peerOut.sin_port         = htons(0);
	peerOut.sin_family       = AF_INET;
}

void ReplyToPlayer( int PacketLen ) {
	if( TrafficOut( sendto(sd, TXPacket, PacketLen, 0, (struct sockaddr *)&peer2, (socklen_t *) sizeof(peer2)) ) < 0) std_err();
	if( TrafficOut( sendto(sd, TXPacket, PacketLen, 0, (struct sockaddr *)&peer2, (socklen_t *) sizeof(peer2)) ) < 0) std_err();
	//	SEND( TXPacket, PacketLen );
}



void Send1ToPlayer( short playerNumber, int PacketLen ) {
	// zit de speler er nog in ??..
	if ( getPortNr(playerNumber)==0) return;
	peerOut.sin_addr.s_addr = ptr_players[PlayerIndex[playerNumber]].AddrIn.s_addr;
	//	peerOut.sin_addr.s_addr = htonl(ptr_players[PlayerIndex[playerNumber]].IP);
	peerOut.sin_port        = htons(ptr_players[PlayerIndex[playerNumber]].PortNr);
	peerOut.sin_family      = AF_INET;
	if( TrafficOut( sendto(sd, TXPacket, PacketLen, 0, (struct sockaddr *)&peerOut, (socklen_t *) sizeof(peerOut)) ) < 0) std_err();
	
	peerOut.sin_addr.s_addr  = INADDR_ANY;
	peerOut.sin_port         = htons(0);
	peerOut.sin_family       = AF_INET;
}

void Reply1ToPlayer( int PacketLen ) {
	if( TrafficOut( sendto(sd, TXPacket, PacketLen, 0, (struct sockaddr *)&peer2, (socklen_t *) sizeof(peer2)) ) < 0) std_err();
	//	SEND( TXPacket, PacketLen );
}






void BufferPacket( short PlayerNr, u_char *Packet, int PacketLen, u_short CMD ) {
	int		i;
	int		j;
	if ( PlayerNr<=0 ) return;
	if ( getPortNr(PlayerNr)==0) return;
	//	if ( getQuit(PlayerNr)!=0 ) return;// zit de speler er nog in ??..
	
	/*
	if ( ptr_players[PlayerIndex[PlayerNr]].BufferedPackets >= MAXBUFF ) {
	
	  MsgprintyIP2( "buffer", PlayerIndex[PlayerNr], "<add FAILED: buffers full> PlayerNr:", PlayerNr);
	  MsgprintyIP2( "buffer", PlayerIndex[PlayerNr], "<add FAILED: buffers full> CMD:", CMD);
	  
		//ptr_players[PlayerIndex[PlayerNr]].BufferedPackets = 0;
		
		  //==== C6 88 ?? server kickt
		  // de andere spelers een C6-08 sturen..
		  PacketLen = sizeof(ServerQuit);
		  memcpy( TXPacket, ServerQuit, PacketLen);
		  
			// playerNumber van deze speler
			if ( GlobalState <= stateLobbyLeave  ) {								
			TXPacket[6] = PlayerNr; //PlayerNr ;//getID(PlayerNr);  ///ptr_players[ PlayerIndex[PlayerNr] ].ID ;
			} else {
			TXPacket[6] = getStartPlayerNr(PlayerNr); //PlayerNr ;//getID(PlayerNr);  ///ptr_players[ PlayerIndex[PlayerNr] ].ID ;
			}
			// de speler verwijderen, en alle variabelen bijwerken..
			//RemovePlayer( PlayerNr );
			
			  for ( j=1; j < NumPlayers; j++ ) {
			  //.			if ( j != TXPacket[6]  && PlayerIndex[j] != -1) {
			  //			if ( TXPacket[6] != j ) {
			  if ( PlayerNr != j ) {
			  // CMD
			  
				increaseChatLine( j );
				tmpUShort = getChatLine(j);
				//tmpUShort = getLastCMD(j);
				//changeLastCMD( j, tmpUShort+1 ); //nieuwe waarde onthouden..
				TXPacket[3] = tmpUShort & 0xFF;
				TXPacket[4] = (tmpUShort >> 8) & 0xFF;
				//
				rdcksum( TXPacket,PacketLen );
				//BufferPacket( j, TXPacket, PacketLen, tmpUShort );
				SendToPlayer( j, PacketLen );//, tmpUShort );
				}
				}
				
				  DisplayBuffers();
				  //BuffersResend();
				  changeState( PlayerNr, 0 );
				  RemovePlayer( PlayerNr );
				  
					return;
					}
	*/
	// test of de volgende write nog past.. anders is de buffer al vol.
	//	if ( (PtrBufferWrite+1==PtrBufferRead) || ((PtrBufferWrite+1==MAXBUFF)&&(PtrBufferRead==0)) ) {
	//	}
/**	
	ptr_players[PlayerIndex[PlayerNr]].PtrBufferWrite++;
	if ( ptr_players[PlayerIndex[PlayerNr]].PtrBufferWrite >= MAXBUFF ) {
		ptr_players[PlayerIndex[PlayerNr]].PtrBufferWrite = 0;
	}
	// een lege buffer zoeken..
	//	for (i=0;i<MAXBUFF;i++) {
	i = ptr_players[PlayerIndex[PlayerNr]].PtrBufferWrite;
	// de buffer-lengte is -1  als teken "buffer niet in gebruik"
	//if ( ptr_players[PlayerIndex[PlayerNr]].BufferLen[i] <= 0 ) { //lege entry gevonden
	//markeren als "in gebruik"
	ptr_players[PlayerIndex[PlayerNr]].BufferLen[i] = PacketLen;
	// packet overnemen naar buffer[i]..
	memcpy( ptr_players[PlayerIndex[PlayerNr]].Packet[i] , Packet, PacketLen);
	// commando overnemen, zodat het packet later kan worden opgezocht..
	ptr_players[PlayerIndex[PlayerNr]].BufferCMD[i] = CMD;
	// teller voor het aantal keren nog proberen..
	ptr_players[PlayerIndex[PlayerNr]].BufferCountdown[i] = MAXRESEND;
**/
	// aantal buffers in gebruik ophogen..
	if ( ptr_players[PlayerIndex[PlayerNr]].BufferedPackets < MAXBUFF ) {
		ptr_players[PlayerIndex[PlayerNr]].BufferedPackets++;

		ptr_players[PlayerIndex[PlayerNr]].PtrBufferWrite++;
		if ( ptr_players[PlayerIndex[PlayerNr]].PtrBufferWrite >= MAXBUFF ) {
			ptr_players[PlayerIndex[PlayerNr]].PtrBufferWrite = 0;
		}
		// een lege buffer zoeken..
		//	for (i=0;i<MAXBUFF;i++) {
		i = ptr_players[PlayerIndex[PlayerNr]].PtrBufferWrite;
		// de buffer-lengte is -1  als teken "buffer niet in gebruik"
		//if ( ptr_players[PlayerIndex[PlayerNr]].BufferLen[i] <= 0 ) { //lege entry gevonden
		//markeren als "in gebruik"
		ptr_players[PlayerIndex[PlayerNr]].BufferLen[i] = PacketLen;
		// packet overnemen naar buffer[i]..
		memcpy( ptr_players[PlayerIndex[PlayerNr]].Packet[i] , Packet, PacketLen);
		// commando overnemen, zodat het packet later kan worden opgezocht..
		ptr_players[PlayerIndex[PlayerNr]].BufferCMD[i] = CMD;
		// teller voor het aantal keren nog proberen..
		ptr_players[PlayerIndex[PlayerNr]].BufferCountdown[i] = MAXRESEND;
	} else {
		// buffer overflow
		MsgprintyIP2( "buffer", PlayerIndex[PlayerNr], "<add FAILED: buffers full> PlayerNr:", PlayerNr);
		MsgprintyIP2( "buffer", PlayerIndex[PlayerNr], "<add FAILED: buffers full> CMD:", CMD);

		KickPlayer( PlayerNr , ""); //connection error
/*
		// de andere spelers een C6-08 sturen..
		PacketLen = sizeof(ServerQuit);
		memcpy( TXPacket, ServerQuit, PacketLen);
		// playerNumber van deze speler
		if ( GlobalState < stateSwitch ) { //changed: 2008apr30
			//						if ( GlobalState <= stateNumbering  ) {								
			TXPacket[6] = PlayerNr; //PlayerNr ;//getID(PlayerNr);  ///ptr_players[ PlayerIndex[PlayerNr] ].ID ;
		} else {
			TXPacket[6] = getStartPlayerNr(PlayerNr); //PlayerNr ;//getID(PlayerNr);  ///ptr_players[ PlayerIndex[PlayerNr] ].ID ;
		}
		for ( j=1; j < NumPlayers; j++ ) {
			if ( PlayerNr != j ) {
				increaseChatLine( j );
				tmpUShort = getChatLine(j);
				//tmpUShort = getLastCMD(j);
				//changeLastCMD( j, tmpUShort+1 ); //nieuwe waarde onthouden..
				TXPacket[3] = tmpUShort & 0xFF;
				TXPacket[4] = (tmpUShort >> 8) & 0xFF;
				//
				rdcksum( TXPacket,PacketLen );
				BufferPacket( j, TXPacket, PacketLen, tmpUShort );// heb ik veranderd
				//SendToPlayer( j, PacketLen );//, tmpUShort );
			}
		}
		//BuffersResend();

		changeState( PlayerNr, 0 );
		// verwijder de speler
		changeQuit(PlayerNr,1);
		GlobalPlayersQuit++;

		RemovePlayer( PlayerNr, (GlobalState<stateSwitch)?1:0 );
//		RemovePlayer( PlayerNr );   //  20080505   toegevoegd... naar aanleiding van verhaal C.. en ik blijf net in een server staan terwijl ik Quit deed...

//		if ( GlobalState < stateSwitch ) { //changed: 2008apr30 zit al in removeplayer
//			// indien in de lobby, playerlist naar de rest
//			SendAllPlayerList( -1 );
//		}
*/

	}
	
	//MsgprintyIP2( "buffer", PlayerIndex[PlayerNr], "<add>", CMD);
	//break;
	//}
	//}
	DisplayBuffers();
}
/* zo was ie
void BuffersResend() {
int		PlayerNr;
int		i;
int		j;
int		k;
int		nrSent;
u_long	removers[8];
int		removersCount;
char	*log[1024];
char	*Nickname[16];

  // de spelers die worden verwijderd onthouden, en achteraf pas wissen uit ptr_players
  removersCount = 0;
  //for ( i=0; i<8; i++ ) removers[i] = 0;
  
	
	  for (PlayerNr=1; PlayerNr<NumPlayers; PlayerNr++) {
	  if ( getPortNr(PlayerNr)==0) continue;
	  // alleen buffers opnieuw verzenden, als er wat in staat..
	  nrSent = 0;
	  if ( ptr_players[PlayerIndex[PlayerNr]].BufferedPackets > 0 ) {
	  // alle buffers..
	  i=ptr_players[PlayerIndex[PlayerNr]].PtrBufferRead;
	  
		for (k=0; k<=MAXBUFF; k++ ) {
		
		  // ..doorlopen die gevuld zijn
		  if ( ptr_players[PlayerIndex[PlayerNr]].BufferLen[i] > 0 ) {
		  
			// maximaal aantal keer geprobeerd met de cient te communiceren?..
			if ( ptr_players[PlayerIndex[PlayerNr]].BufferCountdown[i] <= 0 ) {
			// gebufferde packet herzenden uitschakelen..
			ptr_players[PlayerIndex[PlayerNr]].BufferLen[i] = -1;
			ptr_players[PlayerIndex[PlayerNr]].BufferCMD[i] = 0x0000;
			
			  //DeletePlayer( PlayerNr );
			  //----
			  // moet dit geen C6 88 zijn?  de server kickt iemand
			  //sprintf( log, "%s", ptr_players[PlayerIndex[PlayerNr]].Nick );
			  memcpy( &Nickname, &ptr_players[PlayerIndex[PlayerNr]].Nick, 16 );
			  MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<bufferResend FAILED: no response> :", Nickname);
			  sprintf( log, "bufferResend FAILED, no response for player: %s", Nickname );
			  Log( log );
			  
				// de andere spelers een C6-08 sturen..
				PacketLen = sizeof(ServerQuit);
				memcpy( TXPacket, ServerQuit, PacketLen);
				
				  // playerNumber van deze speler
				  if ( GlobalState <= stateSwitch ) { //changed: 2008apr30
				  //						if ( GlobalState <= stateNumbering ) {								
				  TXPacket[6] = PlayerNr; //PlayerNr ;//getID(PlayerNr);  ///ptr_players[ PlayerIndex[PlayerNr] ].ID ;
				  } else {
				  TXPacket[6] = getStartPlayerNr(PlayerNr); //PlayerNr ;//getID(PlayerNr);  ///ptr_players[ PlayerIndex[PlayerNr] ].ID ;
				  }
				  // de speler verwijderen, en alle variabelen bijwerken..
				  //RemovePlayer( PlayerNr );
				  
					//!!					for ( j=1; j < MaxPlayers; j++ ) {
					//!!						if ( PlayerIndex[PlayerNr] != j  && PlayerIndex[j] != -1) {
					for ( j=1; j < NumPlayers; j++ ) {
					if ( PlayerNr != j ) {
					// CMD
					
					  increaseChatLine( j );
					  tmpUShort = getChatLine(j);
					  //tmpUShort = getLastCMD(j);
					  //changeLastCMD( j, tmpUShort+1 ); //nieuwe waarde onthouden..
					  TXPacket[3] = tmpUShort & 0xFF;
					  TXPacket[4] = (tmpUShort >> 8) & 0xFF;
					  //
					  rdcksum( TXPacket,PacketLen );
					  //BufferPacket( j, TXPacket, PacketLen, tmpUShort );// heb ik veranderd
					  SendToPlayer( j, PacketLen );//, tmpUShort );
					  }
					  }
					  //----
					  
						DisplayBuffers();
						//BuffersResend();
						changeState( PlayerNr, 0 );
						
						  //@						RemovePlayer( PlayerNr );
						  removers[removersCount] = PlayerNr; //TXPacket[6];
						  removersCount++;
						  
							
							  }
							  //					nrSent++;
							  // om de 10 packets, daadwerkelijk een packet herzenden..
							  if ( ptr_players[PlayerIndex[PlayerNr]].BufferCountdown[i] % 10 == 0 ) {
							  //tmpULong = ((MAXRESEND - ptr_players[PlayerIndex[PlayerNr]].BufferCountdown[i] )* 33); // ms since start resend
							  
								//if ( tmpULong % getAvgPing(PlayerNr) > (getAvgPing(PlayerNr)*0.9)  || ptr_players[PlayerIndex[PlayerNr]].BufferCountdown[i]==MAXRESEND ) {// die %30 vind ik zo raar..wanneer zend ie echt?
								
								  // zo doet ie het maar 1 * .. of elke keer na > avgping.. die 1e keer...
								  // hij moet om de AvgPing sturen..ja ik snap t probleem  ?? zoiets ? stuur ie om de avgp/2 ?
								  // 999-buffercount wordt steeds kleiner... mod avgpng
								  //      BufCD
								  //	1000-	999=0  modavgpg = altijd iets van 0..avgping  iig
								  //	999-	998=1	
								  //	999-	997=2
								  //	999-	996=3
								  // volgens mij rijp voor een testje op t net :)  h a
								  // ja ? of fixed eerst even  ja OK..doe ik mee   ok
								  // wat is de max van BufferCountdown[i]   en de max avgping? ja 999 9a9v9g999p9ing = 6000 of zo...en 9dan99 die is 999999999 ofzo
								  // dus om de avgping een x aantal packetjes versturen..  
								  // stel buffercount=999   en avgping=1600     kan?    999 mod 1600 = 999   if < 1600/2
								  // Volgens mij werkt dit ^^ beter, als die bufferesend met een timer werkt bovenin.. dan word een resend om de x seconden gedaan..nu is het maar willekeurig getimed
								  
									// gaat de maxresend op 10 ofzo ? weet ik niet  :o   of die countdown  of hoe moeten we meten of de ping voorbij is
									
									  // ff denken    /// elke resend een buf countdown --   ///   begint bij 0 ? 99of?  met 0 en dan optellen ? of vanaf 99 aftellen  of? 
									  // 99999999 - cd = 1*33 ms ofzo als cd =1 onee 1*33ms  1/30 sec == 33.3ms klopt wel mooi voor een gewone ping
									  //
									  // ok..
									  nrSent++;
									  // alleen bij versturen afbeelden...
									  DisplayBuffers();
									  //SendToPlayer PlayerNr, ptr_players[].BufferLen[i] )
									  peerOut.sin_addr.s_addr = ptr_players[PlayerIndex[PlayerNr]].AddrIn.s_addr;
									  peerOut.sin_port        = htons(ptr_players[PlayerIndex[PlayerNr]].PortNr);
									  peerOut.sin_family      = AF_INET;
									  if( TrafficOut( sendto(sd, ptr_players[PlayerIndex[PlayerNr]].Packet[i], ptr_players[PlayerIndex[PlayerNr]].BufferLen[i], 0, (struct sockaddr *)&peerOut, (socklen_t *) sizeof(peerOut)) ) < 0) std_err();
									  if( TrafficOut( sendto(sd, ptr_players[PlayerIndex[PlayerNr]].Packet[i], ptr_players[PlayerIndex[PlayerNr]].BufferLen[i], 0, (struct sockaddr *)&peerOut, (socklen_t *) sizeof(peerOut)) ) < 0) std_err();
									  
										//MsgprintyIP2(inet_ntoa(peerOut.sin_addr), ntohs(peerOut.sin_port), "<Buffer Resend>", ptr_players[PlayerIndex[PlayerNr]].BufferCMD[i] );
										}
										
										  // aftellen..
										  ptr_players[PlayerIndex[PlayerNr]].BufferCountdown[i]--;
										  
											// socket reset
											peerOut.sin_addr.s_addr  = INADDR_ANY;
											peerOut.sin_port         = htons(0);
											peerOut.sin_family       = AF_INET;
											
											  if ( getPing(PlayerNr) < 50 && nrSent > 10  ) {
											  break;  ///// test   1 pakketje versturen
											  }
											  if ( getPing(PlayerNr) < 500 && nrSent > 15) {
											  break;  ///// test   3 pakketjes versturen
											  }
											  if ( getPing(PlayerNr) > 500 && nrSent > 20 ) {
											  break;  ///// test   8 pakketjes versturen
											  }
											  
												}
												
												  i++;
												  if ( i >= MAXBUFF ) i = 0;
												  }
												  }
												  }
												  //DisplayBuffers();
												  // de game-status controleren
												  //CheckGlobalState();
												  
													// evt. spelers verwijderen..
													for ( i=0; i<removersCount; i++ ) RemovePlayer( removers[i] );
													if ( removersCount>0 ) playerNumber = -1;
													}
*/
void BuffersResend() {
	int		PlayerNr;
	int		i;
	int		j;
	int		k;
	int		nrSent;
	u_long	removers[8];
	int		removersCount;
	char	*log[1024];
	char	*Nickname[16];
	
	// de spelers die worden verwijderd onthouden, en achteraf pas wissen uit ptr_players
	removersCount = 0;
	//for ( i=0; i<8; i++ ) removers[i] = 0;
	
	
	for (PlayerNr=1; PlayerNr<NumPlayers; PlayerNr++) {
		if ( getPortNr(PlayerNr)==0) continue;
		// alleen buffers opnieuw verzenden, als er wat in staat..
		nrSent = 0;
		if ( ptr_players[PlayerIndex[PlayerNr]].BufferedPackets > 0 ) {
			// alle buffers..
			i=ptr_players[PlayerIndex[PlayerNr]].PtrBufferRead;
			
			for (k=0; k<MAXBUFF; k++ ) {
				//for (k=0; k<1; k++ ) {  ///  1 keer...
				
				// ..doorlopen die gevuld zijn
				if ( ptr_players[PlayerIndex[PlayerNr]].BufferLen[i] > 0 ) {
					
					//					nrSent++;
					// om de 10 packets, daadwerkelijk een packet herzenden..
					if ( ptr_players[PlayerIndex[PlayerNr]].BufferCountdown[i] % 10 == 0 ) {
						//tmpULong = ((MAXRESEND - ptr_players[PlayerIndex[PlayerNr]].BufferCountdown[i] )* 33); // ms since start resend
						
						//if ( tmpULong % getAvgPing(PlayerNr) > (getAvgPing(PlayerNr)*0.9)  || ptr_players[PlayerIndex[PlayerNr]].BufferCountdown[i]==MAXRESEND ) {// die %30 vind ik zo raar..wanneer zend ie echt?
						nrSent++;
						// alleen bij versturen afbeelden...
						DisplayBuffers();
						//SendToPlayer PlayerNr, ptr_players[].BufferLen[i] )
						peerOut.sin_addr.s_addr = ptr_players[PlayerIndex[PlayerNr]].AddrIn.s_addr;
						peerOut.sin_port        = htons(ptr_players[PlayerIndex[PlayerNr]].PortNr);
						peerOut.sin_family      = AF_INET;
						if( TrafficOut( sendto(sd, ptr_players[PlayerIndex[PlayerNr]].Packet[i], ptr_players[PlayerIndex[PlayerNr]].BufferLen[i], 0, (struct sockaddr *)&peerOut, (socklen_t *) sizeof(peerOut)) ) < 0) std_err();
						if( TrafficOut( sendto(sd, ptr_players[PlayerIndex[PlayerNr]].Packet[i], ptr_players[PlayerIndex[PlayerNr]].BufferLen[i], 0, (struct sockaddr *)&peerOut, (socklen_t *) sizeof(peerOut)) ) < 0) std_err();
						
						//MsgprintyIP2(inet_ntoa(peerOut.sin_addr), ntohs(peerOut.sin_port), "<Buffer Resend>", ptr_players[PlayerIndex[PlayerNr]].BufferCMD[i] );
					}
					
					// aftellen..
					ptr_players[PlayerIndex[PlayerNr]].BufferCountdown[i]--;
					
					// maximaal aantal keer geprobeerd met de cient te communiceren?..
					if ( ptr_players[PlayerIndex[PlayerNr]].BufferCountdown[i] <= 0 ) {
						// gebufferde packet herzenden uitschakelen..
						ptr_players[PlayerIndex[PlayerNr]].BufferLen[i] = -1;
						ptr_players[PlayerIndex[PlayerNr]].BufferCMD[i] = 0x0000;
						
						//DeletePlayer( PlayerNr );
						//----
						// moet dit geen C6 88 zijn?  de server kickt iemand
						
						//						//sprintf( log, "%s", ptr_players[PlayerIndex[PlayerNr]].Nick );
						//						memcpy( &Nickname, &ptr_players[PlayerIndex[PlayerNr]].Nick, 16 );
						//						MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<bufferResend FAILED: no response> :", Nickname);
						//						sprintf( log, "bufferResend FAILED, no response for player: %s", Nickname );
						//						Log( log );
						
						// de andere spelers een C6-08 sturen..
						PacketLen = sizeof(ServerQuit);
						memcpy( TXPacket, ServerQuit, PacketLen);
						
						// playerNumber van deze speler
						if ( GlobalState < stateSwitch ) { //changed: 2008apr30
							//						if ( GlobalState <= stateNumbering  ) {								
							TXPacket[6] = PlayerNr; //PlayerNr ;//getID(PlayerNr);  ///ptr_players[ PlayerIndex[PlayerNr] ].ID ;
						} else {
							TXPacket[6] = getStartPlayerNr(PlayerNr); //PlayerNr ;//getID(PlayerNr);  ///ptr_players[ PlayerIndex[PlayerNr] ].ID ;
						}
						// de speler verwijderen, en alle variabelen bijwerken..
						//RemovePlayer( PlayerNr );
						
						//!!					for ( j=1; j < MaxPlayers; j++ ) {
						//!!						if ( PlayerIndex[PlayerNr] != j  && PlayerIndex[j] != -1) {
						for ( j=1; j < NumPlayers; j++ ) {
							if ( PlayerNr != j ) {
								// CMD
								
								increaseChatLine( j );
								tmpUShort = getChatLine(j);
								//tmpUShort = getLastCMD(j);
								//changeLastCMD( j, tmpUShort+1 ); //nieuwe waarde onthouden..
								TXPacket[3] = tmpUShort & 0xFF;
								TXPacket[4] = (tmpUShort >> 8) & 0xFF;
								//
								rdcksum( TXPacket,PacketLen );
								BufferPacket( j, TXPacket, PacketLen, tmpUShort );// heb ik veranderd
								//SendToPlayer( j, PacketLen );//, tmpUShort );
							}
						}
						//----
						
						DisplayBuffers();
						//BuffersResend();

						changeState( PlayerNr, 0 );
						//
						changeQuit(PlayerNr,1);
						GlobalPlayersQuit++;

						//@						RemovePlayer( PlayerNr );
						removers[removersCount] = PlayerNr; //TXPacket[6];
						removersCount++;
						
						
					}
					
					// socket reset
					peerOut.sin_addr.s_addr  = INADDR_ANY;
					peerOut.sin_port         = htons(0);
					peerOut.sin_family       = AF_INET;
					
					if ( getPing(PlayerNr) < 50 && nrSent > 1  ) {
						break;  ///// test   1 pakketje versturen
					}
					if ( getPing(PlayerNr) < 500 && nrSent > 15) {
						break;  ///// test   3 pakketjes versturen
					}
					if ( getPing(PlayerNr) > 500 && nrSent > 20 ) {
						break;  ///// test   8 pakketjes versturen
					}
					
					
				}
				
				i++;
				if ( i >= MAXBUFF ) i = 0;
			}
		}
	}
	//DisplayBuffers();
	// de game-status controleren
	//CheckGlobalState();
	
	// evt. spelers verwijderen..
	for ( i=removersCount-1; i>=0; i-- ) {
		changeState( removers[i], 0 );
		changeQuit( removers[i], 1);
		GlobalPlayersQuit++;
		RemovePlayer( removers[i], 0 ); // nu niet voor elke removed player een playerlist sturen..
//		RemovePlayer( removers[i] );
	}
	//if ( removersCount>0 ) playerNumber = -1;

	if ( removersCount>0 && GlobalState<stateSwitch ) { //changed: 2008mei11
		// indien in de lobby, playerlist naar de rest
		SendAllPlayerList( -1 );
	}
}



int FindBufferedPacket( short PlayerNr, u_short CMD ) {
	int		Result=-1; //ongeldig/niet gevonden
	int		i;
	if ( getPortNr(PlayerNr)==0) return;
	// alleen zoeken als er buffer zijn gevuld voor deze speler..
	if ( ptr_players[PlayerIndex[PlayerNr]].BufferedPackets > 0 ) {
		// alle buffers doorzoeken..
		for (i=0;i<MAXBUFF;i++) {
			// buffer in gebruik??
			if ( ptr_players[PlayerIndex[PlayerNr]].BufferLen[i] > 0 ) {
				if ( ptr_players[PlayerIndex[PlayerNr]].BufferCMD[i] == CMD ) {
					Result = i;
					break;
				}
			}
		}
	}
	return Result;
}

//resultaat = 0xXY     X=PlayerNr, Y=BufferNr
// 0xFF = fout code / CMD in buffer niet gevonden..
u_char AckBufferedPacket( short PlayerNr, u_short CMD ) {
	int		Buf;
	// zoek het pakketje op voor deze speler..
	Buf = FindBufferedPacket(PlayerNr, CMD);
	//sprintf( log, "ACK Buf:%d Read:%d", Buf, ptr_players[PlayerIndex[PlayerNr]].PtrBufferRead );
	//Log( log );
	///
	//Buf = ptr_players[PlayerIndex[PlayerNr]].PtrBufferRead;
	///
	
	if ( Buf == -1 ) return 0xFF; //niet gevonden??..
	// verwijder de entry
	ptr_players[PlayerIndex[PlayerNr]].BufferLen[Buf] = -1;
	ptr_players[PlayerIndex[PlayerNr]].BufferCountdown[Buf] = 0;
	ptr_players[PlayerIndex[PlayerNr]].BufferCMD[Buf] = 0x0000;
	
	ptr_players[PlayerIndex[PlayerNr]].BufferedPackets--;
	if (ptr_players[PlayerIndex[PlayerNr]].BufferedPackets<0) ptr_players[PlayerIndex[PlayerNr]].BufferedPackets=0;
	// de LEES-pointer van de ring-buffer aanpassen..
	ptr_players[PlayerIndex[PlayerNr]].PtrBufferRead++;
	if (ptr_players[PlayerIndex[PlayerNr]].PtrBufferRead==MAXBUFF)	ptr_players[PlayerIndex[PlayerNr]].PtrBufferRead=0;
	
	DisplayBuffers();
	return ((PlayerNr << 4) + Buf);
}


int BuffersEmpty() {
	int		Result=0; //leeg
	int		PlayerNr;
	int		i;
	int		kolNr;
	for (PlayerNr=1; PlayerNr<NumPlayers; PlayerNr++) {
		Result += ptr_players[PlayerIndex[PlayerNr]].BufferedPackets;
	}
	
	//DisplayBuffers();
	
	if ( Result == 0 ) {
		Result = -1;
	}
	return Result;
}








void DisplayBuffers() {
	int	i;
	int kolNr=51;
	int	rowNr=LISTTOP+1;
	if (GlobalShowList==0) return;
	// aantal buffers in gebruik afbeelden
	for ( i=0; i<8; i++ ) {
		printxyc(kolNr, rowNr+i, ATTR_BOLD,YELLOW,BLACK, "  " );
		printxyc(kolNr+2, rowNr+i, ATTR_BOLD,YELLOW,BLACK, "  " );
		
		printxycd(kolNr, rowNr+i, ATTR_BOLD,YELLOW,BLACK, ptr_players[i].BufferedPackets );
		printxycd(kolNr+2, rowNr+i, ATTR_BOLD,YELLOW,BLACK, ptr_players[i].BufferCountdown[ptr_players[i].PtrBufferRead] );
	}
}

void DisplayList() {
	u_short kolNr;	//x
	u_short rowNr;	//y De regel van de titelbalk van de lijst
	int		i;
	int		j;
	u_short	style;
	u_char	FG;
	u_char	BG;
	if (GlobalShowList==0) return;
	
	// wis het console scherm vanaf regel rowNr omlaag
	eraseRows(LISTTOP,LISTBOTTOM);
	
	//
	// playerlist afbeelden
	//
	rowNr = LISTTOP;
	if ( GlobalState < stateGridReady ) {
		printxyc(1, rowNr, ATTR_NORMAL,BLACK,GREEN, " S$#ID   IP               Port   Nick             Buf  Country  Car                Gearbox  Ping  Ready  cLine LastCMD  " );
	} else 
		if ( GlobalState < stateRacing ) { // was 7 later 9
			printxyc(1, rowNr, ATTR_NORMAL,BLACK,GREEN, " S$#ID   IP               Port   Nick             Buf  Country  Car                Gearbox  Ping  State  cLine LastCMD  " );
		} else {
			//		printxyc(1, rowNr, ATTR_NORMAL,BLACK,GREEN, " S$#ID   IP               Port   Nick             Buf  Rotation(X,Y,Z)               Position(X,Y,Z)               Time      " );
			if ( GlobalShowTiming == 1 ) {
				if ( GlobalEndPos > 0 ) {
					printxyc(1, rowNr, ATTR_NORMAL,BLACK,GREEN, " S$#ID   IP               Port   Nick             Buf  PingCnt AvgPing   Ping      %                           Time      " );
				} else {
					printxyc(1, rowNr, ATTR_NORMAL,BLACK,GREEN, " S$#ID   IP               Port   Nick             Buf  PingCnt AvgPing   Ping                                  Time      " );
				}
			} else {
				printxyc(1, rowNr, ATTR_NORMAL,BLACK,GREEN, " S$#ID   IP               Port   Nick             Buf  LastPos                                                 Time      " );
			}
		}
		rowNr++;
		//NickC6D6 afbeelden
		for ( i=0; i<NumPlayers; i++ ) {
			if ( ptr_players[PlayerIndex[i]].NickC6D6 == 0xD6 ) {
				printxyc(0, rowNr+i, ATTR_BOLD,CYAN,BLACK, "!" );
			}
		}
		// PlayerIndex's afbeelden
		kolNr = 2;
		for ( i=0; i<NumPlayers; i++ ) {
			printxycd(kolNr, rowNr+PlayerIndex[i], ATTR_NORMAL,BLACK,RED, PlayerIndex[i] ) ;
		}
		kolNr += 1;
		for ( i=0; i<NumPlayers; i++ ) {
			if ( GlobalState <= stateNumbering  ) { //changed: 2008apr30
				printxyc(kolNr, rowNr+PlayerIndex[i], ATTR_NORMAL,BLACK,CYAN, " " );//changed: 2008apr30
			} else {//changed: 2008apr30
				printxycd(kolNr, rowNr+PlayerIndex[i], ATTR_NORMAL,BLACK,CYAN, getStartPlayerNr(i) ) ;
			}//changed: 2008apr30
		}
		kolNr += 1;
		for ( i=0; i<NumPlayers; i++ ) {
			printxycd(kolNr, rowNr+PlayerIndex[i], ATTR_NORMAL,BLACK,GREEN, i ) ;
		}
		kolNr += 1;
		// PlayerID's afbeelden
		for ( i=0; i<NumPlayers; i++ ) {
			printxycx(kolNr, rowNr+PlayerIndex[i], ATTR_NORMAL,BLACK,YELLOW,  ptr_players[PlayerIndex[i]].ID ) ;
		}
		kolNr += 5;
		// IP's afbeelden
		for ( i=0; i<8; i++ ) {
			printxyc(kolNr, rowNr+i, ATTR_BOLD,CYAN,BLACK,  inet_ntoa( ptr_players[i].AddrIn ) ) ;
		}
		kolNr += 17;
		// Poort-nummers afbeelden
		for ( i=0; i<8; i++ ) {
			printxycd(kolNr, rowNr+i, ATTR_BOLD,CYAN,BLACK, ptr_players[i].PortNr );
		}
		kolNr += 7;
		// Nicknames afbeelden
		for ( i=0; i<8; i++ ) {
			printxyc(kolNr, rowNr+i, ATTR_BOLD,YELLOW,BLACK, ptr_players[i].Nick );
		}
		kolNr += 17;
		// aantal buffers in gebruik afbeelden
		for ( i=0; i<8; i++ ) {
			//	printxycd(kolNr, rowNr+i, ATTR_BOLD,YELLOW,BLACK, ptr_players[i].BufferedPackets );
		}
		kolNr += 5;
		
		//	if ( GlobalState < stateRacing ) { //was 9
		if ( GlobalState < stateLobbyWait ) { //was 9
			// Countries afbeelden
			kolNr += 3;
			for ( i=0; i<8; i++ ) {
				printxyc(kolNr, rowNr+i, ATTR_BOLD,GREEN,BLACK, StrCountry[ptr_players[i].Country][0] ); // de afkorting..
			}
			kolNr += 6;
			// Car afbeelden
			for ( i=0; i<8; i++ ) {
				printxyc(kolNr, rowNr+i, ATTR_BOLD,GREEN,BLACK, StrCar[ptr_players[i].Car] );
			}
			kolNr += 19;
			// Gearbox afbeelden
			for ( i=0; i<8; i++ ) {
				printxyc(kolNr, rowNr+i, ATTR_BOLD,GREEN,BLACK, StrTransmission[ptr_players[i].Gearbox] );
			}
			kolNr += 10;
			// Ping afbeelden
			for ( i=0; i<8; i++ ) {
				printxycd(kolNr, rowNr+i, ATTR_BOLD,YELLOW,BLACK, ptr_players[i].AvgPing );
			}
			kolNr += 7;
			if ( GlobalState < stateGridReady ) {
				// Ready afbeelden
				for ( i=0; i<8; i++ ) {
					if (ptr_players[i].Ready==1) {
						printxycd(kolNr, rowNr+i, ATTR_BOLD,WHITE,GREEN, ptr_players[i].Ready );
					} else {
						printxycd(kolNr, rowNr+i, ATTR_BOLD,WHITE,RED, ptr_players[i].Ready );
					}
				}
			} else {
				// State afbeelden
				for ( i=0; i<8; i++ ) {
					if (ptr_players[i].State==6) {
						printxycd(kolNr, rowNr+i, ATTR_BOLD,WHITE,GREEN, ptr_players[i].State );
					} else {
						printxycd(kolNr, rowNr+i, ATTR_BOLD,WHITE,RED, ptr_players[i].State );
					}
				}
			}
			kolNr += 8;
			// cLine  afbeelden
			for ( i=0; i<8; i++ ) {
				printxycx(kolNr, rowNr+i, ATTR_BOLD,YELLOW,BLACK, ptr_players[i].ChatLine);
			}
			kolNr += 8;
			// LastCMD  afbeelden
			for ( i=0; i<8; i++ ) {
				printxycx(kolNr, rowNr+i, ATTR_BOLD,YELLOW,BLACK, ptr_players[i].LastCMD);
			}
			kolNr += 8;
		} else {
			kolNr += 1;
			/*0
			// Rotation+Position  afbeelden
			for ( i=0; i<8; i++ ) {
			printxycx(kolNr,    rowNr+i, ATTR_BOLD,YELLOW,BLACK, ptr_players[i].Rotation.X );
			printxycx(kolNr+10, rowNr+i, ATTR_BOLD,YELLOW,BLACK, ptr_players[i].Rotation.Y );
			printxycx(kolNr+20, rowNr+i, ATTR_BOLD,YELLOW,BLACK, ptr_players[i].Rotation.Z );
			
			  printxycx(kolNr+30, rowNr+i, ATTR_BOLD,YELLOW,BLACK, ptr_players[i].Position.X );
			  printxycx(kolNr+40, rowNr+i, ATTR_BOLD,YELLOW,BLACK, ptr_players[i].Position.Y );
			  printxycx(kolNr+50, rowNr+i, ATTR_BOLD,YELLOW,BLACK, ptr_players[i].Position.Z );
			  }
			  kolNr += 60;
			*/
			if ( GlobalShowTiming == 1 ) {
				// PingCount  afbeelden
				for ( i=0; i<8; i++ ) {
					printxycd(kolNr, rowNr+i, ATTR_BOLD,WHITE,BLACK, ptr_players[i].PingCount);
				}
				kolNr += 9;
				// AvgPing  afbeelden
				for ( i=0; i<8; i++ ) {
					printxycd(kolNr, rowNr+i, ATTR_BOLD,WHITE,BLACK, ptr_players[i].AvgPing);
				}
				kolNr += 9;
				// Ping  afbeelden
				for ( i=0; i<8; i++ ) {
					printxycd(kolNr, rowNr+i, ATTR_BOLD,WHITE,BLACK, ptr_players[i].Ping);
				}
				kolNr += 9;
				// Percentage  afbeelden
				for ( i=0; i<NumPlayers; i++ ) {
					j = (ptr_players[PlayerIndex[i]].LastPos[25] << 8) + ptr_players[PlayerIndex[i]].LastPos[24];
					if (GlobalEndPos>0) {
						// het percentage gereden van deze stage afbeelden
						j = (j * 100.0) / (GlobalEndPos * 100.0) * 100;
						if ( j>100 ) j = 100;
						printxycd(kolNr, rowNr+PlayerIndex[i], ATTR_NORMAL,WHITE,RED, j );
					} else {
						// de 2 bytes uit LastPos afbeelden..
						printxycd(kolNr, rowNr+PlayerIndex[i], ATTR_NORMAL,WHITE,RED, j );
					}
				}
				kolNr += 8;
				// State afbeelden
				for ( i=0; i<8; i++ ) {
					if (ptr_players[i].State==6) {
						printxycd(kolNr, rowNr+i, ATTR_BOLD,WHITE,GREEN, ptr_players[i].State );
					} else {
						printxycd(kolNr, rowNr+i, ATTR_BOLD,WHITE,RED, ptr_players[i].State );
					}
				}
				
				kolNr += 8;
				// Retired afbeelden
				for ( i=0; i<8; i++ ) {
					if (ptr_players[i].hasRetired==1) {
						printxycd(kolNr, rowNr+i, ATTR_BOLD,WHITE,GREEN, ptr_players[i].hasRetired );
					} else {
						printxycd(kolNr, rowNr+i, ATTR_BOLD,WHITE,RED, ptr_players[i].hasRetired );
					}
				}
				
				kolNr += 8;
				// Finished afbeelden
				for ( i=0; i<8; i++ ) {
					if (ptr_players[i].hasFinished>=1) {
						printxycd(kolNr, rowNr+i, ATTR_BOLD,WHITE,GREEN, ptr_players[i].hasFinished );
					} else {
						printxycd(kolNr, rowNr+i, ATTR_BOLD,WHITE,RED, ptr_players[i].hasFinished );
					}
				}
				
				
				kolNr += 4;
			} else {
				// LastPos afbeelden
				for ( i=0; i<NumPlayers; i++ ) {
					for ( j=0; j<27; j++ ) {
						style = ATTR_NORMAL;
						FG = YELLOW;
						BG = BLACK;
						if (j==1) {								// speler nummer{0..7}
							style = ATTR_BOLD;
							FG = RED;
						}
						if (j>=2 && j<=5) {						// RaceTime in ms.
							style = ATTR_BOLD;
							FG = WHITE;
						}
						if (j>=12 && j<=14) style = ATTR_BOLD;	// pos, rot of iets..3 bytes?
						if (j>=16 && j<=18) style = ATTR_BOLD;	// pos, rot of iets..3 bytes?
						if (j>=20 && j<=22) style = ATTR_BOLD;	// pos, rot of iets..3 bytes?
						if (j>=24 && j<=25) {
							// BCD byte LastPos[24] telt van 0x00 tot en met 0x99, en geeft aan
							// het percentage tot aan de 1e split (LastPos[25]==0x00)
							//
							// Na de eerste split, telt LastPos[24] van 0x10..0x25 (BCD met LastPos[25] nog steeds 0x00)
							// (precies op de 2e split, is de waarde LastPos[24]==0x22)
							//
							// (Na LastPos[24]==0x25, komt echter GEEN 0x26 als verwacht, maar..)
							// De waarde LastPos[24] gaat op 0x00, en LastPos[25]=0x01 (en blijft dat tot einde race).
							// Dan telt de waarde LastPos[24] weer op van 0x00 tot en met 0x99,
							//
							// (precies bij de hairpin om de boom in USA5, het laatste stuk zand weer op),
							// Op dat moment (hairpin) wordt LastPos[24]=0x10.
							// De (BCD)teller LastPos[24] loopt op tot en met 0x22.
							//
							// Op BCD-teller LastPos[24]==0x22 finisht de speler.
							// De client blijft wel 42 & 43 packets sturen met tijden die doorlopen,
							// dus op tijd de eind-race-tijd overnemen !
							//
							// Op het moment dat de client het scherm met scores krijgt te zien,
							// worden 42- & 43-packets gestuurd door de client met LastPos[24]==0x23.
							// (de tijd loopt dan nog door).
							style = ATTR_BOLD;
							FG = GREEN;
						}
						if (j==6 || j==8 || j==10) {					// ondergrond? grip? op koers?
							style = ATTR_BOLD;
							FG = BLUE;
						}
						printxycc(kolNr+j*2, rowNr+PlayerIndex[i], style,FG,BG, ptr_players[PlayerIndex[i]].LastPos[j] );
					}
				}
				kolNr += 27*2+1;
			}
			
			// RaceTime afbeelden
			for ( i=0; i<8; i++ ) {
				//snprintf( &tmpString,6, "%6.2f", (float)ptr_players[i].RaceTime/1000 ); //werkt prima zo!
				MStoTimeString( ptr_players[i].RaceTime, &tmpString );
				printxyc(kolNr, rowNr+i, ATTR_BOLD,WHITE,BLACK, tmpString );
			}
			kolNr += 10;

			//TEST of dit connection speed is    changed: 2008mei2
			for ( i=0; i<NumPlayers; i++ ) {
//				printxycc(kolNr, rowNr+PlayerIndex[i], ATTR_NORMAL,YELLOW,BLACK, ptr_players[PlayerIndex[i]].RXPacket30 );
				printxycc(kolNr, rowNr+PlayerIndex[i], ATTR_NORMAL,YELLOW,BLACK, ptr_players[PlayerIndex[i]].ConnectionSpeed );
			}

	}
	//	
	printxyc(2, rowNr+9, ATTR_NORMAL,WHITE,BLACK, "\n" );
}

void DisplayStatus() {
	u_char *StrTrack;
	if (GlobalShowList==0) return;
	//	fprintf( stdout, "\33[s\33[%i;%iH\33[1;30;43m LastTimeValue = %x\33[u", STATUSLINE-1, 2 , LastTimeValue );
	// Hostname, Port, GameMode, Damage, Rally, Stages, Ranking, CarType
	//fprintf( stdout, "\33[s\33[%i;%iH\33[1;30;43m%x\33[u ", STATUSLINE, 2 , GlobalState );
	fprintf( stdout, "\33[s\33[%i;%iH\33[1;30;43m%x\33[u", STATUSLINE, 2 , GlobalState );
	fprintf( stdout, "\33[s\33[%i;%iH\33[1;30;43mBuf: %d\33[u", STATUSLINE+1, 1 , BuffersEmpty() );
	/*
	fprintf( stdout, "\33[s\33[%i;%iH\33[0;33;40m	%s : %d GameType: %s GameMode: %s Password: %s	 IN:%d/%d/%d OUT:%d/%d/%d  \33[u", STATUSLINE, 5 , \
	cmr4hostname, port, StrGameType[GlobalGameType], StrGameMode[GlobalGameMode], StrPassword[GlobalPassword], BytesInSec/KB_PER_SECOND, BytesInMin/KB_PER_MINUTE, BytesInHour/KB_PER_HOUR, BytesOutSec/KB_PER_SECOND, BytesOutMin/KB_PER_MINUTE, BytesOutHour/KB_PER_HOUR  );
	*/
	fprintf( stdout, "\33[s\33[%i;%iH\33[0;33;40m	%s : %d GameType: %s GameMode: %s Password: %s	 IN:%d/%d/%d/(%d) OUT:%d/%d/%d/(%d)  \33[u", STATUSLINE, 5 , \
		cmr4hostname, port, StrGameType[GlobalGameType], StrGameMode[GlobalGameMode], StrPassword[GlobalPassword], BytesInSec/KB_PER_SECOND, BytesInLastMinAvg/KB_PER_MINUTE, BytesInLastHourAvg/KB_PER_HOUR, BytesInPeak/KB_PER_SECOND, BytesOutSec/KB_PER_SECOND, BytesOutLastMinAvg/KB_PER_MINUTE, BytesOutLastHourAvg/KB_PER_HOUR, BytesOutPeak/KB_PER_SECOND  );
	
	
	StrTrack = (GlobalRally==14)? StrStages[GlobalStages_Land][GlobalStages_Stage]: StrRallies[GlobalRally];
	
	fprintf( stdout, "\33[s\33[%i;%iH\33[0;33;40m		%s %d 	CarType: %s	Ranking: %s	Damage: %s   Retired: %d Finished: %d Quit: %d \33[u", STATUSLINE+1, 5 , \
		StrTrack, GlobalEndPos, StrCarType[GlobalCarType], StrRanking[GlobalRanking], StrDamage[GlobalDamage], GlobalPlayersRetired, GlobalPlayersFinished, GlobalPlayersQuit );
}

void DisplayProgress() {
	if (GlobalShowList==0) return;
	fprintf( stdout, "\33[s\33[%i;%iH\33[1;33;40m%s\33[u", STATUSLINE, 1 , StrProgress[ProgressCount] );
	ProgressCount++; //ProgressCount verhogen..
	ProgressCount %= 4; //ProgressCount mod 4 overhouden..
}

void DisplayTiming() {
	u_short kolNr;	//x
	u_short rowNr;	//y De regel van de titelbalk van de lijst
	int		i;
	int		d;
	u_long	vTime;
	u_char	timeString[256];
	if (GlobalShowTiming==0) return;
	if (GlobalState<stateRacing) return;   // was 7 later 9
	printxyc( 1,TIMINGTOP, ATTR_BOLD,BLACK,GREEN, "  RaceTime   SplitTime  Ping1      Ping2     " );
	for ( i=0; i<NumPlayers; i++ ) {
		kolNr = 3;
		rowNr = TIMINGTOP+1+PlayerIndex[i];
		// RaceTime
		vTime = ptr_players[PlayerIndex[i]].RaceTime;
		MStoTimeString( vTime, &timeString );
		printxyc( kolNr,rowNr, ATTR_BOLD,WHITE,BLACK, timeString );
		kolNr += 11;
		// SplitTime
		d = ptr_players[PlayerIndex[i]].SplitsDone;
		vTime = ptr_players[PlayerIndex[i]].SplitTime[d-1];
		MStoTimeString( vTime, &timeString );
		printxyc(kolNr, rowNr, ATTR_BOLD,WHITE,BLACK, timeString );
		kolNr += 11;
		/*
		// Ping1
		//		printxycx( kolNr,rowNr, ATTR_BOLD,WHITE,BLACK, ptr_players[PlayerIndex[i]].Ping1 );
		vTime = ptr_players[PlayerIndex[i]].Ping1;
		MStoTimeString( vTime, &timeString );
		printxyc(kolNr, rowNr, ATTR_BOLD,WHITE,BLACK, timeString );
		kolNr += 11;
		// Ping2
		//		printxycx( kolNr,rowNr, ATTR_BOLD,WHITE,BLACK, ptr_players[PlayerIndex[i]].Ping2 );
		vTime = ptr_players[PlayerIndex[i]].Ping2;
		MStoTimeString( vTime, &timeString );
		printxyc(kolNr, rowNr, ATTR_BOLD,WHITE,BLACK, timeString );
		kolNr += 11;
		*/
	}
}







// de huidige timevalue resulteren
u_long minisec() {
	struct timeval	TimeValue;
	u_long			Result=0;
	// de huidige tijd proberen op te vragen
	if ( gettimeofday( &TimeValue, NULL ) == 0 ) {
		// de tijd in milliseconden omrekenen
		Result = (TimeValue.tv_sec*1000) + (TimeValue.tv_usec/1000);
	}
	return Result;
}

u_char timerPassedSendBuffers( u_long intervalMS ) {
	u_char	Result=0;
	u_long	MS;
	MS = minisec();
	Result = (MS - LastTimeValueSendBuf > intervalMS)? 1: 0;
	if (Result==1) LastTimeValueSendBuf = MS;
	/*
	scrollRows( MSGTOP, MSGBOTTOM );	// regels MSGTOP t/m MSGBOTTOM scrollen
	printxycx( 3,MSGBOTTOM, ATTR_BOLD,MAGENTA,BLACK, LastTimeValue );
	*/
	return Result;
}

u_char timerPassed( u_long intervalMS ) {
	u_char	Result=0;
	u_long	MS;
	MS = minisec();
	Result = (MS - LastTimeValue > intervalMS)? 1: 0;
	if (Result==1) LastTimeValue = MS;
	/*
	scrollRows( MSGTOP, MSGBOTTOM );	// regels MSGTOP t/m MSGBOTTOM scrollen
	printxycx( 3,MSGBOTTOM, ATTR_BOLD,MAGENTA,BLACK, LastTimeValue );
	*/
	return Result;
}

u_char timerPassed2( u_long intervalMS ) {
	u_char	Result=0;
	u_long	MS;
	MS = minisec();
	Result = (MS - LastTimeValue2 > intervalMS)? 1: 0;
	if (Result==1) LastTimeValue2 = MS;
	return Result;
}


u_char timerPassed3( u_long intervalMS ) {
	u_char	Result=0;
	u_long	MS;
	MS = minisec();
	Result = (MS - LastTimeValue3 > intervalMS)? 1: 0;
	if (Result==1) LastTimeValue3 = MS;
	return Result;
}








u_char *getCountryStr(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return "";
	return StrCountry[ptr_players[PlayerIndex[PlayerNr]].Country];
}
u_char *getCarStr(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return "";
	return StrCar[ptr_players[PlayerIndex[PlayerNr]].Car];
}
u_char *getCarTypeStr(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return "";
	return StrCarType[ptr_players[PlayerIndex[PlayerNr]].CarType+1];
}
u_char *getGearboxStr(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return "";
	return StrTransmission[ptr_players[PlayerIndex[PlayerNr]].Gearbox];
}
u_char *getPingStr(short PlayerNr) {
	//u_long
	return "";
}








void MakeIngamePlayerList( short playerNumber ) {
	int i = 0;
	int Offset = 0;
	u_short cl;
	u_char	tmpChar;
	
	//	if ( playerNumber == 0 ) return;
	
	Offset = (TXPacket[0]==0xC6)? 0: 2; // een D6 packet is 2 bytes langer dan een C6
	
	if (TXPacket[0]==0xD6) {
		TXPacket[3] = RXPacket[3];
		TXPacket[4] = RXPacket[4];
	}
	if ( playerNumber != -1 ) {
		// client's eigen handle-dingetje inplakken..
		increaseChatLine( playerNumber);
		cl = getChatLine( playerNumber );
		TXPacket[3+Offset] = cl & 0xFF;
		TXPacket[4+Offset] = (cl >> 8) & 0xFF;
	}
	
	// Max, Num
	TXPacket[6+Offset] = MaxPlayers;
	TXPacket[7+Offset] = NumPlayers;
	
	//	for ( i=0; i < NumPlayers; i++ ) {
	for ( i=0; i < MAXPLAYERS; i++ ) {
		if ( i==0 ) {
			(u_long *)TXPacket[Offset+9+(i*36)] = getID(i); ///////////////   welke ?
			(u_long *)TXPacket[Offset+9+(i*36)+4] = getReady(i);
			(u_char *)TXPacket[Offset+9+(i*36)+24] = getCountry(i);
			(u_long *)TXPacket[Offset+9+(i*36)+28] = getPing(i);
			
			/*tmpChar = getVoteGhost(playerNumber);
			if ( tmpChar==0 ) tmpChar = 1;
			tmpChar = (tmpChar-1+8);*/
			tmpChar = (getVoteGhost(playerNumber)+8);
			//			tmpChar = (getCarType(playerNumber)+8);
			
			//sprintf( log, "playerlist: %d Car: %d CarType: %d Gearbox: %d", tmpChar, ptr_players[tmpChar].Car, ptr_players[tmpChar].CarType, ptr_players[tmpChar].Gearbox );
			//Log( log );
			//MsgprintyIP1( "",0, "<playerlist>", log );
			
			if (minisec() % 10000 > 5000 || GlobalState > stateLobby ) {
				MStoTimeString(ptr_players[tmpChar].RaceTime , (u_char *)TXPacket+Offset+9+(i*36)+8 );   //zie niet veranderen naam van speler tijdens rally...
			} else {
				strcpy( (u_char *) TXPacket+Offset+9+(i*36)+8, ptr_players[tmpChar].Nick );
			}
			
			(u_char *)TXPacket[Offset+9+(i*36)+33] = ptr_players[tmpChar].Car;
			(u_char *)TXPacket[Offset+9+(i*36)+34] = ptr_players[tmpChar].CarType;
			(u_char *)TXPacket[Offset+9+(i*36)+35] = ptr_players[tmpChar].Gearbox;
			
			/*
			switch ( getCarType(playerNumber) ) {
			case 0: //Any
			strcpy( (u_char *) TXPacket+Offset+9+(i*36)+8, getNick(0) );
			(u_char *)TXPacket[Offset+9+(i*36)+33] = getCar(0);
			(u_char *)TXPacket[Offset+9+(i*36)+34] = getCarType(0);
			(u_char *)TXPacket[Offset+9+(i*36)+35] = getGearbox(0);
			break;
			case 1: //4WD
			strcpy( (u_char *) TXPacket+Offset+9+(i*36)+8, getNick(8) );
			(u_char *)TXPacket[Offset+9+(i*36)+33] = getCar(8);
			(u_char *)TXPacket[Offset+9+(i*36)+34] = getCarType(8);
			(u_char *)TXPacket[Offset+9+(i*36)+35] = getGearbox(8);
			break;
			case 2: //2WD
			strcpy( (u_char *) TXPacket+Offset+9+(i*36)+8, getNick(9) );
			(u_char *)TXPacket[Offset+9+(i*36)+33] = getCar(9);
			(u_char *)TXPacket[Offset+9+(i*36)+34] = getCarType(9);
			(u_char *)TXPacket[Offset+9+(i*36)+35] = getGearbox(9);
			break;
			case 3: //Group B
			strcpy( (u_char *) TXPacket+Offset+9+(i*36)+8, getNick(10) );
			(u_char *)TXPacket[Offset+9+(i*36)+33] = getCar(10);
			(u_char *)TXPacket[Offset+9+(i*36)+34] = getCarType(10);
			(u_char *)TXPacket[Offset+9+(i*36)+35] = getGearbox(10);
			break;
			case 4: //Bonus
			strcpy( (u_char *) TXPacket+Offset+9+(i*36)+8, getNick(11) );
			(u_char *)TXPacket[Offset+9+(i*36)+33] = getCar(11);
			(u_char *)TXPacket[Offset+9+(i*36)+34] = getCarType(11);
			(u_char *)TXPacket[Offset+9+(i*36)+35] = getGearbox(11);
			break;
			}
			*/
		} else {
			(u_long *)TXPacket[Offset+9+(i*36)] = getID(i); ///////////////   welke ?
			(u_long *)TXPacket[Offset+9+(i*36)+4] = getReady(i);
			strcpy( (u_char *) TXPacket+Offset+9+(i*36)+8, getNick(i) );
			(u_char *)TXPacket[Offset+9+(i*36)+24] = getCountry(i);
			(u_long *)TXPacket[Offset+9+(i*36)+28] = getAvgPing(i);
			(u_char *)TXPacket[Offset+9+(i*36)+33] = getCar(i);
			(u_char *)TXPacket[Offset+9+(i*36)+34] = getCarType(i);
			(u_char *)TXPacket[Offset+9+(i*36)+35] = getGearbox(i);
		}
	}
}

void SendAllPlayerList(short playerNumber) {
	int i=0;
	u_short CMD;
	
	// het regelnummer van de host ophogen..
	increaseChatLine(0);
	
	// Playerlists maken en verzenden
	// eerst alle C6's aan de andere spelers..
	PacketLen = sizeof(ServerPlayListC6);
	memcpy( TXPacket, ServerPlayListC6, PacketLen );
	for ( i=1; i < NumPlayers; i++ ) { //naar iedereen behalve de host..
		if ( playerNumber != i ) {
			MakeIngamePlayerList( i );
			rdcksum( TXPacket, PacketLen );
			CMD = getChatLine( i );
			BufferPacket( i, TXPacket, PacketLen, CMD );
		}
	}
	//BuffersResend();
	/*
	sprintf( log, "SendAllPlayerList:");
	for ( i=0; i<297 ; i++ ) {
	sprintf( log,"%s%X ", log, TXPacket[i] );
	}
	Log( log );
	*/
	// De D6 aan de zender van het packet (playerNumber)..
	if (playerNumber > 0) {
		PacketLen = sizeof(ServerPlayListD6);
		memcpy( TXPacket, ServerPlayListD6, PacketLen );
		MakeIngamePlayerList( playerNumber );
		rdcksum( TXPacket, PacketLen );
		CMD = getChatLine( playerNumber );//via buffer ook nog
		BufferPacket( playerNumber, TXPacket, PacketLen, CMD );
		/*
		sprintf( log, "SendAllPlayerListD6:");
		for ( i=0; i<297 ; i++ ) {
		sprintf( log,"%s%X ", log, TXPacket[i] );
		}
		Log( log );
		*/
	}
	
	// zend iedereen een ping lijst..
	Server_PingList( -1, 0 ); //iedereen een lijst, zonder timestamps
}






int Answer_ServerInfo(u_char *Packet, u_long Handle,
					  u_char *hostname, u_char *gamever, u_char *hostport, u_char *password, u_char *gametype,
					  u_char *gamemode, u_char *numplayers, u_char *maxplayers, u_char *rally, u_char *stages,
					  u_char *damage, u_char *ranking, u_char *cartype) {
    int i;
    u_char *L;
    u_char *ServerInfo_clienthandle;
    u_char *ServerInfo_hostname;
	
    ServerInfo_clienthandle = Packet + 1;
    ServerInfo_hostname = Packet + 14;
	
    // packet legen vanaf ServerInfo_hostname
    for (i=14; i<250; i++) *(u_char *)(Packet+i) = 0; //zeker genoeg bytes leeg maken
	
    // fill in client request handle
    *(u_long *)(ServerInfo_clienthandle) = Handle;
	
    L = ServerInfo_hostname;
    strcpy( L, hostname );
    L = L +  strlen(hostname) + 1;
	
    strcpy( L, "gamever" );
    L = L +  strlen( "gamever" ) + 1;
    strcpy( L, gamever );
    L = L +  strlen( gamever ) + 1;
	
    strcpy( L, "hostport" );
    L = L +  strlen( "hostport" ) + 1;
    strcpy( L, hostport);
    L = L +  strlen( hostport) + 1;
	
    strcpy( L, "password" );
    L = L +  strlen( "password" ) + 1;
    strcpy( L, password );
    L = L +  strlen( password ) + 1;
	
    strcpy( L, "gametype" );
    L = L +  strlen( "gametype" ) + 1;
    strcpy( L, gametype);
    L = L +  strlen( gametype) + 1;
	
    strcpy( L, "gamemode" );
    L = L +  strlen( "gamemode" ) + 1;
    strcpy( L, gamemode);
    L = L +  strlen( gamemode) + 1;
	
    strcpy( L, "numplayers" );
    L = L +  strlen( "numplayers" ) + 1;
    strcpy( L, numplayers);
    L = L +  strlen( numplayers) + 1;
	
    strcpy( L, "maxplayers" );
    L = L +  strlen( "maxplayers" ) + 1;
    strcpy( L, maxplayers);
    L = L +  strlen( maxplayers) + 1;
	
    strcpy( L, "rally" );
    L = L +  strlen( "rally" ) + 1;
    strcpy( L, rally);
    L = L +  strlen( rally) + 1;
	
    strcpy( L, "stages" );
    L = L +  strlen( "stages" ) + 1;
    strcpy( L, stages);
    L = L +  strlen( stages) + 1;
	
    strcpy( L, "damage" );
    L = L +  strlen( "damage" ) + 1;
    strcpy( L, damage);
    L = L +  strlen( damage) + 1;
	
    strcpy( L, "ranking" );
    L = L +  strlen( "ranking" ) + 1;
    strcpy( L, ranking);
    L = L +  strlen( ranking) + 1;
	
    strcpy( L, "cartype" );
    L = L +  strlen( "cartype" ) + 1;
    strcpy( L, cartype);
    L = L +  strlen( cartype) + 1;
	
	//    fprintf( stdout, "\n lengte pakket=%d \n", L-Packet);
	// het packet bevat nog 7 nullen extra..
	//!	if (L-Packet+7 <= 182) {
	return (L-Packet+7);
	//!	} else {
	//!		return (182);
	//!	}
	
}





int Answer_MiniServerInfo(u_char *Packet, u_long Handle,
						  u_char *hostname, u_char *gamever, 
						  u_char *ranking, u_char *damage,
						  u_char *gamemode, u_char *numplayers, u_char *maxplayers) {
    int i;
    u_char *L;
    u_char *ServerInfo_clienthandle;
    u_char *ServerInfo_hostname;
	
    ServerInfo_clienthandle = Packet + 1;
    ServerInfo_hostname = Packet + 5;
	
    // packet legen en helemaal opnieuw vullen
    for (i=0; i<255; i++) *(u_char *)(Packet+i) = 0; //255 bytes is al genoeg..wordt niet groter dit packet
	
    // vul in de client-handle
    *(u_long *)(ServerInfo_clienthandle) = Handle;
	
	//de hostname
    L = ServerInfo_hostname;
    strcpy( L, hostname );
    L = L +  strlen( hostname ) + 1;
	
    strcpy( L, gamever );
    L = L +  strlen( gamever ) + 1;
	
    strcpy( L, ranking );
    L = L +  strlen( ranking ) + 1;
	
    strcpy( L, damage);
    L = L +  strlen( damage ) + 1;
	
    strcpy( L, gamemode);
    L = L +  strlen( gamemode ) + 1;
	
    strcpy( L, numplayers);
    L = L +  strlen( numplayers ) + 1;
	
    strcpy( L, maxplayers);
    L = L +  strlen( maxplayers ) + 1;
	
	return (L-Packet);
}



void Answer_LeaveLobby() {
	int		i;
	// C6 84 verzenden..
	// met daarin de land+stage aanduidingen..
	increaseChatLine(0);
	PacketLen = sizeof(LeaveLobby);
	memcpy( TXPacket, LeaveLobby, PacketLen);
	for ( i=1; i < NumPlayers; i++ ) {
		
		increaseChatLine( i );
		
		tmpUShort = getChatLine(i);
		TXPacket[3] = tmpUShort & 0xFF;
		TXPacket[4] = (tmpUShort >> 8) & 0xFF;
		
		// het land
		TXPacket[17] = GlobalStages_Land;
		// de stage
		TXPacket[18] = GlobalStages_Stage;
		
		rdcksum( TXPacket, PacketLen );
		BufferPacket( i, TXPacket, PacketLen, tmpUShort );
	}
	//BuffersResend();
}



void Server_NumberThing( u_char Nr ) {
	//	u_char NT[8] = { 0x19, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0C, 0x0C }; //orgineel
	u_char NT[8] = { 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19 }; //testje ff proberen...
	//	u_char NT[8] = { 0x19, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B }; //testje ff proberen...
	int i;

	// C6 10 zenden..
	PacketLen = sizeof(ServerNumberThing);
	memcpy( TXPacket, ServerNumberThing, PacketLen );
	for ( i=0; i < NumPlayers; i++ ) {
		
		increaseChatLine( i );
		
		// chatline / CMD inplakken..
		tmpUShort = getChatLine(i);
		TXPacket[3] = tmpUShort & 0xFF;
		TXPacket[4] = (tmpUShort >> 8) & 0xFF;
		
		// nummer overnemen
		changeStartNr( i, PlayerIndex[Nr] );
	
		// het playerNummer (0..NumPlayers) ten tijde race-start..
		changeStartPlayerNr( i, i );
		
		//@		TXPacket[9] = Nr;
		TXPacket[9] = getStartNr(i); //getStartPlayerNr(i);
		
		MsgprintyIP2(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<number1>", TXPacket[9] );
		
		TXPacket[29] = NT[Nr];
		
		rdcksum( TXPacket, PacketLen );
		BufferPacket( i, TXPacket, PacketLen, tmpUShort );
	}
	//increaseChatLine(0);
	//BuffersResend();
}

void Server_JoinNr( short PlayerNr ) { //de D6-packet ontvanger speler
	// C6 89 zenden..
	PacketLen = sizeof(ServerJoinNrC6);
	memcpy( TXPacket, ServerJoinNrC6, PacketLen );
	for ( i=1; i < NumPlayers; i++ ) {
		if ( PlayerNr != i ) {
			
			// chatline / CMD inplakken..
			increaseChatLine( i );
			tmpUShort = getChatLine(i);
			TXPacket[3] = tmpUShort & 0xFF;
			TXPacket[4] = (tmpUShort >> 8) & 0xFF;
			
			// speler JoinNr inplakken (dat is z'n ID)
			tmpULong = getID(i);
			TXPacket[9]  = tmpULong & 0xFF;
			TXPacket[10] = (tmpULong >> 8) & 0xFF;
			TXPacket[11] = (tmpULong >> 16) & 0xFF;
			TXPacket[12] = (tmpULong >> 24) & 0xFF;
			MsgprintyIP2(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<number2>", tmpULong );
			
			rdcksum( TXPacket, PacketLen );
			BufferPacket( i, TXPacket, PacketLen, tmpUShort );
		}
	}
	
	// D6 89
	if ( PlayerNr > 0 ) {
		PacketLen = sizeof(ServerJoinNrD6);
		memcpy( TXPacket, ServerJoinNrD6, PacketLen );
		//
		TXPacket[3] = LastPlayerRX3; //RXPacket[3];
		TXPacket[4] = LastPlayerRX4; //RXPacket[4];
		// chatline / CMD inplakken..
		increaseChatLine( PlayerNr );
		tmpUShort = getChatLine(PlayerNr);
		TXPacket[5] = tmpUShort & 0xFF;
		TXPacket[6] = (tmpUShort >> 8) & 0xFF;
		// speler JoinNr inplakken (dat is z'n ID)
		tmpULong = getID(PlayerNr);
		TXPacket[11]  = tmpULong & 0xFF;
		TXPacket[12] = (tmpULong >> 8) & 0xFF;
		TXPacket[13] = (tmpULong >> 16) & 0xFF;
		TXPacket[14] = (tmpULong >> 24) & 0xFF;
		rdcksum( TXPacket, PacketLen );
		MsgprintyIP2(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<number3>", tmpULong );
		BufferPacket( PlayerNr, TXPacket, PacketLen, tmpUShort );
	}
	
	//
	increaseChatLine(0);
	//BuffersResend();
}


void Server_StartCountdown( short PlayerNr ) {
	int		i;
	int		j;
	// C6 04 terugzenden..(6 bytes)
	// (iemand krijgt wel een D6  ooit......04 (8 bytes) )
	
	// de rest 2x een C6 sturen..met oplopende ChatLine
	// 1e keer..
	PacketLen = sizeof(ServerStartCountdownC6);
	memcpy( TXPacket, ServerStartCountdownC6, PacketLen);
	for ( j=0; j<2; j++ ) {
		//		for ( i=1; i < NumPlayers; i++ ) {
		//*			if ( LastPlayerNr>0 && LastPlayerNr!=i ) {
		//			if ( i == PlayerNr ) {
		i = PlayerNr;
		increaseChatLine( i );
		
		tmpUShort = getChatLine(i);
		TXPacket[3] = tmpUShort & 0xFF;
		TXPacket[4] = (tmpUShort >> 8) & 0xFF;
		
		rdcksum( TXPacket, PacketLen );
		BufferPacket( i, TXPacket, PacketLen, tmpUShort );
		//SendToPlayer( i, PacketLen );
		//SendToPlayer( i, PacketLen );
		//*			}
		//		}
		increaseChatLine(0);
	}
	/*
	// D6.. aan de laatste die ready is.
	if ( LastPlayerNr > 0 ) {
	PacketLen = sizeof(ServerStartCountdownD6);
	memcpy( TXPacket, ServerStartCountdownD6, PacketLen);
	
	  TXPacket[3] = LastPlayerRX3; //RXPacket[3];
	  TXPacket[4] = LastPlayerRX4; //RXPacket[4];
	  
		increaseChatLine( LastPlayerNr );
		tmpUShort = getChatLine(LastPlayerNr);
		TXPacket[5] = tmpUShort & 0xFF;
		TXPacket[6] = (tmpUShort >> 8) & 0xFF;
		
		  rdcksum( TXPacket, PacketLen );
		  BufferPacket( LastPlayerNr, TXPacket, PacketLen, tmpUShort );
		  }
	*/
	//BuffersResend();
	
	/*   /// net uitgezet !!!
	
	  // iedereen de host in-game positie toesturen..
	  PacketLen = sizeof(PosUSA5);
	  memcpy( TXPacket, PosUSA5, PacketLen);
	  
		//memcpy( TXPacket+3, ptr_players[0].PosRec[0], 27 );
		// host tijd erin..
		tmpULong = minisec();  //0x00000000;
		TXPacket[5] = tmpULong & 0xFF;
		TXPacket[6] = (tmpULong>>8) & 0xFF;
		TXPacket[7] = (tmpULong>>16) & 0xFF;
		TXPacket[8] = (tmpULong>>24) & 0xFF;
		//
		for ( i=1; i < NumPlayers; i++ ) {
		rdcksum( TXPacket, PacketLen );
		SendToPlayer( i, PacketLen );
		}
	*/
	/*
	// alle PosSent[]'s op 1 (al verzonden/"verwerkt")..
	for ( i=0; i < MaxPlayers; i++ ) {
	for ( j=0; j < MaxPlayers; j++ ) {
	ptr_players[i].PosSent[j] = 1;
	}
	}
	*/
}


void Server_PingList( short PlayerNr, int withTimestamp ) {
	u_long	tmpL;
	int		i;
	int		Offset;
	Offset = (withTimestamp==0)? 0: 5;
	if ( withTimestamp==0 ) {
		PacketLen = sizeof(ServerPingList);
		memcpy( TXPacket, ServerPingList, PacketLen);
	} else {
		PacketLen = sizeof(ServerPingListTimestamp);
		memcpy( TXPacket, ServerPingListTimestamp, PacketLen);
		tmpL = minisec();
		TXPacket[4] = tmpL & 0xFF;
		TXPacket[5] = (tmpL>>8) & 0xFF;
		TXPacket[6] = (tmpL>>16) & 0xFF;
		TXPacket[7] = (tmpL>>24) & 0xFF;
	}
	// het aantal in deze lijst
	TXPacket[4+Offset] = NumPlayers;
	// de ID's
	for ( i=0; i < NumPlayers; i++ ) {
		//tmpL = getStartPlayerNr(i);
		//if ( tmpL == 0 ) tmpL = getID(i);
		tmpL = getID(i);
		memcpy( TXPacket+Offset+7+(i*4), &tmpL, 4);
	}
	// de Pings
	for ( i=0; i < NumPlayers; i++ ) {
		//		tmpL = getPing(i);
		tmpL = getAvgPing(i);
		memcpy( TXPacket+Offset+7+(8*4)+(i*4), &tmpL, 4);
	}
	rdcksum( TXPacket, PacketLen );
	
	if ( PlayerNr == -1 ) {
		// naar iedereen sturen..
		for ( i=1; i < NumPlayers; i++ ) {
			Send1ToPlayer( i, PacketLen );
		}
	} else {
		// alleen naar PlayerNr sturen..
		Reply1ToPlayer( PacketLen );
	}
}

void Server_GridPresent1() {
	int		i;
	u_short	CMD;
	// Een C6-0E zenden naar iedereen..
	// met het PlayerNr van de host erin..want die is nu present.
	for ( i=1; i < NumPlayers; i++ ) {
		PacketLen = sizeof(ServerGridPresent1);
		memcpy( TXPacket, ServerGridPresent1, PacketLen);
		// ChatLine
		increaseChatLine( i );
		CMD = getChatLine(i);
		TXPacket[3] = tmpUShort & 0xFF;
		TXPacket[4] = (tmpUShort >> 8) & 0xFF;
		// PlayerNr van de host..
		TXPacket[6] = 0;
		//memcpy( TXPacket+21, RXPacket+21, 16);
		// checksum
		rdcksum( TXPacket,PacketLen );
		BufferPacket( i, TXPacket, PacketLen, CMD );
		
		/*
		// meteen ook een ServerGridPresent2 sturen..
		// Een C6-0D zenden naar iedereen..
		PacketLen = sizeof(ServerGridPresent2);
		memcpy( TXPacket, ServerGridPresent2, PacketLen);
		// ChatLine
		increaseChatLine( i );
		CMD = getChatLine(i);
		TXPacket[3] = tmpUShort & 0xFF;
		TXPacket[4] = (tmpUShort >> 8) & 0xFF;
		// PlayerNr van de host..
		TXPacket[6] = 0;
		// checksum
		rdcksum( TXPacket,PacketLen );
		BufferPacket( i, TXPacket, PacketLen, CMD );
		*/
	}
	//BuffersResend();
}

void Server_GridPresent1Player(u_char PlayerNr) {
	int		i;
	u_short	CMD;
	// Een C6-0E zenden naar iedereen..
	// met het PlayerNr van de host erin..want die is nu present.
	for ( i=3; i < NumPlayers; i++ ) {
		PacketLen = sizeof(ServerGridPresent1);
		memcpy( TXPacket, ServerGridPresent1, PacketLen);
		// ChatLine
		increaseChatLine( i );
		CMD = getChatLine(i);
		TXPacket[3] = tmpUShort & 0xFF;
		TXPacket[4] = (tmpUShort >> 8) & 0xFF;
		// PlayerNr van de host..
		TXPacket[6] = PlayerNr;
		//memcpy( TXPacket+21, RXPacket+21, 16);
		// checksum
		rdcksum( TXPacket,PacketLen );
		SendToPlayer( i, PacketLen );
		//BufferPacket( i, TXPacket, PacketLen, CMD );
		
		/*
		// meteen ook een ServerGridPresent2 sturen..
		// Een C6-0D zenden naar iedereen..
		PacketLen = sizeof(ServerGridPresent2);
		memcpy( TXPacket, ServerGridPresent2, PacketLen);
		// ChatLine
		increaseChatLine( i );
		CMD = getChatLine(i);
		TXPacket[3] = tmpUShort & 0xFF;
		TXPacket[4] = (tmpUShort >> 8) & 0xFF;
		// PlayerNr van de host..
		TXPacket[6] = 0;
		// checksum
		rdcksum( TXPacket,PacketLen );
		BufferPacket( i, TXPacket, PacketLen, CMD );
		*/
	}
	//BuffersResend();
}


void Server_GridReady() {
	int		i;
	u_short	CMD;
	// Een C6-03 terugsturen naar de rest..
	// met de ID van de host erin..want die is nu ready.
	for ( i=1; i < NumPlayers; i++ ) {
		PacketLen = sizeof(GridReadyC6);
		memcpy( TXPacket, GridReadyC6, PacketLen);
		// ChatLine
		increaseChatLine( i );
		tmpUShort = getChatLine(i);
		TXPacket[3] = tmpUShort & 0xFF;
		TXPacket[4] = (tmpUShort >> 8) & 0xFF;
		//
		TXPacket[6] = 0x00;
		TXPacket[7] = 0x00;
		TXPacket[8] = 0x00;
		TXPacket[9] = 0x00;	 // PlayerNr 
		TXPacket[10] = 0x00;
		TXPacket[11] = 0x00;
		TXPacket[12] = 0x00;
		// checksum
		rdcksum( TXPacket,PacketLen );
		
		//SendToPlayer( i, PacketLen );
		CMD = getChatLine( i );
		BufferPacket( i, TXPacket, PacketLen, CMD );
	}
	//BuffersResend();
	
	// de host zelf..
	increaseChatLine(0);
}


void Server_GridReadyPlayer( u_char PlayerNr) {
	int		i;
	u_short	CMD;
	// Een C6-03 terugsturen naar de rest..
	// met de ID van de host erin..want die is nu ready.
	for ( i=1; i < NumPlayers; i++ ) {
		PacketLen = sizeof(GridReadyC6);
		memcpy( TXPacket, GridReadyC6, PacketLen);
		// ChatLine
		increaseChatLine( i );
		tmpUShort = getChatLine(i);
		TXPacket[3] = tmpUShort & 0xFF;
		TXPacket[4] = (tmpUShort >> 8) & 0xFF;
		//
		TXPacket[6] = 0x00;
		TXPacket[7] = 0x00;
		TXPacket[8] = 0x00;
		TXPacket[9] = PlayerNr;	 // PlayerNr 
		TXPacket[10] = 0x00;
		TXPacket[11] = 0x00;
		TXPacket[12] = 0x00;
		// checksum
		rdcksum( TXPacket,PacketLen );
		
		//SendToPlayer( i, PacketLen );
		CMD = getChatLine( i );
		BufferPacket( i, TXPacket, PacketLen, CMD );
	}
	//BuffersResend();
	
	// de host zelf..
	increaseChatLine(0);
}



void Server_ReplyTimestamp() {
	// zend een 40-02 terug, met de host time-stamp
	PacketLen = sizeof(OwnTimeStamp);				
	memcpy( TXPacket, OwnTimeStamp, PacketLen);
	// host timestamp erin plakken..
	tmpULong = minisec();	//time(NULL);
	
	TXPacket[4] = tmpULong & 0xFF; 
	TXPacket[5] = (tmpULong>>8) & 0xFF;
	TXPacket[6] = (tmpULong>>16) & 0xFF;
	TXPacket[7] = (tmpULong>>24) & 0xFF;
	rdcksum( TXPacket,PacketLen );
	ReplyToPlayer( PacketLen );
}





void Server_SendPos() {
	int		i,j,k;
	int		Offset=8;
	int		PlayerNr=0;
	int		lp;
	u_long	Time;
	u_char	tmpChar;
	/*
	short	pnr=0;
	int		highest;
	*/
	if ( GlobalState < stateRacing ) return;  // was 7 later 9
	
											  /*
											  //test
											  //zoek de speler met de hoogste begin-tijd (de tijd in het eerst ontvangen positie-packet)..
											  highest = 0;
											  for (i=1;i<NumPlayers;i++){
											  //	if (ptr_players[PlayerIndex[i]].StartTime>highest) {
											  if (getRaceTime(i)>highest) {
											  //		highest = ptr_players[PlayerIndex[i]].StartTime;
											  highest = getRaceTime(i);
											  pnr=i;
											  }
											  }
	*/
	
	
	// deze speler een lijst terugsturen met de Pos van de anderen..
	// Dat is ook een 42, maar dan dikker..
	PacketLen = Offset + 27 * (MaxPlayers-1); // lengte is niet correct (maar altijd lang genoeg)..
	// een ServerPos overnemen..
	memcpy( TXPacket, ServerPos, PacketLen);
	// de host-pos erin plakken
	//	memcpy( TXPacket+Offset, PosUSA5+3, 27);
	//	TXPacket[] // racetijd aanpassen..(huidige tijd-racestarttijd)
	//
	
	
	for ( PlayerNr=0; PlayerNr < NumPlayers; PlayerNr++ ) {
		
	/*
	if ( PlayerNr != 0 ) {
	if ( ptr_players[PlayerNr].PosCount < 18000 ) {
				memcpy( &ptr_players[PlayerNr].PosRec[27*ptr_players[PlayerNr].PosCount++], &ptr_players[PlayerNr].LastPos, 27 );
				
				  //memcpy( &ptr_players[0].LastPos, &ptr_players[PlayerNr].LastPos, 27 );
				  }
				  //memcpy( TXPacket+Offset, &ptr_players[0].LastPos, 27);
				  //*(TXPacket+Offset + 1) = 0;
				  //tmpInt++;
				  //Offset += 27;
				  
					} else {
		*/
		/*
		if ( ptr_players[0].PosCount < 18000 ) {
		memcpy( &ptr_players[0].LastPos, &ptr_players[0].PosRec[27*ptr_players[0].PosCount++],  27 );
		ptr_players[0].LastPos[1] = 0;
		tmpULong = ptr_players[0].LastPos[2]+(ptr_players[0].LastPos[3]<<8)+(ptr_players[0].LastPos[4]<<16)+(ptr_players[0].LastPos[5]<<24);
		changeRaceTime( 0, tmpULong );
		//ptr_players[0].LastPos[26] = 0;
		//memcpy( &ptr_players[0].LastPos, &ptr_players[PlayerNr].LastPos, 27 );
		}
		*/
		
		//		}
		
		// test of de ontvanger, speler(PlayerNr), al was begonnen met zelf posities te zenden..
		// anders ook niet posities terugsturen..
		if ( PlayerNr!=0 && ptr_players[PlayerIndex[PlayerNr]].PosReceived>0 ) { //|| PlayerNr == 0 ) {						
			// de LastPos van de rest erin plakken..
			tmpInt = 0; //teller aantal vermeld..(van de host alleen nog)
			Offset = 8;
			for ( i=0; i < NumPlayers; i++ ) {
				if ( PlayerNr != i ) {
					
					// kijk of de laatste positie al is verzonden aan deze speler(PlayerNr)
					//%					if ( ptr_players[PlayerIndex[i]].PosSent[getStartNr(PlayerNr)]==0 ) {						
					
					// check of de racetime van speler(i) < die van speler(PlayerNr)
					//$						if ( getRaceTime(i) >= getRaceTime(PlayerNr) ) {
					
					// aanvinken dat ie verstuurd is...
					//%							ptr_players[PlayerIndex[i]].PosSent[getStartNr(PlayerNr)] = 1;
					
					
					
					//1 orgineel				memcpy( TXPacket+Offset, &ptr_players[PlayerIndex[i]].LastPos, 27);
					
					if ( i!=0 && ptr_players[PlayerIndex[i]].PosReceived>0 ) {
						
						//	if ( (ptr_players[PlayerIndex[PlayerNr]].StartTime > ptr_players[PlayerIndex[i]].StartTime) ) {
						/**					if ( getRaceTime(PlayerNr) > getRaceTime(i) ) {
						//	if ( PlayerNr == pnr ) {
						// knipperen van auto's proberen te verwijderen.. beetje proberen hier..
						//@									if ( ptr_players[PlayerIndex[i]].LastPos[10] != 0x00 ) {
						// gebruik LastPos, niet de gebufferde LastPos..
						memcpy( TXPacket+Offset, &ptr_players[PlayerIndex[i]].LastPos, 27);
						//Time = ptr_players[PlayerIndex[i]].LastPos[2];
						//Time += (ptr_players[PlayerIndex[i]].LastPos[3] << 8);
						//Time += (ptr_players[PlayerIndex[i]].LastPos[4] << 16);
						//Time += (ptr_players[PlayerIndex[i]].LastPos[5] << 24);
						//ptr_players[PlayerIndex[i]].LastPosSentTime[PlayerIndex[PlayerNr]] = Time;
						// 
						tmpInt++;
						Offset += 27;
						//@									}
						} else {
						**/
						// de timestamp van speler(PlayerNr)
						Time = getRaceTime(PlayerNr);
						//Time -= getPing(PlayerNr);
						//Time -= (ptr_players[PlayerIndex[PlayerNr]].StartTime-ptr_players[PlayerIndex[i]].StartTime); //testje
						lp = GetLastPos( i, Time );
						if ( lp != -1 ) {
							Time = ptr_players[PlayerIndex[i]].LastPosBuf[lp][2];
							Time += (ptr_players[PlayerIndex[i]].LastPosBuf[lp][3] << 8);
							Time += (ptr_players[PlayerIndex[i]].LastPosBuf[lp][4] << 16);
							Time += (ptr_players[PlayerIndex[i]].LastPosBuf[lp][5] << 24);
							if (Time > ptr_players[PlayerIndex[i]].LastPosSentTime[PlayerIndex[PlayerNr]]) {
								ptr_players[PlayerIndex[i]].LastPosSentTime[PlayerIndex[PlayerNr]] = Time;
								memcpy( TXPacket+Offset, &ptr_players[PlayerIndex[i]].LastPosBuf[lp], 27);
								// 
								tmpInt++;
								Offset += 27;
							}
						}
						//**				}
					}
					if ( i == 0 ) {
						//memcpy( &ptr_players[0].LastPos, &ptr_players[0].PosRec[ptr_players[0].PosCount++], 27);
						//ptr_players[0].LastPos[1] = 0;
						//ptr_players[0].LastPos[26] = 0;
						// host tijd erin..
						//tmpULong = minisec();  //0x00000000;
						//ptr_players[0].LastPos[2] = tmpULong & 0xFF;
						//ptr_players[0].LastPos[3] = (tmpULong>>8) & 0xFF;
						//ptr_players[0].LastPos[4] = (tmpULong>>16) & 0xFF;
						//ptr_players[0].LastPos[5] = (tmpULong>>24) & 0xFF;
						Time = getRaceTime(PlayerNr);


						/*tmpChar = getVoteGhost(PlayerNr);
						if ( tmpChar==0 ) tmpChar = 1;
						tmpChar = tmpChar-1 + 8;*/
						tmpChar = getVoteGhost(PlayerNr) + 8;
						//								tmpChar = getCarType(PlayerNr) + 8;

						// check of de replay-mode actief is..
						if ( GlobalConfigMode==1 && GlobalReplayMode==1 ) {
							j = (ptr_players[PlayerIndex[PlayerNr]].LastPos[25] << 8) + ptr_players[PlayerIndex[PlayerNr]].LastPos[24];
							if (GlobalEndPos>0) {
								// het percentage gereden van deze stage afbeelden
								j = (j * 100.0) / (GlobalEndPos * 100.0) * 100;
								if ( j>100 ) j = 100;
							}

							//	
							// op CurrentFrame = 0 : => uitrekenen welke positie .. 
							//						 => onthouden tijd van player en ghost
							// op CurrentFrame > 0 : => uitrekenen huidige tijd player - tijd van toen
							//						 => dan opvragen frame van tijd van ghost bij start + verschil
							//NOG NIE AF HELEMAAL / NOG TESTEN

							if ( GlobalReplayFrame == 0 ) {
								// loop-begin framenummer van de host
								j-=2; if(j<0)j=0; // beetje terugzetten op de baan (2%)
								tmpULong = (( j * ptr_players[tmpChar].PosCount ) / 100 );
								if ( tmpULong > 100 ) tmpULong -= 100;

								GlobalReplayHostTime = (u_long*)ptr_players[tmpChar].PosRec+ 27*tmpULong + 2;
								GlobalReplayPlayerTime = Time;
							} else {
								tmpTime = GlobalReplayHostTime + (Time - GlobalReplayPlayerTime);
								tmpULong = GetLastPosRec( tmpTime, tmpChar, PlayerNr );
							}
/*@
							//
							//Time = ((j * (ptr_players[tmpChar].RaceTime+5000))/100);
							//tmpULong = GetLastPosRec( Time, tmpChar, PlayerNr );
							tmpULong = (( j * ptr_players[tmpChar].PosCount ) / 100 );
							if ( tmpULong > 100 ) tmpULong -= 100;
*/
							GlobalReplayFrame++;
							if (GlobalReplayFrame>GlobalReplayFrames) GlobalReplayFrame = 0;
/*@
							tmpULong += GlobalReplayFrame;
*/
						} else {
							// niet in replay-mode..
							tmpULong = GetLastPosRec( Time, tmpChar, PlayerNr );
						}

						// intermediate verzenden van BOTs aan speler
						if ( GlobalSendSplits == 1 ) {
							for (k=0;k<4;k++) {
								if ( ptr_players[PlayerIndex[PlayerNr]].PosSplitSent[k] == 0 && Time > ptr_players[tmpChar].SplitTime[k] ) {
									SendSplit( PlayerNr, 0, (u_char) k, ptr_players[tmpChar].SplitTime[k] );
									ptr_players[PlayerIndex[PlayerNr]].PosSplitSent[k] = 1;
								}
							}
						}

						
						//sprintf( log, "tc:%d LPS:%d T:%d LPR:%d PC:%d", tmpChar, ptr_players[8].LastPosRecSent[PlayerNr],  Time, tmpULong, ptr_players[8].PosCount  );
						//MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<pos>", log);
						
						memcpy( TXPacket+Offset, &ptr_players[tmpChar].PosRec[27*tmpULong],  27 );
						TXPacket[1+Offset] = 0;
						
						if ( GlobalConfigMode==1 && GlobalReplayMode==1 ) {
							memcpy( TXPacket+Offset+2, &Time, 4 );
						}

						memcpy( &ptr_players[0].LastPos, TXPacket+Offset, 27 ); 
						//if ( PlayerNr == 1 ) {
						//memcpy( &ptr_players[0].LastPos, TXPacket+Offset,  27 );
						//memcpy( &ptr_players[0].LastPos, &ptr_players[0].PosRec[27*tmpULong],  27 );
						//ptr_players[0].LastPos[1] = 0;
						tmpULong = ptr_players[0].LastPos[2]+(ptr_players[0].LastPos[3]<<8)+(ptr_players[0].LastPos[4]<<16)+(ptr_players[0].LastPos[5]<<24);
						changeRaceTime( 0, tmpULong );
						//}
						/*
						
						  // check of de record-houders cutten..
						  if ( GlobalNoCutsEnabled == 1 && GlobalState == stateRacing ) {
						  for ( i=8; i<12; i++)  {
						  float PosX = (float)ptr_players[i].LastPos[12];
						  float PosY = (float)ptr_players[i].LastPos[16];
						  float PosZ = (float)ptr_players[i].LastPos[20];
						  if ( CheckNoCut(i, PosX,PosY,PosZ) == 1 ) {
						  sprintf( log, "%s", getNick(i) );
						  MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<cut-h>", log);
						  sprintf( log, "<cut-h> %s", getNick(i) );
						  Log( log );
						  }
						  changeLastPosX(i, PosX);
						  changeLastPosY(i, PosY);
						  changeLastPosZ(i, PosZ);
						  }
						  }
						*/
						//memcpy( TXPacket + Offset, &ptr_players[0].LastPos, 27);
						tmpInt++;
						Offset += 27;
						//sprintf( log, "<pos0plak>" );
						//Log( log );
						
					}
					
					if ( GlobalConfigMode == 1 && ( PlayerNr >= 3 ) ) {
						
						Time = getRaceTime(PlayerNr);
						changeRaceTime( i, Time );
						memcpy( &ptr_players[PlayerIndex[i]].LastPos[2], &Time, 4 );
						ptr_players[1].LastPos[1] = 1;
						ptr_players[2].LastPos[1] = 2;
						
						//								ptr_players[2].LastPos[25] = 0;
						//								ptr_players[2].LastPos[24] = 1;
						//ptr_players[PlayerIndex[i]].LastPos[3] = (Time << 8);
						//Time += (ptr_players[PlayerIndex[i]].LastPos[4] << 16);
						//Time += (ptr_players[PlayerIndex[i]].LastPos[5] << 24);
						//if (Time > ptr_players[PlayerIndex[i]].LastPosSentTime[PlayerIndex[PlayerNr]]) {
						memcpy( TXPacket+Offset, &ptr_players[PlayerIndex[i]].LastPos, 27);
						// 
						tmpInt++;
						Offset += 27;
						
						
					}
					
					///1
					
					
					//$						}
					
					//%					}
					
				}
			}
			// tenminste 1 speler in de lijst opgenomen??
			if ( tmpInt > 0 ) {
				// correctie packetlen
				PacketLen = 8 + 27 * tmpInt;
				// 
				//!				TXPacket[3] == 0x06 
				// aantal vermeldde posities van andere spelerts..
				TXPacket[4] = tmpInt & 0xFF; // aantal ingevulde posities
				// checksum
				rdcksum( TXPacket,PacketLen );
				if ( PlayerNr != 0 ) {
					//sprintf( log, "<PosCount>" );
					//for ( i=0; i<PacketLen; i++ ) {
					//	sprintf( log, "%s %x", log, TXPacket[i] );
					//}
					//Log( log );
					
					Send1ToPlayer( PlayerNr, PacketLen );
				}
				//				SendToPlayer( PlayerNr, PacketLen );
			}
			
		}
	}
}



void KickPlayer( short PlayerNr, u_char* aMessage ) {
	// S->cl. (2x)  C6 xx xx xx xx 88
	// S->cl.       CA
	// S->(all-cl.) Playerlist
	// S->(all-cl.) C6 xx xx xx xx 8B       <-8B is nieuw voor ons
	//              (met chat erin: playername has been kicked out)
	// cl.->S       DA
	int		i=0;
	int		j;
	u_short	CMD;
	

	if ( strlen(aMessage)>0 && PlayerNr>0 ) {
		//--- een C6-8B naar de rest..
		PacketLen = sizeof(ChatFromHostToAny);
		memcpy( TXPacket, ChatFromHostToAny, PacketLen);
		
		// nickname opzoeken en plakken
		strcpy( (u_char *) ChatFromHostToAny_Nick, "KICKED" );
		//strncpy( (u_char *) ChatFromHostToAny_Nick,3, (u_char *)"UJE" );
		
		// de chat tekst uit het ontvangen pakketje overnemen
		//L = strlen((u_char *) ChatFromHostToAny_Nick);
		//LMsg = strlen((u_char *) aMessage);
		strcpy( ChatFromHostToAny_Nick + 6 +1, aMessage );
		// de pakket-lengte berekenen..(na chatText+0 afkappen)
		PacketLen = 37 + 6 + 1 + strlen(aMessage) +1;
		
		
		increaseChatLine( PlayerNr );
		
		tmpUShort = getChatLine(PlayerNr);
		TXPacket[3] = tmpUShort & 0xFF;
		TXPacket[4] = (tmpUShort >> 8) & 0xFF;
		
		tmpULong =  getID(0);		// wie zei het ?
		TXPacket[9] = tmpULong & 0xFF;
		TXPacket[10] = (tmpULong >> 8) & 0xFF;
		TXPacket[11] = (tmpULong >> 16) & 0xFF;
		TXPacket[12] = (tmpULong >> 24) & 0xFF;
		
		rdcksum( TXPacket, PacketLen );
		SendToPlayer( PlayerNr, PacketLen );
		//BufferPacket( playerNumber, TXPacket, PacketLen, tmpUShort );
		
		//increaseChatLine( 0 ); //host
	}
	
	sleep(0);
	sleep(0);
	
	PacketLen = sizeof(ServerKickPlayer);				
	memcpy( TXPacket, ServerKickPlayer, PacketLen);
	// ChatLine
	increaseChatLine( PlayerNr );
	CMD = getChatLine( PlayerNr );
	TXPacket[3] = CMD & 0xFF;
	TXPacket[4] = (CMD >> 8) & 0xFF;
	// speler JoinNr inplakken (dat is z'n ID)
	tmpULong = getID(PlayerNr);
	TXPacket[9]  = tmpULong & 0xFF;
	TXPacket[10] = (tmpULong >> 8) & 0xFF;
	TXPacket[11] = (tmpULong >> 16) & 0xFF;
	TXPacket[12] = (tmpULong >> 24) & 0xFF;
	//
	rdcksum( TXPacket, PacketLen );
	SendToPlayer( PlayerNr, PacketLen );
	//BufferPacket( PlayerNr, TXPacket, PacketLen, CMD );
	//BuffersResend();
	// de hosts' chatline..
	increaseChatLine(0);
	
	//============================
	// iemand verlaat het spel
	PacketLen = sizeof(Goner);
	memcpy( TXPacket, Goner, PacketLen);
	// ChatLine overnemen..
	increaseChatLine( PlayerNr );
	tmpUShort = getChatLine( PlayerNr );
	TXPacket[3] = tmpUShort & 0xFF;
	TXPacket[4] = (tmpUShort >> 8) & 0xFF;
	//ReplyToPlayer( PacketLen );
	SendToPlayer( PlayerNr, PacketLen );
	
	
	// playerlist naar de rest
	//@	SendAllPlayerList( -1 );//zit al in removeplayer
	//============================
	
	
	
	
	//-------------
	// de andere spelers een C6-08 sturen..
	PacketLen = sizeof(ServerQuit);
	memcpy( TXPacket, ServerQuit, PacketLen);
	
	// playerNumber van deze speler
	if ( GlobalState < stateSwitch  ) { //changes: 2008apr30
		//	if ( GlobalState <= stateNumbering  ) {								
		TXPacket[6] = PlayerNr; //PlayerNr ;//getID(PlayerNr);  ///ptr_players[ PlayerIndex[PlayerNr] ].ID ;
	} else {
		TXPacket[6] = getStartPlayerNr(PlayerNr); //PlayerNr ;//getID(PlayerNr);  ///ptr_players[ PlayerIndex[PlayerNr] ].ID ;
	}
	// de speler verwijderen, en alle variabelen bijwerken..
	//RemovePlayer( PlayerNr );
	
	//!!					for ( j=1; j < MaxPlayers; j++ ) {
	//!!						if ( PlayerIndex[PlayerNr] != j  && PlayerIndex[j] != -1) {
	for ( j=1; j < NumPlayers; j++ ) {
		if ( PlayerNr != j ) {
			// CMD
			
			increaseChatLine( j );
			tmpUShort = getChatLine(j);
			//tmpUShort = getLastCMD(j);
			//changeLastCMD( j, tmpUShort+1 ); //nieuwe waarde onthouden..
			TXPacket[3] = tmpUShort & 0xFF;
			TXPacket[4] = (tmpUShort >> 8) & 0xFF;
			//
			rdcksum( TXPacket,PacketLen );
			BufferPacket( j, TXPacket, PacketLen, tmpUShort );// heb ik veranderd
			//SendToPlayer( j, PacketLen );//, tmpUShort );
		}
	}
	DisplayBuffers();
	//BuffersResend();
	//-------------
	
	changeState( PlayerNr, 0 );
	changeQuit( PlayerNr, 1);
	GlobalPlayersQuit++;
	
	// de speler verwijderen, en alle variabelen bijwerken..
	RemovePlayer( PlayerNr, (GlobalState<stateSwitch)?1:0 );
//	RemovePlayer( PlayerNr );
}


void Server_RetireDriver( u_char PlayerNr ) {
	int		i=0;
	u_short	CMD;
	PacketLen = sizeof(ServerRetire);				
	memcpy( TXPacket, ServerRetire, PacketLen);
	// nog niet over de finish?? dan rijdt de speler nog..
	if ( getFinished(PlayerNr)==0 ) {
		// indien nog niet retired..deze speler retiren..
		if ( getRetired(PlayerNr)==0 ) {
			
			// deze speler .hasRetired bijwerken..
			changeRetired( PlayerNr, 1 );
			// het aantal spelers retired tot dusver..
			GlobalPlayersRetired++;
			
			// CMD ophogen..
			changeLastCMD( PlayerNr, getLastCMD(PlayerNr)+1 );
			
			// de andere spelers laten weten van de retire van speler(PlayerNr)
			for (i=1; i<NumPlayers; i++) {
				// ChatLine
				increaseChatLine( i );
				CMD = getChatLine(i);
				TXPacket[3] = CMD & 0xFF;
				TXPacket[4] = (CMD >> 8) & 0xFF;
				// PlayerNr
				//!					TXPacket[6] = ptr_players[PlayerIndex[PlayerNr]].LastPos[1]; //het PlayerNr dat deze speler zelf gebruikt..
				TXPacket[6] = getStartPlayerNr(PlayerNr); //het PlayerNr dat deze speler zelf gebruikt..
				//
				rdcksum( TXPacket, PacketLen );
				BufferPacket( i, TXPacket, PacketLen, CMD );
			}
			// de hosts' chatline..
			increaseChatLine(0);
		}
	}
}

void Server_RetireDrivers() {
	int		i=0;
	u_short	CMD;
	u_char	PlayerNr;
	PacketLen = sizeof(ServerRetire);				
	memcpy( TXPacket, ServerRetire, PacketLen);
	// retire alle nog rijdende spelers
	for ( PlayerNr=1; PlayerNr<NumPlayers; PlayerNr++ ) {
		// nog niet over de finish?? dan rijdt de speler nog..
		if ( getFinished(PlayerNr)==0 ) {
			// indien nog niet retired..deze speler retiren..
			if ( getRetired(PlayerNr)==0 ) {
				
				// deze speler .hasRetired bijwerken..
				changeRetired( PlayerNr, 1 );
				// het aantal spelers retired tot dusver..
				GlobalPlayersRetired++;
				
				// CMD ophogen..
				changeLastCMD( PlayerNr, getLastCMD(PlayerNr)+1 );
				
				// de andere spelers laten weten van de retire van speler(PlayerNr)
				for (i=1; i<NumPlayers; i++) {
					// ChatLine
					increaseChatLine( i );
					CMD = getChatLine(i);
					TXPacket[3] = CMD & 0xFF;
					TXPacket[4] = (CMD >> 8) & 0xFF;
					// PlayerNr
					//!					TXPacket[6] = ptr_players[PlayerIndex[PlayerNr]].LastPos[1]; //het PlayerNr dat deze speler zelf gebruikt..
					TXPacket[6] = getStartPlayerNr(PlayerNr); //het PlayerNr dat deze speler zelf gebruikt..
					//
					rdcksum( TXPacket, PacketLen );
					BufferPacket( i, TXPacket, PacketLen, CMD );
				}
				// de hosts' chatline..
				increaseChatLine(0);
				/*
				//=============================================
				// wordt 2 keer gestuurd met ophogende CMD/chatnummers erin
				// CMD ophogen..
				changeLastCMD( PlayerNr, getLastCMD(PlayerNr)+1 );
				
				  // de andere spelers laten weten van de retire van speler(PlayerNr)
				  for (i=1; i<NumPlayers; i++) {
				  // ChatLine
				  increaseChatLine( i );
				  CMD = getChatLine(i);
				  TXPacket[3] = CMD & 0xFF;
				  TXPacket[4] = (CMD >> 8) & 0xFF;
				  // PlayerNr
				  //!					TXPacket[6] = ptr_players[PlayerIndex[PlayerNr]].LastPos[1]; //het PlayerNr dat deze speler zelf gebruikt..
				  TXPacket[6] = getStartPlayerNr(PlayerNr); //het PlayerNr dat deze speler zelf gebruikt..
				  //
				  rdcksum( TXPacket, PacketLen );
				  BufferPacket( i, TXPacket, PacketLen, CMD );
				  }
				  // de hosts' chatline..
				  increaseChatLine(0);
				  //=============================================
				*/
				
			}
		}
	}
	//BuffersResend();
	//
	//CheckRetireHost();
}

void Server_Retire() {
	int		i=0;
	u_short	CMD;
	PacketLen = sizeof(ServerRetire);				
	memcpy( TXPacket, ServerRetire, PacketLen);
	for (i=1; i<NumPlayers; i++) {
		// ChatLine
		increaseChatLine( i );
		CMD = getChatLine(i);
		TXPacket[3] = CMD & 0xFF;
		TXPacket[4] = (CMD >> 8) & 0xFF;
		rdcksum( TXPacket, PacketLen );
		BufferPacket( i, TXPacket, PacketLen, CMD );
	}
	//BuffersResend();
	// de hosts' chatline..
	increaseChatLine(0);
}

void CheckRetireHost() {
	int		i,j=0;
	int		WinnerIndex;
	u_char	timeString[256];
	u_long	WinnerID;
	//	u_long	FastestTime;
	u_long	MemberID;
	u_char	Car;
	u_char	Gearbox;
	u_long	Split0;
	u_long	Split1;
	u_long	Split2;
	u_long	RaceTime;
	u_char*	IP;
	u_char*	Nick;
	u_char*	Country;
	u_char	AllDone;
	if ( GlobalState != stateRacing ) return;  // was 7 later 9
	
	// Allemaal over de finish of allemaal retired??
	// dan de host zelf ook laten afgaan..
	// GlobalPlayersQuit spelers zijn al weg, (NumPlayers is al aangepast indien "openwaiting")
	// GlobalPlayersRetired spelers zitten er nog in, maar hebben opgegeven..
	// GlobalPlayersFinished spelers zijn over de finish gekomen..
	//	if ( GlobalPlayersFinished + GlobalPlayersRetired + GlobalPlayersQuit + 1 == NumPlayers ) {
	//	GlobalPlayersQuit = 0;
	/*
	GlobalPlayersRetired = 0;
	GlobalPlayersFinished = 0;
	for ( i=1; i<NumPlayers; i++ ) {
	//		if ( getQuit( i ) == 1 ) GlobalPlayersQuit++;
	if ( getRetired( i ) != 0 ) GlobalPlayersRetired++;
	if ( getFinished( i ) != 0 ) GlobalPlayersFinished++;
	}
	//DisplayStatus();
	*/
	AllDone=1;
	if ( GlobalConfigMode==1 ) {
		j=3;
	} else {
		j=1;
	}
	for ( i=j; i<NumPlayers; i++ ) {
		if ( getFinished(i) == 0 && getRetired(i) == 0 ) {
			AllDone = 0;
			break;
		}	
	}
	
	if ( AllDone == 1 ) {
		//	if ( (GlobalConfigMode == 0 && GlobalPlayersFinished+GlobalPlayersRetired+1 == NumPlayers) ||
		//		  GlobalConfigMode == 1 && GlobalPlayersFinished+GlobalPlayersRetired+3 == NumPlayers ) {
		
		sprintf( log, "%d players, %d finished, %d retired, %d quiters", (NumPlayers-1), GlobalPlayersFinished, GlobalPlayersRetired, GlobalPlayersQuit );
		Log2( log, "game" );
		// host "retire"  +1 ;)
		Server_Retire();
		
		if ( GlobalConfigMode == 1 ) {
			Server_RetireDriver( 1 );
			Server_RetireDriver( 2 );
		}
		
		// hierna kan men weer naar de lobby..
		
		// nu de speler resultaten opslaan in de MySQL DB..
		// Alleen diegenen die over de finish kwamen bewaren..
		//Log( NumPlayers );
		//FastestTime = sqlGetFastestTime( GlobalStages );
		WinnerID = 0;
		WinnerIndex = 0;
		for ( i=1; i<NumPlayers; i++ ) {
			// is de speler wel over de finish??
			Split0 = getSplitTime(i, 0);
			Split1 = getSplitTime(i, 1);
			Split2 = getSplitTime(i, 2);
			RaceTime = getRaceTime(i);
			
			if ( getFinished(i)>0 &&  getRetired(i)<=0 && ( ( GlobalStages_Stage < 6 && Split0!=RaceTime && Split1!=RaceTime && Split2!=RaceTime ) || ( GlobalStages_Stage==6 && Split0!=0 && Split0!=RaceTime ) ) ) {
				//--- FINISHED
				// de speler in TABLE CMR04_Members
				
				IP = getIP(i);
				Nick = getNick(i);
				Country = StrCountry[getCountry(i)][0];
				// zoek het MemberID op in de DB.
				MemberID = sqlGetMember( IP, Nick, Country );
				
				// de speler in TABLE CMR04_Times
				if ( MemberID != 0 ) {
					
					//Log( "einde race" );
					Car = getCar(i);
					Split0 = getSplitTime(i, 0);
					Split1 = getSplitTime(i, 1);
					Split2 = getSplitTime(i, 2);
					RaceTime = getRaceTime(i);
					Gearbox = getGearbox(i);
					FastestTime = ptr_players[(getCarType(i)+8)].RaceTime;
					MStoTimeString( FastestTime, &timeString );
					sprintf( log, "stage: %d  current stage-record(%s): %s ", GlobalStages_Stage, getCarTypeStr(i), &timeString );
					Log2( log, "game" ); /// als er nu 2 een snellere tijd rijden zie je het misschien 2 x
					
					MStoTimeString( RaceTime, &timeString );
					sprintf( log, "%s: %s ", getNick( i ), &timeString );
					MsgprintyIP1("", 0, "<race-time>", log);
					//sprintf( log, "race-time %s: %s ", getNick( i ), &timeString );
					//Log2( log, "game" );
					
					//if ( GlobalStages_Stage < 6 ) {
					if ( ( RaceTime < FastestTime || FastestTime == 0 ) && RaceTime != 0 ) {
						sprintf( log, "new %s record: %s in %s ", getCarTypeStr(i), getNick( i ), &timeString );
						Log2( log, "game" );
						sprintf( log, "record: %s %s", getNick( i ), &timeString );
						//HostChat( -1, log );
						ServerChat( -1, getCarTypeStr(i), log );
						//							WinnerIndex = i;
						//							WinnerID = MemberID;
						//							FastestTime = RaceTime;
					}
					//} else {
					//	if ( RaceTime < FastestTime || FastestTime == 0 ) {
					//		WinnerIndex = i;
					//		WinnerID = MemberID;
					//		FastestTime = Split1;
					//	}
					//}
					
					//sqlAddTime( MemberID, GlobalDamage, Car, Gearbox, GlobalStages, Split0, Split1, Split2, RaceTime, ptr_players[PlayerIndex[i]].PosCount, &ptr_players[PlayerIndex[i]].PosRec[0] );
				}
				
			} else {
				if ( getRetired(i)>0 ) {
					//--- RETIRED
					// De speler is niet over de finish gekomen.. :-(
					// getSplitsDone(i) geeft aan hoeveel splits de speler is gepasseerd.
					
					// De speler in TABLE CMR04_Members
					if ( MemberID != 0 ) {
						sqlIncRetireCount( MemberID );
					}
					//GlobalPlayersRetired--;
				} else {
					if ( getQuit(i)>0 ) {
						// nog erger..
						//--- QUIT
						
						// De speler in TABLE CMR04_Members
						if ( MemberID != 0 ) {
							sqlIncQuitCount( MemberID );
						}
						//GlobalPlayersQuit--;
					} else {
						// toch maar retire ? Split2=RaceTime
						if ( MemberID != 0 ) {
							sqlIncRetireCount( MemberID );
						}
						
					}
				}
			}
		}
		
		// snelste tijd aller tijden ?
		// dan opslaan...
		/*
		if ( WinnerID != 0 ) {
		
		  MStoTimeString( getRaceTime(WinnerIndex), &timeString );
		  sprintf( log, "%s in %s ", getNick( WinnerIndex ), &timeString );
		  MsgprintyIP1("", 0, "<new record>", log);
		  sprintf( log, "new stage-record: %s in %s ", getNick( WinnerIndex ), &timeString );
		  Log( log );
		  HostChat( -1, log );
		  // store in file / mysql ?
		  //sqlStoreStageRecord( WinnerID, &ptr_players[PlayerIndex[WinnerIndex]].PosRec[0]  );
		  
			
			  }
			  */
			  
			  
			  /*
			  // spelers.Ready weer op 0
			  // spelers.State weer op 1
			  for ( i=0; i< MAXPLAYERS; i++ ) {
			  //!for ( i=0; i<NumPlayers; i++ ) {
			  //!changeReady( i, 0 );
			  //!changeState( i, 1 );
			  ptr_players[i].Ready = 0;		// niet klaar..
			  ptr_players[i].State = 1;		
			  ptr_players[i].VoteStage = 0;	// blanco stem
			  ptr_players[i].hasFinished = 0;
			  ptr_players[i].hasRetired = 0;
			  ptr_players[i].hasQuit = 0;
			  ptr_players[i].PosCount = 0;
			  for (j=0;j<BLOB_LEN;j++) ptr_players[i].PosRec[j] = 0;
			  
				//if ( i != 0 ) {
				//KickPlayer( i, "due to buggy server..." );
				//DeletePlayer( i );
				//}
				}
				*/
				// Playerlists verzenden
				SendAllPlayerList( -1 ); // naar iedereen..
				
				// De GlobalState wordt weer 1
				NextGlobalState( stateInit );// 1=lobby, 10=einde race
				CheckGlobalState();
	}
}


void ServerChangeStage( u_char aStage ) {
	// aStage waarde == 100 + _Land*10 + _Stage
	// 0=ESP, 1=UK, 2=GRC, 3=USA, 4=JPN, 5=SWE, 6=AUS, 7=FIN  (1e index in StrStagesValues & StrStages)
	// 0=Stage 1, 1=S2, 2=S3, 3=S4, 4=S5, 5=S6, 6=Special Stage
	int		i=0;
	u_short	CMD;
	char	log[100];
	u_char	tmpStr[1024];
	
	// stage aanpassen..
	GlobalStages = aStage;
	GlobalStages_Land = ((aStage - 100)-(aStage % 10)) / 10;
	GlobalStages_Stage = aStage % 10;
	
	sprintf( log, "changing stage to %d", aStage );
	Log2( log, "game" );
	
	// Een C6-83 zenden naar iedereen
	PacketLen = sizeof(ServerGameChange);				
	memcpy( TXPacket, ServerGameChange, PacketLen);
	// rally of stages  (0 of 1)
	TXPacket[13] = 1;
	// damage
	TXPacket[14] = GlobalDamage;
	// ranking: points of time
	TXPacket[15] = GlobalRanking;
	// CarType
	TXPacket[16] = GlobalCarType;
	// land
	TXPacket[17] = GlobalStages_Land;
	// stage
	TXPacket[18] = GlobalStages_Stage;
	for (i=1; i<NumPlayers; i++) {
		// ChatLine
		increaseChatLine( i );
		CMD = getChatLine(i);
		TXPacket[3] = CMD & 0xFF;
		TXPacket[4] = (CMD >> 8) & 0xFF;
		//
		rdcksum( TXPacket, PacketLen );
		BufferPacket( i, TXPacket, PacketLen, CMD );
	}
	//test
	// Relay / Doorsturen ??
	if ( RelayEnabled != 0 ) {
		sendto(RelaySocket, TXPacket, PacketLen, 0, (struct sockaddr *)&RelayClient, (socklen_t *)sizeof(RelayClient) );
		//sprintf(log, "%d", sendto(RelaySocket, TXPacket, PacketLen, 0, (struct sockaddr *)&RelayClient, (socklen_t *)sizeof(RelayClient) ));
		//Log( log );
	}
	///test
	//BuffersResend();
	// de host zelf..
	increaseChatLine(0);
	
	
	// highscore laden..
	LoadHighScores( aStage );

	/* kijk maar in de playerlist...
	
	// De snelste tijd (per player-CarType) chatten naar de spelers..
	for (i=1; i<NumPlayers; i++) {
		u_char cartype = getCarType(i);
		u_char *cartypestr = getCarTypeStr(i);
		u_char *Msg[128];
		
		// de snelste tijd ophalen uit de DB
		//FastestTime = sqlGetFastestTime_CarType( aStage, cartypestr );
		if ( ptr_players[cartype+8].RaceTime != 0 ) {
			//sprintf( FastestNick , "%s", sqlGetFastestNick( Result ) );
			MStoTimeString(ptr_players[cartype+8].RaceTime , &tmpStr );
			sprintf( Msg, "%s by %s is the fastest (%s) time on our servers", tmpStr, ptr_players[cartype+8].Nick, cartypestr );
			
			//strcpy( FastestNick, sqlGetFastestNick_CarType( aStage, cartypestr ) ) ;
			//strcat( &tmpStr, " by ");
			//strcat( &tmpStr, FastestNick );
			//strcat( &tmpStr, " is the fastest (" );
			//strcat( &tmpStr, cartypestr );
			//strcat( &tmpStr, ") time on our servers");
			
			ServerChat( i, VOTESYSTEM, &Msg );
		}
	}
	*/
}

void ServerChat( short PlayerNr, u_char* aSpeaker, u_char* aMessage ) {
	int		i;
	int		L;
	int		LMsg;
	
	//--- een C6-8B naar de rest..
	PacketLen = sizeof(ChatFromServerToAny);
	memcpy( TXPacket, ChatFromServerToAny, PacketLen);
	
	//"Server" tekst aanpassen...
	strcpy( TXPacket+37, aSpeaker );
	
	// de chat tekst overnemen
	strcpy( TXPacket + 37 + strlen(aSpeaker)+1, (u_char *) aMessage );
	
	// de pakket-lengte berekenen..(na chatText+0 afkappen)
	LMsg = strlen((u_char *) aMessage);
	PacketLen = 37 + strlen(aSpeaker)+1 + LMsg+1;
	
	// naar allemaal ??
	if ( PlayerNr == -1 ) {
		// C6 terugzenden..
		for ( i=1; i < NumPlayers; i++ ) {
			increaseChatLine( i );
			tmpUShort = getChatLine(i);
			TXPacket[3] = tmpUShort & 0xFF;
			TXPacket[4] = (tmpUShort >> 8) & 0xFF;
			
			rdcksum( TXPacket, PacketLen );
			BufferPacket( i, TXPacket, PacketLen, tmpUShort );
		}
	} else {
		if ( PlayerNr > 0 ) {
			increaseChatLine( PlayerNr );
			tmpUShort = getChatLine(PlayerNr);
			TXPacket[3] = tmpUShort & 0xFF;
			TXPacket[4] = (tmpUShort >> 8) & 0xFF;
			
			rdcksum( TXPacket, PacketLen );
			BufferPacket( PlayerNr, TXPacket, PacketLen, tmpUShort );
		}
	}
	increaseChatLine( 0 ); //host
	//BuffersResend();
}

void HostChat( short PlayerNr, u_char *aMessage ) {
	int		i;
	int		L;
	int		LMsg;
	
	//--- een C6-8B naar de rest..
	PacketLen = sizeof(ChatFromHostToAny);
	memcpy( TXPacket, ChatFromHostToAny, PacketLen);
	
	// nickname opzoeken en plakken
	strcpy( (u_char *) ChatFromHostToAny_Nick, getNick(0) );
	//strncpy( (u_char *) ChatFromHostToAny_Nick,3, (u_char *)"UJE" );
	
	// de chat tekst uit het ontvangen pakketje overnemen
	L = strlen((u_char *) ChatFromHostToAny_Nick);
	LMsg = strlen((u_char *) aMessage);
	strcpy( ChatFromHostToAny_Nick + L+1, (u_char *) aMessage );
	// de pakket-lengte berekenen..(na chatText+0 afkappen)
	PacketLen = 37 + L+1 + LMsg+1;
	
	// naar allemaal ??
	if ( PlayerNr == -1 ) {
		// C6 terugzenden..
		for ( i=1; i < NumPlayers; i++ ) {
			increaseChatLine( i );
			
			tmpUShort = getChatLine(i);
			TXPacket[3] = tmpUShort & 0xFF;
			TXPacket[4] = (tmpUShort >> 8) & 0xFF;
			
			tmpULong =  getID(0);		// wie zei het ?
			TXPacket[9] = tmpULong & 0xFF;
			TXPacket[10] = (tmpULong >> 8) & 0xFF;
			TXPacket[11] = (tmpULong >> 16) & 0xFF;
			TXPacket[12] = (tmpULong >> 24) & 0xFF;
			
			rdcksum( TXPacket, PacketLen );
			//SendToPlayer( i, PacketLen );
			BufferPacket( i, TXPacket, PacketLen, tmpUShort );
		}
	} else {
		if ( PlayerNr > 0 ) {
			increaseChatLine( PlayerNr );
			
			tmpUShort = getChatLine(PlayerNr);
			TXPacket[3] = tmpUShort & 0xFF;
			TXPacket[4] = (tmpUShort >> 8) & 0xFF;
			
			tmpULong =  getID(0);		// wie zei het ?
			TXPacket[9] = tmpULong & 0xFF;
			TXPacket[10] = (tmpULong >> 8) & 0xFF;
			TXPacket[11] = (tmpULong >> 16) & 0xFF;
			TXPacket[12] = (tmpULong >> 24) & 0xFF;
			
			rdcksum( TXPacket, PacketLen );
			//SendToPlayer( PlayerNr, PacketLen );
			BufferPacket( PlayerNr, TXPacket, PacketLen, tmpUShort );
		}
	}
	increaseChatLine( 0 ); //host
	//BuffersResend();
}

// geef een byte terug, met de waarde van de stage in de vote(-string)
u_char IsStage( u_char aStage ) {
	u_char	Result=0;
	int		i;
	int		j;
	for ( i=0; i < 8; i++ ) {
		for ( j=0; j < 7; j++ ) {
			// formaat van de stage
			sprintf( log, "%d", aStage );
			if (  strcasecmp(log,StrStagesValues[i][j])==0 )  {
				// gevonden..nu een Stage-code samenstellen..
				Result = 100 + i*10 + j;
				break ; // uit de 2 for lussen gaan..
			}
		}
		if ( Result != 0 ) break;
	}
	return Result;
}


// geef een byte terug, met de waarde van de stage in de vote(-string)
u_char StrToStageValue( u_char *aStage ) {
	u_char	Result=0;
	int		i;
	int		j;
	for ( i=0; i < 8; i++ ) {
		for ( j=0; j < 7; j++ ) {
			// formaat van de stage
			if ( strcasecmp(aStage, StrVoteStages[i][j])==0 ) {
				// gevonden..nu een Stage-code samenstellen..
				Result = 100 + i*10 + j;
				break ; // uit de 2 for lussen gaan..
			}
			// om het stemmen wat te versoepelen..
			if ( strcasecmp(aStage, StrVoteStages2[i][j])==0 ) {
				// gevonden..nu een Stage-code samenstellen..
				Result = 100 + i*10 + j;
				break ; // uit de 2 for lussen gaan..
			}
			if ( strcasecmp(aStage, StrVoteStages3[i][j])==0 ) {
				// gevonden..nu een Stage-code samenstellen..
				Result = 100 + i*10 + j;
				break ; // uit de 2 for lussen gaan..
			}
			if ( strcasecmp(aStage, StrVoteStages4[i][j])==0 ) {
				// gevonden..nu een Stage-code samenstellen..
				Result = 100 + i*10 + j;
				break ; // uit de 2 for lussen gaan..
			}
			if ( strcasecmp(aStage, StrVoteStages5[i][j])==0 ) {
				// gevonden..nu een Stage-code samenstellen..
				Result = 100 + i*10 + j;
				break ; // uit de 2 for lussen gaan..
			}
			if ( strcasecmp(aStage, StrVoteStages6[i][j])==0 ) {
				// gevonden..nu een Stage-code samenstellen..
				Result = 100 + i*10 + j;
				break ; // uit de 2 for lussen gaan..
			}
			if ( strcasecmp(aStage, StrVoteStages7[i][j])==0 ) {
				// gevonden..nu een Stage-code samenstellen..
				Result = 100 + i*10 + j;
				break ; // uit de 2 for lussen gaan..
			}
			if ( strcasecmp(aStage, StrVoteStages8[i][j])==0 ) {
				// gevonden..nu een Stage-code samenstellen..
				Result = 100 + i*10 + j;
				break ; // uit de 2 for lussen gaan..
			}
			if ( strcasecmp(aStage, StrVoteStages9[i][j])==0 ) {
				// gevonden..nu een Stage-code samenstellen..
				Result = 100 + i*10 + j;
				break ; // uit de 2 for lussen gaan..
			}
			if ( strcasecmp(aStage, StrVoteStages10[i][j])==0 ) {
				// gevonden..nu een Stage-code samenstellen..
				Result = 100 + i*10 + j;
				break ; // uit de 2 for lussen gaan..
			}
		}
		if ( Result != 0 ) break;
	}
	return Result;
}

// resultaat = de cartype die de host-ghost moet rijden
u_char StrToGhostValue( u_char *aGhost ) {
	u_char	Result=0;
	int		i;
	for ( i=1; i<5; i++ ) {
		if ( strcasecmp(aGhost, StrCarType[i])==0 ) {
			Result = i-1;
			break;
		}
	}
	//if ( Result ==0 ) Result = 1; //  2x 1 ?   4 van maken ?  of dan pokt ie
	
	
	return Result;
}

// geef een byte terug, met de waarde van de car in de vote(-string)
u_char StrToCarValue( u_char *aCar ) {
	u_char	Result=0;
	int		i;
	int		j;
	for ( i=0; i < 23; i++ ) {
		// formaat van de stage
		if ( strcasecmp(aCar, StrVoteCar[i])==0 ) {
			// gevonden..nu een Car-code samenstellen..
			Result = i;
			break;
		}
	}
	return Result;
}



// geef een byte terug, met de waarde van de car in de vote(-string)
u_char StrToDamageValue( u_char *aDamage ) {
	u_char	Result=0;
	int		i;
	int		j;
	for ( i=0; i < 3; i++ ) {
		if ( strcasecmp(aDamage, StrDamage[i])==0 ) {
			// gevonden..nu een Car-code samenstellen..
			Result = i;
			break;
		}
	}
	return Result;
}




// resultaat == 0, indien de chat geen "vote status"-chat is,
// resultaat == 1, anders..
u_char isClientVoteStatus( short PlayerNr, u_char* aMessage ) {
	u_char	Result=0;
	if ( PlayerNr<1 || PlayerNr>=NumPlayers ) return Result;
	if ( strlen(aMessage) != 11 ) return Result;
	// de eerste 11 tekens vergelijken..
	if ( strncasecmp( "vote status", aMessage, 11 )==0 ) { //precies gelijk? (op case na dan:)
		Result = 1;
	}
	return Result;
}

// resultaat == 1, indien de chat een "vote kick"-chat is,
// resultaat == 0, indien niet..
u_char isClientVoteKick( short PlayerNr, u_char* aMessage ) {
	u_char	Result=0;
	if ( PlayerNr<1 || PlayerNr>=NumPlayers ) return Result;
	if ( strlen(aMessage) < 11 ) return Result;
	// de eerste 10 tekens vergelijken..
	if ( strncasecmp( "vote kick ", aMessage, 10 )==0 ) { //precies gelijk? (op case na dan:)
		Result = 1;
	} else { 
		if ( strncasecmp( "kick ", aMessage, 5 )==0 ) { //precies gelijk? (op case na dan:)
			Result = 2;
		}
	}
	return Result;
}

// resultaat == carType, indien de chat een "vote ghost"-chat is,
// resultaat == 0, indien niet..
u_char isClientVoteGhost( short PlayerNr, u_char* aMessage ) {
	u_char	Result=255;
	if ( PlayerNr<1 || PlayerNr>=NumPlayers ) return Result;
	if ( strlen(aMessage) < 6 ) return Result;
	// de eerste 10 tekens vergelijken..
	//if ( strcasecmp(aGhost, "pr")==0 ) {
	if ( strncasecmp( "ghost pr", aMessage, 8 )==0 ) { //precies gelijk? (op case na dan:)
		Result = PlayerIndex[PlayerNr] + 3; // +8 erbij in sendpos = de PR per player
		return Result;
	}
	if ( strncasecmp( "ghost ", aMessage, 6 )==0 ) { //precies gelijk? (op case na dan:)
		Result = StrToGhostValue( aMessage+6 );
		/*
		sprintf( log, "isClientVoteGhost = %d", Result );
		MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<isClientVoteGhost>", log);
		sprintf( log, "isClientVoteGhost: %d", Result );
		Log( log );
		*/
	}
	return Result;
}

// resultaat == 0, indien de chat geen "vote"-chat is..
// resultaat == StageValue, als de chat een geldige "vote"-chat is..
u_char isClientVoteChat( short PlayerNr, u_char* aMessage ) {
	u_char	Result=0;
	if ( PlayerNr<1 || PlayerNr>=NumPlayers ) return Result;
	// de korste aMessage, zou tenminste 8 lang zijn..   "vote UK1"
	if ( strlen(aMessage) < 3 ) return Result;
	// geen "vote status" en ook geen "vote kick"??
	// dan controleren op een "vote stage"..
	//@	if ( isClientVoteStatus(PlayerNr,aMessage)==0 && isClientVoteKick(PlayerNr,aMessage)==0 ) {
	// de eerste 5 tekens vergelijken..
	if ( strncasecmp( "vote ", aMessage, 5 )==0 ) { //precies gelijk? (op case na dan:)
		// vergelijk de rest van de chat-tekst met de stages..
		Result = StrToStageValue( aMessage+5 ); //pointer na tekst "vote "
		return Result;
	}
	Result = StrToStageValue( aMessage );
	
	//@	}
	return Result;
}

// resultaat == 0, indien de chat geen "vote car"-chat is..
// resultaat == CarValue, als de chat een geldige "vote car"-chat is..
u_char isClientVoteCar( short PlayerNr, u_char* aMessage ) {
	u_char	Result=0;
	if ( PlayerNr<1 || PlayerNr>=NumPlayers ) return Result;
	// de korste aMessage, zou tenminste 4 lang zijn..   "car 205"
	if ( strlen(aMessage) < 4 ) return Result;
	// geen "vote status" en ook geen "vote kick"??
	// dan controleren op een "vote stage"..
	//@	if ( isClientVoteStatus(PlayerNr,aMessage)==0 && isClientVoteKick(PlayerNr,aMessage)==0 ) {
	
	// de eerste 4 tekens vergelijken..
	if ( strncasecmp( "car ", aMessage, 4 )==0 ) { //precies gelijk? (op case na dan:)
		Result = StrToCarValue( aMessage+4 ); //pointer na tekst "vote "
	} else {
		Result = 0;
	}
	
	//@	}
	return Result;
}

// resultaat == 0, indien de chat geen "vote damage"-chat is..
// resultaat == DamageValue, als de chat een geldige "vote damage"-chat is..
u_char isClientVoteDamage( short PlayerNr, u_char* aMessage ) {
	u_char	Result=0;
	if ( PlayerNr<1 || PlayerNr>=NumPlayers ) return Result;
	// de korste aMessage, zou tenminste 6 lang zijn..   "damage normal"
	if ( strlen(aMessage) < 7 ) return Result;
	// geen "vote status" en ook geen "vote kick"??
	// dan controleren op een "vote stage"..
	//@	if ( isClientVoteStatus(PlayerNr,aMessage)==0 && isClientVoteKick(PlayerNr,aMessage)==0 ) {
	
	// de eerste 4 tekens vergelijken..
	if ( strncasecmp( "damage ", aMessage, 7 )==0 ) { //precies gelijk? (op case na dan:)
		Result = StrToDamageValue( aMessage+7 ); //pointer na tekst "damage "
	} else {
		Result = 0;
	}
	
	//@	}
	return Result;
}


// resultaat == 0, indien de chattekst geen "vote <stage>" bevat..
// anders is resultaat != 0, en bevat het de Value (byte-waarde) van de stage (bv 134 voor usa5)
u_char ClientVoting( short PlayerNr, u_char* aMessage ) {
	u_char	Result=0;
	u_char	tmpStr[1024];
	u_char	Country;
	u_char	Stage;
	u_char	StrTotalVotes[8];
	u_char	StrNVotes[8];
	u_char	StrNeededVotes[8];
	u_char	vStages[8][2];
	u_char	totalVotes=0;
	u_char	neededVotes=0;
	u_char	nVotes=0;
	u_char	mostVotes;
	u_char	stageFound=0;
	u_char	voteComplete=0;
	u_char	Name[16];
	int		i;
	int		j;
	if ( PlayerNr<1 || PlayerNr>=NumPlayers ) return Result;
	if ( GlobalVotingEnabled == 0 ) return Result;
	
	//--- is het een "vote status" string ??
	if ( isClientVoteStatus( PlayerNr, aMessage ) != 0 ) {
		// De voting status zenden naar PlayerNr..
		ServerChat( PlayerNr, VOTESYSTEM, StrVoteStatus );
		//resulteer: geen "vote"-chat, en beeindig de function..
		return 0;
	}
	
	
	//--- is het een "vote kick" string ??
	Result = isClientVoteKick( PlayerNr, aMessage );
	if ( Result != 0 ) {
		// spelernaam uit de chat halen..
		if ( Result == 1 ) {
			strncpy( &Name, aMessage+10, 15 );
		} else {
			strncpy( &Name, aMessage+5, 15 );
		}
		// de speler-ID opzoeken..
		for ( i=1; i < NumPlayers; i++ ) {
			if ( strcasecmp( getNick(i), &Name )==0 ) { //precies gelijk? (op case na dan:)
				
				// speler(PlayerNr) heeft ge-vote tegen speler(i)..
				changeVoteKick( PlayerNr, getID(i) );
				
				//--- UJE KICK VOTING SYSTEM
				// overall aantal votes..totalVotes
				// totaal aantal votes tegen deze speler..nVotes
				// aantal votes nog nodig tegen deze speler..neededVotes
				nVotes = 0;
				totalVotes = 0;
				voteComplete = 0; // 0=de vote is nog niet compleet; Meer stemmen nodig..
				for ( j=1; j < NumPlayers; j++ ) {
					if ( getVoteKick(j) != 0 ) totalVotes++;
					if ( getVoteKick(j) == getVoteKick(PlayerNr) ) nVotes++;
				}
				// maar 1 speler gejoined?, dan altijd van baan wisselen..
				if ( NumPlayers-1 == 1 ) {	// de host telt niet mee; Die vote toch nooit..
					neededVotes = 0;
					voteComplete = 1;
				} else {
					// meer dan 50% van alle spelers moeten hebben ge-vote, 
					// om een complete vote te verkrijgen..de host vote nooit mee..
					neededVotes = (int)((NumPlayers-1)/2)+1 - nVotes;
					if ( neededVotes <= 0 ) voteComplete = 1;
				}
				//---
				
				// string maken van nummers..
				sprintf( &StrTotalVotes, "%d", totalVotes );
				sprintf( &StrNVotes, "%d", nVotes );
				sprintf( &StrNeededVotes, "%d", neededVotes );
				
				// een vote-melding naar iedereen..
				// anonieme "vote-kick"s ??
				if ( GlobalShowVoteKicks == 0 ) {
					strcpy( &tmpStr, "Someone" );
				} else {
					strcpy( &tmpStr, getNick(PlayerNr) );
				}
				strcat( &tmpStr, StrVoteKick );
				strcat( &tmpStr, getNick(i) );
				// meer stemmen nodig??
				if ( neededVotes > 0 ) {
					strcat( &tmpStr, " (votes:" );
					strcat( &tmpStr, StrNVotes );
					strcat( &tmpStr, ", " );
					strcat( &tmpStr, StrNeededVotes );
					strcat( &tmpStr, " more needed)" );
				}
				// vote complete??
				if ( voteComplete == 1 ) strcat( &tmpStr, ". Let's kick some ass!" );
				// melding verzenden..
				
				// evt. de speler kicken..
				if ( voteComplete == 1 ) {
					KickPlayer( i, "You've been voted out by the players.." );
					// "vote-kick"s resetten..
					for ( j=0; j<=MaxPlayers; j++ ) changeVoteKick(j,0);
				}
				
				//resulteer: geen "vote"-chat, en beeindig de function..
				return 0;
			}
		}
		return Result;
	}
	
	
	
	//--- is het een "vote <stage>" string ??
	Result = isClientVoteChat( PlayerNr, aMessage );
	// was het een geldige vote??
	if ( Result != 0 ) {
		//  ..maar 1 keer mogen stemmen?? EN speler nog geen stem uitgebracht ??
		if ( (GlobalVoteOnce==0)  ||  (GlobalVoteOnce!=0  &&  getVoteStage(PlayerNr)==0) ) {
			
			// speler(PlayerNr) heeft ge-vote op stage(Result)..
			changeVoteStage( PlayerNr, Result );
			// even de vote ontleden in een land en een baan..
			Country = ((Result - 100)-(Result % 10)) / 10;
			Stage = Result % 10;
			
			//--- UJE STAGE VOTING SYSTEM
			// overall aantal votes..totalVotes
			// totaal aantal votes voor deze stage..nVotes
			// aantal votes nog nodig voor deze stage..neededVotes
			nVotes = 0;
			totalVotes = 0;
			voteComplete = 0; // 0=de vote is nog niet compleet; Meer stemmen nodig..
			for ( i=1; i < NumPlayers; i++ ) {
				if ( getVoteStage(i) != 0 ) totalVotes++;
				if ( getVoteStage(i) == Result ) nVotes++;
			}
			// maar 1 speler gejoined?, dan altijd van baan wisselen..
			if ( NumPlayers-1 == 1 ) {	// de host telt niet mee; Die vote toch nooit..
				neededVotes = 0;
				voteComplete = 1;
			} else {
				// tenminste 2 spelers moeten hebben ge-vote, om een complete vote te verkrijgen..
				if ( totalVotes > 1 ) {
					neededVotes = ( nVotes > totalVotes/2 )? 0: (int)(totalVotes/2)+1 - nVotes;
				} else {
					neededVotes = 1;
				}
				if ( neededVotes == 0 ) voteComplete = 1;
			}
			//---
			
			// string maken van nummers..
			sprintf( &StrTotalVotes, "%d", totalVotes );
			sprintf( &StrNVotes, "%d", nVotes );
			sprintf( &StrNeededVotes, "%d", neededVotes );
			
			// een vote-melding naar iedereen..
			//strcpy( &tmpStr, getNick(PlayerNr) );
			//strcat( &tmpStr, "voted: " );
			strcpy( &tmpStr, "voted: " );
			strcat( &tmpStr, StrStages[Country][Stage] );
			// meer stemmen nodig??
/*
//			if ( neededVotes > 0 ) {
//				strcat( &tmpStr, " (votes:" );
//				strcat( &tmpStr, StrNVotes );
//				strcat( &tmpStr, ", " );
//				strcat( &tmpStr, StrNeededVotes );
//				if ( neededVotes == 1 ) {
//					strcat( &tmpStr, " more vote needed)" );
//				} else {
//					strcat( &tmpStr, " more votes needed)" );
//				}
//			}
			if ( neededVotes>0 && voteComplete!=1 ) {
				strcat( &tmpStr, " (" );
				strcat( &tmpStr, StrNVotes );
				strcat( &tmpStr, "/" );
				strcat( &tmpStr, StrNeededVotes );
				strcat( &tmpStr, ")" );
			}
*/
			if ( GlobalConfigMode == 1 ) {
				voteComplete = 1;
			}
			// vote complete??
			if ( voteComplete == 1 ) strcat( &tmpStr, ". Changing stage!" );
			// melding verzenden..

			strcpy( &Name, getNick(PlayerNr) );
			ServerChat( -1, &Name, &tmpStr );
			
			// vote al beslist? dan stage wisselen..
			if ( voteComplete == 1 ) {
				// stage wisselen..
				if ( Result != 0 ) ServerChangeStage( Result );
				// alle speler-votes resetten..
				for ( i=1; i < NumPlayers; i++ ) changeVoteStage( i, 0);
				GlobalStagesPrev = 0; /// niet meer random kiezen


				// de stageoounter herstellen na een vote, zodat deze geen stages overslaat.
				if ( GlobalStageCycleEnabled==1 ) {
					GlobalStageCycleCounter = GlobalStageCycleLastCounter;
				}

									  /*
									  // de snelste tijd ophalen uit de DB
									  FastestTime = sqlGetFastestTime( Result );
									  if ( FastestTime != 0 ) {
									  //sprintf( FastestNick , "%s", sqlGetFastestNick( Result ) );
									  strcpy( FastestNick, sqlGetFastestNick( Result ) ) ;
									  MStoTimeString(FastestTime , &tmpStr );
									  strcat( &tmpStr, " by ");
									  strcat( &tmpStr, FastestNick );
									  strcat( &tmpStr, " is the fastest time on our servers");
									  
										ServerChat( -1, VOTESYSTEM, &tmpStr );
										}
				*/
				/*
				// de auto van de snelste tijd ophalen uit de DB
				FastestCar = sqlGetFastestCar( Result );
				*/
			}
			
		}
		return Result;
	}
	
	/*
	//--- is het een "car <car>" string ??
	Result = isClientVoteCar( PlayerNr, aMessage );
	// was het een geldige vote??
	if ( Result != 0 ) {
	changeCar( PlayerNr, Result );
	//changeCarType( PlayerNr, RXPacket[18] );
	//changeGearbox( PlayerNr, RXPacket[19] );
	
	  // Playerlists verzenden
	  SendAllPlayerList( PlayerNr );
	  return Result;
	  }
	  */
	  
	  
	  Result = isClientVoteDamage( PlayerNr, aMessage );
	  // was het een geldige vote??
	  if ( Result != 0 ) {
		  //  ..maar 1 keer mogen stemmen?? EN speler nog geen stem uitgebracht ??
		  //		if ( (GlobalVoteOnce==0)  ||  (GlobalVoteOnce!=0  &&  getVoteStage(PlayerNr)==0) ) {
		  GlobalDamage = Result;
		  ServerChangeStage( GlobalStages );
		  //		}
		  return Result;
	  }
	  
	  
	  // is het een chat om de host-ghost te veranderen
	  Result = isClientVoteGhost( PlayerNr, aMessage );
	  if ( Result != 255 ) {
		  changeVoteGhost( PlayerNr, Result );
		  return Result;
	  } else {
		  Result = 0;
	  }
	  
	  return Result;
}

// geef 0 terug als de tekst geldig is.
// resulteer != 0 als het en foute tekst is..
u_char ChatForbidden( u_char* aMessage ) {
	// strcasecmp is case-insensitive, resultaat == 0 indien strings gelijk.
	if ( strcasecmp("go", aMessage)==0 ) return 1;
	if ( strcasecmp("gogo", aMessage)==0 ) return 2;
	if ( strcasecmp("gogogo", aMessage)==0 ) return 3;
	if ( strcasecmp("help", aMessage)==0 ) return 4;
	return 0; // 0 is goed man
}

// client chat teksten censuur / vervanging..
// Als PlayerNr>0, dan gaat een extra melding naar deze speler.
void ClientNoNonsense( short PlayerNr, u_char* aMessage ) {
	int		i;
	// test op verboden teksten..
	if ( ChatForbidden(aMessage) != 0 ) {
		for ( i=1; i<NumPlayers; i++ ) {
			if ( PlayerNr==i ) {
				// een chat naar de spreker..
				ServerChat( i, FILTERSYSTEM, &StrReplaceGogogoD6 );
			} //else
			//geen verboden chats naar anderen...
		}
	}
}



// Een speler verwijderen.
// (+ alle arrays/variabelen bijwerken..
void RemovePlayer( short PlayerNr, u_char sendPL ) {
	if ( PlayerNr <= 0 ) return;
	/*
	// aan het racen??
	if ( GlobalState > stateJoinNr ) {   // was 3, daarna 5
	// nog niet ge-quit??
	if ( getQuit(PlayerNr)==0 ) {
	// deze speler .hasQuit bijwerken..
	//changeQuit( PlayerNr, 1 );
	
	  // GlobalPlayersFinished evt. bijwerken..
	  // anders is deze teller te hoog; De speler is weg..
	  if ( getFinished(PlayerNr)!=0 ) {
	  //changeFinished(PlayerNr, 0);
	  // het aantal spelers retired tot dusver..
	  //if ( GlobalPlayersFinished>0 ) GlobalPlayersFinished--;
	  }
	  
		// GlobalPlayersRetired evt. bijwerken..
		// anders is deze teller te hoog; De speler is weg..
		if ( getRetired(PlayerNr)!=0 ) {
		//changeRetired(PlayerNr, 0);
		// het aantal spelers retired tot dusver..
		//if ( GlobalPlayersRetired>0 ) GlobalPlayersRetired--;
		}
		}
		}
	*/
	// de speler verwijderen uit de lijst..
	// openwaiting??
	DeletePlayer( PlayerNr );
	playerNumber = -1;

	// Playerlists verzenden.
	// !! LET OP !! Er is geen zender meer om te beantwoorden..!!

	// indien in de lobby, playerlist naar de rest
	if ( sendPL != 0 ) { //changed: 2008apr30
		SendAllPlayerList( -1 );
	}
	
	// evt. "terug naar Lobby" mogelijk maken..
	//CheckRetireHost();
}






int MyTimeout(int sock) {
    struct  timeval tout;
    fd_set  fd_read;
    int     err;
	
    tout.tv_sec = 0;
    tout.tv_usec = TIMEOUT;
    FD_ZERO(&fd_read);
    FD_SET(sock, &fd_read);
    err = select(sock + 1, &fd_read, NULL, NULL, &tout);
    if(err < 0) std_err();
    if(!err) return(-1);
    return(0);
}

int MyTimeoutTCP(int sock) {
    struct  timeval tout;
    fd_set  fd_read;
    int     err;
	
    tout.tv_sec = 0;
    tout.tv_usec = TIMEOUTTCP;
    FD_ZERO(&fd_read);
    FD_SET(sock, &fd_read);
    err = select(sock + 1, &fd_read, NULL, NULL, &tout);
    if(err < 0) std_err();
    if(!err) return(-1);
    return(0);
}


u_long resolv(char *host) {
    struct hostent *hp;
    u_long host_ip;
	
    host_ip = inet_addr(host);
    if(host_ip == INADDR_NONE) {
        hp = gethostbyname(host);
        if(!hp) {
            printf("\nError: Unable to resolv hostname (%s)\n", host);
            exit(1);
        } else host_ip = *(u_long *)hp->h_addr;
    }
    return(host_ip);
}



#ifndef WIN32
void std_err(void) {
	perror("\nError");
	exit(1);
}
#endif







char* trim_right(char* szSource)
{
	char* pszEOS = 0;
	
	// Set pointer to character before terminating NULL
	pszEOS = szSource + strlen( szSource ) - 1;
	
	// iterate backwards until non ' ' is found
	while( (pszEOS >= szSource) && (*pszEOS == ' ') )
		*pszEOS-- = '\0';
	return szSource;
}

char* trim_left(char* szSource)
{
	char* pszBOS = 0;
	
	pszBOS = szSource;
	
	// iterate backwards until non ' ' is found
	while(*pszBOS == ' ')
		*pszBOS++;
	
	return pszBOS;
}

char* trim(char *szSource)
{
	return trim_left( trim_right( trim_left(szSource) ) );
}



// 2 lijnstukken snijden..tbv NO-CUTS controle..
u_char LineSegmentIntersection( float P1x,float P1y, float P2x,float P2y, float P3x,float P3y, float P4x,float P4y ) {
	//bool LineSegmentIntersection(float P1x, P1y,		// Point 1  \ Linesegment 1
	//                                  P2x, P2y,		// Point 2  /
	//								   P3x, P3y,		// Point 3  \ Linesegment 2
	//								   P4x, P4y) {		// Point 4  /
	float lax  = (P2x - P1x);
	float lay  = (P2y - P1y);
	float lbx  = (P4x - P3x);
	float lby  = (P4y - P3y);
	float labx = (P1x - P3x);
	float laby = (P1y - P3y);
	
	float denom = (lby * lax) - (lbx * lay);
	if (denom == 0.0f) return 0;
	
	float numa = (lbx * laby) - (lby * labx);
	float numb = (lax * laby) - (lay * labx);
	float ua = numa / denom;
	float ub = numb / denom;
	
	if (ua >= 0.0f && ua <= 1.0f && ub >= 0.0f && ub <= 1.0f) {
		// intersection-point:
		// x = P1x + ua*lax;
		// y = P1y + ua*lay;
		// even printen ?
		return 1;
	}
	return 0;
}


// NO-CUTS controle: resultaat 0 = geen cut (of nocuts is uitgeschakeld, resultaat 1 = ze cutten
u_char CheckNoCut( short PlayerNr, float aPosX, float aPosY, float aPosZ ) {
	u_char	Result = 0;
	float LPosX;
	float NoCutX1;
	float NoCutX2;
	float LPosY;
	float NoCutY1;
	float NoCutY2;
	float LPosZ;
	float NoCutZ1;
	float NoCutZ2;
	
	if ( GlobalNoCutsEnabled != 1 ) return Result;
	if ( GlobalState != stateRacing ) return Result;
	if ( getRetired(PlayerNr)!=0 || getFinished(PlayerNr)!=0 ) return Result;
	
	LPosX = getLastPosX( PlayerNr );
	LPosY = getLastPosY( PlayerNr );
	LPosZ = getLastPosZ( PlayerNr );
	
	if ( ptr_players[PlayerIndex[PlayerNr]].RaceTime>5000 ) {//changed 2008apr28
		for (i=0; i<GlobalCutsCount; i++ ) {
			
			NoCutX1 = GlobalCuts[i].X1;
			NoCutY1 = GlobalCuts[i].Y1;
			NoCutZ1 = GlobalCuts[i].Z1;
			NoCutX2 = GlobalCuts[i].X2;
			NoCutY2 = GlobalCuts[i].Y2;
			NoCutZ2 = GlobalCuts[i].Z2;
			
			if ( LineSegmentIntersection( (float) aPosX, (float) aPosZ, LPosX,LPosZ, NoCutX1,NoCutZ1, NoCutX2,NoCutZ2) ) {
				//sprintf( log, "<cut>: XYZ(%f, %f, %f) <-> XYZ(%f, %f, %f) ", NoCutX1, NoCutY1, NoCutZ1, NoCutX2, NoCutY2, NoCutZ2 );
				sprintf( log, "<cut>: Pos XYZ(%f, %f, %f) ", aPosX,aPosY,aPosZ );
				//sprintf( log, "%s", getNick(PlayerNr) );
				MsgprintyIP1(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), "<cut>", log);
				//Log( log );
				// de speler is aan het cutten..
				Server_RetireDriver( PlayerNr );
				Result = 1;
			}
		}
	}
	
	return Result;
}



/*
// een willekeurige baan kiezen,
// maar wel 1 die niet pas al is gereden..
u_char RandomStage() {
u_char	Result = 0;
u_char	rCountry;
u_char	rStage;
int		i;

  while ( Result==0 ) {
		// verzin een willekeurige stage
		while ( !IsStage(Result) ) {
		rCountry = (random() % 8) * 10;
		rStage = (random() % 7);
		Result = 100 + rCountry + rStage;
		}
		// zijn alle banen al eens gereden?? dan de array legen..
		if ( StagesDone[51]!=0 ) {
		for (i=0; i<52; i++) StagesDone[i]=0;
		} else {
		// is de stage recentelijk al gereden??
		for (i=0; i<52; i++) { //max 52 in deze array
		//
		if ( StagesDone[i]==0 ) {
		// stage is niet gevonden in StagesDone..dan toevoegen aan deze array
		StagesDone[i] = Result;
		break;
		}
		//
		if ( Result == StagesDone[i] ) {
		Result = 0;
		break;
		}
		}
		}
		}
		return Result;
		}
*/

void SendSplit( u_char PlayerNr, u_char Driver, u_char SplitNr, u_long aSplitTime ) {
	unsigned short	tmpUShort;
    u_char			TXPacket[BUFFSZ];	// the packet to transmit


	// Een C6....0C sturen naar iedereen (behalve de host)
	PacketLen = sizeof(ServerIntermediate);
	memcpy( TXPacket, ServerIntermediate, PacketLen);
	// CMD
	increaseChatLine(PlayerNr);
	tmpUShort = getChatLine(PlayerNr);
	TXPacket[3] = tmpUShort & 0xFF;
	TXPacket[4] = (tmpUShort >> 8) & 0xFF;
	// PlayerNr van deze speler(playerNumber)
	TXPacket[6] = Driver;
	// split
	TXPacket[7] = SplitNr;
//	TXPacket[8] = ??;

	// "\xD0\xD6\x53\x00" constant ??
//het verschill in tijd met de koploper?? test
memcpy( TXPacket+9, &aSplitTime, 4); // in-game te zien dan: de split-tijd van de red-car (maar dan negatief)

	// totaal tijd tot dusver gereden..(+ de rest van het packet..woot (altijd 0?))
	memcpy( TXPacket+13, &aSplitTime, 4);

	// checksum
	rdcksum( TXPacket,PacketLen );
	//							SendToPlayer( i, PacketLen );
	BufferPacket( PlayerNr, TXPacket, PacketLen, tmpUShort );
	// host chatline verhogen..
	increaseChatLine(0);

}


void Toggle_ReplayMode() {
	GlobalReplayMode = (GlobalReplayMode==0)?1:0;
}


void ReadIniFile( char* Filename ) {
	FILE	*iniFile;
	char*	s[1025];
	char*	vWord;
	//char*	vKey;
	char*	vValue;
	char	Result;
	u_char	PrintMessages = 1;

fprintf( stdout, "Filename = %s\n", Filename );

	iniFile = fopen(Filename, "r");

fprintf( stdout, "File opened\n" );

	Result = 0;
	while (!feof(iniFile)) {
fprintf( stdout, "not end of file\n" );
		Result = fgets(s, 1024, iniFile);
fprintf( stdout, "Result = %d\n", Result );
		if ( Result<0 ) {
			// error
		} else if ( Result==0 ) {
			// end-of-line
			
		} else {
			// line read OK.

			vWord = strtok(s," \x09"); // eerste aanroep? dan string opgeven om te doorzoeken.
fprintf( stdout, "word = %s\n", vWord );
			while ( vWord!=NULL ) {

				// verwerk het gevonden woord..
				//if ( strcasecmp(vWord,"=")==0 ) {
					//
				//} else
				// is het een Key?

				if ( strcasecmp(vWord,"ServerName")==0 ) {
fprintf( stdout, "servername\n" );
					//vKey = vWord;
					vWord = strtok(NULL," \x09");
fprintf( stdout, "word = %s\n", vWord );
					if ( strcasecmp(vWord,"=")==0 ) {
						// Value lezen..
						vValue = strtok(NULL," \x09");
						// global var instellen
						strcpy( cmr4hostname, vValue );			// defined in vars.0.9.h
						if ( PrintMessages==1 ) {
							// melding op scherm
							fprintf( stdout, "ServerName = %s\n", vValue );
						}
					} else {
						// ?? onverwacht formaat
						continue;
					}
				}

				if ( strcasecmp(vWord,"Port")==0 ) {
					vWord = strtok(NULL," \x09");
					if ( strcasecmp(vWord,"=")==0 ) {
						// Value lezen..
						vValue = strtok(NULL," \x09");
						// global var instellen
						port = (u_short) atoi(vValue);			// defined in vars.0.9.h
						if ( PrintMessages==1 ) {
							// melding op scherm
							fprintf( stdout, "Port = %s\n", vValue );
						}
					} else {
						// ?? onverwacht formaat
						continue;
					}
				}

				if ( strcasecmp(vWord,"Password")==0 ) {		//	0=geen wachtwoord, 1=wachtwoord gebruikt
					vWord = strtok(NULL," \x09");
					if ( strcasecmp(vWord,"=")==0 ) {
						// Value lezen..
						vValue = strtok(NULL," \x09");
						// global var instellen
						GlobalPassword = (u_char) atoi(vValue);	// defined in vars.0.9.h
						if ( PrintMessages==1 ) {
							// melding op scherm
							fprintf( stdout, "Password = %s\n", vValue );
						}
					} else {
						// ?? onverwacht formaat
						continue;
					}
				}

				if ( strcasecmp(vWord,"GameSpy")==0 ) {			//	0=uit, 1=aan
					vWord = strtok(NULL," \x09");
					if ( strcasecmp(vWord,"=")==0 ) {
						// Value lezen..
						vValue = strtok(NULL," \x09");
						// global var instellen
//						gsEnabled = (u_char) atoi(vValue);		// defined in gamespy.h
						if ( PrintMessages==1 ) {
							// melding op scherm
							fprintf( stdout, "GameSpy = %s\n", vValue );
						}
					} else {
						// ?? onverwacht formaat
						continue;
					}
				}

				if ( strcasecmp(vWord,"ConfigMode")==0 ) {		//	0=uit, 1=aan
					vWord = strtok(NULL," \x09");
					if ( strcasecmp(vWord,"=")==0 ) {
						// Value lezen..
						vValue = strtok(NULL," \x09");
						// global var instellen
						GlobalConfigMode = (u_char) atoi(vValue);	// defined in vars.0.9.h
						if ( PrintMessages==1 ) {
							// melding op scherm
							fprintf( stdout, "ConfigMode = %s\n", vValue );
						}
					} else {
						// ?? onverwacht formaat
						continue;
					}
				}

				if ( strcasecmp(vWord,"NoCuts")==0 ) {			//	0=cuts, 1=nocuts
					vWord = strtok(NULL," \x09");
					if ( strcasecmp(vWord,"=")==0 ) {
						// Value lezen..
						vValue = strtok(NULL," \x09");
						// global var instellen
						GlobalNoCutsEnabled = (u_char) atoi(vValue);	// defined in vars.0.9.h
						if ( PrintMessages==1 ) {
							// melding op scherm
							fprintf( stdout, "NoCuts = %s\n", vValue );
						}
					} else {
						// ?? onverwacht formaat
						continue;
					}
				}

				if ( strcasecmp(vWord,"MaxPlayers")==0 ) {		//	max aantal spelers (incl. de host)
					vWord = strtok(NULL," \x09");
					if ( strcasecmp(vWord,"=")==0 ) {
						// Value lezen..
						vValue = strtok(NULL," \x09");
						// global var instellen
						MaxPlayers = (u_short) atoi(vValue);	// defined in vars.0.9.h
						if ( PrintMessages==1 ) {
							// melding op scherm
							fprintf( stdout, "MaxPlayers = %s\n", vValue );
						}
					} else {
						// ?? onverwacht formaat
						continue;
					}
				}

				if ( strcasecmp(vWord,"CarType")==0 ) {			//	0=Any, 1=4WD, 2=2WD, 3=GroupB, 4=Bonus
					vWord = strtok(NULL," \x09");
					if ( strcasecmp(vWord,"=")==0 ) {
						// Value lezen..
						vValue = strtok(NULL," \x09");
						// global var instellen
						GlobalCarType = (u_char) atoi(vValue);	// defined in vars.0.9.h
						if ( PrintMessages==1 ) {
							// melding op scherm
							fprintf( stdout, "CarType = %s\n", vValue );
						}
					} else {
						// ?? onverwacht formaat
						continue;
					}
				}

				if ( strcasecmp(vWord,"Damage")==0 ) {			//	0=normal, 1=heavy, 2=extreme
					vWord = strtok(NULL," \x09");
					if ( strcasecmp(vWord,"=")==0 ) {
						// Value lezen..
						vValue = strtok(NULL," \x09");
						// global var instellen
						GlobalDamage = (u_char) atoi(vValue);	// defined in vars.0.9.h
						if ( PrintMessages==1 ) {
							// melding op scherm
							fprintf( stdout, "Damage = %s\n", vValue );
						}
					} else {
						// ?? onverwacht formaat
						continue;
					}
				}

				if ( strcasecmp(vWord,"Ranking")==0 ) {			//	0=points, 1=time
					vWord = strtok(NULL," \x09");
					if ( strcasecmp(vWord,"=")==0 ) {
						// Value lezen..
						vValue = strtok(NULL," \x09");
						// global var instellen
						GlobalRanking = (u_char) atoi(vValue);	// defined in vars.0.9.h
						if ( PrintMessages==1 ) {
							// melding op scherm
							fprintf( stdout, "Ranking = %s\n", vValue );
						}
					} else {
						// ?? onverwacht formaat
						continue;
					}
				}

				if ( strcasecmp(vWord,"Rally")==0 ) {			//	14=geen rally maar een stage... 0=JUMPASTIC, etc..
					vWord = strtok(NULL," \x09");
					if ( strcasecmp(vWord,"=")==0 ) {
						// Value lezen..
						vValue = strtok(NULL," \x09");
						// global var instellen
						GlobalRally = (u_char) atoi(vValue);	// defined in vars.0.9.h
						if ( PrintMessages==1 ) {
							// melding op scherm
							fprintf( stdout, "Rally = %s\n", vValue );
						}
					} else {
						// ?? onverwacht formaat
						continue;
					}
				}

				// volgende woord
				vWord = strtok(NULL," \x09");
			}
		}
	}
}