#include <cassert>
#include <sstream>
#include <aikido/statespace/DartStateSpace.hpp>
#include <aikido/statespace/RealVectorStateSpace.hpp>
#include <aikido/statespace/RealVectorJointStateSpace.hpp>
#include <aikido/statespace/SO2StateSpace.hpp>
#include <aikido/statespace/SO2JointStateSpace.hpp>
#include <aikido/statespace/SO3StateSpace.hpp>
#include <aikido/statespace/SE2StateSpace.hpp>
#include <aikido/statespace/SE3StateSpace.hpp>
#include <aikido/statespace/SE3JointStateSpace.hpp>
#include <aikido/statespace/CompoundStateSpace.hpp>
#include <dart/common/StlHelpers.h>
#include <dart/common/console.h>

using dart::dynamics::MetaSkeleton;
using dart::dynamics::MetaSkeletonPtr;
using dart::dynamics::BallJoint;
using dart::dynamics::FreeJoint;
using dart::dynamics::EulerJoint;
using dart::dynamics::PlanarJoint;
using dart::dynamics::PrismaticJoint;
using dart::dynamics::RevoluteJoint;
using dart::dynamics::Joint;
using dart::dynamics::ScrewJoint;
using dart::dynamics::TranslationalJoint;
using dart::dynamics::SingleDofJoint;
using dart::dynamics::WeldJoint;
using dart::dynamics::INVALID_INDEX;

namespace aikido {
namespace statespace {

using JointStateSpacePtr = std::shared_ptr<JointStateSpace>;

namespace {

//=============================================================================
template <class Input, class Output>
std::vector<Output> convertVectorType(const std::vector<Input>& _input)
{
  std::vector<Output> output;
  output.reserve(_input.size());

  for (const auto& x : _input)
    output.emplace_back(x);

  return std::move(output);
}

//=============================================================================
template <class T>
T* isJointOfType(dart::dynamics::Joint* _joint)
{
  // It's safe to do a pointer comparison here, since getType is guaranteed to
  // return the same reference to the corresponding getTypeStatic method.
  if (&_joint->getType() == &T::getStaticType())
    return static_cast<T*>(_joint);
  else
    return nullptr;
}

//=============================================================================
std::shared_ptr<JointStateSpace> createStateSpace(Joint* _joint)
{
  if (const auto revolute_joint = isJointOfType<RevoluteJoint>(_joint))
  {
    if (_joint->isCyclic(0))
      return std::make_shared<SO2JointStateSpace>(revolute_joint);
    else
      return std::make_shared<RealVectorJointStateSpace>(revolute_joint);
  }
  else if (isJointOfType<PrismaticJoint>(_joint))
  {
    return std::make_shared<RealVectorJointStateSpace>(_joint);
  }
  else if (isJointOfType<TranslationalJoint>(_joint))
  {
    return std::make_shared<RealVectorJointStateSpace>(_joint);
  }
  else if (const auto free_joint = isJointOfType<FreeJoint>(_joint))
  {
    return std::make_shared<SE3JointStateSpace>(free_joint);
  }
  // TODO: Handle PlanarJoint, BallJoint, WeldJoint, and EulerJoint.
  else
  {
    std::stringstream msg;
    msg << "Joint '" << _joint->getName() << "' has unsupported type '"
         << _joint->getType() << "'.";
    throw std::runtime_error(msg.str());
  }
}

//=============================================================================
std::vector<std::shared_ptr<JointStateSpace>> createStateSpace(
  MetaSkeleton& _metaskeleton)
{
  std::vector<std::shared_ptr<JointStateSpace>> spaces;
  spaces.reserve(_metaskeleton.getNumJoints());

  for (size_t ijoint = 0; ijoint < _metaskeleton.getNumJoints(); ++ijoint)
  {
    const auto joint = _metaskeleton.getJoint(ijoint);

    // Verify that the joint is not missing any DOFs. This could alter the
    // topology of the space that we create.
    for (size_t idof = 0; idof < joint->getNumDofs(); ++idof)
    {
      const auto dof = joint->getDof(idof);
      if (_metaskeleton.getIndexOf(dof, false) == INVALID_INDEX)
      {
        std::stringstream msg;
        msg << "MetaSkeleton is missing DegreeOfFreedom '" << dof->getName()
            << "' (index: " << idof << ") of Joint '" << joint->getName()
            << "'.";
        throw std::runtime_error(msg.str());
      }
    }

    spaces.emplace_back(createStateSpace(joint));
  }

  return std::move(spaces);
}

} // namespace

//=============================================================================
MetaSkeletonStateSpace::MetaSkeletonStateSpace(MetaSkeletonPtr _metaskeleton)
  : CompoundStateSpace(
      convertVectorType<JointStateSpacePtr, StateSpacePtr>(
        createStateSpace(*_metaskeleton)))
  , mMetaSkeleton(std::move(_metaskeleton))
{
}

//=============================================================================
MetaSkeletonPtr MetaSkeletonStateSpace::getMetaSkeleton() const
{
  return mMetaSkeleton;
}

//=============================================================================
void MetaSkeletonStateSpace::getStateFromMetaSkeleton(State* _state) const
{
  for (size_t ijoint = 0; ijoint < getNumStates(); ++ijoint)
  {
    auto jointSpace = getSubSpace<JointStateSpace>(ijoint);
    jointSpace->getState(getSubState(_state, ijoint));
  }
}

//=============================================================================
auto MetaSkeletonStateSpace::getScopedStateFromMetaSkeleton() const
  -> ScopedState
{
  auto scopedState = createState();
  getStateFromMetaSkeleton(scopedState.getState());
  return std::move(scopedState);
}

//=============================================================================
void MetaSkeletonStateSpace::setStateOnMetaSkeleton(const State* _state)
{
  for (size_t ijoint = 0; ijoint < getNumStates(); ++ijoint)
  {
    auto jointSpace = getSubSpace<JointStateSpace>(ijoint);
    jointSpace->setState(getSubState(_state, ijoint));
  }
}

} // namespace statespace
} // namespace aikido
