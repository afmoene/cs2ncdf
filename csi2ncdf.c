/* $Id$ */
#include   <stdlib.h>
#include   <stdio.h>
#include   <string.h>
#include   <math.h>

#include   "netcdf.h"

#define MAXCOL   100
#include   "ncdef.h"
#include   "csibin.h"
#include   "in_out.h"
#include   "error.h"
#include   "csicond.h"


#define CSI2NCDF_VER "2.0.1"

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
 * Usage    : csi2ncdf [-i Inputfile -o Outputfile -f Formatfile]
 *                     [-l num_lines] -h
 *
 *            -i Inputfile  : name (including path) of inputfile
 *            -o Outputfile : name (including path) of outputfile
 *            -f Formatfile : name (including path) of format file
 *            -l num_lines  : displays num_lines of the input datafile on
 *                            screen; a value of -1 will list the entire
 *                            file; in this way the program can be used as
 *                            a replacement for Campbells split
 *            -s            : be sloppy: do not abort on errors in input file
 *                            but do give warnings
 *            -c Condition  : only output data under certain condition
 *                            (see README for details)
 *            -h            : displays usage
 *
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
     info(boolean   usage);         /* displays info about the program */

/* ........................................................................
 * global variable declarations
 * ........................................................................
 */
char
    program[100];                     /* name of program from command line */

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
void do_conv_csi(FILE *infile, int ncid, FILE *formfile,   int list_line,
                 maincond_def loc_cond[MAXCOND], int n_cond,
                 boolean sloppy )

