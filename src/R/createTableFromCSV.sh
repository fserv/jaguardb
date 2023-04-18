#!/bin/bash

#######################################################################################
##  Shell script to read a CSV file and create schema file for Jaguar database
##
##  Usage: createTableFromCSV.sh  <inputfile.csv>
##
##  DataJaguar, Inc. All Rights Reserved.
##  Author: Xiyang Lu
##
#######################################################################################
#remove empty line
sed '/^\s*$/d' -i $1
#choose the file to read

fname=$1
#save the origin file as origin_oldfilename.csv
#oldfname="origin_${fname}"
#cp $1  $oldfname

if [[ "x$fname" = "x" ]]; then
    echo "Usage: $0  <inputfile.csv>"
    echo "Please provide the input csv file"
    exit 1
fi

if [[ ! -f "$fname" ]]; then
    echo "Error: $fname does not exist"
    exit 1
fi

### Table name is first part of input file name
table_name=`basename $fname|awk -F'.' '{print $1}'`


tmpfname="${fname}.2820219$$"
tr '\r' '\n' < $fname > $tmpfname
/bin/mv -f $tmpfname  $fname




###move unique column to fist
#unfname="${fname}.11$$"
unfname="new_${fname}"
#/bin/rm -f $unfname
count=1
total=`head -1 $fname|tr -d '\r'| tr -d \"`
OLD_IFS="$IFS"
IFS=","
totalCol=($total)
IFS="$OLD_IFS"

