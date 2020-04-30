

struct tRotation {
	u_long	X;
	u_long	Y;
	u_long	Z;
};

struct tPosition {
	u_long	X;
	u_long	Y;
	u_long	Z;
};

struct tPlayer {
	u_long				ID;						// het nummer verkregen tijdens joinen

	u_long				TimeID;					// de ID in CMR04_Times
	u_long				MemberID;				// de ID in CMR04_Members

	u_char				isBOT;					// 0=human, 1=bot //not used

	u_long				IP;						// IP nummer als een long
	struct in_addr		AddrIn;					// adres als struct addr_in
	u_int				PortNr;
	u_char				Nick[16];				// 15 chars + trailing x00
	u_char				Country;
	u_char				Car;
	u_char				CarType;
	u_char				Gearbox;
	u_char				Damage;
	u_char				ConnectionSpeed;
	u_long				Ping;
	u_long				Ready;					// 0=not ready yet, 1=ready
	u_short				ChatLine;
	u_short				LastCMD;				// het laatst ontvangen commando
	short				PacketLen;				// lengte van pakketje in Packet.
	int					State;					// de vooruitgang in het spel 

	// ring-buffer  /  roterende buffer
	short				PtrBufferRead;// index in de array Packet[PtrBufferRead][BUFFSZ]
	short				PtrBufferWrite;			// index in de array Packet[PtrBufferWrite][BUFFSZ]
	//
	short				BufferedPackets;		// het aantal buffer dat momenteel een packet bevat.
	short				BufferLen[MAXBUFF];		// het aantal bytes in elke buffer (in gebruik)
	u_short				BufferCMD[MAXBUFF];		// het commando dat hoort bij het packet in de buffer
	int					BufferCountdown[MAXBUFF];// aftellen hoeveel keer nog opnieuw te verzenden
	u_char				Packet[MAXBUFF][BUFFSZ];// het(/de) opnieuw te versturen pakketje(s).
	
	struct tRotation	Rotation;	//obsolete??!!
	struct tPosition	Position;	//  "   "


	long				PosCount;				// aantal posities opgenomen
	u_char				PosRec[BLOB_LEN];   ///	// de opgenomen posities (LastPos records) van een hele race..
	u_long				LastPosRecSent[MAXPLAYERS];  // de index van de laatst verstuurde PosRec naar (per) player
	u_char				PosSplitSent[4];			// intermediate van host aan speler verzonden
	
	u_char				LastPos[27];			// het laatste in-game Position-packet..
	// Position-packet naar andere spelers al verzonden? lijst
	u_char				PosSent[MAXPLAYERS];	// 0=niet verzonden naar anderen, 1=al verzonden
	u_long				PosReceived;			// >0==al een positie-packet ontvangen van deze speler, 0==nog niets ontvangen..
//test
	u_char				LastPosBuf[MAXLASTPOS][27];	// De MAXLASTPOS laatste in-game Position-packet..[0]==laatst ontvangen (jongste packet), [PosReceived]==oudste packet in buffer..
	u_long				LastPosSentTime[MAXPLAYERS];// de timestamp van de laatst verzonden positie naar een speler..
///test

	u_char				NickC6D6;				// D6=nick opgegeven met een D6-81, anders met een C6-81
	u_char				ReceivedC60E;			// 0==nog geen c6-0e ontvangen, <>0==wel

	// laatste keer sinds ready onthouden
	u_long				LastTimeReady;
	u_char				ReadyWarning;

	// NO-CUTS
	float				LastPosX;				// de vorige 3D positie X-coördinaat
	float				LastPosY;				// de vorige 3D positie Y-coördinaat
	float				LastPosZ;				// de vorige 3D positie Z-coördinaat

	// 2 timestamps uit het ontvangen packet..
	u_long				Ping1;
	u_long				Ping2;
	u_long				PingCount;
	u_long				LastTimeValue;
	u_long				AvgPing;				// gemiddelde ping

	u_char				StartNr;				// het PlayerNr geldig tijdens de race (verandert dan niet)
	u_char				StartPlayerNr;			// het PlayerNr op het moment van race-start (verandert dan niet meer)
	u_char				hasFinished;			// 0=niet gefinished, anders gefinished als #hasFinished
	u_char				hasRetired;				// 0=niet retired, 1=retired
	u_char				hasQuit;				// 0=niet ge-quit, 1=quitted

	// race-tijden en splits..
	u_char				SplitsDone;				// teller hoeveel splits er zijn gepasseerd.
	u_long				SplitTime[4];			// 4 split-tijden  (3 splits + finish)
	u_long				StartTime;				// timestamp in de eerste ontvangen Pos-packet van deze client (na racestart C6-04)
	u_long				RaceTime;				// eind- / totaal-tijd
	u_char				PercToSplit0;
	u_char				Percentage;				// percentage gereden [0..100] %

	// huidige vote waarden..
	u_char				VoteStage;				// een track prefereren..
	u_long				VoteKick;				// een speler (met deze ID) wegcijferen..
	u_char				VoteGhost;				// de host-bot rijdt het record van deze klasse (4WD,2WD,GB,Bonus)
};
struct	tPlayer *ptr_players;

// Een lijst met de player-indexen..allemaal "leeg" nog..
short PlayerIndex[8] = { -1,-1,-1,-1,-1,-1,-1,-1 };







// speler eigenschappen opvragen..
u_char*	getIP( short PlayerNr );
u_long	getID( short PlayerNr );
u_long	getMemberID( short PlayerNr );
u_int	getPortNr( short PlayerNr );
u_char	*getNick( short PlayerNr );
u_char	getCountry( short PlayerNr );
u_char	getCar( short PlayerNr );
u_char	getCarType( short PlayerNr );
u_char	getGearbox( short PlayerNr );
u_char	getConnectionSpeed( short PlayerNr );
u_long	getPing( short PlayerNr );
u_long	getPingCount( short PlayerNr );
u_long	getAvgPing( short PlayerNr );
u_long	getReady( short PlayerNr );
u_short	getChatLine( short PlayerNr );
u_short getLastCMD( short PlayerNr );
u_long	getSplitTime( short PlayerNr, u_char SplitNr ); //0..3
u_long	getRaceTime( short PlayerNr );
u_char	getSplitsDone( short PlayerNr );
u_char	getPercToSplit0( short PlayerNr );
u_char	getStartNr( short PlayerNr );
u_char	getStartPlayerNr( short PlayerNr );
u_char	getFinished( short PlayerNr );
u_char	getRetired( short PlayerNr );
u_char	getQuit( short PlayerNr );
u_char	getVoteStage( short PlayerNr );
u_long	getVoteKick( short PlayerNr );
u_char	getVoteGhost( short PlayerNr );
struct tRotation getRotation( short PlayerNr );
struct tPosition getPosition( short PlayerNr );
float	getLastPosX( short PlayerNr );
float	getLastPosY( short PlayerNr );
float	getLastPosZ( short PlayerNr );

