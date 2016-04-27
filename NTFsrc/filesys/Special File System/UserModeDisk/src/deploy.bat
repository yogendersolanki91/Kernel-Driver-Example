copy ..\bin\debug\i386\mntdrv.sys %windir%\system32\drivers\
sc create CoreMnt type= kernel binPath= system32\drivers\CoreMnt.sys
sc start CoreMnt

pause
