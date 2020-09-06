#include <stdlib.h>
#include <stdio.h>

enum state {GAP, TRACK, ZEROS};

int main()
{
    //Initializing values
    enum state curr = GAP;

    int count = 0;
    int start, end;

    const double sample_rate = (double) 1 / 44100;
    const int threshold = 5;

    int value;
    int zeros;
    
    /*While loop that looks at one input line at a time
    For each line looks at current state and reacts according to the value of the input*/
    while (scanf("%d", &value) > 0)
    {
        switch (curr)
        {
            //If the current state is GAP
            case GAP:
                //If value is high, remember the start position and change state to TRACK
                if (abs(value) > threshold)
                {
                    start = count;
                    curr = TRACK;
                }
                break;

            //If the current state is TRACK
            case TRACK:
                //If value is low, remember the end position and change state to ZEROS
                if (abs(value) <= threshold)
                {
                    end = count - 1;
                    zeros = 1;
                    curr = ZEROS;
                }
                break;

            //If the current state is ZEROS
            case ZEROS:
                //If value is high, change state to TRACK
                if (abs(value) > threshold)
                {
                    curr = TRACK;
                }

                /*If value is low, check if there have been four consecutive low values
                If yes, print start and end of the finished track and change state to GAP*/
                else if (abs(value) <= threshold)
                {
                    zeros = zeros + 1;
                    if (zeros >= threshold - 1)
                    {
                        printf("[%.6f-%.6f]\n", (double)(start) * sample_rate, (double)(end) * sample_rate);
                        zeros = 0;
                        curr = GAP;
                    }
                }
                break;
        }
        count++;
    }

    /*After parsing through the input files, look at final state
    If current state is TRACK or ZEROS, print start and end of the last track*/
    if (curr == TRACK)
    {
        printf("[%.6f-%.6f]\n", (double)(start) * sample_rate, (double)(count - 1) * sample_rate);
    }

    else if (curr == ZEROS)
    {
        printf("[%.6f-%.6f]\n", (double)(start) * sample_rate, (double)(end) * sample_rate);
    }

    return 0;
}
