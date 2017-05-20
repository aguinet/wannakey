Wannakey
========

WARNING
=======

This software has only been tested and known to work under Windows XP, 7 x86,
2003, Vista and Windows Server 2008 (tests by @msuiche).

Please also note that you need some luck for this to work (see below), and so
it might not work in every case!

DISCLAMER
=========

- This code is and will always be GPLv3. See LICENSE file attached.

Updates
=======

v2.0
----

* automatically find the encryption process PID
* write the private key so that the original malware's process can decrypt the files (using the Decrypt button)
* code refactoring and cmake usage

v0.2
----

* The generated private RSA key had invalid computed fields, which made the key
  not importable with CryptImportKey under Windows XP (fixed). wanafork and/or wanadecrypt can
  now be used directly from XP.

* Updated the binary with this fix and a static build (no need for the MSVC
  runtime anymore)

v0.1
----

* Original version

Introduction
============

This software allows to recover the prime numbers of the RSA private key that are used by Wanacry.

It does so by searching for them in the ``wcry.exe`` process. This is the
process that generates the RSA private key. The main issue is that the
``CryptDestroyKey`` and ``CryptReleaseContext`` does not erase the prime
numbers from memory before freeing the associated memory.

This is not really a mistake from the ransomware authors, as they properly use
the Windows Crypto API. Indeed, for what I've tested, under Windows 10,
``CryptReleaseContext`` does cleanup the memory (and so this recovery technique
won't work). It can work under Windows XP because, in this version,
``CryptReleaseContext`` does not do the cleanup. Moreover, MSDN states this,
for this function : "After this function is called, the released CSP handle is
no longer valid. This function does not destroy key containers or key pairs.".
So, it seems that there are no clean and cross-platform ways under Windows to
clean this memory.

If you are lucky (that is the associated memory hasn't been reallocated and
erased), these prime numbers might still be in memory.

That's what this software tries to achieve.

Usage
=====

You can use the binary in the bin/ folder. It will locate the encryption PID by
itself. If it can't, you might need to search it by hand and pass it as an
argument of the ``wannakey.exe`` tool.o

If the key had been succeesfully generated, you will just need to use the
"Decrypt" button of the malware to decrypt your files!

Compile from source
===================

You need at least Visual Studio 2015 express and CMake.

Launch a VS2015 Native Tools command line prompt and run:

.. code::

  > cd /path/to/wannakey/wannakey/
  > mkdir build
  > cd build
  > cmake -G "Visual Studio 14 2015" -T "v140_xp" ..
  > cmake --build . --config "release"

Binary will be in the src/Release directory.

Credits
=======

* @wiskitki who spotted the ``CryptReleaseContext`` issue with Windows 10 (which actually wipe the primes in memory).
* @hackerfantastic for releasing the sample I used
* Miasm (https://github.com/cea-sec/miasm) for its help extracting the DLL and reversing the whole thing
* Wine sources for the Windows RSA private key format.
