#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "netcdf.h"

#include "in_out.h"
#include "error.h"

#define START_OUTPUT   1
#define TWO_BYTE       2
#define FOUR_BYTE_1    3
#define FOUR_BYTE_2    4

#define MASK_START_OUTPUT 0xFC
#define MASK_1_OF_FOUR 0x1B
#define MASK_3_OF_FOUR 0x3B
#define MASK_1_OF_TWO 0x1C

#define MASK_ARRAY_ID 0x03

#define MASK_2_SIGN 0x80
#define MASK_2_DECIMAL 0x60
#define MASK_2_DECIMAL1 0x00
#define MASK_2_DECIMAL2 0x20
#define MASK_2_DECIMAL3 0x40
#define MASK_2_DECIMAL4 0x60

#define MASK_4_SIGN 0x40
#define MASK_4_DECIMAL 0x83
#define MASK_4_DECIMAL1 0x00
#define MASK_4_DECIMAL2 0x80
#define MASK_4_DECIMAL3 0x01
#define MASK_4_DECIMAL4 0x81
#define MASK_4_DECIMAL5 0x02
#define MASK_4_DECIMAL6 0x82

#define MAXCOL 100

/* ........................................................................
 *
 * Program  : csi2ncdf
 * Purpose  : To convert Campbell binary format (final storage) to
 *            netcdf.
 * Interface: The name of input files and the output file  are passed as
 *            commandline arguments; the format of the input file 
 *            (including the names etc. of the data) is read from
 *            a seprate text file, of which the name is also
 *            given on the command line.
 * Usage    : cwcsi2ncdf -i Inputfile -o Outputfile -f Formatfile
 *
 *            Inputfile     : name (including path) of inputfile
 *            Outputfile    : name (including path) of outputfile
 *            Formatfile    : name (including path) of format file
 *
 * Method   : 
 * Remark   : 
 * Author   : Arnold Moene
 *            Department of Meteorlogy, Wageningen Agricultural University
 *
 * Date     : June 1999
 * $Id$
 * ........................................................................
 */

/* ........................................................................
 * type declarations
 * ........................................................................
 */
typedef struct {
  int  array_id;
  char *name;
  char *unit;
  int nc_var;
  int nc_type;
  int nc_dim;
} column_def;

/* ........................................................................
 * function protypes
 * ........................................................................
 */
void  
     do_conv_csi(FILE*, int, FILE*);
void
     def_nc_file(int, FILE*, column_def *, int *);
void
     nc_handle_error(int);
void
     info(boolean usage);        /* displays info about the program */

/* ........................................................................
 * global variable declarations
 * ........................................................................
 */
char
    program[100];                   /* name of program from command line */


/* ........................................................................
 * main
 * ........................................................................
 */
