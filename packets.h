

    u_char  
			// 40 01
			ServerPing[8] = 
				"\x40" "\x00\x00" "\x01" "\x00\x00\x00\x00",	// host timestamp invullen

			// server antwoord op een client ping..
			// 40 02
			ServerPong[8] = 
				"\x40" "\x00\x00" "\x02" "\x00\x00\x00\x00",	// timestamp overnemen

			// de eigen timevalue doorsturen (s->c, c->s)
			// 40 02
			OwnTimeStamp[8] = 
				"\x40" "\xf4\x86" "\x02" "\x84\xc4\xa6\x00",

			// 40 02
            ServerLong50ACK[8] =
                "\x40\x4A\x3B\x02" "\x00\x00\x00\x00",	// timestamp

			// 40 02
			Server_AnswerLongACK[8] = 
				"\x40" "\xe3\x7b" "\x02" "\xe1\x50\x20\x00",

			// 50
			Acknowledge[5] =
                "\x50" "\xE1\xE5" "\x00\x00",

			// 50
			LongAcknowledge[5] =
//				"\x50" "\x20\x54" "\x2a\x00" "\x01" "\x53\xb3\x0c\x00",
				"\x50" "\xc6\x57" "\x0a\x00" "\x02" "\x24\x12\x0e\x00",

		
			// FE FD 02
			clRefresh[8] =
                "\xFE\xFD\x02" "\x00\x00\x00\x00\x00",

			// 05
            clRefreshAnswer[5] = 
                "\x05\x00\x00\x00\x00",


			// FE FD 00
            clMiniServerInfo[17] =
				"\xFE\xFD\x00" "\x00\x00\x00\x00" "\x07\x01\x03\x13\x06\x0b\x08\x0a\x00\x00",

			// FE FD 00....FF FF FF
			clServerInfo[10] =
                "\xFE\xFD\x00" "\x00\x00\x00\x00" "\xFF\xFF\xFF",

                *timestamp_clServerInfo = clServerInfo + 3,	// 4 bytes




			// 08
			ServerWrongPassword[15] =
				"\x08\x07\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01",

			// D8
            ServerJoinReply1[21] =
				"\xD8\xaf\x2c\x00\x00\x00\x00\x07" "\x00\x00\x00\x00\xD0\x07\x00\x00\xD0\x07\x00\x00\x01",
//				"\xD8\xDD\x92\x00\x00\x00\x00\x07" "\x00\x00\x00\x00\xA0\x0F\x00\x00\xA0\x0F\x00\x00\x01",

			// 50
            ServerJoinReply2[5] =
                "\x50\xE1\xE5\x00\x00",


			// C9
            ServerJoinReply3[5] =
                "\xC9\x34\xDC\x01\x00",

			// 50
            ServerJoinReply4[5] =
                "\x50\x5E\x66\x01\x00",


			// C6 80
            ServerJoinReply5[37] =
                "\xC6" "\xD8\x0E" "\x02\x00" "\x80" 
				"\xEA\xA3"				// GameSpy-ID ?? elk spel een eigen waarde..in enkel spel blijft waarde constant.
				"\x00"
				"\x01\x00\x00\x00"		// speler JoinNr
				"\x01\x00\x00\x00\x01\x00\x00\x00\x00\x00\x13\x00\x9C\x0C\x55\x00"
				"\x20\x66\x5C\x00\x42\x10\xD5\x77",


			// C8 07
            clJoin[19+MAXPWD+1] =
                "\xC8"				// type
                "\x00\x00"			// checksum
                "\x00\x00"			// zero
                "\x07"				// type
                CMRVER				// Colin McRae Rally 4 version??? (volgens mij constant 0000)
                MODE_LAN			// 0 = LAN mode, 1 = online
                "\x00\xA0\x0F\x00"
                "\x00\xA0\x0F\x00"
                "\x00" "\x01"		// PWDLEN, 0x01, password, 0x00
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00",

                *checksum	= RXPacket + 1,		// 2 bytes (1 word)
                *PWDLEN		= RXPacket + 17,	// 1 byte (range: 0..MAXPWD)
                *password	= RXPacket + 19,	// PWDLEN bytes (password chars)





            // host server information
            ServerInfo[182] =
                "\x00"
                CLIENTHANDLE		// client request-handle/timestamp-dingetje
                "hostname" "\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00",

                *ServerInfo_clienthandle	= TXPacket + 1,
                *ServerInfo_hostname		= TXPacket + 14,


			// D6 85
			ServerPlayListD6[299] = 
				"\xD6\x0E\x7B" "\x00\x00" "\x03\x00" "\x85"
				"\x08" "\x00"	// MaxPlayers, NumPlayers
				"\x00"
				// player[0]
				"\x00\x00\x00\x00"		// ID
				"\x00\x00\x00\x00"		// READY (0=niet klaar, 1=klaar, 2=niet klaar)
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"	// player[0].Nick + trailing 0
				"\x00"					// player[0].Country
				"\x00\x00\x00"
				"\x00\x00\x00\x00"		// ping??
				"\x00"					// x03 ?
				"\x00"					// player[0].Car
				"\x00"					// player[0].CarType
				"\x00"					// player[0].Gearbox
				// player[1]
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" "\x00" "\x00\x00\x00" "\x00\x00\x00\x00" "\x00" "\x00" "\x00" "\x00"
				// player[2]
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" "\x00" "\x00\x00\x00" "\x00\x00\x00\x00" "\x00" "\x00" "\x00" "\x00"
				// player[3]
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" "\x00" "\x00\x00\x00" "\x00\x00\x00\x00" "\x00" "\x00" "\x00" "\x00"
				// player[4]
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" "\x00" "\x00\x00\x00" "\x00\x00\x00\x00" "\x00" "\x00" "\x00" "\x00"
				// player[5]
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" "\x00" "\x00\x00\x00" "\x00\x00\x00\x00" "\x00" "\x00" "\x00" "\x00"
				// player[6]
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" "\x00" "\x00\x00\x00" "\x00\x00\x00\x00" "\x00" "\x00" "\x00" "\x00"
				// player[7]
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" "\x00" "\x00\x00\x00" "\x00\x00\x00\x00" "\x00" "\x00" "\x00" "\x00",



			// De lijst met spelers (aan reeds ge-join-de spelers)
			// C6 85
			ServerPlayListC6[297] = 
				"\xC6\x00\x00"  "\x00\x00" "\x85"
				"\x08\x00"				// MaxPlayers, NumPlayers
				"\x00"
				// player[0]
				"\x00\x00\x00\x00"		// ID
				"\x00\x00\x00\x00"		// READY (0=niet klaar, 1=klaar, 2=niet klaar)
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"	// player[0].Nick + trailing 0
				"\x00"					// player[0].Country
				"\x00\x00\x00"
				"\x00\x00\x00\x00"		// ping??
				"\x00"					// x03 ?
				"\x00"					// player[0].Car
				"\x00"					// player[0].CarType
				"\x00"					// player[0].Gearbox
				// player[1]
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" "\x00" "\x00\x00\x00" "\x00\x00\x00\x00" "\x00" "\x00" "\x00" "\x00"
				// player[2]
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" "\x00" "\x00\x00\x00" "\x00\x00\x00\x00" "\x00" "\x00" "\x00" "\x00"
				// player[3]
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" "\x00" "\x00\x00\x00" "\x00\x00\x00\x00" "\x00" "\x00" "\x00" "\x00"
				// player[4]
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" "\x00" "\x00\x00\x00" "\x00\x00\x00\x00" "\x00" "\x00" "\x00" "\x00"
				// player[5]
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" "\x00" "\x00\x00\x00" "\x00\x00\x00\x00" "\x00" "\x00" "\x00" "\x00"
				// player[6]
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" "\x00" "\x00\x00\x00" "\x00\x00\x00\x00" "\x00" "\x00" "\x00" "\x00"
				// player[7]
				"\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" "\x00" "\x00\x00\x00" "\x00\x00\x00\x00" "\x00" "\x00" "\x00" "\x00",






			// C6 82
			clCarChange[21] =
				"\xC6"				// type
				"\x63\x81"			// checksum
				"\x20\x00"			// client-linenumber..
				"\x82\x00\x00\x00"	// type
				"\x01\x00\x00\x00"
				"\x00\x00\x00\x00"	// READY
				"\x00"				// car
				"\x00"				// carType
				"\x02"				// gearbox
				"\x00",



			// C6 83
			ServerGameChange[33] = 
				"\xc6"				// type
				"\x1f\xd4"			// checksum
				"\x08\x00"			// ChatLine
				"\x83"				// type
				"\x00\x00\x00\x01\x00\x00\x00"
				"\x01"				// 1==stages, 0==rally
				"\x00"				// damage
				"\x01"				// Ranking: points/time
				"\x00"				// CarType
				"\x03"				// indien stages: land
				"\x04"				//                stage
				"\x55\x00\x20\x66\x5c\x00\x42\x10\xd5\x77\x42\x10\xd5\x77",
				


            // client initiated chat, to host
			// C6 8A
            clChat[97] = 
                "\xC6"				// type
                "\x00\x00"			// checksum
                "\x02\x00"			// client teller/regelnummer
                "\x8A\x00\x00\x00"	// (x8A) client speaks to host
                "\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // chat text content
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00"
                "\x24\x4D\x00",

                *clChat_Line	= RXPacket + 9,		// 4 bytes (1 dword)
                *clChat_Text	= RXPacket + 13,	// 80 bytes (ASCII)


            // host initiated chat, to anyone but the host itself
			// C6 8B
            ChatFromHostToAny[134] =	// 134 length when filled to max.
                "\xC6"					// type
                "\x00\x00"				// checksum
                "\x00\x00"				// clients' linenumber in chatbox
                "\x8B\x01\x61\x73"		// (x8B) (what of this is constant??? test this..)
                "\x00\x00\x00\x00"
                "\x61\x6e\x6b\x29\xef\xd8\x90\x7c"
				"\xe9\x2b\xa5\x71"		// "\xe9\x2b\xa5\x71" met getNick(0), "\x20\x00\x00\x00" van "Server"
				"\x28" "\x05\x00\x00" 
				"\x50"					// "\x50" met getNick(0), "\x01" van "Server"
				"\x01\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"		// MAXNICK bytes (containing host nickname)
                "\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"	// chat text content
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00",

                *ChatFromHostToAny_ClLine	= TXPacket + 3,		// 2 bytes (1 word)
                *ChatFromHostToAny_Line		= TXPacket + 9,		// 4 bytes (1 dword)
                *ChatFromHostToAny_Nick		= TXPacket + 37,	// MAXNICK bytes (maximum)

			// C6 8B
			ChatFromServerToAny[134] = 
				"\xc6"					// type
				"\xc5\x10"				// checksum
				"\x82\x00"				// ChatLine
				"\x8b\x01\x61\x73"		// type
				"\x00\x00\x00\x00"		// ID van de host (altijd 0 voor Server)
				"\x61\x6e\x6b\x29\xef\xd8\x90\x7c"
				"\x20\x00\x00\x00"		// constant..
				"\xdc\xfb\x13\x00"
				"\x01"					// constant..
				"\x00\x00\x00\x0e\x00\x02\x00"
				"\x53\x65\x72\x76\x65\x72\x00"										// "Server"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"	// chat text content
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00",

                *ChatFromServerToAny_Text	= TXPacket + 44,


            // host relayed chat, from anyone but the host itself, to anyone but the speaker himself
			// C6 8B
            ChatRelay[134] =		// 134 length when filled to max.
                "\xC6"				// type
                "\x00\x00"			// checksum
                "\x02\x00"			// clients' linenumber in chatbox
                "\x8B\x01\x00\xC0"	// (x8Bx01x00xC0)
                "\x01\x00\x00\x00"	// chat linenumber (1st = 0)
                "\xac\xfb\x13\x00\xef\xd8\x90\x7c\x44\xfc\x13\x00\x18\xee\x90\x7c\x78\xfb\x90\x7c\xff\xff\xff\xff"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"     // MAXNICK bytes (containing client nickname)
                "\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // chat text content
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00",
                
                *ChatRelay_ClLine	= TXPacket + 3,		// 2 bytes (1 word)
                *ChatRelay_Line		= TXPacket + 9,		// 4 bytes (1 dword)
                *ChatRelay_Nick		= TXPacket + 37,	// MAXNICK bytes (maximum)


            // host relayed chat, from anyone but the host itself, to the speaker himself
			// D6 8B
            ChatRelayToSpeaker[136] =	// 136 length when filled to max.
                "\xD6"					// type
                "\x00\x00"				// checksum
                "\x02\x00"				// deze en volgende zijn => chat-tellers ?
                "\x04\x00"				// client linenumber in chatbox
                "\x8B\x01\x00\xC0"		// (x8Bx01x00xC0)
                "\x01\x00\x00\x00"		// chat linenumber (1st = 0)
                "\xac\xfb\x13\x00\xef\xd8\x90\x7c\x44\xfc\x13\x00\x18\xee\x90\x7c\x78\xfb\x90\x7c\xff\xff\xff\xff"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"     // MAXNICK bytes
                "\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // chat text content
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00",

                *ChatRelayToSpeaker_ClLine	= TXPacket + 5,		// 2 bytes (1 word)
                *ChatRelayToSpeaker_Line	= TXPacket + 11,	// 4 bytes (1 dword)
                *ChatRelayToSpeaker_Nick	= TXPacket + 39,	// MAXNICK bytes (maximum)





			// C6 84
			LeaveLobby[33] = 
				"\xc6"							// type
				"\xd4\xe3"						// checksum
				"\x30\x00"						// ChatLine
				"\x84"							// type
				"\x00\x00\x00"
				"\x02\x00\x00\x00"
				"\x01\x00\x00\x00"
				"\x03"							// country
				"\x04"							// stage
				"\x55\x00\x20\x66\x5c\x00\x42\x10\xd5\x77\x42\x10\xd5\x77",


			// C6 03
			GridReadyC6[13] =
				"\xc6" "\xba\x88"            "\x2c\x00" "\x03" "\x02\x8b\x39" "\x04\x00\x00\x00",
			// D6 03
			GridReadyD6[15] = 
