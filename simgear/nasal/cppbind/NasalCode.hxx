/// @brief Cached compilation of Nasal script code
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026  James Turner <zakalawe@mac.com>

#pragma once

#include "NasalContext.hxx"
#include "NasalObjectHolder.hxx"

#include <string>
#include <vector>

namespace nasal {

/**
   * Compiles and caches a Nasal code string. The compiled code is protected
   * from GC and can be called repeatedly with different arguments, each call
   * creating a fresh naContext.
   */
class NasalCode
{
public:
    /**
       * Construct from a Nasal source string. The code is immediately parsed.
       *
       * @param globals   The globals hash to bind the code to
       * @param source    The Nasal source code to compile
       * @param filename  Optional filename for error reporting
       * @param firstLine Optional first line number for error reporting
       */
    NasalCode(naRef globals,
              const std::string& source,
              const std::string& filename = "<inline>",
              int firstLine = 1);

    NasalCode() = default;

    /**
       * Check whether the code was parsed successfully.
       */
    bool isValid() const;

    /**
       * Get any errors that occurred during parsing.
       */
    const std::vector<std::string>& getErrors() const;

    /**
       * Call the compiled code with no arguments. Creates a new
       * naContext for the invocation.
       *
       * @return The naRef result of calling the code
       */
    naRef call() const;

    /**
       * Call the compiled code with variadic arguments. Creates a new
       * naContext for each invocation.
       *
       * @tparam Args  Argument types (converted via to_nasal)
       * @param  args  Arguments to pass to the code
       * @return The naRef result of calling the code
       */
    template <class... Args>
    naRef call(Args... args) const;

    /**
       * Call the compiled code and convert the result to the requested type.
       *
       * @tparam Ret   Return type (converted via from_nasal)
       * @tparam Args  Argument types (converted via to_nasal)
       * @param  args  Arguments to pass to the code
       * @return The result converted to type Ret
       */
    template <class Ret, class... Args>
    Ret call(Args... args) const;

    /**
       * Call the compiled code with an explicit locals hash. Creates a new
       * naContext for the invocation.
       *
       * @param locals  Hash to use as the local variable namespace (may be nil)
       * @return The naRef result of calling the code
       */
    naRef callWithLocals(naRef locals) const;

private:
    naRef doCall(Context& ctx, std::initializer_list<naRef> args) const;

    ObjectHolder<SGReferenced>::Ref _codeRef;
    std::vector<std::string> _errors;
};

//----------------------------------------------------------------------------
template <class... Args>
naRef NasalCode::call(Args... args) const
{
    Context ctx;
    return doCall(ctx, {ctx.to_nasal(args)...});
}

//----------------------------------------------------------------------------
template <class Ret, class... Args>
Ret NasalCode::call(Args... args) const
{
    Context ctx;
    naRef result = doCall(ctx, {ctx.to_nasal(args)...});
    return ctx.from_nasal<Ret>(result);
}

} // namespace nasal
