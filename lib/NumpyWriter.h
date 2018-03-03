// lib/NumpyWriter.h

#pragma once

#include <unsupported/Eigen/CXX11/Tensor>
#include <vector>
#include <stdexcept>
#include <fcntl.h>
#include <system_error>
#include <unistd.h>

// For now, we only support writing arrays of single precision float

template <int rank>
class NumpyWriter
{
public:
  NumpyWriter(const std::string& path, const std::vector<int>& shape);
    // Opens a file at path with read/write access, and writes header
    // Only tensors with the given shape may be written.

  ~NumpyWriter();
    // Updates the header for the total number of tensors written, then closes the file

  void Append(const Eigen::Tensor<float, rank, Eigen::RowMajor>& tensor);
    // Appends the tensor to the file
    // tensor must have the shape specified in ctor
    // Tensors must use RowMajor layout to be compatible with numpy files

private:

  enum Constants {
    kMagicStrLen = 8,
    TOTAL_HEADER = 6 * 16,  // must be a multiple of 16.
      // This TOTAL_HEADER only has an excess of about 25 bytes from a minimal header
      // (rank 2, both dimension single decimal digits, less than 10 tensors in the file).
      // But 25 bytes is way more than we're likely to ever need. Suppose we have rank 3
      // 4x13x10 and 10 million tensors. In that case we use about 12 more bytes.
    kSizeofShort = sizeof(unsigned short),
    kDictOffset = kMagicStrLen + kSizeofShort,
    HEADER_LEN = TOTAL_HEADER - kDictOffset,
    kSpace = 0x20,
  };

  void Write(const void* buffer, int numBytes) const;

  void PaddedHeader(char header[HEADER_LEN]) const;
    // Fills the entire header with spaces except for last character which will be newline

  void WriteHeader() const;

  std::string ShapeFor() const;

  void UpdateFileSize() const;

private:
  const int mFile;
    // The unix file descriptor

  const std::vector<int> mShape;
    // The shape of all tensors written to this file

  off_t mLenOffset;
    // The offset from the beginning of the file where the number of tensor rows must be written

  unsigned mNumTensors;
    // The number of tensors that have been written to the file

};

template <int rank>
NumpyWriter<rank>::NumpyWriter(const std::string& path, const std::vector<int>& shape)
: mFile(::open(path.c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0644))
, mShape(shape)
, mLenOffset(0)
, mNumTensors(0)
{
  if (mFile == -1) {
    throw std::system_error(std::error_code(), "Error opening numpy file for read/write access" );
  }
  if (mShape.size() != rank) {
    throw std::invalid_argument("Shape not consistent with rank");
  }
  WriteHeader();
}

template <int rank>
NumpyWriter<rank>::~NumpyWriter()
{
  UpdateFileSize();
  ::close(mFile);
}

template <int rank>
void NumpyWriter<rank>::Write(const void* buffer, int numBytes) const
{
  ssize_t actual = ::write(mFile, buffer, numBytes);
  if (actual != numBytes) {
    throw std::system_error(std::error_code(), "Failed to write expected number of bytes");
  }
}

template <int rank>
void NumpyWriter<rank>::PaddedHeader(char fill[HEADER_LEN]) const
{
  memset(fill, kSpace, HEADER_LEN);
  assert(fill[0] == kSpace);
  assert(fill[1] == kSpace);
  assert(fill[HEADER_LEN-1] == kSpace);
  fill[HEADER_LEN-1] = '\n';
}


template <int rank>
void NumpyWriter<rank>::WriteHeader() const {
  Write("\x93NUMPY\x01\x00", kMagicStrLen);

  // Note: we're assuming this code will only run on little-endian machines.
  // Should be a safe assumption because it is highly unlikely it will ever run on anything other than x86.
  const unsigned short kHeaderLen = HEADER_LEN;
  Write(&kHeaderLen, kSizeofShort);

  char header[HEADER_LEN];
  PaddedHeader(header);
  Write(header, HEADER_LEN);
}

template <int rank>
std::string NumpyWriter<rank>::ShapeFor() const
{
  const char* kComma = ", ";
  std::stringstream ss;
  ss << "(" << mNumTensors;
  for (auto it=mShape.begin(); it!=mShape.end(); ++it) {
    ss << kComma << *it;
  }
  ss << ")";
  return ss.str();
}

template <int rank>
void NumpyWriter<rank>::UpdateFileSize() const
{
  // The <f4 is the data type for single precision float. We hard code it here.
  // To support other types, we'd need to derive the correct numpy dtype string from the C++ type.
  const char* dictFormat = "{'descr': '<f4', 'fortran_order': False, 'shape': %s}";

  char dict[HEADER_LEN];
  PaddedHeader(dict);
  int actual = snprintf(dict, HEADER_LEN-1, dictFormat, ShapeFor().c_str());
  assert(dict[actual] == 0);
  dict[actual] = kSpace;
  assert(dict[HEADER_LEN-2] == kSpace);
  assert(dict[HEADER_LEN-1] == '\n');

  lseek(mFile, kDictOffset, SEEK_SET);
  Write(dict, HEADER_LEN);
}

template <int rank>
void NumpyWriter<rank>::Append(const Eigen::Tensor<float, rank, Eigen::RowMajor>& tensor)
{
  const auto& d = tensor.dimensions();
  assert(d.size() == rank);
  for (int i=0; i<rank; i++) {
    assert(d[i] == mShape[i]);
  }
  const float* raw = tensor.data();
  ssize_t kBytes = sizeof(float)*tensor.size();
  assert(kBytes > 4);
  Write(raw, kBytes);
  ++mNumTensors;
}
