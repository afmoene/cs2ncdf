/* $Id$ */
#include "netcdf.h"
#include "in_out.h"
#include "error.h"


#define NO_VALUE -9991
#define MAX_SAMPLES 64
#define MAX_LINELENGTH  1024
#define MAX_STRINGLENGTH 20000

/* ........................................................................
 * type declarations
 * ........................................................................
 */
typedef struct {
  float scale_factor;
  float add_offset;
  float valid_min;
  float valid_max;
  float missing_value;
  float time_mult;
  float time_offset;
  float *values;
  float *follow_val;
  int  array_id;
  int  follow_id;
  int  col_num;
  int nc_var;
  int nc_type;
  int nc_dim[2];
  int curr_index;
  size_t index;
  size_t first_index;
  int ncol;
  int time_colnum;
  int time_num_comp;
  int time_got_comp;
  char *name;
  char *long_name;
  char *unit;
  boolean time_comp;
  boolean time_csi_hm;
  boolean i_am_time;
  boolean got_follow_val;
  boolean got_val; /* needed to handle incomplete lines */
} column_def;

typedef struct {
  char *title;
  char *history;
  char *remark;
} global_def;

typedef struct {
  char *name;
  int  array_id; 
  int  nc_dim; 
  int time_colnum;
  char *unit;
  char *long_name;
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

char *non_space(char *string, const char *word, const char letter) {
   int i;

   for (i = strlen(word); i <strlen(string); i++) {
     if (!isspace(string[i])) {
         if (string[i] == letter)
            return (string+i+1);
         else
            return NULL;
      }
   }
   return NULL;
}

void nc_handle_error(int nc_error) {
char mess[MAX_STRINGLENGTH];
   if (nc_error != NC_NOERR) {
      sprintf(mess, "NetCDF error: %s\n",nc_strerror(nc_error));
      error(mess, nc_error);
   }
}

boolean get_int(char *line, const char *token, const char *name, int *value) {

  char *stringp;
  char mess[MAX_STRINGLENGTH];
  int  status;

  if ( (stringp = strstr(line, token)) ) {
      if ( (stringp = non_space(stringp, token, '=')) ) {
           status = sscanf(stringp, "%i", value);
           if (status != 1) {
              sprintf(mess, "could not convert %s\n", name);
              error(mess, -1);
              return FALSE;
	   } else
              return TRUE;
      }
  }
  return FALSE;
}

boolean get_float(char *line, const char *token, const char *name, float *value) {

  char *stringp;
  char mess[MAX_STRINGLENGTH];
  char dumstring[MAX_STRINGLENGTH];
  int  status;

  if ( (stringp = strstr(line, token)) ) {
        if ( (stringp = non_space(stringp, token, '=')) ) {
           status = sscanf(stringp, "%s", dumstring);
           *value = atof(dumstring);
           if (status != 1) {
              sprintf(mess, "could not convert %s\n", name);
              error(mess, -1);
              return FALSE;
           } else
              return TRUE;
        }
  }
  return FALSE;
}



boolean get_string(char *line, const char *token, 
                   const char *name, char *value) {
   char *stringp;
   char mess[MAX_STRINGLENGTH];

   if ( (stringp = strstr(line, token)) ) {
       if ( (stringp = non_space(stringp, token, '=')) ) {
          if (!strcpy(value, quoted_string(stringp))) {
	      sprintf(mess, "could not convert %s\n", name);
              error(mess, -1);
	      return FALSE;
	  } else
              return TRUE;
       }
   }
   return FALSE;
}

char *get_clearstring(num) {
   char *stringp;
   int   i;

   stringp = (char *) malloc((num+1)*sizeof(char));
   for (i=0; i<num+1;i++)
      stringp[i] = '\0';
   return stringp;
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
 *            time_comp        out  is this part of te time definition
 *            time_cs_hm       out  is this the Campbell hour/minutes time?
 *            time_mult        out  multiplier to apply before adding to time column
 *            time_offset      out  offset to apply before multiplier
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
	      int *follow_id,
	      boolean *time_comp, boolean *time_csi_hm,
	      float *time_mult, float *time_offset) {

char *line = NULL,
     *linecopy = NULL,
     dumline[MAX_LINELENGTH],
     dumstring[MAX_STRINGLENGTH], mess[2*MAX_LINELENGTH],
     *slashp = NULL;
int  pos, dum_int;
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
     follow_id_found = FALSE,
     time_mult_found = FALSE,
     time_offset_found = FALSE,
     time_csi_hm_found = FALSE;

    /* (1) Get one line of text */
    line = get_clearstring(MAX_LINELENGTH);
    while (otherline) {
       if (!fgets(dumline, sizeof(dumline), formfile)) return 0;
       if (!(slashp = strrchr(dumline,'\\'))) {
          otherline = FALSE;
          linecopy = get_clearstring(strlen(line)+strlen(dumline));
          strcpy(linecopy, line);
  	       free(line);
          strcat(linecopy, dumline);
       }
       else {
          linecopy = get_clearstring(strlen(line)+strlen(dumline));
          strcpy(linecopy, line);
	  free(line);
          strncat(linecopy, dumline, (slashp-dumline)/sizeof(char));
       }
       line = get_clearstring(strlen(linecopy));
       strcpy(line, linecopy);
    }


    /* (2) Strip any comments */
    if (strstr(line, "//")) {
       pos = (int) (line-strstr(line, "//"));
       if (!pos) {
       /* line should be freed, but get SIGABORT so, just leave it */
       /* free(line);*/
          return 0;
       }
    } else
       pos = strlen(line);
    linecopy[0]='\0';
    strncpy(linecopy, line, (size_t) pos+1);

    /* line should be freed, but get SIGABORT so, just leave it */
    free(line);
    if (!strlen(linecopy)) return 0;

    /* (3) Get info from line */
    /* (3.1) Array ID */
    id_found = get_int(linecopy, "id", "array ID", id);

    /* (3.2) Column number */
    col_found = get_int(linecopy, "col_num", "column number", col);

    /* (3.3) Name */
    name_found = get_string(linecopy, "var_name", "name", name);

    /* (3.4) Units attribute */
    unit_found = get_string(linecopy, "units", "units", unit);

    /* (3.5)  Long name attribute */
    long_name_found = get_string(linecopy, "long_name","long_name", long_name);

    /* (3.6)  Scale factor attribute */
    scale_factor_found = get_float(linecopy, "scale_factor", 
                                   "scale_factor", scale_factor);

    /* (3.7)  Offset attribute */
    add_offset_found = get_float(linecopy, "add_offset", 
                                   "offset", add_offset);

    /* (3.8)  Valid_min attribute */
    valid_min_found = get_float(linecopy, "valid_min", 
                                   "valid_min", valid_min);

    /* (3.9)  Valid_max attribute */
    valid_max_found = get_float(linecopy, "valid_max", 
                                   "valid_max", valid_max);

    /* (3.10)  Missing_value attribute */
    missing_value_found = get_float(linecopy, "missing_value", 
                                   "missing_value", missing_value);

    /* (3.11)  Variable type */
    type_found = get_string(linecopy, "type", "type", dumstring);
    if (type_found) {
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
	    type_found = FALSE;
            error("unkown type",-1);
	}
    }
    
    /* (3.12) Number of columns */
    ncol_found = get_int(linecopy, "ncol", "number of columns", ncol);

    /* (3.13)  Name of extra dimension */
    dimname_found = get_string(linecopy, "dim_name", "name of extra dimension", 
                    dimname);

    /* (3.14) Follow array ID */
    follow_id_found = get_int(linecopy, "follow_id", "follow array ID", 
                              follow_id);
    
    /* (3.15) Time multiplier */
    time_mult_found = get_float(linecopy, "time_mult", "time_mult", time_mult);
    if (time_mult_found) 
        *time_comp = TRUE;

    /* (3.16) Time offset */
    time_offset_found = get_float(linecopy, "time_offset", "time_offset", 
                                  time_offset);

    /* (3.17)  CSI hour/minutes ? */
    time_csi_hm_found = get_int(linecopy, "time_csi_hm", "time_csi_hm", 
                                  &dum_int);
    if (time_csi_hm_found && (dum_int = 1)) 
         *time_csi_hm = TRUE;
    

    /* (4)  Global attributes */
    /* (4.1)  Title attribute */
    if (get_string(linecopy, "title", "title", dumstring)) {
           (*globaldef).title =  
                   (char *) malloc(strlen(dumstring));
           strcpy((*globaldef).title, dumstring);
    }

    /* (4.2)  History attribute */
    if (get_string(linecopy, "history", "history", dumstring)) {
           (*globaldef).history =  
                   (char *) malloc(strlen(dumstring));
           strcpy((*globaldef).history, dumstring);
    }

    /* (4.3)  Remark attribute */
    if (get_string(linecopy, "remark", "remark", dumstring)) {
           (*globaldef).remark =  
                   (char *) malloc(strlen(dumstring));
           strcpy((*globaldef).remark, dumstring);
    }

    
    /* (5)  Time coordinate */
    time_found = get_string(linecopy, "timevar", "timevar", timename);

     if (ncol_found && !dimname_found) {
        error("number of columns defined, but no name for extra dimension",-1);
     }

     if (follow_id_found && !missing_value_found) {
        error("when follow_id defined, also missing_value should be defined",-1);
     }
     
     if (time_offset_found && !time_mult_found)  {
         error("does not make sense to define time_offset, when not defining time_mult",-1);
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
                 column_def coldef[MAXCOL], int *numcoldef, int maxnumcoldef) {
int 
    status,
    i, j,arrayid, column, type, dimid, ncol, 
    newdim, follow_id, search_id, num_time_comp, numdims;
size_t ndims, get_len;
char
    name[MAX_STRINGLENGTH], unit[MAX_STRINGLENGTH], long_name[MAX_STRINGLENGTH],
    timename[MAX_STRINGLENGTH], mess[MAX_STRINGLENGTH], dimname[MAX_STRINGLENGTH],
    get_name[MAX_STRINGLENGTH];
float
    scale_factor, add_offset, valid_min, valid_max, missing_value,
    time_mult, time_offset;
global_def
    globaldef;
time_def
    timedef;
boolean
    found_timedim, time_comp, time_csi_hm, i_am_time;



    /* (1) Initialize */
    *numcoldef = 0;
    timedef.array_id = -1;
    num_time_comp = 0;
    globaldef.title = NULL;
    globaldef.history = NULL;
    globaldef.remark = NULL;

    /* (2) Loop format file */
    while (!feof(formfile)) {
       /* (2.1) Initialize */
       *long_name = '\0';
       *timename = '\0';
       *unit = '\0';
       scale_factor = NO_VALUE;
       add_offset = NO_VALUE;
       valid_min = NO_VALUE;
       valid_max = NO_VALUE;
       missing_value = NO_VALUE;
       type = NC_FLOAT;
       ncol = 1;
       follow_id = -1;
       time_comp = FALSE;
       time_csi_hm = FALSE;
       time_mult = NO_VALUE;
       time_offset = 0.0;
       i_am_time = FALSE;
       
       /* (2.2) Get info from a line */
       if (scan_line(formfile, &arrayid, &column, name, unit, 
                     long_name, &scale_factor, &add_offset,
                     &valid_min, &valid_max, &missing_value, &type,
                     &globaldef, timename, &ncol, dimname,
          		      &follow_id, &time_comp, &time_csi_hm,
		               &time_mult, &time_offset)) {

	  /* 2.2.1) This is a time coordinate definition */
	  if (strlen(timename)) {
              /* Check whether a time coordinate was defined already for
               * this Array ID */
	      if (timedef.array_id > 0) {
		     sprintf(mess,"already have a time coordimate for array ID %i\n", arrayid);
		     error(mess,-1);
	      }

              /* Fill definition of time dimension, except for its
               * dimension ID */
              timedef.array_id = arrayid;
              if (strlen(timename)) {
                 timedef.name =
                   (char *)malloc(strlen(timename)*sizeof(char));
                 strcpy((timedef.name), timename);
              } else
                 timedef.name = NULL;

              /* Check whether a time coordinate of this name exists
               * already and possibly get its dimension ID */
              status = nc_inq_dimid(ncid, timename, &dimid);
              if (status == NC_NOERR)
                 timedef.nc_dim = dimid;
              else {
                 status = nc_def_dim(ncid, timedef.name,
                       (size_t) NC_UNLIMITED, &(timedef.nc_dim));
                 if (status != NC_NOERR)
                    nc_handle_error(status);
              }
	      
	      /* May have a unit attribute, will be passed to
	       * a variable with name timedef.name, if we are told
	       * how to fill that variable */
              if (strlen(unit)) {
                timedef.unit =
                  (char *)malloc(strlen(unit)*sizeof(char));
                strcpy((timedef.unit), unit);
              } else
                timedef.unit = NULL;

	      /* Long name idem*/
              if (strlen(long_name)) {
       	         timedef.long_name =
     		     (char *)malloc(strlen(long_name)*sizeof(char));
     	         strcpy(timedef.long_name, long_name);
              } else
                timedef.long_name = NULL;
              
	  } else {

	  /* (2.2.2) This is a variable definition */
	      /* Define array ID */
	      (*(coldef+*numcoldef)).array_id = arrayid;
	      /* Define column number */
	      (*(coldef+*numcoldef)).col_num = column;
	      /* Define name */
              if (strlen(name)) {
  	         (*(coldef+*numcoldef)).name =
                    (char *)malloc(strlen(name)*sizeof(char));
	         strcpy((*(coldef+*numcoldef)).name, name);
              } else
                 (*(coldef+*numcoldef)).name = NULL;
	      /* Define units */
              if (strlen(unit)) {
	         (*(coldef+*numcoldef)).unit =
                    (char *)malloc(strlen(unit)*sizeof(char));
	         strcpy((*(coldef+*numcoldef)).unit, unit);
              } else
                 (*(coldef+*numcoldef)).unit = NULL;
	      /* Define netcdf type */
	      (*(coldef+*numcoldef)).nc_type = type;
	      /* Define time dimension */
              if (follow_id == -1)
                 search_id = arrayid;
              else
                 search_id = follow_id;
              found_timedim = FALSE;
              if (timedef.array_id == search_id) {
                  (*(coldef+*numcoldef)).nc_dim[0] =
                      timedef.nc_dim;
                    found_timedim = TRUE;
              }
              if (!found_timedim) {
                 sprintf(mess,"could not find time dimension for arrayID %i\n", search_id);
                 error(mess,-1);
              }

              /* number of columns */
              (*(coldef+*numcoldef)).ncol = ncol;
              if (ncol > 1) {
                 /* First check whether this dimensions exists already */
                 status = nc_inq_ndims(ncid, &numdims);
                 if (status != NC_NOERR)
                    nc_handle_error(status);
                 newdim = -1;
                 for (i = 0; i < numdims; i++) {
                    status = nc_inq_dim(ncid, i, get_name, &get_len);
                    if (status != NC_NOERR)
                       nc_handle_error(status);
                    if (!strcmp(get_name, dimname)) {
                       if (get_len == ncol)
                          newdim = i;
                       else {
                          sprintf(mess, "already have dimension with name %s, but has length %i instead of %i\n",
                                  dimname, (int) get_len, ncol);
                          error(mess,-1);
                       }
                    }
                 }
                 if (newdim == -1) {
                    status = nc_def_dim(ncid, dimname,
                                        (size_t) ncol, &newdim);
                    if (status != NC_NOERR)
                       nc_handle_error(status);
                 }
                 (*(coldef+*numcoldef)).nc_dim[1] = newdim;
              }

	      /* Long name */
              if (strlen(long_name)) {
	         (*(coldef+*numcoldef)).long_name =
		     (char *)malloc(strlen(long_name)*sizeof(char));
	         strcpy((*(coldef+*numcoldef)).long_name, long_name);
              } else
                 (*(coldef+*numcoldef)).long_name = NULL;

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
         if (follow_id > 0)
             (*(coldef+*numcoldef)).got_follow_val = FALSE;
	      
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
	      
	      /* Is time component ?*/
	      (*(coldef+*numcoldef)).time_comp = time_comp;
	      if (time_comp) num_time_comp++;

	      /* Is CSI hour/min ?*/
	      (*(coldef+*numcoldef)).time_csi_hm = time_csi_hm;

	      /* Time multiplier */
	      (*(coldef+*numcoldef)).time_mult = time_mult;

	      /* Time offset */
	      (*(coldef+*numcoldef)).time_offset = time_offset;
	      
	      /* I am time ?*/
	      (*(coldef+*numcoldef)).i_am_time = FALSE;

         /* Did we get a value yet */
 	      (*(coldef+*numcoldef)).got_val = FALSE;

	      (*numcoldef)++;
          }
       }
    }
    
    if (num_time_comp) {
      /* Define name */
      if (strlen(name)) {
         (*(coldef+*numcoldef)).name =
	   (char *)malloc(strlen(timedef.name)*sizeof(char));
         strcpy((*(coldef+*numcoldef)).name, timedef.name);
      } else
        (*(coldef+*numcoldef)).name = NULL;
      /* Define units */
      if (strlen(unit)) {
         (*(coldef+*numcoldef)).unit =
  	    (char *)malloc(strlen(timedef.unit)*sizeof(char));
         strcpy((*(coldef+*numcoldef)).unit, timedef.unit);
      } else
         (*(coldef+*numcoldef)).unit = NULL;
      /* Define long_name */
      if (strlen(long_name)) {
         (*(coldef+*numcoldef)).long_name =
	    (char *)malloc(strlen(timedef.long_name)*sizeof(char));
         strcpy((*(coldef+*numcoldef)).long_name, timedef.long_name);
      } else
         (*(coldef+*numcoldef)).long_name = NULL;
      /* Define netcdf type */
      (*(coldef+*numcoldef)).nc_type = NC_FLOAT;
      /* Define time dimension */
      (*(coldef+*numcoldef)).nc_dim[0] = timedef.nc_dim;
      /* Define number of columns */
      (*(coldef+*numcoldef)).ncol = 1;

      /* Make room for floats (conversions handled by netcdf */
      (*(coldef+*numcoldef)).values = 
           (float *) malloc(sizeof(float)*MAX_SAMPLES*
                     (*(coldef+*numcoldef)).ncol);
      
      /* Scale factor */
      (*(coldef+*numcoldef)).scale_factor = NO_VALUE;
      /* Offset */
      (*(coldef+*numcoldef)).add_offset = NO_VALUE;
      /* Valid minimum value */
      (*(coldef+*numcoldef)).valid_min = NO_VALUE;
      /* Valid maximum value */
      (*(coldef+*numcoldef)).valid_max = NO_VALUE;
      /* Missing value */
      (*(coldef+*numcoldef)).missing_value = NO_VALUE;
      /* I am time !! */
      (*(coldef+*numcoldef)).i_am_time = TRUE;
      /* I am time !! */
      (*(coldef+*numcoldef)).time_num_comp = num_time_comp;
      /* Initialize counters */
      (*(coldef+*numcoldef)).index = 0;
      (*(coldef+*numcoldef)).curr_index = 0;
      (*(coldef+*numcoldef)).first_index = 0;
      (*(coldef+*numcoldef)).time_got_comp = 0;
      /* Did we get a value yet */
      (*(coldef+*numcoldef)).got_val = FALSE;
      
      /* Tell all time components about the current column number */
      for (i=0; i<*numcoldef; i++) {
         if ((*(coldef+i)).time_comp)
	    (*(coldef+i)).time_colnum = *numcoldef;
      }
      (*numcoldef)++;
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
      if ((*(coldef+i)).unit != NULL) {
        status = nc_put_att_text(ncid, (*(coldef+i)).nc_var, "units",
                               strlen((*(coldef+i)).unit), 
                               (*(coldef+i)).unit);
        if (status != NC_NOERR) 
            nc_handle_error(status);
      }
      

      /* (3.3) Long_name attribute */
      if ((*(coldef+i)).long_name != NULL) {
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
      
      /* Is a time component ? */
      if ((*(coldef+i)).time_comp) num_time_comp++;
    }


    /* (4) Global attributes */
    /* (4.1) Title attribute */
    if (globaldef.title != NULL) {
         status = nc_put_att_text(ncid, NC_GLOBAL, 
                                  "title",
                                  strlen(globaldef.title), 
                                  globaldef.title);
         if (status != NC_NOERR) 
            nc_handle_error(status);
    }

    /* (4.2) History attribute */
    if (globaldef.history != NULL) {
         status = nc_put_att_text(ncid, NC_GLOBAL, 
                                  "history",
                                  strlen(globaldef.history), 
                                  globaldef.history);
         if (status != NC_NOERR) 
            nc_handle_error(status);
    }

    /* (4.3) Remark attribute */
    if (globaldef.remark != NULL) {
         status = nc_put_att_text(ncid, NC_GLOBAL, 
                                  "remark",
                                  strlen(globaldef.remark), 
                                  globaldef.remark);
         if (status != NC_NOERR) 
            nc_handle_error(status);
    }


    /* (5) End of definition */
    status = nc_enddef(ncid);
    if (status != NC_NOERR) 
        nc_handle_error(status);
}


