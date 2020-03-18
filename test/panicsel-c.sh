# ABAT test for panicsel component utilities
# tag: rh71__build
# util=/usr/share/panicsel/showsel
util=/usr/sbin/isel
echo "panicsel component, basic acceptance test"
if [ ! -f $util ]
then
   echo "%%FAILURE%%"
   exit 1
fi
$util >/dev/null 2>&1
if [ $? -eq 0 ]
then
   echo "%%SUCCESS%%"
   exit 0
else
   echo "%%FAILURE%%"
   exit 1
fi
