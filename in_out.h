/* $Id$ */
#if !defined(__STDLIB_H)
#include <stdlib.h>
#endif

#if !defined(__STDIO_H)
#include <stdio.h>
#endif

#if !defined(__STRING_H)
#include <string.h>
#endif

#if !defined(__MATH_H)
#include <math.h>
#endif

#if !defined(__IN_OUT_H)
#define __IN_OUT_H


/* boolean */
#define TRUE 1
#define FALSE 0

/* key codes */
#define LEFT    75
#define RIGHT   77
#define UP      72
#define DOWN    80
#define ENTER   13
#define ESC     27
#define PAGE_UP 73
#define PAGE_DN 81
#define TAB     9

typedef int boolean;

void
    progress(
        int     current,
        int     total);
void
    cmd_arg(
        char *(*arg[]),
        int  *arg_cnt,
        int  flag_length,
        char param[]);

/* ........................................................................
 * Function : progress
 * Purpose  : To show progress in a process, which involves a considerable
 *            number of identical steps. This is done by printing marks to
 *            stdout. The total number of marks is printed inadvance.
 *
 * Interface: current : number of the instance of the process now running
 *            total   : total number of instances of the process
 *
 * Method   : The first time PROGRESS is called, a number of static
 *            variables is initialized. These determine how many marks will
 *            be placed in total. Then a pipe-mark '|' is placed at the
 *            position of the first and the last mark.
 *            All next times PROGRESS is called it checks whether a certain
 *            number of instances of the process have passed since the last
 *            mark was printed. If so, a mark ('#')is placed.
 *            If CURRENT equals TOTAL a new-line is printed.
 * Remarks  : - If CURRENT>TOTAL progress does nothing.
 *            - The well-functioning of PROGRESS is based on the assumption
 *              that TOTAL is not changed during execution of the program.
 * Date     : 23-04-93
 * ........................................................................
 */
void progress(int current, int total)
{
    /*
     * variable declarations
     */
    const int
        line_length = 40;           /* maximum length of line of marks */
    static int
        items_per_mark,             /* number of items per mark */
        n_positions,                /* total number of marks */
        current_mark;               /* counter, keeping trace of current */
                                    /* mark */
    int
        i;                          /* counter */
    /* ....................................................................

     * If first time that PROGRESS is being called, initialize variables
     * and place markers for start and end of line of marks */
    if (current == 1)
    {
        /* Initialize static variables */
        items_per_mark = (total < line_length)
                          ?
                          total
                          :
                          ceil (total / ((float) line_length));
        n_positions = floor(total / items_per_mark);
        current_mark=1;

        /* Place start and end markers for marks */
        printf("|");
        for (i=1; i<n_positions-1; i++, printf(" "))
            ;
        printf("|\n");
    }

    /* If ITEMS_PER_MARK  items have passed since last mark has been
     * placed, place new mark and update CURRENT_MARK */
    if ((current >= current_mark*items_per_mark) && (current <= total))
    {
        printf("#");
        current_mark++;
    }

    /* If CURRENT is last one, write new-line */
    if (current == total)
       printf("\n");

}


/* ........................................................................
 * Function : cmd_arg
 * Purpose  : To return the value (string) of a parameter read from the
 *            command line. cmd_arg can handle both of the following forms
 *            of arguments:
 *             -$flag$argument and
 *             -$flag $argument
 *            where: $flag is any flag and $argument is any argument.
 *
 * Interface: arg        : a pointer, pointing to the argumentvector pointer;
 *            arg_cnt    : a pointer, pointing to the argument counter;
 *            flag_length: length (in char) of the flag (including possible
 *                         '-' sign);
 *            param      : array returning the value (char) of the argument
 *                         following the flag under consideration.
 *
 * Method   : First it is checked, whether spaces follow the flag. If so
 *            this info is stored in SPACE and the spaces are read until
 *            the start of the argument is encountered. Then the argument
 *            is read.
 *            If SPACE was set to be true, the argumentvector (through ARG)
 *            is advanced to point to the current argument (and not to the
 *            flag, as it did before). The argument counter (through
 *            ARG_CNT) is lowered one, to keep in pace with the argument
 *            vector.
 * Date     : 23-04-93
 *            Revisions:
 *            - 26-04-93 : - fixing of error in transfer of arg. vector
 *                         - inclusion of ARG_CNT in function arguments
 * ........................................................................
 */
void cmd_arg(char *(*arg[]), int *arg_cnt, int flag_length, char param[])
{
    boolean space = FALSE;          /* did space occur between flag and argument */

    flag_length -= 1;

    /* Check for spaces (and remove) following flag */
    for ( ; *(*arg[0] + ++flag_length) == '\0'; )
       space = TRUE;

    /* Place argument in parameter */
    if (!space)
      strcpy(param, (*(arg[0]) + flag_length));
    else
      strcpy(param, *(arg[0] + 1));

    /* In case flag was separated from paramater by space, decrease argument
     * counter and move one argument forward */
    if (space == TRUE)
    {
       *(*arg)++;
        (*arg_cnt)--;
    }
    return;
}


#endif  /* __IN_OUT_H */
