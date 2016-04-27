
    Virtual disk driver over HTTP for Windows.
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

    8. 2015-04-29
       Support for more ioctl:s.
       Can be compiled with both DDK and WDK.
       Improved error handling.
       Improved debug prints.

    7.02 2015-04-25
       Small bug fix for mounting non CD/DVD images.

    7.01 2015-04-17
       Small update to pass all code analyzes in WDK 7.1.0.

    7. 2009-10-21
       Small bug fix for CD-images on Windows Vista and Windows 7.
       Support for 64-bit systems.

    6. 2009-10-15
       Allow to mount images without administrative privileges by
       Konstantin Vlasov.

    5. 2009-04-12
       Support for DVD images with the UDF file system.
       Can be compiled with the WDK.

    4. 2008-07-11
       Minor changes.

    3. 2006-03-08
       Small bug fix in handling of receive data length.

    2. 2006-03-05
       Added support for persistent connections.

    1. 2006-02-11
       Initial release.
