//--- Global States:
#define stateInit				1	//10
#define stateLobby				2	//1
#define stateLobbyWait			3	//2
#define	stateLobbyCheckStage	12	// new
#define stateLobbyCheckedStage	13	//
#define stateLobbyLeave			4	//3
#define stateNumbering			5	//4
#define stateJoinNr				6	//5
#define stateSwitch				7	//6	
#define stateGridReady			8	//7
#define stateGridReadyACK		9	//8
#define stateCountdown			10	//8
#define stateRacing				11	//9




//--- Relay / doorsturen:
u_char					RelayEnabled	= 0;					// 0==niet doorsturen, <>0 == relay alle packets..
u_char*					StrRelayIP		= "192.168.200.233";		// IP-adres waar naartoe wordt doorgestuurd
u_short					RelayPort		= 30001;				// UDP poort
//
int						RelaySocket;
struct sockaddr_in		RelayClient;
//---



//--- MySQL
u_char		sqlEnabled		= 1;				// <>0==tijden opslaan in de MySQL-DB., 0==niet opslaan
u_char*		sqlHost			= "localhost";
u_char*		sqlUser			= "cmr04" ;
u_char*		sqlPassword		= "cmr04" ;
u_char*		sqlDB			= "cmr04";
/*
MYSQL		*CN;
MYSQL_RES	*RS;
MYSQL_ROW	sqlRow;
char		sqlQuery[500];
*///---






u_char *StrProgress[4] = { "|", "/", "-", "\\" };  //  de status-propellor elementen (let op! de dubbele \\ voor \ ivm esc code)
u_short ProgressCount = 0; //een index in StrProgress




// alle internationaliteiten
u_char *StrCountry[40][2] = { 
	{ "sct", "Scotland" }, 
	{ "aus", "Australia" },
	{ "aut", "Austria" },
	{ "bel", "Belgium" },
	{ "bra", "Brazil" },
	{ "cam", "Cameroon" },
	{ "can", "Canada" },
	{ "chi", "Chile" },
	{ "chn", "China" },
	{ "cro", "Croatia" },

	{ "cze", "Czech Republic" },
	{ "den", "Denmark" },
	{ "egy", "Egypt" },
	{ "eng", "England" },
	{ "fin", "Finland" },
	{ "fra", "France" },
	{ "ger", "Germany" },
	{ "gre", "Greece" },
	{ "isl", "Island" },
	{ "ind", "India" },

	{ "ire", "Ireland" },
	{ "ita", "Italy" },
	{ "jpn", "Japan" },
	{ "nld", "Netherlands" },
	{ "nzl", "New Zealand" },
	{ "nor", "Norway" },
	{ "pol", "Poland" },
	{ "por", "Portugal" },
	{ "rom", "Romania" },
	{ "skr", "South Korea" },

	{ "sad", "Saudi Arabia" },
	{ "slo", "Slovenia" },
	{ "saf", "South Africa" },
	{ "spa", "Spain" },
	{ "swe", "Sweden" },
	{ "swi", "Switzerland" },
	{ "tur", "Turkey" },
	{ "usa", "USA" },
	{ "wal", "Wales" },
	{ "UN ", "United Nations" }
	};


// alle stages
u_char *StrStagesValues[8][7] = {	{"100","101","102","103","104","105",""},		// ESP 1..6
									{"110","111","112","113","114","115","116"},	// UK  1..6 + special
									{"120","121","122","123","124","125",""},		// GRC 1..6
									{"130","131","132","133","134","135","136"},	// USA 1..6 + special
									{"140","141","142","143","144","145",""},		// JPN 1..6
									{"150","151","152","153","154","155","156"},	// SWE 1..6 + special
									{"160","161","162","163","164","165","166"},	// AUS 1..6 + special
									{"170","171","172","173","174","175",""}		// FIN 1..6
								};

