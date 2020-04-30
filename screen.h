
// color-codes for console screen
const u_short ATTR_NORMAL		=0;
const u_short ATTR_BOLD			=1;
const u_short ATTR_UNDERLINE	=4;
const u_short ATTR_BLINK		=5;
const u_short ATTR_REVERSE		=7;
const u_short ATTR_HIDE			=8;

const u_short BLACK				=0;
const u_short RED				=1;
const u_short GREEN				=2;
const u_short YELLOW			=3;
const u_short BLUE				=4;
const u_short MAGENTA			=5;
const u_short CYAN				=6;
const u_short WHITE				=7;

char	*Slog[1024];





void initdisplay();
void closedisplay();


void cls();
void eraseUp( int fromLine );
void eraseDown( int fromLine );
void eraseRows(int first, int last);

void scrollUp();
void scrollDown();
void scrollScreen();
void scrollRows( int first, int last );
void ResetScrollArea();

void gotoxy( int x, int y ); //cursor op x,y positie plaatsen..
void chooseColors( foregroundColor, backgroundColor ); //kleuren kiezen voor console-tekst

void printxy( int x, int y, u_char *text );
void printxyc( int x, int y, u_short Style, u_short foregroundColor, u_short backgroundColor, u_char *text );	//print een string
void printxycc( int x, int y, u_short Style, u_short foregroundColor, u_short backgroundColor, u_char *UCHAR );	//print een byte (altijd 2 nibbles)
void printxycd( int x, int y, u_short Style, u_short foregroundColor, u_short backgroundColor, u_int *UINT );	//print een integer
void printxycx( int x, int y, u_short Style, u_short foregroundColor, u_short backgroundColor, u_long *ULONG );	//print een long
void printxycf( int x, int y, u_short Style, u_short foregroundColor, u_short backgroundColor, float *Single );	//print een float

void Msgprinty( u_char *aMessage );
void MsgprintyIP( u_char *aIP, u_short aPORT, u_char *aMessage );
void MsgprintyIP1( u_char *aIP, u_short aPORT, u_char *aMessage, u_char *aMessage2 );
void MsgprintyIP2( u_char *aIP, u_short aPORT, u_char *aMessage, u_short aValue );
void MsgprintyIP3( u_char *aIP, u_short aPORT, u_char *aMessage, u_short aValue1, u_short aValue2 );
void MsgprintyIP4( u_char *aIP, u_short aPORT, u_char *aMessage, u_short aValue1, u_short aValue2, u_short aValue3 );

void MStoTimeString( u_long MS, u_char *String );	// een tijd in ms. naar een string omzetten in tijd-formaat











void initdisplay() {
	// reset, color attr, cls
	fprintf( stdout, "\33c\33[0;%i;%im\33[2J", 30+WHITE, 40+BLACK);
}

void closedisplay() {
	fprintf( stdout, "\33[0;%i;%im", 30+WHITE, 40+BLACK); // set color attributes
	fprintf( stdout, "\33[K" );
}



void cls() {
	fprintf( stdout, "\33[2J" );
	//fputs("\x0C", stdout);
}

void eraseUp( int fromLine ) {
	fprintf( stdout, "\33[s\33[%iH\33[1J\33[u", fromLine ); //save cursor, gotoy , erase up, restore cursor
}

void eraseDown( int fromLine ) {
	fprintf( stdout, "\33[s\33[%iH\33[J\33[u", fromLine ); //gotoy & erase down
}

void eraseRows( int first, int last ) {
	int i=0;
	for (i=first;i<=last;i++) {
		// save cursor, gotoy(regel i), restore cursor
		fprintf( stdout, "\33[s\33[%i;1H\33[2K\33[u", i );
	}
}


void scrollUp() {
	fprintf( stdout, "\33[M" );
}

void scrollDown() {
	fprintf( stdout, "\33[D" );
}

void scrollScreen() {
	fprintf( stdout, "\33[r" );
}

void ResetScrollArea() {
	fprintf( stdout, "\33[%i;%ir", 1, 80 );
}

void scrollRows( int first, int last ) {
	// save cursor, set scroll-area, gotoy laatste regel scroll-area, linefeed, "restore" scroll-area, restore cursor
	fprintf( stdout, "\33[s\33[%i;%ir\33[%iH\x0A\33[%i;%ir\33[u", first,last, last, 1,80 );
	
	//fprintf( stdout, "\33[s\33[%i;%ir\33[%iH\x0A\33c\33[u", first,last, last );
}


void chooseColors( foregroundColor, backgroundColor ) {
	printf( "\33[0;%i;%im", 30+foregroundColor, 40+backgroundColor); // set color attributes
	fflush( stdout );
}

void gotoxy(int x, int y) {
	printf("\33[%i;%iH", y, x);
}


