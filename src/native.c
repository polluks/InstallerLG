#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "eval.h"
#include "native.h"
#include "util.h"
#include "error.h"
#include "debug.h"

/*
*/
entry_p m_gosub(entry_p contxt)
{
    static int dep = 0; 
    entry_p con = global(contxt);  
    if(con && con->symbols)
    {
        entry_p *cus = con->symbols; 
        while(*cus && 
              *cus != end())
        {
            if((*cus)->type == CUSTOM &&
               !strcmp((*cus)->name, contxt->name))
            {
                entry_p ret; 
                entry_p *arg = (*cus)->symbols, 
                        *ina = contxt->children;
                if(arg && ina)
                {
                    while(*arg && *arg != end() &&
                          *ina && *ina != end())
                    {
                        entry_p res = malloc(sizeof(entry_t)); 
                        if(res)
                        {
                            memmove(res, resolve(*ina), sizeof(entry_t)); 
                            res->name = res->name ? strdup(res->name) : NULL; 
                            res->parent = *arg; 
                            kill((*arg)->resolved); 
                            (*arg)->resolved = res; 
                        }
                        else
                        {
                            error(PANIC);
                            return new_failure(); 
                        }
                        arg++; 
                        ina++;
                    }
                }
                if(dep++ < MAXDEP)
                {
                    ret = invoke(*cus); 
                    dep--; 
                    return ret; 
                }
                else
                {
                    error(contxt->id, "Max recursion depth exceeded", 
                          contxt->name); 
                    return new_failure(); 
                }
            }
            cus++; 
        }
        error(contxt->id, "Undefined function", 
              contxt->name); 
    }
    else
    {
        error(PANIC);
    }
    return new_failure(); 
}

/*
*/
entry_p m_set(entry_p contxt)
{
    entry_p dst = global(contxt);
    if (dst)
    {
        entry_p *sym = contxt->symbols, 
                *val = contxt->children; 
        while(*sym && *sym != end() &&
              *val && *val != end())
        {
            entry_p res = malloc(sizeof(entry_t)); 
            if(res)
            {
                memmove(res, resolve(*val), sizeof(entry_t)); 
                res->name = res->name ? strdup(res->name) : NULL; 
                res->parent = *sym; 
                kill((*sym)->resolved); 
                (*sym)->resolved = res; 
                push(dst, *sym); 
                (*sym)->parent = contxt;  
            }
            else
            {
                break; 
            }
            if(*(++sym) == *(++val))
            {
                return res; 
            }
        }
    }
    error(PANIC);
    return new_failure(); 
}

/*
*/
entry_p m_procedure(entry_p contxt)
{
    entry_p dst = global(contxt);
    if(dst && 
       contxt->symbols &&
       contxt->symbols[0] && 
       contxt->symbols[0] != end())
    {
        push(dst, contxt->symbols[0]); 
        contxt->symbols[0]->parent = contxt; 
        return contxt->symbols[0];
    }
    error(PANIC);
    return new_failure(); 
}

/*
*/
entry_p m_if(entry_p contxt)
{
    ARGS(3); 
    entry_p p = num(a1) ? a2 : a3; 
    if(p->type == CONTXT)
    {
        return invoke(p);
    }
    else if(p->type == NATIVE ||
            p->type == CUSREF)
    {
        if(p->call)
        {
            return p->call(p); 
        }
    }
    error(PANIC);
    return new_failure(); 
}

/*
*/
static entry_p m_whunt(entry_p contxt, int m)
{
    ARGS(2); 
    entry_p r = new_failure(); 
    if(a2->type == CONTXT)
    {
        while((m ^ num(a1)) && 
              !did_error())
        {
            r = invoke(a2);
        }
    }
    else
    {
        error(PANIC);
    }
    return r; 
}

/*
*/
entry_p m_while(entry_p contxt)
{
    return m_whunt(contxt, 0); 
}

/*
*/
entry_p m_until(entry_p contxt)
{
    return m_whunt(contxt, 1); 
}

/*
`(+ <expr1> <expr2> ...)'
     returns sum of expressions
*/
entry_p m_add (entry_p contxt)
{
    ARGS(0); 
    int s = 0; 
    entry_p *cur = contxt->children; 
    while(*cur && *cur != end())
    {
        s += num(*cur);
        cur++; 
    }
    RNUM(s); 
}

/*
`(< <expr1> <expr2>)'
     less than test (returns 0 or 1)
*/
entry_p m_lt(entry_p contxt)
{
    ARGS(2); 
    RNUM
    (
        ((a1->type == STRING && a2->type == STRING && strcmp(a1->name, a2->name) < 0) ||
         (!(a1->type == STRING && a2->type == STRING) && num(a1) < num(a2))) ? 1 : 0
    );
}