// speler eigenschappen veranderen..
void changeID(  short PlayerNr, u_long aID );
void changeMemberID(  short PlayerNr, u_long aMemberID );
void changePortNr( short PlayerNr, u_int aPortNr );
void changeNick( short PlayerNr, u_char *aNickname );
void changeCountry( short PlayerNr, u_char aCountry );
void changeCar( short PlayerNr, u_char aCar );
void changeCarType( short PlayerNr, u_char aCarType );
void changeGearbox( short PlayerNr, u_char aGearbox );
void changeConnectionSpeed( short PlayerNr, u_char aConnectionSpeed );
void changePing( short PlayerNr, u_long aPing );
void changePingCount( short PlayerNr, u_long aPingCount );
void changeAvgPing( short PlayerNr, u_long aPing );
void changeReady( short PlayerNr, u_long aReady );
void changeChatLine( short PlayerNr, u_short aChatLine );
void increaseChatLine( short PlayerNr );
void increaseChatLines();
void changeLastCMD( short PlayerNr, u_short CMD );
void changeSplitTime( short PlayerNr, u_char SplitNr, u_long aSplitTime );
void changeRaceTime( short PlayerNr, u_long aRaceTime );
void changeSplitsDone( short PlayerNr, u_char aSplitsDone );
void increaseSplitsDone( short PlayerNr );
void changePercToSplit0( short PlayerNr, u_char BCD );
void changeStartNr( short PlayerNr, u_char aStartNr );
void changeStartPlayerNr( short PlayerNr, u_char aStartPlayerNr );
void changeFinished( short PlayerNr, u_char aFinished );
void changeRetired( short PlayerNr, u_char aRetired );
void changeQuit( short PlayerNr, u_char aQuit );
void changeVoteStage( short PlayerNr, u_char aVoteStage );
void changeVoteKick( short PlayerNr, u_long aVoteKick );
void changeVoteGhost( short PlayerNr, u_char aVoteGhost );
void changePosition( short PlayerNr, u_long aX, u_long aY, u_long aZ );
void changeRotation( short PlayerNr, u_long aX, u_long aY, u_long aZ );
void changeState( short PlayerNr, int aState );
void changeLastPosX( short PlayerNr, float aLastPosX );
void changeLastPosY( short PlayerNr, float aLastPosY );
void changeLastPosZ( short PlayerNr, float aLastPosZ );


// de positie-buffer vullen en aanschuiven..
void AddLastPos( short PlayerNr );
// de index in de buffer teruggeven, van de positie waarvan de tijd <= aan de opgegeven timestamp..
// resultaat == -1 als er geen geldige positie/tijd is gevonden..
int GetLastPos( short PlayerNr, u_long aTimeStamp );


// spelers zoeken/toevoegen/verwijderen..
short FindPlayer(u_long aIP, u_int aPortNr);
short AddPlayer(struct in_addr aIP, u_int aPortNr);
short DeletePlayer(short PlayerNr);
void ClearPlayer(short SlotNr);





// De speler-IP opvragen als een string
//
u_char*	getIP( short PlayerNr ) {
	if (PlayerNr<1 || PlayerNr>=NumPlayers) return "";
	return inet_ntoa(ptr_players[PlayerIndex[PlayerNr]].AddrIn);
}

// De speler-ID opvragen
//
u_long getID(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].ID;
}

// De speler-MemberID opvragen
//
u_long	getMemberID( short PlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].MemberID;
}

// De speler-PortNr opvragen
//
u_int getPortNr( short PlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].PortNr;
}
// De speler-naam opvragen
//
u_char *getNick(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return "";
	return &ptr_players[PlayerIndex[PlayerNr]].Nick;
}

// Het speler-land opvragen
//
u_char getCountry(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].Country;
}

// De speler-auto opvragen
//
u_char getCar(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].Car;
}

// Het speler-auto-type opvragen
//
u_char getCarType(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].CarType;
}

// De speler-transmissie opvragen
//
u_char getGearbox(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].Gearbox;
}

// De speler-connectionspeed opvragen
//
u_char getConnectionSpeed( short PlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].ConnectionSpeed;
}

// De speler-ping opvragen
//
u_long getPing(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0x00000000;
	return ptr_players[PlayerIndex[PlayerNr]].Ping;
}

// De speler-pingcount opvragen
//
u_long getPingCount(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0x00000000;
	return ptr_players[PlayerIndex[PlayerNr]].PingCount;
}

// De speler gemiddelde ping opvragen
//
u_long getAvgPing(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0x00000000;
	return ptr_players[PlayerIndex[PlayerNr]].AvgPing;
}

// De speler-ready-status opvragen
//
u_long getReady(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0x00000000;  //niet klaar..
	return ptr_players[PlayerIndex[PlayerNr]].Ready;
}

// De speler-ChatLine opvragen
//
u_short getChatLine(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].ChatLine;
}

// De speler-LastCMD opvragen
//
u_short getLastCMD( short PlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].LastCMD;
}


// Een speler's split-tijd opvragen
//
u_long	getSplitTime( short PlayerNr, u_char SplitNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers || SplitNr<0 || SplitNr >3) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].SplitTime[SplitNr];
}

// De speler's actuele race-tijd opvragen
//
u_long	getRaceTime( short PlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].RaceTime;
}

// Het speler's aantal gepasseerde splits opvragen
//
u_char	getSplitsDone( short PlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].SplitsDone;
}

// De speler's percentage tot aan de volgende split opvragen
//
u_char	getPercToSplit0( short PlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].PercToSplit0;
}

// speler Start-nummer opvragen (PlayerNr tijdens de race)
//
u_char	getStartNr( short PlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0xFF; //ongeldig
	return ptr_players[PlayerIndex[PlayerNr]].StartNr;
}

// speler Start-playerNummer opvragen (PlayerNr bij start-race)
//
u_char	getStartPlayerNr( short PlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0xFF; //ongeldig
	return ptr_players[PlayerIndex[PlayerNr]].StartPlayerNr;
}

