#include "shader.h"
#include <stdio.h>
#include <assert.h>

// helper used for shader loading
enum LoadFileToMemoryResult { CouldNotOpenFile = -1, CouldNotReadFile = -2 };
static int _LoadFileToMemory(const char *filename, char **result)
{
    int size = 0;
    FILE *f = fopen(filename, "rb");
    if (f == NULL)
    {
        *result = NULL;
        return CouldNotOpenFile;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    *result = new char[size + 1];

    int newSize = (int)fread(*result, sizeof(char), size, f);
    if (size != newSize)
    {
        fprintf(stderr, "size mismatch, size = %d, newSize = %d\n", size, newSize);
        delete [] *result;
        return CouldNotReadFile;
    }
    fclose(f);
    (*result)[size] = 0;  // make sure it's null terminated.
    return size;
}

static bool _CompileShader(GLint* shader, GLenum type, const char* source, int size)
{
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, (const GLchar**)&source, &size);
    glCompileShader(*shader);

    GLint compiled;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);
    return compiled;
}

static void _DumpShaderInfoLog(GLint shader)
{
    GLint bufferLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &bufferLen);
    if (bufferLen > 1)
    {
        GLsizei len = 0;
        char* buffer = new char[bufferLen];
        glGetShaderInfoLog(shader, bufferLen, &len, buffer);
        fprintf(stderr, "glGetShaderInfoLog() = \n");
        fprintf(stderr, "%s", buffer);
        delete [] buffer;
    }
}

static void _DumpProgramInfoLog(GLint prog)
{
    const GLint bufferLen = 4096;
    GLsizei len = 0;
    char* buffer = new char[bufferLen];
    glGetProgramInfoLog(prog, bufferLen, &len, buffer);
    if (len > 0)
    {
        fprintf(stderr, "glGetProgramInfoLog() = \n");
        fprintf(stderr, "%s", buffer);
        delete [] buffer;
    }
}

static bool _Link(GLint *program, GLint vertShader, GLint fragShader)
{
    assert(vertShader && fragShader);

    *program = glCreateProgram();
    glAttachShader(*program, vertShader);
    glAttachShader(*program, fragShader);
    glLinkProgram(*program);

    GLint linked;
    glGetProgramiv(*program, GL_LINK_STATUS, &linked);
    return linked;
}


Shader::Shader() : m_vertShader(0), m_fragShader(0), m_program(0)
{

}

Shader::~Shader()
{
    if (m_vertShader)
        glDeleteShader(m_vertShader);
    if (m_fragShader)
        glDeleteShader(m_fragShader);
    if (m_program)
        glDeleteProgram(m_program);
}

bool Shader::compileAndLinkFromFiles(const std::string& vertShaderFilename,
                                     const std::string& fragShaderFilename)
{
    char* vertShaderSource;
    int vertShaderSize = _LoadFileToMemory(vertShaderFilename.c_str(), &vertShaderSource);
    if (vertShaderSize <= 0)
    {
        fprintf(stderr, "Shader::compileAndLinkFromFiles() Failed to load vertex shader \"%s\"\n", vertShaderFilename.c_str());
        return false;
    }

    char* fragShaderSource;
    int fragShaderSize = _LoadFileToMemory(fragShaderFilename.c_str(), &fragShaderSource);
    if (fragShaderSize <= 0)
    {
        fprintf(stderr, "Shader::compileAndLinkFromFiles() Failed to load fragment shader \"%s\"\n", fragShaderFilename.c_str());
        delete [] vertShaderSource;
        return false;
    }

    if (!_CompileShader(&m_vertShader, GL_VERTEX_SHADER, vertShaderSource, vertShaderSize))
    {
        fprintf(stderr, "Shader::compileAndLinkFromFiles() Failed to compile vertex shader \"%s\"\n", vertShaderFilename.c_str());
        _DumpShaderInfoLog(m_vertShader);
        glDeleteShader(m_vertShader);
        m_vertShader = 0;
        return false;
    }

    if (!_CompileShader(&m_fragShader, GL_FRAGMENT_SHADER, fragShaderSource, fragShaderSize))
    {
        fprintf(stderr, "Shader::compileAndLinkFromFiles() Failed to compile fragment shader \"%s\"\n", fragShaderFilename.c_str());
        _DumpShaderInfoLog(m_fragShader);
        glDeleteShader(m_fragShader);
        m_fragShader = 0;
        return false;
    }

    if (!_Link(&m_program, m_vertShader, m_fragShader))
    {
        fprintf(stderr, "Shader::compileAndLinkFromFiles() Failed to link shaders \"%s\" and \"%s\"\n", vertShaderFilename.c_str(), fragShaderFilename.c_str());
        _DumpProgramInfoLog(m_program);
        glDeleteProgram(m_program);
        m_program = 0;
        return false;
    }

    m_vertShaderFilename = vertShaderFilename;
    m_fragShaderFilename = fragShaderFilename;

    buildVarMaps();
    return true;
}

