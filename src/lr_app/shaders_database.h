#include <span>

struct ShaderInfo;

extern const std::span<const ShaderInfo*> c_all_shaders;

extern const ShaderInfo c_vs_basic_phong;
extern const ShaderInfo c_ps_basic_phong;

extern const ShaderInfo c_vs_lines;
extern const ShaderInfo c_ps_lines;
