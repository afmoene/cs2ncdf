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

#define NO_VALUE -9991

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
  int  col_num;
  char *name;
  char *long_name;
  float scale_factor;
  float add_offset;
  float valid_min;
  float valid_max;
  float missing_value;
  char *unit;
  int nc_var;
  int nc_type;
  int nc_dim;
} column_def;

typedef struct {
  char *title;
  char *history;
} global_def;

/* ........................................................................
 * function protypes
 * ........................................................................
 */
void  
     do_conv_csi(FILE*, int, FILE*, int);
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
        formatfname[255]="",            /* name of format file */
        dumstring[255]="",              /* dummy string */
        mess[100];                      /* message passed to error */
    FILE
       *infile,                         /* input file */
       *formfile,                       /* format file */
       *outfile;                        /* output_file */
    boolean
        only_usage = TRUE;              /* switch for info */
    int
	status,
        list_line = -1,
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

               /* List number of lines */
               case 'l' :
                 cmd_arg(&argv, &argc, 2, dumstring);
		 list_line = atoi(dumstring);
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
    if (list_line < 0 && !strlen(outfname)) {
        info(only_usage);
        error("no output file specified\n", CMD_LINE_ERROR);
    }
    if (!strlen(infname)) {
        info(only_usage);
        error("no input file specified\n", CMD_LINE_ERROR);
    }
    if (list_line < 0 && !strlen(formatfname)) {
        info(only_usage);
        error("no format file specified\n", CMD_LINE_ERROR);
    }

    /* (4) Open files and test for success */
    /* (4.1) Output file */
    if (list_line < 0) {
      status = nc_create(outfname, NC_CLOBBER, &ncid);
      if (status != NC_NOERR) 
         nc_handle_error(status);
    }

    /* (4.2) Input file */
    if ((infile = fopen(infname, "rb")) == NULL) {
       sprintf(mess, "cannot open file %s for reading.\n", infname);
       error(mess, (int) FILE_NOT_FOUND);
    }

    /* (4.3) Format file */
    if (list_line < 0 && (formfile = fopen(formatfname, "rb")) == NULL) {
       sprintf(mess, "cannot open file %s for reading.\n", formatfname);
       error(mess, (int) FILE_NOT_FOUND);
    }

    /* (5) Do conversion */
    do_conv_csi(infile, ncid, formfile, list_line);

    /* (6) Close files */
    fclose(infile);
    if (list_line < 0) {
      fclose(formfile);
      status = nc_close(ncid);
      if (status != NC_NOERR)
          nc_handle_error(status);
     }
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
 *            list_line        in   number of lines of input file to
 *                                  list
 *
 * Method   : 
 * Date     : June 1999
 * ........................................................................
 */
void do_conv_csi(FILE *infile, int ncid, FILE *formfile, int list_line)
{
    /*
     * variable declarations
     *
     */
     unsigned  char
            somebyte1[2],    /* a set of four bytes */
            somebyte2[2];    /* a set of four bytes */
     float value;
     size_t index;
     int   array_id, doy_id, time_id, temp_id, i;
     int   linenum, var_id, colnum, status, numcoldef;
     column_def
           coldef[MAXCOL];
    /* ....................................................................
     */
    if (list_line < 0) 
      def_nc_file(ncid, formfile, coldef, &numcoldef);

    linenum=0;
    colnum=0;
    array_id = 0;
    while ((list_line < 0 && !feof(infile)) ||
	   (linenum <= list_line && !feof(infile))) {
       if (fread(somebyte1, sizeof(somebyte1[0]), 2, infile) == 2) {
	/*
         printf("%08d %08d\n", byte2bin(somebyte1[0]), 
	                       byte2bin(somebyte1[1]));
        */
         switch (bytetype(somebyte1)) {
            case TWO_BYTE:
	       value = conv_two_byte(somebyte1);
	       if (list_line > -1 && linenum <= list_line)
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
	       if (list_line > -1 && linenum <= list_line) 
		        printf("%f ", value);
	       colnum++;
	       break;
            case START_OUTPUT:
	       array_id =  conv_arrayid(somebyte1);
	       linenum++;
	       colnum=1;
	       if (list_line > -1 && linenum <= list_line) 
		       printf("\n %i ", array_id);
               break;
	    default:
	       error("unknown byte pair",-1);
	       break;
	 }

	 if (list_line < 0) {
	   for (i=0; i< numcoldef; i++) 
              if (coldef[i].array_id == array_id &&
	        coldef[i].col_num == colnum) { 
                var_id = coldef[i].nc_var;
	        index = linenum-1;
                status=nc_put_var1_float(ncid, var_id, 
	                     		 &index, &value);
                if (status != NC_NOERR)
                   nc_handle_error(status);
              }
         }
       } 
    }
}