void main(int argc, char *argv[])
{
    /*
     * variable declarations
     */
    char
        infname[255]="",                /* name of input file */
        outfname[255]="",               /* name of output file */
        formatfname[255]="",            /* name of output file */
        mess[100];                      /* message passed to error */
    FILE
       *infile,                         /* input file */
       *formfile,                       /* format file */
       *outfile;                        /* output_file */
    boolean
        only_usage = TRUE;              /* switch for info */
    int
	status,
        ncid;
    /* ....................................................................
     */

    /* (1) Determine disk file name of program */
    strcpy(program,(argv[0]));
    printf("Program      : %s", program);
    printf("\n");


    /* (2) Parse command line */
    argv++;
    argc--;

    /* Cycle until all arguments read */
    for ( ; argc > 0; argv++, --argc ) {
        /* variable declaration */
        char c, arg[100];

        /* Check for leading '-' in flag */
        if ((*argv)[0] == '-') {
            c = (*(argv[0] + 1));
            switch (c) {
               /* Input file */
               case 'o' :
                 cmd_arg(&argv, &argc, 2, outfname);
                 printf("Output file  : %s \n", outfname);
                 break;

               /* Output file */
               case 'i' :
                 cmd_arg(&argv, &argc, 2, infname);
                 printf("Input file   : %s \n", infname);
                 break;

               /* Format file */
               case 'f' :
                 cmd_arg(&argv, &argc, 2, formatfname);
                 printf("Input file   : %s \n", formatfname);
                 break;

               /* Show help */
               case 'h' :
		 info(FALSE);
                 break;

               /* Invalid flag */
               default :
                  cmd_arg(&argv, &argc, 1, arg);
                  printf("Invalid flag : %s\n", arg);
		  info(TRUE);
                  break;
	    }
	} else {
            strcpy(arg, argv[0]);
            printf("Invalid flag : %s\n", arg);
	    info(TRUE);
	}
    }

    /* (3) Trap error situations */
    only_usage = TRUE;
    if (!strlen(outfname)) {
        info(only_usage);
        error("no output file specified\n", CMD_LINE_ERROR);
    }
    if (!strlen(infname)) {
        info(only_usage);
        error("no input file specified\n", CMD_LINE_ERROR);
    }
    if (!strlen(formatfname)) {
        info(only_usage);
        error("no format file specified\n", CMD_LINE_ERROR);
    }

    /* (4) Open files and test for success */
    /* (4.1) Output file */
    status = nc_create(outfname, NC_CLOBBER, &ncid);
    if (status != NC_NOERR) 
       nc_handle_error(status);

    /* (4.2) Input file */
    if ((infile = fopen(infname, "rb")) == NULL) {
       sprintf(mess, "cannot open file %s for reading.\n", infname);
       error(mess, (int) FILE_NOT_FOUND);
    }

    /* (4.3) Format file */
    /*
    if ((formfile = fopen(formatfname, "rb")) == NULL) {
       sprintf(mess, "cannot open file %s for reading.\n", formatfname);
       error(mess, (int) FILE_NOT_FOUND);
    }
    */

    /* (5) Do conversion */
    do_conv_csi(infile, ncid, formfile);

    /* (6) Close files */
    fclose(infile);
    fclose(formfile);
    status = nc_close(ncid);
    if (status != NC_NOERR)
        nc_handle_error(status);
}


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
       /*
       printf("%i %u %u \n",i,bit[i], c);
       */
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
    if ((byte[0] & (unsigned char) MASK_START_OUTPUT) == MASK_START_OUTPUT) {
        return ((unsigned char) START_OUTPUT);
    } else if ((byte[0] & (unsigned char) MASK_1_OF_FOUR) == MASK_1_OF_FOUR) {
        return ((unsigned char) FOUR_BYTE_1);
    } else if ((byte[0] & (unsigned char) MASK_3_OF_FOUR) == MASK_3_OF_FOUR) {
        return ((unsigned char) FOUR_BYTE_2);
    } else if ((byte[0] & (unsigned char) MASK_1_OF_TWO) != MASK_3_OF_FOUR) {
        return ((unsigned char) TWO_BYTE);
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

    if ((byte1[0] & (unsigned char) MASK_4_SIGN) == MASK_4_SIGN)
        sign = -1;
    else 
        sign = 1;
    dec_byte = byte1[0] & MASK_4_DECIMAL;
    switch (dec_byte) {
        case MASK_4_DECIMAL1:
	   decimal = 1.0;
	   break;
        case MASK_4_DECIMAL2:
	   decimal = 0.1;
	   break;
        case MASK_4_DECIMAL3:
	   decimal = 0.01;
	   break;
        case MASK_4_DECIMAL4:
	   decimal = 0.001;
	   break;
        case MASK_4_DECIMAL5:
	   decimal = 0.0001;
	   break;
        case MASK_4_DECIMAL6:
	   decimal = 0.0001;
	   break;
	default:
	   break;
    }

    base = (byte2[1] & 0x10)*65536+
           (byte1[1] & 0xFF)*256+
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
 * Function : do_conv_csi
 * Purpose  : To do conversion of Campbell binary file to netCDF file.
 *
 * Interface: infile           in   input files
 *            ncid             out  netcdf id of output file
 *            formfile         in   text file describing format
 *
 * Method   : 
 * Date     : June 1999
 * ........................................................................
 */
void do_conv_csi(FILE *infile, int ncid, FILE *formfile)
{
    /*
     * variable declarations
     *
     */
     unsigned  char
            somebyte1[2],    /* a set of four bytes */
            somebyte2[2];    /* a set of four bytes */
     float value;
     int   array_id, doy_id, time_id, temp_id;
     int   linenum, var_id, colnum, status, index, numcoldef;
     column_def
           coldef[MAXCOL];
    /* ....................................................................
     */
    def_nc_file(ncid, formfile, coldef, &numcoldef);

    linenum=0;
    colnum=0;
    while (!feof(infile)) {
       if (fread(somebyte1, sizeof(somebyte1[0]), 2, infile) == 2) {
	/*
         printf("%08d %08d\n", byte2bin(somebyte1[0]), 
	                       byte2bin(somebyte1[1]));
        */
         switch (bytetype(somebyte1)) {
            case TWO_BYTE:
	       value = conv_two_byte(somebyte1);
	       printf("%f ", value);
	       colnum++;
	       break;
            case FOUR_BYTE_1:
               if (fread(somebyte2, sizeof(somebyte2[0]), 2, infile) == 2) {
		  if (bytetype(somebyte2) == FOUR_BYTE_2) 
                    value =  conv_four_byte(somebyte1, somebyte2);
                  else
		    error("unexpected byte pair in file",-1);
	       }
	       printf("%f ", value);
	       colnum++;
	       break;
            case START_OUTPUT:
	       array_id =  conv_arrayid(somebyte1);
	       printf("\n %i ", array_id);
	       linenum++;
	       colnum=1;
               break;
	    default:
	       error("unknown byte pair",-1);
	       break;
	 }
	 switch (colnum) {
            case 2:
	       var_id = coldef[0].nc_var;
	       break;
            case 3:
	       var_id = coldef[1].nc_var;
	       break;
            case 4:
	       var_id = coldef[2].nc_var;
	       break;
         }
	 index=linenum-1;
	 if (colnum > 1) {
         status=nc_put_var1_float(ncid, var_id, 
			 &index, &value);
         if (status != NC_NOERR)
                nc_handle_error(status);
	 }
       } 
    }
}

/* ........................................................................
 * Function : def_nc_file
 * Purpose  : To read definition of the netcdf file
 *            from the format file and defined the netcdf file 
 *
 * Interface: ncid             in  netcdf id of output file
 *            formfile         in   text file describing format
 *
 * Method   : 
 * Date     : June 1999
 * ........................................................................
 */
void def_nc_file(int ncid, FILE *formfile, 
		 column_def *coldef, int *numcoldef) {
int 
    status,
    time_dim_id, i;


    /* (2) Define time dimension */
    status = nc_def_dim(ncid, "time",
		    NC_UNLIMITED, &time_dim_id);
    if (status != NC_NOERR) 
	nc_handle_error(status);

    /* (3) Define variables */
    (*(coldef+0)).name = (char *)malloc(strlen("doy")*sizeof(char));
    (*(coldef+0)).name = "doy";
    (*(coldef+0)).unit = "";
    (*(coldef+0)).nc_type = NC_FLOAT;
    (*(coldef+0)).nc_dim = time_dim_id;
    
    (*(coldef+1)).name = (char *)malloc(strlen("time")*sizeof(char));
    (*(coldef+1)).name = "time";
    (*(coldef+1)).unit = "";
    (*(coldef+1)).nc_type = NC_FLOAT;
    (*(coldef+1)).nc_dim = time_dim_id;

    (*(coldef+2)).name = (char *)malloc(strlen("rel_R_PT100")*sizeof(char));
    (*(coldef+2)).name = "rel_R_PT100";
    (*(coldef+2)).unit = "";
    (*(coldef+2)).nc_type = NC_FLOAT;
    (*(coldef+2)).nc_dim = time_dim_id;

    for (i=0; i<3; i++) {
      status = nc_def_var(ncid, (*(coldef+i)).name, 
		                (*(coldef+1)).nc_type, 1,
		                &(*(coldef+i)).nc_dim, 
				&(*(coldef+i)).nc_var) ;
      if (status != NC_NOERR) 
	  nc_handle_error(status);
    }

    status = nc_enddef(ncid);
    if (status != NC_NOERR) 
	nc_handle_error(status);
}


void nc_handle_error(int nc_error) {

char mess[255];

   if (nc_error != NC_NOERR) {
      sprintf(mess, nc_strerror(nc_error));
      error(mess, nc_error);
   }
}

/* ........................................................................
 * Function : info
 * Purpose  : To display info about program.
 *
 * Interface: usage   in   display only "usage info" (if TRUE) or all info
 *
 * Method   : If only usage info required, name of program and parameters
 *            are printed. If all info is needed, description of parameters
 *            and program is given.
 * Date     : December 1992
 * ........................................................................
 */
void info(boolean usage)
{
    /* Give info about usage of program */
        printf("Usage: average -iInputfile -iInputfile [-iInputfile].. -oOutput [?]\n\n");

    /* If not only usage info requested, give more info */
        if (!usage)
        {
        printf("    Inputfile     : name (including path) of inputfile\n");
        printf("    Outputfile    : name (including path) of outputfile\n");
        printf("    ?             : displays usage (not in combination with\n");
        printf("                    other parameters)\n\n");
        }
}
