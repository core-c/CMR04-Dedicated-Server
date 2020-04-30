
//----------------------------
// NO-CUTS checks
u_long aID;
float aPX1; 
float aPY1;
float aPZ1;
float aPX2;
float aPY2;
float aPZ2;
unsigned short aPercentage1;
unsigned short aPercentage2;

// de lijnstukken die de cuts bepalen..
struct LineSeg {
	u_long ID; // de MySql ID
	u_long Stage;
	float X1; 
	float Y1;
	float Z1;
	float X2;
	float Y2;
	float Z2;
	unsigned short Percentage1;
	unsigned short Percentage2;
} GlobalCuts[32];

// het huidig aantal Cuts in de array
u_long	GlobalCutsCount = 0;
//----------------------------



#define BLOB_LEN		491520 	// 16777216					//1024*1024;//65535;
//u_char	*log;

/*
//---
// per cartype een struct
struct THighScore {
	u_long		MemberID;
	u_long		TimeID;
	u_long		Split0Time;
	u_long		Split1Time;
	u_long		Split2Time;
	u_long		RaceTime;
	u_char		Nickname[16];
	u_char		Car;
	u_char		CarType;
	u_char		GearBox;
	u_char		Damage;
	long		PosCount;				// aantal posities opgenomen
	u_char		PosRec[BLOB_LEN];   ///	// de opgenomen posities (LastPos records) van een hele race..

};
struct THighScore *HighScores;
*/
void LoadHighScores( u_char aStage );


//---
// TABLES controleren / aanmaken indien nodig..
void sqlTABLES();
// Een record toevoegen aan de CMR04_Members..
// Het resultaat is de CMR04_Members.ID
my_ulonglong sqlAddMember( u_char* aIP, u_char* aNickname, u_char* aNationality );
// een member zoeken,
// resulteer het CMR04_Members.ID
my_ulonglong sqlFindMember( u_char* aIP, u_char* aNickname, u_char* aNationality );
// een member zoeken,
// resulteer het CMR04_Members.ID
// Als de member nog geen record heeft in de DB, 
// wordt een nieuw record aangemaakt,
// en de ID van het nieuwe record geresulteerd.
my_ulonglong sqlGetMember( u_char* aIP, u_char* aNickname, u_char* aNationality );
// Een record toevoegen aan de CMR04_Times..
void sqlAddTime( my_ulonglong aMemberID, u_char aDamage, u_char aCar, u_char aGearbox, u_char aStage, u_long aSplit0, u_long aSplit1, u_long aSplit2, u_long aRaceTime, u_long aPosCount, u_char* aPositions );
// De RetireCount-teller ophogen..
void sqlIncRetireCount( my_ulonglong aMemberID );
// De QuitCount-teller ophogen..
void sqlIncQuitCount( my_ulonglong aMemberID );

u_long sqlLoadPositions( u_long aTimeID, u_char *ptrDest );

void sqlRegisterStage( u_char aStage );
void sqlLoadStageRecord( u_char aStage );
void sqlStoreStageRecord( my_ulonglong aMemberID, u_char* aPostions );

u_char *sqlGetFastestNick( u_char aStage );
u_char sqlGetFastestCar( u_char aStage );

u_long sqlAddCut( u_char aStage, unsigned short Procent1, unsigned short Procent2, float aPX1, float aPY1, float aPZ1, float aPX2, float aPY2, float aPZ2 );
void sqlUpdateCut( u_long aID, u_char aStage, unsigned short Procent1, unsigned short Procent2, float aPX1, float aPY1, float aPZ1, float aPX2, float aPY2, float aPZ2 );
void sqlLoadCuts( u_char aStage );

void FillCars();









// TABLES controleren / aanmaken indien nodig..
void sqlTABLES() {
	MYSQL	CN;				// connectie
	u_char	sqlQuery[1024];	// query string
	if ( sqlEnabled == 0 ) return;
	// Een connectie maken met de host
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// CMR04_Members
			// SQL query samenstellen..
			strcpy( sqlQuery, "CREATE TABLE IF NOT EXISTS CMR04_Members ( " );
			strcat( sqlQuery, "ID int unsigned zerofill NOT NULL auto_Increment, " );
			strcat( sqlQuery, "EntryDate timestamp, " );
			strcat( sqlQuery, "Nickname varchar(15), " );
			strcat( sqlQuery, "Nationality varchar(3), " );
			strcat( sqlQuery, "IP varchar(15), " );
			strcat( sqlQuery, "RetireCount int unsigned, " );
			strcat( sqlQuery, "QuitCount int unsigned, " );
			strcat( sqlQuery, "PRIMARY KEY(ID) );" );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				// query uitgevoerd..
			}
			// CMR04_Times
			strcpy( sqlQuery, "CREATE TABLE IF NOT EXISTS CMR04_Times ( " );
			strcat( sqlQuery, "ID int unsigned zerofill NOT NULL auto_Increment, " );
			strcat( sqlQuery, "EntryDate timestamp, " );
			strcat( sqlQuery, "MemberID int, " );
			strcat( sqlQuery, "Damage int, " );
			strcat( sqlQuery, "Car int, " );
			strcat( sqlQuery, "Gearbox int, " );
			strcat( sqlQuery, "Stage int, " );
			strcat( sqlQuery, "Split0 int, " );
			strcat( sqlQuery, "Split1 int, " );
			strcat( sqlQuery, "Split2 int, " );
			strcat( sqlQuery, "RaceTime int, " );
			strcat( sqlQuery, "PRIMARY KEY(ID) );" );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				// query uitgevoerd..
			}
			// CMR04_Cars
			strcpy( sqlQuery, "CREATE TABLE IF NOT EXISTS CMR04_Cars ( " );
			strcat( sqlQuery, "CarID int unsigned NOT NULL, " );
			strcat( sqlQuery, "CarStr varchar(20), " );
			strcat( sqlQuery, "Class VARCHAR(10), " );
			strcat( sqlQuery, "PRIMARY KEY(CarID) );" );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				// query uitgevoerd..
				// nu vullen met auto's
				FillCars();
			}
		}
	}
	// connectie sluiten..
	mysql_close( &CN );
}


// Een record toevoegen aan de MySQL-DB..
// Het resultaat is de CMR04_Members.ID
my_ulonglong sqlAddMember( u_char* aIP, u_char* aNickname, u_char* aNationality ) {
	my_ulonglong	Result=0;
	MYSQL	CN;				// connectie
	u_char	sqlQuery[1024];	// query string
	if ( sqlEnabled == 0 ) return;
	// Een connectie maken met de host
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// De SQL-query string opmaken..
			sprintf( &sqlQuery, "INSERT INTO CMR04_Members (IP,Nickname,Nationality,RetireCount,QuitCount) VALUES ('%s','%s','%s',0,0);", aIP, aNickname, aNationality );
//			sprintf( &sqlQuery, "INSERT INTO CMR04_Members SET IP = '%s', SET Nickname = '%s', SET Nationality = '%s';", aIP, aNickname, aNationality );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				// record is nu toegevoegd..
				Result = mysql_insert_id( &CN );
			}
		}
	}
	// connectie sluiten..
	mysql_close( &CN );
	return Result;
}

