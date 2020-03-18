#!/bin/sh 
#  retro.sh
#
#  retrofit ipmiutil subcommand naming for pre-2.6.0 scripts.
#
#  old            new        full command
#alarms         ialarms      (ipmiutil alarms)
#bmcconfig      iconfig      (ipmiutil config)
#bmchealth      ihealth      (ipmiutil health)
#fruconfig      ifru         (ipmiutil fru)
#getevent       igetevent    (ipmiutil getevent)
#hwreset        ireset       (ipmiutil reset)
#icmd           icmd         (ipmiutil cmd)
#idiscover      idiscover    (ipmiutil discover)
#ievents        ievents      (ipmiutil events)
#isolconsole    isol         (ipmiutil sol)
#pefconfig      ilan         (ipmiutil lan)
#sensor         isensor      (ipmiutil sensor)
#showsel        isel         (ipmiutil sel)
#tmconfig       iserial      (ipmiutil serial)
#wdt            iwdt         (ipmiutil wdt)

mydir=`pwd`

cd /usr/sbin
ln ialarms   alarms
ln iconfig   bmcconfig
ln ihealth   bmchealth
ln ifru      fruconfig
ln igetevent getevent
ln ireset    hwreset
ln isol      isolconsole
ln ilan      pefconfig
ln isensor   sensor
ln iserial   tmconfig
ln iwdt      wdt
cd $mydir