//				"\xd6" "\x4b\x18" "\x03\x00" "\x12\x00" "\x03" "\x7a\xac\x02" "\x07\x00\x00\x00",
				"\xd6" "\x4b\x18" "\x03\x00" "\x12\x00" "\x03" "\x00\x00\x00" "\x07\x00\x00\x00",


			// C6 0E
			clGridPresent1[37] =
				"\xc6" "\x83\xe0" "\x03\x00" "\x0e"
				"\x07"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x2f\xdd\x04\x40\xac\x1c\x4a\x3f\x5c\x8f\x62\x3f\x00\x00\x00\x00",
			// C6 0E
			ServerGridPresent1[37] =
				"\xc6"			// type
				"\x51\xea"		// checksum
				"\x43\x00"		// ChatLine
				"\x0e"			// type
				"\x00"			// PlayerNr
				"\x00\x00\x20\x00\x00\x00\x50\xfa\x13\x00\x50\xd8\x53\x00"				
				"\x91\xed\x00\x40\x8f\xc2\x35\x3f\x7d\x3f\x65\x3f\x00\x00\x00\x00", //copy
			// C6 0E
			ServerGridPresent1a[37] = 
				"\xc6"			// type
				"\xbd\x4e"		// checksum
				"\x33\x00"		// ChatLine
				"\x0e"			// type
				"\x06"			// PlayerNr
				"\x13\x00\x50\xd8\x53\x00\x48\x31\xae\x00\x26\x1a\x93\x5f"
				"\x2f\xdd\x04\x40\xac\x1c\x4a\x3f\x5c\x8f\x62\x3f\x00\x00\x00\x00", //copy
			// C6 0E
			ServerGridPresent1b[37] = 
				"\xc6"
				"\x3a\x8f"
				"\x2f\x00"
				"\x0e"
				"\x02"			// PlayerNr
				"\x13\x00\x50\xd8\x53\x00\x48\x31\xae\x00\x74\xf9\xfe\xce"
				"\x75\x93\xf0\x3f\x42\x60\x35\x3f\xe1\x7a\x64\x3f\x00\x00\x00\x00",	//copy
			// C6 0E
			ServerGridPresent1c[37] = 
				"\xc6"
				"\x02\x43"
				"\x0c\x00"
				"\x0e"
				"\x01"			// PlayerNr
				"\x13\x00\x50\xd8\x53\x00\x48\x31\xae\x00\xdd\x3c\xda\x0e"
				"\x75\x93\xf0\x3f\x42\x60\x35\x3f\xe1\x7a\x64\x3f\x00\x00\x00\x00", //copy
			// C6 0E
			ServerGridPresent1d[37] = 
				"\xc6"
				"\x75\x5c"
				"\x17\x00"
				"\x0e"
				"\x05"
				"\x00\x00\x20\x00\x00\x00\x80\xfb\x13\x00\x50\xd8\x53\x00"
				"\x2f\xdd\x04\x40\xac\x1c\x4a\x3f\x5c\x8f\x62\x3f\x00\x00\x00\x00", //copy

			// D6 0E
			ServerReady2ACK[39] = 
				"\xd6"
				"\xb7\x47"		// checksum
				"\x05\x00"
				"\x0b\x00"		// CMD / ChatLine
				"\x0e"
				"\x02\x13\x00\x50\xd8\x53\x00\x48\x31\xae\x00\x74\xf9\xfe\xce\x75\x93\xf0\x3f\x42\x60\x35\x3f\xe1\x7a\x64\x3f\x00\x00\x00\x00",

			// C6 0D
			clGridPresent2[7] =
				"\xc6" "\x23\xbd" "\x04\x00" "\x0d"
				"\x07",
			ServerGridPresent2[7] = 
				"\xc6"			// type
				"\x23\xbd"		// checksum
				"\x04\x00"		// CMD / ChatLine
				"\x0d"			// type
				"\x00",			// PlayerNr (0==host)


			// C6 04
			ServerStartCountdownC6[6] =
				"\xc6" "\x24\xc3"            "\x49\x00" "\x04",
			// D6 04
			ServerStartCountdownD6[8] =
				"\xd6" "\x94\x17" "\x05\x00" "\x40\x00" "\x04",






			// de ping-lijst die de server dan stuurt..
			// 42 86
			ServerPingList[71] =
				"\x42"
				"\x4c\x2b"			// checksum
				"\x86"				// type
				"\x08"				// aantal geldig in lijst (x/8)
				"\x00\x00"
				"\x00\x00\x00\x00"	// ID host
				"\x02\x00\x00\x00"
				"\x03\x00\x00\x00"
				"\x04\x00\x00\x00"
				"\x05\x00\x00\x00"
				"\x07\x00\x00\x00"
				"\x08\x00\x00\x00"
				"\x09\x00\x00\x00"
				"\x00\x00\x00\x00"	// ping host
				"\xa3\x00\x00\x00"
				"\x78\x00\x00\x00"
				"\x54\x00\x00\x00"
				"\x7f\x00\x00\x00"
				"\x87\x00\x00\x00"
				"\x54\x00\x00\x00"
				"\x38\x00\x00\x00",

			// 43..01....86
			ServerPingListTimestamp[76] = 
				"\x43"
				"\x17\xdf"			// checksum
				"\x01"				// type??
				"\xa4\x9c\xa6\x00"	// timestamp
				"\x86"				// type
				"\x07"				// aantal geldig in lijst (x/8)
				"\x00\x00"
				"\x00\x00\x00\x00"	// ID host
				"\x02\x00\x00\x00"
				"\x03\x00\x00\x00"
				"\x04\x00\x00\x00"
				"\x05\x00\x00\x00"
				"\x07\x00\x00\x00"
				"\x08\x00\x00\x00"
				"\x7c\x4b\x55\x00"	// dummy waarde??
				"\x00\x00\x00\x00"	// ping host
				"\x81\x00\x00\x00"
				"\x53\x00\x00\x00"
				"\xd4\x00\x00\x00"
				"\x94\x00\x00\x00"
				"\x3c\x00\x00\x00"
				"\xad\x00\x00\x00"
				"\xbf\xf1\xed\xc5",	// dummy waarde??


			// 42 06
			ServerPos[8+7*27] = 
				"\x42"				// type
				"\xd2\xc9"
				"\x06"				// type
				"\x07\x00\x00\x00"	// aantal in lijst
				"\x05\x00" "\x82\x2f\x00\x00" "\xff\x00\xc2\x00\x00\x00\xfb\x84\x09\x43\xb4\xfd\x7d\x42\x5a\x21\x11\xc3\x0e\x00\x00"
				"\x05\x02" "\x2c\x2f\x00\x00" "\x27\x00\xe8\x00\x3e\x00\x1d\x18\x12\x43\xb2\x97\x7c\x42\x36\xea\x2a\xc3\x11\x00\x02"
				"\x05\x03" "\x92\x2d\x00\x00" "\x23\x00\xe5\x00\x3c\x00\x29\x40\x07\x43\xe3\xd8\x7c\x42\x0b\x22\x1b\xc3\x0f\x00\x00"
				"\x05\x04" "\x85\x25\x00\x00" "\xef\x00\xd3\x00\x3e\x00\x5f\x5b\x23\x43\x3e\x80\x80\x42\xac\x69\xbc\xc2\x08\x00\x00"
				"\x05\x05" "\x38\x2e\x00\x00" "\x16\x00\xd9\x00\x3d\x00\x39\x94\x0c\x43\x4d\xf8\x7c\x42\x53\x2e\x20\xc3\x10\x00\x00"
				"\x05\x06" "\x4c\x2e\x00\x00" "\x1f\x00\xe1\x00\x3d\x00\x34\x73\x10\x43\x4c\x63\x7c\x42\x5d\xeb\x25\xc3\x10\x00\x00"
				"\x05\x07" "\xca\x2f\x00\x00" "\x2a\x00\xec\x00\x0e\x00\x86\x36\x14\x43\x0a\x4b\x7c\x42\x38\x27\x29\xc3\x11\x00\x00",