// een member zoeken,
// resulteer het CMR04_Members.ID
my_ulonglong sqlFindMember( u_char* aIP, u_char* aNickname, u_char* aNationality ) {
	my_ulonglong	Result=0;
	MYSQL		CN;				// connectie
	MYSQL_RES*	RS;
	MYSQL_ROW	ROW;
	u_char		sqlQuery[1024];	// query string
	if ( sqlEnabled == 0 ) return;
	// Een connectie maken met de host
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// De SQL-query string opmaken..
			sprintf( &sqlQuery, "SELECT ID FROM CMR04_Members WHERE Nickname='%s';", aNickname );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				RS = mysql_use_result( &CN );
				if ( RS != NULL ) {
//					if ( !mysql_eof( &CN ) ) {
						ROW = mysql_fetch_row( RS );
						if ( ROW != NULL ) {
							Result = atoll(ROW[0]);
							//printf( "\n%s\n" , Result );

						}
//					}
					mysql_free_result( RS );
				}
			} else {
				Log2( mysql_error( &CN ), "mysql" );
			}
		} else {
			Log2( mysql_error( &CN ), "mysql" );
		}
	} else {
		Log2( mysql_error( &CN ), "mysql" );
	}
	// connectie sluiten..
	mysql_close( &CN );
	return Result;
}

// een member zoeken,
// resulteer het CMR04_Members.ID
// Als de member nog geen record heeft in de DB, 
// wordt een nieuw record aangemaakt,
// en de ID van het nieuwe record geresulteerd.
my_ulonglong sqlGetMember( u_char* aIP, u_char* aNickname, u_char* aNationality ) {
	my_ulonglong	Result=0;
	if ( sqlEnabled == 0 ) return;
	// bestaat er al een record voor de opgegeven member..??
	Result = sqlFindMember( aIP, aNickname, aNationality );
	if ( Result == 0 ) {
		// nog geen record bestaat, meteen aanmaken dan..INSERT
		Result = sqlAddMember( aIP, aNickname, aNationality );
	}
	return Result;
}

// resultaat is het aantal gelezen positie-recordjes
u_long sqlLoadStageRecord_OLD( u_char aStage ) {
	u_long		Result=0;
	MYSQL		CN;				// connectie
	MYSQL_RES*	RS;
	MYSQL_ROW	ROW;
	u_char		sqlQuery[1024];	// query string
//	u_char		log[1024];
	u_char		*start;
	unsigned int	i;
	u_char		log[1024];
	u_long		TimeID;

	if ( sqlEnabled == 0 ) return 0;
	// Een connectie maken met de host
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// De SQL-query string opmaken..
//			sprintf( &sqlQuery, "SELECT Positions FROM CMR04_Records WHERE Stage=%d", aStage );
			TimeID = sqlGetFastestTimeID( aStage );
			sprintf( &sqlQuery, "SELECT Positions FROM CMR04_Positions WHERE TimeID=%d", TimeID );
			//Log( &sqlQuery );

			Result = sqlLoadPositions( TimeID, &ptr_players[0].PosRec[0] );
			ptr_players[0].PosCount = 0;
/*
			// SQL-query uitvoeren..
			if ( mysql_real_query(&CN, &sqlQuery, strlen(&sqlQuery) ) == 0  ) {
			//if ( mysql_query(&CN, &sqlQuery ) == 0  ) {
				RS = mysql_use_result( &CN );
				if ( RS != NULL ) {
				
unsigned long *lengths;
unsigned int num_fields;
//unsigned int i;

ROW = mysql_fetch_row(RS);
if (ROW)
{
    num_fields = mysql_num_fields(RS);
    lengths = mysql_fetch_lengths(RS);
	Result = lengths[0] / 27;
    //for(i = 0; i < num_fields; i++)
    //{
        //sprintf(log, "Column %u is %lu bytes in length.", i, lengths[i]);
		//Log( log );
                
    //}
			

//					if ( !mysql_eof( &CN ) ) {
						//ROW = mysql_fetch_row( RS );

						//( MYSQL_ROW ) ptr_players[0].PosRec[0] = mysql_fetch_row( RS );
						//mysql_fetch_lengths()
						//Log( "row fetched" );
//}						//if ( ROW != NULL ) {
							//Result = atoll(ROW[0]);
							//printf( "\n%s\n" , Result );
							//Log( ROW );
							//Log( "not null" );
						//	i=0;
							//start = &ROW[0];
							//for (i=0; i<=(BLOB_LEN-32768); i+=32768) {
				//end += mysql_real_escape_string(&CN, end, aPostions[i],65535);
						//		memcpy( &ptr_players[0].PosRec[i], start, 32768 );
						//		//start+=32768;
						//		sprintf( log, "%d bytes loaded", i+32768 );
						//		Log( log );
						//	}
							memcpy( &ptr_players[0].PosRec[0], ROW[0], lengths[0] );
							ptr_players[0].PosCount = 0;
							//Log( ptr_players[0].PosRec[0] );
							sprintf( log, "Stage-record %d (%d bytes) (%d positions) loaded...", aStage, lengths[0], lengths[0]/27 );
							Log( log );
							//PosCount;
						}
//					}
					mysql_free_result( RS );
				}
			} else {
				Log( mysql_error( &CN ) );
			}
*/

		} else {
			Log2( mysql_error( &CN ), "mysql" );
		}
	} else {
		Log2( mysql_error( &CN ), "mysql" );
	}
	// connectie sluiten..
	mysql_close( &CN );
	return Result;

}

u_char sqlNextStage() {
	my_ulonglong	Result;
	MYSQL		CN;				// connectie
	MYSQL_RES*	RS;
	MYSQL_ROW	ROW;
	u_char	sqlQuery[1024];	// query string
	Result = 0;
	if ( sqlEnabled == 0 ) {

//		Log( "MySQL NOT enabled" );
		return;
	}

//	Log( "addtime in MySQL" );
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// De SQL-query string opmaken..
			sprintf( &sqlQuery, "SELECT Stage FROM CMR04_Stats ORDER BY Driven ASC LIMIT 1 ;" );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				RS = mysql_use_result( &CN );
				if ( RS != NULL ) {
//					if ( !mysql_eof( &CN ) ) {
						ROW = mysql_fetch_row( RS );
						if ( ROW != NULL ) {
							Result = atoll(ROW[0]);
//							if ( Result == 0 ) Result = atoll(ROW[1]);
//							if ( Result == 0 ) Result = atoll(ROW[2]);
							//printf( "\n%s\n" , Result );

						}
//					}
					mysql_free_result( RS );
				}
			} else {
				Log2( mysql_error( &CN ), "mysql" );
			}
		} else {
			Log2( mysql_error( &CN ), "mysql" );
		}
	} else {
		Log2( mysql_error( &CN ), "mysql" );
	}
	// connectie sluiten..
	mysql_close( &CN );
	return Result;

}

/// bijhouden hoe vaak een stage gereden wordt...
void sqlRegisterStage( u_char aStage ) {
	MYSQL	CN;				// connectie
	u_char	sqlQuery[1024];	// query string
	if ( sqlEnabled == 0 ) return;
	// Een connectie maken met de host
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// De SQL-query string opmaken..
			sprintf( &sqlQuery, "UPDATE CMR04_Stats SET Driven=Driven+1 WHERE Stage=%d;", aStage );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				// record is nu toegevoegd..
			}
		}
	}
	// connectie sluiten..
	mysql_close( &CN );

}

