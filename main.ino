#include <Arduino.h>
#include "CRC32.h"
#include "chipapi.h"
//#include "rom.h"

void setup( void ) {
	Serial.begin( 9600 );

	while ( ! Serial ); // wait for serial system to come up.
	 
}

struct ParallelChipAPI* Programmer = &ShiftyFlashyAPI;
struct ChipDescription ROM = {
	.ChipSize = 8192,
	// FIXME
	// No easy nanosecond level timing
	.Timing = {
		.ClockFreq = 4000000,			// 4MHz (test)
		.WriteCycleTime = 10,			// 1ms
		.WritePulseTime = 1,			// 1us/1000ns
		.OutputEnableTime = 2,			// 1us/100ns
		.OutputDisableTime = 2,			// 1us/60ns
		.AddressToDataValidTime = 2,	// 1us/250ns
	}
};

void Erase( void ) {
	uint8_t Result = 0;
	bool Pass = true;
	uint16_t i = 0;

	for ( i = 0; i < ROM.ChipSize && Pass == true; i++ ) {
		if ( ( i & 0x003F ) == 0 ) {
			Serial.print( "." );
		}

		Pass = ( ( Result = Programmer->Write( i, 0xFF ) ) == 0xFF );
	}

	Serial.println( );

	if ( Pass == false ) {
		Serial.println( "*** FATAL ***" );
		Serial.print( "Failed to erase location 0x" );
		Serial.println( i, 16 );
		Serial.println( Result, 16 );

		while ( true ) {
		}
	}
}

void Write( const uint8_t* Buffer, int Length ) {
	uint8_t Data = 0;
	bool Pass = true;
	int i = 0;

	Serial.print( "Writing..." );

	for ( i = 0; i < Length && i < ROM.ChipSize && Pass == true; i++ ) {
		if ( ( i % 64 ) == 0 ) {
			Serial.print( "." );
		}

		Pass = ( ( Data = Programmer->Write( i, Buffer[ i ] ) ) == Buffer[ i ] );
	}
  Serial.println( "\n" );
	if ( Pass == false ) {
		Serial.println( "\n*** FATAL ***" );
		Serial.print( "Failed to write location 0x" );
		Serial.println( i, 16 );

		while ( true ) {
		}
	}

	Serial.println( "Done." );
}

void Dump( uint32_t Start, uint32_t End ) {
	char Buffer[ 256 ];

	snprintf( Buffer, sizeof( Buffer ), "Dump from 0x%04X to 0x%04X:\n", Start, End );
	Serial.print( Buffer );

	for ( ; Start <= End; Start++ ) {
		if ( ( Start % 16 ) == 0 ) {
			snprintf( Buffer, sizeof( Buffer ), "\n%04X: ", Start );
			Serial.print( Buffer );
		}

		snprintf( Buffer, sizeof( Buffer ), "%02X ", Programmer->Read( Start ) );
		Serial.print( Buffer );
	}

	Serial.println( );
}

void loop( void ) {
	CRC32 Result;
	int i = 0;
  uint8_t v;

	Programmer->Open( );
	Programmer->SetChip( &ROM );

#if 0
	Erase( );
	//Write( __64kBASIC31A_bin, __64kBASIC31A_bin_len );
#else
	Result.reset( );
	
	for ( i = 0; i < ROM.ChipSize; i++ ) {
		//Serial.write( Programmer->Read( i ) );
    v = Programmer->Read( i );
		
    if ((i & 0x000F) == 0){
      Serial.print( "\r\n" );
      Serial.print( i, HEX );
      Serial.print( ": " );
    }
    
    Result.update( v );
    Serial.print( v,16 );
    Serial.print( " " );
    
   
    /*if ((i & 0x00FF)==0) {
       Serial.print( "\r" );
       Serial.print( i,16 );       
    }*/
  }
	Serial.print( "\n" );

	Serial.print( "ROM CRC32: 0x" );
	Serial.println( Result.finalize( ), 16 );
#endif

	//Dump( 0, 64 );

	while ( true ) {
	}
}
