/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <memory>

template <typename Base>
inline std::shared_ptr<Base> shared_from_base(
    std::enable_shared_from_this<Base>* base) {
  return base->shared_from_this();
}

template <typename Base>
inline std::shared_ptr<const Base> shared_from_base(
    std::enable_shared_from_this<Base> const* base) {
  return base->shared_from_this();
}

template <typename That>
inline std::shared_ptr<That> shared_from(That* that) {
  return std::static_pointer_cast<That>(shared_from_base(that));
}
