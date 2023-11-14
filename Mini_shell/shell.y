/*
 * CS-413 Spring 98
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token	<string_val> WORD

%token 	NOTOKEN GREAT NEWLINE GREATGREAT SMALL AMPERSAND STICK EXIT_COMMAND GGAMP                                                                                                                                                                                                                                                                                                     

%union	{
		char   *string_val;
	}

%{
extern "C" 
{
	int yylex();
	void yyerror (char const *s);
}
#define yylex yylex
#include <stdio.h>
#include "command.h"
%}

%%

goal:	
	commands
	;

commands: 
	command
	| commands command 
	;

command: EXIT_COMMAND NEWLINE { printf("\t Goodbye\n");exit(0); } 
	| 
	simple_command NEWLINE
        ;

simple_command:	
	
	command_and_args iomodifier_ipt iomodifier_opt background STICK simple_command {
	//	printf("   Yacc: Execute command\n");
	//	Command::_currentCommand.execute();
	}
	| command_and_args iomodifier_ipt iomodifier_opt background {
		printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| 
	stick {
	  printf("   Yacc: piping command \n");
	  }
	| NEWLINE
	| error NEWLINE { yyerrok; }
	;

command_and_args:
	command_word arg_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {
               printf("   Yacc: insert argument \"%s\"\n", $1);

	       Command::_currentSimpleCommand->insertArgument( $1 );\
	}
	;

command_word:
	WORD {
               printf("   Yacc: insert command \"%s\"\n", $1);
	       
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

iomodifier_opt:
	GREAT WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
	}
	| GREATGREAT WORD {
		printf("   Yacc: append output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._append = 1;
	}
	|
	GGAMP WORD {
		printf("   Yacc: append output\"%s\"\n", $2);
		printf("   Yacc: append error to error file \n");
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = "err.txt";
		Command::_currentCommand._append = 1;
		Command::_currentCommand._freeonce = 1;
		
	}
	| GREAT WORD SMALL WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		printf("   Yacc: insert input \"%s\"\n", $4);
		Command::_currentCommand._inputFile = $4;
	}
	| /* can be empty */ 
	;

iomodifier_ipt:
	SMALL WORD {
		printf("   Yacc: insert input \"%s\"\n", $2);
		Command::_currentCommand._inputFile = $2;
	}
	| /* can be empty */ 
	;
	
background:
	AMPERSAND {
		printf("   Yacc: background command \n");
		Command::_currentCommand._background = 1;
	}
	| /* can be empty */ 
	;
	
stick:
	simple_command STICK simple_command  {
		printf("   Yacc: piping command \n");
	}
	| // can be empty 
	;

%%
void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}
#if 0
main()
{
	yyparse();
}
#endif
