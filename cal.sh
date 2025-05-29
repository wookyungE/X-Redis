FILE="65536.txt"
#FILE="32768.txt"
#FILE="4096.txt"
ofile="test"
NUM="22 23 24 25"

total_time=0
for n in $NUM; do
    l=0
    grep "$n," $FILE > $ofile
    line=`wc -l $ofile | awk '{print $1}'` 
    while [[ $l -lt $line ]] ; do 
        end=`cat $ofile | head -n $((l+2)) | tail -n 1 | awk -F ', ' '{print $3}' `
        start=`cat $ofile | head -n $((l+1)) | tail -n 1 | awk -F ', ' '{print $3}' `

        l=$((l + 2))
        temp=$((end - start))
        total_time=$((total_time + temp))
        echo $total_time
    done
done

