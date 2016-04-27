/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   NetBIOS tester main

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

#include <iostream>
#include "dispatch.hpp"

extern "C" {
DECLARE_DISPATCH_FUNC(Main_EnumLana);
DECLARE_DISPATCH_FUNC(Main_Listen);
DECLARE_DISPATCH_FUNC(Main_ListNames);
DECLARE_DISPATCH_FUNC(Main_Recv);
DECLARE_DISPATCH_FUNC(Main_PassThrough);
DECLARE_DISPATCH_FUNC(Main_Test);
DECLARE_DISPATCH_FUNC(Main_Call);
DECLARE_DISPATCH_FUNC(Main_Add);
}

int main(int argc, char *argv[])
{
    CCommandLineDispatcher dispatcher;
    dispatcher.Add("enum", Main_EnumLana);
    dispatcher.Add("listen", Main_Listen);
    dispatcher.Add("list", Main_ListNames);
    dispatcher.Add("recv", Main_Recv);
    dispatcher.Add("pass", Main_PassThrough);
    dispatcher.Add("test", Main_Test);
    dispatcher.Add("call", Main_Call);
    dispatcher.Add("add", Main_Add);
    return dispatcher.Dispatch(argc, argv);
}
