#version 450

#include "../shader_type.cpp"

layout(location = 0) in vec3 position_in;
layout(location = 1) in vec2 uv_in;
layout(location = 2) in vec3 normal_in;
layout(location = 3) in vec3 tangent_in;
layout(location = 4) in uvec4 joints_in;
layout(location = 5) in vec4 weights_in;

layout(location = 0) out vec3 position_out;
layout(location = 1) out vec4 shadow_map_coord_out;
layout(location = 2) out vec2 uv_out;
layout(location = 3) out vec3 tbn_position_out;
layout(location = 4) out vec3 tbn_camera_position_out;
layout(location = 5) out vec3 tbn_directional_light_out;
layout(location = 6) out vec3 tbn_point_light_out;

out gl_PerVertex {
  vec4 gl_Position;
};

m_declare_uniform_buffer
m_declare_model_push_constant

void main() {
  mat4 joint_mat = m_mesh_joint_mat(joints_in[0]) * weights_in[0] +
                   m_mesh_joint_mat(joints_in[1]) * weights_in[1] +
                   m_mesh_joint_mat(joints_in[2]) * weights_in[2] +
                   m_mesh_joint_mat(joints_in[3]) * weights_in[3];

  mat4 model_mat = m_model_model_mat * joint_mat;
  mat3 normal_mat = mat3(model_mat);

  vec4 position = model_mat * vec4(position_in, 1);
  position_out = position.xyz;
  gl_Position = m_level_view_proj_mat * position;

  uv_out = uv_in;
  shadow_map_coord_out = m_level_shadow_map_proj_mat * position;

  vec3 normal = normalize(normal_mat * normal_in);
  vec3 tangent = normalize(normal_mat * tangent_in);
  vec3 bitangent = normalize(cross(normal, tangent));
  mat3 inverse_tbn_mat = transpose(mat3(tangent, bitangent, normal));

  tbn_position_out = inverse_tbn_mat * position.xyz;
  tbn_camera_position_out = inverse_tbn_mat * m_level_camera_position.xyz;
  tbn_directional_light_out = inverse_tbn_mat * m_level_directional_light_dir.xyz;
  tbn_point_light_out = inverse_tbn_mat * m_level_point_light_position.xyz;
} 