void printxy( int x, int y, u_char *text) {
	fprintf( stdout, "\33[s\33[%i;%iH%s\33[u", y,x, text);
}

void printxyc( int x, int y, u_short Style, u_short foregroundColor, u_short backgroundColor, u_char *text ) {
	foregroundColor += 30;
	backgroundColor += 40;
	fprintf( stdout, "\33[s\33[%i;%iH\33[%d;%d;%dm%s\33[u", y,x, Style, foregroundColor, backgroundColor, text); //, format
}

void printxycc( int x, int y, u_short Style, u_short foregroundColor, u_short backgroundColor, u_char *UCHAR ) {
	foregroundColor += 30;
	backgroundColor += 40;
	fprintf( stdout, "\33[s\33[%i;%iH\33[%d;%d;%dm%.2x\33[u", y,x, Style, foregroundColor, backgroundColor, UCHAR); //, format
}

void printxycd( int x, int y, u_short Style, u_short foregroundColor, u_short backgroundColor, u_int *UINT ) {
	foregroundColor += 30;
	backgroundColor += 40;
	fprintf( stdout, "\33[s\33[%i;%iH\33[%d;%d;%dm%d\33[u", y,x, Style, foregroundColor, backgroundColor, UINT); //, format
}

void printxycx( int x, int y, u_short Style, u_short foregroundColor, u_short backgroundColor, u_long *ULONG ) {
	foregroundColor += 30;
	backgroundColor += 40;
	fprintf( stdout, "\33[s\33[%i;%iH\33[%d;%d;%dm%x\33[u", y,x, Style, foregroundColor, backgroundColor, ULONG); //, format
}

void printxycf( int x, int y, u_short Style, u_short foregroundColor, u_short backgroundColor, float *Single ) { //print een float*
	foregroundColor += 30;
	backgroundColor += 40;
	fprintf( stdout, "\33[s\33[%i;%iH\33[%d;%d;%dm%8.3f\33[u", y,x, Style, foregroundColor, backgroundColor, Single); //, format
}

void Msgprinty( u_char *aMessage ) {
	u_short FG = 37; //white on
	u_short BG = 40; //black
	if (GlobalShowMessages==0) return;
	scrollRows( MSGTOP, MSGBOTTOM );	// regels MSGTOP t/m MSGBOTTOM scrollen
	// save cursor state, gotoxy(3,MSGBOTTOM), clearEOL, <data>, restore cursor
	fprintf( stdout, "\33[s\33[%i;%iH\33[K\33[1;%d;%dm%s\33[0;%d;%dm\33[u", FG,BG, MSGBOTTOM,3 , aMessage, FG,BG);
}

void MsgprintyIP( u_char *aIP, u_short aPORT, u_char *aMessage ) {
	u_short FG = 30+WHITE;
	u_short BG = 40+BLACK;
	u_short pcFG = 30+RED;
	u_short pcBG = 40+BLACK;
	u_short pcBold = packetCount % 2;
	if (GlobalShowMessages==0) return;
	scrollRows( MSGTOP, MSGBOTTOM );	// regels MSGTOP t/m MSGBOTTOM scrollen
	// save cursor state, gotoxy(3,MSGBOTTOM), clearEOL, <data>, restore cursor
	fprintf( stdout, "\33[s\33[%i;%iH\33[K\33[%d;%d;%dm%d\33[0;%d;%dm %s:%d \33[1;%d;%dm%s\33[0;%d;%dm\33[u", MSGBOTTOM,3, pcBold,pcFG,pcBG,packetCount,FG,BG, aIP, aPORT, FG,BG, aMessage, FG,BG );
	sprintf( Slog, "%s %d %s", aIP, aPORT, aMessage );
	Log2( Slog, "debug" );
}

void MsgprintyIP1( u_char *aIP, u_short aPORT, u_char *aMessage, u_char *aMessage2 ) {
	u_short FG = 30+WHITE;
	u_short BG = 40+BLACK;
	u_short pcFG = 30+RED;
	u_short pcBG = 40+BLACK;
	u_short pcBold = packetCount % 2;
	if (GlobalShowMessages==0) return;
	scrollRows( MSGTOP, MSGBOTTOM );	// regels MSGTOP t/m MSGBOTTOM scrollen
	// save cursor state, gotoxy(3,MSGBOTTOM), clearEOL, <data>, restore cursor
	fprintf( stdout, "\33[s\33[%i;%iH\33[K\33[%d;%d;%dm%d\33[0;%d;%dm %s:%d \33[1;%d;%dm%s\33[0;%d;%dm %s\33[0;37;40m\33[u", MSGBOTTOM,3, pcBold,pcFG,pcBG,packetCount,FG,BG, aIP, aPORT, FG,BG, aMessage, FG,BG, aMessage2 );
	sprintf( Slog, "%s %d %s %s", aIP, aPORT, aMessage, aMessage2 );
	Log2( Slog, "debug" );
}