u_char *StrStages[8][7] = {			{"ESP S1","ESP S2","ESP S3","ESP S4","ESP S5","ESP S6",""},			// ESP 1..6
									{"UK S1", "UK S2", "UK S3", "UK S4", "UK S5", "UK S6", "UK SS"},	// UK  1..6 + special
									{"GRC S1","GRC S2","GRC S3","GRC S4","GRC S5","GRC S6",""},			// GRC 1..6
									{"USA S1","USA S2","USA S3","USA S4","USA S5","USA S6","USA SS"},	// USA 1..6 + special
									{"JPN S1","JPN S2","JPN S3","JPN S4","JPN S5","JPN S6",""},			// JPN 1..6
									{"SWE S1","SWE S2","SWE S3","SWE S4","SWE S5","SWE S6","SWE SS"},	// SWE 1..6 + special
									{"AUS S1","AUS S2","AUS S3","AUS S4","AUS S5","AUS S6","AUS SS"},	// AUS 1..6 + special
									{"FIN S1","FIN S2","FIN S3","FIN S4","FIN S5","FIN S6",""}			// FIN 1..6
						};
u_char *StrVoteStages[8][7] = {		{"ESP1","ESP2","ESP3","ESP4","ESP5","ESP6",""},			// ESP 1..6
									{"UK1", "UK2", "UK3", "UK4", "UK5", "UK6", "UKSS"},		// UK  1..6 + special
									{"GRC1","GRC2","GRC3","GRC4","GRC5","GRC6",""},			// GRC 1..6
									{"USA1","USA2","USA3","USA4","USA5","USA6","USASS"},	// USA 1..6 + special
									{"JPN1","JPN2","JPN3","JPN4","JPN5","JPN6",""},			// JPN 1..6
									{"SWE1","SWE2","SWE3","SWE4","SWE5","SWE6","SWESS"},	// SWE 1..6 + special
									{"AUS1","AUS2","AUS3","AUS4","AUS5","AUS6","AUSSS"},	// AUS 1..6 + special
									{"FIN1","FIN2","FIN3","FIN4","FIN5","FIN6",""}			// FIN 1..6
							};
u_char *StrVoteStages2[8][7] = {	{"ESP 1","ESP 2","ESP 3","ESP 4","ESP 5","ESP 6",""},			// ESP 1..6
									{"UK 1", "UK 2", "UK 3", "UK 4", "UK 5", "UK 6", "UK SS"},		// UK  1..6 + special
									{"GRC 1","GRC 2","GRC 3","GRC 4","GRC 5","GRC 6",""},			// GRC 1..6
									{"USA 1","USA 2","USA 3","USA 4","USA 5","USA 6","USA SS"},	// USA 1..6 + special
									{"JPN 1","JPN 2","JPN 3","JPN 4","JPN 5","JPN 6",""},			// JPN 1..6
									{"SWE 1","SWE 2","SWE 3","SWE 4","SWE 5","SWE 6","SWE SS"},	// SWE 1..6 + special
									{"AUS 1","AUS 2","AUS 3","AUS 4","AUS 5","AUS 6","AUS SS"},	// AUS 1..6 + special
									{"FIN 1","FIN 2","FIN 3","FIN 4","FIN 5","FIN 6",""}			// FIN 1..6
							};
u_char *StrVoteStages3[8][7] = {	{"ESP S1","ESP S2","ESP S3","ESP S4","ESP S5","ESP S6",""},			// ESP 1..6
									{"UK S1", "UK S2", "UK S3", "UK S4", "UK S5", "UK S6", "UK SS"},		// UK  1..6 + special
									{"GRC S1","GRC S2","GRC S3","GRC S4","GRC S5","GRC S6",""},			// GRC 1..6
									{"USA S1","USA S2","USA S3","USA S4","USA S5","USA S6","USA SS"},	// USA 1..6 + special
									{"JPN S1","JPN S2","JPN S3","JPN S4","JPN S5","JPN S6",""},			// JPN 1..6
									{"SWE S1","SWE S2","SWE S3","SWE S4","SWE S5","SWE S6","SWE SS"},	// SWE 1..6 + special
									{"AUS S1","AUS S2","AUS S3","AUS S4","AUS S5","AUS S6","AUS SS"},	// AUS 1..6 + special
									{"FIN S1","FIN S2","FIN S3","FIN S4","FIN S5","FIN S6",""}			// FIN 1..6
							};
