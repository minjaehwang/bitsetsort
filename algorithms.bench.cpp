
#include <algorithm>
#include <cstdint>
#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "CartesianBenchmarks.h"
#include "GenerateInput.h"
#include "benchmark/benchmark.h"
#include "test_macros.h"

#include "bitsetsort.h"
#include "pdqsort/pdqsort.h"

namespace {

enum class ValueType { Uint32, Uint64, Pair, Tuple, String };
struct AllValueTypes : EnumValuesAsTuple<AllValueTypes, ValueType, 5> {
  static constexpr const char* Names[] = {
      "uint32", "uint64", "pair<uint32, uint32>",
      "tuple<uint32, uint64, uint32>", "string"};
};

template <class V>
using Value = std::conditional_t<
    V() == ValueType::Uint32, uint32_t,
    std::conditional_t<
        V() == ValueType::Uint64, uint64_t,
        std::conditional_t<
            V() == ValueType::Pair, std::pair<uint32_t, uint32_t>,
            std::conditional_t<V() == ValueType::Tuple,
                               std::tuple<uint32_t, uint64_t, uint32_t>,
                               std::string> > > >;

enum class Order {
  Random,
  Ascending,
  Descending,
  SingleElement,
  PipeOrgan,
  Heap
};
struct AllOrders : EnumValuesAsTuple<AllOrders, Order, 6> {
  static constexpr const char* Names[] = {"Random",     "Ascending",
                                          "Descending", "SingleElement",
                                          "PipeOrgan",  "Heap"};
};

template <typename T>
void fillValues(std::vector<T>& V, size_t N, Order O) {
  if (O == Order::SingleElement) {
    V.resize(N, 0);
  } else {
    while (V.size() < N)
      V.push_back(V.size());
  }
}

template <typename T>
void fillValues(std::vector<std::pair<T, T> >& V, size_t N, Order O) {
  if (O == Order::SingleElement) {
    V.resize(N, std::make_pair(0, 0));
  } else {
    while (V.size() < N)
      // Half of array will have the same first element.
      if (V.size() % 2) {
        V.push_back(std::make_pair(V.size(), V.size()));
      } else {
        V.push_back(std::make_pair(0, V.size()));
      }
  }
}

template <typename T1, typename T2, typename T3>
void fillValues(std::vector<std::tuple<T1, T2, T3> >& V, size_t N, Order O) {
  if (O == Order::SingleElement) {
    V.resize(N, std::make_tuple(0, 0, 0));
  } else {
    while (V.size() < N)
      // One third of array will have the same first element.
      // One third of array will have the same first element and the same second element.
      switch (V.size() % 3) {
      case 0:
        V.push_back(std::make_tuple(V.size(), V.size(), V.size()));
        break;
      case 1:
        V.push_back(std::make_tuple(0, V.size(), V.size()));
        break;
      case 2:
        V.push_back(std::make_tuple(0, 0, V.size()));
        break;
      }
  }
}

void fillValues(std::vector<std::string>& V, size_t N, Order O) {
  if (O == Order::SingleElement) {
    V.resize(N, getRandomString(64));
  } else {
    while (V.size() < N)
      V.push_back(getRandomString(64));
  }
}

template <class T>
void sortValues(T& V, Order O) {
  assert(std::is_sorted(V.begin(), V.end()));
  switch (O) {
  case Order::Random: {
    std::random_device R;
    std::mt19937 M(R());
    std::shuffle(V.begin(), V.end(), M);
    break;
  }
  case Order::Ascending:
    std::sort(V.begin(), V.end());
    break;
  case Order::Descending:
    std::sort(V.begin(), V.end(), std::greater<>());
    break;
  case Order::SingleElement:
    // Nothing to do
    break;
  case Order::PipeOrgan:
    std::sort(V.begin(), V.end());
    std::reverse(V.begin() + V.size() / 2, V.end());
    break;
  case Order::Heap:
    std::make_heap(V.begin(), V.end());
    break;
  }
}

constexpr size_t TestSetElements =
#if !TEST_HAS_FEATURE(memory_sanitizer)
    1 << 18;
#else
    1 << 14;
#endif

template <class ValueType>
std::vector<std::vector<Value<ValueType> > > makeOrderedValues(size_t N,
                                                               Order O) {
  std::vector<std::vector<Value<ValueType> > > Ret;
  const size_t NumCopies = std::max(size_t{1}, TestSetElements / N);
  Ret.resize(NumCopies);
  for (auto& V : Ret) {
    fillValues(V, N, O);
    sortValues(V, O);
  }
  return Ret;
}

template <class T, class U>
TEST_ALWAYS_INLINE void resetCopies(benchmark::State& state, T& Copies,
                                    U& Orig) {
  state.PauseTiming();
  for (auto& Copy : Copies)
    Copy = Orig;
  state.ResumeTiming();
}

enum class BatchSize {
  CountElements,
  CountBatch,
};

template <class ValueType, class F>
void runOpOnCopies(benchmark::State& state, size_t Quantity, Order O,
                   BatchSize Count, F Body) {
  auto Copies = makeOrderedValues<ValueType>(Quantity, O);
  auto Orig = Copies;

  const size_t Batch = Count == BatchSize::CountElements
                           ? Copies.size() * Quantity
                           : Copies.size();
  while (state.KeepRunningBatch(Batch)) {
    for (auto& Copy : Copies) {
      Body(Copy);
      benchmark::DoNotOptimize(Copy);
    }
    state.PauseTiming();
    Copies = Orig;
    state.ResumeTiming();
  }
}

template <class ValueType, class Order>
struct Sort {
  size_t Quantity;