// resultaat is het aantal gelezen positie-recordjes
void sqlLoadStageRecord( u_char aStage ) {
	int		i;
	u_char	done;
	u_long	pc;
	u_char	idx;
	u_char	log[1024];


	sqlRegisterStage( aStage );

	ptr_players[8].PosCount = sqlLoadPositions( ptr_players[8].TimeID, &ptr_players[8].PosRec[0] );
	ptr_players[9].PosCount = sqlLoadPositions( ptr_players[9].TimeID, &ptr_players[9].PosRec[0] );
	ptr_players[10].PosCount = sqlLoadPositions( ptr_players[10].TimeID, &ptr_players[10].PosRec[0] );
	ptr_players[11].PosCount = sqlLoadPositions( ptr_players[11].TimeID, &ptr_players[11].PosRec[0] );

	GlobalEndPos = 0;
	done = 0;
	for (i=8; i<12 && done==0; i++) {
		pc = ptr_players[i].PosCount;
		if ( pc > 0 ) {
			GlobalEndPos = (ptr_players[i].PosRec[pc*27-2] << 8) + ptr_players[i].PosRec[pc*27-3];
			done = 1;
		}
	}

	// personal records voor alle spelers
	for( i=1; i<NumPlayers; i++) {
		idx = 11 + PlayerIndex[i];
		LoadPersonalRecord_CarType( i, aStage, getCarTypeStr(i), idx );
sprintf( log, "PlayerNr:%d, Loading TimeID: %d", i, ptr_players[idx].TimeID );
MsgprintyIP1( "",0, "<PersonalRecord>", log );
		ptr_players[idx].PosCount = sqlLoadPositions( ptr_players[idx].TimeID, &ptr_players[idx].PosRec[0] );
	}
}



// resultaat is het aantal gelezen positie-recordjes
u_long sqlLoadPositions( u_long aTimeID, u_char *ptrDest ) {
	u_long		Result=0;
	MYSQL		CN;				// connectie
	MYSQL_RES*	RS;
	MYSQL_ROW	ROW;
	u_char		sqlQuery[1024];	// query string
//	u_char		log[1024];
	u_char		*start;
	unsigned int	i;
	u_char		log[1024];
	u_long		TimeID;
	unsigned long *lengths;
	unsigned int num_fields;

	if ( aTimeID == 0 ) return 0;
	if ( sqlEnabled == 0 ) return 0;

sprintf( log, "Loading TimeID: %d", aTimeID );
//Log2( log, "game" );
MsgprintyIP1( "",0, "<highscore>", log );

	// Een connectie maken met de host
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {

			// De SQL-query string opmaken..
			sprintf( &sqlQuery, "SELECT Positions FROM CMR04_Positions WHERE TimeID=%d;", aTimeID );
			//Log( &sqlQuery );

			// SQL-query uitvoeren..
			if ( mysql_real_query(&CN, &sqlQuery, strlen(&sqlQuery) ) == 0  ) {
			//if ( mysql_query(&CN, &sqlQuery ) == 0  ) {
				RS = mysql_use_result( &CN );
				if ( RS != NULL ) {
					ROW = mysql_fetch_row(RS);
					if (ROW) {
						num_fields = mysql_num_fields(RS);
						lengths = mysql_fetch_lengths(RS);
						Result = lengths[0] / 27;

						memcpy( ptrDest, ROW[0], lengths[0] );
						//Log( ptr_players[0].PosRec[0] );
						//sprintf( log, "Positions (%d bytes) (%d positions) loaded...", lengths[0], lengths[0]/27 );
						//Log( log );
						//PosCount;
					} else {
						//Log( "MySQL: niets gevonden" );
					}
					mysql_free_result( RS );
				} else {
					//Log( "MySQL: niets gevonden" );
				}
			} else {
				Log2( mysql_error( &CN ), "mysql" );
			}
		} else {
			Log2( mysql_error( &CN ), "mysql" );
		}
	} else {
		Log2( mysql_error( &CN ), "mysql" );
	}
	// connectie sluiten..
	mysql_close( &CN );
	return Result;

}


void sqlStoreStageRecord( my_ulonglong aMemberID, u_char* aPostions ) {
	MYSQL	CN;				// connectie
	u_char *log[128];
	u_char	sqlQuery[2*BLOB_LEN+10], *end;	// query string
	size_t		i;
	if ( sqlEnabled == 0 ) {
//		Log( "MySQL NOT enabled" );
		return;
	}

//	Log( "addtime in MySQL" );
	// Een connectie maken met de host
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {

			sprintf( &sqlQuery, "DELETE FROM CMR04_Records WHERE Stage=%d;", GlobalStages );
			//Log( &sqlQuery );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, &sqlQuery) == 0  ) {
				// record is nu toegevoegd..
				//Log( "record is nu verwijderd" );
			} else {
				Log2( mysql_error( &CN ), "mysql" );
			}


			sprintf( &sqlQuery, "INSERT INTO CMR04_Records (MemberID,Stage,Positions) VALUES (%d, %d, ", (u_long)aMemberID, GlobalStages );

			end = (unsigned int) &sqlQuery + strlen(&sqlQuery);
			*end++ = '\'';
			//end += mysql_real_escape_string(&CN, end, "ABC",3);
			//end += mysql_real_escape_string(&CN, end, aPostions,BLOB_LEN);

			for (i=0; i<BLOB_LEN; i+=32768) {
				end += mysql_real_escape_string(&CN, end, aPostions+i, 32768);
			}
			
			*end++ = '\'';
			*end++ = ')';

			//Log( &sqlQuery );

			if (mysql_real_query(&CN,sqlQuery,(unsigned int) (end - sqlQuery) )==0) {
			//if ( mysql_query(&CN, &sqlQuery) == 0  ) {
				// record is nu toegevoegd..
				//Log( "Posities-record is nu opgeslagen.." );
			} else {
				Log2( mysql_error( &CN ), "mysql" );
			}
		} else {
			Log2( mysql_error( &CN ), "mysql" );
		}
	} else {
		Log2( mysql_error( &CN ), "mysql" );
	}
	// connectie sluiten..
	mysql_close( &CN );


}

