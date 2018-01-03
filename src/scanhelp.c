/*
 * File:        scanhelp.c
 * Description: help functions for lexer
 * Authors:     Yi Xu (Modified from the original file in Stanford CS346 RedBase)
 *
 * This file is not compiled separately; it is #included into lex.yy.c.
 */

/*
 * size of buffer of strings
 */
#define MAXCHAR 5000000

/*
 * buffer for string allocation
 */
static char charpool[MAXCHAR];
static int charptr = 0;

static int lower(char *dst, char *src, int max);
static char *mk_string(char *s, int len);

/*
 * string_alloc: returns a pointer to a string of length len if possible.
 */
static char *string_alloc(int len) {
    char *s;
    if (charptr + len > MAXCHAR) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    s = charpool + charptr;
    charptr += len;
    return s;
}

/*
 * reset_charptr: releases all memory allocated in preparation for the next query.
 *
 * No return value.
 */
void reset_charptr(void) {
    charptr = 0;
}

/*
 * reset_scanner: resets the scanner after a syntax error.
 *
 * No return value.
 */
void reset_scanner(void) {
    charptr = 0;
    yyrestart(yyin);
}

/*
 * get_id: determines whether s is a reserved word, and returns the appropriate token value if it is.
 * Otherwise, it returns the token value corresponding to a string.
 * If s is longer than the maximum token length (MAXSTRINGLEN) then it returns NOTOKEN,
 * so that the parser will flag an error (this is a stupid kludge).
 */
static int get_id(char *s) {
    static char string[MAXSTRINGLEN];
    int len;
    if ((len = lower(string, s, MAXSTRINGLEN)) == MAXSTRINGLEN) return NOTOKEN;
    if (!strcmp(string, "database"))  return yylval.ival = RW_DATABASE;
    if (!strcmp(string, "databases")) return yylval.ival = RW_DATABASES;
    if (!strcmp(string, "table"))     return yylval.ival = RW_TABLE;
    if (!strcmp(string, "tables"))    return yylval.ival = RW_TABLES;
    if (!strcmp(string, "show"))      return yylval.ival = RW_SHOW;
    if (!strcmp(string, "create"))    return yylval.ival = RW_CREATE;
    if (!strcmp(string, "drop"))      return yylval.ival = RW_DROP;
    if (!strcmp(string, "use"))       return yylval.ival = RW_USE;
    if (!strcmp(string, "primary"))   return yylval.ival = RW_PRIMARY;
    if (!strcmp(string, "key"))       return yylval.ival = RW_KEY;
    if (!strcmp(string, "not"))       return yylval.ival = RW_NOT;
    if (!strcmp(string, "null"))      return yylval.ival = RW_NULL;
    if (!strcmp(string, "insert"))    return yylval.ival = RW_INSERT;
    if (!strcmp(string, "into"))      return yylval.ival = RW_INTO;
    if (!strcmp(string, "values"))    return yylval.ival = RW_VALUES;
    if (!strcmp(string, "delete"))    return yylval.ival = RW_DELETE;
    if (!strcmp(string, "from"))      return yylval.ival = RW_FROM;
    if (!strcmp(string, "where"))     return yylval.ival = RW_WHERE;
    if (!strcmp(string, "update"))    return yylval.ival = RW_UPDATE;
    if (!strcmp(string, "set"))       return yylval.ival = RW_SET;
    if (!strcmp(string, "select"))    return yylval.ival = RW_SELECT;
    if (!strcmp(string, "is"))        return yylval.ival = RW_IS;
    if (!strcmp(string, "int"))       return yylval.ival = RW_INT;
    if (!strcmp(string, "varchar"))   return yylval.ival = RW_VARCHAR;
    if (!strcmp(string, "desc"))      return yylval.ival = RW_DESC;
    if (!strcmp(string, "index"))     return yylval.ival = RW_INDEX;
    if (!strcmp(string, "and"))       return yylval.ival = RW_AND;
    if (!strcmp(string, "date"))      return yylval.ival = RW_DATE;
    if (!strcmp(string, "float"))     return yylval.ival = RW_FLOAT;
    if (!strcmp(string, "foreign"))   return yylval.ival = RW_FOREIGN;
    if (!strcmp(string, "references"))return yylval.ival = RW_REFERENCES;
    if (!strcmp(string, "exit"))      return yylval.ival = RW_EXIT;
    if (!strcmp(string, "print"))     return yylval.ival = RW_PRINT;
    yylval.sval = mk_string(s, len);
    return T_STRING;
}

/*
 * lower: copies src to dst, converting it to lowercase, stopping at the end of src or after max characters.
 *
 * Returns: the length of dst (which may be less than the length of src, if src is too long).
 */
static int lower(char *dst, char *src, int max) {
    int len;
    for (len = 0; len < max && src[len] != '\0'; ++len) {
        dst[len] = src[len];
        if (src[len] >= 'A' && src[len] <= 'Z')
            dst[len] += 'a' - 'A';
    }
    if (len < max) dst[len] = '\0';
    return len;
}

/*
 * get_qstring: removes the quotes from a quoted string, allocates space for the resulting string.
 *
 * Returns: a pointer to the new string
 */
static char *get_qstring(char *qstring, int len) {
    /* replace ending quote with \0 */
    qstring[len - 1] = '\0';
    /* copy everything following beginning quote */
    return mk_string(qstring + 1, len - 2);
}

/*
 * mk_string: allocates space for a string of length len and copies s into it.
 *
 * Returns: a pointer to the new string
 */
static char *mk_string(char *s, int len) {
    char *copy;
    /* allocate space for new string */
    if ((copy = string_alloc(len + 1)) == NULL) {
        printf("out of string space\n");
        exit(1);
    }
    /* copy the string */
    strncpy(copy, s, len + 1);
    return copy;
}
