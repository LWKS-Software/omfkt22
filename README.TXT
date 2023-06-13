Open Media Framework(TM) Interchange Toolkit Version 2.1.2 README.TXT
-------------------------------------------------------------------
(PC NOTE: open *.TXT files in Visual C++)     

This is the README.TXT file for the OMF Toolkit.  You will find a copy of
this file in .../README.TXT or .../Toolkit/README.TXT after successful 
downloading and unpacking of the distribution media.

***** BEFORE PROCEEDING... **********************************************
1) Please read the LICENSE.TXT file that accompanies this README or is
   displayed on the OMF Web site, and only continue if you are willing
   to accept all of the terms contained in the stated license
   agreement.
2) If you have not completed the online registration on the OMF web
   site, please complete the registration form in the accompanying
   file REGISTER.TXT and either email or fax to the OMF Developers'
   Desk using the contact information below in section 2).  When
   registered, you may request a vendor ID from the OMF Developers'
   Desk, which should be used for Mob Identification within the
   Toolkit.  See the Release Notes (Section II) for more information.
*************************************************************************

CONTENTS
========

0) QuickStart
1) What is OMF and the OMF Toolkit?
2) Contact Information
3) Toolkit Road Map
4) Downloading and Unpacking on Various Platforms


0) QuickStart
==============

* Unpack the archive on your platform (details in section 4 below)
* See RELEASE.TXT for changes, known problems and solutions, doc changes.
* See BUILDUNIX.TXT, BUILDMAC.TXT or BUILDPC.TXT for build
  instructions on your platform.  This file also has a QuickStart
  section.  BUILD*.TXT tells you how to build and test the OMF
  Toolkit.
* See the OMF Toolkit User Guide for documentation on how to build
  applications that can read and write OMF files.  The documentation
  is available at ftp://ftp.avid.com/pub/omf/


1) What is OMF and the OMF Toolkit?
===================================

The OMF Toolkit is a software library that can be used by application
programs to interchange media information with other applications
using the Open Media Framework Interchange format.  The Toolkit is a
portable reference implementation of the OMFI 2.0 Specification.  It
is distributed in C source code.  The Toolkit consists of OMFI library
source code, small but useful applications, and buildable and runable
example code (unittests).

The Toolkit is typically built as four different libraries: libBento,
libJPEG, libAJPG and libOMFI.  Applications link with these libraries to
include OMF support in their products.  On the Macintosh libOMFI comes
in two versions, as described in BUILDMAC.TXT.

Bento provides OMFI with a cross-platform container mechanism.  Bento
has handlers for memory management, error handling and file I/O.

For JPEG compression, the Toolkit uses the Independent JPEG Group's
JPEG 4.0.  The JPEG source code is compiled in by default, by defining
the OMFI_JPEG_CODEC variable in the build environment.

AJPG contains the AVID AVR JPEG Coder/Decoder (codec).  This enables
your application to write image media compressed for Avid products at
any supported AVR (Avid Video Resolution).

The OMFI library contains all of the source code that layers
above Bento and JPEG to implement the structures and objects
described in the 2.0 OMFI Specification.  The OMFI library also
contains a set of 1.5 API wrappers, to allow an application built
with the 1.5 OMFI API to transparently link with the 2.0 Toolkit.

End users cannot use the Toolkit directly; instead they can use
applications that support OMF interchange with the Toolkit. Some
useful OMF utilities are available on the OMF web site. End users can
create media information in one application, export an OMF Interchange
file from that application, and then import that file into a second
application. End users do not need to understand the contents of an
OMF Interchange file or be aware that the Toolkit is being called by
the applications.


2) Contact Information:
=======================

You can find information about OMF on our web site:
    http://www.omfi.org/

Subscribe to the low-volume omf-interest email list by sending email
to majordomo@avid.com with the following in the body (NOT the
subject): 
    subscribe omf-interest your-email@yoursite (Your Name)

You can download OMF documentation, sample files and useful utilities
and applications from our ftp site: 
    ftp://ftp.avid.com/pub/omf/

Or use regular ftp to "ftp.avid.com" as user "anonymous",
use your email address as password, then "cd pub/omf". 

Technical discussion is held on the omf Working Group email lists.
There is an automatic signup form on the OMF web site in the "For
Developers" section, or contact omf-request@avid.com.

For help, contact the OMF Developers' Desk at:
    omf-request@avid.com
    OMF Developers' Desk 
    Avid Technology, Inc. 
    1925 Andover Street
    Tewksbury, MA 01876
phone  (800) 949-OMFI 
    +1 (508) 640-3400
fax +1 (508) 640-9768


3) Toolkit Road Map
===================

Please consult the file RELEASE.TXT for information regarding this and
prior releases, known problems and workarounds, and addenda and
corrections to the documentation.

The next section of this file contains INSTALL information on how to
unpack the OMF distribution on each of the platforms.

The OMF Toolkit supports various platforms architectures.  Once you
have downloaded the media, please consult the BUILD*.TXT file in
.../Toolkit directory (this directory) for directions on building on a
specific platform:

    BUILDUNIX.TXT for building the Toolkit for UNIX platforms.
    BUILDPC.TXT   for building the Toolkit for DOS/Windows/NT platforms.
    BUILDMAC.TXT  for building the Toolkit for Macintosh platforms.


The Toolkit directory structure is:

