/* A Bison parser, made from config.ypp, by GNU bison 1.75.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#ifndef BISON_CONFIG_TAB_HPP
# define BISON_CONFIG_TAB_HPP

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     STRING = 258,
     QSTRING = 259,
     ATTRIBUTE = 260,
     SET = 261,
     SECTION_BEGIN = 262,
     SECTION_END = 263,
     INCLUDE_FILE = 264,
     EOF_FILE = 265
   };
#endif
#define STRING 258
#define QSTRING 259
#define ATTRIBUTE 260
#define SET 261
#define SECTION_BEGIN 262
#define SECTION_END 263
#define INCLUDE_FILE 264
#define EOF_FILE 265




#ifndef YYSTYPE
#line 29 "config.ypp"
typedef union {
    char *val;
} yystype;
/* Line 1281 of /usr/local/share/bison/yacc.c.  */
#line 64 "config.tab.hpp"
# define YYSTYPE yystype
#endif




#endif /* not BISON_CONFIG_TAB_HPP */

