//----------------------------------------------------------------------------
// resource.c: 
//
// Encapsulation of resources that we might want to localize later on.
//----------------------------------------------------------------------------

#include "error.h"
#include "resource.h"

//----------------------------------------------------------------------------
// Name:        tr(res_t r)
// Description: Get string from string ID. 
// Input:       A res_t string ID. 
// Return:      The string corresponding to the string ID. 
//----------------------------------------------------------------------------
const char *tr(res_t r)
{
    // Fail nicely if we're out of range.
    res_t i = r > S_GONE ? S_GONE : r;

    // res_t -> string mappings.
    static const char *res[] =
    {
        // Out of range.
        "NONE", 

        "Copying file(s)", 
        "Overwrite existing directory?",
        "Abort copy '%s' to '%s'.\n", 
        "Could not copy '%s' to '%s'.\n", 
        "Delete write protected file?", 
        "Delete file?", 
        "Deleted '%s'.\n",
        "No such file '%s'.\n",
        "Renamed '%s' to '%s'.\n",
        "Copied '%s' to '%s'.\n",
        "Directory '%s' exists already.\n",
        "Created '%s'.\n",
        "Get mask '%s' %d\n",
        "Protect '%s' %d\n",
        "Installer",
        "Set Installation Mode",
        "Destination drawer",
        "Copying file",
        "Files to install",
        "Installation mode and logging",
        "Progress",
        "Proceed",
        "Retry",
        "Abort",
        "Yes",
        "No",
        "Ok",
        "Next",
        "Skip",
        "Unknown error",
        "Setup failed",
        "Failed creating MUI custom class",
        "Failed creating MUI application",
        "Initialization failed",
        "Error",
        "Success",
        "Line %d: %s '%s'",
        "Novice User - All Actions Automatic",
        "Intermediate User - Limited Manual Control",
        "Expert User - Must Confirm all actions",
        "Install for real",
        "Dry run only",
        "No logging",
        "Log to file",
        "installer: required argument missing\n",
        "%s\n\n%s can be found in %s",
        "Installation complete",
        "Executed '%s'.\n",
        "Error: Could not open file '%s'\n",
        "Line %d: %s '%s'\n",
        "Working on installation...\n\n",
        "Select directory",
        "Select file",
        "Assign '%s' to '%s'\n",
        "Remove assign '%s'\n",
        "Could not assign '%s' to '%s'\n",
        "Could not remove assign '%s'\n",

        // Out of range.
        "GONE"
    };

    // res[i] is a valid string. 
    return res[i]; 
}
