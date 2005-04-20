/* $Id$ */
/* csi2ncdf: program to convert data files of Campbell Scientific Inc. to 
 *           NetCDF files.
 * Copyright (C) 2000 Meteorology and Air Quality Group (Wageningen University),
 *                    Arnold Moene
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#include   <stdlib.h>
#include   <stdio.h>
#include   <string.h>
#include   <math.h>

#include   "netcdf.h"

#define MAXCOL   256
#define MAXLINELEN   20000
#include   "ncdef.h"
#include   "csibin.h"
#include   "in_out.h"
#include   "error.h"
#include   "csicond.h"


#define CSI2NCDF_VER "2.2.12"

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
 *                            if Inputfile is a dash (-), data is read 
 *                            from the standard input.
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
 *            -b start condition: start output when condition is true
 *            -e stop condition: stop output when condition is true 
 *            -t ftype      : input file is text file of type ftape:
 *                            csv -> comma separated
 *                            ssv -> space separated
 *                            tsv -> tab separated
 *            -n new_type   : input file is of new type:
 *                            tob1 -> table oriented binary 1 (minimal support, only
 *                            writing to stdout)
 *            -k colnum     : column to write to stdout (only works for tob1); more than
 *                            one -k option is allowed
 *            -h            : displays usage
 *
 *
 * Author   : Arnold Moene
 *            Department of Meteorology, Wageningen Agricultural University
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

/* typeline_decode */
void typeline_decode(char* s, int coltype[MAXCOL],  int *ncol) {

   int status,i, col;
   char dumstring[MAX_STRINGLENGTH], delimiter, *pCh,
        *pChSpace,
	dumstring2[MAX_STRINGLENGTH];
   float dumfloat;

   // Set separator
   delimiter=',';

   col = 0;
   while ((col < MAXCOL) && (strchr(s, delimiter) != NULL)) {
       if (s[0] == delimiter) {
          i = 0;
          while (s[i] == delimiter)
             i++;
          pChSpace = &s[i];
          strcpy(s, pChSpace);
       }
       if (strchr(s, delimiter) != NULL) {
          i = 0;
          while (s[i] != delimiter)
              i++;
       } else
	  i = strlen(s);
       strncpy(dumstring, s, i);
       if (dumstring[i-1] == '"') {
          strncpy(dumstring2, &dumstring[1], i-2);
          dumstring2[i-2]='\0';
       } else {
          strncpy(dumstring2, &dumstring[1], i-4);
          dumstring2[i-4]='\0';
       }
       if (! strcmp(dumstring2, "ULONG"))
	   coltype[col] = TOB_ULONG;
       else if (! strcmp(dumstring2, "IEEE4"))
	   coltype[col] = TOB_IEEE4;
       else if (! strcmp(dumstring2, "IEEE4L"))
	   coltype[col] = TOB_IEEE4L;
       else if (! strcmp(dumstring2, "FP2"))
	   coltype[col] = TOB_FP2;
       else
	   printf("Error: unknown data type in TOB file: %s\n", dumstring2);
       s = s + i ;
       col++;
   }
   *ncol = col;
}

/* ........................................................................
 * Function : do_conv_tob1
 * Purpose  : Rudimentary function to convert one data column from TOB1 
 *            file to text on standard output
 *
 * Interface: infile           in   input files
 *            ncid             out  netcdf id of output file
 *            formfile         in   text file describing format
 *            list_line        in   number of lines of input file to
 *                                  list
 *            print_col        in   switch which columns should be printed
 * Date     : December 16, 2003
 */