u_char *StrVoteStages4[8][7] = {	{"spain S1","spain S2","spain S3","spain S4","spain S5","spain S6",""},
									{"UK S1", "UK S2", "UK S3", "UK S4", "UK S5", "UK S6", "UK SS"},
									{"greece S1","greece S2","greece S3","greece S4","greece S5","greece S6",""},
									{"us S1","us S2","us S3","us S4","us S5","us S6","us SS"},
									{"japan S1","japan S2","japan S3","japan S4","japan S5","japan S6",""},
									{"sweden S1","sweden S2","sweden S3","sweden S4","sweden S5","sweden S6","sweden SS"},
									{"australia S1","australia S2","australia S3","australia S4","australia S5","australia S6","austria SS"},
									{"finland S1","finland S2","finland S3","finland S4","finland S5","finland S6",""}
							};
u_char *StrVoteStages5[8][7] = {	{"spain 1","spain 2","spain 3","spain 4","spain 5","spain 6",""},
									{"UK 1", "UK 2", "UK 3", "UK 4", "UK 5", "UK 6", "UK SS"},
									{"greece 1","greece 2","greece 3","greece 4","greece 5","greece 6",""},
									{"us 1","us 2","us 3","us 4","us 5","us 6","us SS"},
									{"japan 1","japan 2","japan 3","japan 4","japan 5","japan 6",""},
									{"sweden 1","sweden 2","sweden 3","sweden 4","sweden 5","sweden 6","sweden SS"},
									{"australia 1","australia 2","australia 3","australia 4","australia 5","australia 6","australia SS"},
									{"finland 1","finland 2","finland 3","finland 4","finland 5","finland 6",""}
							};
u_char *StrVoteStages6[8][7] = {	{"spain1","spain2","spain3","spain4","spain5","spain6",""},
									{"UK1", "UK2", "UK3", "UK4", "UK5", "UK6", "UKSS"},
									{"greece1","greece2","greece3","greece4","greece5","greece6",""},
									{"us1","us2","us3","us4","us5","us6","usSS"},
									{"japan1","japan2","japan3","japan4","japan5","japan6",""},
									{"sweden1","sweden2","sweden3","sweden4","sweden5","sweden6","swedenSS"},
									{"australia1","australia2","australia3","australia4","australia5","australia6","australiaSS"},
									{"finland1","finland2","finland3","finland4","finland5","finland6",""}
							};
u_char *StrVoteStages7[8][7] = {	{"spain","","","","","",""},
									{"UK", "", "", "", "", "", ""},
									{"greece","","","","","",""},
									{"us","","","","","",""},
									{"japan","","","","","",""},
									{"sweden","","","","","",""},
									{"australia","","","","","",""},
									{"finland","","","","","",""}
							};
u_char *StrVoteStages8[8][7] = {	{"tarmac","","","","","",""},
									{"rain", "", "", "", "", "", ""},
									{"gravel","gre2","","","","",""},
									{"states","","","","","","special"},
									{"hairpins","hairpin","","","","",""},
									{"snow","ice","","","","",""},
									{"dirt","","sand","","","","specials"},
									{"trees","","long","","short","jumps",""}
							};
u_char *StrVoteStages9[8][7] = {	{"spa1","spa2","spa3","spa4","spa5","spa6","spass"},
									{"vk1", "vk2", "vk3", "vk4", "vk5", "vk6", "vkss"},
									{"hellas1","hellas2","hellas3","hellas4","hellas5","hellas6","hellasss"},
									{"vs1","vs2","vs3","vs4","vs5","vs6","vs7"},
									{"jap1","jap2","jap3","jap4","jap5","jap6","japss"},
									{"sve1","sve2","sve3","sve4","sve5","sve6","svess"},
									{"downunder","","","","","",""},
									{"fld","","","","","",""}
							};