void LoadHighScores( u_char aStage ) {
	u_char	idx;
	u_char	log[1024];

	LoadHighScores_CarType(aStage, "4WD", 8);
	LoadHighScores_CarType(aStage, "2WD", 9);
	LoadHighScores_CarType(aStage, "Group B", 10);
	LoadHighScores_CarType(aStage, "Bonus", 11);
	for( i=1; i<NumPlayers; i++) {
		idx = 11 + PlayerIndex[i];
		LoadPersonalRecord_CarType( i, aStage, getCarTypeStr(i), idx );
sprintf( log, "PlayerNr:%d, Loading TimeID: %d", i, ptr_players[idx].TimeID );
MsgprintyIP1( "",0, "<PersonalRecord HS>", log );
	}
}
//-----------
void LoadHighScores_CarType( u_char aStage, u_char *aCarTypeStr, u_char PlayerNr ) { //PlayerNr == slotnummer
	MYSQL		CN;				// connectie
	MYSQL_RES*	RS;
	MYSQL_ROW	ROW;
	u_char	sqlQuery[1024];	// query string
	u_char		*log[128];
	u_char		*tmpStr[128];
	//int			len;

	if ( sqlEnabled == 0 ) {
//		Log( "MySQL NOT enabled" );
		return;
	}
/*
u_long	MemberID;
u_long	Split0Time;
u_long	Split1Time;
u_long	Split2Time;
u_long	RaceTime;
u_char *Nickname;
u_char	Car;
u_char	CarType;
u_char	GearBox;
u_char	Damage;
*/
//	Log( "LoadHighScores in MySQL" );
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// er zijn 4 CarTypes: "4WD", "2WD", "Group B", "Bonus"


			// De SQL-query string opmaken..voor 4WD
			sprintf( &sqlQuery, "SELECT CMR04_Members.ID, CMR04_Times.ID, Nickname, Split0, Split1, Split2, RaceTime, Car, CMR04_Cars.Class, Gearbox, Damage FROM CMR04_Times INNER JOIN CMR04_Members ON CMR04_Members.ID=CMR04_Times.MemberID INNER JOIN CMR04_Cars ON CMR04_Cars.CarID=CMR04_Times.Car WHERE Stage=%d AND CMR04_Cars.Class='%s' ORDER BY RaceTime ASC LIMIT 1 ;", aStage, aCarTypeStr );
			//								0					1				2		3		4		5		6		7		8				9
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				RS = mysql_use_result( &CN );
				if ( RS != NULL ) {
//					if ( !mysql_eof( &CN ) ) {
						ROW = mysql_fetch_row( RS );
						if ( ROW != NULL ) {

							ptr_players[PlayerNr].MemberID = atoll(ROW[0]);
							ptr_players[PlayerNr].TimeID = atoll(ROW[1]);
							strcpy( ptr_players[PlayerNr].Nick, ROW[2] );
							ptr_players[PlayerNr].SplitTime[0] = atoll(ROW[3]);
							ptr_players[PlayerNr].SplitTime[1] = atoll(ROW[4]);
							ptr_players[PlayerNr].SplitTime[2] = atoll(ROW[5]);
							ptr_players[PlayerNr].SplitTime[3] = atoll(ROW[6]);
							ptr_players[PlayerNr].RaceTime = atoll(ROW[6]);
							ptr_players[PlayerNr].Car = (u_char)atoll(ROW[7]);
							strcpy( tmpStr, ROW[8] );
							if ( strcmp(tmpStr,"4WD")==0 ) ptr_players[PlayerNr].CarType = 0;
							if ( strcmp(tmpStr,"2WD")==0 ) ptr_players[PlayerNr].CarType = 1;
							if ( strcmp(tmpStr,"Group B")==0 ) ptr_players[PlayerNr].CarType = 2;
							if ( strcmp(tmpStr,"Bonus")==0 ) ptr_players[PlayerNr].CarType = 3;
							ptr_players[PlayerNr].Gearbox = (u_char)atoll(ROW[9]);
							ptr_players[PlayerNr].Damage = (u_char)atoll(ROW[10]);
						}
//					}
					mysql_free_result( RS );
				}
			} else {
				Log2( mysql_error( &CN ), "mysql" );
			}
		} else {
			Log2( mysql_error( &CN ), "mysql" );
		}
	} else {
		Log2( mysql_error( &CN ), "mysql" );
	}
	// connectie sluiten..
	mysql_close( &CN );
}
//-----------
void LoadPersonalRecord_CarType( u_char PlayerNr,  u_char aStage, u_char *aCarTypeStr, u_char DestSlotNr ) {
	MYSQL		CN;				// connectie
	MYSQL_RES*	RS;
	MYSQL_ROW	ROW;
	u_char	sqlQuery[1024];	// query string
	u_char		*log[128];
	u_char		*tmpStr[128];
	//int			len;
	int			j;


	if ( sqlEnabled == 0 ) {
//		Log( "MySQL NOT enabled" );
		return;
	}


//	Log( "LoadHighScores in MySQL" );
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// er zijn 4 CarTypes: "4WD", "2WD", "Group B", "Bonus"


			// De SQL-query string opmaken..voor 4WD
			sprintf( &sqlQuery, "SELECT CMR04_Members.ID, CMR04_Times.ID, Nickname, Split0, Split1, Split2, RaceTime, Car, CMR04_Cars.Class, Gearbox, Damage FROM CMR04_Times INNER JOIN CMR04_Members ON CMR04_Members.ID=CMR04_Times.MemberID INNER JOIN CMR04_Cars ON CMR04_Cars.CarID=CMR04_Times.Car WHERE Stage=%d AND CMR04_Cars.Class='%s' AND MemberID=%d ORDER BY RaceTime ASC LIMIT 1 ;", aStage, aCarTypeStr, getMemberID(PlayerNr) );
			//								0					1				2		3		4		5		6		7		8				9
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				RS = mysql_use_result( &CN );
				if ( RS != NULL ) {
//					if ( !mysql_eof( &CN ) ) {
						ROW = mysql_fetch_row( RS );
						if ( ROW != NULL ) {

							ptr_players[DestSlotNr].MemberID = atoll(ROW[0]);
							ptr_players[DestSlotNr].TimeID = atoll(ROW[1]);
							strcpy( ptr_players[DestSlotNr].Nick, ROW[2] );
							ptr_players[DestSlotNr].SplitTime[0] = atoll(ROW[3]);
							ptr_players[DestSlotNr].SplitTime[1] = atoll(ROW[4]);
							ptr_players[DestSlotNr].SplitTime[2] = atoll(ROW[5]);
							ptr_players[DestSlotNr].SplitTime[3] = atoll(ROW[6]);
							ptr_players[DestSlotNr].RaceTime = atoll(ROW[6]);
							ptr_players[DestSlotNr].Car = (u_char)atoll(ROW[7]);
							strcpy( tmpStr, ROW[8] );
							if ( strcmp(tmpStr,"4WD")==0 ) ptr_players[DestSlotNr].CarType = 0;
							if ( strcmp(tmpStr,"2WD")==0 ) ptr_players[DestSlotNr].CarType = 1;
							if ( strcmp(tmpStr,"Group B")==0 ) ptr_players[DestSlotNr].CarType = 2;
							if ( strcmp(tmpStr,"Bonus")==0 ) ptr_players[DestSlotNr].CarType = 3;
							ptr_players[DestSlotNr].Gearbox = (u_char)atoll(ROW[9]);
							ptr_players[DestSlotNr].Damage = (u_char)atoll(ROW[10]);
						} else {
							ptr_players[DestSlotNr].MemberID = 0;
							ptr_players[DestSlotNr].TimeID = 0;
							strcpy( ptr_players[DestSlotNr].Nick, "" );
							ptr_players[DestSlotNr].SplitTime[0] = 0;
							ptr_players[DestSlotNr].SplitTime[1] = 0;
							ptr_players[DestSlotNr].SplitTime[2] = 0;
							ptr_players[DestSlotNr].SplitTime[3] = 0;
							ptr_players[DestSlotNr].RaceTime = 0;
							ptr_players[DestSlotNr].Car = 0;
							ptr_players[DestSlotNr].CarType = 0;
							ptr_players[DestSlotNr].Gearbox = 0;
							ptr_players[DestSlotNr].Damage = 0;
						}
//					}
					mysql_free_result( RS );
				}
			} else {
				Log2( mysql_error( &CN ), "mysql" );
			}
		} else {
			Log2( mysql_error( &CN ), "mysql" );
		}
	} else {
		Log2( mysql_error( &CN ), "mysql" );
	}
	// connectie sluiten..
	mysql_close( &CN );
}






// Een snelste tijd opvragen
u_char *sqlGetFastestNick( u_char aStage ) {
	u_char		Result[16]="\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
	MYSQL		CN;				// connectie
	MYSQL_RES*	RS;
	MYSQL_ROW	ROW;
	u_char	sqlQuery[1024];	// query string
	u_char		*log[128];
	//int			len;

	if ( sqlEnabled == 0 ) {
//		Log( "MySQL NOT enabled" );
		return;
	}

//	Log( "addtime in MySQL" );
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// De SQL-query string opmaken..
			sprintf( &sqlQuery, "SELECT Nickname FROM CMR04_Members WHERE ID = (SELECT MemberID FROM CMR04_Times WHERE Stage=%d ORDER BY RaceTime ASC LIMIT 1) ;", aStage );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				RS = mysql_use_result( &CN );
				if ( RS != NULL ) {
//					if ( !mysql_eof( &CN ) ) {
						ROW = mysql_fetch_row( RS );
						if ( ROW != NULL ) {

			//Log( "snelste Nick laden" );

							//sprintf( Result, "%s", ROW[0] );
							//Result = atoll(ROW[0]);
							//sprintf( log, "snelste Nick: %s", &ROW );
							//Log( log );
							//memcpy( Result, ROW[0], 15 );
							//Result = ROW[0];

							//sprintf( log, "%s" , Result );
							//Log( log );
							//len = strlen(ROW[0] );
							//sprintf( log, "lengte Nick: %d", len );
							//Log( log );
							strcpy( Result, ROW[0] );
							//Log( Result );
							//memcpy( &Result, ROW[0], len );   /// moet zoiets worden ?
//							return ROW[0] ;   ///  en free result dan ????????????????



						}
//					}
					mysql_free_result( RS );
				}
			} else {
				Log2( mysql_error( &CN ), "mysql" );
			}
		} else {
			Log2( mysql_error( &CN ), "mysql" );
		}
	} else {
		Log2( mysql_error( &CN ), "mysql" );
	}
	// connectie sluiten..
	mysql_close( &CN );
	return Result;
}