/*
`(<= <expr1> <expr2>)'
     less than or equal test (returns 0 or 1)
*/
entry_p m_lte(entry_p contxt)
{
    ARGS(2); 
    RNUM
    (
        ((a1->type == STRING && a2->type == STRING && strcmp(a1->name, a2->name) <= 0) ||
         (!(a1->type == STRING && a2->type == STRING) && num(a1) <= num(a2))) ? 1 : 0
    );
}

/*
`(< <expr1> <expr2>)'
     greater than test (returns 0 or 1)
*/
entry_p m_gt(entry_p contxt)
{
    ARGS(2); 
    RNUM
    (
        ((a1->type == STRING && a2->type == STRING && strcmp(a1->name, a2->name) > 0) ||
         (!(a1->type == STRING && a2->type == STRING) && num(a1) > num(a2))) ? 1 : 0
    );
}

/*
`(<= <expr1> <expr2>)'
     greater than or equal test (returns 0 or 1)
*/
entry_p m_gte(entry_p contxt)
{
    ARGS(2); 
    RNUM
    (
        ((a1->type == STRING && a2->type == STRING && strcmp(a1->name, a2->name) >= 0) ||
         (!(a1->type == STRING && a2->type == STRING) && num(a1) >= num(a2))) ? 1 : 0
    );
}

/*
`(= <expr1> <expr2>)'
     equality test (returns 0 or 1)
*/
entry_p m_eq(entry_p contxt)
{
    ARGS(2); 
    RNUM
    (
        ((a1->type == STRING && a2->type == STRING && !strcmp(a1->name, a2->name)) ||
         (!(a1->type == STRING && a2->type == STRING) && num(a1) == num(a2))) ? 1 : 0
    );
}

/* 
`(- <expr1> <expr2>)'
     returns `<expr1>' minus `<expr2>'
*/
entry_p m_sub(entry_p contxt)
{
    ARGS(2); 
    RNUM
    (
         num(a1) - num(a2)
    );
}

/*
`(* <expr1> <expr2> ...)'
     returns product of expressions
*/
entry_p m_mul(entry_p contxt)
{
    ARGS(0); 
    int s = 1; 
    entry_p *cur = contxt->children; 
    while(*cur && *cur != end())
    {
        s *= num(*cur);
        cur++; 
    }
    RNUM(s); 
}

/* 
`(/ <expr1> <expr2>)'
     returns `<expr1>' divided by `<expr2>'
*/
entry_p m_div(entry_p contxt)
{
    ARGS(2); 
    int d = num(a2); 
    if(d)
    {
        RNUM(num(a1) / d); 
    }
    else
    {
        error(contxt->id, "Division by zero", 
              contxt->name); 
        return new_failure(); 
    }
}

/* 
`("<fmt>" <expr1> <expr2>)'
     returns a formatted string
*/
entry_p m_fmt(entry_p contxt)
{
    char *ret = NULL, *fmt = contxt->name; 
    char **sct = calloc((strlen(fmt) >> 1) + 1, sizeof(char *));
    if(contxt && fmt && sct)
    {
        size_t i = 0, j = 0, k = 0, l = 0; 
        entry_p *arg = contxt->children; 
        for(; fmt[i]; i++)
        {
            if(fmt[i] == '%')
            {
                if(!i || (i && fmt[i - 1] != '\\'))
                {
                    i++; 
                    if(fmt[i] == 's' || (
                       fmt[i++] == 'l' &&
                       fmt[i] && fmt[i] == 'd'))
                    {
                        sct[k] = calloc(i - j + 2, sizeof(char)); 
                        if(sct[k])
                        {
                            memcpy(sct[k], fmt + j, i - j + 1);
                            j = i + 1;  
                            k++; 
                        }
                        else
                        {
                            error(PANIC);
                        }
                    }
                    else 
                    {
                        error(contxt->id, "Invalid format string", 
                        contxt->name); 
                        break; 
                    }
                }
            }
        }
        if(k)
        {
            for(k = 0; sct[k]; k++)
            {
                if(arg && *arg && 
                   *arg != end())
                {
                    size_t oln = strlen(sct[k]);  
                    entry_p cur = resolve(*arg); 
                    if(sct[k][oln - 1] == 's' &&
                       cur->type == STRING)
                    {
                        size_t nln = oln + strlen(cur->name);  
                        char *new = calloc(nln + 1, sizeof(char)); 
                        if(new)
                        {
                            int n = snprintf(new, nln, sct[k], cur->name); 
                            l += n > 0 ? (size_t) n : 0; 
                            free(sct[k]); 
                            sct[k] = new; 
                        }
                        else
                        {
                            error(PANIC);
                        }
                    }
                    else
                    if(sct[k][oln - 1] == 'd' &&
                       cur->type == NUMBER)
                    {
                        size_t nln = oln + NUMLEN;  
                        char *new = calloc(nln + 1, sizeof(char)); 
                        if(new)
                        {
                            int n = snprintf(new, nln, sct[k], cur->id);  
                            l += n > 0 ? (size_t) n : 0; 
                            free(sct[k]); 
                            sct[k] = new; 
                        }
                        else
                        {
                            error(PANIC);
                        }
                    }
                    else
                    {
                        error(contxt->id, "Format string type mismatch", 
                        contxt->name); 
                    }
                    if(did_error())
                    {
                        arg = NULL; 
                        break;
                    }
                    else
                    {
                        arg++; 
                    }
                }
                else 
                {
                    error(contxt->id, "Missing format string arguments", 
                    contxt->name); 
                    break; 
                }
            }
        }
        if(k)
        {
            l += strlen(fmt + j); 
            ret = calloc(l + 1, sizeof(char)); 
            if(ret)
            {
                for(k = 0; sct[k]; k++)
                {
                    strcat(ret, sct[k]); 
                }
                strcat(ret, fmt + j); 
            }
            else
            {
                error(PANIC);
            }
        }
        for(k = 0; sct[k]; k++)
        {
            free(sct[k]);
        }
        free(sct);
        if(arg && *arg && 
           *arg != end())
        {
            error(contxt->id, "Superfluous format string arguments", 
                  contxt->name); 
        }
        else if(ret)
        {
            RSTR(ret); 
        }
    }
    else
    {
        error(PANIC);
    }
    free(ret);
    return new_failure(); 
}