void do_conv_tob1(FILE *infile, int ncid, FILE *formfile, int list_line, boolean print_col[MAXCOL])
{
	char buffer[1024], two_char[2];
        char dumstring[MAX_STRINGLENGTH], delimiter, *pCh,
             *pChSpace, substring[MAX_STRINGLENGTH], s[MAX_STRINGLENGTH];
	int  i, ncol, coltype[MAXCOL], cur_line;
	unsigned long dum_long;
	float dum_float;


	/* Skip first four lines */
        for (i = 0; i<4; i++) {
           fgets(buffer, sizeof(buffer), infile);
        }
        fgets(buffer, sizeof(buffer), infile);

	/* Determine number of columns and type of numbers */
        typeline_decode(buffer, coltype,  &ncol);
	cur_line = 0;
        while (!feof(infile) && ((cur_line < list_line) || (list_line == -1))) {
	   cur_line++;
           for (i=0; i< ncol; i++) {
              switch (coltype[i])  {
                 case TOB_ULONG:
                    fread(&dum_long, sizeof(dum_long), 1, infile);
		    if (print_col[i]) printf("%u ", dum_long);
		    break;
                 case TOB_IEEE4:
                    fread(&dum_float, sizeof(dum_float), 1, infile);
		    if (print_col[i]) printf("%f ", dum_float);
		    break;
                 case TOB_IEEE4L:
                    fread(&dum_float, sizeof(dum_float), 1, infile);
		    if (print_col[i]) printf("%f ", dum_float);
		    break;
                 case TOB_FP2:
                    fread(two_char, sizeof(two_char[0]), 2, infile);
                    dum_float = conv_two_byte(two_char);
		    if (print_col[i]) printf("%f ", dum_float);
		    break;
		 default:
                    break;
              }
           }
	   printf(" \n");
        }

		
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
void do_conv_csi(FILE *infile, int ncid, FILE *formfile,   int list_line,
                 maincond_def loc_cond[MAXCOND], int n_cond,
                 maincond_def start_cond,
                 maincond_def stop_cond,
                 boolean sloppy, int inftype, boolean txtfile, boolean fake,
		 boolean print_col[MAXCOL])

{
    /*
     * variable declarations
     *
     */

     unsigned   char
         buffer[MAX_BYTES], data[MAX_BYTES*2], myswitch;
     char    txtline[MAX_BYTES];
     float   timvalue;
     double  value, txtdata[MAXCOL];
     size_t  count[2], start[2];
     int     array_id,   i,   j,   num_bytes, curr_byte, timcol, rest_byte;
     int     linenum, colnum, status,  numcoldef;
     int     wanted_data, ncol, def_array_id, l_index, l_curr_index;
     int     ndummy;
     char    *printline  , dumstring[100];
     boolean have_start, have_stop, start_data, stop_data, end_txtline,
             valid_sample, fake_did_start_output;
     column_def
         coldef[MAXCOL];


           
    /* ....................................................................
     */
    /* (1) Read definition of columns from format file */
    numcoldef = 0;
    if    (!list_line)
      def_nc_file(ncid, formfile, coldef,   &numcoldef,   (int)   MAXCOL);

    /* (2) Initialize */
    fake_did_start_output = FALSE;
    linenum=0;
    colnum=0;
    array_id = -1;
    have_start = (start_cond.cond_text != NULL);
    have_stop = (stop_cond.cond_text != NULL);
    if (have_start)
       start_data = FALSE;
    else 
       start_data = FALSE;
    if (have_stop)
       stop_data = FALSE;
    else 
       stop_data = FALSE;
    if (fake)
       if (list_line)
          def_array_id = 0;
       else 
          def_array_id = coldef[0].array_id;
    for (i=0; i < numcoldef; i++)
       coldef[i].got_val = FALSE;
    
    /* (3) Loop input file, reading some data at once, and writing to
     *     netcdf file if array of data is full 
     */      
    curr_byte = 0;
    num_bytes = 0;
    ndummy = 0;
    while ((!stop_data) &&
           (!list_line && !feof(infile)) ||
           (linenum <= list_line   &&   !feof(infile))   ||
           (list_line == -1 && !feof(infile))) {

       rest_byte = num_bytes - curr_byte;

       /* (3.1) Read data  */
       if (txtfile) {
	   num_bytes = (int) fgets(txtline, sizeof(txtline), infile);
           end_txtline = FALSE;
           txtdecode(txtline, txtdata, inftype, &ncol);
	   colnum = 0;
       } else {
           num_bytes = fread(buffer, sizeof(data[0]), MAX_BYTES, infile);
       }


       /* (3.2) Data read, so process now: walk through data */
       /* if no more data in file, skip processing  */
       if (num_bytes)  {
         if (!txtfile) {
            /* Copy data from buffer to data */
            for (i = 0; i < rest_byte; i++)
               data[i] = data[curr_byte+i];
            for (i = 0; i < num_bytes; i++)
               data[i+rest_byte] = buffer[i];
            num_bytes = num_bytes + rest_byte;
            curr_byte = 0;
            /* We have to take the risk that the last 2 bytes are the first half
               of a 4 byte word, bad luck */
	 }
	    
            while ((!txtfile && (!stop_data && (curr_byte < num_bytes-2))) ||
	           ( txtfile && (!stop_data && !end_txtline))) {
               /* (3.2.1) Determine type of byte read */
	      if (txtfile) {
                  if (colnum == 0) {
		     if (fake && fake_did_start_output) 
	                myswitch = TXT_VALUE;
		     else {
	                myswitch = START_OUTPUT;
		        fake_did_start_output = TRUE;
		     }
		  } else {
	             myswitch = TXT_VALUE;
		     fake_did_start_output = FALSE;
		  }
	      } else
                  myswitch = bytetype((data+curr_byte));
	      valid_sample = FALSE;
	      if ((ndummy > 0) && (myswitch != DUMMY_WORD)) {
                  printf("previous message repeated %i times \n;", ndummy);
		  ndummy = 0;
              }
              switch (myswitch)   {
                 case TXT_VALUE:
		     if (colnum >= 0) {
			   colnum++;
	     	           valid_sample = TRUE;
                     }
		     if (colnum ==  ncol)
			     end_txtline = TRUE;

	             value = txtdata[colnum-1];
                     if ((list_line && linenum   <=   list_line) ||
                         (list_line == -1)) {
                         if (print_col[colnum-1]) {
                            sprintf(dumstring, "%G ", value);
                            if (!printline)  printline = get_clearstring(MAXLINELEN);
                            strcat(printline, dumstring);
			 }
                     }
		     // We need >= 0 here because in case of fake, we also need the first column !!
		     break;
                 case TWO_BYTE:
                     value =  (double) conv_two_byte((data+curr_byte));
                     if ((list_line && linenum   <=   list_line) ||
                         (list_line == -1)) {
                         if (print_col[colnum-1]) {
                            sprintf(dumstring, "%f ", value);
                            strcat(printline, dumstring);
			 }
                     }
		     if (colnum > 0) {
			    colnum++;
			    valid_sample = TRUE;
                     }
                     curr_byte = curr_byte + 2;
                     break;
                 case FOUR_BYTE_1:
                     if   (bytetype((data+curr_byte+2))   ==   FOUR_BYTE_2) {
                         value = (double) conv_four_byte((data+curr_byte), 
                             (data+curr_byte+2));
		         if (colnum > 0) {
				 colnum++;
				 valid_sample = TRUE;
                         }
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
                         if (print_col[colnum-1]) {
                            sprintf(dumstring, "%f ", value);
                            strcat(printline, dumstring);
                         }
                     }
                     break;

                 case START_OUTPUT:
                     /* First handle conditions of previous ArrayID */
                     wanted_data = all_cond(loc_cond, n_cond);
                     if (array_id > 0)
                       if (have_start)
                          start_data = (start_data || all_cond(&start_cond, 1));
                       else
                          start_data = TRUE;
                     if (array_id > 0)
                        if (have_stop)
                        stop_data = all_cond(&stop_cond, 1);
                     else
                        stop_data = FALSE;
                        
                     if (printline) {
                       if (((have_start && start_data) && wanted_data) ||
                           (!have_start && wanted_data))
                           printf("%s\n", printline);
                       free(printline);
                       printline = NULL;
                     }

		     /* Make sure that arrays of give array_id are
		      * synchronized */
		     if (sloppy) {
                       for (i=0; i < numcoldef; i++) {
                         if (coldef[i].array_id == array_id) {
                           if (!coldef[i].got_val) { 
                             if (coldef[i].missing_value == NO_VALUE) {
	    	               switch(coldef[i].nc_type)
	  	               {
	  		         case NC_BYTE:
	  	                    value = (double) NC_FILL_BYTE;
                                    break;
	  		         case NC_CHAR:
	  	                    value = (double) NC_FILL_CHAR;
                                    break;
	  		         case NC_INT:
	  		            value = (double) NC_FILL_INT;
                                    break;
	  		         case NC_SHORT:
	  		            value = (double) NC_FILL_SHORT;
                                    break;
	  		         case NC_FLOAT:
	  		             value = (double) NC_FILL_FLOAT;
                                     break;
	  		         case NC_DOUBLE:
	     	  	             value = (double) NC_FILL_DOUBLE;
                                     break;
                               }
                             } else {
		               value = (double) coldef[i].missing_value;
			     }
			     for (j = 0; j< coldef[i].ncol; j++)
                                 coldef[i].values[coldef[i].ncol*
                                                  coldef[i].curr_index+j] = 
			               (double) value;
                             printf("warning: filling missing value with fake\n");
                             printf("line num = %i variable = %s\n", linenum, coldef[i].name);
                             (coldef[i].index)++;
                             (coldef[i].curr_index)++;
                             coldef[i].got_val = TRUE;
                           }
                         }
		       }
                     }
                     for (i=0; i < numcoldef; i++) {
                       if (coldef[i].array_id == array_id) {
			 l_index = coldef[i].index;
			 l_curr_index = coldef[i].curr_index;
		       }
		     }
                     for (i=0; i < numcoldef; i++) {
                       if (coldef[i].array_id == array_id) {
                          if ((l_index != coldef[i].index) ||
                              (l_curr_index != coldef[i].curr_index)) {
                              printf("error: data of various columns not in sync at line num = %i variable = %s\n", linenum, coldef[i].name);
			      error("Either your file is corrupt (try -s) or this is a bug: please report\n", -1);
                          }
                       }
                     }
                     /* If data were not wanted, skip one line back in
                      * storage array (only for those arrays where we got
                        data !! */
                     if (((have_start && !start_data) ||
                          !wanted_data ||
                          (have_stop && stop_data)
                         ) && !list_line)
                       for (i=0; i < numcoldef; i++) {
                          if (coldef[i].got_val) {
                             (coldef[i].index)--;
                             (coldef[i].curr_index)--;
                          }
                       }
                     /* Reset info whether we got a value */
                     for (i=0; i < numcoldef; i++)
                        coldef[i].got_val = FALSE;
               
                   /* First check if array is full; if so, dump data to
                    * file */
                   for (i=0; i < numcoldef; i++)
                   if (coldef[i].curr_index == MAX_SAMPLES)   {
                      start[0]=coldef[i].first_index;
                      start[1]=0;
                      count[0]= coldef[i].index   - coldef[i].first_index;
                      count[1]=coldef[i].ncol;
                      status = nc_put_vara_double(
                       ncid, coldef[i].nc_var,
                                 start,
                                 count,
                                 coldef[i].values);
                      coldef[i].first_index = coldef[i].index;
                      coldef[i].curr_index=0;
                      if (status   !=   NC_NOERR) 
                         nc_handle_error(status);
                   }

                     /* Now start handling of new data */
		     if (fake)
			 array_id = def_array_id;
		     else 
		        if (txtfile)
		           array_id = (int) txtdata[0];
		        else 
                           array_id = conv_arrayid((data+curr_byte));

                     /* Value may be needed for condition checks */
		     if (!fake)
                         value = (double) array_id;
		     else
                         value = (double) txtdata[0];
                     reset_cond(loc_cond, n_cond, array_id);
                  
		     /* Advance one line, if needed make new printline and reset colnum*/
                     linenum++;
                     if ((list_line   &&   linenum <= list_line) ||
                           (list_line == -1)) {
			 free(printline);
                         printline = get_clearstring(MAXLINELEN);
			 if (print_col[colnum-1]) {
			    if (!fake)  
                                sprintf(dumstring, "%i ", array_id);
			    else
                                sprintf(dumstring, "%f ", value);
                            strcat(printline, dumstring);
                         }
                     }
		     if (!txtfile)
                         curr_byte =   curr_byte +   2;
		     if (txtfile && fake) 
                        colnum=0;
		     else
                        colnum=1;
                     break;

                 case DUMMY_WORD:
		     if (ndummy == 0) 
                        printf("found dummy word on line %i\n", linenum);
		     ndummy++;
                     curr_byte =   curr_byte +   2;
			/* Set column number to -1 to show that something is 
			 * wrong */ 
                     colnum = -1;
                     break;
                 default:
                     if (sloppy) {
                        printf("warning unkown byte type\n");
                        curr_byte++;
			/* Set column number to -1 to show that something is 
			 * wrong */ 
                        colnum = -1;
                     } else   {
                        status = nc_close(ncid);
                        close(infile);
                        error("unknown byte type",-1);
                     }
                     break;
            } /* switch */

            /* (3.2.2) Check condition */
            check_cond(loc_cond, n_cond, array_id, colnum, value);
            if (have_start)
              check_cond(&start_cond, 1, array_id, colnum, value);
            if (have_stop)
              check_cond(&stop_cond, 1, array_id, colnum, value);
            
            /* (3.2.2) Put sample in appropriate array */
            if   (!list_line && valid_sample) {
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


                   /* Add data sample to array */
                   /* This is not a following variable and not time */
                   if ((coldef[i].follow_id   ==   -1)   &&
                       (!coldef[i].i_am_time)) {
                       coldef[i].values[coldef[i].ncol*
                                        coldef[i].curr_index+
                                        colnum-coldef[i].col_num]
                           = (double) value;
                       if (coldef[i].col_num + coldef[i].ncol-1 == colnum) {
                          (coldef[i].index)++;
                          (coldef[i].curr_index)++;
                          coldef[i].got_val = TRUE;
                          if (coldef[i].time_comp)   {
                             if (coldef[i].time_csi_hm) 
				  value = (double) conv_hour_min(value);
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
                            (double) value;
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
                            if (coldef[i].follow_missed) {
                               printf("warning: did not have data for following variable %s on %i lines\n",
                               coldef[i].name, coldef[i].follow_missed);
                               coldef[i].follow_missed = 0;
                            }
                          } else
                             (coldef[i].follow_missed)++;
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
      if (((have_start && start_data) && wanted_data) ||
          (!have_start && wanted_data))
         printf("%s\n", printline);
      free(printline);
      printline = NULL;
    }


    /* If data were not wanted, skip one line back in
     * storage array */
    if ((!wanted_data && !list_line) ||
        ((have_start && !start_data) && !list_line) ||
        ((have_stop && stop_data) && !list_line)
       )
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
            status   = nc_put_vara_double(ncid, coldef[i].nc_var,
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
        print_col[MAXCOL],                /* write column to stdout ? */
        all_false,
        sloppy   = FALSE,
        only_usage =   TRUE,                /* switch for info */
	txtfile = FALSE,                  /* is input file a text file */
	fake = FALSE;                     /* fake an array ID */
    int
        i,
        colnum,
        status,
        list_line   = 0,
        ncid = 0,
        n_cond = 0,
	inftype = FTYPE_CSIBIN;
    maincond_def
        loc_cond[MAXCOND];
    maincond_def
        start_cond;
    maincond_def
        stop_cond;
        
        
    /* ....................................................................
     */

    /* (0) Initialize */
    start_cond.cond_text = NULL;
    stop_cond.cond_text = NULL;
    for (i=0; i< MAXCOL; i++)
	    print_col[i] = FALSE;
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
            c = (*(argv[0] + 1));
            switch (c) {
               /* Output file */
               case 'o'   :
                 cmd_arg(&argv, &argc,   2,   outfname);
                 break;

               /* Input file */
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

               /* Fake an arrayID */
               case 'a'   :
                 fake   = TRUE;
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
                 
               /* Start condition */
               case 'b'   :
                 cmd_arg(&argv, &argc,   2,   dumstring);
                 start_cond.cond_text =
                     (char *) malloc(strlen(dumstring)+1);
                 strcpy(start_cond.cond_text, dumstring);
                 parse_main_cond(&start_cond);
                 break;
                 
               /* Stop condition */
               case 'e'   :
                 cmd_arg(&argv, &argc,   2,   dumstring);
                 stop_cond.cond_text =
                     (char *) malloc(strlen(dumstring)+1);
                 strcpy(stop_cond.cond_text, dumstring);
                 parse_main_cond(&stop_cond);
                 break;

               /* Text file type */
               case 't'   :
                 cmd_arg(&argv, &argc,   2,   dumstring);
		 if (!strcmp(dumstring, "csv")) {
		     inftype = FTYPE_TXTCSV;
		     txtfile = TRUE;
		 } else if (!strcmp(dumstring, "ssv")) {
		     inftype = FTYPE_TXTSSV;
		     txtfile = TRUE;
		 } else if (!strcmp(dumstring, "tsv")) {
		     inftype = FTYPE_TXTTSV;
		     txtfile = TRUE;
		 } else
                     error("unknown text file type\n", -1);
		 break;

               /* New table oriented file */
               case 'n' :
                 cmd_arg(&argv, &argc,   2,   dumstring);
		 if (!strcmp(dumstring, "tob1")) {
		     inftype = FTYPE_NEWTOB1;
		     txtfile = FALSE;
		 } else
                     error("unknown new file type\n", -1);
		 break;
              
               /* Stdout column number */
               case 'k':
                 cmd_arg(&argv, &argc,   2,   dumstring);
		 colnum = atoi(dumstring)-1;
                 if ((colnum >= 0) && (colnum < MAXCOL))
                     print_col[atoi(dumstring)-1] = TRUE;
                 else
                     error("invalid column number (larger than MAXCOL)\n", CMD_LINE_ERROR);
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
    /* Check if all print_col is FALSE, then set all print_col to TRUE */
    all_false = TRUE;
    for (i=0; i< MAXCOL; i++)
        if (print_col[i])
		all_false = FALSE;
    if (all_false)
        for (i=0; i< MAXCOL; i++)
           print_col[i] = TRUE;

    /* (3) Trap error situations */
    only_usage   = TRUE;
    if ((inftype == FTYPE_NEWTOB1) && (!list_line)) {
        error("file type is TOB1 and no listing to stdout requested\n", CMD_LINE_ERROR);
    }
    if (!list_line && !strlen(outfname)) {
        info(only_usage);
        error("no output file specified\n", CMD_LINE_ERROR);
    }
    if (!strlen(infname)) {
        info(only_usage);
        error("no input file specified\n", CMD_LINE_ERROR);
    }
    if ((inftype != FTYPE_NEWTOB1) && !list_line && !strlen(formatfname)) {
        info(only_usage);
        error("no format file specified\n", CMD_LINE_ERROR);
    }

    /* (4) Open files and test for success */
    /* (4.1) Output file */
    if (!list_line) {
      status =   nc_create(outfname, NC_WRITE, &ncid);
      if   (status != NC_NOERR)   
         nc_handle_error(status);
    }

    /* (4.2) Input file */
    if (!strcmp(infname,"-")) 
       infile = stdin;
    else {
       if (txtfile) 
          infile = fopen(infname,   "rt");
       else
          infile = fopen(infname,   "rb");
    }
    if (infile  == NULL) {
       sprintf(mess,   "cannot open file %s for reading.\n", infname);
       error(mess, (int) FILE_NOT_FOUND);
    }

    /* (4.3) Format file */
    formfile = NULL;
    if (!list_line && (formfile = fopen(formatfname, "rt"))   ==   NULL)   {
       sprintf(mess,   "cannot open file %s for reading.\n", formatfname);
       error(mess, (int) FILE_NOT_FOUND);
    }

    /* (5) Do conversion */
    if (inftype == FTYPE_NEWTOB1)
       do_conv_tob1(infile, ncid, formfile, list_line, print_col);
    else
       do_conv_csi(infile,   ncid,   formfile, list_line,
                loc_cond, n_cond, start_cond, stop_cond, sloppy,inftype, 
		txtfile, fake, print_col);

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
        printf("Usage: csi2ncdf -i inputfile [-o outputfile \n");
        printf("       -f formatfile]  [-t txtype] [-n] [-l num_lines] [-s] \n");
	printf("       [-c condition] [-a] [-k colnum] -h  \n\n");

    /* If not only usage info requested, give more info */
        if (!usage)
        {
        printf(" -i inputfile     : name of Campbell binary file\n");
        printf("                    if intputfile is a dash (-), data is read from standard input\n");
        printf(" -o outputfile    : name of netcdf file\n");
        printf(" -f formatfile    : name of file describing format of inputfile\n");
        printf(" -l num_lines     : displays num_lines of the input datafile on screen\n");
        printf("                    a value of -1 will list the entire file; in this way\n");
        printf("                    the program can be used as a replacement for Campbells split\n");
        printf(" -s               : be sloppy on errors in input file\n");
        printf(" -c condition     : only output data subject to condition\n");
        printf("                    (see README for details)\n");
        printf(" -t txtftype      : input file is a text file, with type: \n");
        printf("                    csv : comma separated\n");
        printf("                    ssv : space separated\n");
        printf("                    tsv : tab separated\n");
	printf(" -n new_type      : input file is of type new binary type:\n");
        printf("                    tob1: table oriented binary 1 (minimal support\n");
        printf("                          only writing to stdout\n");
        printf(" -k colnum        : column to write to stdout (only works for tob1); more than\n");
        printf("                    one -k option is allowed\n");
        printf(" -a               : don't use arrayID from file (e.g. because there is no \n");
        printf("                    but assume that all lines have the same ID, which is taken \n");
        printf("                    from the first definition in the format file; only needed when\n");
        printf("                    writing a netcdf file. If listing to stdout, arrayID is set to 0\n");
        printf(" -h               : displays usage\n");
	printf("Version: %s\n", CSI2NCDF_VER);
        printf("Copyright (C) 2000-2003 Meteorology and Air Quality Group (Wageningen University), Arnold Moene\n");
        printf("This program is free software; you can redistribute it and/or\n");
        printf("modify it under the terms of the GNU General Public License\n");
        printf("as published by the Free Software Foundation; either version 2\n");
        printf("of the License, or (at your option) any later version.\n");
        }
}
