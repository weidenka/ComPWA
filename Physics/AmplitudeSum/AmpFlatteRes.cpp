//-------------------------------------------------------------------------------
// Copyright (c) 2013 Mathias Michel.
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the GNU Public License v3.0
// which accompanies this distribution, and is available at
// http://www.gnu.org/licenses/gpl.html
//
// Contributors:
//     Mathias Michel - initial API and implementation
//		Peter Weidenkaff - adding correct g1s
//-------------------------------------------------------------------------------
//****************************************************************************
// Class for defining the relativistic Breit-Wigner resonance model, which
// includes the use of Blatt-Weisskopf barrier factors.
//****************************************************************************

// --CLASS DESCRIPTION [MODEL] --
// Class for defining the relativistic Breit-Wigner resonance model, which
// includes the use of Blatt-Weisskopf barrier factors.

#include <cmath>
#include <math.h>
#include "Physics/AmplitudeSum/AmpFlatteRes.hpp"


AmpFlatteRes::AmpFlatteRes(const char *name,
		std::shared_ptr<DoubleParameter> mag, std::shared_ptr<DoubleParameter> phase,
		std::shared_ptr<DoubleParameter> mass, int subSys, Spin spin, Spin m, Spin n,
		std::shared_ptr<DoubleParameter> mesonRadius,
		std::shared_ptr<DoubleParameter> motherRadius,
		std::shared_ptr<DoubleParameter> g1,
		std::shared_ptr<DoubleParameter> g2, double g2_partA, double g2_partB,
		int nCalls, normStyle nS) :
		AmpAbsDynamicalFunction(name, mag, phase, mass, subSys, spin, m, n,
				mesonRadius, motherRadius, nCalls, nS),
				_g2(g2), _g1(g1), _g2_partA(g2_partA), _g2_partB(g2_partB)
{
	if(_g2_partA<0||_g2_partA>5||_g2_partB<0||_g2_partB>5)
		throw std::runtime_error("AmpFlatteRes3Ch::evaluateAmp | particle masses for second channel not set!");

	//setting default normalization
	_norm = 1/sqrt(this->integral());
	modified=0;
}

AmpFlatteRes::~AmpFlatteRes() 
{
}

//std::complex<double> AmpFlatteRes::evaluate(dataPoint& point) {
//	return AmpAbsDynamicalFunction::evaluate(point);
//}