/*
			// 42 06
			// Deze wordt verstuurd net! na de ontvangst van een client C6-0B (finish)
			// de speler zelf staat er niet in vermeld (01)
			ServerPos2[8+7*27] = 
				\x42"
				\xe8\x6b"
				\x06"
				\x05\x00\x00\x00"
				\x05\x00" "\xd2\x74\x02\x00" "\xc2\x00\x01\x00\x00\x00\x1e\x7e\x60\xc4\xf8\x8a\x1d\x42\x4d\x53\x81\xc4\x30\x01\x00"
				\x05\x02" "\x3d\x73\x02\x00" "\x0a\x00\x34\x00\x3e\x00\xcc\x78\x6b\xc4\x70\x85\x21\x42\x3b\xcd\xab\xc3\xe7\x01\x02"
				\x05\x05" "\xc4\x73\x02\x00" "\xea\x00\x28\x00\x3d\x00\x8a\x0a\xa5\xc4\x0d\x2e\x17\x42\xd2\x29\x90\xc4\x7d\x01\x00"
				\x05\x06" "\xdc\x73\x02\x00" "\x3b\x00\x04\x00\x3d\x00\xd3\x27\x85\xc4\xd4\xb3\x25\x42\x79\x0f\xd9\xc3\xd7\x01\x00"
				\x05\x07" "\x29\x75\x02\x00" "\x09\x00\x36\x00\x3e\x00\x39\x88\x97\xc4\x51\x8b\x0f\x42\xf9\xfd\x2a\xc4\xb2\x01\x00",
*/
			// 43..01....06
			// om de 10x een ServerPos verstuurd, dan komt deze 43..
			ServerPosTimestamp[8+5+7*27] = 
				"\x43"				// type
				"\x62\x1b"			// checksum
				"\x01"				// type
				"\xb0\xf1\xa7\x00"	// host timestamp
				"\x06"				// type
				"\x07"				// aantal in deze lijst
				"\x00\x00\x00"
				"\x05\x00\xb6\xeb\x00\x00\x15\x00\xd6\x00\x00\x00\xa3\x65\xb1\x43\x9d\x3f\x5e\x42\x35\xa0\x13\xc4\x56\x00\x00"
				"\x05\x02\x97\xeb\x00\x00\xef\x01\x2d\x00\x3d\x00\xbe\xc8\x05\xc3\x70\x5d\x5c\x42\x34\x2d\x62\xc4\xb7\x00\x02"
				"\x05\x03\x24\xea\x00\x00\x00\x00\xc2\x00\x3e\x00\x85\x25\x5d\x43\xc8\x90\x42\x42\xb2\xdc\x5e\xc4\x82\x00\x00"
				"\x05\x04\x70\xe9\x00\x00\xd4\x00\xef\x00\x3e\x00\xa5\xb4\xb2\x41\x42\x5d\x35\x42\x1d\x9c\x7a\xc4\x9d\x00\x00"
				"\x05\x05\xcb\xea\x00\x00\xff\x00\xc4\x00\x3e\x00\x04\x62\x5f\x43\x0c\x1b\x46\x42\xb8\x04\x54\xc4\x7e\x00\x00"
				"\x05\x06\x86\xea\x00\x00\xcf\x00\xf4\x00\x3e\x00\x98\xdd\xf7\xbf\x64\x5f\x35\x42\x76\x40\x7d\xc4\x9f\x00\x00"
				"\x05\x07\xf8\xeb\x00\x00\xc2\x00\x00\x00\x3e\x00\x56\x76\x09\x43\xec\x9b\x37\x42\x39\xa3\x71\xc4\x90\x00\x00",

/*
// onbekend pakketje:
43 2d ab 2 45 ce 94 7b 5 1 22 5 0 0 cd 0

43			type
2d ab		checksum
 2			type
45 ce 94 7b timestamp??
 5 
 1			playernumber??
 22 5 
 0 0 cd 0
*/

			// 42 05
			PosUSA5[30] = //startpunt usa5
				"\x42"				// type
				"\x21\xaa"			// checksum
				"\x05"				// type
				"\x00"				// spelernummer(0..7)
				"\x00\x00\x00\x00"	// timestamp / racetime
				"\xf0\x00\xd1\x00""\x3f\x00"
				"\xfc\x3a\x52\x43"		// 3D X positie
				"\x4f\x5b\x82\x42"		// Y
				"\xdc\xa8\x13\xc2" 		// Z
				"\x00" "\x00"		// progress counter(s)
				"\x00",
/*
				"\x42"	//net voor split0 usa5
				"\x1b\xf4"
				"\x05\x00\xcc\x95\x00\x00\xfc\x00\xc5\x00\x3f\x00\xb5\x72\xcb\x43\x7c\x34\x56\x42\x9a\x85\x31\xc4\x63\x00\x02",
*/

			// 42 05
			clPos[30] =
				"\x42"              // type
				"\x07\xf0"
				"\x05"				// type
				"\x07\x9e\x01\x00\x00\xf0\x00\xd1\x00\x3f\x00\xed\x4e\x52\x43\x1e\x2c\x82\x42\x22\xf0\x12\xc2\x00\x00\x00",


/* unknown packet:
52 93 4c 45 0 5 6 4e 3 0 0 17 0 d8 0 3f

52 
93 4c 
45 0 
5 
6
4e 3 0 0 17 0 d8 0 3f
*/


/*
"\x42"
"\x2a\x96"
"\x03\x01\x00\x00\x00"
"\x05\x02\x0a\x02\x00\x00\xf0\x00\xd1\x00\x3f\x00\xfc\x3a\x52\x43\x4e\x5b\x82\x42\xdc\xa8\x13\xc2\x00\x00\x00",
*/				



			// C6 0C
			// de server zendt deze ook terug naar de rest..
			ServerIntermediate[21] =
				"\xc6"				// type
				"\x84\x24"			// checksum
				"\x0b\x00"			// clLine / CMD
				"\x0c"				// type
				"\x02"				// PlayerNr  0=host, etc..
				"\x00"				// split# 0=split 1,    1=split 2,   etc..   4f=finish
				"\x00"
				"\xD0\xD6\x53\x00"	// ??
				"\x01\x00\x00\x00"	// totaal tijd zover?? op 1 gezet ff..
				"\x00\x00\x00\x00",

			// C6 0B
			clFinish[21] =
				"\xc6"				// type
				"\xd9\x40"			// checksum
				"\x0e\x00"			// CMD / ChatLine
				"\x0b"				// type
				"\x01"				// PlayerNr  0=host, etc..
				"\x4f"				// constant 4F voor finish
				"\x00"
				"\xbf\xa4"			// bf a4   of   1f 9d
				"\x4f\x00"			// de eerste constant ??
				"\xec\x5e\x02\x00"	// RaceTime
				"\x00\x00\x00\x00",
			// C6 0B
			ServerFinish[21] =
				"\xc6"				// type
				"\xd9\x40"			// checksum
				"\x0e\x00"			// CMD / ChatLine
				"\x0b"				// type
				"\x01"				// PlayerNr  0=host, etc..
				"\x00"
				"\x00"
				"\x40\x93\x17\x00"	// \x40\x93\x17\x00 naar de rest als de eerste finished, \x16\x00\x00\x00 daarna.. ??
				"\xec\x5e\x02\x00"	// racetijd
				"\x00\x00\x00\x00",


			// C6 88
			ServerKickPlayer[17] =
				"\xc6"				// type
				"\xa9\x15"			// checksum
				"\x1a\x00"			// client chatline
				"\x88\x28\xb7\x01"	// type
				"\x0a\x00\x00\x00"	// client JoinNr
				"\x02\x00\x00\x00",	// constant..

/*
			// C6 8B
			ServerKickedMsg[variabel door chat] =
				"\xc6"
				"\xec\x9b"			// checksum
				"\x07\x00"			// ChatLine
				"\x8b"				// type
*/
			// CLIENT QUIT
			// C6 08
			clQuit[13] = 
				"\xc6"				// type
				"\xec\x9b"			// checksum
				"\x07\x00"			// ChatLine
				"\x08"				// type
				"\x04"				// PlayerNr
				"\xa3\x00"			// A3 of 00 ??
				"\x01\x00\x00\x00",	// lijkt constant ??
			// C6 08
			ServerQuit[13] =
				"\xc6"				// type
				"\x0b\x58"			// checksum
				"\x52\x00"			// client ChatLine
				"\x08"				// type
				"\x04"				// PlayerNr
				"\x53\x00"			// lijkt constant ??
				"\x01\x00\x00\x00",	// ..


			// C6 87
			clQuitter[13] = 
				"\xc6"				// type
				"\x00\x81"			// checksum
				"\x26\x00"
				"\x87"				// type
				"\x00\x00\x00\x01\x00\x00\x00",



			// D6 88
			ServerQuitAck[19] =
				"\xd6"				// type
				"\x09\x6f"
				"\x12\x00"			// CMD
				"\x20\x00"			// ChatLine
				"\x88"				// type
				"\x00\x00\x00"
				"\x01\x00\x00\x00"	// JoinNr overnemen
				"\x01\x00\x00\x00",	// lijkt constant te zijn..

			// DA
			clDA[7] = 
				"\xda"				// type
				"\xcf\xbb"			// checksum
				"\x60\x00"
				"\x27\x00",			// ChatLine





			// CA
			// zowel client als server stuurt dit..
			// Een client als ie retired/quit of iets dergelijks,
			// De server stuurt bij het kicken van een speler..
			Goner[5] = 
				"\xca"				// type
				"\x81\xb6"			// checksum
				"\x08\x00",			// CMD / ChatLine

			// 54
			ServerACKGoner[5] = 
				"\x54"				// type
				"\x81\x46"			// checksum
				"\x08\x00",			// client CMD / ChatLine

			// 44 6F 51
			ServerQuitAck2[3] =
				"\x44\x6F\x51",



			// C6 07
			clRetire[7] = 
				"\xc6"				// type
				"\xed\x4f"			// checksum
				"\x06\x00"			// ChatLine
				"\x07"				// type
				"\x01",				// client PlayerNr
			// C6 07
			ServerRetire[7] = 
				"\xc6"				// type
				"\xa5\xbd"			// checksum
				"\x6d\x00"			// client ChatLine
				"\x07"				// type
				"\x00",				// PlayerNr (host==0)




			// C6 10
			ServerNumberThing[61] =
				"\xc6" 
				"\xc2\xe5"		//checksum
				"\x13\x00"		//CMD / ChatLine
				"\x10"			//type
				"\x05\x00\x00"	// 05 of 04 ??
				"\x00"	// 7x een exemplaar zenden, met waarden:  0, 1, 2, 3, 4, 5, 6, 7
				"\x00\x00\x00" "\x60\xea" "\x00\x00" "\x60\xea" "\x00\x00" 
				"\x19" // 14 of 19??
				"\x00\x00\x00\x00"
				"\x00\x00\x00"
				"\x19"	// 7x een exemplaar zenden, met waarden: 19, b, b, b, c, c, c, c
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
/*
c6 67 a9 0e 00 10 
04 00 00 
00 
00 00 00 60 ea 00 00 60 ea 00 00 19 00 00 00 00				("\x00\x00\x00" "\xe0\x47" "\x00\x00" "\x40\x9c" "\x00\x00")
00 00 00 
19 
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
*/


			// C6 89
			ServerJoinNrC6[13] = 
				"\xc6"
				"\xfd\x69"				// checksum
				"\x45\x00"				// CMD / ChatLine
				"\x89\x76\x21\x1e"		// type constant/altijd hetzelfde..
				"\x02\x00\x00\x00",		// het JoinNr van de speler

			ServerJoinNrD6[15] = 
				"\xd6"
				"\x5a\x3c"				// checksum
				"\x04\x00"
				"\x3d\x00"				// CMD / ChatLine
				"\x89\x76\x21\x1e"
				"\x05\x00\x00\x00";		// het JoinNr van de speler















