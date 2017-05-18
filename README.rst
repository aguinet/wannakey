Wannakey
========

WARNING
=======

This software has only been tested and known to work under Windows XP. In order
to work, your computer must not have been rebooted after being infected.

Please also note that you need some luck for this to work (see below), and so
it might not work in every cases!

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

You can use the binary in the bin/ folder. You first need to find the PID of
the ``wcry.exe`` process using the Task Manager, and locate the
``00000000.pky`` file.

Once you've got this, launch using ``cmd.exe``:

.. code::

   > search_primes.exe PID path\to\00000000.pky

If a valid prime is found in memory, the ``priv.key`` file will be generated in
the current directory.

You can then use https://github.com/odzhan/wanafork/ to decrypt your files!

**WARNING**: wanafork does not work directly for now directly under Windows XP. This should be fixed soon (hopefully)!

Compile from source
===================

You can use Visual Studio 2015 express to compile the associated project. Be
sure to select the compatible Windows XP toolchain in the project properties!

Credits
=======

* @wiskitki who spotted the ``CryptReleaseContext`` issue with Windows 10 (which actually wipe the primes in memory).
* @hackerfantastic for releasing the sample I used
* Miasm (https://github.com/cea-sec/miasm) for its help extracting the DLL and reversing the whole thing
* Wine sources for the Windows RSA private key format.