// speler-Finished opvragen
//
u_char	getFinished( short PlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 1;
	return ptr_players[PlayerIndex[PlayerNr]].hasFinished;
}

// speler-hasRetired opvragen
//
u_char	getRetired( short PlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 1;
	return ptr_players[PlayerIndex[PlayerNr]].hasRetired;
}

// speler-hasQuit opvragen
//
u_char	getQuit( short PlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 1;
	return ptr_players[PlayerIndex[PlayerNr]].hasQuit;
}

// De speler-VoteStage opvragen
//
u_char	getVoteStage( short PlayerNr ) { 
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].VoteStage;
}

// De speler-VoteKick opvragen
//
u_long	getVoteKick( short PlayerNr ) { 
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].VoteKick;
}

// De speler-VoteRecordGhost opvragen
//
u_char	getVoteGhost( short PlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 1;
	return ptr_players[PlayerIndex[PlayerNr]].VoteGhost;
}

// De speler-Rotatie opvragen
//
struct tRotation getRotation(short PlayerNr) {
//	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].Rotation;
}

// De speler-Positie opvragen
//
struct tPosition getPosition(short PlayerNr) {
//	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].Position;
}

int getState( short PlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0;
	return ptr_players[PlayerIndex[PlayerNr]].State;
}

// de laatste positie X-coord opvragen
float getLastPosX( short PlayerNr ) {
	if (PlayerNr>=8) return ptr_players[PlayerNr].LastPosX;
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0.0f;
	return ptr_players[PlayerIndex[PlayerNr]].LastPosX;
}
// de laatste positie Y-coord opvragen
float getLastPosY( short PlayerNr ) {
	if (PlayerNr>=8) return ptr_players[PlayerNr].LastPosY;
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0.0f;
	return ptr_players[PlayerIndex[PlayerNr]].LastPosY;
}
// de laatste positie Z-coord opvragen
float getLastPosZ( short PlayerNr ) {
	if (PlayerNr>=8) return ptr_players[PlayerNr].LastPosZ;
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return 0.0f;
	return ptr_players[PlayerIndex[PlayerNr]].LastPosZ;
}






// De speler-ID veranderen
//
void changeID(short PlayerNr, u_long aID) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].ID = aID;
}

// De speler-MemberID veranderen
//
void changeMemberID(  short PlayerNr, u_long aMemberID ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].MemberID = aMemberID;
}



void changePortNr( short PlayerNr, u_int aPortNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].PortNr = aPortNr;
}

// De speler-naam veranderen
//
void changeNick(short PlayerNr, u_char *aNickname) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	strcpy(ptr_players[PlayerIndex[PlayerNr]].Nick, aNickname);
}

// Het speler-land veranderen
//
void changeCountry(short PlayerNr, u_char aCountry) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].Country = aCountry;
}

// De speler-auto veranderen
//
void changeCar(short PlayerNr, u_char aCar) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].Car = aCar;
}

// De speler-auto-type veranderen
//
void changeCarType(short PlayerNr, u_char aCarType) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].CarType = aCarType;
	// de host-ghost instellen voor dezelfde klasse
	ptr_players[PlayerIndex[PlayerNr]].VoteGhost = aCarType;
}

// De speler-transmissie veranderen
//
void changeGearbox(short PlayerNr, u_char aGearbox) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].Gearbox = aGearbox;
}

// De speler-connectionspeed veranderen
//
void changeConnectionSpeed( short PlayerNr, u_char aConnectionSpeed ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].ConnectionSpeed = aConnectionSpeed;
}

// De speler-Ping veranderen
//
void changePing(short PlayerNr, u_long aPing) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].Ping = aPing;
	if ( ptr_players[PlayerIndex[PlayerNr]].PingCount == 2 ) {
		changeAvgPing( PlayerNr, aPing );
	}
	// de gemiddelde ping
	if ( aPing < 10000 ) {
		ptr_players[PlayerIndex[PlayerNr]].PingCount++;

		u_long	PingCnt;
		PingCnt = ptr_players[PlayerIndex[PlayerNr]].PingCount;
		float mpA, mpB;
		mpA = ( (PingCnt - 1) / PingCnt );
		mpB = ( 1/ PingCnt );

		//a = (getAvgPing(PlayerNr) * (( (ptr_players[PlayerIndex[PlayerNr]].PingCount-1) / ptr_players[PlayerIndex[PlayerNr]].PingCount )) + (aPing / ptr_players[PlayerIndex[PlayerNr]].PingCount));//      /ptr_players[PlayerIndex[PlayerNr]].PingCount) *  ;
		//b = (aPing / ptr_players[PlayerIndex[PlayerNr]].PingCount);
		//changeAvgPing( PlayerNr, (u_long ) ( (mpA * getAvgPing(PlayerNr) ) + ( mpB * aPing ) )  );
		//changeAvgPing( PlayerNr, (u_long ) (a + b) );
		//changeAvgPing( PlayerNr, (u_long ) (getAvgPing(PlayerNr) * (( (ptr_players[PlayerIndex[PlayerNr]].PingCount-1) / ptr_players[PlayerIndex[PlayerNr]].PingCount )) + (aPing / ptr_players[PlayerIndex[PlayerNr]].PingCount)) );
		//changeAvgPing( PlayerNr, (u_long ) ( (( (PingCnt - 1) / PingCnt ) * getAvgPing(PlayerNr) ) + ( ( 1/ PingCnt ) * aPing ) )  );
		changeAvgPing( PlayerNr, ( 0.8 * getAvgPing(PlayerNr) + 0.2 * aPing ) );
	}
}

// De speler-PingCount veranderen
//
void changePingCount(short PlayerNr, u_long aPingCount) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].PingCount = aPingCount;
}

// De speler-Gemiddelde Ping veranderen
//
void changeAvgPing(short PlayerNr, u_long aPing) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].AvgPing = aPing;
}

// De speler-ChatLine veranderen
//
void changeChatLine(short PlayerNr, u_short aChatLine) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].ChatLine = aChatLine;
//	ptr_players[PlayerIndex[PlayerNr]].LastCMD = aChatLine-1; //laatste waarde onthouden..
}

// De speler-ChatLine verhogen
//
void increaseChatLine(short PlayerNr) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].ChatLine++;
}
void increaseChatLines() {
	int i=0;
	for (i=0;i<NumPlayers;i++) {
		ptr_players[PlayerIndex[i]].ChatLine++;
	}
}

