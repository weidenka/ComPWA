//-------------------------------------------------------------------------------
// Copyright (c) 2013 Mathias Michel.
//
// This file is part of ComPWA, check license.txt for details
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the GNU Public License v3.0
// which accompanies this distribution, and is available at
// http://www.gnu.org/licenses/gpl.html
//
// Contributors:
//     Mathias Michel - initial API and implementation
//-------------------------------------------------------------------------------
#include <memory>

#include "DataReader/Data.hpp"
#include "Estimator/Estimator.hpp"
#include "Physics/Amplitude.hpp"
#include "Optimizer/Optimizer.hpp"
#include "Core/ParameterList.hpp"
//#include "Core/DPKinematics.hpp"

#include "Core/Event.hpp"
#include "Core/Generator.hpp"
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/generator_iterator.hpp>

#include "Core/RunManager.hpp"

RunManager::RunManager(std::shared_ptr<Data> inD, std::shared_ptr<Amplitude> inP,
		std::shared_ptr<Optimizer> inO, std::shared_ptr<Efficiency> eff)
: eff_(eff), pData_(inD), pPhys_(inP), pOpti_(inO), success_(false),
  validSize(0),validAmplitude(0),validData(0),validOptimizer(0)
{
	std::cout<<"!2313"<<std::endl;
	if(eff && inD && inP && inO){
		validEfficiency=1;
		validAmplitude=1;
		validData=1;
		validOptimizer=1;
	}
}
RunManager::RunManager( unsigned int size, std::shared_ptr<Amplitude> inP,
		std::shared_ptr<Efficiency> eff, std::shared_ptr<Generator> gen)
: gen_(gen), eff_(eff), size_(size), pPhys_(inP), success_(false),
  validSize(0),validAmplitude(0),validData(0),validOptimizer(0)
{
	std::cout<<"!2313"<<std::endl;
	if(inP && eff){
		validEfficiency=1;
		validAmplitude=1;
	}
	validSize=1;
}

RunManager::~RunManager(){
	/* nothing */
}

bool RunManager::startFit(ParameterList& inPar){
	if( !(validEfficiency==1 && validAmplitude==1 && validData==1 && validOptimizer==1) )
		return false;

	pOpti_->exec(inPar);
	success_ = true;

	return success_;
}
bool RunManager::generate( unsigned int number ) {
	if( !(validEfficiency==1 && validAmplitude==1 && validSize==1) )
		return false;

	if(pData_->getNEvents()>0){
		//What do we do if dataset is not empty?
	}
	ParameterList minPar;
	pPhys_->fillStartParVec(minPar);

	//initialize random number generator
	boost::minstd_rand rndGen(0);//what seed should we use here?
	boost::uniform_real<> uni_dist(0,1);
	boost::variate_generator<boost::minstd_rand&, boost::uniform_real<> > uni(rndGen, uni_dist);

	//Determing an estimate on the maximum of the physics amplitude using 10k events.
	double genMaxVal=0;
	for(unsigned int i=0; i<10000;i++){
		Event tmp;
		gen_->generate(tmp);
		Particle part1 = tmp.getParticle(0);
		Particle part2 = tmp.getParticle(1);
		Particle part3 = tmp.getParticle(2);
		double m23sq = Particle::invariantMass(part2,part3);
		double m13sq = Particle::invariantMass(part1,part3);
		double m12sq = Particle::invariantMass(part1,part2);
		std::vector<double> x;
		x.push_back(m23sq);
		x.push_back(m13sq);
		ParameterList list = pPhys_->intensity(x,minPar);
		double AMPpdf = *list.GetDoubleParameter(0);
		if(AMPpdf > genMaxVal) genMaxVal= AMPpdf;
	}
	genMaxVal=1.5*genMaxVal;

	unsigned int i=0;
	double maxTest=0;
	int scale = (int) size_/10;
	std::cout<<"== Using "<<genMaxVal<< " as maximum value for random number generation!"<<std::endl;
	std::cout << "Generating MC: ["<<size_<<" events] "<<std::endl;
	while( i<size_){
		Event tmp;
		gen_->generate(tmp);
		Particle part1 = tmp.getParticle(0);
		Particle part2 = tmp.getParticle(1);
		Particle part3 = tmp.getParticle(2);
		double m23sq = Particle::invariantMass(part2,part3);
		double m13sq = Particle::invariantMass(part1,part3);
		double m12sq = Particle::invariantMass(part1,part2);
		std::vector<double> x;
		x.push_back(m23sq);
		x.push_back(m13sq);

		double ampRnd = uni()*genMaxVal;
		ParameterList list = pPhys_->intensity(x,minPar);
		double AMPpdf = *list.GetDoubleParameter(0);

		if(AMPpdf > maxTest ) maxTest = AMPpdf;
		if(ampRnd>AMPpdf ) continue;
		pData_->pushEvent(tmp);
		i++;

		//progress bar
		if( (i % scale) == 0) { std::cout<<(i/scale)*10<<"%..."<<std::flush; }

	}
	//	double t=3;
	std::cout<<"100%"<<std::endl;

	if( maxTest > (int) (0.9*genMaxVal) ) {
		std::cout<<"==========ATTENTION==========="<<std::endl;
		std::cout<<"== Max value of function close to maximum value of rnd. number generation!"<<std::endl;
		std::cout<<"== Choose higher max value!"<<std::endl;
		std::cout<<"==========ATTENTION==========="<<std::endl;
		return false;
	}

	return true;
};
