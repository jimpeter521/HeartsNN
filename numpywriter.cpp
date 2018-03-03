// numpywriter.cpp

#include "lib/NumpyWriter.h"
#include <unsupported/Eigen/CXX11/Tensor>

int main(int argc, char** argv)
{
  Eigen::Tensor<float, 2, Eigen::RowMajor> data1(2, 3);
  data1.setValues({{0., 1., 2.}, {3., 4., 5.}});
  Eigen::Tensor<float, 2, Eigen::RowMajor> data2(2, 3);
  data2.setValues({{6., 7., 8.}, {9., 10., 11.}});

  std::vector<int> shape({2, 3});
  NumpyWriter<2> w("test.npy", shape);
  w.Append(data1);
  w.Append(data2);
}
