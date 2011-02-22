/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#if SDL_VIDEO_RENDER_OGL && !SDL_RENDER_DISABLED

#include "SDL_stdinc.h"
#include "SDL_log.h"
#include "SDL_opengl.h"
#include "SDL_video.h"
#include "SDL_shaders_gl.h"

/* OpenGL shader implementation */

/*#define DEBUG_SHADERS*/

typedef struct
{
    GLenum program;
    GLenum vert_shader;
    GLenum frag_shader;
} GL_ShaderData;

struct GL_ShaderContext
{
    GLenum (*glGetError)(void);

    PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
    PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
    PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
    PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
    PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
    PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
    PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
    PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
    PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
    PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
    PFNGLUNIFORM1IARBPROC glUniform1iARB;
    PFNGLUNIFORM1FARBPROC glUniform1fARB;
    PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;

    SDL_bool GL_ARB_texture_rectangle_supported;

    GL_ShaderData shaders[NUM_SHADERS];
};

/*
 * NOTE: Always use sampler2D, etc here. We'll #define them to the
 *  texture_rectangle versions if we choose to use that extension.
 */
static const char *shader_source[NUM_SHADERS][2] =
{
    /* SHADER_NONE */
    { NULL, NULL },

    /* SHADER_SOLID */
    {
        /* vertex shader */
"varying vec4 v_color;\n"
"\n"
"void main()\n"
"{\n"
"    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"    v_color = gl_Color;\n"
"}",
        /* fragment shader */
"varying vec4 v_color;\n"
"\n"
"void main()\n"
"{\n"
"    gl_FragColor = v_color;\n"
"}"
    },

    /* SHADER_RGB */
    {
        /* vertex shader */
"varying vec4 v_color;\n"
"varying vec2 v_texCoord;\n"
"\n"
"void main()\n"
"{\n"
"    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"    v_color = gl_Color;\n"
"    v_texCoord = vec2(gl_MultiTexCoord0);\n"
"}",
        /* fragment shader */
"varying vec4 v_color;\n"
"varying vec2 v_texCoord;\n"
"uniform sampler2D tex0;\n"
"\n"
"void main()\n"
"{\n"
"    gl_FragColor = texture2D(tex0, v_texCoord) * v_color;\n"
"}"
    },

    /* SHADER_YV12 */
    {
        /* vertex shader */
"varying vec4 v_color;\n"
"varying vec2 v_texCoord;\n"
"\n"
"void main()\n"
"{\n"
"    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"    v_color = gl_Color;\n"
"    v_texCoord = vec2(gl_MultiTexCoord0);\n"
"}",
        /* fragment shader */
"varying vec4 v_color;\n"
"varying vec2 v_texCoord;\n"
"uniform sampler2D tex0; // Y \n"
"uniform sampler2D tex1; // U \n"
"uniform sampler2D tex2; // V \n"
"\n"
"// YUV offset \n"
"const vec3 offset = vec3(-0.0625, -0.5, -0.5);\n"
"\n"
"// RGB coefficients \n"
"const vec3 Rcoeff = vec3(1.164,  0.000,  1.596);\n"
"const vec3 Gcoeff = vec3(1.164, -0.391, -0.813);\n"
"const vec3 Bcoeff = vec3(1.164,  2.018,  0.000);\n"
"\n"
"void main()\n"
"{\n"
"    vec2 tcoord;\n"
"    vec3 yuv, rgb;\n"
"\n"
"    // Get the Y value \n"
"    tcoord = v_texCoord;\n"
"    yuv.x = texture2D(tex0, tcoord).r;\n"
"\n"
"    // Get the U and V values \n"
"    tcoord *= 0.5;\n"
"    yuv.y = texture2D(tex1, tcoord).r;\n"
"    yuv.z = texture2D(tex2, tcoord).r;\n"
"\n"
"    // Do the color transform \n"
"    yuv += offset;\n"
"    rgb.r = dot(yuv, Rcoeff);\n"
"    rgb.g = dot(yuv, Gcoeff);\n"
"    rgb.b = dot(yuv, Bcoeff);\n"
"\n"
"    // That was easy. :) \n"
"    gl_FragColor = vec4(rgb, 1.0) * v_color;\n"
"}"
    },
};

static SDL_bool
CompileShader(GL_ShaderContext *ctx, GLenum shader, const char *defines, const char *source)
{
    GLint status;
    const char *sources[2];

    sources[0] = defines;
    sources[1] = source;

    ctx->glShaderSourceARB(shader, SDL_arraysize(sources), sources, NULL);
    ctx->glCompileShaderARB(shader);
    ctx->glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &status);
    if (status == 0) {
        GLint length;
        char *info;

        ctx->glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
        info = SDL_stack_alloc(char, length+1);
        ctx->glGetInfoLogARB(shader, length, NULL, info);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER,
            "Failed to compile shader:\n%s%s\n%s", defines, source, info);
#ifdef DEBUG_SHADERS
        fprintf(stderr,
            "Failed to compile shader:\n%s%s\n%s", defines, source, info);
#endif
        SDL_stack_free(info);

        return SDL_FALSE;
    } else {
        return SDL_TRUE;
    }
}

