// Copyright (c) 2021 Tara Keeling
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <Arduino.h>
//#include <SPI.h>
#include "chipapi.h"

static bool ShiftyFlashy_Open( void );
static void ShiftyFlashy_Close( void );
static uint8_t ShiftyFlashy_Read( uint16_t Address );
static uint8_t ShiftyFlashy_Write( uint16_t Address, uint8_t Data );
static void InitOutputWithLevel( int Pin, int Level );
static void SetDataBus_Input( void );
static void SetDataBus_Output( void );
static void PinChangeWithDelay( int Pin, int Level, uint32_t DelayUS );
static void SetAddress( uint16_t Address );
static void ShiftyFlashy_SetChip( struct ChipDescription* ChipInfo );
static uint8_t GetDataBus( void );
static void SetDataBus( const uint8_t Data );

//static SPIClass SPI_5V( PB15, PB14, PB13 );

// 3.3V Logic pins should still register as logic HIGH
// on the 5v powered ICs.
static const int Pin_Latch = PA0;
static const int Pin_OE    = PA1;
static const int Pin_WE    = PA2;
static const int Pin_CLK   = PB13;
static const int Pin_D     = PB14;

static uint32_t LatchDelay = 100; // Actually should be ~20ns

struct ChipDescription* Chip = NULL;

uint16_t CurrentAddress = 0x0000;

#define NSecsPerClock floor( 1.0 / ( double ) F_CPU )

// These MUST be on 5v tolerant IOs!!!
static const int DataBusPins[ 8 ] = {
    PA8,    // D0
    PA9,    // D1
    PA10,   // D2
    PA15,   // D3
    PB10,    // D4
    PB4,    // D5
    PB6,    // D6
    PB7     // D7
};

struct ParallelChipAPI ShiftyFlashyAPI = {
    ShiftyFlashy_Open,
    ShiftyFlashy_Close,
    ShiftyFlashy_SetChip,
    ShiftyFlashy_Read,
    ShiftyFlashy_Write
};

static void ShiftyFlashy_SetChip( struct ChipDescription* ChipInfo ) {
    Chip = ChipInfo;
}

static bool ShiftyFlashy_Open( void ) {
   // SPI_5V.begin( );
  //  SPI_5V.setClockDivider( 16 );

    InitOutputWithLevel( Pin_Latch, LOW );
    InitOutputWithLevel( Pin_OE,  HIGH );
    InitOutputWithLevel( Pin_WE,  HIGH );
    InitOutputWithLevel( Pin_D,   HIGH );
    InitOutputWithLevel( Pin_CLK, HIGH );

    SetAddress( CurrentAddress );
    SetDataBus_Input( );

    return true;
}

static void ShiftyFlashy_Close( void ) {
   // SPI_5V.end( );
    SetDataBus_Input( );
}

uint8_t ShiftyFlashy_Read( uint16_t Address ) {
    uint8_t Result = 0;

   // noInterrupts( );
    SetDataBus_Input( );
    
    SetAddress( ( uint16_t ) Address );
    
    digitalWrite( Pin_OE, LOW );  // PinChangeWithDelay( Pin_OE, LOW, Chip->Timing.OutputEnableTime );

   // interrupts( );
    delay(1);
    Result = GetDataBus( );

    digitalWrite( Pin_OE, HIGH ); // PinChangeWithDelay( Pin_OE, HIGH, Chip->Timing.OutputDisableTime );

    return Result;
}

uint8_t GetDataBus( void ) {
    uint8_t Result = 0;

    Result = (  
      (digitalRead( PB7 )  << 7) |
      (digitalRead( PB6 )  << 6) |
      (digitalRead( PB4 )  << 5) |
      (digitalRead( PB10 )  << 4) |
      (digitalRead( PA15 ) << 3) |
      (digitalRead( PA10 ) << 2) |
      (digitalRead( PA9 )  << 1) |
      (digitalRead( PA8 )  << 0) 
     );
     /*
     Serial.print( digitalRead( PB7 ) );
     Serial.print( digitalRead( PB6 ) );
     Serial.print( digitalRead( PB4 ) );
     Serial.print( digitalRead( PB10 ) );
     Serial.print( digitalRead( PA15 ) );
     Serial.print( digitalRead( PA10 ) );
     Serial.print( digitalRead( PA9 ) );
     Serial.print( digitalRead( PA8 ) );
     Serial.print( "\r\n" );
     */

    return Result;
}

