(rexx "script") ; "","scriptrx0",""
(rexx "script" "x") ; "","scriptxrx0",""
(rexx "script" (safe)) ; "","scriptrx0",""
(rexx (safe) "trash" "my" "script") ; "","trashmyscriptrx0",""
(rexx "script" (safe) (quiet)) ; "","scriptrx0",""
(rexx "trash" " my " "script" (safe) (quiet)) ; "","trash my scriptrx0",""
(rexx (safe) (quiet) "script") ; "","scriptrx0",""
(rexx (safe) "script" (quiet)) ; "","scriptrx0",""
(rexx "script" "x" (safe) (quiet)) ; "","scriptxrx0",""
(rexx) ; "","Line 1: syntax error, unexpected ')' ')'",""
