
// 

void Log( u_char *String );	// 
void Log2( u_char *String, u_char *Level );	// 


void Log1( u_char *String ) {

	FILE	*fh;
	struct	tm *ptr;
	time_t	tm;
	char	str[60];
	u_char	*log[128];

	sprintf( log, "cmr04ds.%d.game.log.txt", port );

	//fh = fopen( "cmr04ds.log.txt", "a" );
	fh = fopen( log, "a" );

	if ( fh != NULL ) {

		tm = time(NULL);
		ptr = localtime(&tm);
		strftime(str ,59 , "%Y-%m-%d %H:%M:%S",ptr);

		if ( String=="" ) {
			fprintf( fh, "\n" );
		} else {
			fprintf( fh, "%s:%d %s\n", str, port, String );
		}


		fclose( fh );
	}
	
	return;
}

void Log2( u_char *String, u_char *Level ) {

	FILE	*fh;
	struct	tm *ptr;
	time_t	tm;
	char	str[60];
	u_char	*Llog[1024];

	sprintf( Llog, "cmr04ds.%d.%s.log.txt", port, Level );

	//fh = fopen( "cmr04ds.log.txt", "a" );
	fh = fopen( Llog, "a" );

	if ( fh != NULL ) {

		tm = time(NULL);
		ptr = localtime(&tm);
		strftime(str ,59 , "%Y-%m-%d %H:%M:%S",ptr);

		if ( String=="" ) {
			fprintf( fh, "\n" );
		} else {
			fprintf( fh, "%s:%d %s\n", str, port, String );
		}


		fclose( fh );
	}
	
	return;
}
