// SPDX-FileCopyrightText: 2007 Ivan Leben
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "shConfig.h"

#if defined(VG_API_MACOSX)
#include <OpenGL/gl3.h>
#elif defined(VG_API_WINDOWS)
#include <GL/glcorearb.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/glcorearb.h>
#endif

/*
 * We only need to query function pointers manually on Windows.
 *
 * On Linux assume that the GL library exports all the core profile functions
 * when GL_GLEXT_PROTOTYPES is used. Technically this is not guaranteed by the
 * OpenGL ABI, but in practice it (almost) always works.
 *
 * On MacOS we are stuck with whatever Apple decided to implement in
 * OpenGL/gl3.h
 */
#if defined(VG_API_WINDOWS)
extern PFNGLUNIFORM1IPROC glUniform1i;
extern PFNGLUNIFORM2FVPROC glUniform2fv;
extern PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;
extern PFNGLUNIFORM2FPROC glUniform2f;
extern PFNGLUNIFORM4FVPROC glUniform4fv;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORM3FPROC glUniform3f;
extern PFNGLUNIFORM4FPROC glUniform4f;
extern PFNGLUNIFORM1FVPROC glUniform1fv;
extern PFNGLUNIFORM3FVPROC glUniform3fv;
extern PFNGLUNIFORM2IPROC glUniform2i;
extern PFNGLUNIFORM3IPROC glUniform3i;
extern PFNGLUNIFORM4IPROC glUniform4i;
extern PFNGLUNIFORM1IVPROC glUniform1iv;
extern PFNGLUNIFORM2IVPROC glUniform2iv;
extern PFNGLUNIFORM3IVPROC glUniform3iv;
extern PFNGLUNIFORM4IVPROC glUniform4iv;
extern PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv;
extern PFNGLGETUNIFORMFVPROC glGetUniformfv;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLACTIVETEXTUREPROC glActiveTexture;
/* FlightGear additions to use VAOs and VBOs */
extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;
#endif

void shLoadExtensions(void);