/* format 1
c6 9d d3 52 00 85
08 08											// maxplayers, numplayers
00

00 00 00 00										// Nth speler die joint sinds start server..
02 00 00 00										// READY
55 4a 45 20 46 61 73 74 20 46 72 61 6e 6b 00 00 // Nick UJE Fast Frank
17												// Country
00 00 00 00 00 00 00							//
02												// ??
0d												// Car
02												// CarType
02												// Gearbox

02 00 00 00										//
00 00 00 00										//
53 70 65 65 64 20 52 61 63 65 72 00 00 00 00 00	// Speed Racer
25												// usa
00 00 00 84 00 00 00							//
02												//
0d												// 205
02												// groupB
02												// manual

03 00 00 00
00 00 00 00
43 52 54 2e 44 72 69 76 65 72 33 00 00 00 00 00	// CRT.Driver3
0a
ff 13 00 a0 00 00 00
02
0d
02
02

08 00 00 00
01 00 00 00
6b 65 72 6e 6f 00 00 00 00 00 00 00 00 00 00 00	// kerno
0f
00 00 00 a9 00 00 00
02
0d
02
02

09 00 00 00
00 00 00 00
45 73 6b 69 6c 6c 65 72 00 00 00 00 00 00 00 00	// Eskiller
0e
93 17 00 54 00 00 00
02
00
00
00

0a 00 00 00
01 00 00 00
52 2e 20 52 6f 6f 73 65 20 45 53 54 00 00 00 00	// R. Roose EST
27
00 00 00 45 00 00 00
02
00
00
00

0b 00 00 00
00 00 00 00
73 6d 69 6c 65 00 00 00 00 00 00 00 00 00 00 00	// smile
00
da 4d 00 52 00 00 00
02
00
00
00

0c 00 00 00
00 00 00 00
65 6e 67 69 6e 00 00 00 00 00 00 00 00 00 00 00	// engin
24
0c 55 00 00 00 00 00
02
03
00
00




// format 2
c6 9d d3 52 00 85
08 08											// maxplayers, numplayers
00

speler Nr
sinds start  ready or not speler-naam                                      
___________  ___________  _______________________________________________  __
00 00 00 00  02 00 00 00  55 4a 45 20 46 61 73 74 20 46 72 61 6e 6b 00 00  17  00 00 00 00 00 00 00  02  0d  02  02  UJE Fast Frank
02 00 00 00  00 00 00 00  53 70 65 65 64 20 52 61 63 65 72 00 00 00 00 00  25  00 00 00 84 00 00 00  02  0d  02  02  Speed Racer
03 00 00 00  00 00 00 00  43 52 54 2e 44 72 69 76 65 72 33 00 00 00 00 00  0a  ff 13 00 a0 00 00 00  02  0d  02  02  CRT.Driver3
08 00 00 00  01 00 00 00  6b 65 72 6e 6f 00 00 00 00 00 00 00 00 00 00 00  0f  00 00 00 a9 00 00 00  02  0d  02  02  kerno
09 00 00 00  00 00 00 00  45 73 6b 69 6c 6c 65 72 00 00 00 00 00 00 00 00  0e  93 17 00 54 00 00 00  02  00  00  00  Eskiller
0a 00 00 00  01 00 00 00  52 2e 20 52 6f 6f 73 65 20 45 53 54 00 00 00 00  27  00 00 00 45 00 00 00  02  00  00  00  R. Roose EST
0b 00 00 00  00 00 00 00  73 6d 69 6c 65 00 00 00 00 00 00 00 00 00 00 00  00  da 4d 00 52 00 00 00  02  00  00  00  smile
0c 00 00 00  00 00 00 00  65 6e 67 69 6e 00 00 00 00 00 00 00 00 00 00 00  24  0c 55 00 00 00 00 00  02  03  00  00  engin

*/





/*
MINI SERVER LIST
                                     00 __clHandle_ 55  `.u0.V.1...6~..U
0030   4a 45 20 73 65 72 76 65 72 74 00 31 2e 30 00 30  JE servert.1.0.0
0040   00 31 00 6f 70 65 6e 77 61 69 74 69 6e 67 00 31  .1.openwaiting.1
0050   00 38 00                                         .8.
*/

/*
                                     00 3b 82 0e 00 68  ..u0.7..)5.;...h
0030   6f 73 74 6e 61 6d 65 00 55 4a 45 20 77 69 6e 6e  ostname.UJE winn
0040   65 72 63 68 6f 6f 73 65 00 67 61 6d 65 76 65 72  erchoose.gamever
0050   00 31 2e 30 00 68 6f 73 74 70 6f 72 74 00 33 30  .1.0.hostport.30
0060   30 30 30 00 70 61 73 73 77 6f 72 64 00 30 00 67  000.password.0.g
0070   61 6d 65 74 79 70 65 00 31 00 67 61 6d 65 6d 6f  ametype.1.gamemo
0080   64 65 00 6f 70 65 6e 77 61 69 74 69 6e 67 00 6e  de.openwaiting.n
0090   75 6d 70 6c 61 79 65 72 73 00 32 00 6d 61 78 70  umplayers.2.maxp
00a0   6c 61 79 65 72 73 00 38 00 72 61 6c 6c 79 00 30  layers.8.rally.0
00b0   00 73 74 61 67 65 73 00 31 30 30 00 64 61 6d 61  .stages.100.dama
00c0   67 65 00 30 00 72 61 6e 6b 69 6e 67 00 30 00 63  ge.0.ranking.0.c
00d0   61 72 74 79 70 65 00 30 00 00 00 00 00 00 00 00  artype.0........
00e0  
*/






/*
// client1 pingt. (echo-request)
                                     50 47 64 2b 00 01  .d.8u0....PGd+..
0030   0a 9e 0e 00                                      ............



// server antwoord op client1 ping (echo-reply)
                                     40 24 da 02 0a 9e  ..u0.8..A9@$....
0030   0e 00                                            ..


// client2 pingt. (echo-request)
                                     50 20 54 2a 00 01  .d..u0..p.P T*..
0030   53 b3 0c 00                                      S...........


// server antwoord op client2 ping (echo-reply)
                                     40 bf 8d 02 53 b3  .2u0....G#@...S.
0030   0c 00                                            ..




// server naar client1 (afterping)
                                     43 0c 00 01 db c1            C.....
0030   06 00 86 03 00 00 00 00 00 00 01 00 00 00 02 00  ................
0040   00 00 c8 da 4d 00 00 00 00 00 0c 75 51 00 00 00  ....M......uQ...
0050   00 00 f4 ea a3 00 00 00 00 00 01 00 00 00 0a 00  ................
0060   00 00 74 fe 13 00 9c 0c 55 00 20 66 5c 00 42 10  ..t.....U. f\.B.
0070   d5 77 9f 2b 46 d1                                .w.+F.

// server naar client2 (afterping)
                                     43 0c 00 01 db c1            C.....
0030   06 00 86 03 00 00 00 00 00 00 01 00 00 00 02 00  ................
0040   00 00 c8 da 4d 00 00 00 00 00 0c 75 51 00 00 00  ....M......uQ...
0050   00 00 f4 ea a3 00 00 00 00 00 01 00 00 00 0a 00  ................
0060   00 00 74 fe 13 00 9c 0c 55 00 20 66 5c 00 42 10  ..t.....U. f\.B.
0070   d5 77 9f 2b 46 d1                                .w.+F.

// later packets..
//                                     43 a4 75 01 0e ae  ..u0.8.T..C.u...
//0030   09 00 86 03 00 00 00 00 00 00 01 00 00 00 03 00  ................
//0040   00 00 c8 da 4d 00 00 00 00 00 0c 75 51 00 00 00  ....M......uQ...
//0050   00 00 f4 ea a3 00 00 00 00 00 02 00 00 00 05 00  ................
//0060   00 00 74 fe 13 00 9c 0c 55 00 20 66 5c 00 42 10  ..t.....U. f\.B.
//0070   d5 77 9f 2b 46 d1                                .w.+F.
//
//                                     43 3d 62 01 df b5  ..u0.8.TXKC=b...
//0030   09 00 86 03 00 00 00 00 00 00 01 00 00 00 03 00  ................
//0040   00 00 c8 da 4d 00 00 00 00 00 0c 75 51 00 00 00  ....M......uQ...
//0050   00 00 f4 ea a3 00 00 00 00 00 01 00 00 00 04 00  ................
//0060   00 00 74 fe 13 00 9c 0c 55 00 20 66 5c 00 42 10  ..t.....U. f\.B.
//0070   d5 77 9f 2b 46 d1                                .w.+F.




// client1 ping stap 3
                                     40 47 6b 02 db c1  .d.8u0....@Gk...
0030   06 00                                            ............


// client2 ping stap 3

                                     40 47 6b 02 db c1  .d..u0....@Gk...
0030   06 00                                            ............



//2x zenden{
// server stuurt naar client1
                                     42 20 b9 86 03 00  ..u0.8.OA.B ....
0030   00 00 00 00 00 01 00 00 00 02 00 00 00 c8 da 4d  ...............M
0040   00 00 00 00 00 0c 75 51 00 00 00 00 00 f4 ea a3  ......uQ........
0050   00 00 00 00 00 01 00 00 00 07 00 00 00 74 fe 13  .............t..
0060   00 9c 0c 55 00 20 66 5c 00 42 10 d5 77 9f 2b 46  ...U. f\.B..w.+F
0070   d1                                               .

// server stuurt naar client2
                                     42 20 b9 86 03 00  .2u0...OBjB ....
0030   00 00 00 00 00 01 00 00 00 02 00 00 00 c8 da 4d  ...............M
0040   00 00 00 00 00 0c 75 51 00 00 00 00 00 f4 ea a3  ......uQ........
0050   00 00 00 00 00 01 00 00 00 07 00 00 00 74 fe 13  .............t..
0060   00 9c 0c 55 00 20 66 5c 00 42 10 d5 77 9f 2b 46  ...U. f\.B..w.+F
0070   d1                                               .
//}




// client1 stap 4
                                     40 e3 58 01 88 a9  .d.8u0..Dp@.X...
0030   0e 00                                            ............

// server antwoord op stap 4
                                     40 eb c6 02 88 a9  ..u0.8...f@.....
0030   0e 00                                            ..




// client2 stap 4
                                     40 83 09 01 d6 be  .d..u0..HU@.....
0030   0c 00                                            ............

// server antwoord op stap 4
                                     40 83 e0 02 d6 be  .2u0....qS@.....
0030   0c 00                                            ..




// server stuurt weer wat naar client1  stap 5
                                     43 1e 7c 01 94 cd  ..u0.8.T.RC.|...
0030   06 00 86 03 00 00 00 00 00 00 01 00 00 00 02 00  ................
0040   00 00 c8 da 4d 00 00 00 00 00 0c 75 51 00 00 00  ....M......uQ...
0050   00 00 f4 ea a3 00 00 00 00 00 01 00 00 00 07 00  ................
0060   00 00 74 fe 13 00 9c 0c 55 00 20 66 5c 00 42 10  ..t.....U. f\.B.
0070   d5 77 9f 2b 46 d1                                .w.+F.

// server stuurt weer wat naar client2  stap 5
                                     43 1e 7c 01 94 cd  .2u0...T..C.|...
0030   06 00 86 03 00 00 00 00 00 00 01 00 00 00 02 00  ................
0040   00 00 c8 da 4d 00 00 00 00 00 0c 75 51 00 00 00  ....M......uQ...
0050   00 00 f4 ea a3 00 00 00 00 00 01 00 00 00 07 00  ................
0060   00 00 74 fe 13 00 9c 0c 55 00 20 66 5c 00 42 10  ..t.....U. f\.B.
0070   d5 77 9f 2b 46 d1                                .w.+F.


*/


