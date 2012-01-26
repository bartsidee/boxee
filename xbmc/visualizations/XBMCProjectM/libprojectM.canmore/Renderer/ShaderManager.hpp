#ifndef SHADER_MANAGER_HPP
#define SHADER_MANAGER_HPP

#include <string>

#define VERTEX_BUFFER 0
#define COLOR_BUFFER 1
#define TEX_COORD_BUFFER 2

class ShaderManager
{
  private:
    int loadShader(const char* vertex, const char* fragment); 
  public:
    int shader;
    int shader2;
    int shader3;
    int shader_tex;
    int shader_tex_scaled;
    ShaderManager();
    ~ShaderManager();
};

#endif

