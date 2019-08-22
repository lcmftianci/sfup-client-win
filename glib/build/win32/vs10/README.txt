Note that all this is rather experimental.

This VS10 solution and the projects it includes are intented to be used
in a GLib source tree unpacked from a tarball. In a git checkout you
first need to use some Unix-like environment or manual work to expand
the .in files needed, mainly config.h.win32.in into config.h.win32 and
glibconfig.h.win32.in into glibconfig.h.win32. You will also need to
expand the .vcprojin files here into .vcproj files.

The required dependencies are zlib and proxy-libintl. Fetch the latest
proxy-libintl-dev and zlib-dev zipfiles from
http://ftp.gnome.org/pub/GNOME/binaries/win32/dependencies/ for 32-bit
builds, and correspondingly
http://ftp.gnome.org/pub/GNOME/binaries/win64/dependencies/ for 64-bit
builds.

One may wish to build his/her own ZLib-It is recommended that ZLib is
built using the win32/Makefile.msc makefile with VS10 with the ASM routines
to avoid linking problems-see win32/Makefile.msc in ZLib for more details.

One may optionally use his/her own PCRE installation by selecting the
(BuildType)_ExtPCRE configuration, but please note the PCRE must be built
with VS10 with unicode support using the /MD (release) or /MDd (debug)
runtime option which corresponds to your GLib build flavour (release, debug).
(These are the defaults set by CMAKE, which is used in recent versions of PCRE.)
Not doing so will most probably result in unexpected crashes in 
your programs due to the use of different CRTs.  If using a static PCRE
build, add PCRE_STATIC to the "preprocessor definitions".
Note that one may still continue to build with the bundled PCRE by selecting
the (BuildType) configuration.

Set up the source tree as follows under some arbitrary top
folder <root>:

<root>\<this-glib-source-tree>
<root>\vs10\<PlatformName>

*this* file you are now reading is thus located at
<root>\<this-glib-source-tree>\build\win32\vs10\README.

<PlatformName> is either Win32 or x64, as in VS10 project files.

You should unpack the proxy-libintl-dev zip file into
<root>\vs10\<PlatformName>, so that for instance libintl.h end up at
<root>\vs10\<PlatformName>\include\libintl.h.

The "install" project will copy build results and headers into their
appropriate location under <root>\vs10\<PlatformName>. For instance,
built DLLs go into <root>\vs10\<PlatformName>\bin, built LIBs into
<root>\vs10\<PlatformName>\lib and GLib headers into
<root>\vs10\<PlatformName>\include\glib-2.0. This is then from where
project files higher in the stack are supposed to look for them, not
from a specific GLib source tree.

--Tor Lillqvist <tml@iki.fi>
--Updated by Chun-wei Fan <fanc999@gmail.com>