u_char *StrVoteStages10[8][7] = {	{"spa 1","spa 2","spa 3","spa 4","spa 5","spa 6","spa ss"},
									{"vk 1", "vk 2", "vk 3", "vk 4", "vk 5", "vk 6", "vk ss"},
									{"hellas 1","hellas 2","hellas 3","hellas 4","hellas 5","hellas 6","hellas ss"},
									{"vs 1","vs 2","vs 3","vs 4","vs 5","vs 6","vs 7"},
									{"jap 1","jap 2","jap 3","jap 4","jap 5","jap 6","jap ss"},
									{"sve 1","sve 2","sve 3","sve 4","sve 5","sve 6","sve ss"},
									{"downunder","","","","","",""},
									{"fld","","","","","",""}
							};

//alle rallies
//indien rally==14 dan worden er stages gereden, geen rally..
u_char *StrRallies[16] = {	"JUMPTASTIC", "TARMAC ATTACK", "GRAVELFEST", 
							"Custom Rally A", "Custom Rally B",
							"ESP", "UK", "GRC", "USA", "JPN", "SWE", "AUS", "FIN", "Super Specials",
							"", "" };	//stages

u_char *StrRalliesValue[16] = { "0","1","2","3","4","5","6","7","8","9","10","11","12","13", "14", "15" };

int RallyJUMPTASTIC[6][2] = {		{ 6,4 },	//country, stage		//AUS S5
									{ 7,2 },							//FIN S3
									{ 7,4 },							//FIN S5
									{ 2,3 },							//GRC S4
									{ 7,5 },							//FIN S5
									{ 6,1 } };							//AUS S2

int RallyTARMACATTACK[6][2] = {		{ 1,2 },							//UK S3
									{ 4,1 },							//JPN S2
									{ 4,2 },							//JPN S3
									{ 0,2 },							//ESP S3
									{ 0,4 },							//ESP S5
									{ 0,5 } };							//ESP S6

int RallyGRAVELFEST[6][2] = {		{ 2,0 },							//GRC S1
									{ 2,1 },							//GRC S2
									{ 1,4 },							//UK S5
									{ 3,3 },							//USA S3
									{ 6,2 },							//AUS S3
									{ 7,1 } };							//FIN S2

int RallySuperSpecials[6][2] = {	{ 1,6 },							//UK SS
									{ 3,6 },							//USA SS
									{ 5,6 },							//SWE SS
									{ 6,6 },							//AUS SS
									{ -1,-1 },							//leeg
									{ -1,-1 } };						//leeg


int RallyCustomA[6][2] = {			{ -1,-1 },							//leeg
									{ -1,-1 },							//leeg
									{ -1,-1 },							//leeg
									{ -1,-1 },							//leeg
									{ -1,-1 },							//leeg
									{ -1,-1 } };						//leeg

int RallyCustomB[6][2] = {			{ -1,-1 },							//leeg
									{ -1,-1 },							//leeg
									{ -1,-1 },							//leeg
									{ -1,-1 },							//leeg
									{ -1,-1 },							//leeg
									{ -1,-1 } };						//leeg




// alle auto's
u_char *StrCar[23] = { "Citroen XSara", "Ford Focus", "Subaru Impreza", "Peugeot 206", "Mitsubishi Lancer", 
	"MG ZR", "Citroen Saxo", "Volkswagen Golf", "Ford Puma", "Fiat Punto", 
	"Audi Quattro",	"", "Lancia 037", "Peugeot 205", "Ford RS200",
	"MGC GTS", "Citroen 2CV", "Ford Escort", "", "Lancia Delta", "Ford Transit", "Mitsubishi Pajero", "Subaru Impreza" };
//		x			x				x					19				20				21					22
u_char *StrClass[23] = { "4WD", "4WD", "4WD", "4WD", "4WD", 
	"2WD", "2WD", "2WD", "2WD", "2WD", 
	"Group B",	"", "Group B", "Group B", "Group B",
	"Bonus", "Bonus", "Bonus", "", "Bonus", "Bonus", "Bonus", "Bonus" };

// om op de auto's te kunnen stemmen //test
u_char *StrVoteCar[23] = { "XSara", "Focus", "Impreza", "206", "Lancer", 
	"MG ZR", "Saxo", "Golf", "Puma", "Punto", 
	"Quattro",	"", "037", "205", "RS200",
	"GTS", "2CV", "Escort", "", "Delta", "Transit", "Pajero", "Subaru" };


