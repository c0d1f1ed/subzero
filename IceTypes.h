/* Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// -*- Mode: c++ -*-
#ifndef _IceTypes_h
#define _IceTypes_h

#include "IceDefs.h"

enum IceType {
  IceType_void,
  IceType_i1,
  IceType_i8,
  IceType_i16,
  IceType_i32,
  IceType_i64,
  IceType_f32,
  IceType_f64,
};

IceOstream& operator<<(IceOstream &Str, IceType T);

#endif // _IceTypes_h