std::complex<double> AmpFlatteRes::evaluateAmp(dataPoint& point) {
	if( _g1->GetValue()!=tmp_g1 || _g2->GetValue()!=tmp_g2 ) {
		SetModified();
		tmp_g1 = _g1->GetValue();
		tmp_g2 = _g2->GetValue();
	}

	DalitzKinematics* kin = dynamic_cast<DalitzKinematics*>(Kinematics::instance());
	double mSq=-999;
	switch(_subSys){
	case 3:
		mSq=(kin->getThirdVariableSq(point.getVal(0),point.getVal(1)));	break;
	case 4: mSq=(point.getVal(1)); break;
	case 5: mSq=(point.getVal(0)); break;
	}

	return dynamicalFunction(mSq,_mass->GetValue(),
			_ma,_mb,_g1->GetValue(),
			_g2_partA,_g2_partB,_g2->GetValue(),_spin,_mesonRadius->GetValue());
}
std::complex<double> AmpFlatteRes::dynamicalFunction(double mSq, double mR,
		double massA1, double massA2, double gA,
		double massB1, double massB2, double couplingRatio,
		unsigned int J, double mesonRadius ){
	std::complex<double> i(0,1);
	double sqrtS = sqrt(mSq);

	//channel A - signal channel
	//break-up momentum
	//	std::complex<double> rhoA = Kinematics::phspFactor(sqrtS, massA1, massA2);
	double barrierA = Kinematics::FormFactor(sqrtS,massA1,massA2,J,mesonRadius)/Kinematics::FormFactor(mR,massA1,massA2,J,mesonRadius);
	barrierA=1;
	//std::complex<double> qTermA = std::pow((qValue(sqrtS,massA1,massA2) / qValue(mR,massA1,massA2)), (2.*J+ 1.));
	//	std::complex<double> qTermA = std::pow((phspFactor(sqrtS,massA1,massA2) / phspFactor(mR,massA1,massA2))*mR/sqrtS, (2*J+ 1));
	//convert coupling to partial width of channel A
	std::complex<double> gammaA = couplingToWidth(mSq,mR,gA,massA1,massA2,J,mesonRadius);
	//including the factor qTermA, as suggested by PDG, leads to an amplitude that doesn't converge.
	std::complex<double> termA = gammaA*barrierA*barrierA;

	//channel B - hidden channel
	//break-up momentum
	//	std::complex<double> rhoB = Kinematics::phspFactor(sqrtS, massB1, massB2);
	double barrierB = Kinematics::FormFactor(sqrtS,massB1,massB2,J,mesonRadius)/Kinematics::FormFactor(mR,massB1,massB2,J,mesonRadius);
	barrierB=1;
	//std::complex<double> qTermB = std::pow((qValue(sqrtS,massB1,massB2) / qValue(mR,massB1,massB2)), (2.*J+ 1.));
	//	std::complex<double> qTermB = std::pow((phspFactor(sqrtS,massB1,massB2) / phspFactor(mR,massB1,massB2))*mR/sqrtS, (2*J+ 1));
	double gB = couplingRatio; 
	//convert coupling to partial width of channel B
	std::complex<double> gammaB = couplingToWidth(mSq,mR,gB,massB1,massB2,J,mesonRadius);
	std::complex<double> termB = gammaB*barrierB*barrierB;

	//Coupling constant from production reaction. In case of a particle decay the production
	//coupling doesn't depend on energy since the CM energy is in the (RC) system fixed to the
	//mass of the decaying particle
	double g_production = 1;

	//-- old approach(BaBar)
	//std::complex<double> denom( mR*mR - mSq, (-1)*(rhoA*gA*gA + rhoB*gB*gB) );
	//std::complex<double> result = std::complex<double>(gA,0) / denom;
	//-- new approach - for spin 0 resonances in the imaginary part of the denominator the term qTerm
	//is added, compared to the old formula
	std::complex<double> denom = std::complex<double>( mR*mR - mSq,0) + (-1.0)*i*sqrtS*(termA + termB);
	std::complex<double> result = std::complex<double>(gA*g_production,0) / denom;

	if(result.real()!=result.real() || result.imag()!=result.imag()){
		std::cout<<"AmpFlatteRes::dynamicalFunction() | "<<barrierA<<" "<<mR<<" "<<mSq
				<<" "<<massA1<<" "<<massA2<<std::endl;
		return 0;
	}
	return result;
}


