#ifndef __SHADER_H__
#define __SHADER_H__

#include <string>
#include <map>
#include "opengl.h"

class Shader
{
public:
    Shader();
    ~Shader();
    bool compileAndLinkFromFiles(const std::string& vertShaderFilename,
                                 const std::string& fragShaderFilename);
    int getUniformLoc(const std::string& uniformName) const;
    int getAttribLoc(const std::string& attribName) const;
    GLint getProgram() const;
    void apply(const Shader* prev) const;
    void dump() const;
protected:
    void buildVarMaps();

    std::string m_vertShaderFilename;
    std::string m_fragShaderFilename;

    GLint m_vertShader;
    GLint m_fragShader;
    GLint m_program;

    struct Var
    {
        GLint size;
        GLenum type;
        GLint loc;
    };

    typedef std::map<const std::string, Var> VarMap;
    VarMap m_uniformVarMap;
    VarMap m_attribVarMap;
    uint32_t m_attribLocMask;
};

#endif
