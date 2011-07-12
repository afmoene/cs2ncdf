/* $Id$ */
/* 
 * csicond.h: set of routines to handle conditions 
 *
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
 */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


#include "error.h"

#define COND_MAXCOL 100

#define EQUAL 0
#define GT 1
#define LT 2
#define GE 3
#define LE 5
#define NE 7
#define N_COND 8

#define COND_AND 0
#define COND_OR 1
#define MAXCOND 100
#define MAXSUBCOND 100

const char* conditions[N_COND] = {"==",">","<",">=","=>","<=","=<","!="};


typedef struct {
   char  *cond_text;
   int   cond_type;
   double cond_number;
   int   cond_col;
   int   cond_arrayid;
   boolean status;
} cond_def;

typedef struct {
   char      *cond_text;
   cond_def  subcond[MAXSUBCOND];
   int       subcondrel[MAXSUBCOND-1];
   int       n_subcond;
   boolean   status;
} maincond_def;

int column_cond[COND_MAXCOL];


void parse_cond(cond_def *thiscond)
{
   char *pos1, *pos2, *pos3 = NULL, *dumpos;
   int i;
   char *colstring;

   /* Array ID */
   pos1 = strstr((*thiscond).cond_text,"a");
   if (!pos1)
      pos1 = strstr((*thiscond).cond_text,"A");
   if (!pos1) {
      printf("We got condition %s\n",(*thiscond).cond_text);
      error("No arrayid indicator (a or A) found in condition",-1);
   }

   /* Find column number */
   pos2 = strstr((*thiscond).cond_text,"c");
   if (!pos2)
      pos2 = strstr((*thiscond).cond_text,"C");
   if (!pos2) {
      error("No column indicator (c or C) found in condition",-1);
   }

   /* extract array id */
   colstring = get_clearstring(pos2-pos1);
   strncat(colstring, pos1+1, (size_t) (pos2-pos1-1));
   (*thiscond).cond_arrayid = (int) atoi(colstring);
   free(colstring);

   /* Find condition */
   (*thiscond).cond_type = -1;
   for (i=0; i<N_COND;i++) {
      dumpos = strstr((*thiscond).cond_text, conditions[i]);
      if (dumpos) {
         pos3 = dumpos;
         if (i == 4)
           (*thiscond).cond_type = 3;
         else if (i == 6)
           (*thiscond).cond_type = 5;
         else
           (*thiscond).cond_type = i;
     }
   }
   if (!pos3) {
      error("No comparison found in condition",-1);
   }
   colstring = get_clearstring(pos3-pos1);
   strncat(colstring, pos2+1,(size_t) (pos3-pos1-1));
   (*thiscond).cond_col = (int) atoi(colstring);
   (*thiscond).cond_number =
       (double)atof(pos3+strlen(conditions[(*thiscond).cond_type]));
   free(colstring);
}

void parse_main_cond(maincond_def *thiscond) {

   char *dumpos, *restcond, *onecond;
   boolean token_found;
   int length;

   /* Initialize */
   (*thiscond).n_subcond = 0;
   token_found = TRUE;

   /* Put text in restcond */
   restcond  = get_clearstring(strlen((*thiscond).cond_text));
   strcpy(restcond,(*thiscond).cond_text);

   /* Search for token that gives relation between conditions */
   while (token_found) {
      token_found = FALSE;

      /* Check for AND */
      dumpos = strstr(restcond, "&&");
      if (dumpos) {
        token_found = TRUE;
        (*thiscond).n_subcond += 1;
        (*thiscond).subcondrel[(*thiscond).n_subcond-1] = COND_AND;
      }

      /* Check for OR */
      if (!token_found) {
        dumpos = strstr(restcond, "||");
        if (dumpos) {
          token_found = TRUE;
          (*thiscond).n_subcond += 1;
          (*thiscond).subcondrel[(*thiscond).n_subcond-1] = COND_OR;
        }
      }
      /* If a token found, get this condition, store the relationship
       * and strip this condition from restcond */
      if (token_found) {
        length = (int) (dumpos - restcond)/sizeof(char);
        onecond = get_clearstring(length);
        strncpy(onecond, restcond, length);
      } else {
        (*thiscond).n_subcond += 1;
        onecond = get_clearstring(strlen(restcond)+1);
        strcpy(onecond, restcond);
      }
      free(restcond);
      if (dumpos) {
         restcond = get_clearstring(strlen(dumpos)-1);
         strncpy(restcond, dumpos+2,strlen(dumpos+2));
      } else
         restcond = NULL;
              
      /* Copy text of this condition to one member of the condition array */
      (*thiscond).subcond[(*thiscond).n_subcond-1].cond_text =
           (char *) malloc((strlen(onecond)+1)*sizeof(char));
      strcpy(((*thiscond).subcond[(*thiscond).n_subcond-1]).cond_text,
               onecond);

      /* Parse this condition */
      parse_cond(&((*thiscond).subcond[(*thiscond).n_subcond-1]));

      /*
      printf("Condition compare %i\n", ((*thiscond).subcond[(*thiscond).n_subcond-1]).cond_type);
      printf("Condition number %f\n", ((*thiscond).subcond[(*thiscond).n_subcond-1]).cond_number);
      printf("Condition column %i\n", ((*thiscond).subcond[(*thiscond).n_subcond-1]).cond_col);
      printf("Condition arrayID %i\n",((*thiscond).subcond[(*thiscond).n_subcond-1]).cond_arrayid);
      printf("Condition text %s\n", ((*thiscond).subcond[(*thiscond).n_subcond-1]).cond_text);
      */

   }
}