/* 
`(AND <expr1> <expr2>)'
     returns logical `AND' of `<expr1>' and `<expr2>'
*/
entry_p m_and(entry_p contxt)
{
    ARGS(2); 
    RNUM
    (
        num(a1) && num(a2)
    );
}

/* 
`(OR <expr1> <expr2>)'
     returns logical `OR' of `<expr1>' and `<expr2>'
*/
entry_p m_or(entry_p contxt)
{
    ARGS(2); 
    RNUM
    (
        num(a1) || num(a2)
    );
}

/* 
`(XOR <expr1> <expr2>)'
     returns logical `XOR' of `<expr1>' and `<expr2>'
*/
entry_p m_xor(entry_p contxt)
{
    ARGS(2); 
    RNUM
    (
        (num(a1) && !num(a2)) || 
        (num(a2) && !num(a1)) 
    );
}

/* 
`(NOT <expr>)'
     returns logical `NOT' of `<expr>'
*/
entry_p m_not(entry_p contxt)
{
    ARGS(1); 
    RNUM
    (
         !num(a1)
    );
}

/* 
`(BITAND <expr1> <expr2>)'
     returns bitwise `AND' of `<expr1>' and `<expr2>'
*/
entry_p m_bitand(entry_p contxt)
{
    ARGS(2); 
    RNUM
    (
         num(a1) & num(a2)
    );
}

/* 
`(BITOR <expr1> <expr2>)'
     returns bitwise `OR' of `<expr1>' and `<expr2>'
*/
entry_p m_bitor(entry_p contxt)
{
    ARGS(2); 
    RNUM
    (
         num(a1) | num(a2)
    );
}

/* 
`(BITXOR <expr1> <expr2>)'
     returns bitwise `XOR' of `<expr1>' and `<expr2>'
*/
entry_p m_bitxor(entry_p contxt)
{
    ARGS(2); 
    RNUM
    (
         num(a1) ^ num(a2)
    );
}

/* 
`(BITNOT <expr>)'
     returns bitwise `NOT' of `<expr>'
*/
entry_p m_bitnot(entry_p contxt)
{
    ARGS(1); 
    RNUM
    (
         ~num(a1)
    );
}

/* 
`(shiftleft <number> <amount to shift>)'
     logical shift left
*/
entry_p m_shiftleft(entry_p contxt)
{
    ARGS(2); 
    RNUM
    (
         num(a1) << num(a2)
    );
}

/* 
`(shiftright <number> <amount to shift>)'
     logical shift right
*/
entry_p m_shiftright(entry_p contxt)
{
    ARGS(2); 
    RNUM
    (
         num(a1) >> num(a2)
    );
}

/*
`(IN <expr> <bit-number> <bitnumber>...)'
     returns `<expr>' `AND' bits
*/
entry_p m_in(entry_p contxt)
{
    ARGS(2); 
    if(a2->children)
    {
        int m = 0;  
        entry_p *cur = a2->children; 
        while(*cur && *cur != end())
        {
            m += 1 << num(*cur);
            cur++; 
        }
        RNUM(num(a1) & m); 
    }
    error(PANIC);
    return new_failure();
}