//
u_char *StrCarType[5] = { "Any", "4WD", "2WD", "Group B", "Bonus" };
u_char *StrCarTypeValue[5] = { "0", "1", "2", "3", "4" };

u_char *StrTransmission[3] = { "Auto", "Semi-Auto", "Manual" };
u_char *StrTransmissionValue[3] = { "0", "1", "2" };

u_char *StrPassword[2] = { "No PWD", "PWD" };
u_char *StrPasswordValue[2] = { "0", "1" };

u_char *StrGameType[3] = { "LAN", "Online", "Fixed" };
u_char *StrGameTypeValue[3] = { "0", "1", "2" };

u_char *StrGameMode[2] = { "openwaiting", "closedplaying" };

u_char *StrDamage[3] = { "Normal", "Heavy", "Extreme" };
u_char *StrDamageValue[3] = { "0", "1", "2" };

u_char *StrRanking[2] = { "Points", "Time" };
u_char *StrRankingValue[2] = { "0", "1" };

u_char *StrConnectionType[4] = { "Modem", "ISDN", "Broadband", "LAN" };
u_char *StrConnectionTypeValue[4] = { "0", "1", "2", "3" };





// teksten
u_char *StrWelcome[128];  ///			= "Welcome at UJE";
u_char *StrVoteKick			= " voted to kick ";
u_char *StrVoteStatus		= "not yet implemented.. ";

u_char *StrMySQLerror		= "mysql error..";

u_char *StrReplaceGogogoD6[256];
//u_char *StrReplaceGogogoD6;//	= "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" ;	// dit ziet de persoon zelf
u_char *StrReplaceGogogoC6	= "help";					// dit zien de anderen

u_char *StrDSVersion		= "Linux Dedicated Server v0.9";
u_char *StrHelp				= "Say \"help\" to display extra help";
u_char *StrVoting			= "Say \"vote \"<stage>\" to vote for a stage e.g.  \"vote USA5\"..";
u_char *StrRules			= "No shortcuts, or get retired..";
u_char *StrStats			= "Stats at http://cmr04.fastfrank.speedxs.nl" ;







/*
struct tHostConfig {
	u_char *ServerName;								//the server name
	u_char *PortNr;									//de host-poort
	u_char *HostNick;								//hosts nickname (player 0)
	u_char *Password;								//server password
	u_char  MaxPlayers;
	u_char  NumPlayers;
	u_char *GameVer;								//"1.0"
	u_char  GameMode;								//LAN,online,fixed
	u_char  Rally;									//rally nummer??
	u_char  Stages;									//100 + AB     waarbij geldt: A=Land-1 & B=Stage-1
	u_char  Damage;									//normal,heavy,extreme
	u_char  Ranking;								//points,time
	u_char  CarType;								//4wd,2wd,groupB,bonus,any

	u_char	Status;

	u_long  ChatLine;								//aantal regels ge-chat
	struct tPlayer *ptr_Players;					//the list of players
}; // *HostConfig;
struct tHostConfig *HostConfig;
*/





	
	
	u_char*	cmr4hostname		= CMR4HOSTNAME;	//variables
	u_char*	cmr4hostnick		= CMR4HOSTNICK;

	u_char*	cmr4password		= CMR4PASSWORD;
	u_char	GlobalPassword		= 0;			// 0=geen wachtwoord, 1=wachtwoord gebruikt
	
	u_char	GlobalGameType		= 1;			// 0=LAN, 1=INET, 2=FIXED
	u_char	GlobalGameMode		= 0;			// 0=openwaiting, 1=closedplaying

	u_char	GlobalRally			= 14;			// 14=geen rally maar een stage... 0=JUMPASTIC, etc..
	u_char	GlobalStages		= 134;			// waarde: 100 + Land*10 + Stage
	u_char	GlobalStages_Land	= 3;			// 0=ESP, 1=UK, 2=GRC, 3=USA, 4=JPN, 5=SWE, 6=AUS, 7=FIN  (1e index in StrStagesValues & StrStages)
	u_char	GlobalStages_Stage	= 4;			// 0=Stage 1, 1=S2, 2=S3, 3=S4, 4=S5, 5=S6, 6=Special Stage
	u_char	GlobalStagesPrev	= 0;			// de vorige stage ivm random kiezen