void reset_cond(maincond_def *loc_cond, int ncond, int arrayid){
   int i,j, n_subcond;

   for (i=0; i<ncond; i++) {
      n_subcond = (*(loc_cond+i)).n_subcond;
      for (j=0; j<n_subcond; j++) {
         if (((*(loc_cond+i)).subcond[j]).cond_arrayid  == arrayid)
            ((*(loc_cond+i)).subcond[j]).status = FALSE;
      }
      (*(loc_cond+i)).status = FALSE;
   }
}

void check_cond(maincond_def loc_cond[MAXCOND], int ncond,
                int arrayid, int col, double value){
   int i,j, n_subcond;

   for (i=0; i<ncond; i++) {
      n_subcond = (*(loc_cond+i)).n_subcond;
      for (j=0; j<n_subcond; j++) {
         if ((((*(loc_cond+i)).subcond[j]).cond_arrayid == arrayid) &&
             (((*(loc_cond+i)).subcond[j]).cond_col == col)) {
             ((*(loc_cond+i)).subcond[j]).status = FALSE;
             switch(((*(loc_cond+i)).subcond[j]).cond_type) {
                case EQUAL:
                   if (value == ((*(loc_cond+i)).subcond[j]).cond_number)
                      ((*(loc_cond+i)).subcond[j]).status = TRUE;
                   break;
                case GT:
                   if (value > ((*(loc_cond+i)).subcond[j]).cond_number)
                      ((*(loc_cond+i)).subcond[j]).status = TRUE;
                   break;
                case LT:
                   if (value < ((*(loc_cond+i)).subcond[j]).cond_number)
                      ((*(loc_cond+i)).subcond[j]).status = TRUE;
                   break;
                case GE:
                   if (value >= ((*(loc_cond+i)).subcond[j]).cond_number)
                      ((*(loc_cond+i)).subcond[j]).status = TRUE;
                   break;
                case LE:
                   if (value <= ((*(loc_cond+i)).subcond[j]).cond_number)
                      ((*(loc_cond+i)).subcond[j]).status = TRUE;
                   break;
                case NE:
                   if (value != ((*(loc_cond+i)).subcond[j]).cond_number)
                      ((*(loc_cond+i)).subcond[j]).status = TRUE;
                   break;
             }
             /*
             printf("condition on arrayid %i %i col %i %f %s %f : %i\n",
                       j,  arrayid,  col, value,
                         conditions[((*(loc_cond+i)).subcond[j]).cond_type],
                       ((*(loc_cond+i)).subcond[j]).cond_number,
                       ((*(loc_cond+i)).subcond[j]).status);
             */

         }
      }
   }
}

boolean all_cond(maincond_def loc_cond[MAXCOND], int ncond){
   int i,j, n_subcond;
   boolean return_value = TRUE;

   for (i=0; i<ncond; i++) {
      n_subcond = (*(loc_cond+i)).n_subcond;
      (*(loc_cond+i)).status = ((*(loc_cond+i)).subcond[0]).status;
      for (j=1; j<n_subcond; j++) {
         if (((*(loc_cond+i)).subcondrel[j-1]) == COND_AND)
            (*(loc_cond+i)).status = ((*(loc_cond+i)).status &&
                          ((*(loc_cond+i)).subcond[j]).status);
         if (((*(loc_cond+i)).subcondrel[j-1]) == COND_OR)
            (*(loc_cond+i)).status = ((*(loc_cond+i)).status ||
                          ((*(loc_cond+i)).subcond[j]).status);
      }
      /*
      printf("and/or %i status = %i %i %i \n",
                                 (int) ((*(loc_cond+i)).subcondrel[0]),
                                 (int) ((*(loc_cond+i)).subcond[0]).status,
                                 (int) ((*(loc_cond+i)).subcond[1]).status,
                                 (int) (*(loc_cond+i)).status);
      */
      return_value = (return_value && (*(loc_cond+i)).status);

   }
   return return_value;
}