/*
`(askdir (prompt..) (help..) (default..) (newpath) (disk))'
      ask for directory name

GRAMMAR: all options ignored

*/
entry_p m_askdir(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(askfile (prompt..) (help..) (default..) (newpath) (disk))'
     ask for file name

GRAMMAR: all options ignored

*/
entry_p m_askfile(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(askstring (prompt..) (help..) (default..))'
     ask for a string

GRAMMAR: all options ignored

*/
entry_p m_askstring(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(asknumber (prompt..) (help..) (range..) (default..))'
     ask for a number

GRAMMAR: all options ignored

*/
entry_p m_asknumber(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(askchoice (prompt..) (choices..) (default..))'
     choose 1 options

GRAMMAR: all options ignored

*/
entry_p m_askchoice(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(askoptions (prompt (help..) (choices..) default..))'
     choose n options

GRAMMAR: all options ignored

*/
entry_p m_askoptions(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(askbool (prompt..) (help..) (default..) (choices..))'
     0=no, 1=yes

GRAMMAR: all options ignored

*/
entry_p m_askbool(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(askdisk (prompt..) (help..) (dest..) (newname..) (assigns))'

GRAMMAR: all options ignored

*/
entry_p m_askdisk(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(cat <string1> <string2>...)'
     returns concatenation of strings
*/
entry_p m_cat(entry_p contxt)
{
    ARGS(0); 
    char *buf; 
    size_t n = 64; 
    buf = calloc(n + 1, sizeof(char));  
    if(buf)
    {
        size_t l = 0, i = 0; 
        while(contxt->children[i] && 
              contxt->children[i] != end()) 
        {
            size_t ln; 
            const char *s = str(contxt->children[i++]); 
            if(did_error())
            {
                free(buf); 
                return new_failure();
            }
            ln = strlen(s); 
            if(ln)
            {
                l += strlen(s); 
                if(l > n) 
                {
                    char *tmp; 
                    while(n && n < l)
                    {
                        n = n << 1;  
                    }
                    tmp = calloc(n + 1, sizeof(char));  
                    if(tmp && n) 
                    {
                        memcpy(tmp, buf, l - ln + 1); 
                        free(buf); 
                        buf = tmp; 
                    }
                    else
                    {
                        free(tmp); 
                        free(buf); 
                        buf = NULL; 
                        break; 
                    }
                }
                strncat(buf, s, n); 
            }
        } 
        if(buf)
        {
            RSTR(buf); 
        }
    }
    error(PANIC);
    return new_failure();
}

/*
`(exists <filename> (noreq))'
     0 if no, 1 if file, 2 if dir
*/
entry_p m_exists(entry_p contxt)
{
    ARGS(1); 
    struct stat fs; 
    if(!(contxt->children[1] &&
         contxt->children[1] != end() &&
         contxt->children[1]->type == OPTION &&
         contxt->children[1]->id == OPT_NOREQ))
    {
        /* show mount req */
    }
    if(!stat(str(a1), &fs))
    {
        if(S_ISREG(fs.st_mode))
        {
            RNUM(1); 
        }
        if(S_ISDIR(fs.st_mode))
        {
            RNUM(2); 
        }
    }
    RNUM(0); 
}

/*
`(expandpath <path>)'
     Expands a short path to its full path equivalent
*/
entry_p m_expandpath(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(earlier <file1> <file2>)'
     true if file1 earlier than file2
*/
entry_p m_earlier(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(fileonly <path>)'
     return file part of path (see pathonly)
*/
entry_p m_fileonly(entry_p contxt)
{
    ARGS(1); 
    const char *s = str(a1); 
    size_t l = strlen(s), i = l - 1;
    if(l && 
       s[i] != '/' &&
       s[i] != ':' )
    {
        char *r; 
        while(i &&
              s[i - 1] != '/' && 
              s[i - 1] != ':' ) i--;
        r = calloc(l - i + 1, sizeof(char)); 
        if(r)
        {
            memcpy(r, s + i, l - i); 
            RSTR(r); 
        }
        error(PANIC); 
    }
    else
    {
        error(contxt->id, "Not a file", s); 
    }
    return new_failure(); 
}

/*
`(getassign <name> <opts>)'
     return value of logical name (no `:') `<opts>': 'v'-volumes,
     'a'-assigns, 'd'-devices
*/
entry_p m_getassign(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(getdevice <path>)'
     returns name of device upon which <path> resides
*/
entry_p m_getdevice(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(getdiskspace <path>)'
     return available space
*/
entry_p m_getdiskspace(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(getenv <name>)'
     return value of environment variable
*/
entry_p m_getenv(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(getsize <file>)'
     return size
*/
entry_p m_getsize(entry_p contxt)
{
    ARGS(1); 
    const char *n = str(a1); 
    FILE *f = fopen(n, "r"); 
    if(f)
    {
        long s; 
        fseek(f, 0L, SEEK_END);
        s = ftell(f);
        fclose(f); 
        RNUM((int) s); 
    }
    else
    {
        error(contxt->id, "Could not open file", n); 
        return new_failure(); 
    }
}

/*
`(getsum <file>)'
     return checksum of file for comparison purposes
*/
entry_p m_getsum(entry_p contxt)
{
    ARGS(1); 
    FILE *f = fopen(str(a1), "r"); 
    if(f)
    {
        int c = getc(f); 
        int s = 0, n = 1;
        while(c != EOF)
        {
            s -= (c + n);
            c = getc(f); 
            n = ~s; 
        }
        fclose(f); 
        RNUM(s); 
    }
    else
    {
        error(contxt->id, "Could not open file", str(a1)); 
        return new_failure(); 
    }
}

/*
`(getversion <file> (resident))'
     return version/revision of file, library, etc. as 32 bit num

GRAMMAR: (resident) is missing

*/
entry_p m_getversion(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(iconinfo <parameters>)'
      return information about an icon (V42.12)
*/
entry_p m_iconinfo(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
 `(pathonly <path>)'
      return dir part of path (see fileonly)
*/
entry_p m_pathonly(entry_p contxt)
{
    ARGS(1); 
    const char *s = str(a1); 
    size_t i = strlen(s); 
    if(i)
    {
        while(i--) 
        {
            if(s[i] == '/' ||
               s[i] == ':' )
            {
                char *r; 
                r = calloc(i + 2, sizeof(char)); 
                if(r)
                {
                    memcpy(r, s, i + 1); 
                    RSTR(r); 
                }
                error(PANIC); 
                return new_failure(); 
            }
        }
    }
    RSTR(strdup("")); 
}

/*
`(patmatch <pattern> <string>)'
      Does <pattern> match <string> ? TRUE : FALSE
*/
entry_p m_patmatch(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(select <n> <item1> <item2> ...)'
    return n'th item
*/
entry_p m_select(entry_p contxt)
{
    ARGS(2); 
    if(a2->children)
    {
        int i = 0, j = num(a1) - 1; 
        while(a2->children[i] && 
              a2->children[i] != end())
        {
            if(i == j)
            {
                return resolve(a2->children[i]); 
            }
            else
            {
                i++; 
            }
        }
        error(contxt->id, "No such item", 
              contxt->name); 
    }
    else
    {
        error(PANIC); 
    }
    return new_failure(); 
}

/*
`(strlen <string>)'
    returns string length
*/
entry_p m_strlen(entry_p contxt)
{
    ARGS(1); 
    RNUM((int) strlen(str(a1))); 
}

/*
`(substr <string> <start> [<count>])'
    returns a substring of <string>
*/
entry_p m_substr(entry_p contxt)
{
    ARGS(2); 
    if(a2->children &&
       a2->children[0] && 
       a2->children[0] != end()) 
    {
        char *r; 
        const char *s = str(a1);
        size_t n, l = strlen(s); 
        size_t i = num(a2->children[0]) > 0 ? (size_t) 
                   num(a2->children[0]) : 0; 
        i = i >= l ? 0 : i; 
        n = l - i; 
        if(a2->children[1] && 
           a2->children[1] != end()) 
        {
            size_t j = num(a2->children[1]) > 0 ? (size_t) 
                       num(a2->children[1]) : n; 
            n = j < n ? j : n; 
        }
        r = calloc(n + 1, sizeof(char)); 
        if(r)
        {
            memcpy(r, s + i, n); 
            RSTR(r); 
        }
    }
    error(PANIC);
    return new_failure(); 
}

/*
`(symbolset <symbolname> <expression>)'
    assign a value to a variable named by the string result of
    `<symbolname>' (V42.9)
*/
entry_p m_symbolset(entry_p contxt)
{
    ARGS(2); 
    entry_p res = malloc(sizeof(entry_t)); 
    if(res)
    {
        size_t i = 0; 
        const char *n = str(a1); 
        memmove(res, resolve(a2), sizeof(entry_t)); 
        res->name = res->name ? strdup(res->name) : NULL; 
        while(contxt->symbols[i] &&
              contxt->symbols[i] != end())
        {
            if(!strcmp(contxt->symbols[i]->name, n))
            {
                kill(contxt->symbols[i]->resolved); 
                contxt->symbols[i]->resolved = res;
                push(global(contxt), contxt->symbols[i]); 
                res->parent = contxt->symbols[i]; 
                return res; 
            }
            i++; 
        }
        entry_p sym = new_symbol(strdup(n)); 
        if(sym)
        {
            res->parent = sym; 
            sym->resolved = res; 
            if(append(&contxt->symbols, sym))
            {
                push(global(contxt), sym); 
                sym->parent = contxt;  
                return res; 
            }
            kill(sym); 
        }
        else
        {
            kill(res); 
        }
    }
    error(PANIC); 
    return new_failure(); 
}

/*
`(symbolval <symbolname>)'
    returns the value of the symbol named by the string expression
    `<smbolval>' (V42.9)
*/
entry_p m_symbolval(entry_p contxt)
{
    ARGS(1); 
    entry_p ref = new_symref(strdup(str(a1)), contxt->id); 
    if(ref)
    {
        entry_p val; 
        ref->parent = contxt; 
        val = resolve(ref); 
        kill(ref); 
        if(!did_error())
        {
            return val; 
        }
    }
    else
    {
        error(PANIC);
    }
    return new_failure(); 
}

/*
`(tackon <path> <file>)'
     return properly concatenated file to path
*/
entry_p m_tackon(entry_p contxt)
{
    ARGS(2); 
    char *r; 
    const char *p = str(a1), *f = str(a2); 
    size_t lp = strlen(p), lf = strlen(f); 
    if(!lp) 
    {
        RSTR(strdup(f)); 
    }
    if(!lf) 
    {
        RSTR(strdup(p)); 
    }
    if(f[lf - 1] == '/' ||
       f[lf - 1] == ':') 
    {
        error(contxt->id, "Not a file", f); 
        return new_failure(); 
    }
    r = calloc(lp + lf + 2, sizeof(char)); 
    if(r)
    {
        memcpy(r, p, lp); 
        if(p[lp - 1] == '/' ||
           p[lp - 1] == ':') 
        {
            if(f[0] == '/' ||
               f[0] == ':') 
            {
                f++; 
            }
        }
        else
        {
            if(f[0] != '/' && 
               f[0] != ':') 
            {
                strcat(r, "/"); 
            }
        }
        strcat(r, f); 
        RSTR(r); 
    }
    error(PANIC); 
    return new_failure(); 
}

/*
`(transcript <string1> <string2>)'
     puts concatenated strings in log file
*/
entry_p m_transcript(entry_p contxt)
{
    if(contxt && 
       contxt->children)
    {
        size_t len = 0; 
        const char *log = "default.log"; 
        for(size_t i = 0; 
            contxt->children[i] &&
            contxt->children[i] != end(); 
            i++)
        {
            entry_p arg = contxt->children[i]; 
            if(arg->type == OPTION)
            {
                if(arg->id == OPT_OVERRIDE &&
                   arg->children) 
                {
                    log = str(arg->children[0]); 
                }
                else
                {
                    error(contxt->id, "Invalid option", 
                          arg->name); 
                    return new_failure(); 
                }
            }
            else
            {
                len += strlen(str(arg)); 
            }
        }
        char *buf = calloc(len + 2, sizeof(char));
        if(buf)
        {
            for(size_t i = 0; 
                contxt->children[i] &&
                contxt->children[i] != end(); 
                i++)
            {
                entry_p arg = contxt->children[i]; 
                if(arg->type != OPTION)
                {
                    strcat(buf, str(arg)); 
                }
            }
            FILE *fp = fopen(log, "a"); 
            if(fp)
            {
                buf[len] = '\n'; 
                if(fwrite(buf, sizeof(char), 
                   len + 1, fp) == len + 1)
                {
                    fclose(fp); 
                    free(buf); 
                    RNUM(1);
                }
                fclose(fp); 
            }
            free(buf); 
            RNUM(0); 
        }
    }
    error(PANIC); 
    return new_failure(); 
}

/*
`(makedir <name> (prompt..) (help..) (infos) (confirm..) (safe))'
     make a directory
*/
entry_p m_makedir(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(copyfiles (prompt..) (help..) (source..) (dest..) (newname..) (choices..)'
     `(all) (pattern..) (files) (infos) (confirm..) (safe) (optional
     <option> <option> ...) (delopts <option> <option> ...) (nogauge))'

     copy files (and subdir's by default).  files option say NO
     subdirectories
*/
entry_p m_copyfiles(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(copylib (prompt..) (help..) (source..) (dest..) (newname..) (infos) (confirm)'
     `(safe) (optional <option> <option> ...) (delopts <option>
     <option> ...) (nogauge))'

     install a library if newer version
*/
entry_p m_copylib(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(startup (prompt..) (command..))'
     add a command to the boot scripts (startup-sequence, user-startup)
*/
entry_p m_startup(entry_p contxt)
{
    ARGS(2); 
    entry_p help = NULL, prompt = NULL, confirm = NULL, 
            command = NULL, override = NULL;
    if(contxt->children[2] &&
       contxt->children[2] != end())
    {
        entry_p opt = contxt->children[2]; 
        if(opt->type == CONTXT && 
           opt->children)
        {
            for(size_t i = 0; 
                opt->children[i] &&
                opt->children[i] != end() &&
                opt->children[i]->type == OPTION;
                i++)
            {
                switch(opt->children[i]->id)
                {
                    case OPT_HELP: 
                        help = opt->children[i];
                        break;
                    case OPT_PROMPT: 
                        prompt = opt->children[i];
                        break;
                    case OPT_CONFIRM: 
                        confirm = opt->children[i];
                        break;
                    case OPT_COMMAND: 
                        command = opt->children[i];
                        break;
                    case OPT_OVERRIDE: 
                        override = opt->children[i];
                        break;
                    default: 
                        error(contxt->id, "Invalid option", 
                              opt->children[i]->name); 
                        return new_failure(); 
                }
            }
        }
        else
        {
            error(PANIC); 
            return new_failure(); 
        }
    }
/*
 Hantera OPTS!
*/
    if(a2->type == OPTION && 
       a2->id == OPT_COMMAND &&
       a2->children)
    {
        const char *fln = override ? str(override->children[0]) : "s:user-startup";
        const char *app = str(a1), *cmd = str(a2->children[0]);
        FILE *fp = fopen(fln, "r"); 
        if(fp)
        {
            size_t al = strlen(app) + 2; 
            if(al < 3)
            {
                fclose(fp); 
                error(contxt->id, "Invalid application name", "NULL"); 
                RNUM(0);
            }
            else
            {
                char *fnd, *buf;
                size_t fl, cl = strlen(cmd); 
                fseek(fp, 0L, SEEK_END);
                fl = ftell(fp);
                fnd = calloc(al + 1, sizeof(char)); 
                buf = calloc(fl + 2 * (al + 1) + cl + 2, sizeof(char)); 
                if(buf && fnd)
                {
                    fseek(fp, 0L, SEEK_SET);
                    sprintf(fnd, ";%s\n", app); 
                    if(fread(buf, sizeof(char), fl, fp) == fl)
                    {
                        char *b = strstr(buf, fnd),
                             *e = b ? strstr(b + al, fnd) : NULL; 
                        b += al; 
                        fclose(fp); 
                        if(e && b)
                        {
                            fp = fopen(fln, "w"); 
                            if(fp)
                            {
                                memmove(b + cl + 1, e, buf + fl - e); 
                                fl -= e - b - cl - 1;
                                memcpy(b, cmd, cl); 
                                b[cl] = '\n';
                                if(fwrite(buf, sizeof(char), fl, fp) == fl)
                                {
                                    fclose(fp); 
                                    free(fnd); 
                                    free(buf); 
                                    RNUM(1);
                                }
                            }
                        }
                        else
                        {
                            fp = fopen(fln, "a"); 
                            if(fp)
                            {
                                if(fprintf(fp, "%s%s\n%s", 
                                   fnd, cmd, fnd) > 0)
                                {
                                    fclose(fp); 
                                    free(fnd); 
                                    free(buf); 
                                    RNUM(1);
                                }
                            }
                        }
                        error(contxt->id, "Could not write to file", fln); 
                    }
                }
                if(fp)
                {
                    fclose(fp); 
                }
                free(buf);
                free(fnd);
            }
        }
        else
        {
            error(contxt->id, "Could not read from file", fln); 
        }
    }
    else
    {
        error(PANIC); 
    }
    RNUM(0);
}

/*
`(tooltype (prompt..) (help..) (dest..) (settooltype..) (setstack..)'
     `(setdefaulttool..) (noposition) (confirm..) (safe))'

     modify an icon
*/
entry_p m_tooltype(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(textfile (prompt..) (help..) (dest..) (append) (include..) (confirm..) (safe))'
     create text file from other text files and strings
*/
entry_p m_textfile(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(execute <arg> (help..) (prompt..) (confirm) (safe))'
     execute script file
*/
entry_p m_execute(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(run <arg> (help..) (prompt..) (confirm..) (safe))'
     execute program
*/
entry_p m_run(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(rexx <arg> (help..) (prompt..) (confirm..) (safe))'
     execute ARexx script
*/
entry_p m_rexx(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(makeassign <assign> <path> (safe)) ; note: <assign> doesn't need `:''
     create an assignment
*/
entry_p m_makeassign(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(rename <old> <new> (help..) (prompt..) (confirm..) (safe))'
     rename files
*/
entry_p m_rename(entry_p contxt)
{
    ARGS(2); 
    entry_p help = NULL, prompt = NULL, 
            confirm = NULL, safe = NULL; 
    if(contxt->children[2] &&
       contxt->children[2] != end())
    {
        entry_p opt = contxt->children[2]; 
        if(opt->type == CONTXT && 
           opt->children)
        {
            for(size_t i = 0; 
                opt->children[i] &&
                opt->children[i] != end() &&
                opt->children[i]->type == OPTION;
                i++)
            {
                switch(opt->children[i]->id)
                {
                    case OPT_HELP: 
                        help = opt->children[i];
                        break;
                    case OPT_PROMPT: 
                        prompt = opt->children[i];
                        break;
                    case OPT_CONFIRM: 
                        confirm = opt->children[i];
                        break;
                    case OPT_SAFE: 
                        safe = opt->children[i];
                        break;
                    default: 
                        error(contxt->id, "Invalid option", 
                              opt->children[i]->name); 
                        return new_failure(); 
                }
            }
        }
        else
        {
            error(PANIC); 
            return new_failure(); 
        }
    }
/*

 Hantera OPTS!

*/
    if(rename(str(a1), str(a2)) == 0)
    {
        RNUM(1); 
    }
    error(contxt->id, "Could not rename file", str(a1)); 
    RNUM(0); 
}

/*
`(delete file (help..) (prompt..) (confirm..) (infos) (optional <option> <option> ...) (all)'
     `(delopts <option> <option> ...) (safe))'

     delete file
*/
entry_p m_delete(entry_p contxt)
{
    ARGS(1); 
    entry_p help = NULL, prompt = NULL, 
            confirm = NULL, infos = NULL, 
            optional = NULL, all = NULL, 
            delopts = NULL, safe = NULL; 
    if(contxt->children[1] &&
       contxt->children[1] != end())
    {
        entry_p opt = contxt->children[1]; 
        if(opt->type == CONTXT && 
           opt->children)
        {
            for(size_t i = 0; 
                opt->children[i] &&
                opt->children[i] != end() &&
                opt->children[i]->type == OPTION;
                i++)
            {
                switch(opt->children[i]->id)
                {
                    case OPT_HELP: 
                        help = opt->children[i];
                        break;
                    case OPT_PROMPT: 
                        prompt = opt->children[i];
                        break;
                    case OPT_CONFIRM: 
                        confirm = opt->children[i];
                        break;
                    case OPT_INFOS: 
                        infos = opt->children[i];
                        break;
                    case OPT_OPTIONAL: 
                        optional = opt->children[i];
                        break;
                    case OPT_ALL: 
                        all = opt->children[i];
                        break;
                    case OPT_DELOPTS: 
                        delopts = opt->children[i];
                        break;
                    case OPT_SAFE: 
                        safe = opt->children[i];
                        break;
                    default: 
                        error(contxt->id, "Invalid option", 
                              opt->children[i]->name); 
                        return new_failure(); 
                }
            }
        }
        else
        {
            error(PANIC); 
            return new_failure(); 
        }
    }
/*

 Hantera OPTS!

*/
    if(remove(str(a1)) == 0)
    {
        RNUM(1); 
    }
    RNUM(0); 
}

/*
`(protect <file> [<string of flags to change>] [<decimal mask>] <parameters>)'
     get/set file protection flags
*/
entry_p m_protect(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(complete <num>)'
     display percentage through install in titlebar
*/
entry_p m_complete(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(message <string1> <string2>... (all))'
     display message with Proceed, Abort buttons
*/
entry_p m_message(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(working)'
     indicate to user that Installer is busy doing things
*/
entry_p m_working(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(welcome <string> <string> ...)'
     allow Installation to commence.
*/
entry_p m_welcome(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(foreach <dir> <pattern> <statements>)'
     do for entries in directory
*/
entry_p m_foreach(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(abort <string1> <string2> ...)'
     abandon installation
*/
entry_p m_abort(entry_p contxt)
{
    ARGS(0); 
    entry_p *cur = contxt->children; 
    while(*cur && *cur != end())
    {
        /* show message */
        cur++; 
    }
    error(-3, "Abort", __func__); 
    return new_failure(); 
}

/*
`(exit <string> <string> ... (quiet))'
     end installation after displaying strings (if provided)
*/
entry_p m_exit(entry_p contxt)
{
    if(contxt)
    {
        if(contxt->children)
        {
            for(size_t i = 0; 
                contxt->children[i] &&
                contxt->children[i] != end(); i++)
            {
                if(contxt->children[i]->type == OPTION && 
                   contxt->children[i]->id == OPT_QUIET)
                {
                    error(HALT); 
                    RNUM(0); 
                }
            }
            for(size_t i = 0; 
                contxt->children[i] &&
                contxt->children[i] != end(); i++)
            {
                /*
                printf("%s\n", str(contxt->children[i])); 

                This causes normal termination of a script.  If strings are
                provided, they are displayed.  The "done with installation" message is
                then displayed.  The "onerror" statements are not executed.  If (quiet)
                is specified, the final report display is skipped.

                MISSING
                */
            }
        }
        error(HALT); 
        RNUM(0); 
    }
    error(PANIC);
    RNUM(1); 
}

/*
`(trap <flags> <statements>)'
     trap errors.  flags: 1-abort, 2-nomem, 3-error, 4-dos, 5-badargs
*/
entry_p m_trap(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(onerror (<statements>))'
     general error trap

 ------------------------------------------------------
 In part implemented using m_procedure. This function
 just invokes the @onerror custom procedure inserted
 using (onerror) which is a special case of (procedure)
 ------------------------------------------------------
*/
entry_p m_onerror(entry_p contxt)
{
    if(contxt)
    {
        entry_p handler = new_cusref(strdup("@onerror"), __LINE__, NULL);
        push(contxt, handler);
        return m_gosub(handler); 
    }
    error(PANIC);  
    return new_failure(); 
}

/*
`(user <user-level>)'
   change the user level (debugging purposes only)

   -----------------------
   Implemented using m_set
   -----------------------
*/

/*
`(debug <anything> <anything> ...)'
    print to stdout when running from a shell
*/
entry_p m_debug(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

/*
`(database <feature> [<checkvalue>])'
    return information about the Amiga that the Installer 
    is running on.  
*/
entry_p m_database(entry_p contxt)
{
    (void) contxt; 
    error(MISS); 
    return new_failure(); 
}

