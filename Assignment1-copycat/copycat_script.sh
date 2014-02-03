#! /usr/bin/bash
#copycat_script.sh: generates data for copycat throughput analysis

out="$HOME/Documents/OS/Assignment1-copycat/analysis.txt"

((MAX_BUF=256*1024))
infile="$HOME/Documents/OS/Assignment1-copycat/file3.txt"
file_size=$(stat -c '%s' "$infile")
outfile_prefix="$HOME/Documents/OS/Assignment1-copycat/outfile_b="
buf_size=1

{
printf "******Copycat Throughput Analysis******\n\n"
printf "Infile Size: $file_size \n"
while (($buf_size<=$MAX_BUF)); do
	outfile="$outfile_prefix""$buf_size".txt
	printf "\nbuf_size=$buf_size"
	time copycat -b "$buf_size"  -o "$outfile" "$infile"
	((buf_size*=2))
done
} &>"$out"