std::shared_ptr<FunctionTree> AmpFlatteRes::setupTree(
		allMasses& theMasses,allMasses& toyPhspSample,std::string suffix, ParameterList& params){

	DalitzKinematics* kin = dynamic_cast<DalitzKinematics*>(Kinematics::instance());
	double phspVol = kin->getPhspVolume();
	BOOST_LOG_TRIVIAL(info) << "AmpFlatteRes::setupBasicTree() | "<<_name;
	//------------Setup Tree---------------------
	std::shared_ptr<FunctionTree> newTree(new FunctionTree());
	//------------Setup Tree Pars---------------------
	std::shared_ptr<MultiDouble> m23sq( new MultiDouble("m23sq",theMasses.masses_sq.at( std::make_pair(2,3) )) );
	std::shared_ptr<MultiDouble> m13sq( new MultiDouble("m13sq",theMasses.masses_sq.at( std::make_pair(1,3) )) );
	std::shared_ptr<MultiDouble> m12sq( new MultiDouble("m12sq",theMasses.masses_sq.at( std::make_pair(1,2) )) );
	std::shared_ptr<MultiDouble> m23sq_phsp( new MultiDouble("m23sq_phsp",toyPhspSample.masses_sq.at( std::make_pair(2,3) )) );
	std::shared_ptr<MultiDouble> m13sq_phsp( new MultiDouble("m13sq_phsp",toyPhspSample.masses_sq.at( std::make_pair(1,3) )) );
	std::shared_ptr<MultiDouble> m12sq_phsp( new MultiDouble("m12sq_phsp",toyPhspSample.masses_sq.at( std::make_pair(1,2) )) );

	//----Strategies needed
	std::shared_ptr<MultAll> mmultStrat(new MultAll(ParType::MCOMPLEX));
	std::shared_ptr<MultAll> mmultDStrat(new MultAll(ParType::MDOUBLE));
	std::shared_ptr<AddAll> maddStrat(new AddAll(ParType::MCOMPLEX));
	std::shared_ptr<AbsSquare> msqStrat(new AbsSquare(ParType::MDOUBLE));
	std::shared_ptr<LogOf> mlogStrat(new LogOf(ParType::MDOUBLE));
	std::shared_ptr<MultAll> multStrat(new MultAll(ParType::COMPLEX));
	std::shared_ptr<MultAll> multDStrat(new MultAll(ParType::DOUBLE));
	std::shared_ptr<AddAll> addStrat(new AddAll(ParType::DOUBLE));
	std::shared_ptr<AddAll> addComplexStrat(new AddAll(ParType::COMPLEX));
	std::shared_ptr<AbsSquare> sqStrat(new AbsSquare(ParType::DOUBLE));
	std::shared_ptr<LogOf> logStrat(new LogOf(ParType::DOUBLE));
	std::shared_ptr<Complexify> complStrat(new Complexify(ParType::COMPLEX));
	std::shared_ptr<Inverse> invStrat(new Inverse(ParType::DOUBLE));
	std::shared_ptr<SquareRoot> sqRootStrat(new SquareRoot(ParType::DOUBLE));

	//----Add Nodes
	std::shared_ptr<WignerDStrategy> angdStrat(	new WignerDStrategy(_name,ParType::MDOUBLE) );
	std::shared_ptr<WignerDPhspStrategy> angdPhspStrat(	new WignerDPhspStrategy(_name,ParType::MDOUBLE) );
	std::shared_ptr<FlatteStrategy> flatteStrat(new FlatteStrategy(_name,ParType::MCOMPLEX));
	std::shared_ptr<FlattePhspStrategy> flattePhspStrat(new FlattePhspStrategy(_name,ParType::MCOMPLEX));

	newTree->createHead("Reso_"+_name, mmultStrat, theMasses.nEvents); //Reso=BW*C_*AD*N_

	newTree->createNode("FlatteRes_"+_name, flatteStrat, "Reso_"+_name, theMasses.nEvents); //BW
	newTree->createNode("C_"+_name, complStrat, "Reso_"+_name); //c
	newTree->createLeaf("Intens_"+_name, params.GetDoubleParameter("mag_"+_name), "C_"+_name); //r
	newTree->createLeaf("Phase_"+_name, params.GetDoubleParameter("phase_"+_name), "C_"+_name); //phi
	newTree->createNode("AngD_"+_name, angdStrat, "Reso_"+_name, theMasses.nEvents); //AD

	//Flatte
	newTree->createLeaf("m0_"+_name, params.GetDoubleParameter("m0_"+_name), "FlatteRes_"+_name); //m0
	newTree->createLeaf("m23sq", m23sq, "FlatteRes_"+_name); //ma
	newTree->createLeaf("m13sq", m13sq, "FlatteRes_"+_name); //mb
	newTree->createLeaf("m12sq", m12sq, "FlatteRes_"+_name); //mc
	newTree->createLeaf("subSysFlag_"+_name, _subSys, "FlatteRes_"+_name); //_subSysFlag
	newTree->createLeaf("spin_"+_name, _spin, "FlatteRes_"+_name); //spin
	newTree->createLeaf("d_"+_name, params.GetDoubleParameter("d_"+_name) , "FlatteRes_"+_name); //d
	newTree->createLeaf("mHiddenA_"+_name, _g2_partA, "FlatteRes_"+_name);
	newTree->createLeaf("mHiddenB_"+_name, _g2_partB, "FlatteRes_"+_name);
	if(_name.find("a_0") != _name.npos) {
		try {
			newTree->createLeaf("g1_a_0", params.GetDoubleParameter("g1_a_0"), "FlatteRes_"+_name);//use global parameter g1_a0 (asdfef)
		} catch (BadParameter& e) {
			newTree->createLeaf("g1_"+_name, params.GetDoubleParameter("g1_"+_name), "FlatteRes_"+_name);//use local parameter g1_a0
		}
		try {
			newTree->createLeaf("g2_a_0", params.GetDoubleParameter("g2_a_0"), "FlatteRes_"+_name);//use global parameter g1_a0 (asdfef)
		} catch (BadParameter& e) {
			newTree->createLeaf("g2_"+_name, params.GetDoubleParameter("g2_"+_name), "FlatteRes_"+_name);//use local parameter g1_a0
		}
	} else {
		newTree->createLeaf("g1_"+_name, params.GetDoubleParameter("g1_"+_name), "FlatteRes_"+_name);//use local parameter g1_a0
		newTree->createLeaf("g2_"+_name, params.GetDoubleParameter("g2_"+_name), "FlatteRes_"+_name);
	}
	//Angular distribution
	newTree->createLeaf("m23sq", m23sq, "AngD_"+_name); //ma
	newTree->createLeaf("m13sq", m13sq, "AngD_"+_name); //mb
	newTree->createLeaf("m12sq", m12sq, "AngD_"+_name); //mc
	newTree->createLeaf("M", kin->M, "AngD_"+_name); //M
	newTree->createLeaf("m1", kin->m1, "AngD_"+_name); //m1
	newTree->createLeaf("m2", kin->m2, "AngD_"+_name); //m2
	newTree->createLeaf("m3", kin->m3, "AngD_"+_name); //m3
	newTree->createLeaf("subSysFlag_"+_name, _subSys, "AngD_"+_name); //_subSysFlag
	newTree->createLeaf("spin_"+_name,_spin, "AngD_"+_name); //spin
	newTree->createLeaf("m_"+_name, 0, "AngD_"+_name); //OutSpin 1
	newTree->createLeaf("n_"+_name, 0, "AngD_"+_name); //OutSpin 2

	//Normalization
	if(_normStyle!=normStyle::none){
		newTree->createNode("N_"+_name, sqRootStrat, "Reso_"+_name); //N = sqrt(NSq)
		newTree->createNode("NSq_"+_name, multDStrat, "N_"+_name); //NSq = N_phspMC * 1/PhspVolume * 1/Sum(|A|^2)
		newTree->createLeaf("PhspSize_"+_name, toyPhspSample.nEvents, "NSq_"+_name); // N_phspMC
		newTree->createLeaf("PhspVolume_"+_name, 1/phspVol, "NSq_"+_name); // 1/PhspVolume
		newTree->createNode("InvSum_"+_name, invStrat, "NSq_"+_name); //1/Sum(|A|^2)
		newTree->createNode("Sum_"+_name, addStrat, "InvSum_"+_name); //Sum(|A|^2)
		newTree->createNode("AbsVal_"+_name, msqStrat, "Sum_"+_name); //|A_i|^2
		newTree->createNode("NormReso_"+_name, mmultStrat, "AbsVal_"+_name, toyPhspSample.nEvents); //BW

		//Flatte (Normalization)
		newTree->createNode("NormFlatte_"+_name, flattePhspStrat, "NormReso_"+_name, toyPhspSample.nEvents); //BW
		newTree->createLeaf("m0_"+_name, params.GetDoubleParameter("m0_"+_name), "NormFlatte_"+_name); //m0
		newTree->createLeaf("m23sq_phsp", m23sq_phsp, "NormFlatte_"+_name); //ma
		newTree->createLeaf("m13sq_phsp", m13sq_phsp, "NormFlatte_"+_name); //mb
		newTree->createLeaf("m12sq_phsp", m12sq_phsp, "NormFlatte_"+_name); //mc
		newTree->createLeaf("subSysFlag_"+_name, _subSys, "NormFlatte_"+_name); //_subSysFlag
		newTree->createLeaf("spin_"+_name, _spin, "NormFlatte_"+_name); //spin
		newTree->createLeaf("d_"+_name,  params.GetDoubleParameter("d_"+_name), "NormFlatte_"+_name); //d
		newTree->createLeaf("mHiddenA_"+_name, _g2_partA, "NormFlatte_"+_name);
		newTree->createLeaf("mHiddenB_"+_name, _g2_partB, "NormFlatte_"+_name);
		if(_name.find("a_0(980)") != _name.npos){
			try {
				newTree->createLeaf("g1_a_0", params.GetDoubleParameter("g1_a_0"), "NormFlatte_"+_name);//use global parameter g1_a0 (asdfef)
			} catch (BadParameter& e) {
				newTree->createLeaf("g1_"+_name, params.GetDoubleParameter("g1_"+_name), "NormFlatte_"+_name);//use local parameter g1_a0
			}
			try {
				newTree->createLeaf("g2_a_0", params.GetDoubleParameter("g2_a_0"), "NormFlatte_"+_name);//use global parameter g1_a0 (asdfef)
			} catch (BadParameter& e) {
				newTree->createLeaf("g2_"+_name, params.GetDoubleParameter("g2_"+_name), "NormFlatte_"+_name);//use local parameter g1_a0
			}
		} else {
			newTree->createLeaf("g1_"+_name, params.GetDoubleParameter("g1_"+_name), "NormFlatte_"+_name);//use local parameter g1_a0
			newTree->createLeaf("g2_"+_name, params.GetDoubleParameter("g2_"+_name), "NormFlatte_"+_name);
		}

		//Angular distribution (Normalization)
		newTree->createNode("NormAngD_"+_name, angdPhspStrat, "NormReso_"+_name, toyPhspSample.nEvents); //AD
		newTree->createLeaf("m23sq_phsp", m23sq_phsp, "NormAngD_"+_name); //ma
		newTree->createLeaf("m13sq_phsp", m13sq_phsp, "NormAngD_"+_name); //mb
		newTree->createLeaf("m12sq_phsp", m12sq_phsp, "NormAngD_"+_name); //mc
		newTree->createLeaf("M", kin->M, "NormAngD_"+_name); //M
		newTree->createLeaf("m1", kin->m1, "NormAngD_"+_name); //m1
		newTree->createLeaf("m2", kin->m2, "NormAngD_"+_name); //m2
		newTree->createLeaf("m3", kin->m3, "NormAngD_"+_name); //m3
		newTree->createLeaf("subSysFlag_"+_name, _subSys, "NormAngD_"+_name); //subSysFlag
		newTree->createLeaf("spin_"+_name,_spin, "NormAngD_"+_name); //spin
		newTree->createLeaf("m_"+_name, 0, "NormAngD_"+_name); //OutSpin 1
		newTree->createLeaf("n_"+_name, 0, "NormAngD_"+_name); //OutSpin 2
	} else {
		newTree->createLeaf("N_"+_name, 1., "Reso_"+_name);
	}

	switch(_subSys){
	case 3:{ //reso in sys of particles 1&2
		//newTree->createLeaf("mym_"+_name, m12, "RelBW_"+_name); //m
		newTree->createLeaf("ma_"+_name, kin->m1, "FlatteRes_"+_name); //ma
		newTree->createLeaf("mb_"+_name, kin->m2, "FlatteRes_"+_name); //mb
		if(_normStyle!=normStyle::none){
			newTree->createLeaf("ma_"+_name, kin->m1, "NormFlatte_"+_name); //ma
			newTree->createLeaf("mb_"+_name, kin->m2, "NormFlatte_"+_name); //mb
		}
		break;
	}
	case 4:{ //reso in sys of particles 1&3
		//newTree->createLeaf("mym_"+_name, m13, "FlatteRes_"+_name); //m
		newTree->createLeaf("ma_"+_name, kin->m1, "FlatteRes_"+_name); //ma
		newTree->createLeaf("mb_"+_name, kin->m3, "FlatteRes_"+_name); //mb
		if(_normStyle!=normStyle::none){
			newTree->createLeaf("ma_"+_name, kin->m1, "NormFlatte_"+_name); //ma
			newTree->createLeaf("mb_"+_name, kin->m3, "NormFlatte_"+_name); //mb
		}
		break;
	}
	case 5:{ //reso in sys of particles 2&3
		//newTree->createLeaf("mym_"+_name, m23, "FlatteRes_"+_name); //m
		newTree->createLeaf("ma_"+_name, kin->m2, "FlatteRes_"+_name); //ma
		newTree->createLeaf("mb_"+_name, kin->m3, "FlatteRes_"+_name); //mb
		if(_normStyle!=normStyle::none){
			newTree->createLeaf("ma_"+_name, kin->m2, "NormFlatte_"+_name); //ma
			newTree->createLeaf("mb_"+_name, kin->m3, "NormFlatte_"+_name); //mb
		}
		break;
	}
	default:{
		BOOST_LOG_TRIVIAL(error)<<"AmpSumIntensity::setupBasicTree(): Subsys not found!!";
	}
	}


	return newTree;
}