// Een snelste tijd opvragen
u_char *sqlGetFastestNick_CarType( u_char aStage, u_char *aCarTypeStr ) {
	u_char		Result[16]="\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
	MYSQL		CN;				// connectie
	MYSQL_RES*	RS;
	MYSQL_ROW	ROW;
	u_char	sqlQuery[1024];	// query string
	u_char		*log[128];
	//int			len;

	if ( sqlEnabled == 0 ) {
//		Log( "MySQL NOT enabled" );
		return;
	}

//	Log( "addtime in MySQL" );
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// De SQL-query string opmaken..
			sprintf( &sqlQuery, "SELECT Nickname FROM CMR04_Members WHERE ID = (SELECT MemberID FROM CMR04_Times INNER JOIN CMR04_Cars ON CMR04_Cars.CarID=CMR04_Times.Car  WHERE Stage=%d AND CMR04_Cars.Class='%s' ORDER BY RaceTime ASC LIMIT 1) ;", aStage, aCarTypeStr );
			//sprintf( &sqlQuery, "SELECT RaceTime FROM CMR04_Times INNER JOIN CMR04_Cars ON CMR04_Cars.CarID=CMR04_Times.Car WHERE Stage=%d AND CMR04_Cars.Class='%s' ORDER BY RaceTime ASC LIMIT 1 ;", aStage, aCarTypeStr );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				RS = mysql_use_result( &CN );
				if ( RS != NULL ) {
//					if ( !mysql_eof( &CN ) ) {
						ROW = mysql_fetch_row( RS );
						if ( ROW != NULL ) {

			//Log( "snelste Nick laden" );

							//sprintf( Result, "%s", ROW[0] );
							//Result = atoll(ROW[0]);
							//sprintf( log, "snelste Nick: %s", &ROW );
							//Log( log );
							//memcpy( Result, ROW[0], 15 );
							//Result = ROW[0];

							//sprintf( log, "%s" , Result );
							//Log( log );
							//len = strlen(ROW[0] );
							//sprintf( log, "lengte Nick: %d", len );
							//Log( log );
							strcpy( Result, ROW[0] );
							//Log( Result );
							//memcpy( &Result, ROW[0], len );   /// moet zoiets worden ?
//							return ROW[0] ;   ///  en free result dan ????????????????



						}
//					}
					mysql_free_result( RS );
				}
			} else {
				Log2( mysql_error( &CN ), "mysql" );
			}
		} else {
			Log2( mysql_error( &CN ), "mysql" );
		}
	} else {
		Log2( mysql_error( &CN ), "mysql" );
	}
	// connectie sluiten..
	mysql_close( &CN );
	return Result;
}

// Een snelste tijd CAR  opvragen
u_char sqlGetFastestCar( u_char aStage ) {
	u_char		Result=0;
	MYSQL		CN;				// connectie
	MYSQL_RES*	RS;
	MYSQL_ROW	ROW;
	u_char	sqlQuery[1024];	// query string
	u_char		*log[128];
	//int			len;

	if ( sqlEnabled == 0 ) {
//		Log( "MySQL NOT enabled" );
		return;
	}

//	Log( "addtime in MySQL" );
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// De SQL-query string opmaken..
			sprintf( &sqlQuery, "SELECT Car FROM CMR04_Times WHERE Stage=%d ORDER BY RaceTime ASC LIMIT 1 ;", aStage );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				RS = mysql_use_result( &CN );
				if ( RS != NULL ) {
//					if ( !mysql_eof( &CN ) ) {
						ROW = mysql_fetch_row( RS );
						if ( ROW != NULL ) {

							Result = ROW[0];

						}
//					}
					mysql_free_result( RS );
				}
			} else {
				Log2( mysql_error( &CN ), "mysql" );
			}
		} else {
			Log2( mysql_error( &CN ), "mysql" );
		}
	} else {
		Log2( mysql_error( &CN ), "mysql" );
	}
	// connectie sluiten..
	mysql_close( &CN );
	return Result;
}


// Een snelste tijd opvragen
u_long sqlGetFastestTime( u_char aStage ) {
	my_ulonglong	Result;
	MYSQL		CN;				// connectie
	MYSQL_RES*	RS;
	MYSQL_ROW	ROW;
	u_char	sqlQuery[1024];	// query string
	Result = 0;
	if ( sqlEnabled == 0 ) {

//		Log( "MySQL NOT enabled" );
		return;
	}

//	Log( "addtime in MySQL" );
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// De SQL-query string opmaken..
			sprintf( &sqlQuery, "SELECT RaceTime FROM CMR04_Times WHERE Stage=%d ORDER BY RaceTime ASC LIMIT 1 ;", aStage );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				RS = mysql_use_result( &CN );
				if ( RS != NULL ) {
//					if ( !mysql_eof( &CN ) ) {
						ROW = mysql_fetch_row( RS );
						if ( ROW != NULL ) {
							Result = atoll(ROW[0]);
//							if ( Result == 0 ) Result = atoll(ROW[1]);
//							if ( Result == 0 ) Result = atoll(ROW[2]);
							//printf( "\n%s\n" , Result );

						}
//					}
					mysql_free_result( RS );
				}
			} else {
				Log2( mysql_error( &CN ), "mysql" );
			}
		} else {
			Log2( mysql_error( &CN ), "mysql" );
		}
	} else {
		Log2( mysql_error( &CN ), "mysql" );
	}
	// connectie sluiten..
	mysql_close( &CN );
	return Result;
}

// Een snelste tijd-id opvragen
u_long sqlGetFastestTimeID( u_char aStage ) {
	u_long		Result;
	MYSQL		CN;				// connectie
	MYSQL_RES*	RS;
	MYSQL_ROW	ROW;
	u_char	sqlQuery[1024];	// query string
	Result = 0;
	if ( sqlEnabled == 0 ) {

//		Log( "MySQL NOT enabled" );
		return;
	}

//	Log( "addtime in MySQL" );
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// De SQL-query string opmaken..
			sprintf( &sqlQuery, "SELECT ID FROM CMR04_Times WHERE Stage=%d ORDER BY RaceTime ASC LIMIT 1 ;", aStage );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				RS = mysql_use_result( &CN );
				if ( RS != NULL ) {
//					if ( !mysql_eof( &CN ) ) {
						ROW = mysql_fetch_row( RS );
						if ( ROW != NULL ) {
							Result = atol(ROW[0]);
//							if ( Result == 0 ) Result = atoll(ROW[1]);
//							if ( Result == 0 ) Result = atoll(ROW[2]);
							//printf( "\n%s\n" , Result );

						}
//					}
					mysql_free_result( RS );
				}
			} else {
				Log2( mysql_error( &CN ), "mysql" );
			}
		} else {
			Log2( mysql_error( &CN ), "mysql" );
		}
	} else {
		Log2( mysql_error( &CN ), "mysql" );
	}
	// connectie sluiten..
	mysql_close( &CN );
	return Result;
}


