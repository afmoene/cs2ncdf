/* $Id$ */
#include "netcdf.h"
#include "in_out.h"
#include "error.h"


#define NO_VALUE -9991
#define MAX_SAMPLES 64
#define MAX_LINELENGTH  1024
#define MAX_STRINGLENGTH 1024

/* ........................................................................
 * type declarations
 * ........................................................................
 */
typedef struct {
  int  array_id;
  int  follow_id;
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
  int nc_dim[2];
  float *values;
  float *follow_val;
  size_t index;
  int curr_index;
  size_t first_index;
  int ncol;
} column_def;

typedef struct {
  char *title;
  char *history;
} global_def;

typedef struct {
  char *name;
  int  array_id; 
  int  nc_dim; 
} time_def;


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


void nc_handle_error(int nc_error) {
char mess[MAX_STRINGLENGTH];
   if (nc_error != NC_NOERR) {
      sprintf(mess, "%s\n",nc_strerror(nc_error));
      error(mess, nc_error);
   }
}
/* ........................................................................
 * Function : scan_line
 * Purpose  : Scan one line of the format file
 *
 * Interface: formfile         in   format file
 *            id               out  array id
 *            col              out  column number
 *            unit             out  unit string
 *            long_name        out  long name string
 *            scale_factor     out  scale factor
 *            add_offset       out  offset
 *            valid_min        out  valid minimum value
 *            valid_max        out  valid maximum value
 *            missing_value    out  value signifying a missing value
 *            type             out  storage type
 *            globaldef        out  global definitions
 *            timename         out  time variable name
 *            ncol             out  number of contiguous columns
 *            dimname          out  name for extra dimension
 *            follow_id        out  which array_id to follow
 * Return value: 1 if line was read succesfully, -1 of 0 otherwise
 *
 * Method   : 
 * Date     : June 1999
 * ........................................................................
 */
