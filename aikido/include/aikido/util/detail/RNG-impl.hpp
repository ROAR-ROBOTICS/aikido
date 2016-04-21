#include <cassert>

namespace aikido {
namespace util {

//=============================================================================
template <class Engine, class Scalar, class Quaternion>
Quaternion sampleQuaternion(
  Engine& _engine, std::uniform_real_distribution<Scalar>& _distribution)
{
  assert(_distribution.a() == 0.);
  assert(_distribution.b() == 1.);

  const double u1 = _distribution(_engine);
  const double u2 = _distribution(_engine);
  const double u3 = _distribution(_engine);

  return Quaternion(
    std::sqrt(1. - u1) * std::sin(2. * M_PI * u2),
    std::sqrt(1. - u1) * std::cos(2. * M_PI * u2),
    std::sqrt(u1) * std::sin(2. * M_PI * u3),
    std::sqrt(u1) * std::cos(2. * M_PI * u3));
}

} // namespace util
} // namespace aikido
