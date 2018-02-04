#include <vector>

namespace {

struct S {
  int a;
  int b;
  void *c;
};

class X {
public:
  explicit X(S s): m_s(s) {}
  void test(S s) {
    m_s = s;
  }
  int geta() const {
    return m_s.a;
  }
private:
  S m_s;
  std::vector<S> m_v;
};

int test() {
  S s1;
  s1.a = 4;

  S s2;
  s2.a = 42;

  X x(s1);
  x.test(s2);

  return x.geta();
}

}

extern "C" {

int main() {
  return test();
}

}

