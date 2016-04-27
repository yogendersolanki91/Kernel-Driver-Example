
    This is a virtual disk driver for Windows that uses one or more files to
    emulate physical disks.
    Copyright (C) 1999-2015 Bo Brantén.
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    The GNU General Public License is also available from:
    http://www.gnu.org/copyleft/gpl.html

    Windows and Windows NT are either registered trademarks or trademarks of
    Microsoft Corporation in the United States and/or other countries.

    Please send comments, corrections and contributions to bosse@acc.umu.se.

    The most recent version of this program is available from:
    http://www.acc.umu.se/~bosse/

    Revision history:

   19. 2015-04-23
       Support for more ioctl:s.
       Can be compiled with both DDK and WDK.

   18. 2015-04-17
       Corrected an error when creating a sparse image file. This error only
       showed up on Windows Vista and later versions of Windows while sparse
       files where created successfully on Windows XP.

   17.01 2015-04-15
       Small update to pass all code analyzes in WDK 7.1.0.

   17. 2009-10-21
       Small bug fix for CD-images on Windows Vista and Windows 7.

   16. 2009-10-15
       Allow to mount images without administrative privileges by
       Konstantin Vlasov.

   15. 2009-04-12
       Support for DVD images with the UDF file system.
       Set image file to sparse as default.
       Small bug fix.
       Can be compiled with the WDK.

   14. 2006-01-05
       Updated impersonation so that use of image files stored on network
       drives work on Windows 2000 SP4, Windows XP SP2 and
       Windows Server 2003 SP1.

   13. 2004-06-09
       Small fix in file size handling.

   12. 2004-05-27
       Possible fix for memory leak.

   11. 2002-11-30
       Added ioctl to query information about mounted disk image files by
       request from developer of GUI.

   10. 2002-11-24
       Added a check so that FileDisk doesn't use compressed or encrypted
       images. For an explanation why this doesn't work see comment in the
       source code.

    9. 2002-08-26
       Corrected the share access for read-only FileDisk images.

    8. 2002-08-11
       Updated the control application to support UNC paths.
       Changed the handling of CD-ROM device objects to avoid some problems on
       Windows XP.
       Corrected the handling of file sizes so that FileDisk images can be
       sparse files.

    7. 2002-02-28
       Added support for CD-images.

    6. 2002-01-21
       Added support for impersonation so that FileDisk images can be stored
       on network drives.

    5. 2002-01-18
       Updated for Windows XP by Robert A. Rose.

    4. 2001-07-08
       Formating to FAT on Windows 2000 now works.

    3. 2001-05-14
       Corrected the error messages from the usermode control application.

    2. 2000-03-15
       Added handling of IOCTL_DISK_CHECK_VERIFY to make the driver work on
       Windows 2000 (tested on beta 3, build 2031). Formating to FAT still
       doesn't work but formating to NTFS does.

    1. 1999-06-09
       Initial release.
