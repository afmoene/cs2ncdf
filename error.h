/* $Id$ */
/* error.h: some routines for simple error handling 
 * Copyright (C) 1993  Arnold Moene 
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
#if !defined(__ERROR_H)
#define __ERROR_H

#define INCOMP_FILES     10         /* incompatibel files */
#define FILE_NOT_FOUND   20         /* file not found */
#define FILE_NOT_CREATE  21         /* could not create file */
#define CMD_LINE_ERROR   30         /* some error in command line */
#define IMAGE_DEF_ERROR  40         /* definition of image is wrong */
#define FILE_CONTENTS    50         /* file contents do not match expected
                                     * file contents
                                     */
#define ARRAY_SIZE_LIMIT 60         /* requested size of array is not
                                     * supported by compiled size
                                     */



void
    error(
        char    mess[],
        int     error_code);


/* ........................................................................
 * Function : error
 * Purpose  : To print a message to stdout and exit program the program
 *            with an ERROR_CODE.
 * Interface: mess       : message to be displayed;
 *            error_code : number of error.
 * Method   : The MESSage is concatenated to the prefix "Error :" and
 *            printed. Then ERROR exits with code ERROR_CODE.
 * Date     : February 1993
 *            Revisions:
 *            - 26-04-93 : inclusion of ERROR_CODE in function arguments
 * ........................................................................
 */
void error(char mess[], int error_code)
{
	char lead[] = "Error : ";

	strcat(lead, mess);
	printf(lead);
	exit(error_code);
}


#endif     /* __ERROR_H */