while [ "$count" -le  ${#totalCol[@]} ]
do
  dup=`cut -d ',' -f$count $fname | sort | uniq -cd | wc -l`  #print duplicate row number
  if [ $dup == 0 ]
  then 
#  echo "column $count has no duplicate records "
  break
  fi
count=`expr $count + 1`
done

#if all column are not unique.just use the origin csv
if [ $count -le  ${#totalCol[@]} ]
then
#unfname="${fname}_new.csv"
awk -v count=$count -F,  'function pr(str, nth, T)
  { split(str, arr);
    if ( nth > T ) { return; }
    if ( 1 == nth ) { print str; return; }
    printf "%s,", arr[nth];
    for ( i=1; i<=nth-1; i++) {printf "%s,",arr[i];}
    for ( i=nth+1; i<=T; i++) {if (i<T) printf "%s,",arr[i]; else printf "%s",arr[i]; }
    print "";
  }
  #BEGIN{ print "begin processing:" } 
  {pr($0,count,NF);}'  $1 > $unfname

#./a.awk $1 $unfname $count
#/bin/mv -f $unfname  $fname
fi

#echo "[$fname] [$unfname] [$count]"


my_title=`head -1 $fname|tr -d '\r'| tr -d \"`
#my_title is a string with ',' as delimiter
#OLD_IFS: field splitting-- use it to split my_title into array1
OLD_IFS="$IFS"
IFS=","
array1=($my_title)
IFS="$OLD_IFS"



###Calculate number of rows and columns
tmpfname="${fname}.2820219$$"
num_col=${#array1[@]}
num_row=`wc -l $fname | awk '{print $1}'`
((an=num_row-1))
#tail -n $an $fname | shuf -n 1000 | tr -d '\r' > $tmpfname
tail -n $an $fname | shuf -n 1000 > $tmpfname 
#get all rows except firstline,then randomly pick 1000 row from all and store into tmpfile
fname=$tmpfname




###Echo the title column into array
declare -a first_result
for i in "${!array1[@]}"; do
  first_result[$i]=${array1[$i]// /_}
done

#declare data type and data result
declare -a data_result
#assign data type
char="char"
int="int"
date="date"
numeric="numeric"
datetime="datetime"
double="double"
#determine the data type of examples
#try to use awk to set field delimiter

n=1
while read line; do
  n=`expr $n + 1`
  example_data=`echo $line | awk -v v='"' 'BEGIN{FS=OFS=v}{gsub(","," ",$2); print}'`
  OLD_IFS="$IFS"
  IFS=","
  array2=($example_data)
  IFS="$OLD_IFS"
  for ((j = 0; j < ${#array1[@]}; j++)); do
      result1=$(echo "${array2[$j]}" | grep -Eo "[[:digit:]]+" | wc -l|cut -d' ' -f1) #remove char except digit
      result2=$(echo "${array2[$j]}" | grep -Eo "[[:alpha:]]+|\/" | wc -l|cut -d ' ' -f1) #remove char except alpha
#echo ${array2[$j]} $result1 $result2
      #determine datatype
      data_result[$j]=$char
      if [ $result1 -eq 0 -a $result2 -eq 0 ]; then #neither number nor alpha, probably special variable
        data_result[$j]=$char
      elif [ $result1 -gt 0 -a $result2 -gt 0 ]; then #contain both digits and alpha
        data_result[$j]=$char
      elif [[ $result2 -eq 0 ]]; then #no alpha, could be number, double, or time
        if [[ `echo "${array2[$j]}" | sed 's/[^\.]//g'` ]];then
          data_result[$j]=$double
        elif [[ `echo "${array2[$j]}" | sed 's/[0-9]+//g'` ]];then
          data_result[$j]=$int
        elif [[ `echo "${array2[$j]}" | grep '^[0-9]{1,2}-[0-9a-zA-Z]{1,3}-[0-9]{4}$'` ]]; then
          data_result[$j]=$date
        elif [[ `echo "${array2[$j]}" | grep '^[1-9][0-9]/{3/}-[0-1][0-9]-[0-9]/{2/}\ [0-2][0-9]:[0-6][0-9]:[0-6][0-9]$'` ]]; then
          data_result[$j]=$datetime
        elif [[  ! -z  $(echo ${array2[$j]} | sed 's/[^.]//g') ]]; then
          data_result[$j]=$numeric
        fi
      elif [[ $result1 -eq 0 ]]; then #no digits
        data_result[$j]=$char
      fi
  done
  if (($n > 18)); then # get first 18 lines from 1000 sample records
    break
  fi
done < $fname


#find data_length
declare -a data_length=( $(for i in $(seq 1 $num_col); do echo 0; done) )
declare -a data_decimal=( $(for i in $(seq 1 $num_col); do echo 0; done) )
m=1
while read line; do
  example_data=`echo $line | awk -v v='"' 'BEGIN{FS=OFS=v}{gsub(","," ",$2); print}'`
  OLD_IFS="$IFS"
  IFS=","
  array_data_length=($example_data)
  IFS="$OLD_IFS"
  for ((j = 0; j < $num_col; j++)); do

    #if current element is double, store both
    if [ "${data_result[$j]}" = "$double" ]; then
    #if [ -n "$(echo ${array_data_length[$j]} | sed 's/[^.]//g')" ]; then
        integer="$(echo ${array_data_length[$j]} | cut -d. -f1)"
        decimal="$(echo ${array_data_length[$j]} | cut -d. -f2)"
        length=${#integer}
        ((length=32*(length/32) + 32))
         if [[ "$length" -gt "${data_length[$j]}" ]]; then
          data_length[$j]=$length
         fi
   
        decimal_length=${#decimal}
        ((decimal_length=decimal_length + 2))
         if [[ "$decimal_length" -gt "${data_decimal[$j]}" ]];then
           data_decimal[$j]=$decimal_length
         fi
   
     else # if it is not double  
       length=${#array_data_length[$j]}
       ((length=32*(length/32) + 32))
       if [[ "$length" -gt "${data_length[$j]}" ]]; then
         data_length[$j]=$length
       fi
      fi
  done
m=`expr $m + 1`
if (($m > 18)); then # get first 18 lines from 1000 sample records
    break
fi
done < $fname


#merge three array together
declare -a final_result
for ((k = 0; k < ${#array1[@]}; k++)); do
  if [[ k -eq 0 ]]; then
    final_result[$k]="key: ${first_result[$k]} ${data_result[$k]}(${data_length[$k]}),"
  elif [[ k -eq 2 ]]; then
#   final_result[$k]="value: ${first_result[$k]} ${data_result[$k]}(${data_length[$k]}),"
    if (("${data_decimal[$k]}" > 0));then
#    if [${data_result[$k]}==$double];then
       final_result[$k]="value: ${first_result[$k]} ${data_result[$k]}(${data_length[$k]},${data_decimal[$k]}),"
    else
       final_result[$k]="value: ${first_result[$k]} ${data_result[$k]}(${data_length[$k]}),"
    fi

  else 
    if (("${data_decimal[$k]}" > 0));then
#    if [${data_result[$k]}==$double];then
    final_result[$k]="${first_result[$k]} ${data_result[$k]}(${data_length[$k]},${data_decimal[$k]}),"
    else
    final_result[$k]="${first_result[$k]} ${data_result[$k]}(${data_length[$k]}),"
    fi
  fi
done


#print out the output
seperate1="("
seperate2=");"

#echo -ne  #if don't want to start new line
echo -ne  "create table $table_name "
echo -ne  "$seperate1"
for ((ENTRY = 0; ENTRY < ${#array1[@]}; ENTRY++)) do
    if [[ ENTRY -eq 0 ]]; then
      echo -ne   " ${final_result[$ENTRY]}"
    elif [[ ENTRY -eq 2 ]]; then
      echo -ne " ${final_result[$ENTRY]}"
    else
      echo -ne " ${final_result[$ENTRY]}"
    fi
done
echo -e "$seperate2"
#/bin/rm -f $unfname
/bin/rm -f $tmpfname
