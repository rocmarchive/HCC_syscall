// RUN: %cxxamp -emit-llvm -S -c %s -o -|c++filt|%FileCheck %s
// UN: %amp_device -c -D__GPU__=1 -S -emit-llvm %s -o -|c++filt|%FileCheck %s
//Serialization object decl
#include <cstdlib>
namespace Concurrency {
class Serialize {
 public:
  void Append(size_t x, const void *s);
};
}

class base{
 public:
  __attribute__((annotate("deserialize"))) /* For compiler */
  base(int a_,float b_) restrict(amp) :a(a_), b(b_) {}
  int cho(void) restrict(amp);
  int a;
  float b;
};
class baz {
 public:
#if 0 // This declaration is supposed to be generated
  __attribute__((annotate("deserialize"))) /* For compiler */
  baz(base&, int foo) restrict(amp);
#endif
  int cho(void) restrict(amp) { return 0; };

  base &B;
  int bar;
};

int kerker(void) restrict(amp,cpu) {
  base b(1234, 0.0f);
  // Will pass if deserializer declaration and definition are generated
  baz bl(b, 1);
  Concurrency::Serialize s;
  bl.__cxxamp_serialize(s);
  return bl.cho();
}
// The definition should be generated by clang
// CHECK: define {{.*}}void @baz::__cxxamp_serialize(Concurrency::Serialize&)
// CHECK: call void @base::__cxxamp_serialize(Concurrency::Serialize&)
