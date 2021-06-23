/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

namespace Protocol {

struct Encoder {
  /* data */
};

class Encodable {
 public:
  virtual void Encode(/* to */ Encoder) = 0;
};
}  // namespace Protocol
