// Copyright (c) 2013 The ComPWA Team.
// This file is part of the ComPWA framework, check
// https://github.com/ComPWA/ComPWA/license.txt for details.

#include <string>
#include <complex>
#include <memory>

#include "Core/TreeNode.hpp"
#include "Core/Functions.hpp"

using namespace ComPWA;

TreeNode::TreeNode(std::string name,
                   std::shared_ptr<ComPWA::Parameter> parameter,
                   std::shared_ptr<Strategy> strategy,
                   std::shared_ptr<TreeNode> parent)
    : _name(name), _changed(true), _strat(strategy) {
  _parameters.push_back(parameter);
  if (parent) {
    _parents.push_back(parent);
  }
}

TreeNode::TreeNode(std::string name,
                   std::vector<std::shared_ptr<ComPWA::Parameter>> &parameters,
                   std::shared_ptr<Strategy> strategy,
                   std::shared_ptr<TreeNode> parent)
    : _name(name), _changed(true), _strat(strategy) {
  for (unsigned int i = 0; i < parameters.size(); i++) {
    _parameters.push_back(parameters.at(i));
  }
  if (parent) {
    _parents.push_back(parent);
  }
}

TreeNode::~TreeNode() {}

void TreeNode::update() {
  for (unsigned int i = 0; i < _parents.size(); i++)
    _parents.at(i)->update();
  _changed = true;
};

void TreeNode::recalculate() {

  if (_children.size() < 1) {
    _changed = false;
    return;
  }

  if (_parameters.size() == 1) { // Single dimension
    ParameterList newVals;
    for (auto ch : _children) {
      ch->recalculate();
      for (auto p : ch->parameters()) {
        newVals.addParameter(p);
      }
    }

    try {
      _strat->execute(newVals, _parameters.at(0));
    } catch (std::exception &ex) {
      LOG(error) << "TreeNode::Recalculate() | Strategy " << _strat
                 << " failed on node " << name() << ": " << ex.what();
      throw;
    }
  } else { // Multi dimensions
    for (unsigned int ele = 0; ele < _parameters.size(); ele++) {
      ParameterList newVals;

      for (auto ch : _children) {
        ch->recalculate();
        if (ch->dimension() == 1)
          newVals.addParameter(ch->parameter(0));
        else if (ch->dimension() != _parameters.size())
          newVals.addParameter(ch->parameter(ele));
        else
          throw std::runtime_error("TreeNode::Recalculate() | Dimension of "
                                   "child nodes does not match");
      }

      try {
        _strat->execute(newVals, _parameters.at(ele));
      } catch (std::exception &ex) {
        LOG(error) << "TreeNode::recalculate() | Strategy " << _strat
                   << " failed on node " << name() << ": " << ex.what();
        throw;
      }
    }
  }
  _changed = false;
}

std::shared_ptr<Parameter> TreeNode::parameter(unsigned int position) {
  return _parameters.at(position);
}

std::vector<std::shared_ptr<Parameter>> &TreeNode::parameters() {
  return _parameters;
}

void TreeNode::fillParameters(ComPWA::ParameterList &list) {
  for (auto ch : _children) {
    ch->fillParameters(list);
  }
  for (auto i : _parameters) {
    if (i->type() == ComPWA::ParType::DOUBLE)
      list.addParameter(i);
  }
}

std::shared_ptr<TreeNode> TreeNode::findChildNode(std::string name) const {
  std::shared_ptr<TreeNode> node;
  if (!_children.size())
    node = std::shared_ptr<TreeNode>();
  for (unsigned int i = 0; i < _children.size(); i++) {
    if (_children.at(i)->name() == name) {
      return _children.at(i);
    } else
      node = _children.at(i)->findChildNode(name);
    if (node)
      return node;
  }
  return node;
}

// std::shared_ptr<Parameter> TreeNode::ChildValue(std::string name) const {
//  std::shared_ptr<TreeNode> node = FindChildNode(name);
//  if (node)
//    return node->Parameter();
//
//  return std::shared_ptr<Parameter>();
//}
//
// std::complex<double> TreeNode::getChildSingleValue(std::string name) const {
//  std::shared_ptr<TreeNode> node = std::shared_ptr<TreeNode>();
//  node = FindChildNode(name);
//  if (node) {
//    std::shared_ptr<Parameter> val = node->Parameter();
//    if (val->type() == ParType::DOUBLE)
//      return std::complex<double>(
//          (std::dynamic_pointer_cast<DoubleParameter>(val))->value(), 0);
//    if (val->type() == ParType::COMPLEX)
//      return std::complex<double>(
//          (std::dynamic_pointer_cast<ComplexParameter>(val))->value());
//    if (val->type() == ParType::INTEGER)
//      return std::complex<double>(
//          (std::dynamic_pointer_cast<IntegerParameter>(val))->value(), 0);
//    if (val->type() == ParType::BOOL)
//      return std::complex<double>(
//          (std::dynamic_pointer_cast<BoolParameter>(val))->value(), 0);
//    if (val->type() == ParType::MDOUBLE)
//      return std::complex<double>(
//          (std::dynamic_pointer_cast<MultiDouble>(val))->GetValue(0), 0);
//    if (val->type() == ParType::MCOMPLEX)
//      return std::complex<double>(
//          (std::dynamic_pointer_cast<MultiComplex>(val))->GetValue(0));
//  }
//  return std::complex<double>(-999, 0);
//}

std::string TreeNode::print(int level, std::string prefix) const {
  std::stringstream oss;
  if (_changed && _children.size()) {
    oss << prefix << _name << " = ?";
  } else {
    oss << prefix << _name;
    auto it = _parameters.begin();
    for (; it != _parameters.end(); ++it) {
      if (!_children.size()) // print parameter name for leafs
        oss << " [" << (*it)->name() << "]";
      oss << " = " << (*it)->val_to_str();
      if (it != _parameters.end())
        oss << ", ";
    }
  }

  if (_children.size())
    oss << " (" << _children.size() << " children/" << _parameters.size()
        << " values)" << std::endl;
  else
    oss << std::endl;

  if (level == 0)
    return oss.str();
  for (unsigned int i = 0; i < _children.size(); i++) {
    oss << _children.at(i)->print(level - 1, prefix + ". ");
  }
  return oss.str();
}

void TreeNode::addChild(std::shared_ptr<TreeNode> childNode) {
  _children.push_back(childNode);
}

void TreeNode::addParent(std::shared_ptr<TreeNode> parentNode) {
  _parents.push_back(parentNode);
  parentNode->_children.push_back(shared_from_this());
}

void TreeNode::fillParentNames(std::vector<std::string> &names) const {
  for (auto i : _parents) {
    names.push_back(i->name());
  }
}

void TreeNode::linkParents() {
  for (unsigned int i = 0; i < _parents.size(); i++)
    _parents.at(i)->_children.push_back(shared_from_this());
}

void TreeNode::deleteLinks() {
  _children.clear();
  _parents.clear();
  for (unsigned int i = 0; i < _parameters.size(); i++) {
    _parameters.at(i)->Detach(shared_from_this());
  }
}

std::vector<std::shared_ptr<TreeNode>> &TreeNode::childNodes() {
  return _children;
}

void TreeNode::fillChildNames(std::vector<std::string> &names) const {
  for (unsigned int i = 0; i < _children.size(); i++)
    names.push_back(_children.at(i)->name());
}
