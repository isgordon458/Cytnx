#ifndef __cuTensordot_internal_H__
#define __cuTensordot_internal_H__

#include <assert.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include "Storage.hpp"
#include "Type.hpp"

namespace cytnx {

  namespace linalg_internal {

    /// cuTensordot
    void cuTensordot_internal_cd(Tensor &out,
                                 const Tensor &Lin,
                                 const Tensor &Rin, const std::vector<cytnx_uint64> &idxl, const std::vector<cytnx_uint64> &idxr);

    


  }  // namespace linalg_internal
}  // namespace cytnx

#endif