static void SetDataBus( const uint8_t Data ) {
    digitalWrite( DataBusPins[ 7 ], ( Data & 0x80 ) ? HIGH : LOW );
    digitalWrite( DataBusPins[ 6 ], ( Data & 0x40 ) ? HIGH : LOW );
    digitalWrite( DataBusPins[ 5 ], ( Data & 0x20 ) ? HIGH : LOW );
    digitalWrite( DataBusPins[ 4 ], ( Data & 0x10 ) ? HIGH : LOW );
    digitalWrite( DataBusPins[ 3 ], ( Data & 0x08 ) ? HIGH : LOW );
    digitalWrite( DataBusPins[ 2 ], ( Data & 0x04 ) ? HIGH : LOW );
    digitalWrite( DataBusPins[ 1 ], ( Data & 0x02 ) ? HIGH : LOW );
    digitalWrite( DataBusPins[ 0 ], ( Data & 0x01 ) ? HIGH : LOW );
}

static uint8_t ShiftyFlashy_Write( uint16_t Address, uint8_t Data ) {
    int Timeout = 100;
    uint8_t Result = 0;
    uint8_t Temp = 0;

    // Disable chip outputs
    PinChangeWithDelay( Pin_OE, HIGH, Chip->Timing.OutputDisableTime );

    // Setup data and address lines
    SetAddress( ( uint16_t ) Address );

    noInterrupts( );
        SetDataBus_Output( );
        SetDataBus( Data );

        // Toggle write pulse
        PinChangeWithDelay( Pin_WE, LOW, Chip->Timing.WritePulseTime );
        PinChangeWithDelay( Pin_WE, HIGH, Chip->Timing.WriteCycleTime );

        SetDataBus_Input( );

        do {
            PinChangeWithDelay( Pin_OE, LOW, Chip->Timing.OutputEnableTime );
                Temp = GetDataBus( );
            PinChangeWithDelay( Pin_OE, HIGH, Chip->Timing.OutputDisableTime );

            PinChangeWithDelay( Pin_OE, LOW, Chip->Timing.OutputEnableTime );
                Result = GetDataBus( );
            PinChangeWithDelay( Pin_OE, HIGH, Chip->Timing.OutputDisableTime );

            delayMicroseconds( 1 );
        } while ( Temp != Result != Data && Timeout-- );
    interrupts( );

    return Result;
}

static void InitOutputWithLevel( int Pin, int Level ) {
    digitalWrite( Pin, Level );
    pinMode( Pin, OUTPUT );
}

static void SetDataBus_Input( void ) {
    int i = 0;

    for ( i = 0; i < 8; i++ ) {
        pinMode( DataBusPins[ i ], INPUT );
    }
}

static void SetDataBus_Output( void ) {
    int i = 0;

    for ( i = 0; i < 8; i++ ) {
        InitOutputWithLevel( DataBusPins[ i ], LOW );
    }
}

static void PinChangeWithDelay( int Pin, int Level, uint32_t DelayUS ) {
    digitalWrite( Pin, Level );

    if ( DelayUS ) {
        delayMicroseconds( DelayUS );
    }
}

static void SetAddress( uint16_t Address ) {

  //  if ( CurrentAddress != Address ) {
        shiftOut(PB14, PB13, MSBFIRST, ( Address >> 8 ) & 0xFF );
        shiftOut(PB14, PB13, MSBFIRST,  Address & 0xFF );
        PinChangeWithDelay( Pin_Latch, HIGH, LatchDelay );
        PinChangeWithDelay( Pin_Latch, LOW, LatchDelay );
    //    CurrentAddress = Address;
  //  }
    
}
