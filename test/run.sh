run()
{
    instfile=`mktemp ./installer.tmp.XXXXXX`
    echo $1 > $instfile
    if [ `uname` = "Linux" ]; then
        l=`mktemp ./leak.tmp.XXXXXX`
        o=`valgrind --errors-for-leak-kinds=all --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 $prg $instfile 2>$l`
        if [ $? -ne 0 ]; then
            s=`mktemp ./leak.tmp.XXXXXX`
            cat $l | sed -r 's/==[0-9]+/;==/g' >> $s
            cat $instfile >> $s
            return 2
        fi
        e=`cat $l | grep -v "^==[0-9]\+=="`
        if [ -n "$e" ]; then
            o="$e $o"
        fi
        rm $l
    else
        o=`$prg $instfile 2>&1`
    fi
    rm $instfile
    o=`echo $o | tr -d  '\n'`
    if [ "$o" = "$2" ]; then
        return 1
    else
        l=`mktemp ./err.tmp.XXXXXX`
        echo "; In line $3" > $l
        echo $1 >> $l
        echo "; Actual result [$o]" >> $l
        echo "; Expected result [$2]" >> $l
        echo "" >> $l
        return 0
    fi
}

evl()
{
    ret=1
    pre=`echo "$2" | sed -e 's/.*\"\(.*\)\",\".*\",\".*\".*$/\1/'`
    inf=`echo "$2" | sed -e 's/.*\".*\",\"\(.*\)\",\".*\".*$/\1/'`
    pst=`echo "$2" | sed -e 's/.*\".*\",\".*\",\"\(.*\)\".*$/\1/'`
    if [ -n "$pre" ]; then
        o=`eval "$pre" 2>&1`
        if [ $? -ne 0 ]; then
            echo "ERR:$o"
            return 0
        fi
    fi
    run "$1 ; [$pre ; $pst]" "$inf" "$3" "$4"
    ret=$?
    if [ -n "$pst" ]; then
        o=`eval "$pst" 2>&1`
        if [ $? -ne 0 ]; then
            echo "In line $3 FAIL/ERR:$o"
            if [ $ret -eq 1 ]; then
                ret=0
            fi
        fi
    fi
    return $ret
}

prg=$1
tst=$2
oom=$3
nfl=0
nok=0

for f in `ls -t $tst/test.*`;
do
    tno=0
    echo "-> $f"
    while read -r l;
    do
       p=`echo "$l" | sed -e 's/;.*$//'`
       r=`echo "$l" | sed -e 's/.*;//'`
       if [ ! -z "$p" ]; then
           tno=$(( $tno + 1 ))
           evl "$p" "$r" "$tno"
           s=$?
           p=$(echo $p | tr '\n' ' ')
           if [ $s -eq 2 ]; then
               echo "LEAK -> $p"
               nok=$(( $nok + 1 ))
               exit 1
           elif [ $s -eq 1 ]; then
               echo "OK -> $p" 
               nok=$(( $nok + 1 ))
           elif [ $s -eq 0 ]; then
               if [ ! -z "$oom" ]; then
                   echo "IGNORE -> $p"
                   nok=$(( $nok + 1 ))
               else
                   echo "FAIL -> $p"
                   nfl=$(( $nfl + 1 ))
               fi
           fi
       fi
    done < $f
done
tot=$(( $nok + $nfl ))
echo "--------------------------------------------"
if [ $nfl -eq 0 ]; then
    echo "All $tot test(s) passed"
else
    echo "$nfl test(s) failed"
    echo "$nok test(s) passed"
fi
echo "--------------------------------------------"

