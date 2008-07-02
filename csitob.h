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
#include   <time.h>
#include   "error.h"
#include   "conv_endian.h"

#define MAX_STRINGLENGTH 20000

/* Struct to store information about the TOB file */
typedef struct {
	int frame_length;
	float samp_interval;
} tob_info;


/* Some time/date helper functions */
/* from Kernighan and Ritchie, C Handbook */
int isleapyear(int a) {
	  return ((a % 4 == 0 && a % 100 != 0) || a % 400 == 0);
}   

int dayinyear(int a) {
	return (365+isleapyear(a));
}


/* from Kernighan and Ritchie, C Handbook */
static char daytab[2][13] = {
	{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

int daynumber( int year, int month, int day){
	int i;
	for (i=0; i< month; i++)
		day += daytab[isleapyear(year)][i];
	return day;
}


/* Get a string from the delimited header line of a TOB file */
char *get_tob_string(char *s, char delimiter) {
	char dumstring[MAX_STRINGLENGTH], *pChSpace, *dumstring2;
	int i;
	
	dumstring2 = (char *) malloc(MAX_STRINGLENGTH);

	/* Delimiter is in front, remove it */
	if (s[0] == delimiter) {
		i = 0;
		while (s[i] == delimiter)
		i++;
		pChSpace = &s[i];
		strcpy(s, pChSpace);
	}
	/* Find closing delimiter and take part of strnig before that */
	if (strchr(s, delimiter) != NULL) {
		i = 0;
		while (s[i] != delimiter)
		i++;
	} else
		i = strlen(s);
	strncpy(dumstring, s, i);

	/* remove quotes */
	if (dumstring[i-1] == '"') {
		strncpy(dumstring2, &dumstring[1], i-2);
		dumstring2[i-2]='\0';
	} else {
		strncpy(dumstring2, &dumstring[1], i-4);
		dumstring2[i-4]='\0';
	}
	return dumstring2;
}

/* Decode the second line of a TOB2 or TOB3 file */
void tob_decode(char* s, tob_info *info){
   int i, framelength;
   char delimiter,
        *unitstring, 
	numstring[MAX_STRINGLENGTH],
	dumstring2[MAX_STRINGLENGTH];

   /* Set separator */
   delimiter=',';

   /* First strip trailing whitespace */
   i=strlen(s);
   while (isspace(s[--i])) 
   s[i]='\0';

   /* Skip first column */
   for (i=0; i<1; i++){
      strcpy(dumstring2 , get_tob_string(s, delimiter));
      s += strlen(dumstring2) + 2;
   }

   /* Read second column */
   strcpy(dumstring2 , get_tob_string(s, delimiter));
   s += strlen(dumstring2) + 2;

   /* Find time units */
   if ((unitstring = strstr(dumstring2, "MSEC"))) {
	   strncpy(numstring, dumstring2, (unitstring-dumstring2));
	   if (((*info).samp_interval = atof(numstring)))
	      (*info).samp_interval *= 0.001;
           else
              error("can not decode samping interval in MSEC", -1);
   }

   /* Read third column */
   strcpy(dumstring2 , get_tob_string(s, delimiter));
   s += strlen(dumstring2) + 2;

   /* Convert to int */
   if (!(framelength = atoi(dumstring2))){
       error("can not decode frame length", -1);
   } else
      (*info).frame_length = framelength;
}

/* Decode the final line of the TOB header, which contains the definition
   of the variable types */
void typeline_decode(char* s, int coltype[MAXCOL],  int *ncol) {
   int i, col;
   char delimiter,
	dumstring2[MAX_STRINGLENGTH];

   /* Set separator */
   delimiter=',';

   /* First strip trailing whitespace */
   i=strlen(s);
   while (isspace(s[--i])) 
   s[i]='\0';

   /* Cycle as long as delimiters in line */
   col = 0;
   while ((col < MAXCOL) && (strchr(s, delimiter) != NULL)) {
       strcpy(dumstring2 , get_tob_string(s, delimiter));
       if (! strcmp(dumstring2, "ULONG"))
	   coltype[col] = TOB_ULONG;
       else if (! strcmp(dumstring2, "IEEE4"))
	   coltype[col] = TOB_IEEE4;
       else if (! strcmp(dumstring2, "IEEE4L"))
	   coltype[col] = TOB_IEEE4L;
       else if (! strcmp(dumstring2, "IEEE4B"))
	   coltype[col] = TOB_IEEE4B;
       else if (! strcmp(dumstring2, "FP2"))
	   coltype[col] = TOB_FP2;
       else
	   printf("Error: unknown data type in TOB file: %s\n", s);
       col++;
       /* I'm not sure whether this always works */
       s += strlen(dumstring2) + 2;
   }
   *ncol = col;
}

/* Decode the timestap information contained in the frame header */
struct tm decode_tobtime(long tobtime){
	struct tm base_time;
	long      del_sec, del_min, del_hour, del_day, rest_sec, rest_min, rest_hour;

	/* Set base time */
	base_time.tm_mday=0;
	base_time.tm_mon=0;
	base_time.tm_year=90;
	base_time.tm_min=0;
	base_time.tm_sec=0;
	base_time.tm_hour=0;

	/* Decompose TOB timestamp into sec, min, hour and day */
	rest_sec = tobtime;
	del_day = (rest_sec - rest_sec%(24*3600))/(24*3600);
	rest_sec -= del_day*24*3600;
	del_hour = (rest_sec - rest_sec%3600)/3600;
	rest_sec -= del_hour*3600;
	del_min = (rest_sec - rest_sec%60)/60;
	rest_sec -= del_min*60;
	del_sec = rest_sec;

	/* Add timestamp to base time */
	base_time.tm_sec += del_sec;
	rest_sec = base_time.tm_sec % 60;
	base_time.tm_min += (base_time.tm_sec - rest_sec)/60;
	base_time.tm_sec = rest_sec;

	base_time.tm_min += del_min;
	rest_min = base_time.tm_min % 60;
	base_time.tm_hour += (base_time.tm_min - rest_min)/60;
	base_time.tm_min = rest_min;

	base_time.tm_hour += del_hour;
	rest_hour = base_time.tm_hour % 60;
	del_day += (base_time.tm_hour - rest_hour)/24;
	base_time.tm_hour = rest_hour;

        /* Now allocate days to days, months and years */
	while (del_day > daytab[isleapyear(base_time.tm_year)][base_time.tm_mon+1]) {
                del_day -= daytab[isleapyear(base_time.tm_year)][base_time.tm_mon+1];
		base_time.tm_mon += 1;
		if (base_time.tm_mon > 11) {
			base_time.tm_mon = 0;
			base_time.tm_year++;
		}
	}
	base_time.tm_mday += del_day;

	return base_time;
}

/* ........................................................................
 * Function : do_conv_tob
 * Purpose  : Rudimentary function to convert one data column from TOB (1 and 3)
 *            file to text on standard output
 *
 * Interface: infile           in   input files
 *            ncid             out  netcdf id of output file
 *            formfile         in   text file describing format
 *            list_line        in   number of lines of input file to
 *                                  list
 *            print_col        in   switch which columns should be printed
 *            tob_type         in   switch for type of TOB file
 *            conv_tob1_time   in   should we convert time info in first TOB1 columns to real time ? (add extra columns in output)
 *            decimal_places   in   number of decimal places in text output
 * Date     : December 16, 2003
 * Update   : August 28, 2006   : generalized to TOB1 and TOB3.
 * Update   : June 1, 2007      : write TOB1 time to output
 */
void do_conv_tob(FILE *infile, int ncid, FILE *formfile, int list_line, boolean print_col[MAXCOL], int tob_type, boolean conv_tob1_time, int decimal_places)
{
	char buffer[MAX_STRINGLENGTH]; 
        unsigned char two_char[2];
        char dumstring[MAX_STRINGLENGTH];
	int  i, ncol, coltype[MAXCOL], cur_line, nskip, byte_inframe, frame_length,
	     machine_endian;
	unsigned long dum_long;
	unsigned int dum_int;
	float dum_float, samp_interval, subseconds, rest;
	struct tm tobtime, tob1time;
	tob_info  tobfileinfo;
	int      nitems;

	/* Check endianness of machine */
        machine_endian = UtilEndianType();

	/* Skip first four or five lines, getting appropriate data from those
           lines */
	if (tob_type == FTYPE_TOB1)
		nskip = 4;
	else 
		nskip = 5;
        for (i = 0; i<nskip; i++) {
           fgets(buffer, sizeof(buffer), infile);
	   if ((i==1) && ((tob_type == FTYPE_TOB2) || (tob_type == FTYPE_TOB3))) {
              tob_decode(buffer, &tobfileinfo);
	      frame_length = tobfileinfo.frame_length;
	      samp_interval = tobfileinfo.samp_interval;
           }
        }
        /* Get last line of header and determine number of columns and type of
           numbers*/
        fgets(buffer, sizeof(buffer), infile);
        typeline_decode(buffer, coltype,  &ncol);


	byte_inframe=0;
	cur_line = 0;

	/* Get first frame header */
	if (tob_type == FTYPE_TOB2) {
         	fread(&dum_int, sizeof(dum_int), 1, infile);
		tobtime = decode_tobtime((long) dum_int);
         	fread(&dum_int, sizeof(dum_int), 1, infile);
		byte_inframe+=8;
	}
	if (tob_type == FTYPE_TOB3) {
         	fread(&dum_int, sizeof(dum_int), 1, infile);
		tobtime = decode_tobtime((long) dum_int);
         	fread(&dum_int, sizeof(dum_int), 1, infile);
         	fread(&dum_int, sizeof(dum_int), 1, infile);
		byte_inframe+=12;
        }

	/* Initialize subseconds */
	subseconds = 0;
	
        /* Loop data */
        while (!feof(infile) && ((cur_line < list_line) || (list_line == -1))) {
	   cur_line++;
	   /* report date and time */
           if ((tob_type == FTYPE_TOB2) || (tob_type == FTYPE_TOB3)) {
		printf("%i ", tobtime.tm_year+1900);
		printf("%i ", daynumber(tobtime.tm_year, tobtime.tm_mon+1, tobtime.tm_mday));
		printf("%i ", tobtime.tm_hour*100+tobtime.tm_min);
		printf("%f ", tobtime.tm_sec+subseconds);
           }
	   if (!feof(infile)) {
	    nitems = 0 ;
            for (i=0; i< ncol; i++) {
              /* We finished a frame */
              if ((tob_type == FTYPE_TOB2) || (tob_type == FTYPE_TOB3)) {
		if (byte_inframe + 4  >= frame_length) {
			/* skip frame footer */
			fread(dumstring, sizeof(char), 4, infile);
	
			/* reset counter */
			byte_inframe=0;
	
			/* skip frame header */
			if (tob_type == FTYPE_TOB2) {
				fread(&dum_int, sizeof(dum_int), 1, infile);
				tobtime = decode_tobtime((long) dum_int);
				fread(&dum_int, sizeof(dum_int), 1, infile);
				byte_inframe+=8;
			}
			if (tob_type == FTYPE_TOB3) {
				fread(&dum_int, sizeof(dum_int), 1, infile);
				tobtime = decode_tobtime((long) dum_int);
				fread(&dum_int, sizeof(dum_int), 1, infile);
				fread(&dum_int, sizeof(dum_int), 1, infile);
				byte_inframe+=12;
			}
	
		}
              }
              /* Processing and reading depends on type of variable */
              switch (coltype[i])  {
                 case TOB_ULONG:
                    nitems = fread(&dum_long, sizeof(dum_long), 1, infile);
		    if (nitems) {
	    	        byte_inframe+=4;
		        if (conv_tob1_time) {
			    if (i==0) tob1time = decode_tobtime((long) dum_long);
		            if (i==1) {
		               subseconds = dum_long;
	              	       printf("%i ", tob1time.tm_year+1900);
		               printf("%i ", daynumber(tob1time.tm_year, tob1time.tm_mon+1, tob1time.tm_mday));
		               printf("%i ", tob1time.tm_hour*100+tob1time.tm_min);
		               printf("%f ", tob1time.tm_sec+subseconds/1e9);
                            }
                        } 
		        if (print_col[i])
		            if (!conv_tob1_time || (i>1)) printf("%i ", (int) dum_long);
	            }
		    break;
                 case TOB_IEEE4:
                    nitems = fread(&dum_float, sizeof(dum_float), 1, infile);
		    if (nitems) {
	  	       byte_inframe+=4;
		       if (print_col[i]) printf("%.*f ", decimal_places, dum_float);
		    }
		    break;
                 case TOB_IEEE4L:
                    nitems = fread(&dum_float, sizeof(dum_float), 1, infile);
		    if (nitems) {
		       if (machine_endian == ENDIAN_BIG) (void) ReverseBytesInArray(&dum_float, sizeof(dum_float));
		       byte_inframe+=4;
		       if (print_col[i]) printf("%.*f ", decimal_places,  dum_float);
		    }
		    break;
                 case TOB_IEEE4B:
                    nitems = fread(&dum_float, sizeof(dum_float), 1, infile);
		    if (nitems) {
		       printf("ieee4b nitems = %i\n", nitems);
		       if (machine_endian == ENDIAN_LITTLE) (void) ReverseBytesInArray(&dum_float, sizeof(dum_float));
		       byte_inframe+=4;
		       if (print_col[i]) printf("%.*f ", decimal_places, dum_float);
		    }
		    break;
                 case TOB_FP2:
                    nitems = fread(two_char, sizeof(two_char[0]), 2, infile);
		    if (nitems) {
		       byte_inframe+=2;
                       dum_float = conv_two_byte(two_char);
		       if (print_col[i]) printf("%.*f ", decimal_places, dum_float);
		    }
		    break;
		 default:
                    break;
              }
	    }
	    if (nitems)  printf(" \n");
           }

	   /* Update time */
	   subseconds += samp_interval;
	   rest = floor(subseconds);
           subseconds -= rest;
           tobtime.tm_sec += rest;

	   rest = tobtime.tm_sec - tobtime.tm_sec%60;
           tobtime.tm_sec  -= rest;
           tobtime.tm_min += rest/60;

           /* Subseconds tends to run away, so we reset it */
	   if (subseconds < 0.001 * samp_interval) subseconds = 0;
        }
}

/* ........................................................................
 * Function : do_conv_toa
 * Purpose  : Rudimentary function to convert data from TOA files (TOA5)
 *            to text on standard output
 *
 * Interface: infile           in   input files
 *            ncid             out  netcdf id of output file
 *            formfile         in   text file describing format
 *            list_line        in   number of lines of input file to
 *                                  list
 *            print_col        in   switch which columns should be printed
 *            toa_type         in   switch for type of TOA file
 *            decimal_places   in   number of decimal places in text output
 * Date     : October 3, 2006
 */
void do_conv_toa(FILE *infile, int ncid, FILE *formfile, int list_line, boolean print_col[MAXCOL], int toa_type, int decimal_places)
{
	char *buffer, *bufferstart, dumstring2[MAX_STRINGLENGTH];
        char dumstring[MAX_STRINGLENGTH], delimiter, *pChSpace;
	int  i, cur_line, nskip, col;
        int year, month, day, hour, min;
        float sec;
        int valid_value;
	double conv_value;


	/* Allocate buffer and retain original start to reset pointer location later */
        buffer  = (char *) malloc(MAX_STRINGLENGTH);
	bufferstart = buffer;

        /* TOA5 is comma separated */
        delimiter = ',';

	/* Skip first 3 lines */
	if (toa_type == FTYPE_TOA5)
		nskip = 3;
        for (i = 0; i<nskip; i++)  {
           fgets(buffer, MAX_STRINGLENGTH, infile);
         }

        /* Get last line of header and determine number of columns and type of
           numbers*/
        fgets(buffer, MAX_STRINGLENGTH, infile);

	// We might implement a check whether this is really TOA
        // typeline_decode(buffer, coltype,  &ncol);

	cur_line = 0;
        col = 0;
	
        /* Loop data */
        while (!feof(infile) && ((cur_line < list_line) || (list_line == -1))) {
           /* Reset start of buffer */
           buffer = bufferstart;

	   /* Get a line */
           fgets(buffer, MAX_STRINGLENGTH, infile);
           cur_line++;
           col = 0;
           /* Get the time stamp */
           while (strchr(buffer, delimiter) != NULL) {
               // We start with a delimiter
               if (buffer[0] == delimiter) {
                   i = 0;
                   while (buffer[i] == delimiter)
                       i++;
                   pChSpace = &buffer[i];
                   strcpy(buffer, pChSpace);
               }
              // There is a delimiter in the line, proceed until found
              if (strchr(buffer, delimiter) != NULL) {
                 i = 0;
                 while (buffer[i] != delimiter)
                    i++;
             // There is no longer a delimiter in the rest of the line, so just find the 
             // part of the line that contains printable characters.
              } else
                 while (isprint(buffer[i])) i++;
              strncpy(dumstring, buffer, i);
              dumstring[i]='\0';

              // Check if this really can be a number; it might be end of line!
              if (isprint(dumstring[i-1])) {
                   if (col==0) {
                       year = -1;
                       month = -1;
                       day = -1;
                       hour = -1 ;
                       min = -1;
                       sec = -1;

		       strncpy(dumstring2,&(dumstring[1]),4);
                       dumstring2[4]='\0';
		       year = atoi(dumstring2);
		       strncpy(dumstring2,&(dumstring[6]),2);
                       dumstring2[2]='\0';
		       month = atoi(dumstring2);
		       strncpy(dumstring2,&(dumstring[9]),2);
                       dumstring2[2]='\0';
		       day = atoi(dumstring2);
		       strncpy(dumstring2,&(dumstring[12]),2);
                       dumstring2[2]='\0';
		       hour = atoi(dumstring2);
		       strncpy(dumstring2,&(dumstring[15]),2);
                       dumstring2[2]='\0';
		       min = atoi(dumstring2);
		       strcpy(dumstring2,&(dumstring[18]));
		       sec = atof(dumstring2);

                       printf("%i %i %04i %f ", year, daynumber(year, month, day),  hour*100+min, sec);
                   }
                   if (col>0 && print_col[col]) {
			   valid_value = sscanf(dumstring, "%lg", &conv_value);
			   if (valid_value)
				   printf("%.*lg ", decimal_places,  conv_value);
			   else
				   printf("NaN ");
	           }
                   buffer = buffer + i ;
                   col++;
              }

           }
	   printf(" \n");

        }
        free(bufferstart);
}
