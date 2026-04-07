// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2001 David Megginson <david@megginson.com>

/**
 * @file
 * @brief Interface definition for encapsulated commands
 */

#include <simgear_config.h>

#include <simgear/compiler.h>
#include "SGBinding.hxx"

#include "simgear/debug/debug_types.h"
#include "simgear/props/props.hxx"

#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

static std::map<std::string, SGAbstractBinding::BindingFactory> sg_bindingFactories;

SGAbstractBinding::SGAbstractBinding()
    : _arg(new SGPropertyNode)
{
}

void SGAbstractBinding::clear()
{
    _arg.clear();
}

void SGAbstractBinding::fire() const
{
    if (test()) {
        innerFire();
    }
}

void SGAbstractBinding::fire(SGPropertyNode* params) const
{
    if (test()) {
        if (params != nullptr) {
            copyProperties(params, _arg);
        }

        innerFire();
    }
}

void SGAbstractBinding::fire(double offset, double max) const
{
    if (test()) {
        _arg->setDoubleValue("offset", offset / max);
        innerFire();
    }
}

void SGAbstractBinding::fire(double setting) const
{
    if (test()) {
        // A value is automatically added to
        // the args
        if (!_setting) { // save the setting node for efficiency
            _setting = _arg->getChild("setting", 0, true);
        }
        _setting->setDoubleValue(setting);
        innerFire();
    }
}

void SGAbstractBinding::read(const SGPropertyNode* config, SGPropertyNode* root)
{
    const SGPropertyNode* conditionNode = config->getChild("condition");
    _debug = config->getBoolValue("debug", false);
    _arg = const_cast<SGPropertyNode*>(config);

    if (conditionNode)
        setCondition(sgReadCondition(root, conditionNode));
}

///////////////////////////////////////////////////////////////////////////////

SGSharedPtr<SGAbstractBinding> SGAbstractBinding::createFromProps(SGPropertyNode_ptr config, SGPropertyNode* root)
{
    const auto cmdName = config->getStringValue("command");
    SGAbstractBinding_ptr binding;

    // Check registered factories first
    auto it = sg_bindingFactories.find(cmdName);
    if (it != sg_bindingFactories.end()) {
        binding = it->second(config, SGPropertyNode_ptr{root});
    }

    if (!binding) {
        binding = new SGBinding{cmdName};
    }

    binding->read(config, root);
    return binding;
}

void SGAbstractBinding::registerFactory(const std::string& commandName, BindingFactory factory)
{
    sg_bindingFactories[commandName] = std::move(factory);
}


///////////////////////////////////////////////////////////////////////////////

SGBinding::SGBinding() = default;

SGBinding::SGBinding(const std::string& commandName)
{
    _command_name = commandName;
}

void
SGBinding::clear()
{
  SGAbstractBinding::clear();
  _root.clear();
  _setting.clear();
}

/**
 * @brief Create a binding object from node
 * node: binding configuration
 * root: property tree root
 */
void SGBinding::read(const SGPropertyNode* node, SGPropertyNode* root)
{
    SGAbstractBinding::read(node, root);
    _command_name = node->getStringValue("command", "");
    _root = const_cast<SGPropertyNode*>(root);
    _setting.clear();
}

void
SGBinding::innerFire () const
{
    auto cmd = SGCommandMgr::instance()->getCommand(_command_name);
    // first try command
    if (cmd) {
        try {
            if (!(*cmd)(_arg, _root)) {
                SG_LOG(SG_INPUT, SG_ALERT, "Failed to execute command " << _command_name);
            }
        } catch (sg_exception& e) {
            SG_LOG(SG_GENERAL, SG_ALERT, "command '" << _command_name << "' failed with exception\n"
                                                    << "\tmessage:" << e.getMessage() << " (from " << e.getOrigin() << ")");
        }
    } else {
        // command names are dynamic (Nasal / add-ons can change them), so we can only validate this now
        SG_LOG(SG_GENERAL, SG_ALERT, "Binding: unknown command:" << _command_name << " from\n\t" << _arg->getLocation());
    }
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

void fireBindingList(const SGBindingList& aBindings, SGPropertyNode* params)
{
    for (auto b : aBindings) {
        b->fire(params);
    }
}

void fireBindingList(const std::vector<SGBinding_ptr>& aBindings, SGPropertyNode* params)
{
    for (auto b : aBindings) {
        b->fire(params);
    }
}

void fireBindingListWithOffset(const SGBindingList& aBindings, double offset, double max)
{
    for (auto b : aBindings) {
        b->fire(offset, max);
    }
}

SGBindingList readBindingList(const simgear::PropertyList& aNodes, SGPropertyNode* aRoot)
{
    SGBindingList result;
    for (auto node : aNodes) {
        result.push_back(SGAbstractBinding::createFromProps(node, aRoot));
    }

    return result;
}

void clearBindingList(const SGBindingList& aBindings)
{
    for (auto b : aBindings) {
        b->clear();
    }
}

bool anyBindingEnabled(const SGBindingList& aBindings)
{
    if (aBindings.empty()) {
        return false;
    }

    for (auto b : aBindings) {
        if (b->test()) {
            return true;
        }
    }

    return false;
}
