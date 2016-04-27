/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   file system tester main

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

//extern "C" {
DECLARE_DISPATCH_FUNC(create);
DECLARE_DISPATCH_FUNC(df);
DECLARE_DISPATCH_FUNC(df2);
DECLARE_DISPATCH_FUNC(write);
DECLARE_DISPATCH_FUNC(contend);
DECLARE_DISPATCH_FUNC(contend2);
DECLARE_DISPATCH_FUNC(contend3);
//}

int main(int argc, char *argv[])
{
    CCommandLineDispatcher dispatcher;
    dispatcher.Add("create", create);
    dispatcher.Add("df", df);
    dispatcher.Add("df2", df2);
    dispatcher.Add("write", write);
    dispatcher.Add("contend", contend);
    dispatcher.Add("contend2", contend2);
    dispatcher.Add("contend3", contend3);
    return dispatcher.Dispatch(argc, argv);
}