  void run(benchmark::State& state) const {
    runOpOnCopies<ValueType>(
        state, Quantity, Order(), BatchSize::CountElements,
        [](auto& Copy) { std::sort(Copy.begin(), Copy.end()); });
  }

  bool skip() const { return Order() == ::Order::Heap; }

  std::string name() const {
    return "BM_Sort" + ValueType::name() + Order::name() + "_" +
           std::to_string(Quantity);
  };
};

template <class ValueType, class Order>
struct PdqSort {
  size_t Quantity;

  void run(benchmark::State& state) const {
    runOpOnCopies<ValueType>(
        state, Quantity, Order(), BatchSize::CountElements,
        [](auto& Copy) { pdqsort(Copy.begin(), Copy.end()); });
  }

  bool skip() const { return Order() == ::Order::Heap; }

  std::string name() const {
    return "BM_PdqSort" + ValueType::name() + Order::name() + "_" +
           std::to_string(Quantity);
  };
};

template <class ValueType, class Order>
struct BitsetSort {
  size_t Quantity;

  void run(benchmark::State& state) const {
    runOpOnCopies<ValueType>(
        state, Quantity, Order(), BatchSize::CountElements,
        [](auto& Copy) { stdext::bitsetsort(Copy.begin(), Copy.end()); });
  }

  bool skip() const { return Order() == ::Order::Heap; }

  std::string name() const {
    return "BM_BitsetSort" + ValueType::name() + Order::name() + "_" +
           std::to_string(Quantity);
  };
};

template <class ValueType, class Order>
struct StableSort {
  size_t Quantity;

  void run(benchmark::State& state) const {
    runOpOnCopies<ValueType>(
        state, Quantity, Order(), BatchSize::CountElements,
        [](auto& Copy) { std::stable_sort(Copy.begin(), Copy.end()); });
  }

  bool skip() const { return Order() == ::Order::Heap; }

  std::string name() const {
    return "BM_StableSort" + ValueType::name() + Order::name() + "_" +
           std::to_string(Quantity);
  };
};

template <class ValueType, class Order>
struct MakeHeap {
  size_t Quantity;

