uniform mat4 u_mat;

varying vec2 m_cordY;
varying vec2 m_cordU;
varying vec2 m_cordV;

attribute vec2 a_texCoordY;
attribute vec2 a_texCoordU;
attribute vec2 a_texCoordV;

attribute vec3 a_vertex;

void main()
{
  m_cordY = a_texCoordY;
  m_cordU = a_texCoordU;
  m_cordV = a_texCoordV;
  
  gl_Position = u_mat * vec4(a_vertex,1.0);
}