%option prefix="conf_"
%option yylineno
%option  noyywrap

%{
#if defined(WIN32)
#pragma warning(disable:4102)
extern "C" {
# include <io.h>
};
#endif

#include <cstring>
#include "config.tab.hpp"
#define LOOKUP 0

extern YYSTYPE conf_lval;

#define YY_DECL int conf_lex(YYSTYPE *lvalp)
//#define YY_DECL int conf_lex (YYSTYPE *lvalp, YYLTYPE *llocp)
%}

%%

[A-Za-z0-9_]+	{ lvalp->val = strdup(conf_text); return ATTRIBUTE; }
\"[^"\n]*(\\\"[^"]*)*["\n]	{ 
	    {
		if (strlen(conf_text)!=0) {
		    *(conf_text+strlen(conf_text)-1) = '\0';
		}
		char *conf_text_ptr_r = conf_text+1;
		char *conf_text_ptr_w = conf_text_ptr_r;
		while (*conf_text_ptr_r != '\0') {
		    if (*conf_text_ptr_r == '\\' && *(conf_text_ptr_r+1)!='\\') {
			++conf_text_ptr_r;
			continue;
		    }
		    if (conf_text_ptr_r != conf_text_ptr_w) { //need write.
			*conf_text_ptr_w = *conf_text_ptr_r;
		    }
		    ++conf_text_ptr_w;
		    ++conf_text_ptr_r;
		}
		*conf_text_ptr_w='\0';
		lvalp->val = strdup(conf_text+1); return QSTRING; 
	    }
	}
"{"			{ return SECTION_BEGIN; }
"}"			{ return SECTION_END; }
[A-Za-z0-9_-]+		{ lvalp->val = strdup(conf_text); return STRING; }
<<EOF>>		{ return EOF_FILE; }
#.*[\r\n]		{ ; }
[ \t\n\r]		{ ; }
=			{ return SET; }
.			{ return conf_text[0]; }

%%