  void run(benchmark::State& state) const {
    runOpOnCopies<ValueType>(
        state, Quantity, Order(), BatchSize::CountElements,
        [](auto& Copy) { std::make_heap(Copy.begin(), Copy.end()); });
  }

  std::string name() const {
    return "BM_MakeHeap" + ValueType::name() + Order::name() + "_" +
           std::to_string(Quantity);
  };
};

template <class ValueType>
struct SortHeap {
  size_t Quantity;

  void run(benchmark::State& state) const {
    runOpOnCopies<ValueType>(
        state, Quantity, Order::Heap, BatchSize::CountElements,
        [](auto& Copy) { std::sort_heap(Copy.begin(), Copy.end()); });
  }

  std::string name() const {
    return "BM_SortHeap" + ValueType::name() + "_" + std::to_string(Quantity);
  };
};

template <class ValueType, class Order>
struct MakeThenSortHeap {
  size_t Quantity;

  void run(benchmark::State& state) const {
    runOpOnCopies<ValueType>(state, Quantity, Order(), BatchSize::CountElements,
                             [](auto& Copy) {
                               std::make_heap(Copy.begin(), Copy.end());
                               std::sort_heap(Copy.begin(), Copy.end());
                             });
  }

  std::string name() const {
    return "BM_MakeThenSortHeap" + ValueType::name() + Order::name() + "_" +
           std::to_string(Quantity);
  };
};

template <class ValueType, class Order>
struct PushHeap {
  size_t Quantity;

  void run(benchmark::State& state) const {
    runOpOnCopies<ValueType>(
        state, Quantity, Order(), BatchSize::CountElements, [](auto& Copy) {
          for (auto I = Copy.begin(), E = Copy.end(); I != E; ++I) {
            std::push_heap(Copy.begin(), I + 1);
          }
        });
  }

  bool skip() const { return Order() == ::Order::Heap; }

  std::string name() const {
    return "BM_PushHeap" + ValueType::name() + Order::name() + "_" +
           std::to_string(Quantity);
  };
};

template <class ValueType>
struct PopHeap {
  size_t Quantity;

  void run(benchmark::State& state) const {
    runOpOnCopies<ValueType>(
        state, Quantity, Order(), BatchSize::CountElements, [](auto& Copy) {
          for (auto B = Copy.begin(), I = Copy.end(); I != B; --I) {
            std::pop_heap(B, I);
          }
        });
  }

  std::string name() const {
    return "BM_PopHeap" + ValueType::name() + "_" + std::to_string(Quantity);
  };
};

} // namespace

int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);
  if (benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;

  const std::vector<size_t> Quantities = {1 << 0, 1 << 2,  1 << 4,  1 << 6,
                                          1 << 8, 1 << 10, 1 << 14,
    // Running each benchmark in parallel consumes too much memory with MSAN
    // and can lead to the test process being killed.
#if !TEST_HAS_FEATURE(memory_sanitizer)
                                          1 << 18
#endif
  };
  makeCartesianProductBenchmark<Sort, AllValueTypes, AllOrders>(Quantities);
  makeCartesianProductBenchmark<BitsetSort, AllValueTypes, AllOrders>(Quantities);
  makeCartesianProductBenchmark<PdqSort, AllValueTypes, AllOrders>(Quantities);
  makeCartesianProductBenchmark<StableSort, AllValueTypes, AllOrders>(
      Quantities);
  makeCartesianProductBenchmark<MakeHeap, AllValueTypes, AllOrders>(Quantities);
  makeCartesianProductBenchmark<SortHeap, AllValueTypes>(Quantities);
  makeCartesianProductBenchmark<MakeThenSortHeap, AllValueTypes, AllOrders>(
      Quantities);
  makeCartesianProductBenchmark<PushHeap, AllValueTypes, AllOrders>(Quantities);
  makeCartesianProductBenchmark<PopHeap, AllValueTypes>(Quantities);
  benchmark::RunSpecifiedBenchmarks();
}