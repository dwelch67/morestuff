
void PUT32 ( unsigned int, unsigned int );
void dummy ( unsigned int );

int notmain ( void )
{
    unsigned int ra;

    for(ra=0;ra<10;ra++) dummy(ra);
    PUT32(0xD0000000,0x12345678);
    return(0);
}
