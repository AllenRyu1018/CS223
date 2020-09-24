#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * Prints the tags (all or leading depending on the command-line switch).
 * 
 * @param input a file to read from; stdin by default
 * @param output a file to write into (overwriting as necessary); stdout by default
 * @param mode mode character denoting the mode ('a' for all tags, 'l' for leading tags); 'l' by default
 */
void print_tags(FILE *input, FILE *output, const char mode);

// There are 6 states for the state machine:
// CODE, which is just regular code (simplest of all)
// BLOCK, when in the middle of a C code block (delimited by {})
// COMMENT, when in the middle of a C comment (either single line, beginning with //, or multi-line, delimited by /* */)
// STRING, when in the middle of a C string literal, delimited by single or double quotes ('' or "")
// SINGLETAG, when in the middle of a tag (beginning with @) of a single line comment
// DOUBLETAG, when in the middle of a tag (also beginning with @) of a multi-line comment
enum state {CODE, BLOCK, COMMENT, STRING, SINGLETAG, MULTITAG};

int main(int argc, char **argv)
{
    // Initializing variables
    // tag_count counts number of "-a" and "-l" in the command-line argument
    // If there is more than one, the program returns error
    int tag_count = 0;
    FILE *input = stdin;
    FILE *output = stdout;
    char all_or_leading = 'l';

    // Check for "-i" in the command-line, denoting a file to read from
    // If "-i" is followed by nothing, or the file cannot be opened, 
    // return error; otherwise, change input to the filename following "-i"
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-i") == 0)
        {
            if (i == argc - 1)
            {
                fprintf(stderr, "%s: must specify input file after \"-i\"\n", argv[0]);
                return 1;
            }
            else
            {
                input = fopen(argv[i + 1], "r");
                if (!input)
                {
                    fprintf(stderr, "%s: could not open %s\n", argv[0], argv[i + 1]);
                    return 1;
                }
            } 
        }
        // Do the same for "-o", but with output instead of input
        else if (strcmp(argv[i], "-o") == 0)
        {
            if (i == argc - 1)
            {
                fprintf(stderr, "%s: must specify output file after \"-o\"\n", argv[0]);
                return 1;
            }
            else
            {
                output = fopen(argv[i + 1], "w");
                if (!output)
                {
                    fprintf(stderr, "%s: could not open %s\n", argv[0], argv[i + 1]);
                    return 1;
                }
            }
        }
    }
    
    // Check for mode switches and count the number of them
    // If more than one, return error
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-a") == 0)
        {
            all_or_leading = 'a';
            tag_count++;
        }
        else if (strcmp(argv[i], "-l") == 0)
        {
            all_or_leading = 'l';
            tag_count++;
        }
    }
    if (tag_count > 1)
    {
        fprintf(stderr, "%s: you can only input at most one of \"-a\" or \"-l\"\n", argv[0]);
        return 1;
    }

    // If all goes well, print the tags as instructed
    print_tags(input, output, all_or_leading);
    return 0;
}