// Een snelste tijd opvragen voor een CarType..
u_long sqlGetFastestTime_CarType( u_char aStage, u_char *aCarTypeStr ) {
	my_ulonglong	Result;
/*
	if ( aCarTypeStr=="4WD" ) {
		ptr_players[
	} else 
	if ( aCarTypeStr=="2WD" ) {
	} else 
	if ( aCarTypeStr=="Group B" ) {
	} else 
	if ( aCarTypeStr=="Bonus" ) {
	}
	MYSQL		CN;				// connectie
	MYSQL_RES*	RS;
	MYSQL_ROW	ROW;
	u_char	sqlQuery[1024];	// query string
	Result = 0;
	if ( sqlEnabled == 0 ) {

//		Log( "MySQL NOT enabled" );
		return;
	}

//	Log( "addtime in MySQL" );
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// De SQL-query string opmaken..
			sprintf( &sqlQuery, "SELECT RaceTime FROM CMR04_Times INNER JOIN CMR04_Cars ON CMR04_Cars.CarID=CMR04_Times.Car WHERE Stage=%d AND CMR04_Cars.Class='%s' ORDER BY RaceTime ASC LIMIT 1 ;", aStage, aCarTypeStr );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				RS = mysql_use_result( &CN );
				if ( RS != NULL ) {
//					if ( !mysql_eof( &CN ) ) {
						ROW = mysql_fetch_row( RS );
						if ( ROW != NULL ) {
							Result = atoll(ROW[0]);
//							if ( Result == 0 ) Result = atoll(ROW[1]);
//							if ( Result == 0 ) Result = atoll(ROW[2]);
							//printf( "\n%s\n" , Result );

						}
//					}
					mysql_free_result( RS );
				}
			} else {
				Log( mysql_error( &CN ) );
			}
		} else {
			Log( mysql_error( &CN ) );
		}
	} else {
		Log( mysql_error( &CN ) );
	}
	// connectie sluiten..
	mysql_close( &CN );
*/

	return Result;
}


// Een record toevoegen aan de MySQL-DB..
void sqlAddTime( my_ulonglong aMemberID, u_char aDamage, u_char aCar, u_char aGearbox, u_char aStage, u_long aSplit0, u_long aSplit1, u_long aSplit2, u_long aRaceTime, u_long aPosCount, u_char* aPositions ) {
	MYSQL	CN;				// connectie
	MYSQL_RES*	RS;
	MYSQL_ROW	ROW;
	u_char *log[128];
	u_char	sqlQuery[2*BLOB_LEN+10], *end;	// query string
	u_long	TimeID;
	u_long	Result;

	if ( sqlEnabled == 0 ) {
//		Log( "MySQL NOT enabled" );
		return;
	}

//	Log( "addtime in MySQL" );
	// Een connectie maken met de host
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {

			Result = 0;
			TimeID = 0;
			// De SQL-query string opmaken..
			sprintf( &sqlQuery, "SELECT RaceTime, ID FROM CMR04_Times WHERE Stage=%d AND MemberID=%d ORDER BY RaceTime ASC LIMIT 1;", aStage, aMemberID );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				RS = mysql_use_result( &CN );
				if ( RS != NULL ) {
//					if ( !mysql_eof( &CN ) ) {
						ROW = mysql_fetch_row( RS );
						if ( ROW != NULL ) {
							Result = atoll(ROW[0]);
							TimeID = atoll(ROW[1]);
//							if ( Result == 0 ) Result = atoll(ROW[1]);
//							if ( Result == 0 ) Result = atoll(ROW[2]);
							//printf( "\n%s\n" , Result );
						}
//					}
					mysql_free_result( RS );
				}
			} else {
				Log2( mysql_error( &CN ), "mysql" );
			}


			//if ( aRaceTime < Result && Result != 0 ) { // sneller dan ooit voor deze player
			if ( TimeID != 0 && aRaceTime < Result ) { //
				// De SQL-query string opmaken..
				sprintf( &sqlQuery, "UPDATE CMR04_Times Set Damage=%d,Car=%d,Gearbox=%d,Split0=%d,Split1=%d,Split2=%d,RaceTime=%d WHERE MemberID=%d AND Stage=%d;", aDamage, aCar, aGearbox, aSplit0, aSplit1, aSplit2, aRaceTime, aMemberID, aStage );
				if ( mysql_query(&CN, sqlQuery) == 0  ) {
					// record is nu toegevoegd..
					//Log( sqlQuery );
				} else {
					Log2( mysql_error( &CN ), "mysql" );
				}

				sprintf( &sqlQuery, "DELETE FROM CMR04_Positions WHERE TimeID=%d;", TimeID );
				if ( mysql_query(&CN, sqlQuery) == 0  ) {
					// record is nu verwijderd..
					//Log( sqlQuery );
				} else {
					Log2( mysql_error( &CN ), "mysql" );
				}
				

			} else if ( TimeID == 0 ) { // geen snelste tijd voor deze speler gevonden... aanmaken
				// De SQL-query string opmaken..
				sprintf( &sqlQuery, "INSERT INTO CMR04_Times (MemberID,Damage,Car,Gearbox,Stage,Split0,Split1,Split2,RaceTime ) VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d );", (u_long)aMemberID, aDamage, aCar, aGearbox, aStage, aSplit0, aSplit1, aSplit2, aRaceTime );
				if ( mysql_query(&CN, sqlQuery) == 0  ) {
					// record is nu toegevoegd..
					//Log( sqlQuery );
				} else {
					Log2( mysql_error( &CN ), "mysql" );
				}
				TimeID=mysql_insert_id(&CN);	

			} else {
				TimeID = 0;  // TimeID  op 0   voor als je niet sneller was... anders wordt positie toegevoegd
			}

			// positions bewaren..
			if ( TimeID != 0 ) {
				sprintf( &sqlQuery, "INSERT INTO CMR04_Positions ( TimeID, PosCount, Positions) VALUES ( %d, %d, ", TimeID, aPosCount );

				end = (unsigned int) &sqlQuery + strlen(&sqlQuery);
				*end++ = '\'';

				for (i=0; i<aPosCount*27 && i<BLOB_LEN; i+=27) {
					end += mysql_real_escape_string(&CN, end, aPositions+i, 27);
				}
				
				*end++ = '\'';
				*end++ = ')';

				// SQL-query uitvoeren..
				if (mysql_real_query(&CN,sqlQuery,(unsigned int) (end - sqlQuery) )==0) {
					// record is nu toegevoegd..
					//Log( sqlQuery );
				} else {
					Log2( mysql_error( &CN ), "mysql" );
				}
			}

			
			
		} else {
			Log2( mysql_error( &CN ), "mysql" );
		}
	} else {
		Log2( mysql_error( &CN ), "mysql" );
	}
	// connectie sluiten..
	mysql_close( &CN );
}


