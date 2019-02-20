// Copyright (c) 2013, 2017 The ComPWA Team.
// This file is part of the ComPWA framework, check
// https://github.com/ComPWA/ComPWA/license.txt for details.

#ifndef PHYSICS_NORMALIZATIONINTENSITYDECORATOR_HPP_
#define PHYSICS_NORMALIZATIONINTENSITYDECORATOR_HPP_

#include "Core/Intensity.hpp"

namespace ComPWA {

namespace Tools {
class IntegrationStrategy;
}

namespace Physics {

class NormalizationIntensityDecorator : public ComPWA::Intensity {
public:
  NormalizationIntensityDecorator(
      const std::string &name, std::shared_ptr<ComPWA::Intensity> intensity,
      std::shared_ptr<ComPWA::Tools::IntegrationStrategy> integrator);

  double evaluate(const ComPWA::DataPoint &point) const final;

  void updateParametersFrom(const ParameterList &list) final;
  void addUniqueParametersTo(ParameterList &list) final;

  std::shared_ptr<FunctionTree>
  createFunctionTree(const ParameterList &DataSample,
                     const std::string &suffix) const final;

  std::shared_ptr<const ComPWA::Intensity> getUnnormalizedIntensity() const;

private:
  std::string Name;
  std::shared_ptr<ComPWA::Intensity> UnnormalizedIntensity;

  double Normalization;
  ParameterList PreviousParameterList;
  /// Phsp sample for numerical integration
  std::shared_ptr<ComPWA::Tools::IntegrationStrategy> Integrator;
};

} // namespace Physics
} // namespace ComPWA

#endif
