# ABAT test for panicsel kernel patch
# tag:  panicsel__build
echo "panicsel kernel patch (BMC_PANIC), basic acceptance test"
# dmesg |grep "BMC IPMI"     >/dev/null 2>&1
dmesg |grep bmc_panic    >/dev/null 2>&1
if [ $? -eq 0 ]
then
   echo "%%SUCCESS%%"
   exit 0
else
   echo "%%FAILURE%%"
   exit 1
fi