/*

	                                 d6 f3 5a 01 00 03  .2u0...3.C..Z...
0030   00 85 08 02 00 00 00 00 00 00 00 00 00 55 4a 45  .............UJE
0040   20 46 61 73 74 20 46 72 61 6e 6b 00 00 17 00 00   Fast Frank.....
0050   00 00 00 00 00 00 00 00 00 01 00 00 00 00 00 00  ................
0060   00 55 4a 45 20 43 00 00 00 00 00 00 00 00 00 00  .UJE C..........
0070   00 17 00 00 00 00 00 00 00 03 0d 02 02 01 00 00  ................
0080   00 04 00 00 00 54 0f 00 00 e4 72 18 00 00 00 00  .....T....r.....
0090   00 a0 fd 13 00 b0 ff 13 00 58 16 f5 4f 01 00 00  .........X..O...
00a0   00 00 00 00 00 48 fd 50 00 e0 72 18 00 05 00 00  .....H.P..r.....
00b0   00 00 00 00 00 04 00 00 00 04 00 00 00 6c fd 50  .............l.P
00c0   00 02 00 00 00 00 04 00 00 00 00 00 00 e0 0d a2  ................
00d0   01 c5 00 00 00 b0 ff 13 00 88 11 f5 4f e4 72 18  ............O.r.
00e0   00 00 00 00 00 00 00 00 00 00 00 00 00 78 fe 13  .............x..
00f0   00 c3 d7 4d 00 e1 d7 4d 00 a6 da 4d 00 6c c9 d4  ...M...M...M.l..
0100   77 00 00 00 00 00 00 00 00 00 00 00 00 62 bd 9f  w............b..
0110   13 99 9e 36 00 01 00 00 00 00 00 00 00 06 75 51  ...6..........uQ
0120   00 e4 72 18 00 c8 da 4d 00 00 00 00 00 0c 75 51  ..r....M......uQ
0130   00 00 00 00 00 f4 ea a3 00 00 00 00 00 10 00 00  ................
0140   00 00 00 00 00 74 fe 13 00 9c 0c 55 00 20 66 5c  .....t.....U. f\
0150   00 42 10 d5 77                                   .B..w



ServerPlayList[299] = 
		"\xD6\x0E\x7B\x01\x00\x03"
		"\x00\x85\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x17\x00\x00\x00\x00\x00\x00\x00\x03\x0D\x02\x02\x01\x00\x00"
		"\x00\x04\x00\x00\x00\xD8\x0B\x00\x00\x44\x93\x17\x00\x00\x00\x00"
		"\x00\xA0\xFD\x13\x00\xB0\xFF\x13\x00\x58\x16\xF5\x4F\x01\x00\x00"
		"\x00\x00\x00\x00\x00\x48\xFD\x50\x00\x40\x93\x17\x00\x05\x00\x00"
		"\x00\x00\x00\x00\x00\x04\x00\x00\x00\x04\x00\x00\x00\x6C\xFD\x50"
		"\x00\x02\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\xE0\x0D\xA2"
		"\x01\xC5\x00\x00\x00\xB0\xFF\x13\x00\x88\x11\xF5\x4F\x44\x93\x17"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x78\xFE\x13"
		"\x00\xC3\xD7\x4D\x00\xE1\xD7\x4D\x00\xA6\xDA\x4D\x00\x6C\xC9\xD4"
		"\x77\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xC4\x88\x6D"
		"\x0D\x99\x9E\x36\x00\x01\x00\x00\x00\x00\x00\x00\x00\x06\x75\x51"
		"\x00\x44\x93\x17\x00\xC8\xDA\x4D\x00\x00\x00\x00\x00\x0C\x75\x51"
		"\x00\x00\x00\x00\x00\xF4\xEA\xA3\x00\x00\x00\x00\x00\x10\x00\x00"
		"\x00\x00\x00\x00\x00\x74\xFE\x13\x00\x9C\x0C\x55\x00\x20\x66\x5C"
		"\x00\x42\x10\xD5\x77",



*/

/*

0000   00 10 4b 17 2a bd 00 08 02 5a ab 1b 08 00 45 00  ..K.*....Z....E.
0010   01 45 79 82 00 00 80 11 ad be c0 a8 c8 64 c0 a8  .Ey..........d..
0020   c8 b1 75 30 04 38 01 31 1e 1b c6 e0 a9 05 00 85  ..u0.8.1........
0030   08 03 00 00 00 00 00 00 00 00 00 
55 4a 45 20 46 61 73 74 20 46 72 61 6e 6b 00 00 17 00 00 00 00 00 00 00 00 00 00 00 01 00 00 00 00 00 00 00 ...........UJE Fast Frank......................
55 4a 45 20 46 6c 79 69 6e 67 00 00 00 00 00 00 17 00 00 00 01 00 00 00 03 00 00 00 02 00 00 00 00 00 00 00   UJE Flying.......................
55 4a 45 20 43 00 00 00 00 00 00 00 00 00 00 00 17 ff 13 00 00 00 00 00 03 00 00 00 00 00 00 00 48 fd 50 00   ...UJE C........
0090    ................
00a0    ...H.P.@........
00b0   00 00 00 04 00 00 00 04 00 00 00 6c fd 50 00 01  ...........l.P..
00c0   00 00 00 00 04 00 00 00 00 00 00 e0 0d a2 01 e6  ................
00d0   00 00 00 b0 ff 13 00 88 11 f5 4f 44 93 17 00 00  ..........OD....
00e0   00 00 00 00 00 00 00 00 00 00 00 78 fe 13 00 c3  ...........x....
00f0   d7 4d 00 e1 d7 4d 00 a6 da 4d 00 6c c9 d4 77 00  .M...M...M.l..w.
0100   00 00 00 00 00 00 00 00 00 00 00 2b 0b 3c 2a 99  ...........+.<*.
0110   9e 36 00 01 00 00 00 00 00 00 00 06 75 51 00 44  .6..........uQ.D
0120   93 17 00 c8 da 4d 00 00 00 00 00 0c 75 51 00 00  .....M......uQ..
0130   00 00 00 f4 ea a3 00 00 00 00 00 10 00 00 00 00  ................
0140   00 00 00 74 fe 13 00 9c 0c 55 00 20 66 5c 00 42  ...t.....U. f\.B
0150   10 d5 77                                         ..w


*/





/*
de rest ontvangt:
c6 9d d3 03 00 85
08 08												// maxplayers, numplayers
00
8*player-record{									// +9
	00 00 00 00										// +0	1 long?? waarde Nth speler die wordt joint sinds start server
	02 00 00 00										// +4	1 long?? waarde 2..
	55 4a 45 20 46 61 73 74 20 46 72 61 6e 6b 00 00 // +8	Nick UJE Fast Frank
	17												// +24	Country
	00 00 00
	00 00 00 00										// +
	02												// +32	??
	0d												// +33	Car
	02												// +34	CarType
	02												// +35	Gearbox
}

de joiner ontvangt:
d6 xx xx   01 00   03 00 85
08 08												// maxplayers, numplayers
00
8*player-record{									// +11
	00 00 00 00										// +0	1 long?? waarde Nth speler die wordt joint sinds start server
	02 00 00 00										// +4	1 long?? waarde 2..
	55 4a 45 20 46 61 73 74 20 46 72 61 6e 6b 00 00 // +8	Nick UJE Fast Frank
	17												// +24	Country
	00 00 00
	00 00 00 00										// +28	ping?? long
	02												// +32	??
	0d												// +33	Car
	02												// +34	CarType
	02												// +35	Gearbox
}
*/


//  c6 43 91 0f 00 82 00 00 00 02 00 00 00 00 00 00 00 02 00 02 00  .d..u0.....C...................

// <QUIT>
// client zendt dit packet als eerste stap van het quit-proces..
// c6 fd cf 12 00 87 00 00 00 01 00 00 00
//
//		 server antwoordt..
//		 d6 09 6f 12 00 20 00 88 00 00 00 01 00 00 00 01 00 00 00
//
// client stuurt weer..2x
// da 06 4c 20 00 13 00
//
// client stuurt..1x
// 54 ce 23 20 00
//
// client stuurt weer..1x
// 54 97 af 21 00
//
// client stuurt 3x
// 44 6f 51
//
//		 server antwoordt..1x
//		 54 06 f6 13 00
//
//		 server stuurt..4x
//		 44 6f 51
// </QUIT>


/*

 Nadat de client heeft gezonden (50 xx xx 00 00 01 xx xx xx xx),
 begint de server met tellen van gezonden packets aan clients/spelers.
 Elk packet wordt acknowledge-d door de client dmv een (50 cc cc ACK xx) of een (50 cc cc xx xx ACK hh hh hh hh).

*/



