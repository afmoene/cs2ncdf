/* $Id$ */

#define START_OUTPUT   1
#define TWO_BYTE       2
#define FOUR_BYTE_1    3
#define FOUR_BYTE_2    4
#define DUMMY_WORD     5

#define MASK_START_OUTPUT 0xFC
#define MASK_1_OF_FOUR 0x3C
#define MASK_3_OF_FOUR 0xFC
#define MASK_1_OF_TWO 0x1C
#define MASK_DUMMY_WORD 0xFF
#define PATT_START_OUTPUT 0xFC
#define PATT_1_OF_FOUR 0x1C
#define PATT_3_OF_FOUR 0x3C
#define PATT_1_OF_TWO 0x1C
#define PATT_DUMMY_WORD 0x7F

#define MASK_ARRAY_ID 0x03

#define MASK_2_SIGN 0x80
#define MASK_2_DECIMAL 0x60
#define MASK_2_DECIMAL1 0x00
#define MASK_2_DECIMAL2 0x20
#define MASK_2_DECIMAL3 0x40
#define MASK_2_DECIMAL4 0x60

#define MASK_4_SIGN 0x40
#define PATT_4_SIGN 0x40
#define MASK_4_DECIMAL 0x83
#define PATT_4_DECIMAL1 0x00
#define PATT_4_DECIMAL2 0x80
#define PATT_4_DECIMAL3 0x01
#define PATT_4_DECIMAL4 0x81
#define PATT_4_DECIMAL5 0x02
#define PATT_4_DECIMAL6 0x82

#define NO_VALUE -9991
#define MAX_BYTES 10000


/* ........................................................................
 * Routine:   byte2bin
 * Purpose:   to give a binary representation of an unsigned char
 *            (i.e. one byte)
 * ........................................................................
 */
long byte2bin(unsigned char c) {
    char bit[8];
    long result;
    int  i;
    
    for (i=7; i>=0; i--) {
       bit[i] = (c / (unsigned) pow(2,i));
       c-=bit[i]*((unsigned) pow(2,i));
    }
    result=0;
    for (i=0; i<8; i++) {
       result+= bit[i]*pow(10,i);
    }
    return (result);
}

/* ........................................................................
 * Routine:   bytetype
 * Purpose:   To determine the type of byte pair this is
 * ........................................................................
 */
unsigned char bytetype(unsigned char byte[2]) {

    // printf("%i %i %i %i %i\n", byte[0], byte[1], byte[0]&MASK_1_OF_FOUR, PATT_1_OF_FOUR, byte[0]&MASK_3_OF_FOUR,PATT_3_OF_FOUR);
    if ((byte[0] & (unsigned char) MASK_1_OF_TWO) != PATT_1_OF_TWO) {
        return ((unsigned char) TWO_BYTE);
    } else if ((byte[0] & (unsigned char) MASK_START_OUTPUT) == (unsigned char) PATT_START_OUTPUT) {
        return ((unsigned char) START_OUTPUT);
    } else if ((byte[0] & (unsigned char) MASK_1_OF_FOUR) == (unsigned char) PATT_1_OF_FOUR) {
        return ((unsigned char) FOUR_BYTE_1);
    } else if ((byte[0] & (unsigned char) MASK_3_OF_FOUR) == (unsigned char) PATT_3_OF_FOUR) {
        return ((unsigned char) FOUR_BYTE_2);
    } else if ((byte[0] & (unsigned char) MASK_DUMMY_WORD) == (unsigned char) PATT_DUMMY_WORD) {
        return ((unsigned char) DUMMY_WORD);
    } else {
        return (unsigned char) 0;
    }
}

/* ........................................................................
 * Routine:   conv_two_byte
 * Purpose:   To derive the value of a two-byte low resolution byte pair.
 * ........................................................................
 */
float conv_two_byte(unsigned char byte[2]) {

    float sign, decimal;
    unsigned char dec_byte;
    unsigned int base;

    if ((byte[0] & MASK_2_SIGN) == MASK_2_SIGN)
        sign = -1;
    else 
        sign = 1;

    dec_byte = byte[0] & MASK_2_DECIMAL;
    switch (dec_byte) {
        case MASK_2_DECIMAL1:
           decimal = 1.0;
           break;
        case MASK_2_DECIMAL2:
           decimal = 0.1;
           break;
        case MASK_2_DECIMAL3:
           decimal = 0.01;
           break;
        case MASK_2_DECIMAL4:
           decimal = 0.001;
           break;
    }

    base = (byte[0] & 0x1F)*256+byte[1];

    return (base*sign*decimal);
}

/* ........................................................................
 * Routine:   conv_four_byte
 * Purpose:   To derive the value of a four-byte high resolution byte set 
 * ........................................................................
 */
float conv_four_byte(unsigned char byte1[2], unsigned char byte2[2]) {
    float sign, decimal;
    unsigned char dec_byte;
    unsigned int base;

    if ((byte1[0] & (unsigned char) MASK_4_SIGN) == PATT_4_SIGN)
        sign = -1;
    else 
        sign = 1;
    dec_byte = byte1[0] & MASK_4_DECIMAL;
    switch (dec_byte) {
        case PATT_4_DECIMAL1:
           decimal = 1.0;
           break;
        case PATT_4_DECIMAL2:
           decimal = 0.1;
           break;
        case PATT_4_DECIMAL3:
           decimal = 0.01;
           break;
        case PATT_4_DECIMAL4:
           decimal = 0.001;
           break;
        case PATT_4_DECIMAL5:
           decimal = 0.0001;
           break;
        case PATT_4_DECIMAL6:
           decimal = 0.00001;
           break;
        default:
           break;
    }
    base = (byte2[0] & 0x01)*65536 +
            byte1[1]*256+
            byte2[1];

    return (base*sign*decimal);
}

/* ........................................................................
 * Routine:   conv_arrayid
 * Purpose:   To get the array ID from a byte pair
 * ........................................................................
 */
unsigned int conv_arrayid(unsigned char byte[2]) {
    return ((unsigned int) (byte[0] & MASK_ARRAY_ID)*256 + byte[1]);
}


/* ........................................................................
 * Routine:   conv_hour_min
 * Purpose:   To convert from CSI hour minutes to decimal hours
 * ........................................................................
 */
float conv_hour_min(float value){
   return (floor(value/100)+(value/100-floor(value/100))/0.6);
}

