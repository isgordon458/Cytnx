#include <benchmark/benchmark.h>
#include <cytnx.hpp>

using namespace cytnx;
 
namespace BMTest_Vsplit {

static void BM_Vsplit_F64(benchmark::State& state) 
{
  //prepare data
  auto D_row = state.range(0);
  auto D_col = state.range(1);
  int len;
  std::vector<cytnx_uint64> dims(len = D_row, 1); //[1, 1, ..., 1]
  Tensor T = ones({D_row, D_col}, Type.Double);

  //start test here
  for (auto _ : state) {
    std::vector<Tensor> v_splits = algo::Vsplit(T, dims);
  }
}
BENCHMARK(BM_Vsplit_F64)
    //{D_row, D_col, tens_num}
    ->Args({10, 10})
    ->Args({100, 100})
    ->Args({1000, 1000})
    ->Args({1000, 10000});
 
static void BM_Vsplit_C128(benchmark::State& state) 
{
  //prepare data
  auto D_row = state.range(0);
  auto D_col = state.range(1);
  int len;
  std::vector<cytnx_uint64> dims(len = D_row, 1); //[1, 1, ..., 1]
  Tensor T = ones({D_row, D_col}, Type.ComplexDouble);

  //start test here
  for (auto _ : state) {
    std::vector<Tensor> v_splits = algo::Vsplit(T, dims);
  }
}
BENCHMARK(BM_Vsplit_C128)
    //{D_row, D_col, tens_num}
    ->Args({10, 10})
    ->Args({100, 100})
    ->Args({1000, 1000})
    ->Args({1000, 10000});

static void BM_Vsplit_F64_non_conti(benchmark::State& state) 
{
  //prepare data
  auto D_row = state.range(0);
  auto D_col = state.range(1);
  int len;
  std::vector<cytnx_uint64> dims(len = D_row, 1); //[1, 1, ..., 1]
  Tensor T = ones({D_col, D_row}, Type.Double);
  T.permute_({1, 0}); // change row <-> col

  //start test here
  for (auto _ : state) {
    std::vector<Tensor> v_splits = algo::Vsplit(T, dims);
  }
}
BENCHMARK(BM_Vsplit_F64_non_conti)
    //{D_row, D_col, tens_num}
    ->Args({10, 10})
    ->Args({100, 100})
    ->Args({1000, 1000})
    ->Args({1000, 10000});

} //namespace

