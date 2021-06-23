/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

namespace Protocol {

struct Decoder {
  /* data */
};

class Decodable {
  virtual void init(/* from */ Decoder) = 0;
};
}  // namespace Protocol
