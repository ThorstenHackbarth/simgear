
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 James Turner


#include "simgear/structure/ExpressionBinding.hxx"
#include "simgear/debug/debug_types.h"
#include "simgear/structure/SGExpression.hxx"
#include "simgear_config.h"

#include "simgear/structure/SGBinding.hxx"
#include "simgear/structure/exception.hxx"
#include <simgear/props/props.hxx>

using namespace simgear;


Expression* bindingSettingParser(const SGPropertyNode* exp,
                                 expression::Parser* parser)
{
    SG_UNUSED(exp);
    const auto location = parser->getBindingLayout().addBinding("setting",
                                                                expression::DOUBLE);
    VariableExpression<double>* settingVarExp = new VariableExpression<double>(location);
    return settingVarExp;
}

expression::ExpParserRegistrar bindingSettingRegistrar("binding-setting",
                                                       bindingSettingParser);

static SGSharedPtr<SGAbstractBinding>
expressionBindingFactory(SGPropertyNode_ptr config, SGPropertyNode_ptr root)
{
    return new simgear::ExpressionBinding();
}

static bool expressionBindingRegistered = [] {
    SGAbstractBinding::registerFactory("expression", expressionBindingFactory);
    return true;
}();

///////////////////////////////////////////////////////////////////////////////

void ExpressionBinding::read(const SGPropertyNode* node, SGPropertyNode* root)
{
    SGAbstractBinding::read(node, root);

    const SGPropertyNode* expressionNode = node->getChild("expression");
    const SGPropertyNode* target = node->getChild("property");
    if (!target) {
        throw sg_exception("Expression-binding must specify a <property> target", {}, sg_location{node->getLocation()});
    }

    _setting.clear();

    _target = root->getNode(target->getStringValue(), true);

    // input value is stored in 'setting'
    _setting = _arg->getChild("setting", 0, true);

    if (_debug) {
        SG_LOG(SG_GENERAL, SG_MANDATORY_INFO, "Reading expression for binding " << node->getPath());
        SG_LOG(SG_GENERAL, SG_MANDATORY_INFO, "Input from " << _setting->getPath());
        SG_LOG(SG_GENERAL, SG_MANDATORY_INFO, "Output to " << _target->getPath());
    }

    expression::ExpressionParser parser;

    /*
    Pass the arg node as property tree root to expression.
    Absolute property paths in <expression> XML will work as usual
    */
    _expression = SGReadDoubleExpression(_arg, expressionNode->getChild(0), &parser);
    if (!_expression) {
        throw sg_exception("Failed to read expression correctly", {}, sg_location{expressionNode->getLocation()});
    }

    expression::VariableBinding svb;
    if (parser.getBindingLayout().findBinding("setting", svb)) {
        _settingArgLocation = svb.location;
    }
}

void ExpressionBinding::innerFire() const
{
    expression::FixedLengthBinding<1> binding;
    if (_settingArgLocation != -1) {
        assert(_settingArgLocation == 0);
        binding.getBindings()[_settingArgLocation] = expression::Value(_setting->getDoubleValue());
    }

    double result = _expression->getDoubleValue(&binding);
    if (_debug) {
        SG_LOG(SG_INPUT, SG_MANDATORY_INFO, "Expression result {" << _arg->getPath() << "}:" << result);
    }

    _target->setDoubleValue(result);
}
