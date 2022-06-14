#include <torch/csrc/lazy/ts_backend/dynamic_ir.h>

namespace torch {
namespace lazy {

DimensionNode::DimensionNode(OpKind op, OpList operands, hash_t hash_seed)
    : TsNode(
          op,
          operands,
          /*num_outputs=*/1,
          /* hash_seed */ HashCombine(op.hash(), hash_seed)) {}

const DimensionNode* DimensionNode::getOpDimNode(size_t index) const {
  return dynamic_cast<const DimensionNode*>(operand(index).node);
}

std::string DimensionNode::ToString() const {
  return "DimensionNode";
}

TSOpVector SizeNode::Lower(
    std::shared_ptr<torch::jit::GraphFunction> function,
    TSLoweringContext* loctx) const {
  std::vector<torch::jit::NamedValue> arguments;
  std::vector<torch::jit::NamedValue> kwarguments;
  arguments.reserve(2);
  auto index = loctx->graph()->insertConstant(static_cast<int64_t>(this->dim_));
  arguments.emplace_back(loctx->GetOutputOp(operand(0)));
  arguments.emplace_back(index);
  torch::lazy::TSOpVector size_out =
      torch::lazy::LowerTSBuiltin(function, op().op, arguments, kwarguments);
  CHECK_EQ(size_out.size(), 1);
  return size_out;
}

SizeNode::SizeNode(Value input, size_t dim)
    : DimensionNode(
          OpKind{c10::Symbol::fromQualString("aten::size")},
          {input},
          MHash(dim)),
      dim_(dim){};

int64_t SizeNode::getStaticValue() const {
  return dynamic_cast<const TsNode*>(operand(0).node)->shape(0).size(dim_);
}
bool SizeNode::isDynamic() const {
  auto symbolic_vec =
      dynamic_cast<const TsNode*>(operand(0).node)->shape(0).is_symbolic();
  if (!symbolic_vec.has_value()) {
    return true;
  }
  return symbolic_vec->at(dim_);
}

std::string SizeNode::ToString() const {
  return "SizeNode";
}

SizeAdd::SizeAdd(Value a, Value b)
    : DimensionNode(OpKind{c10::Symbol::fromQualString("aten::add")}, {a, b}){};

int64_t SizeAdd::getStaticValue() const {
  return getOpDimNode(0)->getStaticValue() + getOpDimNode(1)->getStaticValue();
}

bool SizeAdd::isDynamic() const {
  return getOpDimNode(0)->isDynamic() || getOpDimNode(1)->isDynamic();
}

std::string SizeAdd::ToString() const {
  return "SizeAdd";
}

SizeMul::SizeMul(Value a, Value b)
    : DimensionNode(OpKind{c10::Symbol::fromQualString("aten::mul")}, {a, b}){};

int64_t SizeMul::getStaticValue() const {
  return getOpDimNode(0)->getStaticValue() * getOpDimNode(1)->getStaticValue();
}

bool SizeMul::isDynamic() const {
  return getOpDimNode(0)->isDynamic() || getOpDimNode(1)->isDynamic();
}

std::string SizeMul::ToString() const {
  return "SizeMul";
}

SizeDiv::SizeDiv(Value a, Value b)
    : DimensionNode(OpKind{c10::Symbol::fromQualString("aten::div")}, {a, b}){};

int64_t SizeDiv::getStaticValue() const {
  TORCH_CHECK(
      getOpDimNode(1)->getStaticValue() != 0,
      "Can't divide a dimension by zero");
  return getOpDimNode(0)->getStaticValue() / getOpDimNode(1)->getStaticValue();
}

bool SizeDiv::isDynamic() const {
  return getOpDimNode(0)->isDynamic() || getOpDimNode(1)->isDynamic();
}

std::string SizeDiv::ToString() const {
  return "SizeDiv";
}

} // namespace lazy
} // namespace torch