static SDL_bool
CompileShaderProgram(GL_ShaderContext *ctx, int index, GL_ShaderData *data)
{
    const int num_tmus_bound = 4;
    const char *vert_defines = "";
    const char *frag_defines = "";
    int i;
    GLint location;

    if (index == SHADER_NONE) {
        return SDL_TRUE;
    }

    ctx->glGetError();

    /* Make sure we use the correct sampler type for our texture type */
    if (ctx->GL_ARB_texture_rectangle_supported) {
        frag_defines = 
"#define sampler2D sampler2DRect\n"
"#define texture2D texture2DRect\n";
    }

    /* Create one program object to rule them all */
    data->program = ctx->glCreateProgramObjectARB();

    /* Create the vertex shader */
    data->vert_shader = ctx->glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
    if (!CompileShader(ctx, data->vert_shader, vert_defines, shader_source[index][0])) {
        return SDL_FALSE;
    }

    /* Create the fragment shader */
    data->frag_shader = ctx->glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
    if (!CompileShader(ctx, data->frag_shader, frag_defines, shader_source[index][1])) {
        return SDL_FALSE;
    }

    /* ... and in the darkness bind them */
    ctx->glAttachObjectARB(data->program, data->vert_shader);
    ctx->glAttachObjectARB(data->program, data->frag_shader);
    ctx->glLinkProgramARB(data->program);

    /* Set up some uniform variables */
    ctx->glUseProgramObjectARB(data->program);
    for (i = 0; i < num_tmus_bound; ++i) {
        char tex_name[5];
        SDL_snprintf(tex_name, SDL_arraysize(tex_name), "tex%d", i);
        location = ctx->glGetUniformLocationARB(data->program, tex_name);
        if (location >= 0) {
            ctx->glUniform1iARB(location, i);
        }
    }
    ctx->glUseProgramObjectARB(0);
    
    return (ctx->glGetError() == GL_NO_ERROR);
}

static void
DestroyShaderProgram(GL_ShaderContext *ctx, GL_ShaderData *data)
{
    ctx->glDeleteObjectARB(data->vert_shader);
    ctx->glDeleteObjectARB(data->frag_shader);
    ctx->glDeleteObjectARB(data->program);
}

GL_ShaderContext *
GL_CreateShaderContext()
{
    GL_ShaderContext *ctx;
    SDL_bool shaders_supported;
    int i;

    ctx = (GL_ShaderContext *)SDL_calloc(1, sizeof(*ctx));
    if (!ctx) {
        return NULL;
    }

    if (SDL_GL_ExtensionSupported("GL_ARB_texture_rectangle")
        || SDL_GL_ExtensionSupported("GL_EXT_texture_rectangle")) {
        ctx->GL_ARB_texture_rectangle_supported = SDL_TRUE;
    }

    /* Check for shader support */
    shaders_supported = SDL_FALSE;
    if (SDL_GL_ExtensionSupported("GL_ARB_shader_objects") &&
        SDL_GL_ExtensionSupported("GL_ARB_shading_language_100") &&
        SDL_GL_ExtensionSupported("GL_ARB_vertex_shader") &&
        SDL_GL_ExtensionSupported("GL_ARB_fragment_shader")) {
        ctx->glGetError = (GLenum (*)(void)) SDL_GL_GetProcAddress("glGetError");
        ctx->glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC) SDL_GL_GetProcAddress("glAttachObjectARB");
        ctx->glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC) SDL_GL_GetProcAddress("glCompileShaderARB");
        ctx->glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC) SDL_GL_GetProcAddress("glCreateProgramObjectARB");
        ctx->glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC) SDL_GL_GetProcAddress("glCreateShaderObjectARB");
        ctx->glDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC) SDL_GL_GetProcAddress("glDeleteObjectARB");
        ctx->glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC) SDL_GL_GetProcAddress("glGetInfoLogARB");
        ctx->glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC) SDL_GL_GetProcAddress("glGetObjectParameterivARB");
        ctx->glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC) SDL_GL_GetProcAddress("glGetUniformLocationARB");
        ctx->glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC) SDL_GL_GetProcAddress("glLinkProgramARB");
        ctx->glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC) SDL_GL_GetProcAddress("glShaderSourceARB");
        ctx->glUniform1iARB = (PFNGLUNIFORM1IARBPROC) SDL_GL_GetProcAddress("glUniform1iARB");
        ctx->glUniform1fARB = (PFNGLUNIFORM1FARBPROC) SDL_GL_GetProcAddress("glUniform1fARB");
        ctx->glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC) SDL_GL_GetProcAddress("glUseProgramObjectARB");
        if (ctx->glGetError &&
            ctx->glAttachObjectARB &&
            ctx->glCompileShaderARB &&
            ctx->glCreateProgramObjectARB &&
            ctx->glCreateShaderObjectARB &&
            ctx->glDeleteObjectARB &&
            ctx->glGetInfoLogARB &&
            ctx->glGetObjectParameterivARB &&
            ctx->glGetUniformLocationARB &&
            ctx->glLinkProgramARB &&
            ctx->glShaderSourceARB &&
            ctx->glUniform1iARB &&
            ctx->glUniform1fARB &&
            ctx->glUseProgramObjectARB) {
            shaders_supported = SDL_TRUE;
        }
    }

    if (!shaders_supported) {
        SDL_free(ctx);
        return NULL;
    }

    /* Compile all the shaders */
    for (i = 0; i < NUM_SHADERS; ++i) {
        if (!CompileShaderProgram(ctx, i, &ctx->shaders[i])) {
            GL_DestroyShaderContext(ctx);
            return NULL;
        }
    }

    /* We're done! */
    return ctx;
}

void
GL_SelectShader(GL_ShaderContext *ctx, GL_Shader shader)
{
    ctx->glUseProgramObjectARB(ctx->shaders[shader].program);
}

void
GL_DestroyShaderContext(GL_ShaderContext *ctx)
{
    int i;

    for (i = 0; i < NUM_SHADERS; ++i) {
        DestroyShaderProgram(ctx, &ctx->shaders[i]);
    }
    SDL_free(ctx);
}

#endif /* SDL_VIDEO_RENDER_OGL && !SDL_RENDER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
