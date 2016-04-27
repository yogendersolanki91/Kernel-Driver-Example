/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   dispatch object header

   This file is part of FIFS (Framework for Implementing File Systems). 

   This software is distributed with NO WARRANTY OF ANY KIND.  No
   author or distributor accepts any responsibility for the
   consequences of using it, or for whether it serves any particular
   purpose or works at all, unless he or she says so in writing.
   Refer to the included modified Alladin Free Public License (the
   "License") for full details.

   Every copy of this software must include a copy of the License, in
   a plain ASCII text file named COPYING.  The License grants you the
   right to copy, modify and redistribute this software, but only
   under certain conditions described in the License.  Among other
   things, the License requires that the copyright notice and this
   notice be preserved on all copies.

*/

#ifndef __DISPATCH_HPP__
#define __DISPATCH_HPP__

typedef int (*DISPATCH_FUNC)(const char*, const char*, int, char *[]);

#define DECLARE_DISPATCH_FUNC(name) \
int name(const char* program, const char* command, int argc, char *argv[])

class CCommandLineDispatcher_Rep;

class CCommandLineDispatcher
{
private:
    CCommandLineDispatcher_Rep *rep;
public:
    CCommandLineDispatcher();
    ~CCommandLineDispatcher();
    void Add(
        const char *name,
        DISPATCH_FUNC exec
        );
    int Dispatch(
        int argc,
        char* argv[]
        );
    int DoCommand(
        const char *program, 
        char *command, 
        int argc, 
        char *argv[]
        );
};

#endif // __DISPATCH_HPP
