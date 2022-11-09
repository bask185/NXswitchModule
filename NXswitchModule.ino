#include "src/io.h"
#include "src/date.h"
#include "src/version.h"
#include "src/macros.h"
#include "src/debounceClass.h"
#include "src/XpressNetMaster.h"

#ifndef DEBUG
XpressNetMasterClass Xnet ;
#endif

const int nPointsPerStreet = 20 ;

const int C = 0x8000 ;          // CURVED
const int S = 0x0000 ;          // STRAIGHT
const int lastPoint = 10000 ;   // must be in the end of every line in table to flag that the last point is set.
const int points[][nPointsPerStreet+3] =
{
//  1e knop 2e    wissels + standen        vlag laatste wissel
    { 0,    1 , 1|C,    2|S,    3|C,    4|S, lastPoint }, 
    { 1,    0 , 1|S,    2|C,    3|S,    4|C, lastPoint }, 
    { 0,    2 , 5|C,    7|S,    8|C,    8|S, lastPoint },
    { 0,    3 , 1|C,    2|S,    3|C,    4|S, lastPoint },
    { 0,    4 , 1|C,    2|S,    3|C,    4|S, lastPoint },
    { 0,    5 , 1|C,    2|S,    3|C,    4|S, lastPoint },
    { 2,    3 , 1|C,    2|S,    3|C,    4|S, lastPoint },
    { 2,    4 , 1|C,    2|S,    3|C,    4|S, lastPoint },
    { 2,    5 , 1|C,    2|S,    3|C,    4|S, lastPoint },
    { 2,    6 , 1|C,    2|S,    3|C,    4|S, lastPoint },
    { 2,    7 , 1|C,    2|S,    3|C,    4|S, lastPoint },
} ;

const int nStreets = sizeof( points ) / sizeof( points[0][0] ) / nPointsPerStreet - 1 ; // calculate amount of streets, depending on the size of the above table

const int leds[][2] =
{ // address |  Pin
    {  1,        3 },
    {  4,        5 },
    {  3,        2 },
    {  3,        2 },
    {  3,        2 },
} ;

const int nLeds = sizeof( leds ) / sizeof(leds[0][0] ) / 2 ; // test me

const int nButtons = 16 ;

enum states 
{
    getFirstButton,
    getSecondButton,
    getIndex,
    setPoints,
} ;

uint8 state = getFirstButton ;

uint8 firstButton, secondButton ;
uint8 index = 0 ;
uint8 street ;

Debounce button[nButtons] =
{
    Debounce(3) ,
    Debounce(4) ,
    Debounce(5) ,
    Debounce(6) ,
    Debounce(7) ,
    Debounce(8) ,
    Debounce(9) ,
    Debounce(10) ,
    Debounce(11) ,
    Debounce(12) ,
    Debounce(A0) ,
    Debounce(A1) ,
    Debounce(A2) ,
    Debounce(A3) ,
    Debounce(A4) ,
    Debounce(A5) ,
} ;





void setup()
{
    initIO() ;

#ifdef DEBUG
    Serial.begin( 115200 ) ;
    Serial.println("hello world") ;
    printNumberln("nStreets: ", nStreets ) ;
    printNumberln("nleds: ", nLeds ) ;
    
    notifyXNetTrnt( 0 , 1 | 0b1000 ) ;
    notifyXNetTrnt( 3 , 1 | 0b1000 ) ;
    delay(2000);
    notifyXNetTrnt( 0 , 0 | 0b1000 ) ;
    notifyXNetTrnt( 3 , 0 | 0b1000 ) ;
#else
    Xnet.setup( Loco128, 2 ) ;
#endif
}




void readSwitches()
{
    REPEAT_MS( 20 )
    {
        for( int i = 0 ; i < nButtons ; i ++ )
        {
            button[i].debounce() ;
        }
    }
    END_REPEAT
}

void runNx()
{
    uint16 point ;
    uint16 address ;
    uint8  pointState ;

    for( int i = 0 ; i < nButtons ; i ++ )
    {
        uint8 btnState = button[i].getState() ;

        switch( state )
        {
        case getFirstButton:
            if( btnState == FALLING )
            {
                firstButton = i ;
                printNumberln("first button: ",    firstButton ) ;
                state = getSecondButton ;
            }
            break ;

        case getSecondButton:
            if( btnState == RISING  )
            { 
                state = getFirstButton ;
                #ifdef DEBUG 
                Serial.println("first is released again") ; // first button is released before second is pressed
                #endif
            }
            if( btnState == FALLING )
            {
                secondButton = i ;
                printNumberln("second button: ",    secondButton ) ;
                state = getIndex ;
            }
            break ;

        case getIndex:                          // 2 buttons are found, go find out which street matches with these buttons 
            for( int i = 0 ; i < nStreets ; i ++ )
            {
                if( points[i][0] == firstButton
                &&  points[i][1] == secondButton )
                {
                    street = i ;
                    index = 2 ;                 // reset index before setting new street
                    printNumberln(" index found: ", street);
                    state = setPoints ;
                    goto indexFound ;
                }
            }

            state = getFirstButton ;
            #ifdef DEBUG 
            Serial.println("no index found, invalid combination");
            #endif
        
        indexFound:
            break ;

        case setPoints:
            REPEAT_MS( 250 )
            {
                point = points[street][index++] ;
                if( point == lastPoint )
                {
                    #ifdef DEBUG 
                    Serial.println("all points set\r\n") ;
                    #endif
                    state = getFirstButton ;
                    break ;
                }
                else
                {
                    address = point & 0x03FF ;
                    pointState = point >> 15 ;  
                    printNumber_("setting point: ", address ) ;
                #ifdef DEBUG 
                    if( pointState ) Serial.println("curved") ;
                    else             Serial.println("straight") ;
                #else
                    
                #endif
                }
            }
            END_REPEAT
            break ;
        }
    }
} 

void notifyXNetTrnt(uint16_t Address, uint8_t data)
{
    if( bitRead( data , 3 ) == 1 )
    {
        Address ++ ;
        data &= 0b11 ;
        if( data >= 2) data -= 2 ;   // convers 1-2 into 0-1
        
        for( int i = 0 ; i < nLeds ; i ++ )  // match switched addresses to our led addresses
        {
            if( Address == leds[i][0] )
            {
                digitalWrite( leds[i][1], data ) ; // todo add shift register support
            #ifdef DEBUG
                printNumber_("received address#", Address ) ;
                printNumber_("setting led: ", leds[i][1] ) ;
                printNumberln("state: ", data ) ;
            #endif
            }
        }
    }
}


void loop()
{
    readSwitches() ;
    runNx() ;

#ifndef DEBUG
    Xnet.update() ;
#endif
}