FlatteConf::FlatteConf(const boost::property_tree::ptree &pt_) : basicConf(pt_){
	m_mass= pt_.get<double>("mass");
	m_mass_fix= pt_.get<bool>("mass_fix");
	m_mass_min= pt_.get<double>("mass_min");
	m_mass_max= pt_.get<double>("mass_max");
	m_mesonRadius= pt_.get<double>("mesonRadius");
	m_spin= pt_.get<unsigned int>("spin");
	m_m= pt_.get<unsigned int>("m");
	m_n= pt_.get<unsigned int>("n");
	m_daughterA= pt_.get<unsigned int>("daughterA");
	m_daughterB= pt_.get<unsigned int>("daughterB");
	m_g1= pt_.get<double>("g1");
	m_g1_fix= pt_.get<bool>("g1_fix");
	m_g1_min= pt_.get<double>("g1_min");
	m_g1_max= pt_.get<double>("g1_max");
	m_g2= pt_.get<double>("g2");
	m_g2_fix= pt_.get<bool>("g2_fix");
	m_g2_min= pt_.get<double>("g2_min");
	m_g2_max= pt_.get<double>("g2_max");
	m_g2_part1= pt_.get<std::string>("g2_part1");
	m_g2_part2= pt_.get<std::string>("g2_part2");

	return;
}

