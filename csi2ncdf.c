#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "netcdf.h"

#include "ncdef.h"
#include "csibin.h"
#include "in_out.h"
#include "error.h"

#define MAXCOL 100

#define MAX_BYTES 256

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
 * function protypes
 * ........................................................................
 */
void  
     do_conv_csi(FILE*, int, FILE*, int);
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
int main(int argc, char *argv[])
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
       *formfile;                       /* format file */
    boolean
        only_usage = TRUE;              /* switch for info */
    int
        status,
        list_line = 0,
        ncid;
    /* ....................................................................
     */

    /* (1) Determine disk file name of program */
    strcpy(program,(argv[0]));

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
                 break;

               /* Output file */
               case 'i' :
                 cmd_arg(&argv, &argc, 2, infname);
                 break;

               /* Format file */
               case 'f' :
                 cmd_arg(&argv, &argc, 2, formatfname);
                 break;

               /* List number of lines */
               case 'l' :
                 cmd_arg(&argv, &argc, 2, dumstring);
                 list_line = atoi(dumstring);
                 break;

               /* Show help */
               case 'h' :
                 info(FALSE);
		 return 0;
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
    if (!list_line && !strlen(outfname)) {
        info(only_usage);
        error("no output file specified\n", CMD_LINE_ERROR);
    }
    if (!strlen(infname)) {
        info(only_usage);
        error("no input file specified\n", CMD_LINE_ERROR);
    }
    if (!list_line && !strlen(formatfname)) {
        info(only_usage);
        error("no format file specified\n", CMD_LINE_ERROR);
    }

    /* (4) Open files and test for success */
    /* (4.1) Output file */
    if (!list_line) {
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
    if (!list_line && (formfile = fopen(formatfname, "rb")) == NULL) {
       sprintf(mess, "cannot open file %s for reading.\n", formatfname);
       error(mess, (int) FILE_NOT_FOUND);
    }

    /* (5) Do conversion */
    do_conv_csi(infile, ncid, formfile, list_line);

    /* (6) Close files */
    fclose(infile);
    if (!list_line) {
      fclose(formfile);
      status = nc_close(ncid);
      if (status != NC_NOERR)
          nc_handle_error(status);
    }
    return 0;
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
            data[MAX_BYTES];
     float value;
     size_t count;
     int   array_id, i, num_bytes, curr_byte;
     int   linenum, colnum, status, numcoldef;
     column_def
           coldef[MAXCOL];
    /* ....................................................................
     */
    /* (1) Read definition of columns from format file */
    if (!list_line) 
      def_nc_file(ncid, formfile, coldef, &numcoldef, (int) MAXCOL);

    /* (2) Initialize */
    linenum=0;
    colnum=0;
    array_id = 0;

    /* (3) Loop input file, reading some data at once, and writing to
     *     netcdf file if array of data is full 
     */     
    while ((!list_line && !feof(infile)) ||
           (linenum <= list_line && !feof(infile)) ||
           (list_line == -1 && !feof(infile))) {

       /* (3.1) Read data; if no more data in file, return  */
       if ((num_bytes =
           fread(data, sizeof(data[0]), MAX_BYTES, infile)) == 0)
           return;
       else {

         /* (3.2) Data read, so process now: walk through data */
         curr_byte = 0;
         while (curr_byte < num_bytes) {
            /* (3.2.1) Determine type of byte read */
            switch (bytetype((data+curr_byte))) {
              case TWO_BYTE:
                value = conv_two_byte((data+curr_byte));
                if ((list_line && linenum <= list_line) ||
                    (list_line == -1))
                        printf("%f ", value);
                colnum++;
                curr_byte = curr_byte + 2;
                break;
              case FOUR_BYTE_1:
                  if (bytetype((data+curr_byte+2)) == FOUR_BYTE_2)
                      value =  conv_four_byte((data+curr_byte), (data+curr_byte+2));
                    else
                      error("unexpected byte pair in file",-1);
                  if ((list_line && linenum <= list_line) ||
		      (list_line == -1))
                           printf("%f ", value);
                  colnum++;
                  curr_byte = curr_byte + 4;
                  break;
              case START_OUTPUT:
                  array_id =  conv_arrayid((data+curr_byte));
                  linenum++;
                  colnum=1;
                  if ((list_line && linenum <= list_line) ||
		      (list_line == -1))
                          printf("\n %i ", array_id);
                  curr_byte = curr_byte + 2;
                  break;
              default:
                  error("unknown byte pair",-1);
                  break;
            }

            /* (3.2.2) Put sample in appropriate array */
            if (!list_line) {
              for (i=0; i< numcoldef; i++) 
                 if (coldef[i].array_id == array_id &&
                     coldef[i].col_num == colnum) {
		   /* First check if array is full; if so, dump data to
		    * file */
                   if (coldef[i].curr_index == MAX_SAMPLES) {
                      count = coldef[i].index - coldef[i].first_index;
                      status = nc_put_vara_float(
		                 ncid, coldef[i].nc_var,
                                 &(coldef[i].first_index),
                                 &count,
                                 coldef[i].values);
                      coldef[i].first_index = coldef[i].index;
                      coldef[i].curr_index=0;
                      if (status != NC_NOERR) 
                         nc_handle_error(status);
                   } 
		   /* Add data sample to array */
                   coldef[i].values[coldef[i].curr_index]
			       = (float) value; 
                   (coldef[i].index)++;
                   (coldef[i].curr_index)++;
                 }
            }
         }
       } 
    }
    /* (4) Dump the remains of the data samples to file */
    for (i=0 ; i<numcoldef; i++) {
        count = coldef[i].index - coldef[i].first_index;
	      status = nc_put_vara_float(
		  ncid, coldef[i].nc_var,
		  &(coldef[i].first_index),
		  &count,
		  coldef[i].values);
    if (status != NC_NOERR)
           nc_handle_error(status);

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
        printf("                    a value of -1 will list the entire file; in this way\n");
        printf("                    the program can be used as a replacement for Campbells split\n");
        printf(" -h               : displays usage\n");
        }
}