int scan_line(FILE *formfile, int *id, int *col, 
              char *name, char *unit, char *long_name,
              float *scale_factor, float *add_offset,
              float *valid_min, float *valid_max,
              float *missing_value, int *type,
              global_def *globaldef,
	      char *timename, int *ncol, char *dimname,
	      int *follow_id) {

char *line, *linecopy, *stringp, dumline[MAX_LINELENGTH],
     dumstring[MAX_STRINGLENGTH], mess[2*MAX_LINELENGTH], *slashp;
int  pos, status, i, j;
boolean
     otherline = TRUE,
     id_found = FALSE,
     col_found = FALSE,
     name_found = FALSE,
     unit_found = FALSE,
     long_name_found = FALSE,
     scale_factor_found = FALSE,
     add_offset_found = FALSE,
     valid_min_found = FALSE,
     valid_max_found = FALSE,
     missing_value_found = FALSE,
     time_found = FALSE,
     type_found = FALSE,
     ncol_found = FALSE,
     dimname_found = FALSE,
     follow_id_found = FALSE;

    /* (1) Get one line of text */
    line = (char *) malloc(MAX_LINELENGTH*sizeof(char));
    for (i=0; i<MAX_LINELENGTH; i++) {
       line[i] = '\0';
    }
    while (otherline) {
       if (!fgets(dumline, sizeof(dumline), formfile)) return 0;
       if (!(slashp = strrchr(dumline,'\\')))
          otherline = FALSE;
       linecopy = (char *) malloc(
                    (strlen(line)+strlen(dumline)-1)*sizeof(char));
       strcpy(linecopy, line);
       strncat(linecopy, dumline, (slashp-dumline)/sizeof(char));
       free(line);
       line = (char *) malloc(strlen(linecopy)*sizeof(char));
       strcpy(line, linecopy);
    }


    /* (2) Strip any comments */
    pos = strcspn(line, "//");
    strncpy(linecopy, line, pos);
    if (!linecopy) return 0;

    /* (3) Get info from line */
    /* (3.1) Array ID */
    if ( (stringp = strstr(linecopy, "id")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           status = sscanf(stringp, "=%i", id);
           if (status != 1) 
              error("could not convert array ID", -1);
           else
              id_found = TRUE;
        } else {
              error("could not find = sign for array ID", -1);
        }
    }

    /* (3.2) Column number */
    if ( (stringp = strstr(linecopy, "col")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           status = sscanf(stringp, "=%i", col);
           if (status != 1) 
              error("could not convert column number ", -1);
           else
              col_found = TRUE;
        } else {
              error("could not find = sign for column number", -1);
        }
    }

    /* (3.3) Name */
    if ( (stringp = strstr(linecopy, "var_name")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           if (!strcpy(name, quoted_string(stringp))) 
              error("could not convert name", -1);
           else
              name_found = TRUE;
        } else {
              error("could not find = sign for name", -1);
        }
    }

    /* (3.4) Units attribute */
    if ( (stringp = strstr(linecopy, "units")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           if (!strcpy(unit, quoted_string(stringp))) 
              error("could not convert units", -1);
           else
              unit_found = TRUE;
        } else {
              error("could not find = sign for units", -1);
        }
    }

    /* (3.5)  Long name attribute */
    if ( (stringp = strstr(linecopy, "long_name")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           if (!strcpy(long_name, quoted_string(stringp))) 
              error("could not convert long_name ", -1);
           else 
              long_name_found = TRUE;
        } else {
              error("could not find = sign for long_name", -1);
        }
    }

    /* (3.6)  Scale factor attribute */
    if ( (stringp = strstr(linecopy, "scale_factor")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           status = sscanf(stringp, "= %s", dumstring);
           *scale_factor = atof(dumstring);
           if (status != 1) 
              error("could not convert scale_factor", -1);
           else
              scale_factor_found = TRUE;
        } else {
              error("could not find = sign for scale_factor", -1);
        }
    }

    /* (3.7)  Offset attribute */
    if ( (stringp = strstr(linecopy, "add_offset")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           status = sscanf(stringp, "= %s", dumstring);
           *add_offset = atof(dumstring);
           if (status != 1) 
              error("could not convert offset", -1);
           else
              add_offset_found = TRUE;
        } else {
              error("could not find = sign for offset", -1);
        }
    }

    /* (3.8)  Valid_min attribute */
    if ( (stringp = strstr(linecopy, "valid_min")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           status = sscanf(stringp, "= %s", dumstring);
           *valid_min = atof(dumstring);
           if (status != 1) 
              error("could not convert valid_min", -1);
           else
              valid_min_found = TRUE;
        } else {
              error("could not find = sign for valid_min", -1);
        }
    }

    /* (3.9)  Valid_max attribute */
    if ( (stringp = strstr(linecopy, "valid_max")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           status = sscanf(stringp, "= %s", dumstring);
           *valid_max = atof(dumstring);
           if (status != 1) 
              error("could not convert valid_max", -1);
           else
              valid_max_found = TRUE;
        } else {
              error("could not find = sign for valid_max", -1);
        }
    }

    /* (3.10)  Missing_value attribute */
    if ( (stringp = strstr(linecopy, "missing_value")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           status = sscanf(stringp, "= %s", dumstring);
           *missing_value = atof(dumstring);
           if (status != 1) 
              error("could not convert missing_value", -1);
           else
              missing_value_found = TRUE;
        } else {
              error("could not find = sign for missing_value", -1);
        }
    }

    /* (3.11)  Variable type */
    if ( (stringp = strstr(linecopy, "type")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           status = sscanf(stringp, "= %s", dumstring);
           if (!strcpy(dumstring, quoted_string(stringp))) 
              error("could not convert type", -1);
           else
              type_found= TRUE;
	   if (!strcmp(dumstring, "float"))
	       *type = NC_FLOAT;
	   else if (!strcmp(dumstring, "int"))
	       *type = NC_INT;
	   else if (!strcmp(dumstring, "short"))
	       *type = NC_SHORT;
	   else if (!strcmp(dumstring, "double"))
	       *type = NC_DOUBLE;
	   else if (!strcmp(dumstring, "char"))
	       *type = NC_CHAR;
	   else if (!strcmp(dumstring, "byte"))
	       *type = NC_BYTE;
	   else {
	       error("unkown type",-1);
	   }
        } else {
              error("could not find = sign for type", -1);
        }
    }

    /* (3.12) Number of columns */
    if ( (stringp = strstr(linecopy, "ncol")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           status = sscanf(stringp, "=%i", ncol);
           if (status != 1) 
              error("could not convert number of columns", -1);
           else
              ncol_found = TRUE;
        } else {
              error("could not find = sign for number of columns", -1);
        }
    }

    /* (3.13)  Name of extra dimension */
    if ( (stringp = strstr(linecopy, "dim_name")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           if (!strcpy(dimname, quoted_string(stringp)))
              error("could not convert name of extra dimension ", -1);
           else 
              dimname_found = TRUE;
        } else {
              error("could not find = sign for name of extra dimension", -1);
        }
    }

    /* (3.14) Follow array ID */
    if ( (stringp = strstr(linecopy, "follow_id")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           status = sscanf(stringp, "=%i", follow_id);
           if (status != 1) 
              error("could not convert follow array ID", -1);
           else
              follow_id_found = TRUE;
        } else {
              error("could not find = sign for follow array ID", -1);
        }
    }


    /* (4)  Global attributes */
    /* (4.1)  Title attribute */
    if ( (stringp = strstr(linecopy, "title")) )  {
        if ( (stringp = strstr(stringp, "=")) ) {
           (*globaldef).title =  
                   (char *) malloc(strlen(quoted_string(stringp)));
           strcpy((*globaldef).title, quoted_string(stringp));
        } else {
              error("could not find = sign for title", -1);
        }
    }

    /* (4.2)  History attribute */
    if ( (stringp = strstr(linecopy, "history")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           (*globaldef).history =  
                   (char *) malloc(strlen(quoted_string(stringp)));
           strcpy((*globaldef).history, quoted_string(stringp));
        } else {
              error("could not find = sign for title", -1);
        }
    }

    /* (5)  Time coordinate */
    if ( (stringp = strstr(linecopy, "timevar")) ) {
        if ( (stringp = strstr(stringp, "=")) ) {
           if (!strcpy(timename, quoted_string(stringp))) 
              error("could not convert timevar ", -1);
	   else
	      time_found = TRUE;
        } else {
              error("could not find = sign for name of time coordinate", -1);
        }
    }

     if (ncol_found && !dimname_found) {
        error("number of columns defined, but no name for extra dimension",-1);
     }

     if (follow_id_found && !missing_value_found) {
        error("when follow_id defined, also missing_value should be defined",-1);
     }

     if (id_found || col_found || name_found || unit_found) {
        if ((id_found && col_found && name_found && unit_found) ||
	    (id_found && time_found))
           return 1;
        else {
           sprintf(mess, "incomplete line in format file:\n %s\n", linecopy);
           error(mess, -1);
           return 0;
        }
     }

     return 0;
}
/* ........................................................................
 * Function : def_nc_file
 * Purpose  : To read definition of the netcdf file
 *            from the format file and defined the netcdf file 
 *
 * Interface: ncid             in     netcdf id of output file
 *            formfile         in     text file describing format
 *            coldef           in/out array of column definitions
 *            numcoldef        in/out number of column definitions
 *            maxnumcoldef     in     length of coldef array
 *
 * Method   : 
 * Date     : June 1999
 * ........................................................................
 */