// De speler-LastCMD veranderen
//
void changeLastCMD( short PlayerNr, u_short CMD ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].LastCMD = CMD;
}

// Een speler's Split-tijd veranderen
//
void changeSplitTime( short PlayerNr, u_char SplitNr, u_long aSplitTime ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers || SplitNr<0 || SplitNr >3) return;
	ptr_players[PlayerIndex[PlayerNr]].SplitTime[SplitNr] = aSplitTime;
}

// De speler's actuele race-tijd veranderen
//
void changeRaceTime( short PlayerNr, u_long aRaceTime ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].RaceTime = aRaceTime;
}

// Het speler's aantal gepasseerde splits veranderen
//
void changeSplitsDone( short PlayerNr, u_char aSplitsDone ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].SplitsDone = aSplitsDone;
}

void increaseSplitsDone( short PlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].SplitsDone++;
}


// De speler's percentage tot aan de volgende split veranderen
// de opgegeven waarde voor argument BCD is een "Binary Coded Decimal"
void changePercToSplit0( short PlayerNr, u_char BCD ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].PercToSplit0 = (BCD >> 4)*10 + (BCD & 0x0F);
}

// Het speler StartNr waarde veranderen
//
void changeStartNr( short PlayerNr, u_char aStartNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].StartNr = aStartNr;
}

// Het speler StartPlayerNr waarde veranderen
//
void changeStartPlayerNr( short PlayerNr, u_char aStartPlayerNr ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[aStartPlayerNr]].StartPlayerNr = aStartPlayerNr;
}

// De speler-Finished waarde veranderen
//
void changeFinished( short PlayerNr, u_char aFinished ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].hasFinished = aFinished;
}

// De speler-hasRetired waarde veranderen
//
void changeRetired( short PlayerNr, u_char aRetired ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].hasRetired = aRetired;
}

// De speler-hasQuit waarde veranderen
//
void changeQuit( short PlayerNr, u_char aQuit ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].hasQuit = aQuit;
}


// De speler-VoteStage veranderen
//
void changeVoteStage( short PlayerNr, u_char aVoteStage ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].VoteStage = aVoteStage;
}

// De speler-VoteKick veranderen
//
void changeVoteKick( short PlayerNr, u_long aVoteKick ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].VoteKick = aVoteKick;
}

// De speler-VoteGhost veranderen
//
void changeVoteGhost( short PlayerNr, u_char aVoteGhost ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
//	if ( aVoteGhost==0 ) aVoteGhost = 1;
	ptr_players[PlayerIndex[PlayerNr]].VoteGhost = aVoteGhost;
}

// De speler-Ready-status veranderen
//
void changeReady(short PlayerNr, u_long aReady) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].Ready = aReady;
}

// De speler-rotatie veranderen
//
void changeRotation(short PlayerNr, u_long aX, u_long aY, u_long aZ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].Rotation.X = aX;
	ptr_players[PlayerIndex[PlayerNr]].Rotation.Y = aY;
	ptr_players[PlayerIndex[PlayerNr]].Rotation.Z = aZ;
}

// De speler-positie veranderen
//
void changePosition(short PlayerNr, u_long aX, u_long aY, u_long aZ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].Position.X = aX;
	ptr_players[PlayerIndex[PlayerNr]].Position.Y = aY;
	ptr_players[PlayerIndex[PlayerNr]].Position.Z = aZ;
}

// De speler-State veranderen
//
void changeState( short PlayerNr, int aState ) {
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].State = aState;
}

// LastPos X-coord instellen
void changeLastPosX( short PlayerNr, float aLastPosX ) {
	if (PlayerNr>=8) ptr_players[PlayerNr].LastPosX = aLastPosX;
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].LastPosX = aLastPosX;
}
// LastPos Y-coord instellen
void changeLastPosY( short PlayerNr, float aLastPosY ) {
	if (PlayerNr>=8) ptr_players[PlayerNr].LastPosX = aLastPosY;
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].LastPosY = aLastPosY;
}
// LastPos Z-coord instellen
void changeLastPosZ( short PlayerNr, float aLastPosZ ) {
	if (PlayerNr>=8) ptr_players[PlayerNr].LastPosX = aLastPosZ;
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	ptr_players[PlayerIndex[PlayerNr]].LastPosZ = aLastPosZ;
}




// LastPosBuffer vullen..
void AddLastPos( short PlayerNr ) {
	int		i;
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return;
	// eerst alle posities in de buffer opschuiven..de laatste zal verdwijnen uit de array..
	for ( i=MAXLASTPOS-1 - 1; i>=0; i-- ) {
		memcpy( &ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[i+1], &ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[i], 27);
	}
	// de laatst ontvangen positie komt op array-index 0
	memcpy( &ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[0], &ptr_players[PlayerIndex[PlayerNr]].LastPos, 27);
}

// de index in de buffer teruggeven, van de positie waarvan de tijd >= aan de opgegeven timestamp..
// resultaat == -1 als er geen geldige positie/tijd is gevonden..
int GetLastPos( short PlayerNr, u_long aTimeStamp ) {
	int		Result = -1;
	int		i;
	int		N;
	u_long	Time, TimeB;
	u_long	diffA, diffB;
	if (PlayerNr<0 || PlayerNr>=NumPlayers) return Result;
	N = ptr_players[PlayerIndex[PlayerNr]].PosReceived;
	// heeft de speler al (tenminste 1) posities verzonden??
	if ( N == 0 ) return Result;
	// alle positie in de buffer doorlopen van deze speler..
	if (N>MAXLASTPOS) N = MAXLASTPOS;

	// zoek de eerste tijd die <= aTimeStamp..
	for ( i=0; i<N; i++ ) {
		// de RaceTime uit LastPos lezen..
		Time = ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[i][2];
		Time += (ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[i][3] << 8);
		Time += (ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[i][4] << 16);
		Time += (ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[i][5] << 24);
		//
		if ( Time <= aTimeStamp ) {
			Result = i;
/*
			// dichtstbijzijnde timestamp zoeken (ze vliegen door de lucht !! woot woot)
			if ( i>0 ) {
				diffA = aTimeStamp - Time;
				TimeB = ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[i-1][2];
				TimeB += ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[i-1][3] << 8;
				TimeB += ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[i-1][4] << 16;
				TimeB += ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[i-1][5] << 24;
				diffB = TimeB - aTimeStamp;
				if ( diffB < diffA ) Result = i-1;
			}
*/
			break;
		}
	}


/*
	// zoek de eerste tijd die >= aTimeStamp..
	for ( i=N-1; i>=0; i-- ) {
		// de RaceTime uit LastPos lezen..
		Time = ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[i][2];
		Time += ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[i][3] << 8;
		Time += ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[i][4] << 16;
		Time += ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[i][5] << 24;
		//
		if ( Time >= aTimeStamp ) {
			Result = i;
			break;
		}
	}
*/
	return Result;
}