//	sqlAddTime_CarType(		MemberID,				GlobalDamage,			Car,		Gearbox,		GlobalStages, Split0,		Split1,				Split2,				RaceTime, getCarTypeStr(playerNumber), ptr_players[PlayerIndex[playerNumber]].PosCount, &ptr_players[PlayerIndex[playerNumber]].PosRec[0] );
// Een record toevoegen aan de MySQL-DB..
void sqlAddTime_CarType( u_long aMemberID, u_char aDamage, u_char aCar, u_char aGearbox, u_char aStage, u_long aSplit0, u_long aSplit1, u_long aSplit2, u_long aRaceTime, char *aCarTypeStr, u_long aPosCount, u_char* aPositions ) {
	MYSQL	CN;				// connectie
	MYSQL	CN2;				// connectie
	MYSQL_RES*	RS;
	MYSQL_ROW	ROW;
	u_char *log[128];
	u_char	sqlQuery[2*BLOB_LEN+10], *end;	// query string
	u_long	TimeID;
	u_long	Result;
	u_long	TopTime;
	u_long	TopTimeID;

	if ( sqlEnabled == 0 ) {
//		Log( "MySQL NOT enabled" );
		return;
	}

	//sprintf( log, "Adding Time for member: %d %d %d %d %d %d %d %d %d", MemberID, GlobalDamage, Car, Gearbox, GlobalStages, Split0, Split1, Split2, RaceTime );
//	sprintf( log, "Adding Time for member: %d %d %d %d %d %d %d %d %d", aMemberID, aDamage, aCar, aGearbox, aStage, aSplit0, aSplit1, aSplit2, aRaceTime );
//	MsgprintyIP(inet_ntoa(peer2.sin_addr), ntohs(peer2.sin_port), log );
//	Log( "addtime in MySQL" );
	// Een connectie maken met de host
	if ( mysql_init( &CN ) ) {
		if ( mysql_init( &CN2 ) ) {
			if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  
				&& mysql_real_connect( &CN2, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {

				Result = 0;
				TimeID = 0;
				TopTime = 0;
				TopTimeID = 0;//??
				// ik heb nog een ideetje
				// een lusje die TimeID opvraagt en verwijdert... tot ie er geen meer vindt
				// en dan tijd toevoegen
				/// hij doet nu ook altijd verwijderen en opnieuw toevoegen... alleen maar 1tje
				//  eerst 1 maal door de tijden ... kijken of er meer zijn.. zoja.. is de snelste sneller.. dan die overhouden anders alles weg
				//  nu blijven alle snellere staan.. kunnen er nog meer dan 1 zijn.  /// zo nie anders...lus voor nop hihi

				// De SQL-query string opmaken..
				sprintf( &sqlQuery, "SELECT RaceTime, ID FROM CMR04_Times INNER JOIN CMR04_Cars ON CMR04_Cars.CarID=CMR04_Times.Car WHERE Stage=%d AND MemberID=%d AND CMR04_Cars.Class='%s' ORDER BY RaceTime ASC;", aStage, aMemberID, aCarTypeStr );
				// SQL-query uitvoeren..
				if ( mysql_query(&CN, sqlQuery) == 0  ) {
					RS = mysql_use_result( &CN );
					if ( RS != NULL ) {
	//					if ( !mysql_eof( &CN ) ) {
							ROW = mysql_fetch_row( RS );
							while ( ROW!=NULL ) {
								Result = atol(ROW[0]);
								TimeID = atol(ROW[1]);
								/// Result = tijd in tabel... is groter dan nieuwe racetijd = delete
							if ( TimeID != 0 && aRaceTime < Result ) {  /// als aRaceTime 0 is... moet ie hier toch komen ?jadacht ik ook maar doet ie niet :S
									sprintf( &sqlQuery, "DELETE FROM CMR04_Times WHERE ID=%d;", TimeID );
									if ( mysql_query(&CN2, sqlQuery) == 0  ) {
										// record is nu verwijderd..
										//Log2( sqlQuery );
									} else {
										Log2( mysql_error( &CN2 ), "mysql" );
									}

									sprintf( &sqlQuery, "DELETE FROM CMR04_Positions WHERE TimeID=%d;", TimeID );
									if ( mysql_query(&CN2, sqlQuery) == 0  ) {
										// record is nu verwijderd..
										//Log( sqlQuery );
									} else {
										Log2( mysql_error( &CN2 ), "mysql" );
									}
								} else {
									// een snellere tijd in DB.. dan de huidige... 
									if ( TopTime > Result || TopTime==0 ) {
										TopTime = Result;
										TopTimeID = TimeID;
									}
	// volgens mij iets niet goed nog..
	// als er 1 tijd in DB zou staan.. en die is sneller dan huidige..komt ie hier..
	//  TopTime > Result   is dan altijd false
								}

								ROW = mysql_fetch_row( RS );
							}

	//					}
						mysql_free_result( RS );
					}
				} else {
					Log2( mysql_error( &CN ), "mysql" );
				}

				// opnieuw opvragen ?
				// De SQL-query string opmaken..
				sprintf( &sqlQuery, "SELECT RaceTime, ID FROM CMR04_Times INNER JOIN CMR04_Cars ON CMR04_Cars.CarID=CMR04_Times.Car WHERE Stage=%d AND MemberID=%d AND CMR04_Cars.Class='%s' ORDER BY RaceTime ASC;", aStage, aMemberID, aCarTypeStr );
				// SQL-query uitvoeren..
				if ( mysql_query(&CN, sqlQuery) == 0  ) {
					RS = mysql_use_result( &CN );
					if ( RS != NULL ) {
	//					if ( !mysql_eof( &CN ) ) {
							ROW = mysql_fetch_row( RS );
							while ( ROW!=NULL ) {
								Result = atol(ROW[0]);
								TimeID = atol(ROW[1]);

								/// Result = tijd in tabel... is groter dan snelste racetijd = delete
							if ( TimeID != TopTimeID && TopTimeID != 0 ) { //&& TopTime < Result ) {
	// || ??
									sprintf( &sqlQuery, "DELETE FROM CMR04_Times WHERE ID=%d;", TimeID );
									if ( mysql_query(&CN2, sqlQuery) == 0  ) {
										// record is nu verwijderd..
										//Log( sqlQuery );
									} else {
										Log2( mysql_error( &CN2 ), "mysql" );
									}

									sprintf( &sqlQuery, "DELETE FROM CMR04_Positions WHERE TimeID=%d;", TimeID );
									if ( mysql_query(&CN2, sqlQuery) == 0  ) {
										// record is nu verwijderd..
										//Log( sqlQuery );
									} else {
										Log2( mysql_error( &CN2 ), "mysql" );
									}
								} else {
									// een snellere tijd.. dan de huidige... 
								}

								ROW = mysql_fetch_row( RS );
							}

	//					}
						mysql_free_result( RS );
					}
				} else {
					Log2( mysql_error( &CN ), "mysql" );
				}
				
	/*
				//if ( aRaceTime < Result && Result != 0 ) { // sneller dan ooit voor deze player
				if ( TimeID != 0 && aRaceTime < Result ) { // wat nu ? :) ja je moet wel ergens checken of de tijd sneller was..ooit/// en anders de snelste dus niet deleten ja
					sprintf( &sqlQuery, "DELETE FROM CMR04_Times WHERE TimeID=%d;", TimeID );
					if ( mysql_query(&CN, sqlQuery) == 0  ) {
						// record is nu verwijderd..
						//Log( sqlQuery );
					} else {
						Log2( mysql_error( &CN ), "mysql" );
					}

					sprintf( &sqlQuery, "DELETE FROM CMR04_Positions WHERE TimeID=%d;", TimeID );
					if ( mysql_query(&CN, sqlQuery) == 0  ) {
						// record is nu verwijderd..
						//Log( sqlQuery );
					} else {
						Log2( mysql_error( &CN ), "mysql" );
					}
					

				} // else if ( TimeID == 0 ) { // geen snelste tijd voor deze speler gevonden... aanmaken

	*/
				//} else {
				//	TimeID = 0;  // TimeID  op 0   voor als je niet sneller was... anders wordt positie toegevoegd
				//}

				// positions bewaren..
				if ( TopTimeID == 0 ) {

					// De SQL-query string opmaken..
					sprintf( &sqlQuery, "INSERT INTO CMR04_Times (MemberID,Damage,Car,Gearbox,Stage,Split0,Split1,Split2,RaceTime ) VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d );", (u_long)aMemberID, aDamage, aCar, aGearbox, aStage, aSplit0, aSplit1, aSplit2, aRaceTime );
					if ( mysql_query(&CN, sqlQuery) == 0  ) {
						// record is nu toegevoegd..
						//Log( sqlQuery );
					} else {
						Log2( mysql_error( &CN ), "mysql" );
					}
					TimeID=mysql_insert_id(&CN);	


					sprintf( &sqlQuery, "INSERT INTO CMR04_Positions ( TimeID, PosCount, Positions) VALUES ( %d, %d, ", TimeID, aPosCount );

					end = (unsigned int) &sqlQuery + strlen(&sqlQuery);
					*end++ = '\'';

					for (i=0; i<aPosCount*27 && i<BLOB_LEN; i+=27) {
						end += mysql_real_escape_string(&CN, end, aPositions+i, 27);
					}
					
					*end++ = '\'';
					*end++ = ')';

					// SQL-query uitvoeren..
					if (mysql_real_query(&CN,sqlQuery,(unsigned int) (end - sqlQuery) )==0) {
						// record is nu toegevoegd..
						//Log( sqlQuery );
					} else {
						Log2( mysql_error( &CN ), "mysql" );
					}
				}

				
				
			} else {
				Log2( mysql_error( &CN ), "mysql" );
			}
		} else {
			Log2( mysql_error( &CN2 ), "mysql" );
		}
	} else {
		Log2( mysql_error( &CN ), "mysql" );
	}

	// connectie sluiten..
	mysql_close( &CN );
	mysql_close( &CN2 );
}


void sqlIncRetireCount( my_ulonglong aMemberID ) {
	MYSQL	CN;				// connectie
	u_char	sqlQuery[1024];	// query string
	if ( sqlEnabled == 0 ) return;
	// Een connectie maken met de host
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// De SQL-query string opmaken..
			sprintf( &sqlQuery, "UPDATE CMR04_Members SET RetireCount=RetireCount+1 WHERE ID=%d;", (u_long)aMemberID );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				// record is nu toegevoegd..
			}
		}
	}
	// connectie sluiten..
	mysql_close( &CN );
}