/*

// clients aan server




// server naar iedereen..
                                     c6 d4 e3 30 00 84
0030   00 00 00 02 00 00 00 01 00 00 00 03 04 55 00 20
0040   66 5c 00 42 10 d5 77 42 10 d5 77
// 50's komen binnen van clients..
// server stuurt dan 40's
// clients een 40-pakket terug, (soms een 42 lijkt t)
// ..pings??..
// c6....0e van clients ,  daarna
// c6....0d van een paar clients     (ready??)
// server stuurt dan c6....0e naar een hele hoop
// closedplaying gamemode dan ineens...
// c6(d6)...0e volgen nog van server naar alle clients



// van server->client   + ACK's van client komen dan..
                                     c6 87 72 4a 00 0c
       02 00 00 d0 d6 53 00 92 82 00 00 00 00 00 00





  pakketje dat de server zendt naar clients
  voordat een stage/rally begint.
  Op offset +9 staat een getal dat oploopt van 0 t/m 7.

                                     c6 4f e0 3b 00 10
0030   05 00 00 07 00 00 00 e0 47 00 00 40 9c 00 00 14
0040   00 00 00 00 00 00 00 0c 00 00 00 00 00 00 00 00
0050   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0060   00 00 00 00 00 00 00


// een ander pakketje van server -> client
                                     c6 ba 88 2c 00 03
       7a ac 02 04 00 00 00


//op deze pakketjes komen ACK's.




// server -> clients
//laatste speler ready?
d6 94 17 05 00 40 00 04
//de rest..
c6 20 8f 47 00 04
// het aftellen begint (?)







client->server
                              c6 83 e0 03 00 0e
07 00 00 00 00 00 00 00 00 00 00 00 00 00 00 2f
dd 04 40 ac 1c 4a 3f 5c 8f 62 3f 00 00 00 00

                              c6 23 bd 04 00 0d 07


// na een c6....03 aan iedereen, stuurt de server
//    een 42 (71 bytes) naar 2 spelers,
// en een 43 (76 bytes) naar 5 andere spelers.



// daarna begint de server aan het zenden van C6....10's aan iedereen.
// alles achter elkaar verzenden met oplopende nummers 0..7
// en die ene andere byte (+9) op 19,b,b,b,c,c,c,c

  pakketje dat de server zendt naar clients
  voordat een stage/rally begint.
  Op offset +9 staat een getal dat oploopt van 0 t/m 7.

                                     c6 4f e0 3b 00 10
0030   05 00 00 07 00 00 00 e0 47 00 00 40 9c 00 00 14
0040   00 00 00 00 00 00 00 0c 00 00 00 00 00 00 00 00
0050   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0060   00 00 00 00 00 00 00



// daarna krijgt iedereen een C6....0E

// dan komen alle ACK's binnen van de packets die net tevoren zijn verstuurd in 1 burst..


// daarna stuurt de server een C6....89 naar de spelers
// met erachteraan meteen een playerlist (naar iedereen)

      checksum  CMD      constant       JoinNr
c6    fd 69     45 00    89 76 21 1e    02 00 00 00




// full-kick capture

	nummer		richting	pakket
	3239		S->C		C6....85 playerlist iedereen
	openwaiting nu nog
	3269		S->C		C6....84 aan iedereen
	closedplaying nu nog
	3295		S->C		(40-packets van 8 bytes) iedereen
	3308		S->C		(42-packets van 71 bytes) to any
	3332		C->S		C6....0E client zendt als eerste
	3338		S->C		C6....0E  naar 1 speler een D6....0E
	3350		S->gamespy	closeplaying 8 8   packet van 43 bytes.
	3380		C->S		C6....0E en een C6....0D naar server
	3402		S->C		C6....0E aan iedereen
	3414		S->C		C6....03 aan iedereen
	3426		S->C		C6....0E aan iedereen
	3454		C->S		C6....03 van 1 iemand ontvangen 83.69.56.1
	3461		S->C		C6....03 naar iedereen (sommigen nog een C6....0E ervoor)
	3516		S->C		C6....0E en een C6....03 naar sommigen, anderen alleen een c6-0e, anderen weer alleen een c6-03
	3629		C->S		C6....03 ontvangen van 83.202.244.104 weer
	3630		S->C		C6....03 naar 6 spelers..
	3642		C->S		C6....03 ontvangen van 83.202.244.104 weer
	3657		C->S		C6....0E en een C6....0D van 82.41.222.46
	3661		S->C		C6....0E naar wat spelers
	3678		S->C		C6....0E weer
	3698		C->S		C6....03 van 82.41.222.46
	3700		S->C		C6....03 naar 6 spelers
	3713		C->S		C6....03 van 82.41.222.46
	3718		C->S		C6....03 van 70.16.96.252
	3721		S->C		C6....03 naar 5 spelers
	3854		S->C		C6....0E naar 7 spelers.
	3945		C->S		C6....0E van 213.138.226.241
	3949		S->gamespy	closeplaying 8 8   packet van 43 bytes.
	3950-4042	S->C		C6....0E naar iedereen
	4111-4124	S->C		C6....03 naar 7 spelers/iedereen
	4178-4189	S->C		C6....03 naar 6 spelers
	4190-4203	S->C		C6....04 naar 6 spelers


	3742-4054	S->C
*/


/*
	richting	grootte	pakket..								omschrijving
	C->S		37		C6 xx xx xx xx 81 ...					Client wil joinen (packet met naam erin)

	S->C		37		C6 xx xx xx xx 80 ...					Client join bevestiging

	C->S		21		C6 xx xx xx xx 82 ...					Client kiest een andere auto instelling

	S->C		33		C6 xx xx xx xx 83 ...

	S->C		33		C6 xx xx xx xx 84 ...

	S->C		297		C6 xx xx xx xx 85 ...					Server stuurt PlayerList aan de rest
	S->C		299		D6 xx xx xx xx xx xx 85 ...				Server stuurt PlayerList aan joiner

	S->C		13		C6 xx xx xx xx 89						?? (gebeurt voordat spel begonnen is, met alle spelers zowat)
	S->C		15		D6 xx xx xx xx xx xx 89 ...				?? (gebeurt voordat spel begonnen is, met maar 1 speler)

	S->C		17		C6 xx xx xx xx 88 ...					Server kickt speler?? (na spel gebeurd)

	C->S		97		C6 xx xx xx xx 8A ...					Client chat
	S->C		var		C6 xx xx xx xx 8B 01 00 C0...			Server geeft chat door naar clients
	S->C		var		D6 xx xx xx xx xx xx 8B 01 00 C0...		Server geeft chat door naar spreker
	S->C		var		C6 xx xx xx xx 8B 01 xx xx...			Server chat zelf naar clients

	S->C		61		C6 xx xx xx xx 10 xx xx xx !! ...		!! bevat een waarde die oploopt van 0 t/m 7..voor race aanvang




	S<->C		5/10	50 xx xx xx xx(xx xx xx xx xx)			Bevestiging/Acknowledge


	Het enige C9 packet:
	S->C		5		C9 34 DC 01 00							join-bevestiging (ServerJoinReply3)


    d6 09 6f 12 00 20 00 88 00 00 00 01 00 00 00 01 00 00 00

*/


/*
	knockout mode		laatste speler vliegt eruit..
	winnerChoose		de winnaar kiest een stage
	normal				telkens een ander baan (op volgorde)
	rallies				telkens een andere rally (op volgorde)
	random				willekeurig wat (damage, cartype etc..)
	cup mode			allemaal dezelfde auto
	cup vs cup			2 verschillende autos te kiezen

*/

/*

  voor de race begint..zendt de server naar de clients (PingList)
43   17 df     01   a4 9c a6 00    86 07 00 00 00 00 00 00 02 00 00 00 03 00 00 00 04 00 00 00 05 00 00 00 07 00 00 00 08 00 00 00 7c 4b 55 00 00 00 00 00 81 00 00 00 53 00 00 00 d4 00 00 00 94 00 00 00 3c 00 00 00 ad 00 00 00 bf f1 ed c5
  


  inrace stuurt de server: (202 bytes  etc... in)
43   24 a6     01   97 21 a7 00 06 07 00 00 00 05 00 a5 1b 00 00 ea 00 d7 00 3f 00 07 89 4a 43 04 4f 82 42 41 89 56 c2 02 00 00 05 01 12 1b 00 00 e6 00 dd 00 3e 00 86 c4 49 43 89 56 82 42 e5 bd 52 c2 02 00 02 05 03 e6 19 00 00 ec 00 d7 00 00 00 fe 29 4d 43 52 1a 82 42 7c fc 3f c2 02 00 00 05 04 22 19 00 00 f3 00 d0 00 3e 00 c1 59 4f 43 36 2d 82 42 ac 2d 2e c2 01 00 00 05 05 c6 1a 00 00 e1 00 e1 00 00 00 2e 82 4a 43 92 2b 82 42 e8 d5 4c c2 02 00 00 05 06 87 1a 00 00 de 00 e5 00 3e 00 69 db 49 43 7e 2b 82 42 d2 66 4e c2 02 00 00 05 07 ca 1b 00 00 e8 00 da 00 3d 00 b2 61 49 43 45 03 82 42 99 0c 5f c2 03 00 00





clients sturen dit 43-packet als de race eenmaal bezig is:
     checksum  type                  3d-positie van een speler
43   51 3d     01    38 3f   06 00   05 01 bd 27 00 00 f3 00 32 ff 3e 00 c2 53 29 43 7f ef 80 42 7f 91 a6 c2 07 00 00




*/