int GetLastPosRec( u_long aTimeStamp, u_char aPlayer, u_char ForPlayer ) {
	int		Result;// = ptr_players[0].PosReceived;
	int		i;
	int		N;
	u_long	Time;

	N = BLOB_LEN/27-1;
	Result = ptr_players[aPlayer].LastPosRecSent[ForPlayer];
	// heeft de speler al (tenminste 1) posities verzonden??
	if ( N == 0 ) return Result;
	// alle positie in de buffer doorlopen van deze speler..
	if (N>MAXLASTPOS) N = MAXLASTPOS;

	// zoek de eerste tijd die <= aTimeStamp..
	for ( i=Result; i<18000; i++ ) {//search forward..

// check this out..changed: 2008mei15
		if ( i>=ptr_players[aPlayer].PosCount ) return Result; //extrapolate player

		// de RaceTime uit LastPos lezen..
		Time = ptr_players[aPlayer].PosRec[i*27+2];
		Time += ( ptr_players[aPlayer].PosRec[i*27+3] << 8 );
		Time += ( ptr_players[aPlayer].PosRec[i*27+4] << 16 );
		Time += ( ptr_players[aPlayer].PosRec[i*27+5] << 24 );
		//Time = (u_long *)ptr_players[aPlayer].PosRec+i*27+2;

		//
		if ( Time >= aTimeStamp ) {
			Result = i;

			// onthouden waar we zijn gebleven..
			ptr_players[aPlayer].LastPosRecSent[ForPlayer] = i; //interpolate player

			break;
		}
	}
	return Result;
}





// Zoek de speler op met de opgegeven IP en PortNr.
// resulteer het speler-nummer (PlayerNr met waarden vanaf 0. -1 = ongeldig/mislukt)
//
short FindPlayer(u_long aIP, u_int aPortNr) {
	short PlayerNr=-1;
	int i=1;
	for ( i=1 ; i<NumPlayers ; i++ ) {
		if ( ptr_players[PlayerIndex[i]].PortNr == aPortNr ) {
			if ( ptr_players[PlayerIndex[i]].IP == aIP ) {
				PlayerNr = i;
				break;
			}
		}
	}
	return PlayerNr;
}


