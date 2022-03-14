function has-space {
    [[ "$1" != "${1%[[:space:]]*}" ]] && return 0 || return 1
}
Suite=""
test -f testList.txt && rm testList.txt
test -f runTests.sh && rm runTests.sh
./main --gtest_list_tests > testList.txt
while IFS= read -r line
do
    if has-space "$line" ; then
    
    var="${line#"${line%%[![:space:]]*}"}"
    echo "./main --gtest_filter=$Suite$var" >> runTests.sh
    else
    Suite=$line
    fi

done < "testList.txt"
test -f testList.txt && rm testList.txt
chmod +x runTests.sh