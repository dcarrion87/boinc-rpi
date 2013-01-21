/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * alx.cpp
 *
 * OLib
 * High performance library of mathematical functions and algorithms
 *
 * $Id: alx.cpp 354 2012-12-10 16:30:36Z luk.swierczewski@gmail.com $
 *
 * Library page: http://lib.oproject.info
 *
 * Written by Lukasz Swierczewski <luk.swierczewski@gmail.com>
 * Home page: http://goldbach.pl/~lswierczewski
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

# define OLIB_VERSION "$Rev: 354 $"
# define OLIB_LAST_CHANGE_DATE "$Date: 2012-12-10 17:30:36 +0100 (pon, 10 gru 2012) $"

# include <stdio.h>
# include <stdlib.h>
# include <time.h>
# include <math.h>

# include "boinc_api.h"


#if defined(_WIN32) || defined(_WIN64)

    # include <windows.h>

#endif

# ifdef __linux__

    # include <unistd.h>

# else

    # include <unistd.h>

# endif


# define ITERATIONS 150

int checkpoint(unsigned long long int *actual_iteration, int mode)
{

    if(mode == 0) // Read checkpoint
    {
        FILE *fp;

        fp=fopen("checkpoint", "rb");

        if (fp==NULL)
        {
            return 1;
        }

        fread (actual_iteration, 1, sizeof(unsigned long long int), fp);

        fclose (fp);

    }
    else if(mode == 1) // Save checkpoint
    {

        FILE *fp;

        fp=fopen("checkpoint", "wb");

        if (fp==NULL)
        {
            // printf ("Error: No save checkpoint to the file!");
            return -1;
        }

        fwrite (actual_iteration , 1 , sizeof(unsigned long long int), fp);

        fclose (fp);

    }

    return 0;

}


int main()
{

    int rc = boinc_init();
    if (rc)
    {
        fprintf(stderr, "APP: boinc_init() failed. rc=%d\n", rc);
        exit(rc);
    }


    srand(time(NULL));

    FILE *file_progress;

    unsigned long long int actual_iteration;
    int sleep_time;

    int read_checkpoint;

    read_checkpoint = checkpoint(&actual_iteration, 0); // Read checkpoint


    if(read_checkpoint == 1)
    {
        actual_iteration = 0;
    }


    for(actual_iteration; actual_iteration < ITERATIONS; actual_iteration++)
    {

        if (!(file_progress = fopen("progress", "wt")))
        {
            fprintf(stderr,"ERROR: CANNOT OPEN FILE PROGRESS\n");
        }
        else
        {
            if(actual_iteration == 0)
            {
                fprintf(file_progress, "0");
            }
            else
            {
                fprintf(file_progress, "%lf", ((double) actual_iteration / (double) ITERATIONS ) );
            }

            fclose(file_progress);

        }

        #if defined(_WIN32) || defined(_WIN64)

            sleep_time = (rand() % 7) + 1;
            sleep_time *= 1000;

            printf("ALX sleep...\n");
            Sleep(sleep_time);
            printf("ALX woke up...\n");

        #endif


        # ifdef __linux__

            sleep_time = (rand() % 7) + 1;

            printf("ALX sleep...\n");
            sleep(sleep_time);
            printf("ALX woke up...\n");

        # else

            sleep_time = (rand() % 7) + 1;

            printf("ALX sleep...\n");
            sleep(sleep_time);
            printf("ALX woke up...\n");

        # endif



        # ifdef BOINC_VERSION

            if( boinc_time_to_checkpoint() )
            {
                checkpoint(&actual_iteration, 1); // Save checkpoint
                boinc_checkpoint_completed();
            }

        # else

            checkpoint(&actual_iteration, 1); // Save checkpoint

        # endif


        boinc_fraction_done( ((double) actual_iteration) / ((double) ITERATIONS) );

    }



    // RESULTS FILE ---------------------------------------

    FILE *fp;

    if ((fp=fopen("result", "wt"))==NULL)
    {
        printf ("Error: No save result to the file!");
        return 1;
    }

    fprintf(fp, "1");

    fclose (fp);

    // RESULTS FILE ---------------------------------------



    boinc_fraction_done(1.0);
    boinc_finish(0);


    return 0;
}