// Een speler toevoegen.
// resulteer het speler-nummer (PlayerNr met waarden vanaf 0. -1 = ongeldig/mislukt)
//
short AddPlayer(struct in_addr aIP, u_int aPortNr) {
	short PlayerNr=-1;
	int i=0;
	int j=0;
	u_char isOK;
	char *Country;
	if ( GlobalConfigMode == 1 ) {
		PlayerNr = NumPlayers++;  //NumPlayers achteraf ophogen..
		i = PlayerNr;
		PlayerIndex[PlayerNr] = i;
		// de gegevens overnemen naar dit record..
		ptr_players[i].Car = 1;
		ptr_players[i].AddrIn = aIP;
		ptr_players[i].IP = htonl(aIP.s_addr);
		ptr_players[i].PortNr = aPortNr;
		ptr_players[i].ChatLine = 2;
		ptr_players[i].LastCMD = 0x0000; //ptr_players[i].ChatLine-1;
		ptr_players[i].RaceTime = 0x00000000;	//tijd in ms.
		for (j=0; j<4; j++) ptr_players[i].SplitTime[j] = 0;
		ptr_players[i].SplitsDone = 0;
		ptr_players[i].PercToSplit0 = 0x00;	// BCD coded (Binary Coded Decimal), 0x00=0%, 0x09=9%, 0x10=10% etc..
		ptr_players[i].VoteStage = 0;				// geen vote voor een stage..
		ptr_players[i].VoteKick = 0;				// geen vote tegen een speler met deze ID..
		ptr_players[i].VoteGhost = 1;				// 4WD ghost
		ptr_players[i].StartNr = 0xFF;				// ongeldig startnummer..
		ptr_players[i].hasFinished = 0x00;			// nog niet gefinisht..
		ptr_players[i].hasRetired = 0x00;			// nog niet retired..
		ptr_players[i].hasQuit = 0x00;				// nog niet ge-quit..
		ptr_players[i].State = -1; //<geen speler>
		
		ptr_players[i].Ping = 0x00000020;
		ptr_players[i].PingCount = 0x00000001;
		ptr_players[i].AvgPing = 0x00000020;

		ptr_players[i].BufferedPackets = 0;		// nog geen enkel pakketje reeds gebufferd (tbv ACK/resent)
		ptr_players[i].PtrBufferRead = 3;
		ptr_players[i].PtrBufferWrite = 2; // MAXBUFF-1; //roterende buffer

		ptr_players[i].PosCount = 0;
		ptr_players[i].LastTimeReady = minisec();
		ptr_players[i].ReadyWarning = 0;


		//PosRec;
		for (j=0;j<4;j++) ptr_players[i].PosSplitSent[j] = 0; //1 == intermediate[j] verzonden naar speler
		
		for (j=0;j<MAXBUFF;j++) {
			ptr_players[i].BufferLen[j] = -1;	// de buffer markeren als zijnde niet in gebruik..
			ptr_players[i].BufferCountdown[j] = 0;
		}
		// laatst ontvangen Position-packet al verstuurd aan spelers?  reset
		for (j=0; j < MAXPLAYERS; j++ ) {    // MaxPlayers
			ptr_players[i].PosSent[j] = 1; //als "al verzonden" instellen  naar andere speler(j)
			//test
			ptr_players[i].LastPosSentTime[j] = 0x00000000;
			ptr_players[i].LastPosRecSent[j] = 0;
			///test
		}
		if ( ++GlobalJoinCount > 255 ) GlobalJoinCount = 1;  ///   20080504 ivm player 0
//--test of andere spelers dit nummer al gebruiken
isOK = 0;
while ( isOK==0 ) {
	isOK = 1;
	for ( j=1; j<NumPlayers; j++ ) {
		if ( ptr_players[PlayerIndex[j]].ID == GlobalJoinCount ) {
			if ( ++GlobalJoinCount > 255 ) GlobalJoinCount = 1;  ///   20080504 ivm player 0
			isOK = 0;
		}
	}
}
//--
		ptr_players[i].ID = GlobalJoinCount;	// het totaal joins ophogen..
		return PlayerNr;
	}

	// zoek de eerste vrije entry in de array ptr_players
	for ( i=1; i < MaxPlayers; i++ ) {
		//if (ptr_players[i].IP==0 && ptr_players[i].PortNr==0) {
		if (ptr_players[i].PortNr==0) {//empty slot
			PlayerNr = NumPlayers++;  //NumPlayers achteraf ophogen..
			PlayerIndex[PlayerNr] = i;
			// Nick instellen voor naamloos ( of geen naampakket sturen.. )
			strcpy( &ptr_players[i].Nick, "Anonymous\0" );
			// de gegevens overnemen naar dit record..
			ptr_players[i].AddrIn = aIP;
			ptr_players[i].IP = htonl(aIP.s_addr);
			ptr_players[i].PortNr = aPortNr;
			ptr_players[i].ConnectionSpeed = 0x00;
			ptr_players[i].ChatLine = 2;
			ptr_players[i].LastCMD = 0x0000; //ptr_players[i].ChatLine-1;
			ptr_players[i].Country = 0;
			ptr_players[i].Car = 1;
			ptr_players[i].CarType = 0x00;
			ptr_players[i].Gearbox = 0;
			ptr_players[i].RaceTime = 0x00000000;	//tijd in ms.
			for (j=0; j<4; j++) ptr_players[i].SplitTime[j] = 0;
			ptr_players[i].SplitsDone = 0;
			ptr_players[i].PercToSplit0 = 0x00;	// BCD coded (Binary Coded Decimal), 0x00=0%, 0x09=9%, 0x10=10% etc..
			ptr_players[i].Percentage = 0x00;	//start..finish == 0..100 %
			ptr_players[i].VoteStage = 0;				// geen vote voor een stage..
			ptr_players[i].VoteKick = 0;				// geen vote tegen een speler met deze ID..
			ptr_players[i].VoteGhost = 0;				// host-ghost record laten zien van deze klasse
			ptr_players[i].StartNr = 0xFF;				// ongeldig startnummer..
			ptr_players[i].StartPlayerNr = 0xFF;		// ongeldig startnummer..
			ptr_players[i].Ready = 0x00000000;
			ptr_players[i].hasFinished = 0x00;			// nog niet gefinisht..
			ptr_players[i].hasRetired = 0x00;			// nog niet retired..
			ptr_players[i].hasQuit = 0x00;				// nog niet ge-quit..
			ptr_players[i].State = -1; //<geen speler>
			
			ptr_players[i].Ping = 0x00000020;
			ptr_players[i].Ping1 = 0x00000000;
			ptr_players[i].Ping2 = 0x00000000;
			ptr_players[i].PingCount = 0x00000001;
			ptr_players[i].AvgPing = 0x00000020;

			ptr_players[i].PacketLen = 0;
			ptr_players[i].BufferedPackets = 0;		// nog geen enkel pakketje reeds gebufferd (tbv ACK/resent)
			ptr_players[i].PtrBufferRead = 3;
			ptr_players[i].PtrBufferWrite = 2; // MAXBUFF-1; //roterende buffer

			ptr_players[i].LastTimeReady = minisec();
			ptr_players[i].LastTimeValue = 0x00000000;
			ptr_players[i].ReadyWarning = 0x00;

			ptr_players[i].PosCount = 0;
			ptr_players[i].PosReceived = 0x00000000;
			ptr_players[i].ReceivedC60E = 0x00;		//gridpresent1 al ontvangen?
			ptr_players[i].NickC6D6 = 0x00;		//gridpresent1 al ontvangen?

			// tbv. No-Cuts check..
			ptr_players[i].LastPosX = 0.0f;
			ptr_players[i].LastPosY = 0.0f;
			ptr_players[i].LastPosZ = 0.0f;

			//PosRec;
			//PosRec;
			for (j=0;j<4;j++) ptr_players[i].PosSplitSent[j] = 0x00; //1 == intermediate[j] verzonden naar speler

			for (j=0;j<MAXBUFF;j++) {
				ptr_players[i].BufferLen[j] = -1;	// de buffer markeren als zijnde niet in gebruik..
				ptr_players[i].BufferCountdown[j] = 0;
			}
			// laatst ontvangen Position-packet al verstuurd aan spelers?  reset
			for (j=0; j<MAXPLAYERS; j++ ) {    // MaxPlayers
				ptr_players[i].PosSent[j] = 1; //als "al verzonden" instellen  naar andere speler(j)
				//test
				ptr_players[i].LastPosSentTime[j] = 0x00000000;
				ptr_players[i].LastPosRecSent[j] = 0;
				///test
			}

			for (j=0; j<MAXLASTPOS; j++) {
				//for (i=0; i<27; i++) ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[j][i] = 0x00;
				memset( &ptr_players[i].LastPosBuf[j][0], 0x00, 27 );
			}

			//for (j=0; j<27; j++) ptr_players[PlayerIndex[PlayerNr]].LastPos[j] = 0x00;
			memset( &ptr_players[i].LastPos[0], 0x00, 27 );

			//for (j=0; j<BLOB_LEN; j++) ptr_players[PlayerIndex[PlayerNr]].PosRec[j] = 0x00;
			memset( &ptr_players[i].PosRec[0], 0x00, BLOB_LEN );



			if ( ++GlobalJoinCount > 255 ) GlobalJoinCount = 1;  ///   20080504 
			//--test of andere spelers dit nummer al gebruiken
			isOK = 0;
			while ( isOK==0 ) {
				isOK = 1;
				for ( j=1; j<NumPlayers; j++ ) {
					if ( ptr_players[PlayerIndex[j]].ID == GlobalJoinCount ) {
						if ( ++GlobalJoinCount > 255 ) GlobalJoinCount = 1;  ///   20080504 ivm player 0
						isOK = 0;
					}
				}
			}
			ptr_players[i].ID = GlobalJoinCount;	// het totaal joins ophogen..

			break;
		}
	}
	return PlayerNr;
}