/*
                                     00 e3 d8 65 01 68  .du0.....J...e.h
0030   6f 73 74 6e 61 6d 65 00 73 70 65 65 64 00 67 61  ostname.speed.ga
0040   6d 65 76 65 72 00 31 2e 30 00 68 6f 73 74 70 6f  mever.1.0.hostpo
0050   72 74 00 33 30 30 30 30 00 70 61 73 73 77 6f 72  rt.30000.passwor
0060   64 00 30 00 67 61 6d 65 74 79 70 65 00 30 00 67  d.0.gametype.0.g
0070   61 6d 65 6d 6f 64 65 00 63 6c 6f 73 65 64 70 6c  amemode.closedpl
0080   61 79 69 6e 67 00 6e 75 6d 70 6c 61 79 65 72 73  aying.numplayers
0090   00 34 00 6d 61 78 70 6c 61 79 65 72 73 00 38 00  .4.maxplayers.8.
00a0   72 61 6c 6c 79 00 31 00 73 74 61 67 65 73 00 31  rally.1.stages.1
00b0   30 32 31 34 31 31 34 32 31 30 32 31 30 34 31 30  0214114210210410
00c0   35 00 64 61 6d 61 67 65 00 30 00 72 61 6e 6b 69  5.damage.0.ranki
00d0   6e 67 00 31 00 63 61 72 74 79 70 65 00 30 00 00  ng.1.cartype.0..
00e0   00 00 00 00 00 00                                ......


                                     00 21 d9 65 01 68  .du0....<+.!.e.h
0030   6f 73 74 6e 61 6d 65 00 6c 69 74 65 6f 6e 20 20  ostname.liteon  
0040   73 75 63 6b 73 2e 2e 20 00 67 61 6d 65 76 65 72  sucks.. .gamever
0050   00 31 2e 30 00 68 6f 73 74 70 6f 72 74 00 33 30  .1.0.hostport.30
0060   30 30 30 00 70 61 73 73 77 6f 72 64 00 30 00 67  000.password.0.g
0070   61 6d 65 74 79 70 65 00 31 00 67 61 6d 65 6d 6f  ametype.1.gamemo
0080   64 65 00 63 6c 6f 73 65 64 70 6c 61 79 69 6e 67  de.closedplaying
0090   00 6e 75 6d 70 6c 61 79 65 72 73 00 38 00 6d 61  .numplayers.8.ma
00a0   78 70 6c 61 79 65 72 73 00 38 00 72 61 6c 6c 79  xplayers.8.rally
00b0   00 31 34 00 73 74 61 67 65 73 00 31 31 35 00 64  .14.stages.115.d
00c0   61 6d 61 67 65 00 30 00 72 61 6e 6b 69 6e 67 00  amage.0.ranking.
00d0   30 00 63 61 72 74 79 70 65 00 31 00 00 00 00 00  0.cartype.1.....
00e0   00 00 00                                         ...

                                     00 b4 d8 65 01 68  .du0...."....e.h
0030   6f 73 74 6e 61 6d 65 00 62 6f 62 00 67 61 6d 65  ostname.bob.game
0040   76 65 72 00 31 2e 30 00 68 6f 73 74 70 6f 72 74  ver.1.0.hostport
0050   00 33 30 30 30 30 00 70 61 73 73 77 6f 72 64 00  .30000.password.
0060   31 00 67 61 6d 65 74 79 70 65 00 31 00 67 61 6d  1.gametype.1.gam
0070   65 6d 6f 64 65 00 63 6c 6f 73 65 64 70 6c 61 79  emode.closedplay
0080   69 6e 67 00 6e 75 6d 70 6c 61 79 65 72 73 00 32  ing.numplayers.2
0090   00 6d 61 78 70 6c 61 79 65 72 73 00 34 00 72 61  .maxplayers.4.ra
00a0   6c 6c 79 00 30 00 73 74 61 67 65 73 00 31 36 30  lly.0.stages.160
00b0   00 64 61 6d 61 67 65 00 30 00 72 61 6e 6b 69 6e  .damage.0.rankin
00c0   67 00 30 00 63 61 72 74 79 70 65 00 30 00 00 00  g.0.cartype.0...
00d0   00 00 00 00 00                                   .....



                                     00 e3 53 05 08 68  .du0....q...S..h
0030   6f 73 74 6e 61 6d 65 00 55 4a 45 20 6c 69 6e 75  ostname.UJE linu
0040   78 20 63 68 61 74 00 67 61 6d 65 76 65 72 00 31  x chat.gamever.1
0050   2e 30 00 68 6f 73 74 70 6f 72 74 00 33 30 30 30  .0.hostport.3000
0060   30 00 70 61 73 73 77 6f 72 64 00 30 00 67 61 6d  0.password.0.gam
0070   65 74 79 70 65 00 30 00 67 61 6d 65 6d 6f 64 65  etype.0.gamemode
0080   00 6f 70 65 6e 77 61 69 74 69 6e 67 00 6e 75 6d  .openwaiting.num
0090   70 6c 61 79 65 72 73 00 31 00 6d 61 78 70 6c 61  players.1.maxpla
00a0   79 65 72 73 00 38 00 72 61 6c 6c 79 00 31 34 00  yers.8.rally.14.
00b0   73 74 61 67 65 73 00 31 33 34 00 64 61 6d 61 67  stages.134.damag
00c0   65 00 30 00 72 61 6e 6b 69 6e 67 00 31 00 63 61  e.0.ranking.1.ca
00d0   72 74 79 70 65 00 30 00 00 00 00 00 00 00 00     rtype.0........











Server aan gamespy (207.38.8.34:27900):
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

server krijgt terug dezelfde gamespy-server:
                                     fe fd 01 85 ce 4a  .dl.u0."}-.....J
0030   3d 34 39 36 6b 6c 4a 35 33 36 32 46 35 39 45 37  =496klJ5362F59E7
0040   35 33 30 00                                      530.

server krijgt van een andere gamespy-server (poort:30000):
                                     fe fd 02 85 ce 4a  .d.xu0.-.Y.....J
0030   3d 47 61 6d 65 53 70 79 20 46 69 72 65 77 61 6c  =GameSpy Firewal
0040   6c 20 50 72 6f 62 65 20 50 61 63 6b 65 74 2e     l Probe Packet.

server stuurt terug naar de 1e gamespy-server (master):
                                     01 85 ce 4a 3d 64  ."u0l..&r....J=d
0030   45 63 33 63 65 68 36 79 34 75 5a 79 4f 71 38 35  Ec3ceh6y4uZyOq85
0040   34 2b 38 49 30 6d 75 00                          4+8I0mu.

server stuurt naar die andere gamespy-server:
                                     05 85 ce 4a 3d 47  ..u0.x.+.[...J=G
0030   61 6d 65 53 70 79 20 46 69 72 65 77 61 6c 6c 20  ameSpy Firewall 
0040   50 72 6f 62 65 20 50 61 63 6b 65 74 2e           Probe Packet.

server krijgt van de master:
                                     fe fd 01 85 ce 4a  .dl.u0."}-.....J
0030   3d 34 39 36 6b 6c 4a 35 33 36 32 46 35 39 45 37  =496klJ5362F59E7
0040   35 33 30 00                                      530.

server stuurt naar de 1e master:
                                     01 85 ce 4a 3d 64  ."u0l..&r....J=d
0030   45 63 33 63 65 68 36 79 34 75 5a 79 4f 71 38 35  Ec3ceh6y4uZyOq85
0040   34 2b 38 49 30 6d 75 00                          4+8I0mu.



#4092	s->gamespy?
08 85 ce 4a 3d









c->s: //retire/quit
c6 
2a 94 
0b 00 
08 
05 00 00 01 00 00 00 00 00 00 00 00

c->s: // plus een CA..
ca 
78 de 
0c 00

s->c: (de rest , niet de speler die quit):
c6 
81 7b 
6a 00 
08 
05 53 00 01 00 00 00

s->c die quit:
54 
6e 9e 
0b 00

s->c die quit: plus nog 1
54 
b8 08 
0c 00

s->c die quit: plus 3x een..
44 6f 51

s->c (de rest   niet de speler die quit)
cb 
1c 77 
6b 00 
00 e8 03 00 
00 e8 03 00

s->c (de rest   niet de speler die quit)
een playerlist











intermediates:
#11070	83.69.56.1->s		c6 84 24 0b 00 0c  02 00 00  80 7c 41 00  92 82 00 00 00 00 00 00
#11702	84.248.22.96->s		c6 df 5e 06 00 0c  07 00 00  90 7c 41 00  14 8f 00 00 00 00 00 00
#11984	70.16.96.252->s		c6 e9 79 0a 00 0c  01 00 00  80 7c 41 00  96 93 00 00 00 00 00 00
#12013	83.202.244.104->s	c6 b8 58 06 00 0c  06 00 00  90 7c 41 00  01 93 00 00 00 00 00 00
#12329	213.138.226.241->s	c6 7c 62 06 00 0c  04 00 00  90 7c 41 00  90 96 00 00 00 00 00 00
#13542	81.214.35.47->s		c6 30 ed 08 00 0c  03 00 00  90 7c 41 00  63 af 00 00 00 00 00 00
#13643	82.41.222.46->s		c6 56 a7 06 00 0c  05 00 00  90 7c 41 00  b0 b1 00 00 00 00 00 00
					

#17839	83.69.56.1->s		c6 bc b6 0c 00 0c  02 01 00  80 7c 41 00  5d 05 01 00 00 00 00 00
#19095	70.16.96.252->s		c6 be aa 0b 00 0c  01 01 00  80 7c 41 00  3a 1d 01 00 00 00 00 00
#19274	83.202.244.104->s	c6 de 22 07 00 0c  06 01 00  90 7c 41 00  48 20 01 00 00 00 00 00
#19851	84.248.22.96->s		c6 f0 4f 07 00 0c  07 01 00  90 7c 41 00  eb 2e 01 00 00 00 00 00
#21504	81.214.35.47->s		c6 7b 84 09 00 0c  03 01 00  90 7c 41 00  39 52 01 00 00 00 00 00
#22163	82.41.222.46->s		c6 57 22 07 00 0c  05 01 00  90 7c 41 00  26 61 01 00 00 00 00 00
					

#23798	83.69.56.1->s		c6 6c 9f 0d 00 0c  02 02 00  80 7c 41 00  34 87 01 00 00 00 00 00
#25184	70.16.96.252->s		c6 67 f0 0c 00 0c  01 02 00  80 7c 41 00  e0 a5 01 00 00 00 00 00
#25391	83.202.244.104->s	c6 48 62 08 00 0c  06 02 00  90 7c 41 00  97 a9 01 00 00 00 00 00
#27358	84.248.22.96->s		c6 e0 1c 08 00 0c  07 02 00  90 7c 41 00  81 d7 01 00 00 00 00 00
#30576	82.41.222.46->s		c6 f3 3d 08 00 0c  05 02 00  90 7c 41 00  99 26 02 00 00 00 00 00


#31041	83.69.56.1->s		c6 e5 59 0e 00 0c  02 03 00  80 7c 41 00  ae 33 02 00 00 00 00 00
#32480	70.16.96.252->s		c6 94 ce 0d 00 0c  01 03 00  80 7c 41 00  ec 5e 02 00 00 00 00 00
#32827	83.202.244.104->s	c6 b5 de 09 00 0c  06 03 00  90 7c 41 00  43 6a 02 00 00 00 00 00
#34246	84.248.22.96->s		c6 05 20 09 00 0c  07 03 00  90 7c 41 00  5e ae 02 00 00 00 00 00
#35555	82.41.222.46->s		c6 d7 5b 09 00 0c  05 03 00  90 7c 41 00  0c 01 03 00 00 00 00 00



70.16.96.252 ontvangt van server na  split 0 van 83.69.56.1:
							c6 87 72 4a 00 0c  02 00 00  d0 d6 53 00  92 82 00 00 00 00 00 00
na split1:					c6 64 20 51 00 0c  02 01 00  d0 d6 53 00  5d 05 01 00 00 00 00 00
na split2:					c6 81 6e 59 00 0c  02 02 00  d0 d6 53 00  34 87 01 00 00 00 00 00
na split3/finish:			c6 f8 34 61 00 0c  02 03 00  d0 d6 53 00  ae 33 02 00 00 00 00 00





83.69.56.1 over split3/finish aan server:
							c6 8e b5 0f 00 0b  02 4f 00  bf a4 4f 00  ae 33 02 00 00 00 00 00
70.16.96.252 ontvangt van server na  split3/finish van 83.69.56.1:
							c6 70 a4 62 00 0b  02 00 00  40 93 17 00  ae 33 02 00 00 00 00 00











// na het C6-10 circus, komen er C6-0E's en C6-0D's van clients:
#3945	213.138.226.241->s
c6 60 c4 03 00 0e   04   02 02 7c f2 64 00 f0 8a a9 00 00 00 00 00   2f dd 04 40 ac 1c 4a 3f 5c 8f 62 3f 00 00 00 00
c6 3e 54 04 00 0d   04

#3950	s->6clients (behalve 213.138.226.241)
c6 2e 6c 44 00 0e   04   00 00 20 00 00 00 f0 fb 13 00 50 d8 53 00   2f dd 04 40 ac 1c 4a 3f 5c 8f 62 3f 00 00 00 00

...

#4004	84.248.22.96->s
60 30 99 19 00
#4005	s-> terug
c6 0a 80 19 00 10 05 00 00 06 00 00 00 e0 47 00 00 40 9c 00 00 14 00 00 00 00 00 00 00 0c 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00                             .......
c6 4c 8d 1b 00 0e   00   00 00 0c 00 00 00 00 00 00 00 00 00 00 00   75 93 f0 3f 42 60 35 3f e1 7a 64 3f 00 00 00 00
#4011	s->terug naar anderen:
c6 01 0f 22 00 0e   00   00 00 0c 00 00 00 00 00 00 00 00 00 00 00   75 93 f0 3f 42 60 35 3f e1 7a 64 3f 00 00 00 00
c6 47 9e 23 00 0e   04   00 00 20 00 00 00 f0 fb 13 00 50 d8 53 00   2f dd 04 40 ac 1c 4a 3f 5c 8f 62 3f 00 00 00 00
c6 8a ba 24 00 89 76 21 1e 08 00 00 00
playerlist
















// C6-81  /  D6-81
nicknames van clients:
#60		84.99.226.145	alain
 c6 38 23       01 00 81 00 00 00   01 00 00 00   61 6c 61 69 6e 00 00 00 00 00 00 00 00 00 00 00   0f 02 00 00 00 ea d3 77

#111	70.16.96.252	Speed Racer
 d6 3f dd 02 00 01 00 81 00 00 00   02 00 00 00   53 70 65 65 64 20 52 61 63 65 72 00 00 00 00 00   25 02 0d 02 02 00 00 00                                               .

#149	83.69.56.1		CRT.Driver3
 c6 9d 64       01 00 81 00 00 00   03 00 00 00   43 52 54 2e 44 72 69 76 65 72 33 00 00 00 00 00   0a 02 07 01 02 91 d3 77

#231	81.214.35.47	engin
 c6 74 f3       01 00 81 00 00 00   04 00 00 00   65 6e 67 69 6e 00 00 00 00 00 00 00 00 00 00 00   24 02 03 00 00 00 00 00

#424	213.138.226.241	Impulsivo
 c6 0b eb       01 00 81 00 00 00   05 00 00 00   49 6d 70 75 6c 73 69 76 6f 00 00 00 00 00 00 00   1b 02 00 00 00 00 00 00

#1723	84.99.226.145	alain
 d6 dd c9 02 00 01 00 81 00 00 00   06 00 00 00   61 6c 61 69 6e 00 00 00 00 00 00 00 00 00 00 00   0f 02 00 00 00 00 00 00

#1775	82.41.222.46	KEV
 c6 a6 77       01 00 81 00 00 00   07 00 00 00   4b 45 56 00 00 00 00 00 00 00 00 00 00 00 00 00   00 02 00 00 00 00 00 00

#2159	83.202.244.104	kerno
 c6 10 80       01 00 81 00 00 00   08 00 00 00   6b 65 72 6e 6f 00 00 00 00 00 00 00 00 00 00 00   0f 02 00 00 02 00 00 00

#2836	84.248.22.96	Eskiller
 c6 38 18       01 00 81 00 00 00   09 00 00 00   45 73 6b 69 6c 6c 65 72 00 00 00 00 00 00 00 00   0e 02 00 00 00 00 00 00










//serverinfo's:
c->s:
fe fd 00 5f ad 06 00 ff ff ff

s->c:
                                     00 5f ad 06 00 68  ..u0.z....._...h
0030   6f 73 74 6e 61 6d 65 00 55 4a 45 20 73 65 72 76  ostname.UJE serv
0040   65 72 74 00 67 61 6d 65 76 65 72 00 31 2e 30 00  ert.gamever.1.0.
0050   68 6f 73 74 70 6f 72 74 00 33 30 30 30 30 00 70  hostport.30000.p
0060   61 73 73 77 6f 72 64 00 30 00 67 61 6d 65 74 79  assword.0.gamety
0070   70 65 00 31 00 67 61 6d 65 6d 6f 64 65 00 6f 70  pe.1.gamemode.op
0080   65 6e 77 61 69 74 69 6e 67 00 6e 75 6d 70 6c 61  enwaiting.numpla
0090   79 65 72 73 00 35 00 6d 61 78 70 6c 61 79 65 72  yers.5.maxplayer
00a0   73 00 38 00 72 61 6c 6c 79 00 31 34 00 73 74 61  s.8.rally.14.sta
00b0   67 65 73 00 31 33 34 00 64 61 6d 61 67 65 00 30  ges.134.damage.0
00c0   00 72 61 6e 6b 69 6e 67 00 30 00 63 61 72 74 79  .ranking.0.carty
00d0   70 65 00 30 00 00 00 00 00 00 00 00              pe.0........



c->s:
fe fd 00 29 ca 06 00 07 01 03 13 06 0b 08 0a 00 00

s->c:
                                     00 29 ca 06 00 55  ..u0...1...)...U
0030   4a 45 20 73 65 72 76 65 72 74 00 31 2e 30 00 30  JE servert.1.0.0
0040   00 31 00 6f 70 65 6e 77 61 69 74 69 6e 67 00 35  .1.openwaiting.5
0050   00 38 00                                         .8.



c->s:
fe fd 00 45 ce 06 00 ff ff ff

s->c:
                                     00 45 ce 06 00 68  ..u0.......E...h
0030   6f 73 74 6e 61 6d 65 00 55 4a 45 20 73 65 72 76  ostname.UJE serv
0040   65 72 74 00 67 61 6d 65 76 65 72 00 31 2e 30 00  ert.gamever.1.0.
0050   68 6f 73 74 70 6f 72 74 00 33 30 30 30 30 00 70  hostport.30000.p
0060   61 73 73 77 6f 72 64 00 30 00 67 61 6d 65 74 79  assword.0.gamety
0070   70 65 00 31 00 67 61 6d 65 6d 6f 64 65 00 6f 70  pe.1.gamemode.op
0080   65 6e 77 61 69 74 69 6e 67 00 6e 75 6d 70 6c 61  enwaiting.numpla
0090   79 65 72 73 00 35 00 6d 61 78 70 6c 61 79 65 72  yers.5.maxplayer
00a0   73 00 38 00 72 61 6c 6c 79 00 31 34 00 73 74 61  s.8.rally.14.sta
00b0   67 65 73 00 31 33 34 00 64 61 6d 61 67 65 00 30  ges.134.damage.0
00c0   00 72 61 6e 6b 69 6e 67 00 30 00 63 61 72 74 79  .ranking.0.carty
00d0   70 65 00 30 00 00 00 00 00 00 00 00              pe.0........

*/