void sqlIncQuitCount( my_ulonglong aMemberID ) {
	MYSQL	CN;				// connectie
	u_char	sqlQuery[1024];	// query string
	if ( sqlEnabled == 0 ) return;
	// Een connectie maken met de host
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			// De SQL-query string opmaken..
			sprintf( &sqlQuery, "UPDATE CMR04_Members SET QuitCount=QuitCount+1 WHERE ID=%d;", (u_long)aMemberID );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				// record is nu toegevoegd..
			}
		}
	}
	// connectie sluiten..
	mysql_close( &CN );
}


// De TABLE CMR04_Cars vullen met alle auto namen
void FillCars() {
	MYSQL	CN;				// connectie
	u_char	sqlQuery[1024];	// query string
	int		i;
	if ( sqlEnabled == 0 ) return;
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			for ( i=0; i<23; i++ ) {
				sprintf( &sqlQuery, "INSERT INTO CMR04_Cars (CarID,CarStr,Class) VALUES ( %d, '%s', '%s' );", i, StrCar[i], StrClass[i] );
				// SQL-query uitvoeren..
				if ( mysql_query(&CN, sqlQuery) == 0  ) {
					// record is nu toegevoegd..
				}
			}
		}
	}
	// connectie sluiten..
	mysql_close( &CN );
}





// een CUT toevoegen, tbv. de NO-CUT controles
// Het resultaat van deze functie is de MySql-ID van het toegevoegde record.
u_long sqlAddCut( u_char aStage, unsigned short Procent1, unsigned short Procent2, float aPX1, float aPY1, float aPZ1, float aPX2, float aPY2, float aPZ2 ) {
	MYSQL	CN;				// connectie
	u_char	sqlQuery[1024];	// query string
	u_long	Result = 0;
	if ( sqlEnabled == 0 ) return;
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			sprintf( &sqlQuery, "INSERT INTO CMR04_Cuts (STAGE,PROCENT1,PROCENT2,POINT1X,POINT1Y,POINT1Z,POINT2X,POINT2Y,POINT2Z) VALUES ( %d, %d, %d, %f, %f, %f, %f, %f, %f );", GlobalStages, Procent1, Procent2, aPX1, aPY1, aPZ1, aPX2, aPY2, aPZ2 );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				// is nu toegevoegd..
				Result = mysql_insert_id(&CN);
			}
		}
	}
	// connectie sluiten..
	mysql_close( &CN );
	return Result;
}

// een CUT wijzigen
void sqlUpdateCut( u_long aID, u_char aStage, unsigned short Procent1, unsigned short Procent2, float aPX1, float aPY1, float aPZ1, float aPX2, float aPY2, float aPZ2 ) {
	MYSQL	CN;				// connectie
	u_char	sqlQuery[1024];	// query string
	if ( sqlEnabled == 0 ) return;
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			sprintf( &sqlQuery, "UPDATE CMR04_Cuts SET STAGE=%d, PROCENT1=%d,PROCENT2=%d,POINT1X=%f,POINT1Y=%f,POINT1Z=%f,POINT2X=%f,POINT2Y=%f,POINT2Z=%f WHERE ID=%d;", GlobalStages, Procent1, Procent2, aPX1, aPY1, aPZ1, aPX2, aPY2, aPZ2, aID );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				// is nu toegevoegd..
			}
		}
	}
	// connectie sluiten..
	mysql_close( &CN );
}


// alle CUTS van een stage laden in de GlobalCuts array
void sqlLoadCuts( u_char aStage ) {
	MYSQL	CN;				// connectie
	u_char	sqlQuery[1024];	// query string
	MYSQL_RES*	RS;
	MYSQL_ROW	ROW;

	GlobalCutsCount = 0;
	if ( sqlEnabled == 0 ) return;
	if ( mysql_init( &CN ) ) {
		if ( mysql_real_connect( &CN, sqlHost, sqlUser, sqlPassword, sqlDB, 0,NULL,0 )  ) {
			sprintf( &sqlQuery, "SELECT * FROM CMR04_Cuts WHERE Stage=%d;", GlobalStages );
			// SQL-query uitvoeren..
			if ( mysql_query(&CN, sqlQuery) == 0  ) {
				RS = mysql_use_result( &CN );
				if ( RS != NULL ) {
					// het aantal gelezen cuts naar de globale array overnemen..
					ROW = mysql_fetch_row( RS );
					while ( ROW != NULL ) {
						GlobalCuts[GlobalCutsCount].ID = atol(ROW[0]);
						GlobalCuts[GlobalCutsCount].Stage = atol(ROW[1]);
						GlobalCuts[GlobalCutsCount].X1 = atof(ROW[2]);
						GlobalCuts[GlobalCutsCount].Y1 = atof(ROW[3]);
						GlobalCuts[GlobalCutsCount].Z1 = atof(ROW[4]);
						GlobalCuts[GlobalCutsCount].X2 = atof(ROW[5]);
						GlobalCuts[GlobalCutsCount].Y2 = atof(ROW[6]);
						GlobalCuts[GlobalCutsCount].Z2 = atof(ROW[7]);
						GlobalCuts[GlobalCutsCount].Percentage1 = atol(ROW[8]);
						GlobalCuts[GlobalCutsCount].Percentage2 = atol(ROW[9]);
						GlobalCutsCount++;
						ROW = mysql_fetch_row( RS );
					}
					// recordset wegmikken..
					mysql_free_result( RS );
				}
			}
		}
	}
	// connectie sluiten..
	mysql_close( &CN );
}





