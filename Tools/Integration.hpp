//
//  Integration.hpp
//  COMPWA
//
//  Created by Peter Weidenkaff on 18.04.17.
//
//

#include <cmath>
#include <math.h>
#include <complex>

#ifndef Integration_h
#define Integration_h

namespace ComPWA {
  namespace Tools {
    
    struct testGauss {
      double mu = 0;
      double sigma = 1;
      double operator()(double x) const {
        // Normalized Gaussian
        double expon = (-0.5)*(mu - x) * (mu - x) / (sigma*sigma);
        double val = 1 / (sigma* std::sqrt(2 * M_PI)) * std::exp(expon);
        return val;
      }
    };
    
    template <typename T> class IntegralByQuadrature {
      
    public:
      IntegralByQuadrature(const T &func, std::pair<double, double> lim)
      : _func(func), _limits(lim), _depth(1) {}
      
      double Integral(int precision=100) {
        while( _depth<precision ){
//          std::cout<< "Current integral approximation: "<<_integral
//          <<" depth: "<<_depth<<std::endl;
          Next();
          _depth*=2;
        }
        return _integral;
      }
      
    protected:
      const T& _func;
      std::pair<double, double> _limits;
      int _depth;
      double _integral;
      
      double Next() {
        double range = (_limits.second-_limits.first);
        if(_depth == 1){
          _integral = 0.5*(range)*(_func(_limits.first)+_func(_limits.second));
        } else {
          double s=0;
          int n=0;
          double stepSize = 2*(_limits.second-_limits.first)/(_depth);
          double x = _limits.first+0.5*stepSize;
          while( x<_limits.second ){
            s += _func(x);
            x += stepSize;
            n++;
          }
        _integral = 0.5*(_integral+range*s/n);
        }
        return _integral;
      }
    };
    
  }
}
#endif /* Integration_h */