void def_nc_file(int ncid, FILE *formfile, 
                 column_def *coldef, int *numcoldef, int maxnumcoldef) {
int 
    status,
    i, j,arrayid, column, numtimedef, type, dimid, ncol, 
    newdim, ndims, follow_id, search_id;
char
    name[MAX_STRINGLENGTH], unit[MAX_STRINGLENGTH], long_name[MAX_STRINGLENGTH],
    timename[MAX_STRINGLENGTH], mess[MAX_STRINGLENGTH], dimname[MAX_STRINGLENGTH];
float
    scale_factor, add_offset, valid_min, valid_max, missing_value;
global_def
    globaldef;
time_def
    timedef[100];
boolean
    found_timedim;



    /* (1) Initialize */
    *numcoldef = 0;
    numtimedef = 0;

    /* (2) Loop format file */
    while (!feof(formfile)) {
       /* (2.1) Initialize */
       *long_name = '\0';
       *timename = '\0';
       scale_factor = NO_VALUE;
       add_offset = NO_VALUE;
       valid_min = NO_VALUE;
       valid_max = NO_VALUE;
       missing_value = NO_VALUE;
       type = NC_FLOAT;
       ncol = 1;
       follow_id = -1;
       
       /* (2.2) Get info from a line */
       if (scan_line(formfile, &arrayid, &column, name, unit, 
                     long_name, &scale_factor, &add_offset,
                     &valid_min, &valid_max, &missing_value, &type,
                     &globaldef, timename, &ncol, dimname,
		     &follow_id)) {

	  /* 2.2.1) This is a time coordinate definition */
	  if (strlen(timename)) {
              /* Check whether a time coordinate was defined already for
               * this Array ID */
	      for (i=0; i<numtimedef; i++) {
	          if (timedef[i].array_id ==arrayid) {
		     sprintf(mess,"already have a time coordimate for array ID %i\n", arrayid);
		     error(mess,-1);
		  }
	      }

              /* Fill definition of time dimension, except for its
               * dimension ID */
              timedef[numtimedef].array_id = arrayid;
              timedef[numtimedef].name = 
               (char *)malloc(strlen(timename)*sizeof(char));
              strcpy((timedef[numtimedef].name), timename);

              /* Check whether a time coordinate of this name exists
               * already and possibly get its dimension ID */
              status = nc_inq_dimid(ncid, timename, &dimid);
              if (status == NC_NOERR)
                 timedef[numtimedef].nc_dim = dimid;
              else {
                 status = nc_def_dim(ncid, timedef[numtimedef].name,
                       NC_UNLIMITED, &(timedef[numtimedef].nc_dim));
                 if (status != NC_NOERR)
                    nc_handle_error(status);
              }
	      numtimedef++;
	  } else {

	  /* (2.2.2) This is a variable definition */
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
	      (*(coldef+*numcoldef)).nc_type = type;
	      /* Define time dimension */
              if (follow_id == -1)
                 search_id = arrayid;
              else
                 search_id = follow_id;
              found_timedim = FALSE;
              for (i=0; i < numtimedef; i++) {
                 if (timedef[i].array_id == search_id) {
                    (*(coldef+*numcoldef)).nc_dim[0] =
                       timedef[i].nc_dim;
                    found_timedim = TRUE;
                 }
              }
              if (!found_timedim) {
                 sprintf(mess,"could not find time dimension for arrayID %i\n", search_id);
                 error(mess,-1);
              }

              /* number of columns */
              (*(coldef+*numcoldef)).ncol = ncol;
              if (ncol > 1) {
                 status = nc_def_dim(ncid, dimname,
                                     (size_t) ncol, &newdim);
                 (*(coldef+*numcoldef)).nc_dim[1] = newdim;
                 if (status != NC_NOERR)
                    nc_handle_error(status);
              }

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
	      /* Make room for floats (conversions handled by netcdf */
              (*(coldef+*numcoldef)).values = 
	           (float *) malloc(sizeof(float)*MAX_SAMPLES*
	                     (*(coldef+*numcoldef)).ncol);
	      /* Array Id to follow (faster changing) */
              (*(coldef+*numcoldef)).follow_id = follow_id;
	      
              /* Make room for values that are kept for variables
               * that follow other array ID */
	      if (follow_id != -1) {
                (*(coldef+*numcoldef)).follow_val = 
	           (float *) malloc(sizeof(float)*
	                     (*(coldef+*numcoldef)).ncol);
		for (j=0; j<ncol; j++) {
		   (*(coldef+*numcoldef)).follow_val[j] = 
		    missing_value;
		}
	      }
	      /* Initialize counters */
	      (*(coldef+*numcoldef)).index = 0;
	      (*(coldef+*numcoldef)).curr_index = 0;
	      (*(coldef+*numcoldef)).first_index = 0;

	      (*numcoldef)++;
          }
       }
    }
    
    /* (3) Define variables and attributes */
    for (i=0; i<*numcoldef; i++) {
      /* (3.1) Variable */
      if ((*(coldef+i)).ncol > 1)
          ndims=2;
      else
          ndims=1;
      status = nc_def_var(ncid, (*(coldef+i)).name, 
                                (*(coldef+i)).nc_type, ndims,
                                (*(coldef+i)).nc_dim,
                                &(*(coldef+i)).nc_var) ;
      if (status != NC_NOERR) 
          nc_handle_error(status);

      /* (3.2) Units attribute */
      status = nc_put_att_text(ncid, (*(coldef+i)).nc_var, "units",
                               strlen((*(coldef+i)).unit), 
                               (*(coldef+i)).unit);
      if (status != NC_NOERR) 
          nc_handle_error(status);

      /* (3.3) Long_name attribute */
      if (strlen((*(coldef+i)).long_name)) {
         status = nc_put_att_text(ncid, (*(coldef+i)).nc_var, 
                                  "long_name",
                                  strlen((*(coldef+i)).long_name), 
                                  (*(coldef+i)).long_name);
         if (status != NC_NOERR) 
            nc_handle_error(status);
      }

      /* (3.4) Scale_factor attribute */
      if ((*(coldef+i)).scale_factor != NO_VALUE) {
         status = nc_put_att_float(ncid, (*(coldef+i)).nc_var, 
                                   "scale_factor", NC_FLOAT, 1,
                                   &((*(coldef+i)).scale_factor));
         if (status != NC_NOERR) 
            nc_handle_error(status);
      }

      /* (3.5) Add_offset attribute */
      if ((*(coldef+i)).add_offset != NO_VALUE) {
         status = nc_put_att_float(ncid, (*(coldef+i)).nc_var, 
                                   "add_offset", NC_FLOAT, 1,
                                   &((*(coldef+i)).add_offset));
         if (status != NC_NOERR) 
            nc_handle_error(status);
      }

      /* (3.6) Valid_min attribute */
      if ((*(coldef+i)).valid_min != NO_VALUE) {
         status = nc_put_att_float(ncid, (*(coldef+i)).nc_var, 
                                   "valid_min", NC_FLOAT, 1,
                                   &((*(coldef+i)).valid_min));
         if (status != NC_NOERR) 
            nc_handle_error(status);
      }

      /* (3.7) Valid_max attribute */
      if ((*(coldef+i)).valid_max != NO_VALUE) {
         status = nc_put_att_float(ncid, (*(coldef+i)).nc_var, 
                                   "valid_max", NC_FLOAT, 1,
                                   &((*(coldef+i)).valid_max));
         if (status != NC_NOERR) 
            nc_handle_error(status);
      }

      /* (3.8) Missing_value attribute */
      if ((*(coldef+i)).missing_value != NO_VALUE) {
         status = nc_put_att_float(ncid, (*(coldef+i)).nc_var, 
                                   "missing_value", NC_FLOAT, 1,
                                   &((*(coldef+i)).missing_value));
         if (status != NC_NOERR) 
            nc_handle_error(status);
      }
    }

    /* (4) Global attributes */
    /* (4.1) Title attribute */
    if (strlen(globaldef.title)) {
         status = nc_put_att_text(ncid, NC_GLOBAL, 
                                  "title",
                                  strlen(globaldef.title), 
                                  globaldef.title);
         if (status != NC_NOERR) 
            nc_handle_error(status);
    }

    /* (4.2) History attribute */
    if (strlen(globaldef.history)) {
         status = nc_put_att_text(ncid, NC_GLOBAL, 
                                  "history",
                                  strlen(globaldef.history), 
                                  globaldef.history);
         if (status != NC_NOERR) 
            nc_handle_error(status);
    }

    /* (5) End of definition */
    status = nc_enddef(ncid);
    if (status != NC_NOERR) 
        nc_handle_error(status);
}


