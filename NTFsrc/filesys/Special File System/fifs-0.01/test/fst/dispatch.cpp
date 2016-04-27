/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   dispatch object

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

#include "dispatch.hpp"
#include <string.h>
#include <iostream>
#include <vector>

struct DISPATCH_ENTRY {
    char *name;
    DISPATCH_FUNC exec;
};

bool operator < (const DISPATCH_ENTRY& x, const DISPATCH_ENTRY& y) {
        return strcmp(x.name, y.name) < 0;
}

bool operator == (const DISPATCH_ENTRY& x, const DISPATCH_ENTRY& y) {
    return !strcmp(x.name, y.name);
}

class CCommandLineDispatcher_Rep
{
private:
    std::vector<DISPATCH_ENTRY> disp_table;
    friend class CCommandLineDispatcher;
};

CCommandLineDispatcher::CCommandLineDispatcher()
{
    rep = new CCommandLineDispatcher_Rep;
}

CCommandLineDispatcher::~CCommandLineDispatcher()
{
    delete rep;
}

void
CCommandLineDispatcher::Add(
    const char *name,
    DISPATCH_FUNC exec
    )
{
    DISPATCH_ENTRY entry;
    entry.name = (char*) name;
    entry.exec = exec;
    rep->disp_table.push_back(entry);
}

int
CCommandLineDispatcher::Dispatch(
    int argc,
    char* argv[]
    )
{
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " command [args]\n"
                  << "    Where 'command' is one of:\n";
        for (int i = 0; i < rep->disp_table.size(); i++) {
            std::cout << "        " << rep->disp_table[i].name << "\n";
        }
        return 1;
    }
    return DoCommand(argv[0], argv[1], argc - 2, &argv[2]);
}

int
CCommandLineDispatcher::DoCommand(
    const char *program, 
    char *command, 
    int argc, 
    char *argv[]
    )
{
    for (int i = 0; i < rep->disp_table.size(); i++) {
        if (!stricmp(rep->disp_table[i].name, command)) {
            return rep->disp_table[i].exec(program, command, argc, argv);
        }
    }
    std::cout << "Command " << command << " not found\n";
    return -1;
}