void FlatteConf::put(boost::property_tree::ptree &pt_){
	basicConf::put(pt_);
	pt_.put("mass", m_mass);
	pt_.put("mass_fix", m_mass_fix);
	pt_.put("mass_min", m_mass_min);
	pt_.put("mass_max", m_mass_max);
	pt_.put("mesonRadius", m_mesonRadius);
	pt_.put("spin", m_spin);
	pt_.put("m", m_m);
	pt_.put("n", m_n);
	pt_.put("daughterA", m_daughterA);
	pt_.put("daughterB", m_daughterB);
	pt_.put("g1", m_g1);
	pt_.put("g1_fix", m_g1_fix);
	pt_.put("g1_min", m_g1_min);
	pt_.put("g1_max", m_g1_max);
	pt_.put("g2", m_g2);
	pt_.put("g2_fix", m_g2_fix);
	pt_.put("g2_min", m_g2_min);
	pt_.put("g2_max", m_g2_max);
	pt_.put("g2_part1", m_g2_part1);
	pt_.put("g2_part2", m_g2_part2);

	return;
}

void FlatteConf::update(ParameterList par){
	basicConf::update(par);
	try{// only update parameters if they are found in list
		m_mass= par.GetDoubleParameter("m0_"+m_name)->GetValue();
		m_mass_fix = par.GetDoubleParameter("m0_"+m_name)->IsFixed();
		m_mass_min = par.GetDoubleParameter("m0_"+m_name)->GetMinValue();
		m_mass_max = par.GetDoubleParameter("m0_"+m_name)->GetMaxValue();
	} catch (BadParameter b) { }

	std::shared_ptr<DoubleParameter> p;
	std::shared_ptr<DoubleParameter> p2;
	if(m_name.find("a_0(980)") == 0) {
		try{
			p = par.GetDoubleParameter("g1_a_0");
		} catch(BadParameter& e){
			p= par.GetDoubleParameter("g1_"+m_name);
		}
		try{// only update parameters if they are found in list
			p2 = par.GetDoubleParameter("g2_a_0");
		} catch (BadParameter b) {
			p2 = par.GetDoubleParameter("g2_"+m_name);
		}
	} else {
		p= par.GetDoubleParameter("g1_"+m_name);
		p2 = par.GetDoubleParameter("g2_"+m_name);
	}
	m_g1 = p->GetValue();
	m_g1_fix = p->IsFixed();
	m_g1_min = p->GetMinValue();
	m_g1_max = p->GetMaxValue();
	m_g2 = p2->GetValue();
	m_g2_fix = p2->IsFixed();
	m_g2_min = p2->GetMinValue();
	m_g2_max = p2->GetMaxValue();

}

