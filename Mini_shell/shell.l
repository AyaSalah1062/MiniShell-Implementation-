/*
 * CS-413 Spring 98
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 */

%{
#include <string.h>
#include "y.tab.h"
%}

%%

\n      {
    return NEWLINE; // Return token for newline character
}

[ \t]    {
    /* Discard spaces and tabs */
}

"exit"  {
    return EXIT_COMMAND; // Return token for the "exit" command
}

">"     {
    return GREAT; // Return token for the ">" symbol
}

">>"    {
    return GREATGREAT; // Return token for ">>"
}

"<"     {
    return SMALL; // Return token for "<"
}

"&"     {
    return AMPERSAND; // Return token for "&"
}

"|"     {
    return STICK; // Return token for "|"
}

">>&"   {
    return GGAMP; // Return token for ">>&"
}

[^ <>|&\t\n]+  {
    /* Assume that file names have only alpha chars */
    yylval.string_val = strdup(yytext);
    return WORD; // Return token for a word (filename)
}

.       {
    /* Handle unexpected characters here, or remove this rule */
    fprintf(stderr, "Unexpected character: %c\n", yytext[0]);
    return NOTOKEN;
}

%%