void print_tags(FILE *input, FILE *output, const char mode)
{
    // Initialize a lot of variables
    
    // Stores the current state of the state machine
    enum state current = CODE;

    // Store the current character from the file into character
    char character;

    // Count the number of slashes preceding the current character
    // in order to know whether to switch states to COMMENTS
    int slash_count = 0;

    // Count the number of line-continuation characters ('\') immediately
    // preceding current character; two \'s (\\) cancel each other out, and
    // if there was one before the current character, escape both
    int continue_count = 0;

    // Denotes whether current state (or the one immediately before the current
    // one) was CODE or BLOCK; useful for switching between CODE/BLOCK and COMMENT/STRING
    int code_or_block = 0;

    // Denotes whether comment is in a potential end-state; if conditions are met, 
    // the comment ends and state returns to CODE or BLOCK
    int end_of_comment = 0;

    // Denotes whether the current tag is a leading tag or not; leading if 0 and not if 1
    int lead_count = 0;
    
    // Counts the number of open brackets; if 0, then every code block is closed
    int bracket_count = 0;

    // Counts the number of asterisks; useful in multi-line tags, where counting asterisks may be tricky
    int asterisk_count = 0;

    // Denotes whether the current comment is single or multi-line comment (single if 0, multi if 1)
    int single_or_multi_comment = 0;

    // Denotes whether the current string literal is in single or double quotes (single if 0, double if 1)
    int single_or_double = 0;

    // Reads the input file one character at a time until the end
    // At each character, checks current state and reacts accordingly
    while ((character = getc(input)) != EOF)
    {
        switch(current)
        {
            // Current state is CODE
            case CODE:
                if (character == '/')
                {
                    // If there wasn't a / before, add one to slash_count
                    if (slash_count == 0)
                    {
                        slash_count++;
                        break;
                    }
                    // If there was a / before, change state to COMMENT (single line)
                    else if (slash_count == 1)
                    {
                        current = COMMENT;
                        single_or_multi_comment = 0;
                        slash_count = 0;
                    }
                    break;
                }

                // If character is '*' and there was a / before, change state to COMMENT (multi-line)
                else if (character == '*')
                {
                    if (slash_count == 1)
                    {
                        current = COMMENT;
                        single_or_multi_comment = 1;
                        slash_count = 0;
                    }
                    break;
                }

                // Otherwise reset slash_count
                slash_count = 0;

                // Change state to string if character is ' or "; keep track of which
                if (character == '\'' || character == '\"')
                {
                    current = STRING;
                    single_or_double = (character == '\'') ? 0 : 1;
                    break;
                }

                // Change state to BLOCK if character is open bracket
                else if (character == '{')
                {
                    code_or_block = 1;
                    bracket_count = 1;
                    current = BLOCK;
                    break;
                }

                // Otherwise continue to next character
                break;

            // Current state is BLOCK
            case BLOCK:
                if (character == '/')
                {
                    // If there wasn't a / before, add one to slash_count
                    if (slash_count == 0)
                    {
                        slash_count++;
                        break;
                    }
                    // If there was a / before, change state to COMMENT (single line)
                    else if (slash_count == 1)
                    {
                        current = COMMENT;
                        single_or_multi_comment = 0;
                        slash_count = 0;
                    }
                    break;
                }

                // If there was a / before, change state to COMMENT (multi-line)
                else if (character == '*')
                {
                    if (slash_count == 1)
                    {
                        current = COMMENT;
                        single_or_multi_comment = 1;
                        slash_count = 0;
                    }
                    break;
                }

                // Otherwise reset slash_count
                slash_count = 0;

                // If character opens another block, add one to bracket_count
                if (character == '{')
                {
                    bracket_count++;
                    break;
                }

                // If character closes a bracket, then subtract from bracket_count
                // If bracket_count = 0, then there are no more open brackets,
                // so we aren't in a code block anymore; change state to CODE
                else if (character == '}')
                {
                    bracket_count--;
                    if (bracket_count == 0)
                    {
                        code_or_block = 0;
                        current = CODE;
                    }
                    break;
                }

                // Change state to string if character is ' or "; keep track of which
                else if (character == '\'' || character == '\"')
                {
                    code_or_block = 1;
                    current = STRING;
                    single_or_double = (character == '\'') ? 0 : 1;
                    break;
                }
                // Otherwise continue
                break;

            // Current state is COMMENT
            case COMMENT:
                // If character is \, then keep track of it with continue_count
                if (character == '\\')
                {
                    continue_count = 1;
                    break;
                }

                else if (character == '\n')
                {
                    // If there was  a \ right before, then ignore both of them
                    if (continue_count == 1)
                    {
                        continue_count = 0;
                        break;
                    }
                    // Otherwise, reset lead_count
                    // If single line comment, change state to CODE or BLOCK depending on code_or_block
                    // If multi-line comment, reset end_of_comment
                    else
                    {
                        lead_count = 0;
                        if (single_or_multi_comment == 0)
                        {
                            if (code_or_block == 0)
                            {
                                current = CODE;
                                break;
                            }
                            else if (code_or_block == 1)
                            {
                                current = BLOCK;
                                break;
                            }
                        }
                        else if (single_or_multi_comment == 1)
                        {
                            end_of_comment = 0;
                        }
                        break;
                    }
                }
                
                // Otherwise reset continue_count
                continue_count = 0;

                // If comment is not within a code block and character is @,
                // and it is a leading tag (matters only if mode is -l), then
                // print @ and switch state to SINGLETAG or MULTITAG depending on mode
                if (code_or_block == 0 && character == '@')
                {
                    if ((mode == 'l' && lead_count == 0) || mode == 'a')
                    {
                        fprintf(output, "%c", character);
                        current = (single_or_multi_comment == 0) ? SINGLETAG : MULTITAG;
                        break;
                    }
                }

                // If single line comment and character is not * or
                // a whitespace character, then change lead_count to 1
                else if (single_or_multi_comment == 0)
                {
                    if (character != '*' && !isspace(character))
                    {
                        lead_count = 1;
                        break;
                    }
                    break;
                }
                
                // Otherwise, the comment is multi-line
                else if (single_or_multi_comment == 1)
                {
                    // If current character is / and not at end
                    // of comment, then change lead_count to 1
                    // If at end_of_comment, switch state to either
                    // CODE or BLOCK and reset lead_count
                    if (character == '/')
                    {
                        if (end_of_comment == 0)
                        {
                            lead_count = 1;
                            break;
                        }
                        else if (end_of_comment == 1)
                        {
                            if (code_or_block == 0)
                            {
                                current = CODE;
                                lead_count = 0;
                                break;
                            }
                            else if (code_or_block == 1)
                            {
                                current = BLOCK;
                                lead_count = 0;
                                break;
                            }
                        }
                    }
                    // If character is *, change end_of_comment
                    // to 1
                    else if (character == '*')
                    {
                        end_of_comment = 1;
                        break;
                    }
                    // If whitespace, then reset end_of_comment
                    else if (isspace(character))
                    {
                        end_of_comment = 0;
                        break;
                    }
                    // Otherwise, reset end_of_comment and change
                    // lead_count to 1
                    else
                    {
                        end_of_comment = 0;
                        lead_count = 1;
                        break;
                    }
                }

            // Current state is STRING
            case STRING:
                // If current character is continuation character
                if (character == '\\')
                {
                    if (continue_count == 1)
                    {
                        continue_count = 0;
                        break;
                    }
                    else if (continue_count == 0)
                    {
                        continue_count = 1;
                        break;
                    }
                }

                // If current character closes the quote, then switch
                // state to CODE or BLOCK depending on code_or_block value
                else if ((character == '\'' && single_or_double == 0) || (character == '\"' && single_or_double == 1))
                {
                    if (continue_count == 1)
                    {
                        continue_count = 0;
                        break;
                    }
                    else if (code_or_block == 0)
                    {
                        current = CODE;
                        break;
                    }
                    else if (code_or_block == 1)
                    {
                        current = BLOCK;
                        break;
                    }
                }
                // Otherwise continue
                continue_count = 0;
                break;

            // Current state is SINGLETAG
            case SINGLETAG:
                // If current character is continuation character
                if (character == '\\')
                {
                    continue_count = 1;
                    break;
                }

                // If current character is newline character and there
                // wasn't a continuation character before, then print
                // newline and change state to CODE
                else if (character == '\n')
                {
                    if (continue_count == 1)
                    {
                        break;
                    }
                    else
                    {
                        if (!(mode == 'l' && lead_count == 1))
                        {
                            fprintf(output, "\n");
                        }
                        current = CODE;
                        lead_count = 0;
                        break;
                    }
                }

                // If current character is whitespace, then stop printing
                // and change state to COMMENT (single line)
                else if (isspace(character))
                {
                    if (!(mode == 'l' && lead_count == 1))
                    {
                        fprintf(output, "\n");
                    }
                    current = COMMENT;
                    single_or_multi_comment = 0;
                    continue_count = 0;
                    lead_count = 1;
                    break;
                }

                // Otherwise print current character
                else
                {
                    if (!(mode == 'l' && lead_count == 1))
                    {
                        fprintf(output, "%c", character);
                    }
                    continue_count = 0;
                    break;
                }
            
            // Current state is MULTITAG
            case MULTITAG:
                // If current character is continuation character
                if (character == '\\')
                {
                    if ((mode == 'l' && lead_count == 0 && asterisk_count > 0) || mode == 'a')
                    {
                        for (int i = 0; i < asterisk_count - 1; i++)
                        {
                            fprintf(output, "*");
                        }
                        asterisk_count = 1;
                    }
                    continue_count = 1;
                    break;
                }

                // If current character is newline and there wasn't a continuation
                // character before, then print all the asterisks immediately before
                // it and then a newline; reset lead_count and change state to COMMENT
                else if (character == '\n')
                {
                    if (continue_count == 1)
                    {
                        break;
                    }
                    else
                    {
                        if ((mode == 'l' && lead_count == 0) || mode == 'a')
                        {
                            for (int i = 0; i < asterisk_count; i++)
                            {
                                fprintf(output, "*");
                            }
                            asterisk_count = 0;
                            fprintf(output, "\n");
                        }
                        current = COMMENT;
                        single_or_multi_comment = 1;
                        lead_count = 0;
                        break;
                    }
                }

                // If mode is -a or mode is -l but current character
                // is within the leading tag
                else if (lead_count == 0 || mode == 'a')
                {
                    // If character is *, then increment asterisk_count
                    if (character == '*')
                    {
                        asterisk_count++;
                        break;
                    }
                    // If character is / and there was at least one asterisk
                    // before it, print all of them except one, print newline,
                    // reset asterisk_count, and change state to CODE
                    else if (character == '/')
                    {
                        if (asterisk_count > 0)
                        {
                            for (int i = 0; i < asterisk_count - 1; i++)
                            {
                                fprintf(output, "*");   
                            }
                            fprintf(output, "\n");
                            asterisk_count = 0;
                            current = CODE;
                            break;
                        }
                    }
                    // Otherwise print all the asterisks immediately before
                    // current character and reset asterisk_count
                    for (int i = 0; i < asterisk_count; i++)
                    {
                        fprintf(output, "*");
                    }
                    asterisk_count = 0;
                    // If current character is a whitespace character, then
                    // print a newline and change current state to COMMENT
                    if (isspace(character))
                    {
                        fprintf(output, "\n");
                        current = COMMENT;
                        single_or_multi_comment = 1;
                        continue_count = 0;
                        lead_count = 1;
                        break;
                    }
                    // Otherwise print current character
                    else
                    {
                        fprintf(output, "%c", character);
                        continue_count = 0;
                        break;
                    }
                }

                // If mode is -l and current character is not 
                // in the leading tag, then continue
                else if (lead_count == 1)
                {
                    continue_count = 0;
                    break;
                }
        }
    }
    
    // If final state is SINGLETAG or MULTITAG, file ended
    // in the middle of a tag; return newline for consistency
    if (current == SINGLETAG || current == MULTITAG)
    {
        fprintf(output, "\n");
    }
}
