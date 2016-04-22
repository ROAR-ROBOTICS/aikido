#include <stdexcept>
#include <aikido/constraint/uniform/RealVectorBoxConstraint.hpp>

namespace aikido {
namespace statespace {

using constraint::ConstraintType;

//=============================================================================
class RealVectorBoxConstraintSampleGenerator
  : public constraint::SampleGenerator
{
public:
  statespace::StateSpacePtr getStateSpace() const override;

  bool sample(statespace::StateSpace::State* _state) override;

  int getNumSamples() const override;

  bool canSample() const override;

private:
  RealVectorBoxConstraintSampleGenerator(
    std::shared_ptr<statespace::RealVectorStateSpace> _space,
    std::unique_ptr<util::RNG> _rng,
    const Eigen::VectorXd& _lowerLimits,
    const Eigen::VectorXd& _upperLimits);

  std::shared_ptr<statespace::RealVectorStateSpace> mSpace;
  std::unique_ptr<util::RNG> mRng;
  std::vector<std::uniform_real_distribution<double>> mDistributions;

  friend class RealVectorBoxConstraint;
};

//=============================================================================
RealVectorBoxConstraintSampleGenerator::RealVectorBoxConstraintSampleGenerator(
      std::shared_ptr<statespace::RealVectorStateSpace> _space,
      std::unique_ptr<util::RNG> _rng,
      const Eigen::VectorXd& _lowerLimits,
      const Eigen::VectorXd& _upperLimits)
  : mSpace(std::move(_space))
  , mRng(std::move(_rng))
{
  const auto dimension = mSpace->getDimension();
  mDistributions.reserve(dimension);

  for (size_t i = 0; i < dimension; ++i)
    mDistributions.emplace_back(_lowerLimits[i], _upperLimits[i]);
}

//=============================================================================
statespace::StateSpacePtr
  RealVectorBoxConstraintSampleGenerator::getStateSpace() const
{
  return mSpace;
}

//=============================================================================
bool RealVectorBoxConstraintSampleGenerator::sample(
  statespace::StateSpace::State* _state)
{
  Eigen::VectorXd value(mDistributions.size());

  for (size_t i = 0; i < value.size(); ++i)
    value[i] = mDistributions[i](*mRng);

  mSpace->setValue(static_cast<RealVectorStateSpace::State*>(_state), value);

  return true;
}

//=============================================================================
int RealVectorBoxConstraintSampleGenerator::getNumSamples() const
{
  return NO_LIMIT;
}

//=============================================================================
bool RealVectorBoxConstraintSampleGenerator::canSample() const
{
  return true;
}

//=============================================================================
RealVectorBoxConstraint
  ::RealVectorBoxConstraint(
      std::shared_ptr<statespace::RealVectorStateSpace> _space,
      std::unique_ptr<util::RNG> _rng,
      const Eigen::VectorXd& _lowerLimits,
      const Eigen::VectorXd& _upperLimits)
  : mSpace(std::move(_space))
  , mRng(std::move(_rng))
  , mLowerLimits(_lowerLimits)
  , mUpperLimits(_upperLimits)
{
  if (!mSpace)
    throw std::invalid_argument("StateSpace is null.");

  const auto dimension = mSpace->getDimension();

  if (mLowerLimits.size() != dimension)
  {
    std::stringstream msg;
    msg << "Lower limits have incorrect dimension: expected "
        << mSpace->getDimension() << ", got " << mLowerLimits.size() << ".";
    throw std::invalid_argument(msg.str());
  }

  if (mUpperLimits.size() != dimension)
  {
    std::stringstream msg;
    msg << "Upper limits have incorrect dimension: expected "
        << mSpace->getDimension() << ", got " << mUpperLimits.size() << ".";
    throw std::invalid_argument(msg.str());
  }

  for (size_t i = 0; i < dimension; ++i)
  {
    if (mLowerLimits[i] > mUpperLimits[i])
    {
      std::stringstream msg;
      msg << "Unable to sample from StateSpace because lower limit exeeds"
             " upper limit on dimension " << i << ": "
          << mLowerLimits[i] << " > " << mUpperLimits[i] << ".";
      throw std::invalid_argument(msg.str());
    }
  }
}

//=============================================================================
statespace::StateSpacePtr RealVectorBoxConstraint::getStateSpace() const
{
  return mSpace;
}

//=============================================================================
size_t RealVectorBoxConstraint::getConstraintDimension() const
{
  // TODO: Only create constraints for bounded dimensions.
  return mSpace->getDimension();
}

//=============================================================================
std::vector<ConstraintType> RealVectorBoxConstraint::getConstraintTypes() const
{
  return std::vector<ConstraintType>(
    mSpace->getDimension(), ConstraintType::INEQUALITY);
}

//=============================================================================
bool RealVectorBoxConstraint::isSatisfied(const StateSpace::State* state) const
{
  const auto value = mSpace->getValue(
    static_cast<const RealVectorStateSpace::State*>(state));

  for (size_t i = 0; i < value.size(); ++i)
  {
    if (value[i] < mLowerLimits[i] || value[i] > mUpperLimits[i])
      return false;
  }
  return true;
}

//=============================================================================
bool RealVectorBoxConstraint::project(
  const statespace::StateSpace::State* _s,
  statespace::StateSpace::State* _out) const
{
  Eigen::VectorXd value = mSpace->getValue(
    static_cast<const RealVectorStateSpace::State*>(_s));

  for (size_t i = 0; i < value.size(); ++i)
  {
    if (value[i] < mLowerLimits[i])
      value[i] = mLowerLimits[i];
    else if (value[i] > mUpperLimits[i])
      value[i] = mUpperLimits[i];
  }

  mSpace->setValue(
    static_cast<RealVectorStateSpace::State*>(_out), value);

  return true;
}

//=============================================================================
Eigen::VectorXd RealVectorBoxConstraint::getValue(
  const statespace::StateSpace::State* _s) const
{
  auto stateValue = mSpace->getValue(
    static_cast<const RealVectorStateSpace::State*>(_s));

  const size_t dimension = mSpace->getDimension();
  Eigen::VectorXd constraintValue(dimension);

  for (size_t i = 0; i < dimension; ++i)
  {
    if (stateValue[i] < mLowerLimits[i])
      constraintValue[i] = stateValue[i] - mLowerLimits[i];
    else if (stateValue[i] > mUpperLimits[i])
      constraintValue[i] = mUpperLimits[i] - stateValue[i];
    else
      constraintValue[i] = 0.;
  }

  return constraintValue;
}

//=============================================================================
Eigen::MatrixXd RealVectorBoxConstraint::getJacobian(
  const statespace::StateSpace::State* _s) const
{
  auto stateValue = mSpace->getValue(
    static_cast<const RealVectorStateSpace::State*>(_s));

  const size_t dimension = mSpace->getDimension();
  Eigen::MatrixXd jacobian = Eigen::MatrixXd::Zero(dimension, dimension);

  for (size_t i = 0; i < jacobian.rows(); ++i)
  {
    if (stateValue[i] < mLowerLimits[i])
      jacobian(i, i) = -1.;
    else if (stateValue[i] > mUpperLimits[i])
      jacobian(i, i) =  1.;
    else
      jacobian(i, i) =  0.;
  }

  return jacobian;
}

//=============================================================================
std::pair<Eigen::VectorXd, Eigen::MatrixXd> RealVectorBoxConstraint
  ::getValueAndJacobian(const statespace::StateSpace::State* _s) const
{
  // TODO: Avoid calling getValue twice here.
  return std::make_pair(getValue(_s), getJacobian(_s));
}

//=============================================================================
std::unique_ptr<constraint::SampleGenerator>
  RealVectorBoxConstraint::createSampleGenerator() const
{  
  if (!mRng)
    throw std::invalid_argument("mRng is null.");

  for (size_t i = 0; i < mSpace->getDimension(); ++i)
  {
    if (!std::isfinite(mLowerLimits[i]) || !std::isfinite(mUpperLimits[i]))
    {
      std::stringstream msg;
      msg << "Unable to sample from StateSpace because dimension "
          << i << " is unbounded.";
      throw std::runtime_error(msg.str());
    }
  }

  return std::unique_ptr<RealVectorBoxConstraintSampleGenerator>(
    new RealVectorBoxConstraintSampleGenerator(
      mSpace, mRng->clone(), mLowerLimits, mUpperLimits));
}

//=============================================================================
Eigen::VectorXd RealVectorBoxConstraint::getLowerLimits()
{
  return mLowerLimits;
}

//=============================================================================
Eigen::VectorXd RealVectorBoxConstraint::getUpperLimits()
{
  return mUpperLimits;
}

} // namespace statespace
} // namespace aikido