/*
server kickt spelers: 
#37422		c6   75 e7   0c 00   88 28 b7 01   0c 00 00 00   02 00 00 00

#37511		c6   23 64   12 00   88 28 b7 01   0b 00 00 00   02 00 00 00

#37574		c6   a9 15   1a 00   88 28 b7 01   0a 00 00 00   02 00 00 00

#37638		c6   5c 12   64 00   88 28 b7 01   09 00 00 00   02 00 00 00

#37685		c6   27 bb   6e 00   88 28 b7 01   08 00 00 00   02 00 00 00

#37714		c6   e9 ee   8f 00   88 28 b7 01   03 00 00 00   02 00 00 00

#37748		c6   9a 8e   95 00   88 28 b7 01   02 00 00 00   02 00 00 00
*/












/*
.. .. tt tt tt tt XX XX XX XX XX XX ,, ,, ,, ,, ,, ,, ,, ,, ,, ,, ,, ,, ,, ,, ,,
.. .. RaceTime___ 11    22    33

XX11 schijnt altijd 1 hoger te zijn in het antwoord packet naar andere clients..



#4297		van SpeedRacer:
42 5e e0 
05 01 77 01 00 00 f0 00 d1 00 40 00 fc 3a 52 43 3e 5b 82 42 dc a8 13 c2 00 00 02


#4298		van client:
42 0d 53 
05 05 e5 01 00 00 f0 00 d1 00 3f 00 ed 4e 52 43 2f 2c 82 42 22 f0 12 c2 00 00 00


#4299		van andere client:
42 07 f0 
05 07 9e 01 00 00 f0 00 d1 00 3f 00 ed 4e 52 43 1e 2c 82 42 22 f0 12 c2 00 00 00




#4300		naar clients:
                                     42 ce 72 06 07 00  `.u0.[....B.r...
0030   00 00 05 00 25 02 00 00 f0 00 d1 00 3f 00 fc 3a  ....%.......?..:
0040   52 43 4f 5b 82 42 dc a8 13 c2 00 00 00 05 02 de  RCO[.B..........
0050   00 00 00 f1 00 d2 00 3e 00 fc 3a 52 43 3d 5b 82  .......>..:RC=[.
0060   42 dc a8 13 c2 00 00 02 05 03 ea 00 00 00 f1 00  B...............
0070   d2 00 00 00 b0 48 52 43 5b 19 82 42 f2 29 13 c2  .....HRC[..B.)..
0080   00 00 00 05 04 15 01 00 00 f1 00 d2 00 3e 00 ed  .............>..
0090   4e 52 43 1e 2c 82 42 22 f0 12 c2 00 00 00 

05 05 e5 01 00 00 f1 00 d2 00 3e 00 ed 4e 52 43 2f 2c 82 42 22 f0 12 c2 00 00 00 

                                  05 06 20 01 00 00 f1  .B"........ ....
00c0   00 d2 00 40 00 ed 4e 52 43 1e 2c 82 42 22 f0 12  ...@..NRC.,.B"..
00d0   c2 00 00 00 

05 07 9e 01 00 00 f1 00 d2 00 3e 00 ed 4e 52 43 1e 2c 82 42 22 f0 12 c2 00 00 00






*/
