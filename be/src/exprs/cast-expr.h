// Copyright (c) 2012 Cloudera, Inc. All rights reserved.

#ifndef IMPALA_EXPRS_CAST_EXPR_H_
#define IMPALA_EXPRS_CAST_EXPR_H_

#include <string>
#include "expr.h"

namespace impala {

class TExprNode;

class CastExpr: public Expr {
 public:
  virtual llvm::Function* Codegen(LlvmCodeGen* codegen);

  virtual Status Prepare(RuntimeState* state, const RowDescriptor& desc);
  virtual std::string DebugString() const;

  virtual bool IsJittable(LlvmCodeGen* codegen) const;

 protected:
  friend class Expr;
  CastExpr(const TExprNode& node);
};

}

#endif
