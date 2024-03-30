#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

// "src/external"
//#include "external/staticstruct.hh"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

//

#include "primvar.hh"
#include "pprinter.hh"
#include "value-types.hh"
#include "value-pprint.hh"

namespace tinyusdz {
namespace primvar {

bool PrimVar::get_interpolated_value(const double t, const value::TimeSampleInterpolationType tinterp, value::Value *dst) const {

  if (is_blocked()) {
    return false;
  }

  if (is_scalar()) {
    (*dst) = _value;
    return true;
  }

  if (is_timesamples()) {
    const std::vector<value::TimeSamples::Sample> &samples = _ts.get_samples();

    if (samples.empty()) {
      // ???
      return false;
    }

    if (value::TimeCode(t).is_default())  {
      // FIXME: Use the first item for now.
      if (samples[0].blocked) {
        return false;
      }

      (*dst) = samples[0].value;
      return true;
    } else {
      auto it = std::lower_bound(
          samples.begin(), samples.end(), t,
          [](const value::TimeSamples::Sample &a, double tval) { return a.t < tval; });

      if (tinterp == value::TimeSampleInterpolationType::Linear) {
        size_t idx0 = size_t(std::max(
            int64_t(0),
            std::min(int64_t(samples.size() - 1),
                     int64_t(std::distance(samples.begin(), it - 1)))));
        size_t idx1 =
            size_t(std::max(int64_t(0), std::min(int64_t(samples.size() - 1),
                                                 int64_t(idx0) + 1)));

        double tl = samples[idx0].t;
        double tu = samples[idx1].t;

        double dt = (t - tl);
        if (std::fabs(tu - tl) < std::numeric_limits<double>::epsilon()) {
          // slope is zero.
          dt = 0.0;
        } else {
          dt /= (tu - tl);
        }

        // Just in case.
        dt = std::max(0.0, std::min(1.0, dt));

        const value::Value &p0 = samples[idx0].value;
        const value::Value &p1 = samples[idx1].value;

        bool ret = value::Lerp(p0, p1, dt, dst);
        return ret;
      } else {
        if (it == samples.end()) {
          // ???
          return false;
        }

        (*dst) = it->value;
        return true;
      }
    }
  }

  return false;
}

} // namespace primvar
}  // namespace tinyusdz