void MsgprintyIP2( u_char *aIP, u_short aPORT, u_char *aMessage, u_short aValue ) {
	u_short FG = 30+WHITE;
	u_short BG = 40+BLACK;
	u_short pcFG = 30+RED;
	u_short pcBG = 40+BLACK;
	u_short pcBold = packetCount % 2;
	if (GlobalShowMessages==0) return;
	scrollRows( MSGTOP, MSGBOTTOM );	// regels MSGTOP t/m MSGBOTTOM scrollen
	// save cursor state, gotoxy(3,MSGBOTTOM), clearEOL, <data>, restore cursor
	fprintf( stdout, "\33[s\33[%i;%iH\33[K\33[%d;%d;%dm%d\33[0;%d;%dm %s:%d \33[1;%d;%dm%s\33[0;%d;%dm %x\33[0;37;40m\33[u", MSGBOTTOM,3, pcBold,pcFG,pcBG,packetCount,FG,BG, aIP, aPORT, FG,BG, aMessage, FG,BG, aValue );
	sprintf( Slog, "%s %d %s %d", aIP, aPORT, aMessage, aValue );
	Log2( Slog, "debug" );
}

void MsgprintyIP3( u_char *aIP, u_short aPORT, u_char *aMessage, u_short aValue1, u_short aValue2 ) {
	u_short FG = 30+WHITE;
	u_short BG = 40+BLACK;
	u_short pcFG = 30+RED;
	u_short pcBG = 40+BLACK;
	u_short pcBold = packetCount % 2;
	if (GlobalShowMessages==0) return;
	scrollRows( MSGTOP, MSGBOTTOM );	// regels MSGTOP t/m MSGBOTTOM scrollen
	// save cursor state, gotoxy(3,MSGBOTTOM), clearEOL, <data>, restore cursor
	fprintf( stdout, "\33[s\33[%i;%iH\33[K\33[%d;%d;%dm%d\33[0;%d;%dm %s:%d \33[1;%d;%dm%s\33[0;%d;%dm	%x	%x\33[0;37;40m\33[u", MSGBOTTOM,3, pcBold,pcFG,pcBG,packetCount,FG,BG, aIP, aPORT, FG,BG, aMessage, FG,BG, aValue1, aValue2 );
	sprintf( Slog, "%s %d %s %d %d", aIP, aPORT, aMessage, aValue1, aValue2 );
	Log2( Slog, "debug" );
}
void MsgprintyIP4( u_char *aIP, u_short aPORT, u_char *aMessage, u_short aValue1, u_short aValue2, u_short aValue3 ) {
	u_short FG = 30+WHITE;
	u_short BG = 40+BLACK;
	u_short pcFG = 30+RED;
	u_short pcBG = 40+BLACK;
	u_short pcBold = packetCount % 2;
	if (GlobalShowMessages==0) return;
	scrollRows( MSGTOP, MSGBOTTOM );	// regels MSGTOP t/m MSGBOTTOM scrollen
	// save cursor state, gotoxy(3,MSGBOTTOM), clearEOL, <data>, restore cursor
	fprintf( stdout, "\33[s\33[%i;%iH\33[K\33[%d;%d;%dm%d\33[0;%d;%dm %s:%d \33[1;%d;%dm%s\33[0;%d;%dm	%x	%x	%x\33[0;37;40m\33[u", MSGBOTTOM,3, pcBold,pcFG,pcBG,packetCount,FG,BG, aIP, aPORT, FG,BG, aMessage, FG,BG, aValue1, aValue2, aValue3 );
	sprintf( Slog, "%s %d %s %d %d %d", aIP, aPORT, aMessage, aValue1, aValue2, aValue3 );
	Log2( Slog, "debug" );
}







void MStoTimeString( u_long MS, u_char *String ) {
	u_short	Hours;
	u_char	Minutes;
	u_char	Seconds;
	u_char	Sec1000;

	Hours = (u_short) ( MS / 3600000 ); // /1000=sec   /60=min   /60=uur
	Minutes = (u_char)( (MS-(Hours*3600000)) / 60000 );
	Seconds = (u_char)( (MS-(Hours*3600000)-(Minutes*60000)) / 1000 );
	Sec1000 = (u_char)( MS % 1000 );

	if (Hours>0) {
		sprintf( String, "%d:%.2d:%.2d.%.3d", Hours, Minutes, Seconds, Sec1000 );
	} else
		if (Minutes>0) {
			sprintf( String, "%.2d:%.2d.%.3d", Minutes, Seconds, Sec1000 );
		} else 
			if (Seconds>=10) {
				sprintf( String, "   %.2d.%.3d", Seconds, Sec1000 );
			} else {
				sprintf( String, "    %d.%.3d", Seconds, Sec1000 );
			}
	return;
}