int Shader::getUniformLoc(const std::string& uniformName) const
{
    VarMap::const_iterator iter = m_uniformVarMap.find(uniformName);
    if (iter != m_uniformVarMap.end())
        return iter->second.loc;
    else
        return -1;
}

int Shader::getAttribLoc(const std::string& attribName) const
{
    VarMap::const_iterator iter = m_attribVarMap.find(attribName);
    if (iter != m_attribVarMap.end())
        return iter->second.loc;
    else
        return -1;
}

GLint Shader::getProgram() const
{
    return m_program;
}

void Shader::buildVarMaps()
{
    const int MAX_STRING_SIZE = 1024;
    char tempStr[MAX_STRING_SIZE];
    int numUniforms = 0;
    int numAttribs = 0;
    Var v;

    glGetProgramiv(m_program, GL_ACTIVE_UNIFORMS, &numUniforms);
    for (int i = 0; i < numUniforms; ++i)
    {
        GLsizei strLen;
        glGetActiveUniform(m_program, i, MAX_STRING_SIZE, &strLen, &v.size, &v.type, tempStr);
        v.loc = glGetUniformLocation(m_program, tempStr);
        m_uniformVarMap[std::string(tempStr)] = v;
    }

    m_attribLocMask = 0;
    glGetProgramiv(m_program, GL_ACTIVE_ATTRIBUTES, &numAttribs);
    for (int i = 0; i < numAttribs; ++i)
    {
        GLsizei strLen;
        glGetActiveAttrib(m_program, i, MAX_STRING_SIZE, &strLen, &v.size, &v.type, tempStr);
        v.loc = glGetAttribLocation(m_program, tempStr);
        m_attribVarMap[std::string(tempStr)] = v;
        m_attribLocMask |= ((uint32_t)1 << v.loc);
        assert(v.loc < 32);
    }

    dump();
}

void Shader::apply(const Shader* prev) const
{
    if (prev && prev->m_program == m_program)
        return;

    assert(m_program);
    glUseProgram(m_program);

    uint64_t diff = m_attribLocMask;
    if (prev)
        diff = m_attribLocMask ^ prev->m_attribLocMask;

    if (diff) {
        for (int i = 0; i < 32; i++) {
            if (diff & (1 << i)) {
                if (m_attribLocMask & (1 << i)) {
                    glEnableVertexAttribArray(i);
                } else {
                    glDisableVertexAttribArray(i);
                }
            }
        }
    }
}

void Shader::dump() const
{
    printf("Shader \"%s\", \"%s\"\n", m_vertShaderFilename.c_str(), m_fragShaderFilename.c_str());

    printf("    uniforms\n");
    VarMap::const_iterator uIter = m_uniformVarMap.begin();
    VarMap::const_iterator uEnd = m_uniformVarMap.end();
    for (; uIter != uEnd; uIter++)
    {
        printf("        \"%s\": loc = %d, type = %d, size = %d\n",
               uIter->first.c_str(), uIter->second.loc, uIter->second.type, uIter->second.size);
    }

    printf("    attribs\n");
    uIter = m_attribVarMap.begin();
    uEnd = m_attribVarMap.end();
    for (; uIter != uEnd; uIter++)
    {
        printf("        \"%s\": loc = %d, type = %d, size = %d\n",
               uIter->first.c_str(), uIter->second.loc, uIter->second.type, uIter->second.size);
    }
}