/* Get a sting from between quotes */
char *quoted_string(char *string) {
char *stringp, *stringp2, *outstring;
           if ((stringp = strchr(string, '"')) && 
	       (stringp2 = strchr(stringp+1,'"'))) {
              outstring = (char *) malloc((stringp2-stringp-2)*sizeof(char));
              *outstring='\0';
              strncat(outstring, stringp+1, 
	             (stringp2-stringp)/sizeof(char)-1);
	      return outstring;
           } else {
	      return NULL;
	   }
}


int scan_line(FILE *formfile, int *id, int *col, 
              char *name, char *unit, char *long_name,
	      float *scale_factor, float *add_offset,
	      float *valid_min, float *valid_max,
	      float *missing_value,
	      global_def *globaldef) {

char line[255], linecopy[255], *stringp, dumstring[255], *stringp2;
int  pos, status;
boolean 
     id_found = FALSE,
     col_found = FALSE,
     name_found = FALSE,
     unit_found = FALSE,
     long_name_found = FALSE,
     scale_factor_found = FALSE,
     add_offset_found = FALSE,
     valid_min_found = FALSE,
     valid_max_found = FALSE,
     missing_value_found = FALSE;

    /* (1) Get one line of text */
    if (!fgets(line, sizeof(line), formfile)) return 0;

    /* (2) Strip any comments */
    pos = strcspn(line, "//");
    strncpy(linecopy, line, pos);
    if (!linecopy) return 0;

    /* (3) Get info from line */
    /* (3.1) Array ID */
    if (stringp = strstr(linecopy, "id"))
        if (stringp = strstr(stringp, "=")) {
           status = sscanf(stringp, "=%i", id);
           if (status != 1) 
	      error("could not convert array ID", -1);
	   else
              id_found = TRUE;
        } else {
	      error("could not find = sign for array ID", -1);
        }

    /* (3.2) Column number */
    if (stringp = strstr(linecopy, "col"))
        if (stringp = strstr(stringp, "=")) {
           status = sscanf(stringp, "=%i", col);
           if (status != 1) 
	      error("could not convert column number ", -1);
	   else
              col_found = TRUE;
        } else {
	      error("could not find = sign for column number", -1);
        }

    /* (3.3) Name */
    if (stringp = strstr(linecopy, "name"))
        if (stringp = strstr(stringp, "=")) {
           if (!strcpy(name, quoted_string(stringp))) 
	      error("could not convert name", -1);
	   else
              name_found = TRUE;
        } else {
	      error("could not find = sign for name", -1);
        }

    /* (3.4) Units attribute */
    if (stringp = strstr(linecopy, "units"))
        if (stringp = strstr(stringp, "=")) {
           if (!strcpy(unit, quoted_string(stringp))) 
	      error("could not convert units", -1);
	   else
              unit_found = TRUE;
        } else {
	      error("could not find = sign for units", -1);
        }

    /* (3.5)  Long name attribute */
    if (stringp = strstr(linecopy, "long_name"))
        if (stringp = strstr(stringp, "=")) {
           if (!strcpy(long_name, quoted_string(stringp))) 
	      error("could not convert long_name ", -1);
	   else 
              long_name_found = TRUE;
        } else {
	      error("could not find = sign for long_name", -1);
        }

    /* (3.6)  Scale factor attribute */
    if (stringp = strstr(linecopy, "scale_factor"))
        if (stringp = strstr(stringp, "=")) {
           status = sscanf(stringp, "= %s", dumstring);
	   *scale_factor = atof(dumstring);
           if (status != 1) 
	      error("could not convert scale_factor", -1);
	   else
              scale_factor_found = TRUE;
        } else {
	      error("could not find = sign for scale_factor", -1);
        }

    /* (3.7)  Offset attribute */
    if (stringp = strstr(linecopy, "add_offset"))
        if (stringp = strstr(stringp, "=")) {
           status = sscanf(stringp, "= %s", dumstring);
	   *add_offset = atof(dumstring);
           if (status != 1) 
	      error("could not convert offset", -1);
	   else
              scale_factor_found = TRUE;
        } else {
	      error("could not find = sign for offset", -1);
        }

    /* (3.8)  Valid_min attribute */
    if (stringp = strstr(linecopy, "valid_min"))
        if (stringp = strstr(stringp, "=")) {
           status = sscanf(stringp, "= %s", dumstring);
	   *valid_min = atof(dumstring);
           if (status != 1) 
	      error("could not convert valid_min", -1);
	   else
              valid_min_found = TRUE;
        } else {
	      error("could not find = sign for valid_min", -1);
        }

    /* (3.9)  Valid_max attribute */
    if (stringp = strstr(linecopy, "valid_max"))
        if (stringp = strstr(stringp, "=")) {
           status = sscanf(stringp, "= %s", dumstring);
	   *valid_max = atof(dumstring);
           if (status != 1) 
	      error("could not convert valid_max", -1);
	   else
              valid_max_found = TRUE;
        } else {
	      error("could not find = sign for valid_max", -1);
        }

    /* (3.10)  Missing_value attribute */
    if (stringp = strstr(linecopy, "missing_value"))
        if (stringp = strstr(stringp, "=")) {
           status = sscanf(stringp, "= %s", dumstring);
	   *missing_value = atof(dumstring);
           if (status != 1) 
	      error("could not convert missing_value", -1);
	   else
              missing_value_found = TRUE;
        } else {
	      error("could not find = sign for missing_value", -1);
        }

    /* (4)  Global attributes */
    /* (4.1)  Title attribute */
    if (stringp = strstr(linecopy, "title"))
        if (stringp = strstr(stringp, "=")) {
           (*globaldef).title =  
		   (char *) malloc(strlen(quoted_string(stringp)));
           strcpy((*globaldef).title, quoted_string(stringp));
	   return 0;
        } else {
	      error("could not find = sign for title", -1);
        }

    /* (4.2)  History attribute */
    if (stringp = strstr(linecopy, "history"))
        if (stringp = strstr(stringp, "=")) {
           (*globaldef).history =  
		   (char *) malloc(strlen(quoted_string(stringp)));
           strcpy((*globaldef).history, quoted_string(stringp));
	   return 0;
        } else {
	      error("could not find = sign for title", -1);
        }

     if (id_found || col_found || name_found || unit_found) {
        if (id_found && col_found && name_found && unit_found)
           return 1;
        else
           printf("%u %u %u %u\n", id_found, col_found, name_found, unit_found);
           error("incomplete line in format file", -1);
           return 0;
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
    time_dim_id, i, arrayid, column;
char
    name[255], unit[255], long_name[255], title[255], history[255];
float
    scale_factor, add_offset, valid_min, valid_max, missing_value;
size_t
    pos;
global_def
    globaldef;


    /* (2) Define time dimension */
    status = nc_def_dim(ncid, "time",
		    NC_UNLIMITED, &time_dim_id);
    if (status != NC_NOERR) 
	nc_handle_error(status);

    /* (3) Define variables */
    *numcoldef = 0;
    while (!feof(formfile)) {
       *long_name = '\0';
       scale_factor = NO_VALUE;
       add_offset = NO_VALUE;
       valid_min = NO_VALUE;
       valid_max = NO_VALUE;
       missing_value = NO_VALUE;
       if (scan_line(formfile, &arrayid, &column, name, unit, 
	             long_name, &scale_factor, &add_offset,
		     &valid_min, &valid_max, &missing_value,
		     &globaldef)) {
	  /* Define array ID */
          (*(coldef+*numcoldef)).array_id = arrayid;
	  /* Define column number */
          (*(coldef+*numcoldef)).col_num = column;
	  /* Define name */
          (*(coldef+*numcoldef)).name = 
	    (char *)malloc(strlen(name)*sizeof(char));
          strcpy((*(coldef+*numcoldef)).name, name);
	  /* Define units */
          (*(coldef+*numcoldef)).unit = 
	    (char *)malloc(strlen(unit)*sizeof(char));
          strcpy((*(coldef+*numcoldef)).unit, unit);
	  /* Define netcdf type */
          (*(coldef+*numcoldef)).nc_type = NC_FLOAT;
	  /* Define dimension type */
          (*(coldef+*numcoldef)).nc_dim = time_dim_id;

	  /* Long name */
          (*(coldef+*numcoldef)).long_name =
               (char *)malloc(strlen(long_name)*sizeof(char));
          strcpy((*(coldef+*numcoldef)).long_name, long_name);

	  /* Scale factor */
          (*(coldef+*numcoldef)).scale_factor = scale_factor;
	  /* Offset */
          (*(coldef+*numcoldef)).add_offset = add_offset;
	  /* Valid minimum value */
          (*(coldef+*numcoldef)).valid_min = valid_min;
	  /* Valid maximum value */
          (*(coldef+*numcoldef)).valid_max = valid_max;
	  /* Missing value */
          (*(coldef+*numcoldef)).missing_value = missing_value;

	  (*numcoldef)++;
        }
    }
    

    for (i=0; i<*numcoldef; i++) {
      status = nc_def_var(ncid, (*(coldef+i)).name, 
		                (*(coldef+i)).nc_type, 1,
		                &(*(coldef+i)).nc_dim, 
				&(*(coldef+i)).nc_var) ;
      if (status != NC_NOERR) 
	  nc_handle_error(status);

      status = nc_put_att_text(ncid, (*(coldef+i)).nc_var, "units",
		               strlen((*(coldef+i)).unit), 
			       (*(coldef+i)).unit);
      if (status != NC_NOERR) 
	  nc_handle_error(status);

      if (strlen((*(coldef+i)).long_name)) {
         status = nc_put_att_text(ncid, (*(coldef+i)).nc_var, 
			          "long_name",
		                  strlen((*(coldef+i)).long_name), 
			          (*(coldef+i)).long_name);
         if (status != NC_NOERR) 
	    nc_handle_error(status);
      }

      if ((*(coldef+i)).scale_factor != NO_VALUE) {
         status = nc_put_att_float(ncid, (*(coldef+i)).nc_var, 
			           "scale_factor", NC_FLOAT, 1,
			           &((*(coldef+i)).scale_factor));
         if (status != NC_NOERR) 
	    nc_handle_error(status);
      }

      if ((*(coldef+i)).add_offset != NO_VALUE) {
         status = nc_put_att_float(ncid, (*(coldef+i)).nc_var, 
			           "add_offset", NC_FLOAT, 1,
			           &((*(coldef+i)).add_offset));
         if (status != NC_NOERR) 
	    nc_handle_error(status);
      }

      if ((*(coldef+i)).valid_min != NO_VALUE) {
         status = nc_put_att_float(ncid, (*(coldef+i)).nc_var, 
			           "valid_min", NC_FLOAT, 1,
			           &((*(coldef+i)).valid_min));
         if (status != NC_NOERR) 
	    nc_handle_error(status);
      }

      if ((*(coldef+i)).valid_max != NO_VALUE) {
         status = nc_put_att_float(ncid, (*(coldef+i)).nc_var, 
			           "valid_max", NC_FLOAT, 1,
			           &((*(coldef+i)).valid_max));
         if (status != NC_NOERR) 
	    nc_handle_error(status);
      }

      if ((*(coldef+i)).missing_value != NO_VALUE) {
         status = nc_put_att_float(ncid, (*(coldef+i)).nc_var, 
			           "missing_value", NC_FLOAT, 1,
			           &((*(coldef+i)).missing_value));
         if (status != NC_NOERR) 
	    nc_handle_error(status);
      }
    }
    if (strlen(globaldef.title)) {
         status = nc_put_att_text(ncid, NC_GLOBAL, 
			          "title",
		                  strlen(globaldef.title), 
			          globaldef.title);
         if (status != NC_NOERR) 
	    nc_handle_error(status);
    }

    if (strlen(globaldef.history)) {
         status = nc_put_att_text(ncid, NC_GLOBAL, 
			          "history",
		                  strlen(globaldef.history), 
			          globaldef.history);
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
        printf("Usage: csi2ncdf [-i inputfile -o outputfile \n");
        printf("       -f formatfile] [-l num_lines] -h   \n\n");

    /* If not only usage info requested, give more info */
        if (!usage)
        {
        printf(" -i inputfile     : name of Campbell binary file\n");
        printf(" -o outputfile    : name of netcdf file\n");
        printf(" -f formatfile    : name of file describing format of inputfile\n");
        printf(" -l num_lines     : displays num_lines of the input datafile on screen\n");
        printf(" -h               : displays usage\n");
        }
}
