#include <stdlib.h>
#include <stdio.h>

enum state {GAP, TRACK, ZEROS};

int main()
{
    enum state curr = GAP;

    int count = 0;
    int start, end;

    const double sample_rate = (double) 1 / 44100;
    //const double min_gap_sec = 0.0001;
    //const int min_gap_samples = (int)(sample_rate * min_gap_sec);
    const int threshold = 5;

    int value;
    int zeros;
    while (scanf("%d", &value) != EOF)
    {
        switch (curr)
        {
            case GAP:
                if (abs(value) > threshold)
                {
                    start = count;
                    curr = TRACK;
                }
                break;

            case TRACK:
                if (abs(value) <= threshold)
                {
                    end = count - 1;
                    zeros = 1;
                    curr = ZEROS;
                }
                break;

            case ZEROS:
                if (abs(value) > threshold)
                {
                    curr = TRACK;
                }

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
    if (curr == TRACK)
    {
        printf("[%.6f-%.6f]\n", (double)(start) * sample_rate, (double)(count - 1) * sample_rate);
    }
    else if (curr == ZEROS)
    {
        printf("[%.6f-%.6f]\n", (double)(start) * sample_rate, (double)(end) * sample_rate);
    }
}
