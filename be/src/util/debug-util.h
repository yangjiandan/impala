// Copyright (c) 2012 Cloudera, Inc. All rights reserved.

#ifndef IMPALA_UTIL_DEBUG_UTIL_H
#define IMPALA_UTIL_DEBUG_UTIL_H

#include <ostream>
#include <string>
#include <boost/cstdint.hpp>

#include "gen-cpp/Exprs_types.h"
#include "gen-cpp/Opcodes_types.h"
#include "gen-cpp/PlanNodes_types.h"
#include "gen-cpp/RuntimeProfile_types.h"

namespace impala {

class RowDescriptor;
class TupleDescriptor;
class Tuple;
class TupleRow;
class RowBatch;

std::ostream& operator<<(std::ostream& os, const TExprOpcode::type& op);
std::ostream& operator<<(std::ostream& os, const TAggregationOp::type& op);
std::ostream& operator<<(std::ostream& os, const TUniqueId& id);

std::string PrintTuple(const Tuple* t, const TupleDescriptor& d);
std::string PrintRow(TupleRow* row, const RowDescriptor& d);
std::string PrintBatch(RowBatch* batch);
std::string PrintId(const TUniqueId& id);
std::string PrintPlanNodeType(const TPlanNodeType::type& type);

std::string GetVersionString();

// Returns the stack trace as a string from the current location.
// Note: there is a libc bug that causes this not to work on 64 bit machines
// for recursive calls.
std::string GetStackTrace();

class PrettyPrinter {
 public:
  // Prints the 'value' in a human friendly format depending on the data type.
  // i.e. for bytes: 3145728 -> 3MB
  static std::string Print(int64_t value, TCounterType::type type);
};

}

#endif
