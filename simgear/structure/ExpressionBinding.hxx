
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 James Turner

#pragma once

#include "simgear/props/props.hxx"
#include <simgear/structure/SGBinding.hxx>

#include "SGExpression.hxx"

namespace simgear {

class ExpressionBinding : public SGAbstractBinding
{
public:
    ExpressionBinding() = default;

protected:
    void read(const SGPropertyNode* config, SGPropertyNode* root) override;
    void innerFire() const override;

private:
    SGPropertyNode_ptr _target;
    SGExpressiond_ref _expression;
    size_t _settingArgLocation = -1;
};


} // namespace simgear
