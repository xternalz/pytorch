#include <ATen/ATen.h>
#include <ATen/Config.h>
#include <ATen/NativeFunctions.h>
#include <ATen/native/utils/ParamUtils.h>
#include <tuple>


#if !AT_MKLDNN_ENABLED()

namespace at {
namespace native {

Tensor mkldnn_max_pool2d(
    const Tensor& self,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    IntArrayRef dilation,
    bool ceil_mode) {
  AT_ERROR(
      "mkldnn_max_pool2d: ATen not compiled with MKLDNN support");
}

Tensor mkldnn_avg_pool2d(
    const Tensor& self,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    bool ceil_mode,
    bool count_include_pad) {
  AT_ERROR("mkldnn_avg_pool2d: ATen not compiled with MKLDNN support");
}

Tensor& mkldnn_avg_pool2d_out(
    Tensor& output,
    const Tensor& self,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    bool ceil_mode,
    bool count_include_pad) {
  AT_ERROR("mkldnn_avg_pool2d_out: ATen not compiled with MKLDNN support");
}

Tensor mkldnn_adaptive_avg_pool2d(Tensor const& input, IntArrayRef output_size) {
  AT_ERROR("mkldnn_adaptive_avg_pool2d: ATen not compiled with MKLDNN support");
}

Tensor& mkldnn_adaptive_avg_pool2d_out(
    Tensor& output,
    const Tensor& input,
    IntArrayRef output_size) {
  AT_ERROR(
      "mkldnn_adaptive_avg_pool2d_out: ATen not compiled with MKLDNN support");
}

} // namespace native
} // namespace at

#else // AT_MKLDNN_ENABLED

#include <ATen/native/mkldnn/MKLDNNCommon.h>
#include <ATen/native/mkldnn/Utils.h>

namespace at {
namespace native {

static Tensor _mkldnn_pool2d(
    const Tensor& input,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    IntArrayRef dilation,
    bool ceil_mode,
    ideep::algorithm algo) {
  AT_CHECK(!ceil_mode, "Currently Mkldnn Pooling operators do not support ceil_mode.");
  auto kernel_size_vec = expand_param_if_needed(kernel_size, "kernel_size", 2);
  auto stride_vec = expand_param_if_needed(stride, "stride", 2);
  auto padding_vec = expand_param_if_needed(padding, "padding", 2);
  auto dilation_vec = expand_param_if_needed(dilation, "dilation", 2);

  const ideep::tensor& x = itensor_from_mkldnn(input);
  const std::vector<int64_t> output_sizes = pool_output_sizes(
      input.sizes(),
      kernel_size_vec,
      stride_vec,
      padding_vec,
      dilation_vec,
      ceil_mode);
  ideep::tensor y;
  ideep::pooling_forward::compute<AllocForMKLDNN>(
      x,
      {output_sizes.cbegin(), output_sizes.cend()},
      y,
      {stride_vec.cbegin(), stride_vec.cend()},
      {kernel_size_vec.cbegin(), kernel_size_vec.cend()},
      {padding_vec.cbegin(), padding_vec.cend()},
      {padding_vec.cbegin(), padding_vec.cend()},
      algo,
      ideep::prop_kind::forward);

  return new_with_itensor_mkldnn(std::move(y), input.options());
}

Tensor mkldnn_max_pool2d(
    const Tensor& input,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    IntArrayRef dilation,
    bool ceil_mode) {
  return _mkldnn_pool2d(
      input,
      kernel_size,
      stride,
      padding,
      dilation,
      ceil_mode,
      ideep::algorithm::pooling_max);
}

Tensor mkldnn_avg_pool2d(
    const Tensor& input,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    bool ceil_mode,
    bool count_include_pad) {
  return _mkldnn_pool2d(
      input,
      kernel_size,
      stride,
      padding,
      /*dilation*/ std::vector<int64_t>{1, 1},
      ceil_mode,
      count_include_pad ? ideep::algorithm::pooling_avg_include_padding
                        : ideep::algorithm::pooling_avg_exclude_padding);
}

Tensor& mkldnn_avg_pool2d_out(
    Tensor& output,
    const Tensor& input,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    bool ceil_mode,
    bool count_include_pad) {
  AT_ERROR(
      "mkldnn_avg_pool2d_out: in-place mkldnn operations are not supported yet");
}

Tensor mkldnn_adaptive_avg_pool2d(
    Tensor const& input,
    IntArrayRef output_size) {
  AT_ASSERTM(input.dim() == 4, "mkldnn_adaptive_avg_pool2d: Expect 2D input");

  auto output_size_vec =
      expand_param_if_needed(output_size, "output_size", input.dim() - 2);
  std::vector<int64_t> kernel_size(input.dim() - 2);
  for (size_t i = 2; i < input.dim(); ++i) {
    auto s1 = input.size(i);
    auto s2 = output_size_vec[i - 2];
    AT_ASSERTM(s2 != 0, "output size can not be zero");
    AT_ASSERTM(
        s1 % s2 == 0,
        "input size is not divisible by the output size is not supported yet");
    kernel_size[i - 2] = s1 / s2;
  }
  return _mkldnn_pool2d(
      input,
      kernel_size,
      /*stride*/ kernel_size,
      /*padding*/ {0, 0},
      /*dilation*/ {1, 1},
      /*ceil_mode*/ false,
      /*algo*/ ideep::algorithm::pooling_avg);
}

Tensor& mkldnn_adaptive_avg_pool2d_out(
    Tensor& output,
    const Tensor& input,
    IntArrayRef output_size) {
  AT_ERROR(
      "mkldnn_adaptive_avg_pool2d_out: in-place mkldnn operations are not supported yet");
}


} // namespace native
} // namespace at

#endif // AT_MKLDNN_ENABLED
