#ifndef ASLAM_CV_NUMDIFF_JACOBIAN_TESTER_H_
#define ASLAM_CV_NUMDIFF_JACOBIAN_TESTER_H_

#include <Eigen/Dense>
#include <glog/logging.h>

#include <aslam/common/eigen-helpers.h>

namespace aslam {
namespace common {

// Enable/disable debug outputs.
#define NUMDIFF_DEBUG_OUTPUT false

/// \def TEST_JACOBIAN_FINITE_DIFFERENCE(FUNCTOR_TYPE, X, STEP, TOLERANCE, ...)
///  Test macro and numerical differentiator to unit test Jacobian implementations.
///  @param[in] FUNCTOR_TYPE Functor that wraps the value and symbolic Jacobian generating function
///                          to the required form. See example below.
///  @param[in] X Evaluation point. (struct Functor : public aslam::common::NumDiffFunctor<Nf, Nx>)
///               Nf: output dimension, Nx: number of parameters --> dim(J)=(Nf x Nx)
///  @param[in] STEP Step size used for the numerical differentiation. (Eigen::VectorXd)
///  @param[in] TOLERANCE Tolerance used for comparing the numerical and symbolic Jacobian
///                       evaluations. (double)
///  @param[in] ... Argument list forwarded to Functor constructor. (__VA_ARGS__)
///
/// Example:
/// @code
///   struct Functor : public aslam::common::NumDiffFunctor<2, 3> {
///    Functor(MyClass::Ptr my_class) : my_class_(my_class) {};
///    virtual ~Functor() {};
///
///    void functional(const typename NumDiffFunctor::InputType& x,
///                    typename NumDiffFunctor::ValueType& fvec,
///                    typename NumDiffFunctor::JacobianType* Jout) const {
///        fvec = getValue(x,my_class->params);
///        Jout = my_class_->getAnalyticalJacobian(x, my_class->params);
///    };
///
///    MyClass::Ptr my_class_;
///  };
///
///  double stepsize = 1e-3;
///  double test_tolerance = 1e-2;
///  Eigen::Vector2d x0(0.0, 1.0); // Evaluation point
///  TEST_JACOBIAN_FINITE_DIFFERENCE(Functor, x0, stepsize, tolerance, my_class_);
///
/// @endcode
#define TEST_JACOBIAN_FINITE_DIFFERENCE(FUNCTOR_TYPE, X, STEP, TOLERANCE, ...) \
    do {\
      FUNCTOR_TYPE functor(__VA_ARGS__);                                 \
      aslam::common::NumericalDiff<FUNCTOR_TYPE> numDiff(functor, STEP); \
      typename FUNCTOR_TYPE::JacobianType Jnumeric;                      \
      bool success = numDiff.getJacobianNumerical(X, Jnumeric);          \
      EXPECT_TRUE(success) << "Num. differentiation failed!";            \
      typename FUNCTOR_TYPE::JacobianType Jsymbolic;                     \
      success = functor.getJacobian(X, &Jsymbolic);                      \
      EXPECT_TRUE(success) << "Getting analytical Jacobian failed!";     \
      EXPECT_NEAR_EIGEN(Jnumeric, Jsymbolic, TOLERANCE);                 \
      VLOG(3) << "Jnumeric: " << Jnumeric << "\n";                       \
      VLOG(3) << "Jsymbolic: " << Jsymbolic << "\n";                     \
    } while (0)

// Functor base for numerical differentiation.
template<int NY, int NX, typename _Scalar = double>
struct NumDiffFunctor {
  // Type definitions.
  enum {
    InputsAtCompileTime = NX,
    ValuesAtCompileTime = NY
  };

  typedef _Scalar Scalar;
  typedef Eigen::Matrix<Scalar, InputsAtCompileTime, 1> InputType;
  typedef Eigen::Matrix<Scalar, ValuesAtCompileTime, 1> ValueType;
  typedef Eigen::Matrix<Scalar, ValuesAtCompileTime, InputsAtCompileTime> JacobianType;

  NumDiffFunctor() {};
  virtual ~NumDiffFunctor() {};

  virtual bool functional(const typename NumDiffFunctor::InputType& x,
                          typename NumDiffFunctor::ValueType& fvec,
                          typename NumDiffFunctor::JacobianType* Jout) const = 0;

  bool operator()(const InputType& x, ValueType& fvec) const {
    return functional(x, fvec, nullptr);
  };

  bool getJacobian(const InputType& x,
                   JacobianType* out_jacobian) const {
    ValueType fvec;
    return functional(x, fvec, out_jacobian);
  };
};

/// Differentiation schemes for class NumericalDiff.
enum NumericalDiffMode {
  Forward,
  Central,
  CentralSecond
};

/// \class NumericalDiff
/// \brief Modified numerical differentiation code from unsupported/Eigen library
template<typename _Functor, NumericalDiffMode mode = CentralSecond>
class NumericalDiff : public _Functor {
 public:
  typedef _Functor Functor;
  typedef typename Functor::Scalar Scalar;
  typedef typename Functor::InputType InputType;
  typedef typename Functor::ValueType ValueType;
  typedef typename Functor::JacobianType JacobianType;

  NumericalDiff(const Functor& f, Scalar _epsfcn = 0.)
      : Functor(f),
        epsfcn(_epsfcn) {}

  enum {
    InputsAtCompileTime = Functor::InputsAtCompileTime,
    ValuesAtCompileTime = Functor::ValuesAtCompileTime
  };

  bool getJacobianNumerical(const InputType& _x, JacobianType &jac) const {
    using std::sqrt;
    using std::abs;
    /* Local variables */
    Scalar h;
    const typename InputType::Index n = _x.size();
    const Scalar eps = sqrt(((std::max)(epsfcn, Eigen::NumTraits<Scalar>::epsilon())));

    // Build jacobian.
    InputType x = _x;
    ValueType val1, val2, val3, val4;
    bool success = true;  // Success of value generating function.

    for (int j = 0; j < n; ++j) {
      h = eps * abs(x[j]);
      if (h == 0.) {
        h = eps;
      }
      switch (mode) {
        case Forward:
          success &= Functor::operator()(x, val1);
          x[j] += h;
          success &= Functor::operator()(x, val2);
          x[j] = _x[j];
          jac.col(j) = (val2 - val1) / h;
          break;
        case Central:
          x[j] += h;
          success &= Functor::operator()(x, val2);
          x[j] -= 2 * h;
          success &= Functor::operator()(x, val1);
          x[j] = _x[j];
          jac.col(j) = (val2 - val1) / (2 * h);
          break;
        case CentralSecond:
          x[j] += 2.0 * h;
          success &= Functor::operator()(x, val1);
          x[j] -= h;
          success &= Functor::operator()(x, val2);
          x[j] -= 2.0 * h;
          success &= Functor::operator()(x, val3);
          x[j] -= h;
          success &= Functor::operator()(x, val4);
          x[j] = _x[j];
          jac.col(j) = ((8.0 * val2) + val4 - val1 - (8.0 * val3)) / (h * 12.0);
          break;
        default:
          eigen_assert(false);
      };
    }
    return success;
  }
 private:
  Scalar epsfcn;
  NumericalDiff& operator=(const NumericalDiff&);
};

}  // namespace common
}  // namespace aslam

#endif  // ASLAM_CV_NUMDIFF_JACOBIAN_TESTER_H_