/*
	u_char	GlobalRally			= 0;			// 14=geen rally maar een stage... 0=JUMPASTIC, etc..
	u_char	GlobalStages		= 100;			// waarde: 100 + Land*10 + Stage
	u_char	GlobalStages_Land	= 0;			// 0=ESP, 1=UK, 2=GRC, 3=USA, 4=JPN, 5=SWE, 6=AUS, 7=FIN  (1e index in StrStagesValues & StrStages)
	u_char	GlobalStages_Stage	= 0;			// 0=Stage 1, 1=S2, 2=S3, 3=S4, 4=S5, 5=S6, 6=Special Stage
*/
	u_char	GlobalCarType		= 0;			// 0=Any, 1=4WD, 2=2WD, 3=GroupB, 4=Bonus
	u_char	GlobalRanking		= 1;			// 0=points, 1=time
	u_char	GlobalDamage		= 2;			// 0=normal, 1=heavy, 2=extreme
	u_long	GlobalJoinCount		= 0x00000000;	// globale teller voor aantal joins zover
	int		GlobalState			= 0;			// status van de game host/server

	u_long	GlobalStartTimeVal	= 0x00000000;	// de timevalue op het tijdstip van de race-start
	u_long	GlobalFinishTimeVal	= 0x00000000;	// de timevalue op het tijdstip van 1e die finisht
	
	// relevant tijdens de race 
	u_char	GlobalPlayersFinished = 0;			// het aantal spelers finished tot dusver..
	u_char	GlobalPlayersRetired = 0;			// het aantal spelers retired tot dusver..
	u_char	GlobalPlayersQuit = 0;				// het aantal spelers quit tot dusver..

	u_char	GlobalShowList		= 1;			// 1=speler-info lijst afbeelden, 0=niet afbeelden
	u_char	GlobalShowTiming	= 0;			// 1=lijst met tijden afbeelden, 0=niet afbeelden
	u_char	GlobalShowMessages	= 1;			// 1=meldingen afbeelden, 0=niet afbeelden
	u_char	GlobalShowPackets	= 0;			// 1=ontvangen packets afbeelden, 0=niet afbeelden

	u_char	GlobalVotingEnabled	= 1;			// voting aan/uit schakelen
	u_char	GlobalShowVoteKicks	= 0;			// 1="vote kick"s zijn zichtbaar voor anderen, 0="vote kick"s zijn anomiem..
	u_char	GlobalVoteOnce		= 0;			// 1=maar 1 keer kunnen stemmen in de lobby, 0=blijf maar stemmen (ready==go)
												//  (0: na elke vote complete, de player-votes resetten om opnieuw te kunnen stemmen)
	u_char	GlobalConfigMode	= 0;			// NoCuts configuratie-modus
	u_char	GlobalNoCutsEnabled	= 1;			// no-cuts controle actief??
	u_char	GlobalSendSplits	= 1;			// no-cuts controle actief??

	u_char	GlobalReplayMode	= 0;			// om opgenomen races te controleren. (waarden: 0 of 1)
	u_long	GlobalReplayFrames	= 350;			// het aantal frames om te loopen
	u_long	GlobalReplayFrame	= 0;			// het huidige frame
	u_long	GlobalReplayPlayerTime = 0;			// 2 te onthouden tijden van speler en host
	u_long	GlobalReplayHostTime = 0;			//

	u_char	GlobalHTTPEnabled	= 1;			// webserver interface

	// tbv timers
	u_long						LastTimeValue;
	u_long						LastTimeValue2;
	u_long						LastTimeValue3;
	u_long						LastTimeValueSendBuf;