Toolkit			OMF Toolkit root directory.
Toolkit/CW68KProjects	(Macintosh only) CodeWarrior 680x0 project files.
Toolkit/CWPPCProjects	(Macintosh only) CodeWarrior PowerPC project files.
Toolkit/CWPrefixes	(Macintosh only) CodeWarrior prefix files.
Toolkit/Makefile	(UNIX only) Top level Makefile (see BUILDUNIX.TXT)
Toolkit/README.TXT	This file.
Toolkit/RELEASE.TXT	Release notes: changes, bugfixes, known
			problems & solutions, documentation addenda
			and corrections.
Toolkit/apps		OMF 2.0 useful/example applications.
Toolkit/apps1x		OMF 1.x useful/example applications that use
			the back-compatibility API.
Toolkit/avidjpg		contains the AVID AVR JPEG codec
Toolkit/bento		Source and documentation for the Bento
			container manager library.
Toolkit/common		(UNIX only) Shared files for GNU Make.
Toolkit/compat		1.5 compatibility include files and sources.
Toolkit/include		contains include files common to apps and kitomfi dir 
Toolkit/jpeg		Source and documentation for the JPEG library.
Toolkit/kitomfi		Source for the OMF Toolkit library.
Toolkit/portinc		
Toolkit/unittest	1.0 (compat) and 2.0 unit test sources.

Note: In the PC version of the Toolkit, the Visual C++ makefiles can 
be found in the top level (Toolkit directory).  Also, the UNIX version
has a Makefile in the top level directory as well as in each of the 
sub-directories that contain test, example or application code.

There are also a few other README.TXT files that you may find useful:

Toolkit/apps/README.TXT	    describes the 2.0 applications.
Toolkit/apps1x/README.TXT   describes the 1.x applications that build with the
 			    back-compatibility API.
Toolkit/unittest/README.TXT describes the 1.x compat and 2.0 unit tests.


4) Downloading and Unpacking on Various Platforms
=================================================

This section describes how to unpack the distribution on Macintosh,
PC, and UNIX.

The 2.1.2 Toolkit requires about 10 MB of disk space to unpack, about
100 MB fully built (including sample omf files and applications), and
about 120 MB when you have run all the unit tests, which generate more
OMF files.  This will vary somewhat depending on the installation
platform.

4.1) Macintosh
--------------

If the Toolkit is on the network AND you are on a Macintosh:
------------------------------------------------------------

Use Netscape or other web browser to contact the OMF web site:
    http://www.omfi.org/

Click the "Download the 2.1.2 Toolkit" link, register, then select the
Macintosh archive, either .bin or .hqx.  .bin is MacBinary, which is a
smaller format.  If you have problems downloading MacBinary, use .hqx
(BinHex) which is an ascii format.

If your browser didn't automatically launch StuffIt Expander, run it
on the resulting .bin or .hqx file (you can drag-and-drop the .bin or
.hqx icon onto StuffIt Expander).  If your ftp client automatically
unpacked the .bin or .hqx file, you may double-click on the
self-extracting archive icon.

This will create a folder containing the Toolkit distribution.  See
BUILDMAC.TXT for further details.

If you need StuffIt Expander, see an Info-Mac mirror site, in the cmp
folder, e.g.
    ftp://mirror.apple.com/mirrors/Info-Mac.Archive/

If the Toolkit is on disk(s) AND you are on a Macintosh:
--------------------------------------------------------

Place the disk (3.5") in the drive.  Double-click on the
self-extracting archive.  This will create a folder containing the
Toolkit distribution.  See BUILDMAC.TXT for further details.


4.2) PC/Windows/DOS/NT
----------------------

If the Toolkit is on the network AND you are on a PC:
-----------------------------------------------------
Use Netscape or other web browser to contact the OMF web site:
    http://www.omfi.org/

Click the "Download the 2.1.2 Toolkit" link, register, then select the
PC archive.

Use pkunzip -d omfkt202.zip to unzip the file, preserving the directory
structure. 

This will create a folder containing the Toolkit distribution.  See
BUILDPC.TXT for further details.

If the Toolkit is on disk(s) AND you are on a PC:
-------------------------------------------------

The Toolkit is contatined in a zip file. To extract the files
change to the directory where you want to place the toolkit and execute the
pkunzip on the archive downloaded from the ftp site.  You should use
the '-d' flag or similar option to preserve the directory structure of
the archive.


4.3) UNIX
---------

If the Toolkit is on the network AND you are on a UNIX machine:
---------------------------------------------------------------

Use Netscape or other web browser to contact the OMF web site:
    http://www.omfi.org/

Click the "Download the 2.1.2 Toolkit" link, register, then select the
UNIX archive in either .gz (GnuZip) or .Z (compress) format. GnuZip is
smaller.  If you don't have GnuZip you may download the .Z archive and
use standard unix uncompress.

Un-gzip the file:
    gunzip omfkt2.1.2.tar.gz

If you need gzip, contact:
    ftp://prep.ai.mit.edu/
    
Uncompress the file:
    uncompress omfkt2.1.2.tar.Z

Now un-tar the tar file:
    tar xvf omfkt2.1.2.tar

This will create a folder containing the Toolkit distribution.  See
BUILDUNIX.TXT for further details.

If the Toolkit is on tape AND you are on Unix:
----------------------------------------------

To read the tape, cd to the (parent) directory where you want to install
the release, put the tape in a local tape drive and type:

tar xv

Make sure that the default tape drive is correctly set.  If not, you will have
to specify it.  Refer to your system documentation in order to specify
the correct tape device name. 

On an SGI, you can specify a local device:

tar xvf /dev/tape

or, on an SGI, you can specify a remote device:

tar xvf hostname:/dev/tape

This will create a folder containing the Toolkit distribution.  See
BUILDUNIX.TXT for further details.

-----

Please contact the OMF Developers' Desk if you have any problems,
suggestions or questions.  Welcome to OMF!

