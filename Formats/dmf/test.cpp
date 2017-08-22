#include <stdio.h>
#include "unpack.cpp"

void main ( void )
{
    unsigned char Source[2] = { 0x4D, 0x05 };
    unsigned char Dest[2];
    
    if ( Unpack8 ( Source, Dest, 2, 2 ) )
    {
        printf ( "Failed.\n" );
    } else {
        printf ( "%d, %d\n", Dest[0], Dest[1] );
    }
}
