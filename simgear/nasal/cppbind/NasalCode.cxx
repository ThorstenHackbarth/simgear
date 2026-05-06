/// @brief Cached compilation of Nasal script code
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026  James Turner <zakalawe@mac.com>

#include "NasalCode.hxx"

#include <sstream>

#include <simgear/structure/exception.hxx>

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
    _codeRef = ObjectHolder<SGReferenced>::makeShared(func);
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
naRef NasalCode::doCall(Context& ctx, std::initializer_list<naRef> args) const
{
    if (!isValid()) {
        return naNil();
    }

    naRef result = naCallMethodCtx(
        ctx,
        _codeRef->get_naRef(),
        naNil(), // self (no 'me' object)
        args.size(),
        const_cast<naRef*>(args.begin()),
        naNil() // locals
    );

    // naCallMethodCtx has already invoked the registered error handler
    // (which logs the error).  Re-surface it as a C++ exception so that
    // callers can detect the failure and disable the offending code path.
    if (const char* err = naGetError(ctx)) {
        int line = naGetLine(ctx, 0);
        char* file = naStr_data(naGetSourceFile(ctx, 0));
        throw sg_exception(std::string("Nasal runtime error: ") + err,
                           "",
                           sg_location(file, line));
    }

    return result;
}

//----------------------------------------------------------------------------
naRef NasalCode::call() const
{
    Context ctx;
    return doCall(ctx, {});
}

//----------------------------------------------------------------------------
naRef NasalCode::callWithLocals(naRef locals) const
{
    if (!isValid()) {
        return naNil();
    }

    Context ctx;
    int lsave = naGCSave(locals);
    naRef result = naCallMethodCtx(
        ctx,
        _codeRef->get_naRef(),
        naNil(), // self
        0, nullptr,
        locals);

    naGCRelease(lsave);

    // naCallMethodCtx has already invoked the registered error handler
    // (which logs the error).  Re-surface it as a C++ exception so that
    // callers can detect the failure and disable the offending code path.
    if (const char* err = naGetError(ctx)) {
        int line = naGetLine(ctx, 0);
        char* file = naStr_data(naGetSourceFile(ctx, 0));
        throw sg_exception(std::string("Nasal runtime error: ") + err,
                           "",
                           sg_location(file, line));
    }

    return result;
}

} // namespace nasal
