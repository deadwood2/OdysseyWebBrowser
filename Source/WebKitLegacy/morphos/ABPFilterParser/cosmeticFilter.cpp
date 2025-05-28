/* Copyright (c) 2015 Brian R. Bondy. Distributed under the MPL2 license.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "./cosmeticFilter.h"
#include "hashFn.h"

namespace ABP {

static HashFn fn(19);

uint64_t CosmeticFilter::GetHash() const {
  return fn(data, static_cast<int>(strlen(data)));
}

}