{
    /*
     * variable declarations
     *
     */
     column_def
         coldef[MAXCOL];

     unsigned   char
         buffer[MAX_BYTES], data[MAX_BYTES*2];
     float value, timvalue;
     size_t   count[2], start[2];
     int     array_id,   i,   j,   num_bytes, curr_byte, timcol, rest_byte;
     int     linenum, colnum, status,  numcoldef = 0;
     int     wanted_data;
     char    *printline = NULL, dumstring[100];


           
    /* ....................................................................
     */
    /* (1) Read definition of columns from format file */
    if (!list_line)
      def_nc_file(ncid, formfile, coldef,   &numcoldef,   (int)   MAXCOL);

    /* (2) Initialize */
    linenum=0;
    colnum=0;
    array_id =   -1;

    /* (3) Loop input file, reading some data at once, and writing to
     *     netcdf file if array of data is full 
     */      
    curr_byte = 0;
    num_bytes = 0;
    while ((!list_line && !feof(infile)) ||
           (linenum <= list_line   &&   !feof(infile))   ||
           (list_line == -1 && !feof(infile))) {

       rest_byte = num_bytes - curr_byte;
       
       /* (3.1) Read data; if no more data in file, return  */
       if ((num_bytes =
           fread(buffer, sizeof(data[0]), MAX_BYTES, infile))   ==   0)
           return;
		 /* (3.2) Data read, so process now: walk through data */
       else   {
         /* Copy data from buffer to data */
         for (i = 0; i < rest_byte; i++)
            data[i] = data[curr_byte+i];
         for (i = 0; i < num_bytes; i++)
            data[i+rest_byte] = buffer[i];
         num_bytes = num_bytes + rest_byte;
         curr_byte = 0;
         /* We have to take the risk that the last 2 bytes are the first half
            of a 4 byte word, bad luck */
         while   (curr_byte < num_bytes-2)   {
            /* (3.2.1) Determine type of byte read */
            switch (bytetype((data+curr_byte)))   {
              case TWO_BYTE:
                  value =   conv_two_byte((data+curr_byte));
                  if ((list_line && linenum   <=   list_line) ||
                      (list_line == -1)) {
                      sprintf(dumstring, "%f ", value);
                      strcat(printline, dumstring);
                  }
                  colnum++;
                  curr_byte = curr_byte + 2;
                  break;
              case FOUR_BYTE_1:
                  if   (bytetype((data+curr_byte+2))   ==   FOUR_BYTE_2) {
                      value =  conv_four_byte((data+curr_byte), 
                          (data+curr_byte+2));
                      colnum++;
                      curr_byte = curr_byte + 4;
                  } else  {
                      if (sloppy)  {
                          printf("warning unkown byte pair in 4 bytes\n");
                          printf("line num = %i %i\n", linenum, colnum);
                          curr_byte++;
                      } else {
                          status   = nc_close(ncid);
                          close(infile);
                          printf("line num = %i %i\n", linenum, colnum);
                          error("unexpected byte pair in file", -1);
                      }
                  }
                  if ((list_line && linenum   <=   list_line) ||
                      (list_line == -1)) {
                      sprintf(dumstring, "%f ", value);
                      strcat(printline, dumstring);
                  }

                  break;
              case START_OUTPUT:
                  /* First handle conditions of previous ArrayID */
                  wanted_data = all_cond(loc_cond, n_cond);
                  if (printline) {
                    if (wanted_data)
                       printf("%s\n", printline);
                    free(printline);
                    printline = NULL;
                  }
                  /* If data were not wanted, skip one line back in
                   * storage array (only for those arrays where we got
                     data !! */
                  if (!wanted_data && !list_line)
                    for (i=0; i < numcoldef; i++) {
                       if (coldef[i].got_val) {
                          (coldef[i].index)--;
                          (coldef[i].curr_index)--;
                       }
                    }
                  /* Reset info whether we got a value */
                  for (i=0; i < numcoldef; i++)
                     coldef[i].got_val = FALSE;
               
                  /* Now start handling of new data */
                  array_id   =   conv_arrayid((data+curr_byte));
                  /* Value may be needed for condition checks */
                  value = (float) array_id;
                  reset_cond(loc_cond, n_cond, array_id);
                  
                  linenum++;
                  colnum=1;
                  if ((list_line   &&   linenum <= list_line) ||
                        (list_line == -1)) {
                      printline = get_clearstring(10000);
                      sprintf(dumstring, "%i ", array_id);
                      strcat(printline, dumstring);
                  }
                  
                  curr_byte =   curr_byte +   2;
                  break;
              case DUMMY_WORD:
                  printf("found dummy word on line %i\n", linenum);
                  curr_byte =   curr_byte +   2;
                  break;
              default:
                  if (sloppy) {
                     printf("warning unkown byte pair\n");
                     curr_byte++;
                  } else   {
                     status = nc_close(ncid);
                     close(infile);
                     error("unknown byte pair",-1);
                  }
                  break;
            } /* switch */

            /* (3.2.2) Check condition */
            check_cond(loc_cond, n_cond, array_id, colnum, value);

            /* (3.2.2) Put sample in appropriate array */
            if   (!list_line) {
               for   (i=0;   i<   numcoldef; i++) {

                /*  Either:
                 *  - correct array_id and column number, or
                 *  - correct array_id and column number in range
                 *    between first and last column of 2D variable, or
                 *  - a variable that follows this array_id
                 *  - first column and i am the time variable
                 */
                 if ((coldef[i].array_id == array_id &&
                      ((coldef[i].col_num   ==   colnum) ||
                       ((coldef[i].col_num <= colnum)   &&
                        (coldef[i].col_num +   coldef[i].ncol-1 >= colnum)
                       ))
                     ) ||
                     ((coldef[i].follow_id   ==   array_id) &&
                      (colnum == 1)
                     ) ||
                     (coldef[i].i_am_time)
                    )   {
                    if (coldef[i].i_am_time   &&   
                          (coldef[i].time_got_comp == 
                           coldef[i].time_num_comp)) {
                       coldef[i].curr_index++;
                       coldef[i].index++;
                       coldef[i].got_val = TRUE;
                       coldef[i].time_got_comp   = 0;
                    }

                   /* First check if array is full; if so, dump data to
                    * file */
                   if (coldef[i].curr_index == MAX_SAMPLES)   {
                      start[0]=coldef[i].first_index;
                      start[1]=0;
                      count[0]= coldef[i].index   - coldef[i].first_index;
                      count[1]=coldef[i].ncol;
                      status = nc_put_vara_float(
                       ncid, coldef[i].nc_var,
                                 start,
                                 count,
                                 coldef[i].values);
                      coldef[i].first_index = coldef[i].index;
                      coldef[i].curr_index=0;
                      if (status   !=   NC_NOERR) 
                         nc_handle_error(status);
                   }
                   /* Add data sample to array */
                   /* This is not a following variable and not time */
                   if ((coldef[i].follow_id   ==   -1)   &&
                       (!coldef[i].i_am_time)) {
                       coldef[i].values[coldef[i].ncol*
                                        coldef[i].curr_index+
                                        colnum-coldef[i].col_num]
                           = (float) value;
                       if (coldef[i].col_num + coldef[i].ncol-1 == colnum) {
                          (coldef[i].index)++;
                          (coldef[i].curr_index)++;
                          coldef[i].got_val = TRUE;
                          if (coldef[i].time_comp)   {
                             if (coldef[i].time_csi_hm) 
				                    value = conv_hour_min(value);
                             timcol=coldef[i].time_colnum;
                             if (coldef[timcol].time_got_comp   ==   0)
                                 coldef[timcol].values[coldef[timcol].curr_index] =   0.0;
                             coldef[timcol].values[coldef[timcol].curr_index]+=
                               (value-coldef[i].time_offset)*coldef[i].time_mult;
                             coldef[timcol].time_got_comp++;
                          }
                       }
                   /* This is a following variable and not time */
                   } else if (!coldef[i].i_am_time)   {
                      /* This is a line with its own array_id:
                       * get data */
                      if (coldef[i].array_id == array_id)   {
                         coldef[i].follow_val[colnum-coldef[i].col_num] =
                            (float) value;
                         coldef[i].got_follow_val = TRUE;
                      /* This is a line with the array_id to follow:
                       * put previously stored data in array */
                      } else {
                          if (coldef[i].got_follow_val) {
                             for (j=0; j<coldef[i].ncol; j++)
                                coldef[i].values[(coldef[i].ncol)*
                                                 coldef[i].curr_index+j]
                                   = coldef[i].follow_val[j];
	                         (coldef[i].index)++;
                            (coldef[i].curr_index)++;
                            coldef[i].got_val = TRUE;
                          } else {
               			     printf("error: do not have value for following variable %s\n",
                             coldef[i].name);
                             nc_close(ncid);
               			     exit;
                          }
                      }
                      /* This is a time component, but we're not reading the
                         line of a following variable */
                      if (!((coldef[i].follow_id != -1) &&
                            (coldef[i].array_id == array_id))
                            &&
                            coldef[i].time_comp) {
                         if (coldef[i].time_csi_hm)
                            timvalue = conv_hour_min(coldef[i].follow_val[0]);
                         else
                            timvalue = coldef[i].follow_val[0];
                         timcol=coldef[i].time_colnum;
                         if (coldef[timcol].time_got_comp == 0)
                            coldef[timcol].values[coldef[timcol].curr_index]   = 0.0;
                         coldef[timcol].values[coldef[timcol].curr_index]+=
                            (timvalue-coldef[i].time_offset)*coldef[i].time_mult;
                         coldef[timcol].time_got_comp++;
                      } /* if */
                   } /* else */
                 } /* if */
               } /* for */
            } /* if */
         } /* while */
       }  /* else */ 
    } /* while */

    /* This was the last line in file,
       first handle conditions of previous ArrayID */
    wanted_data = all_cond(loc_cond, n_cond);
    if (printline) {
      if (wanted_data)
         printf("%s\n", printline);
      free(printline);
      printline = NULL;
    }


    /* If data were not wanted, skip one line back in
     * storage array */
    if (!wanted_data && !list_line)
      for (i=0; i < numcoldef; i++) {
         if (coldef[i].got_val) {
            (coldef[i].index)--;
            (coldef[i].curr_index)--;
         }
      }

    /* (4) Dump the remains of the data samples to file */
    if (!list_line) {
        for (i=0 ; i<numcoldef; i++)   {
            start[0]=coldef[i].first_index;
            start[1]=0;
            count[0]=coldef[i].index   - coldef[i].first_index;
            count[0]=(count[0]>0?count[0]:0);
            count[1]=coldef[i].ncol;
            status   = nc_put_vara_float(ncid, coldef[i].nc_var,
                                         start, count, coldef[i].values);
            if (status != NC_NOERR)
                 nc_handle_error(status);
        }
    }
}


