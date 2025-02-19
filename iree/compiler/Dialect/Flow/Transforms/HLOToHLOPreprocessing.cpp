// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "llvm/Support/Casting.h"
#include "mlir/Dialect/StandardOps/IR/Ops.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/StandardTypes.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Support/LogicalResult.h"
#include "tensorflow/compiler/mlir/xla/ir/hlo_ops.h"
#include "tensorflow/compiler/mlir/xla/transforms/rewriters.h"

namespace mlir {
namespace iree_compiler {
namespace IREE {
namespace Flow {

namespace {

static bool isAllZero(DenseIntElementsAttr attr) {
  if (!attr.isSplat()) return false;
  return attr.getSplatValue<IntegerAttr>().getInt() == 0;
}

class ExtractReduceWindowOpPaddingAttributes
    : public OpRewritePattern<xla_hlo::ReduceWindowOp> {
 public:
  using OpRewritePattern<xla_hlo::ReduceWindowOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(xla_hlo::ReduceWindowOp op,
                                PatternRewriter &rewriter) const override {
    if (!op.padding()) return failure();
    if (op.base_dilations() || op.window_dilations()) return failure();
    if (isAllZero(op.paddingAttr())) return failure();

    auto inputType = op.operand().getType().cast<ShapedType>();
    int rank = inputType.getRank();
    SmallVector<int64_t, 4> paddingLow, paddingHigh, interiorPadding, shape;
    for (unsigned i = 0; i < rank; ++i) {
      // xla_hlo.pad doesn't support dynamic shape.
      if (inputType.isDynamicDim(i)) return failure();
      interiorPadding.push_back(0);
      paddingLow.push_back(op.paddingAttr().getValue<int64_t>({i, 0}));
      paddingHigh.push_back(op.paddingAttr().getValue<int64_t>({i, 1}));
      int size = inputType.getShape()[i];
      shape.push_back(size + paddingLow.back() + paddingHigh.back());
    }

    auto toDenseAttr = [&rewriter](ArrayRef<int64_t> elements) {
      return DenseIntElementsAttr::get(
          RankedTensorType::get(elements.size(), rewriter.getIntegerType(64)),
          elements);
    };

    auto loc = op.getLoc();
    auto padResultType =
        RankedTensorType::get(shape, inputType.getElementType());
    auto padOp = rewriter.create<xla_hlo::PadOp>(
        loc, padResultType, op.operand(), op.init_value(),
        toDenseAttr(paddingLow), toDenseAttr(paddingHigh),
        toDenseAttr(interiorPadding));
    auto newOp = rewriter.create<xla_hlo::ReduceWindowOp>(
        loc, op.getResult().getType(), padOp, op.init_value(),
        op.window_dimensions(), op.window_stridesAttr(),
        op.base_dilationsAttr(), op.window_dilationsAttr(),
        /*padding=*/nullptr);
    rewriter.inlineRegionBefore(op.body(), newOp.body(), newOp.body().begin());
    rewriter.replaceOp(op, newOp.getResult());
    return success();
  }
};

// Adjust the shape of depthwise_conv filter where is applied by xla_hlo.
class AdjustDepthwiseFilterShape : public OpRewritePattern<xla_hlo::ConvOp> {
 public:
  using OpRewritePattern<xla_hlo::ConvOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(xla_hlo::ConvOp op,
                                PatternRewriter &rewriter) const override {
    const auto featureInDim =
        op.dimension_numbers().kernel_input_feature_dimension().getInt();
    const auto featureOutDim =
        op.dimension_numbers().kernel_output_feature_dimension().getInt();
    const auto &kernelShape = op.rhs().getType().cast<ShapedType>().getShape();
    if (kernelShape[featureInDim] != 1) return failure();

    const auto groupCount = op.feature_group_count().getZExtValue();
    if (groupCount == 1) return failure();
    if (kernelShape[featureOutDim] % groupCount != 0) return failure();

    SmallVector<int64_t, 4> newShape(kernelShape.begin(), kernelShape.end());
    newShape[featureInDim] = groupCount;
    newShape[featureOutDim] /= groupCount;
    auto loc = op.getLoc();
    auto elemType = op.rhs().getType().cast<ShapedType>().getElementType();
    auto reshapeOp = rewriter.create<xla_hlo::ReshapeOp>(
        loc, RankedTensorType::get(newShape, elemType), op.rhs());
    auto resultType = op.getResult().getType();
    SmallVector<Value, 2> operands = {op.lhs(), reshapeOp.getResult()};
    auto newOp = rewriter.create<xla_hlo::ConvOp>(op.getLoc(), resultType,
                                                  operands, op.getAttrs());
    rewriter.replaceOp(op, newOp.getResult());
    return success();
  }
};

struct HLOToHLOPreprocessing
    : public PassWrapper<HLOToHLOPreprocessing, FunctionPass> {
  void runOnFunction() override {
    MLIRContext *context = &getContext();
    OwningRewritePatternList patterns;
    xla_hlo::PopulateUnfuseBatchNormPatterns(context, &patterns);
    // Note that various input modalities may do their own legalization of
    // CHLO. Converting here allows IREE to accept CHLO dialect regardless of
    // whether it was legalized away at a higher level.
    xla_chlo::PopulateLegalizeChloToHloPatterns(context, &patterns);
    patterns.insert<ExtractReduceWindowOpPaddingAttributes,
                    AdjustDepthwiseFilterShape>(context);
    applyPatternsAndFoldGreedily(getOperation(), patterns);
  }
};

}  // namespace

std::unique_ptr<OperationPass<FuncOp>> createHLOPreprocessingPass() {
  return std::make_unique<HLOToHLOPreprocessing>();
}

static PassRegistration<HLOToHLOPreprocessing> legalize_pass(
    "iree-flow-hlo-to-hlo-preprocessing",
    "Apply hlo to hlo transformations for some hlo ops");

}  // namespace Flow
}  // namespace IREE
}  // namespace iree_compiler
}  // namespace mlir