bool FlatteStrategy::execute(ParameterList& paras, std::shared_ptr<AbsParameter>& out) {
	if( checkType != out->type() ) {
		throw(WrongParType(std::string("Output Type ")+ParNames[out->type()]+std::string(" conflicts expected type ")+ParNames[checkType]+std::string(" of ")+name+" BW strat"));
		return false;
	}

	double m0, d, ma, mb, g1, g2, mHiddenA, mHiddenB;
	unsigned int spin, subSys;
	//Get parameters from ParameterList -
	//enclosing in try...catch for the case that names of nodes have changed
	try{
		m0 = double(paras.GetParameterValue("m0_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter m0_"+name;
		throw;
	}
	try{
		spin = (unsigned int)(paras.GetParameterValue("ParOfNode_spin_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter ParOfNode_spin_"+name;
		throw;
	}
	try{
		d = double(paras.GetParameterValue("d_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter d_"+name;
		throw;
	}
	//		norm = double(paras.GetParameterValue("ParOfNode_norm_"+name));
	try{
		subSys = double(paras.GetParameterValue("ParOfNode_subSysFlag_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter ParOfNode_subSysFlag_"+name;
		throw;
	}

	try{
		ma = double(paras.GetParameterValue("ParOfNode_ma_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter ParOfNode_ma_"+name;
		throw;
	}

	try{
		mb = double(paras.GetParameterValue("ParOfNode_mb_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter ParOfNode_mb_"+name;
		throw;
	}
	try{
		mHiddenA = double(paras.GetParameterValue("ParOfNode_mHiddenA_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter ParOfNode_mHiddenA_"+name;
		throw;
	}
	try{
		mHiddenB = double(paras.GetParameterValue("ParOfNode_mHiddenB_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter ParOfNode_mHiddenB_"+name;
		throw;
	}
	try{
		g1 = double(paras.GetParameterValue("g1_a_0"));//special case for peter's channel
	}catch(BadParameter& e){
		try{
			g1 = double(paras.GetParameterValue("g1_"+name));
		}catch(BadParameter& e){
			BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter g1_a_0";
			BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter g1_"+name;
			throw;
		}
	}
	try{
		g2 = double(paras.GetParameterValue("g2_a_0"));//special case for peter's channel
	}catch(BadParameter& e){
		try{
			g2 = double(paras.GetParameterValue("g2_"+name));
		}catch(BadParameter& e){
			BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter g2_a_0";
			BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter g2_"+name;
			throw;
		}
	}

	//MultiDim output, must have multidim Paras in input
	if(checkType == ParType::MCOMPLEX){
		if(paras.GetNMultiDouble()){
			unsigned int nElements = paras.GetMultiDouble(0)->GetNValues();

			std::vector<std::complex<double> > results(nElements, std::complex<double>(0.));
			std::shared_ptr<MultiDouble> mp;//=paras.GetMultiDouble("mym_"+name);
			switch(subSys){
			case 3:{ mp  = (paras.GetMultiDouble("m12sq")); break; }
			case 4:{ mp  = (paras.GetMultiDouble("m13sq")); break; }
			case 5:{ mp  = (paras.GetMultiDouble("m23sq")); break; }
			}

			//calc BW for each point
			for(unsigned int ele=0; ele<nElements; ele++){
				double mSq = (mp->GetValue(ele));
				results[ele] = AmpFlatteRes::dynamicalFunction(
						mSq,m0,ma,mb,g1,mHiddenA,mHiddenB,g2,spin,d);
			}
			out = std::shared_ptr<AbsParameter>(new MultiComplex(out->GetName(),results));
			return true;
		}else{ //end multidim para treatment
			throw(WrongParType("Input MultiDoubles missing in BW strat of "+name));
			return false;
		}
	}//end multicomplex output

	double mSq;// = sqrt(paras.GetParameterValue("mym_"+name));
	switch(subSys){
	case 3:{ mSq  = (double(paras.GetParameterValue("m12sq"))); break; }
	case 4:{ mSq  = (double(paras.GetParameterValue("m13sq"))); break; }
	case 5:{ mSq  = (double(paras.GetParameterValue("m23sq"))); break; }
	}

	std::complex<double> result = AmpFlatteRes::dynamicalFunction(
			mSq,m0,ma,mb,g1,mHiddenA,mHiddenB,g2,spin,d);
	out = std::shared_ptr<AbsParameter>(new ComplexParameter(out->GetName(), result));
	return true;
}

bool FlattePhspStrategy::execute(ParameterList& paras, std::shared_ptr<AbsParameter>& out) {
	if( checkType != out->type() ) {
		throw(WrongParType(std::string("Output Type ")+ParNames[out->type()]+std::string(" conflicts expected type ")+ParNames[checkType]+std::string(" of ")+name+" BW strat"));
		return false;
	}

	double m0, d, ma, mb, g1, g2, mHiddenA, mHiddenB;
	unsigned int spin, subSys;

	//Get parameters from ParameterList -
	//enclosing in try...catch for the case that names of nodes have changed
	try{
		m0 = double(paras.GetParameterValue("m0_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlattePhspStrategy: can't find parameter m0_"+name;
		throw;
	}
	try{
		spin = (unsigned int)(paras.GetParameterValue("ParOfNode_spin_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlattePhspStrategy: can't find parameter ParOfNode_spin_"+name;
		throw;
	}
	try{
		d = double(paras.GetParameterValue("d_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlattePhspStrategy: can't find parameter d_"+name;
		throw;
	}
	try{
		subSys = double(paras.GetParameterValue("ParOfNode_subSysFlag_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlattePhspStrategy: can't find parameter ParOfNode_subSysFlag_"+name;
		throw;
	}
	try{
		ma = double(paras.GetParameterValue("ParOfNode_ma_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlattePhspStrategy: can't find parameter ParOfNode_ma_"+name;
		throw;
	}
	try{
		mb = double(paras.GetParameterValue("ParOfNode_mb_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlattePhspStrategy: can't find parameter ParOfNode_mb_"+name;
		throw;
	}
	try{
		mHiddenA = double(paras.GetParameterValue("ParOfNode_mHiddenA_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlattePhspStrategy: can't find parameter ParOfNode_mHiddenA_"+name;
		throw;
	}
	try{
		mHiddenB = double(paras.GetParameterValue("ParOfNode_mHiddenB_"+name));
	}catch(BadParameter& e){
		BOOST_LOG_TRIVIAL(error) <<"FlattePhspStrategy: can't find parameter ParOfNode_mHiddenB_"+name;
		throw;
	}
	try{
		g1 = double(paras.GetParameterValue("g1_a_0"));
	}catch(BadParameter& e){
		try{
			g1 = double(paras.GetParameterValue("g1_"+name));
		}catch(BadParameter& e){
			BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter g1_a_0";
			BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter g1_"+name;
			throw;
		}
	}
	try{
		g2 = double(paras.GetParameterValue("g2_a_0"));
	}catch(BadParameter& e){
		try{
			g2 = double(paras.GetParameterValue("g2_"+name));
		}catch(BadParameter& e){
			BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter g1_a_0";
			BOOST_LOG_TRIVIAL(error) <<"FlatteStrategy: can't find parameter g1_"+name;
			throw;
		}
	}


	//MultiDim output, must have multidim Paras in input
	if(checkType == ParType::MCOMPLEX){
		if(paras.GetNMultiDouble()){
			unsigned int nElements = paras.GetMultiDouble(0)->GetNValues();

			std::vector<std::complex<double> > results(nElements, std::complex<double>(0.));
			std::shared_ptr<MultiDouble> mp;//=paras.GetMultiDouble("mym_"+name);
			switch(subSys){
			case 3:{ mp  = (paras.GetMultiDouble("m12sq_phsp")); break; }
			case 4:{ mp  = (paras.GetMultiDouble("m13sq_phsp")); break; }
			case 5:{ mp  = (paras.GetMultiDouble("m23sq_phsp")); break; }
			}

			//calc BW for each point
			for(unsigned int ele=0; ele<nElements; ele++){
				double mSq = (mp->GetValue(ele));
				results[ele] = AmpFlatteRes::dynamicalFunction(
						mSq,m0,ma,mb,g1,mHiddenA,mHiddenB,g2,spin,d);
				//					if(ele<10) std::cout<<"Strategy BWrel "<<results[ele]<<std::endl;
			}
			out = std::shared_ptr<AbsParameter>(new MultiComplex(out->GetName(),results));
			return true;
		}else{ //end multidim para treatment
			throw(WrongParType("Input MultiDoubles missing in BW strat of "+name));
			return false;
		}
	}//end multicomplex output


	double mSq;// = sqrt(paras.GetParameterValue("mym_"+name));
	switch(subSys){
	case 3:{ mSq  = (double(paras.GetParameterValue("m12sq_phsp"))); break; }
	case 4:{ mSq  = (double(paras.GetParameterValue("m13sq_phsp"))); break; }
	case 5:{ mSq  = (double(paras.GetParameterValue("m23sq_phsp"))); break; }
	}

	std::complex<double> result = AmpFlatteRes::dynamicalFunction(
			mSq,m0,ma,mb,g1,mHiddenA,mHiddenB,g2,spin,d);
	out = std::shared_ptr<AbsParameter>(new ComplexParameter(out->GetName(), result));
	return true;
}
