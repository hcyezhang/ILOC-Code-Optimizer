echo "0: Loop Unrolling followed by SVN [suggested],"
echo "1: SVN followed by Loop Unrolling,"
echo "2: Loop Unrolling,"
echo "3: SVN "
read case_id

echo "Please type your source file with only filename prefix included: (i.e. if you have a file with name 'algred.i', you should type 'algred') "
read source_filename

case "$case_id" in
"0")
    ./optimizer-unroll $source_filename'.i' 3 $source_filename'_unroll.i' 0
    ./optimizer-svn $source_filename'_unroll.i' $source_filename'_unroll_svn.i'
    ;;

"1")
   ./optimizer-svn $source_filename'.i' $source_filename'_svn.i'
   ./optimizer-unroll $source_filename'_svn.i' 3 $source_filename'_svn_unroll.i' 0
   ;;

"2")
  ./optimizer-unroll $source_filename'.i' 3 $source_filename'_unroll.i' 0
  ;;

"3")
  ./optimizer-svn $source_filename'.i' $source_filename'_svn.i' 
  ;;

esac
