#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "alloc.h"
#include "native.h"

entry_p new_s ()
{
    int default_size = 64; 
    return new_native (0, default_size); 
}

entry_p new_native (call_t call, int nargs)
{
    entry_p entry = malloc (sizeof (entry_t)); 
    if (entry)
    {
        entry->value.native.args = calloc (nargs + 1, sizeof (entry_p));
        if (entry->value.native.args)
        {
            entry->type = NATIVE;
            entry->value.native.call = call;
            entry->value.native.args[nargs] = entry; 
            return entry;
        }
    }
    free (entry);
    return 0;
}

entry_p new_number (int n)
{
    entry_p entry = malloc (sizeof (entry_t)); 
    if (entry)
    {
        entry->type = NUMBER;
        entry->value.number = n;
    }
    else
    {
        // panic
    }
    return entry;
}

void push (entry_p dst, entry_p src)
{
    int u = used (dst);
    if (!left (dst))
    {
        int new_size = 1 + (u << 1); 
        entry_p *new_args = calloc (new_size, sizeof (entry_p));
        if (new_args) 
        {
            memmove (new_args, dst->value.native.args, u * sizeof (entry_p)); 
            free (*dst->value.native.args); 
            new_args[new_size - 1] = dst; 
            dst->value.native.args = new_args; 
        }
        else
        {
            // panic
            return; 
        }
    }
    dst->value.native.args[u] = src; 
}

void kill (entry_p entry)
{
    if (entry)
    {
    }
}

static int n_true (entry_p entry, int t)
{
    int n = 0; 
    int f = t ? 0 : 1; 

    if (entry->type == NATIVE)
    {
        int i = 0; 
        while (entry->value.native.args[i] != entry)
        {
            n += entry->value.native.args[i] ? t : f; 
            i++;
        }
    }
    return n;
}

int used (entry_p entry)
{
    return n_true (entry, 1); 
}

int left (entry_p entry)
{
    return n_true (entry, 0); 
}
