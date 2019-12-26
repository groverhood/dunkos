#ifndef DUNKOS_CTYPE_H
#define DUNKOS_CTYPE_H

#define isalpha(c) (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
#define isdigit(c) ('0' <= c && c <= '9')
#define isalnum(c) (isalpha(c) || isdigit(c))
#define isspace(c) (c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f')

#endif