// Een speler verwijderen.
// resulteer -1 indien succesvol, anders het speler-nummer (PlayerNr met waarden vanaf 0)
//
short DeletePlayer(short PlayerNr) {
	int i=0;
	int j=0;
	char *log[100];

	//sprintf( log, "deleting player %d", PlayerNr );
	//Log( log );

	if (PlayerNr<1 || PlayerNr>=NumPlayers) return PlayerNr;

	// de hele struct van de player in de list legen
//	ptr_players[PlayerIndex[PlayerNr]].ID = 0;
	ptr_players[PlayerIndex[PlayerNr]].TimeID = 0x00000000;
	ptr_players[PlayerIndex[PlayerNr]].MemberID = 0x00000000;
	ptr_players[PlayerIndex[PlayerNr]].IP = 0;						//"0.0.0.0\x00";
	ptr_players[PlayerIndex[PlayerNr]].AddrIn.s_addr = 0;
	ptr_players[PlayerIndex[PlayerNr]].PortNr = 0;
	ptr_players[PlayerIndex[PlayerNr]].ChatLine = 0x0000;
	ptr_players[PlayerIndex[PlayerNr]].LastCMD = 0x0000;
	changeNick( PlayerNr, "               " );
	ptr_players[PlayerIndex[PlayerNr]].ConnectionSpeed = 0x00;
	ptr_players[PlayerIndex[PlayerNr]].Country = 0;
	ptr_players[PlayerIndex[PlayerNr]].Car = 0;
	ptr_players[PlayerIndex[PlayerNr]].CarType = 0x00;
	ptr_players[PlayerIndex[PlayerNr]].Gearbox = 0;
	ptr_players[PlayerIndex[PlayerNr]].Damage = 0x00;
	ptr_players[PlayerIndex[PlayerNr]].RaceTime = 0x00000000;	// tijd in ms.
	for (j=0; j<4; j++) ptr_players[PlayerIndex[PlayerNr]].SplitTime[j] = 0x00000000;
	ptr_players[PlayerIndex[PlayerNr]].SplitsDone = 0;
	ptr_players[PlayerIndex[PlayerNr]].StartTime = 0x00000000;	// tijd in ms.
	ptr_players[PlayerIndex[PlayerNr]].VoteStage = 0x00;
	ptr_players[PlayerIndex[PlayerNr]].VoteKick = 0x00000000;			// geen vote tegen player ID
	ptr_players[PlayerIndex[PlayerNr]].VoteGhost = 0x00;
	ptr_players[PlayerIndex[PlayerNr]].State = 0;				// <geen speler>
	ptr_players[PlayerIndex[PlayerNr]].Ready = 0x00000000;
	ptr_players[PlayerIndex[PlayerNr]].StartNr = 0xFF;			// ongeldig startnummer..
	ptr_players[PlayerIndex[PlayerNr]].StartPlayerNr = 0xFF;	// ongeldig startnummer..
	ptr_players[PlayerIndex[PlayerNr]].hasFinished = 0x00;		// nog niet gefinished..
	ptr_players[PlayerIndex[PlayerNr]].hasRetired = 0x00;		// nog niet retired..
	ptr_players[PlayerIndex[PlayerNr]].hasQuit = 0x00;			// nog niet ge-quit..
	ptr_players[PlayerIndex[PlayerNr]].Ping = 0x00000000;
	ptr_players[PlayerIndex[PlayerNr]].Ping1 = 0x00000000;
	ptr_players[PlayerIndex[PlayerNr]].Ping2 = 0x00000000;
	ptr_players[PlayerIndex[PlayerNr]].PingCount = 0x00000000;
	ptr_players[PlayerIndex[PlayerNr]].AvgPing = 0x00000000;
	ptr_players[PlayerIndex[PlayerNr]].PercToSplit0 = 0x00;
	ptr_players[PlayerIndex[PlayerNr]].Percentage = 0x00;
	ptr_players[PlayerIndex[PlayerNr]].PosCount = 0;
	ptr_players[PlayerIndex[PlayerNr]].PosReceived = 0x00000000;
	ptr_players[PlayerIndex[PlayerNr]].ReceivedC60E = 0x00;		//gridpresent1 al ontvangen?
	ptr_players[PlayerIndex[PlayerNr]].NickC6D6 = 0x00;		//gridpresent1 al ontvangen?
	ptr_players[PlayerIndex[PlayerNr]].ReadyWarning = 0x00;
	ptr_players[PlayerIndex[PlayerNr]].LastTimeReady = 0x00000000;
	ptr_players[PlayerIndex[PlayerNr]].LastTimeValue = 0x00000000;
	ptr_players[PlayerIndex[PlayerNr]].PacketLen = 0;
	ptr_players[PlayerIndex[PlayerNr]].LastPosX = 0.0f;
	ptr_players[PlayerIndex[PlayerNr]].LastPosY = 0.0f;
	ptr_players[PlayerIndex[PlayerNr]].LastPosZ = 0.0f;

//short				PtrBufferRead;// index in de array Packet[PtrBufferRead][BUFFSZ]
//short				PtrBufferWrite;			// index in de array Packet[PtrBufferWrite][BUFFSZ]

	for (j=0; j<MAXPLAYERS; j++) {
		ptr_players[PlayerIndex[PlayerNr]].LastPosSentTime[j] = 0x00000000;
		ptr_players[PlayerIndex[PlayerNr]].LastPosRecSent[j] = 0;
		ptr_players[PlayerIndex[PlayerNr]].PosSent[j] = 0x00;
	}

	for (j=0; j<4; j++) ptr_players[PlayerIndex[PlayerNr]].PosSplitSent[j] = 0x00;

	for (j=0; j<MAXLASTPOS; j++) {
		//for (i=0; i<27; i++) ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[j][i] = 0x00;
		memset( &ptr_players[PlayerIndex[PlayerNr]].LastPosBuf[j][0], 0x00, 27 );
	}

	//for (j=0; j<27; j++) ptr_players[PlayerIndex[PlayerNr]].LastPos[j] = 0x00;
	memset( &ptr_players[PlayerIndex[PlayerNr]].LastPos[0], 0x00, 27 );

	//for (j=0; j<BLOB_LEN; j++) ptr_players[PlayerIndex[PlayerNr]].PosRec[j] = 0x00;
	memset( &ptr_players[PlayerIndex[PlayerNr]].PosRec[0], 0x00, BLOB_LEN );

	// de buffers van de speler niet meer gebruiken..
	ptr_players[PlayerIndex[PlayerNr]].BufferedPackets = 0;		// nog geen enkel pakketje reeds gebufferd (tbv ACK/resent)
	for (i=0;i<MAXBUFF;i++) {
		ptr_players[PlayerIndex[PlayerNr]].BufferLen[i] = -1;	// de buffer markeren als zijnde niet in gebruik..
		ptr_players[PlayerIndex[PlayerNr]].BufferCountdown[i] = 0;
		ptr_players[PlayerIndex[PlayerNr]].BufferCMD[i] = 0x0000;
		//for (j=0;j<BUFFSZ;j++) ptr_players[PlayerIndex[PlayerNr]].Packet[i][j] = 0x00;
		memset( &ptr_players[PlayerIndex[PlayerNr]].Packet[i][0], 0x00, BUFFSZ );
	}
	// nu de lijst met indexen aanschuiven/sorteren. 
	for (i=PlayerNr; i < MaxPlayers-1; i++) PlayerIndex[i] = PlayerIndex[i+1];
	for (i=NumPlayers-1; i < MaxPlayers; i++) PlayerIndex[i] = -1;
	// de rest van de globale variabelen bijwerken..
	playerNumber = -1;
	// het aantal spelers verlagen..
	NumPlayers--;

	//sprintf( log, "%d deleted, number of players: %d", PlayerNr, NumPlayers-1 );
	//Log( log );

	if (NumPlayers<1) NumPlayers = 1; //fail-safe
	return -1;
}


