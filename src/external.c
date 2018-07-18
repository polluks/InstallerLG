//----------------------------------------------------------------------------
// external.c: 
//
// Execution of external scripts / binaries
//----------------------------------------------------------------------------
// Copyright (C) 2018, Ola Söder. All rights reserved.
// Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
//----------------------------------------------------------------------------

#include "alloc.h"
#include "error.h"
#include "eval.h"
#include "external.h"
#include "file.h"
#include "gui.h"
#include "resource.h"
#include "util.h"

#include <stdio.h>
#include <unistd.h>

#ifdef AMIGA
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#endif

#include <string.h>

//----------------------------------------------------------------------------
// h_run - m_run / m_execute / m_rexx helper
//----------------------------------------------------------------------------
static entry_p h_run(entry_p contxt, const char *pre, const char *dir)
{
    // We need atleast one argument
    if(c_sane(contxt, 1))
    {
        entry_p prompt   = get_opt_va(OPT_PROMPT, CARG(2), CARG(3), NULL),
                help     = get_opt_va(OPT_HELP, CARG(2), CARG(3), NULL),
                confirm  = get_opt_va(OPT_CONFIRM, CARG(2), CARG(3), NULL),
                safe     = get_opt_va(OPT_SAFE, CARG(2), CARG(3), NULL); 

        DNUM = 0; 

        // Find out if we need confirmation...
        if(confirm)
        {
            // The default threshold is expert.
            int level = get_numvar(contxt, "@user-level"); 
            int th = 2;

            // If the (confirm ...) option contains 
            // something that can be translated into
            // a new threshold value...
            if(confirm->children && 
               confirm->children[0] && 
               confirm->children[0] != end())
            {
                // ...then do so.
                th = num(confirm->children[0]);
            }
                            
            // If we are below the threshold value,
            // don't care about getting confirmation
            // from the user.
            if(level < th) 
            {
                confirm = NULL; 
            }

            // Make sure that we have the prompt and
            // help texts that we need if 'confirm'
            // is set. It's not strictly necessary 
            // if 'confirm' is not set, but it's not
            // valid code so lets fail anyway.
            if(!prompt || !help)
            {
                error(contxt->id, ERR_MISSING_OPTION, 
                      prompt ? "help" : "prompt"); 
                RCUR; 
            }
        }

        // Did we need it? (confirmation)
        if(confirm)
        {
            int c = gui_confirm(str(prompt), str(help));

            // HALT if abort
            if(c == -1)
            {
                error(HALT); 
            } 

            // Abort or skip
            if(c != 1)
            {
                RCUR; 
            }
        }

        // Is this call safe to run or are we not 
        // in pretend mode? 
        if(safe || !get_numvar(contxt, "@pretend"))
        {
            // Command / script. Merge all.
            char *cmd = get_chlstr(contxt); 

            if(cmd)
            {
                #ifdef AMIGA
                // Not sure we need input and output but let's 
                // make sure that we're not getting something
                // we don't want / expect.
                BPTR inp = (BPTR) Open("NIL:", MODE_OLDFILE),
                     out = (BPTR) Open("NIL:", MODE_NEWFILE); 

                // Can this not be true? 
                if(inp && out)
                {
                    // Working dir.
                    char *cwd = NULL; 

                    // If we have a valid destination dir,
                    // change to that directory. We're not
                    // treating errors as such. 
                    if(dir && *dir && h_exists(dir))
                    {
                        // Use the global buffer.
                        char *buf = get_buf();

                        // Try to get current working dir.
                        if(getcwd(buf, buf_size()) == buf)
                        {
                            // Try to change to the new dir
                            // and save the old one so that
                            // we can go back afterwards.
                            if(!chdir(dir))
                            {
                                cwd = buf;
                            }
                        }
                    }

                    // DOS / Arexx script?
                    if(pre)
                    {
                        size_t cl = strlen(cmd) + strlen(pre) + 2;
                        char *t = malloc(cl); 

                        if(t)
                        {
                            // Prepend prefix to command string.
                            snprintf(t, cl, "%s %s", pre, cmd); 
                            free(cmd);
                            cmd = t;
                        }
                        else
                        {
                            // Out of memory
                            error(PANIC); 
                            free(cmd); 

                            // Necessary? 
                            Close(inp); 
                            Close(out); 

                            // Bail.
                            RCUR; 
                        }
                    } 

                    // Execute whatever we have in cmd. 
                    DNUM = SystemTags
                    (
                        cmd, 
                        SYS_Input, inp, 
                        SYS_Output, out, 
                        TAG_END
                    ); 

                    // Get and set secondary status. 
                    LONG ioe = IoErr(); 
                    set_numvar(contxt, "@ioerr", ioe); 

                    // Have we changed the working dir?
                    if(cwd)
                    {
                        // Go back to where we started. 
                        chdir(cwd); 
                    }

                    // Necessary? 
                    Close(inp); 
                    Close(out); 

                    // We should have all zeroes
                    if(DNUM || ioe)
                    {
                        // Give IoErr priority.
                        DNUM = ioe ? ioe : DNUM;

                        // Only fail if we're in 'strict' mode.
                        if(get_numvar(contxt, "@strict"))
                        {
                            error(contxt->id, ERR_EXEC, cmd); 
                        }
                    } 
                } 
                else
                {
                    // Unknown error.
                    error(PANIC); 
                    free(cmd); 
                    RCUR;  
                }
                #else
                // For testing purposes only.
                printf("%s%s%s", cmd,
                        pre ? pre : "",
                        dir ? dir : "");
                #endif
                    
                // Free concatenation.
                free(cmd); 
            }
            else
            {
                // OOM.
                error(PANIC); 
                RCUR;  
            }
        }

        // Write an explanation of what we just did /
        // tried to do to the log file.
        h_log(contxt, tr(S_XCTD), str(CARG(1))); 
    }
    else
    {
        // The parser is broken
        error(PANIC); 
    }

    // Return whatever we have, 
    // a success or a failure.
    RCUR;  
}

//----------------------------------------------------------------------------
// (execute <arg> (help..) (prompt..) (confirm) (safe))
//     execute script file
//
// Refer to Installer.guide 1.19 (29.4.96) 1995-96 by ESCOM AG
//----------------------------------------------------------------------------
entry_p m_execute(entry_p contxt)
{
    return h_run
    (
        contxt, "execute",
        get_strvar(contxt, "@execute-dir")
    );
}

//----------------------------------------------------------------------------
// (rexx <arg> (help..) (prompt..) (confirm..) (safe))
//     execute ARexx script
//
// Refer to Installer.guide 1.19 (29.4.96) 1995-96 by ESCOM AG
//----------------------------------------------------------------------------
entry_p m_rexx(entry_p contxt)
{
    return h_run(contxt, "rx", NULL); 
}

//----------------------------------------------------------------------------
// (run <arg> (help..) (prompt..) (confirm..) (safe))
//     execute program
//
// Refer to Installer.guide 1.19 (29.4.96) 1995-96 by ESCOM AG
//----------------------------------------------------------------------------
entry_p m_run(entry_p contxt)
{
    return h_run
    (
        contxt, NULL,
        get_strvar(contxt, "@execute-dir")
    );
}