/* ........................................................................
 * main
 * ........................................................................
 */
int main(int argc, char   *argv[])
{
    /*
     * variable declarations
     */
    char
        infname[255]="",                /* name of input file */
        outfname[255]="",                /* name of output file */
        formatfname[255]="",             /* name of format file */
        dumstring[255]="",                /* dummy string */
        mess[100];                      /* message passed to error */
    FILE
       *infile,                         /* input file */
       *formfile;                         /* format file */
    boolean
        sloppy   = FALSE,
        only_usage =   TRUE;                /* switch for info */
    int
        status,
        list_line   = 0,
        ncid = 0,
        n_cond = 0;
    maincond_def
        loc_cond[MAXCOND];
        
    /* ....................................................................
     */

    /* (1) Determine disk file name of program */
    strcpy(program,(argv[0]));

    /* (2) Parse command line */
    argv++;
    argc--;

    /* Cycle until all arguments read */
    for ( ;   argc > 0; argv++,   --argc )   {
        /* variable declaration */
        char c, arg[100];

        /* Check for leading '-' in flag */
        if ((*argv)[0] == '-') {
            c = (*(argv[0]   + 1));
            switch (c) {
               /* Input file */
               case 'o'   :
                 cmd_arg(&argv, &argc,   2,   outfname);
                 break;

               /* Output file */
               case 'i'   :
                 cmd_arg(&argv, &argc,   2,   infname);
                 break;

               /* Format file */
               case 'f'   :
                 cmd_arg(&argv, &argc,   2,   formatfname);
                 break;

               /* List number of lines */
               case 'l'   :
                 cmd_arg(&argv, &argc,   2,   dumstring);
                 list_line   = atoi(dumstring);
                 break;

               /* Show help */
               case 'h'   :
                 info(FALSE);
                 return   0;
                 break;

               /* Be sloppy on errors in input file */
               case 's'   :
                 sloppy   = TRUE;
                 break;
                 
               /* Condition */
               case 'c'   :
                 n_cond += 1;
                 cmd_arg(&argv, &argc,   2,   dumstring);
                 (loc_cond[n_cond-1]).cond_text =
                     (char *) malloc(strlen(dumstring)+1);
                 strcpy(loc_cond[n_cond-1].cond_text, dumstring);
                 parse_main_cond(&(loc_cond[n_cond-1]));
                 break;

               /* Invalid flag */
               default :
                  cmd_arg(&argv,   &argc, 1, arg);
                  printf("Invalid flag : %s\n",   arg);
                  info(TRUE);
                  break;
            }
        } else   {
            strcpy(arg,   argv[0]);
            printf("Invalid flag : %s\n",   arg);
            info(TRUE);
        }
    }

    /* (3) Trap error situations */
    only_usage   = TRUE;
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
      status =   nc_create(outfname, NC_CLOBBER, &ncid);
      if   (status != NC_NOERR)   
         nc_handle_error(status);
    }

    /* (4.2) Input file */
    if ((infile =   fopen(infname,   "rb")) == NULL) {
       sprintf(mess,   "cannot open file %s for reading.\n", infname);
       error(mess, (int) FILE_NOT_FOUND);
    }

    /* (4.3) Format file */
    if (!list_line && (formfile = fopen(formatfname, "rt"))   ==   NULL)   {
       sprintf(mess,   "cannot open file %s for reading.\n", formatfname);
       error(mess, (int) FILE_NOT_FOUND);
    }

    /* (5) Do conversion */
    do_conv_csi(infile,   ncid,   formfile, list_line,
                loc_cond, n_cond,  sloppy);

    /* (6) Close files */
    fclose(infile);
    if (!list_line) {
      fclose(formfile);
      status =   nc_close(ncid);
      if   (status != NC_NOERR)
          nc_handle_error(status);
    }
    return 0;
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
        printf("       -f formatfile] [-l num_lines] -s -h  \n\n");

    /* If not only usage info requested, give more info */
        if (!usage)
        {
        printf(" -i inputfile     : name of Campbell binary file\n");
        printf(" -o outputfile    : name of netcdf file\n");
        printf(" -f formatfile    : name of file describing format of inputfile\n");
        printf(" -l num_lines     : displays num_lines of the input datafile on screen\n");
        printf("                    a value of -1 will list the entire file; in this way\n");
        printf("                    the program can be used as a replacement for Campbells split\n");
        printf(" -s               : be sloppy on errors in input file\n");
        printf(" -c condition     : only output data subject to condition\n");
        printf("                    (see README for details)\n");
        printf(" -h               : displays usage\n");
	printf("Version: %s", CSI2NCDF_VER);
        }
}