/*	struct timezone {
		int  tz_minuteswest; // minutes W of Greenwich
		int  tz_dsttime;     // type of dst correction
	};*/



	u_short						MaxPlayers	= MAXPLAYERS;
	u_short						NumPlayers	= 0;


    static struct sockaddr_in	peerOut;
    static struct sockaddr_in	peer2;
    static struct sockaddr_in	peer;
    static struct sockaddr_in	peerRA;
    struct sockaddr_storage		peerin;
    struct sockaddr_storage		peerout;
    u_int						seed;
    int							sd,
								sdRA,
								sockOut,
								pid,
								i,
								len,
								on = 1;
    int							inTimeout = 100;
    u_short						port = PORT;
    u_char						RXPacket[BUFFSZ];	// received packet buffer
    u_char						TXPacket[BUFFSZ];	// the packet to transmit

	u_long						Offset = 0;


	// strings tbv. ServerInfo
	u_char			strMaxPlayers[6]	= "00000\x00";
	u_char			strNumPlayers[6]	= "00000\x00";
	u_char			strPort[6]			= "00000\x00";

	u_char			*tmpStr[1024];

	u_char			*BufHTTP[HTTPBUFFSZ];


	u_long			clHandle = 0;
	int				clCRC = 0;
	int				PacketLen = 0;
	u_long			packetCount = 0;
	u_long			PosSentCount = 0;					// aantal keer posities verstuurd.
	u_long			TotalPacketCount	= 0;			// totaal aantal packets ontvangen.
	u_char			PacketProcessed		= 0;			// waarden: 0=niet verwerkt, 1=wel.
	u_char			PacketPrinted		= 0;			// waarden: 0=niet afgebeeld, 1=wel.

	struct in_addr	BanAddrIn;
	
	struct in_addr	playerAddrIn;
	u_long			playerIP = 0;				// "0.0.0.0\x00";
	u_int			playerPort = 0;
	short			playerNumber = -1;
	int				playerFound = 0;
	u_short			playerCMD;
	short			LastPlayerNr = -1;
	u_short			LastPlayerRX3 = 0x00;
	u_short			LastPlayerRX4 = 0x00;


	// wat algemene variabelen
	int				tmpInt;
	short			tmpShort;
	u_short			tmpUShort;
	u_long			tmpULong;
	u_char			tmpUChar;
	char			tmpChar;
	float			tmpFloat;
	u_char			tmpString[1024];
	u_char			tmpString2[1024];
	u_char*			ptrString;
	
	u_short			tmpCMD;
	u_long			tmpPing;
	u_char			tmpPlayerNr;
	u_char			tmpSplit;
	u_long			tmpTime;



// test tmp
	u_char*	IP;
	u_char*	Nick;
	u_char*	Country;
	u_long	MemberID;
	u_char	Car;
	u_long	Split0;
	u_long	Split1;
	u_long	Split2;
	u_long	RaceTime;

/*
	// de willekeurig gekozen banen die het laatst zijn gereden..
	u_char StagesDone[52];
*/


// een stage-cycle: stages achter elkaar rijden..niet willekeurig..
/*
// alle landen op volgorde.
u_char GlobalStageCycle[52] = {	100,101,102,103,104,105,		// ESP 1..6
								110,111,112,113,114,115,116,	// UK  1..6 + special
								120,121,122,123,124,125,		// GRC 1..6
								130,131,132,133,134,135,136,	// USA 1..6 + special
								140,141,142,143,144,145,		// JPN 1..6
								150,151,152,153,154,155,156,	// SWE 1..6 + special
								160,161,162,163,164,165,166,	// AUS 1..6 + special
								170,171,172,173,174,175			// FIN 1..6
							};
*/
// alle rallies eerst, en dan de rest
u_char GlobalStageCycle[52] = {	164,172,174,123,175,161,		// JUMPTASTIC
								112,141,142,102,104,105,		// TARMAC ATTACK
								120,121,114,133,162,171,		// GRAVEL FEST
								116,136,156,166,				// SUPER SPECIALS

								100,153,132,145,101,151,		// de rest is willekeurig..
								103,110,155,130,125,111,
								160,124,173,135,113,150,
								115,144,122,163,143,152,
								131,170,165,154,140,134
							};

u_char GlobalStageCycleCount = 52;		// aantal banen in de cycle
u_char GlobalStageCycleCounter = 0;		// teller voor de huidige te rijden baan in de cycle
u_char GlobalStageCycleLastCounter = 0;	// onthouden
u_char GlobalStageCycleEnabled = 1;		// stage-cycle aan/uit-schakelen