// Een speler verwijderen.
// resulteer -1 indien succesvol, anders het speler-nummer (PlayerNr met waarden vanaf 0)
//
void ClearPlayer(short SlotNr) {
	int i=0;
	int j=0;
	char *log[100];

	//sprintf( log, "deleting player %d", PlayerNr );
	//Log( log );

	if (SlotNr<0 || SlotNr>=8+4+8) return;

	// de hele struct van de player in de list legen
//	ptr_players[SlotNr].ID = 0;
	ptr_players[SlotNr].TimeID = 0x00000000;
	ptr_players[SlotNr].MemberID = 0x00000000;
	ptr_players[SlotNr].IP = 0;						//"0.0.0.0\x00";
	ptr_players[SlotNr].AddrIn.s_addr = 0;
	ptr_players[SlotNr].PortNr = 0;
	ptr_players[SlotNr].ChatLine = 0x0000;
	ptr_players[SlotNr].LastCMD = 0x0000;
	ptr_players[SlotNr].LastCMD = 0x0000;
	strcpy(ptr_players[SlotNr].Nick, "               ");
	ptr_players[SlotNr].ConnectionSpeed = 0x00;
	ptr_players[SlotNr].Country = 0;
	ptr_players[SlotNr].Car = 0;
	ptr_players[SlotNr].CarType = 0x00;
	ptr_players[SlotNr].Gearbox = 0;
	ptr_players[SlotNr].Damage = 0x00;
	ptr_players[SlotNr].RaceTime = 0x00000000;	// tijd in ms.
	for (j=0; j<4; j++) ptr_players[SlotNr].SplitTime[j] = 0x00000000;
	ptr_players[SlotNr].SplitsDone = 0;
	ptr_players[SlotNr].StartTime = 0x00000000;	// tijd in ms.
	ptr_players[SlotNr].VoteStage = 0x00;
	ptr_players[SlotNr].VoteKick = 0x00000000;			// geen vote tegen player ID
	ptr_players[SlotNr].VoteGhost = 0x00;
	ptr_players[SlotNr].State = 0;				// <geen speler>
	ptr_players[SlotNr].Ready = 0x00000000;
	ptr_players[SlotNr].StartNr = 0xFF;			// ongeldig startnummer..
	ptr_players[SlotNr].StartPlayerNr = 0xFF;	// ongeldig startnummer..
	ptr_players[SlotNr].hasFinished = 0x00;		// nog niet gefinished..
	ptr_players[SlotNr].hasRetired = 0x00;		// nog niet retired..
	ptr_players[SlotNr].hasQuit = 0x00;			// nog niet ge-quit..
	ptr_players[SlotNr].Ping = 0x00000000;
	ptr_players[SlotNr].Ping1 = 0x00000000;
	ptr_players[SlotNr].Ping2 = 0x00000000;
	ptr_players[SlotNr].PingCount = 0x00000000;
	ptr_players[SlotNr].AvgPing = 0x00000000;
	ptr_players[SlotNr].PercToSplit0 = 0x00;
	ptr_players[SlotNr].Percentage = 0x00;
	ptr_players[SlotNr].PosCount = 0;
	ptr_players[SlotNr].PosReceived = 0x00000000;
	ptr_players[SlotNr].ReceivedC60E = 0x00;		//gridpresent1 al ontvangen?
	ptr_players[SlotNr].NickC6D6 = 0x00;		//gridpresent1 al ontvangen?
	ptr_players[SlotNr].ReadyWarning = 0x00;
	ptr_players[SlotNr].LastTimeReady = 0x00000000;
	ptr_players[SlotNr].LastTimeValue = 0x00000000;
	ptr_players[SlotNr].PacketLen = 0;
	ptr_players[SlotNr].LastPosX = 0.0f;
	ptr_players[SlotNr].LastPosY = 0.0f;
	ptr_players[SlotNr].LastPosZ = 0.0f;

//short				PtrBufferRead;// index in de array Packet[PtrBufferRead][BUFFSZ]
//short				PtrBufferWrite;			// index in de array Packet[PtrBufferWrite][BUFFSZ]

	for (j=0; j<MAXPLAYERS; j++) {
		ptr_players[SlotNr].LastPosSentTime[j] = 0x00000000;
		ptr_players[SlotNr].LastPosRecSent[j] = 0;
		ptr_players[SlotNr].PosSent[j] = 0x00;
	}

	for (j=0; j<4; j++) ptr_players[SlotNr].PosSplitSent[j] = 0x00;

	for (j=0; j<MAXLASTPOS; j++) {
		memset( &ptr_players[SlotNr].LastPosBuf[j][0], 0x00, 27 );
	}

	memset( &ptr_players[SlotNr].LastPos[0], 0x00, 27 );

	memset( &ptr_players[SlotNr].PosRec[0], 0x00, BLOB_LEN );

	// de buffers van de speler niet meer gebruiken..
	ptr_players[SlotNr].BufferedPackets = 0;		// nog geen enkel pakketje reeds gebufferd (tbv ACK/resent)
	for (i=0;i<MAXBUFF;i++) {
		ptr_players[SlotNr].BufferLen[i] = -1;	// de buffer markeren als zijnde niet in gebruik..
		ptr_players[SlotNr].BufferCountdown[i] = 0;
		ptr_players[SlotNr].BufferCMD[i] = 0x0000;
		memset( &ptr_players[SlotNr].Packet[i][0], 0x00, BUFFSZ );
	}

	return;
}
