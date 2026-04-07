/// @brief Cached compilation of Nasal script code
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026  James Turner <zakalawe@mac.com>

#include "NasalCode.hxx"

#include <sstream>

namespace nasal {

//----------------------------------------------------------------------------
NasalCode::NasalCode(naRef globals,
                     const std::string& source,
                     const std::string& filename,
                     int firstLine) : _codeRef()
{
    Context ctx;
    int errLine = -1;

    naRef srcfile = naNewString(ctx);
    naStr_fromdata(srcfile, const_cast<char*>(filename.c_str()),
                   filename.length());

    naRef code = naParseCode(ctx, srcfile, firstLine,
                             const_cast<char*>(source.c_str()),
                             source.length(), &errLine);

    if (naIsNil(code)) {
        std::ostringstream os;
        os << "Nasal parse error: " << naGetError(ctx)
           << " in " << filename << ", line " << errLine;
        _errors.push_back(os.str());
        return;
    }

    // Bind to the provided closure (global namespace) and GC-protect
    naRef func = naBindFunction(ctx, code, globals);
    _codeRef.reset(func);
}

//----------------------------------------------------------------------------
bool NasalCode::isValid() const
{
    return _codeRef.valid();
}

//----------------------------------------------------------------------------
const std::vector<std::string>& NasalCode::getErrors() const
{
    return _errors;
}

//----------------------------------------------------------------------------
naRef NasalCode::doCall(Context& ctx, std::initializer_list<naRef> args)
{
    if (!isValid()) {
        return naNil();
    }

    naRef result = naCallMethodCtx(
        ctx,
        _codeRef.get_naRef(),
        naNil(), // self (no 'me' object)
        args.size(),
        const_cast<naRef*>(args.begin()),
        naNil() // locals
    );

    if (const char* error = naGetError(ctx)) {
        _errors.push_back(std::string("Nasal runtime error: ") + error);
        return naNil();
    }

    return result;
}

//----------------------------------------------------------------------------
naRef NasalCode::callWithLocals(naRef locals)
{
    if (!isValid()) {
        return naNil();
    }

    Context ctx;
    naRef result = naCallMethodCtx(
        ctx,
        _codeRef.get_naRef(),
        naNil(), // self
        0, nullptr,
        locals);

    if (const char* error = naGetError(ctx)) {
        _errors.push_back(std::string("Nasal runtime error: ") + error);
        return naNil();
    }

    return result;
}

} // namespace nasal
