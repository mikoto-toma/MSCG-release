This directory contains links to the Multi-scale Coarse-graining
(MS-CG) library which is required to use the MSCG package and its fix
command in a LAMMPS input script.

The MS-CG library is available at
https://github.com/uchicago-voth/MSCG-release and was developed by
Jacob Wagner in Greg Voth's group at the University of Chicago.

This library requires a compiler with C++11 support (e.g., g++ v4.9+), LAPACK, 
 and the GNU scientific library (GSL v 2.1+).
-----------------

You must perform the following steps yourself.

1.  Download MS-CG at https://github.com/uchicago-voth/MSCG-release
    either as a tarball or via SVN, and unpack the tarball either in
    this /lib/mscg directory or somewhere else on your system.

2.  Ensure that you have LAPACK and GSL (or Intel MKL) as well as a compiler
    with support for C++11.
    
3.  Compile MS-CG from within its home directory using your makefile of choice:
    % make -f Makefile."name" libmscg.a
	It is recommended that you start with Makefile.g++_simple for most machines

4.  There is no need to install MS-CG if you only wish 
    to use it from LAMMPS.

5.  A soft link in this dir (lib/mscg) to the MS-CG src
    directory.  E.g if you built MS-CG in the src dir:
      % ln -s src includelink

6.  Modify your LAMMPS Makefile to include -std=c++11 on the CCFLAGS line

-----------------

When these steps are complete you can build LAMMPS with the MS-CG
package installed:

% cd lammps/src
% make yes-MSCG
% make g++ (or whatever target you wish)

Note that if you download and unpack a new LAMMPS tarball, the
"includelink" files will be lost and you will need to
re-create them (step 4).  If you built MS-CG in this directory (as
opposed to somewhere else on your system) and did not install it
somewhere else, you will also need to repeat steps 1,2,3.

The Makefile.lammps file in this directory is there for compatibility
with the way other libraries under the lib dir are linked with by
LAMMPS.  MS-CG requires the GSL and LAPACK as listed in Makefile.lammps.
If they are not in default locations where your
LD_LIBRARY_PATH environment settings can find them, then you should
add the approrpriate -L paths to the mscg_SYSPATH variable in
Makefile.lammps.
