/***************************************************************************************************/
/*          Copyright (C) 2017-2018 By Yang Chen (yngccc@gmail.com). All Rights Reserved.          */
/***************************************************************************************************/

#pragma once

#include "../vendor/include/json/json.hpp"

#include "../vendor/include/bullet/btBulletCollisionCommon.h"
#include "../vendor/include/bullet/btBulletDynamicsCommon.h"
#include "../vendor/include/bullet/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"

#include "geometry.cpp"
#include "simd.cpp"

const uint32 level_max_entity_count = 1024;
const uint32 level_max_directional_light_count = 1;
const uint32 level_max_point_light_count = 4;
const uint32 level_max_spot_light_count = 4;
const uint32 level_terrain_resolution = 128;
const float level_terrain_size = 64;

struct model_scene {
	uint32 node_indices[m_countof(gpk_model_scene::node_indices)];
	uint32 node_index_count;
	char name[sizeof(gpk_model_scene::name)];
};

struct model_node {
	uint32 mesh_index;
	transform local_transform;
	mat4 local_transform_mat;
	uint32 children[m_countof(gpk_model_node::children)];
	uint32 child_count;
};

struct model_mesh_primitive {
	uint32 material_index;
	uint32 index_count;
	uint32 index_buffer_offset;
	uint32 vertex_count;
	uint32 vertex_buffer_offset;
	bool has_joints;
	uint8 *vertices_data;
	uint8 *indices_data;
};

struct model_mesh {
	model_mesh_primitive *primitives;
	uint32 primitive_count;
	char name[sizeof(gpk_model_mesh::name)];
};

struct model_joint {
	uint32 node_index;
	mat4 inverse_bind_mat;
};

struct model_skin {
	model_joint *joints;
	uint32 joint_count;
	char name[sizeof(gpk_model_skin::name)];
};

struct model_animation_channel {
	uint32 node_index;
	uint32 channel_type;
	uint32 sampler_index;
};

struct model_animation_key_frame {
	float time;
	vec4 transform_data;
};

struct model_animation_sampler {
	uint32 interpolation_type;
	model_animation_key_frame *key_frames;
	uint32 key_frame_count;
};

struct model_animation {
	model_animation_channel *channels;
	uint32 channel_count;
	model_animation_sampler *samplers;
	uint32 sampler_count;
	char name [sizeof(gpk_model_animation::name)];
};

struct model_material {
	uint32 diffuse_map_descriptor_index;
	uint32 metallic_map_descriptor_index;
	uint32 roughness_map_descriptor_index;
	uint32 normal_map_descriptor_index;
	vec4 diffuse_factor;
	float metallic_factor;
	float roughness_factor;
	char name[sizeof(gpk_model_material::name)];
};

struct model_image {
	uint32 index;
};

struct model {
	model_scene *scenes;
	uint32 scene_count;
	model_node *nodes;
	uint32 node_count;
	model_mesh *meshes;
	uint32 mesh_count;
	model_skin *skins;
	uint32 skin_count;
	model_animation *animations;
	uint32 animation_count;
	model_material *materials;
	uint32 material_count;
	model_image *images;
	uint32 image_count;
	char gpk_file[128];
};

struct skybox {
	uint32 cubemap_descriptor_index;
	char gpk_file[128];
};

struct terrain_vertex {
	vec3 position;
	vec2 uv;
};
static_assert(sizeof(struct terrain_vertex) == 20, "");

struct terrain {
	uint32 height_map_descriptor_index;
	uint32 diffuse_map_descriptor_index;
	uint8 *height_map_data;
	btHeightfieldTerrainShape *bt_terrain_shape;
	char gpk_file[128];
};

enum light_type {
	light_type_ambient,
	light_type_directional,
	light_type_point
};

struct ambient_light {
	vec3 color;
};

struct directional_light {
	vec3 color;
	vec3 direction;
};

struct point_light {
	vec3 color;
	vec3 position;
	float attenuation;
};

enum collision_shape {
	collision_shape_sphere,
	collision_shape_capsule,
	collision_shape_box,
	collision_shape_terrain
};

struct entity_info {
	char name[32];
};

enum entity_component_flag {
	entity_component_flag_model    = 1,
	entity_component_flag_collision = 2,
	entity_component_flag_physics   = 4,
	entity_component_flag_light     = 8,
	entity_component_flag_terrain   = 16
};

struct entity_model_component {
	uint32 model_index;
	transform adjustment_transform;
	uint32 animation_index;
	double animation_time;
	bool hide;
};

struct entity_collision_component {
	collision_shape shape;
	union {
		struct {
			float radius;
		} sphere;
		struct {
			float height, radius;
		} capsule;
		struct {
			vec3 size;
		} box;
		struct {
			uint32 terrain_index;
		};
	};
	btCollisionObject *bt_collision_object;
};

struct entity_physics_component {
	vec3 velocity;
	float mass;
	float max_speed;
	btRigidBody *bt_rigid_body;
};

struct entity_light_component {
	light_type light_type;
	union {
		ambient_light ambient_light;
		directional_light directional_light;
		point_light point_light;
	};
};

struct entity_terrain_component {
	uint32 terrain_index;
	transform transform;
};

struct entity_modification {
	bool remove;
	bool remove_model_component;
	bool remove_collision_component;
	bool remove_physics_component;
	bool remove_light_component;
	bool remove_terrain_component;
	entity_info *info;
	transform *transform;
	entity_model_component *model_component;
	entity_collision_component *collision_component;
	entity_physics_component *physics_component;
	entity_light_component *light_component;
	entity_terrain_component *terrain_component;
};

struct entity_addition {
	uint32 flag;
	entity_info info;
	transform transform;
	entity_model_component *model_component;
	entity_collision_component *collision_component;
	entity_physics_component *physics_component;
	entity_light_component *light_component;
	entity_terrain_component *terrain_component;
	entity_addition *next;
};

struct primitive_render_data {
	uint32 primitive_index;
	uint32 uniform_region_buffer_offset;
};

struct mesh_render_data {
	uint32 mesh_index;
	uint32 uniform_region_buffer_offset;
	primitive_render_data *primitives;
};

struct model_render_data {
	uint32 model_index;
	uint32 uniform_region_buffer_offset;
	mesh_render_data *meshes;
	uint32 mesh_count;
};

struct terrain_render_data {
	uint32 terrain_index;
};

struct level_render_data {
	model_render_data *models;
	uint32 model_count;
	terrain_render_data *terrains;
	uint32 terrain_count;
};

struct level_persistant_data {
	uint32 vulkan_vertex_region_offset;
	uint32 vulkan_image_region_offset;
	uint32 vulkan_image_region_image_index;
	uint32 vulkan_combined_2d_image_samplers_index;
	uint32 vulkan_combined_cube_image_samplers_index;

	uint32 bound_vertex_region_buffer_offset;
	uint32 sphere_vertex_region_buffer_offset;
	uint32 cylinder_vertex_region_buffer_offset;
	uint32 hollow_circle_vertex_region_buffer_offset;
	uint32 torus_vertex_region_buffer_offset;
	uint32 terrain_vertex_region_buffer_offset;

	uint32 default_diffuse_map_descriptor_index;
	uint32 default_metallic_map_descriptor_index;
	uint32 default_roughness_map_descriptor_index;
	uint32 default_normal_map_descriptor_index;
	uint32 default_height_map_descriptor_index;
};

struct level {
	uint32 *entity_flags;
	entity_info *entity_infos;
	transform *entity_transforms;
	uint32 entity_count;

	entity_model_component *model_components;
	entity_collision_component *collision_components;
	entity_light_component *light_components;
	entity_physics_component *physics_components;
	entity_terrain_component *terrain_components;
	uint32 model_component_count;
	uint32 collision_component_count;
	uint32 light_component_count;
	uint32 physics_component_count;
	uint32 terrain_component_count;

	entity_modification *entity_modifications;
	entity_addition *entity_addition;

	uint32 player_entity_index;

	model *models;
	uint32 model_count;
	uint32 model_capacity;

	skybox *skyboxes;
	uint32 skybox_count;
	uint32 skybox_capacity;
	uint32 skybox_index;

	terrain *terrains;
	uint32 terrain_count;
	uint32 terrain_capacity;

	level_render_data render_data;

	memory_arena assets_memory_arena;
	memory_arena frame_memory_arena;
	memory_arena entity_memory_arenas[2];
	uint32 entity_memory_arena_index;

	level_persistant_data persistant_data;

	char json_file[256];
};

void initialize_level(level *level, vulkan *vulkan) {
	{ // memories
		level->assets_memory_arena.name = "level assets";
		level->assets_memory_arena.capacity = m_megabytes(64);
		level->assets_memory_arena.memory = allocate_virtual_memory(level->assets_memory_arena.capacity);
		m_assert(level->assets_memory_arena.memory);

		level->frame_memory_arena.name = "level frame";
		level->frame_memory_arena.capacity = m_megabytes(4);
		level->frame_memory_arena.memory = allocate_virtual_memory(level->frame_memory_arena.capacity);
		m_assert(level->frame_memory_arena.memory);

		for (uint32 i = 0; i < m_countof(level->entity_memory_arenas); i += 1) {
			level->entity_memory_arenas[i].name = "entity_components";
			level->entity_memory_arenas[i].capacity = m_megabytes(4);
			level->entity_memory_arenas[i].memory = allocate_virtual_memory(level->entity_memory_arenas[i].capacity);
			m_assert(level->entity_memory_arenas[i].memory);
		}

		level->model_count = 0;
		level->model_capacity = 1024;
		level->models = allocate_memory<struct model>(&level->assets_memory_arena, level->model_capacity);
		level->skybox_count = 0;
		level->skybox_capacity = 16;
		level->skyboxes = allocate_memory<struct skybox>(&level->assets_memory_arena, level->skybox_capacity);
		level->terrain_count = 0;
		level->terrain_capacity = 16;
		level->terrains = allocate_memory<struct terrain>(&level->assets_memory_arena, level->terrain_capacity);
	}
	{ // vertices
		level->persistant_data.bound_vertex_region_buffer_offset = append_vulkan_vertex_region(vulkan, bound_vertices, sizeof(bound_vertices), 12u);
		level->persistant_data.sphere_vertex_region_buffer_offset = append_vulkan_vertex_region(vulkan, sphere_vertices, sizeof(sphere_vertices), 12u);
		level->persistant_data.cylinder_vertex_region_buffer_offset = append_vulkan_vertex_region(vulkan, cylinder_vertices, sizeof(cylinder_vertices), 12u);
		level->persistant_data.hollow_circle_vertex_region_buffer_offset = append_vulkan_vertex_region(vulkan, hollow_circle_vertices, sizeof(hollow_circle_vertices), 12u);
		level->persistant_data.torus_vertex_region_buffer_offset = append_vulkan_vertex_region(vulkan, torus_vertices, sizeof(torus_vertices), 12u);

		uint32 vertices_size = level_terrain_resolution * level_terrain_resolution * 6 * sizeof(struct terrain_vertex);
		m_memory_arena_undo_allocations_at_scope_exit(&level->frame_memory_arena);
		terrain_vertex *vertices = allocate_memory<terrain_vertex>(&level->frame_memory_arena, 128 * 128 * 6);
		const float dp = level_terrain_size / level_terrain_resolution;
		const float duv = 1.0f / level_terrain_resolution;
		vec3 position = {-level_terrain_size / 2, 0, -level_terrain_size / 2};
		vec2 uv = {0, 0};
		terrain_vertex *vertices_ptr = vertices;
		for (uint32 i = 0; i < level_terrain_resolution; i += 1) {
			for (uint32 j = 0; j < level_terrain_resolution; j += 1) {
				vec3 pa = position;
				vec3 pb = position; pb.z += dp;
				vec3 pc = position; pc.x += dp;
				vec3 pd = position; pd.x += dp; pd.z += dp;
				vec2 ua = uv;
				vec2 ub = uv; ub.y += duv;
				vec2 uc = uv; uc.x += duv;
				vec2 ud = uv; ud.x += duv; ud.y += duv;
				vertices_ptr[0] = {pa, ua};
				vertices_ptr[1] = {pb, ub};
				vertices_ptr[2] = {pc, uc};
				vertices_ptr[3] = {pc, uc};
				vertices_ptr[4] = {pb, ub};
				vertices_ptr[5] = {pd, ud};
				position += {dp, 0, 0};
				uv += {duv, 0};
				vertices_ptr += 6;
			}
			position = {-32, 0, position.z + dp};
			uv = {0, uv.y + duv};
		}
		level->persistant_data.terrain_vertex_region_buffer_offset = append_vulkan_vertex_region(vulkan, (uint8 *)vertices, vertices_size, sizeof(struct terrain_vertex));
	}
	{ // images
		uint8 default_diffuse_map_data[] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
		uint8 default_metallic_map_data[] = {255, 255, 255, 255};
		uint8 default_roughness_map_data[] = {255, 255, 255, 255};
		uint8 default_normal_map_data[] = {128, 128, 255, 0, 128, 128, 255, 0, 128, 128, 255, 0, 128, 128, 255, 0};
		uint8 default_height_map_data[] = {0, 0, 0, 0};

		VkImageCreateInfo image_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
		image_info.extent = {2, 2, 1};
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		image_info.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkImageViewCreateInfo image_view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = image_info.format;
		image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_info.subresourceRange.levelCount = 1;
		image_view_info.subresourceRange.layerCount = 1;

		uint32 diffuse_map_index = append_vulkan_image_region(vulkan, image_info, image_view_info, default_diffuse_map_data, sizeof(default_diffuse_map_data), 1, 4);
		level->persistant_data.default_diffuse_map_descriptor_index = append_vulkan_combined_2d_image_samplers(vulkan, diffuse_map_index, vulkan->samplers.mipmap_samplers[0]);

		image_info.format = VK_FORMAT_R8_UNORM;
		image_view_info.format = image_info.format;
		uint32 metallic_map_index = append_vulkan_image_region(vulkan, image_info, image_view_info, default_metallic_map_data, sizeof(default_metallic_map_data), 1, 1);
		level->persistant_data.default_metallic_map_descriptor_index = append_vulkan_combined_2d_image_samplers(vulkan, metallic_map_index, vulkan->samplers.mipmap_samplers[0]);

		image_info.format = VK_FORMAT_R8_UNORM;
		image_view_info.format = image_info.format;
		uint32 roughness_map_index = append_vulkan_image_region(vulkan, image_info, image_view_info, default_roughness_map_data, sizeof(default_roughness_map_data), 1, 1);
		level->persistant_data.default_roughness_map_descriptor_index = append_vulkan_combined_2d_image_samplers(vulkan, roughness_map_index, vulkan->samplers.mipmap_samplers[0]);

		image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
		image_view_info.format = image_info.format;
		uint32 normal_map_index = append_vulkan_image_region(vulkan, image_info, image_view_info, default_normal_map_data, sizeof(default_normal_map_data), 1, 4);
		level->persistant_data.default_normal_map_descriptor_index = append_vulkan_combined_2d_image_samplers(vulkan, normal_map_index, vulkan->samplers.mipmap_samplers[0]);

		image_info.format = VK_FORMAT_R8_UNORM;
		image_view_info.format = image_info.format;
		uint32 height_map_index = append_vulkan_image_region(vulkan, image_info, image_view_info, default_height_map_data, sizeof(default_height_map_data), 1, 1);
		level->persistant_data.default_height_map_descriptor_index = append_vulkan_combined_2d_image_samplers(vulkan, height_map_index, vulkan->samplers.mipmap_samplers[0]);
	}
	level->persistant_data.vulkan_vertex_region_offset = vulkan->memory_regions.vertex_region_size;
	level->persistant_data.vulkan_image_region_offset = (uint32)vulkan->memory_regions.image_region_size;
	level->persistant_data.vulkan_image_region_image_index = vulkan->memory_regions.image_region_image_count;
	level->persistant_data.vulkan_combined_2d_image_samplers_index = vulkan->descriptors.combined_2d_image_sampler_count;
	level->persistant_data.vulkan_combined_cube_image_samplers_index = vulkan->descriptors.combined_cube_image_sampler_count;
}

entity_model_component *entity_get_model_component(level *level, uint32 entity_index) {
	m_assert(entity_index < level->entity_count);
	m_assert(level->entity_flags[entity_index] & entity_component_flag_model);
	uint32 index = 0;
	for (uint32 i = 0; i < entity_index; i += 1) {
		if (level->entity_flags[i] & entity_component_flag_model) {
			index += 1;
		}
	}
	return &level->model_components[index];
}

entity_collision_component *entity_get_collision_component(level *level, uint32 entity_index) {
	m_assert(entity_index < level->entity_count);
	m_assert(level->entity_flags[entity_index] & entity_component_flag_collision);
	uint32 index = 0;
	for (uint32 i = 0; i < entity_index; i += 1) {
		if (level->entity_flags[i] & entity_component_flag_collision) {
			index += 1;
		}
	}
	return &level->collision_components[index];
}

entity_light_component *entity_get_light_component(level *level, uint32 entity_index) {
	m_assert(entity_index < level->entity_count);
	m_assert(level->entity_flags[entity_index] & entity_component_flag_light);
	uint32 index = 0;
	for (uint32 i = 0; i < entity_index; i += 1) {
		if (level->entity_flags[i] & entity_component_flag_light) {
			index += 1;
		}
	}
	return &level->light_components[index];
}

entity_physics_component *entity_get_physics_component(level *level, uint32 entity_index) {
	m_assert(entity_index < level->entity_count);
	m_assert(level->entity_flags[entity_index] & entity_component_flag_physics);
	uint32 index = 0;
	for (uint32 i = 0; i < entity_index; i += 1) {
		if (level->entity_flags[i] & entity_component_flag_physics) {
			index += 1;
		}
	}
	return &level->physics_components[index];
}

entity_terrain_component *entity_get_terrain_component(level *level, uint32 entity_index) {
	m_assert(entity_index < level->entity_count);
	m_assert(level->entity_flags[entity_index] & entity_component_flag_terrain);
	uint32 index = 0;
	for (uint32 i = 0; i < entity_index; i += 1) {
		if (level->entity_flags[i] & entity_component_flag_terrain) {
			index += 1;
		}
	}
	return &level->terrain_components[index];
}

uint32 entity_component_get_entity_index(level *level, uint32 component_index, entity_component_flag entity_component_flag) {
	uint32 index = 0;
	for (uint32 i = 0; i < level->entity_count; i += 1) {
		if (level->entity_flags[i] & entity_component_flag) {
			if (index == component_index) {
				return i;
			}
			index += 1;
		}
	}
	m_assert(false);
	return UINT32_MAX;
}

void level_update_entity_components(level *level) {
	uint32 new_entity_count = level->entity_count;
	uint32 new_model_component_count = level->model_component_count;
	uint32 new_collision_component_count = level->collision_component_count;
	uint32 new_physics_component_count = level->physics_component_count;
	uint32 new_light_component_count = level->light_component_count;
	uint32 new_terrain_component_count = level->terrain_component_count;
	for (uint32 i = 0; i < level->entity_count; i += 1) {
		entity_modification *mod = &level->entity_modifications[i];
		uint32 entity_flag = level->entity_flags[i];
		if (mod->remove) {
			new_entity_count -= 1;
			if (entity_flag & entity_component_flag_model) {
				new_model_component_count -= 1;
			}
			if (entity_flag & entity_component_flag_collision) {
				new_collision_component_count -= 1;
			}
			if (entity_flag & entity_component_flag_physics) {
				new_physics_component_count -= 1;
			}
			if (entity_flag & entity_component_flag_light) {
				new_light_component_count -= 1;
			}
			if (entity_flag & entity_component_flag_terrain) {
				new_terrain_component_count -= 1;
			}
		}
		else {
			if (mod->remove_model_component) {
				new_model_component_count -= 1;
			}
			else if (mod->model_component && !(entity_flag & entity_component_flag_model)) {
				new_model_component_count += 1;
			}
			if (mod->remove_collision_component) {
				new_collision_component_count -= 1;
			}
			else if (mod->collision_component && !(entity_flag & entity_component_flag_collision)) {
				new_collision_component_count += 1;
			}
			if (mod->remove_physics_component) {
				new_physics_component_count -= 1;
			}
			else if (mod->physics_component && !(entity_flag & entity_component_flag_physics)) {
				new_physics_component_count += 1;
			}
			if (mod->remove_light_component) {
				new_light_component_count -= 1;
			}
			else if (mod->light_component && !(entity_flag & entity_component_flag_light)) {
				new_light_component_count += 1;
			}
			if (mod->remove_terrain_component) {
				new_terrain_component_count -= 1;
			}
			else if (mod->terrain_component && !(entity_flag & entity_component_flag_terrain)) {
				new_terrain_component_count += 1;
			}
		}
	}
	entity_addition *addition = level->entity_addition;
	while (addition) {
		new_entity_count += 1;
		if (addition->model_component) {
			new_model_component_count += 1;
		}
		if (addition->collision_component) {
			new_collision_component_count += 1;
		}
		if (addition->physics_component) {
			new_physics_component_count += 1;
		}
		if (addition->light_component) {
			new_light_component_count += 1;
		}
		if (addition->terrain_component) {
			new_terrain_component_count += 1;
		}
		addition = addition->next;
	}

	level->entity_memory_arena_index = (level->entity_memory_arena_index + 1) % 2;
	memory_arena *entity_components_memory_arena = &level->entity_memory_arenas[level->entity_memory_arena_index];
	entity_components_memory_arena->size = 0;

	uint32 *new_entity_flags = new_entity_count > 0 ? allocate_memory<uint32>(entity_components_memory_arena, new_entity_count) : nullptr;
	entity_info *new_entity_infos = new_entity_count > 0 ? allocate_memory<entity_info>(entity_components_memory_arena, new_entity_count) : nullptr;
	transform *new_entity_transforms = new_entity_count > 0 ? allocate_memory<transform>(entity_components_memory_arena, new_entity_count) : nullptr;
	entity_modification *new_entity_modifications = new_entity_count > 0 ? allocate_memory<entity_modification>(entity_components_memory_arena, new_entity_count) : nullptr;
	entity_model_component *new_model_components = new_model_component_count > 0 ? allocate_memory<entity_model_component>(entity_components_memory_arena, new_model_component_count) : nullptr;
	entity_collision_component *new_collision_components = new_collision_component_count > 0 ? allocate_memory<entity_collision_component>(entity_components_memory_arena, new_collision_component_count) : nullptr;
	entity_physics_component *new_physics_components = new_physics_component_count > 0 ? allocate_memory<entity_physics_component>(entity_components_memory_arena, new_physics_component_count) : nullptr;
	entity_light_component *new_light_components = new_light_component_count > 0 ? allocate_memory<entity_light_component>(entity_components_memory_arena, new_light_component_count) : nullptr;
	entity_terrain_component *new_terrain_components = new_terrain_component_count > 0 ? allocate_memory<entity_terrain_component>(entity_components_memory_arena, new_terrain_component_count) : nullptr;

	uint32 entity_index = 0;
	uint32 model_component_index = 0;
	uint32 collision_component_index = 0;
	uint32 physics_component_index = 0;
	uint32 light_component_index = 0;
	uint32 terrain_component_index = 0;
	for (uint32 i = 0; i < level->entity_count; i += 1) {
		entity_modification *mod = &level->entity_modifications[i];
		if (!mod->remove) {
			uint32 entity_flags = level->entity_flags[i];
			if (mod->remove_model_component) {
				entity_flags &= ~entity_component_flag_model;
			}
			else if (entity_flags & entity_component_flag_model || mod->model_component) {
				entity_flags |= entity_component_flag_model;
				new_model_components[model_component_index++] = mod->model_component ? *mod->model_component : *entity_get_model_component(level, i);
			}
			if (mod->remove_collision_component) {
				entity_flags &= ~entity_component_flag_collision;
			}
			else if (entity_flags & entity_component_flag_collision || mod->collision_component) {
				entity_flags |= entity_component_flag_collision;
				new_collision_components[collision_component_index++] = mod->collision_component ? *mod->collision_component : *entity_get_collision_component(level, i);
			}
			if (mod->remove_physics_component) {
				entity_flags &= ~entity_component_flag_physics;
			}
			else if (entity_flags & entity_component_flag_physics || mod->physics_component) {
				entity_flags |= entity_component_flag_physics;
				new_physics_components[physics_component_index++] = mod->physics_component ? *mod->physics_component : *entity_get_physics_component(level, i);
			}
			if (mod->remove_light_component) {
				entity_flags &= ~entity_component_flag_light;
			}
			else if (entity_flags & entity_component_flag_light || mod->light_component) {
				entity_flags |= entity_component_flag_light;
				new_light_components[light_component_index++] = mod->light_component ? *mod->light_component : *entity_get_light_component(level, i);
			}
			if (mod->remove_terrain_component) {
				entity_flags &= ~entity_component_flag_terrain;
			}
			else if (entity_flags & entity_component_flag_terrain || mod->terrain_component) {
				entity_flags |= entity_component_flag_terrain;
				new_terrain_components[terrain_component_index++] = mod->terrain_component ? *mod->terrain_component : *entity_get_terrain_component(level, i);
			}
			new_entity_flags[entity_index] = entity_flags;
			new_entity_infos[entity_index] = mod->info ? *mod->info : level->entity_infos[i];
			new_entity_transforms[entity_index] = mod->transform ? *mod->transform : level->entity_transforms[i];
			entity_index += 1;
		}
	}
	addition = level->entity_addition;
	while (addition) {
		new_entity_flags[entity_index] = addition->flag;
		new_entity_infos[entity_index] = addition->info;
		new_entity_transforms[entity_index] = addition->transform;
		if (addition->model_component) {
			new_model_components[model_component_index++] = *addition->model_component;
		}
		if (addition->collision_component) {
			new_collision_components[collision_component_index++] = *addition->collision_component;
		}
		if (addition->physics_component) {
			new_physics_components[physics_component_index++] = *addition->physics_component;
		}
		if (addition->light_component) {
			new_light_components[light_component_index++] = *addition->light_component;
		}
		if (addition->terrain_component) {
			new_terrain_components[terrain_component_index++] = *addition->terrain_component;
		}
		entity_index += 1;
		addition = addition->next;
	}

	level->entity_flags = new_entity_flags;
	level->entity_infos = new_entity_infos;
	level->entity_transforms = new_entity_transforms;
	level->entity_count = new_entity_count;

	level->entity_modifications = new_entity_modifications;
	level->entity_addition = nullptr;

	level->model_components = new_model_components;
	level->collision_components = new_collision_components;
	level->physics_components = new_physics_components;
	level->light_components = new_light_components;
	level->terrain_components = new_terrain_components;

	level->model_component_count = new_model_component_count;
	level->collision_component_count = new_collision_component_count;
	level->physics_component_count = new_physics_component_count;
	level->light_component_count = new_light_component_count;
	level->terrain_component_count = new_terrain_component_count;
}

uint32 level_get_model_index(level *level, const char *gpk_file) {
	for (uint32 i = 0; i < level->model_count; i += 1) {
		if (!strcmp(level->models[i].gpk_file, gpk_file)) {
			return i;
		}
	}
	return UINT32_MAX;
}

uint32 level_get_terrain_index(level *level, const char *gpk_file) {
	for (uint32 i = 0; i < level->terrain_count; i += 1) {
		if (!strcmp(level->terrains[i].gpk_file, gpk_file)) {
			return i;
		}
	}
	return UINT32_MAX;
}

uint32 level_add_gpk_model(level *level, vulkan *vulkan, const char *gpk_file, bool store_vertices = false) {
	m_assert(level->model_count < level->model_capacity);
	for (uint32 i = 0; i < level->model_count; i += 1) {
		m_assert(strcmp(level->models[i].gpk_file, gpk_file));
	}

	file_mapping gpk_file_mapping = {};
	m_assert(open_file_mapping(gpk_file, &gpk_file_mapping));
	m_scope_exit(close_file_mapping(&gpk_file_mapping));

	model *model = &level->models[level->model_count];
	gpk_model *gpk_model = (struct gpk_model *)gpk_file_mapping.ptr;
	m_assert(!strcmp(gpk_model->format_str, m_gpk_model_format_str));
	snprintf(model->gpk_file, sizeof(model->gpk_file), "%s", gpk_file);
	model->scene_count = gpk_model->scene_count;
	model->node_count = gpk_model->node_count;
	model->mesh_count = gpk_model->mesh_count;
	model->skin_count = gpk_model->skin_count;
	model->animation_count = gpk_model->animation_count;
	model->material_count = gpk_model->material_count;
	model->image_count = gpk_model->image_count;
	model->scenes = allocate_memory<struct model_scene>(&level->assets_memory_arena, model->scene_count);
	model->nodes = allocate_memory<struct model_node>(&level->assets_memory_arena, model->node_count);
	model->meshes = allocate_memory<struct model_mesh>(&level->assets_memory_arena, model->mesh_count);
	if (model->skin_count > 0) {
		model->skins = allocate_memory<struct model_skin>(&level->assets_memory_arena, model->skin_count);
	}
	if (model->animation_count > 0) {
		model->animations = allocate_memory<struct model_animation>(&level->assets_memory_arena, model->animation_count);
	}
	if (model->material_count > 0) {
		model->materials = allocate_memory<struct model_material>(&level->assets_memory_arena, model->material_count);
	}
	if (model->image_count > 0) {
		model->images = allocate_memory<struct model_image>(&level->assets_memory_arena, model->image_count);
	}
	for (uint32 i = 0; i < model->scene_count; i += 1) {
		gpk_model_scene *gpk_model_scene = ((struct gpk_model_scene*)(gpk_file_mapping.ptr + gpk_model->scene_offset)) + i;
		model_scene *model_scene = &model->scenes[i];

		array_copy(model_scene->name, gpk_model_scene->name);
		array_copy(model_scene->node_indices, gpk_model_scene->node_indices);
		model_scene->node_index_count = gpk_model_scene->node_index_count;
	}
	for (uint32 i = 0; i < model->node_count; i += 1) {
		gpk_model_node *gpk_model_node = ((struct gpk_model_node*)(gpk_file_mapping.ptr + gpk_model->node_offset)) + i;
		model_node *model_node = &model->nodes[i];

		model_node->mesh_index = gpk_model_node->mesh_index;
		model_node->local_transform = gpk_model_node->local_transform;
		model_node->local_transform_mat = gpk_model_node->local_transform_mat;
		array_copy(model_node->children, gpk_model_node->children);
		model_node->child_count = gpk_model_node->child_count;
	}
	for (uint32 i = 0; i < model->mesh_count; i += 1) {
		gpk_model_mesh *gpk_model_mesh = ((struct gpk_model_mesh*)(gpk_file_mapping.ptr + gpk_model->mesh_offset)) + i;
		model_mesh *model_mesh = &model->meshes[i];
		array_copy(model_mesh->name, gpk_model_mesh->name);
		model_mesh->primitive_count = gpk_model_mesh->primitive_count;
		model_mesh->primitives = allocate_memory<struct model_mesh_primitive>(&level->assets_memory_arena, model_mesh->primitive_count);
		for (uint32 i = 0; i < model_mesh->primitive_count; i += 1) {
			gpk_model_mesh_primitive *gpk_primitive = ((gpk_model_mesh_primitive *)(gpk_file_mapping.ptr + gpk_model_mesh->primitive_offset)) + i;
			model_mesh_primitive *primitive = &model_mesh->primitives[i];

			primitive->material_index = gpk_primitive->material_index;
			primitive->has_joints = gpk_primitive->has_joints;

			uint32 indices_size = gpk_primitive->index_count * sizeof(uint16);
			uint32 vertices_size = gpk_primitive->vertex_count * sizeof(struct gpk_model_vertex);
			uint8 *indices_data = gpk_file_mapping.ptr + gpk_primitive->indices_offset;
			uint8 *vertices_data = gpk_file_mapping.ptr + gpk_primitive->vertices_offset;
			
			primitive->index_count = gpk_primitive->index_count;
			primitive->index_buffer_offset = append_vulkan_vertex_region(vulkan, indices_data, indices_size, sizeof(uint16));

			primitive->vertex_count = gpk_primitive->vertex_count;
			primitive->vertex_buffer_offset = append_vulkan_vertex_region(vulkan, vertices_data, vertices_size, sizeof(struct gpk_model_vertex));

			if (store_vertices) {
				primitive->indices_data = allocate_memory<uint8>(&level->assets_memory_arena, indices_size);
				primitive->vertices_data = allocate_memory<uint8>(&level->assets_memory_arena, vertices_size);
				memcpy(primitive->indices_data, indices_data, indices_size);
				memcpy(primitive->vertices_data, vertices_data, vertices_size);
			}
		}
	}
	for (uint32 i = 0; i < model->skin_count; i += 1) {
		gpk_model_skin *gpk_model_skin = ((struct gpk_model_skin*)(gpk_file_mapping.ptr + gpk_model->skin_offset)) + i;
		model_skin *model_skin = &model->skins[i];
		array_copy(model_skin->name, gpk_model_skin->name);
		model_skin->joint_count = gpk_model_skin->joint_count;
		m_assert(model_skin->joint_count > 0);
		model_skin->joints = allocate_memory<struct model_joint>(&level->assets_memory_arena, model_skin->joint_count);
		gpk_model_joint *gpk_joints = (gpk_model_joint *)(gpk_file_mapping.ptr + gpk_model_skin->joints_offset);
		for (uint32 i = 0; i < model_skin->joint_count; i += 1) {
			model_skin->joints[i].node_index = gpk_joints[i].node_index;
			model_skin->joints[i].inverse_bind_mat = gpk_joints[i].inverse_bind_mat;
		}
	}
	for (uint32 i = 0; i < model->animation_count; i += 1) {
		gpk_model_animation *gpk_animation = ((struct gpk_model_animation*)(gpk_file_mapping.ptr + gpk_model->animation_offset)) + i;
		model_animation *animation = &model->animations[i];
		array_copy(animation->name, gpk_animation->name);
		animation->channel_count = gpk_animation->channel_count;
		animation->channels = allocate_memory<struct model_animation_channel>(&level->assets_memory_arena, animation->channel_count);
		for (uint32 i = 0; i < animation->channel_count; i += 1) {
			gpk_model_animation_channel *gpk_channel = ((struct gpk_model_animation_channel*)(gpk_file_mapping.ptr + gpk_animation->channel_offset)) + i;
			model_animation_channel *channel = &animation->channels[i];
			channel->node_index = gpk_channel->node_index;
			channel->channel_type = gpk_channel->channel_type;
			channel->sampler_index = gpk_channel->sampler_index;
		}
		animation->sampler_count = gpk_animation->sampler_count;
		animation->samplers = allocate_memory<model_animation_sampler>(&level->assets_memory_arena, animation->sampler_count);
		for (uint32 i = 0; i < animation->sampler_count; i += 1) {
			gpk_model_animation_sampler *gpk_sampler = ((struct gpk_model_animation_sampler*)(gpk_file_mapping.ptr + gpk_animation->sampler_offset)) + i;
			model_animation_sampler *sampler = &animation->samplers[i];
			sampler->interpolation_type = gpk_sampler->interpolation_type;
			sampler->key_frame_count = gpk_sampler->key_frame_count;
			sampler->key_frames = allocate_memory<struct model_animation_key_frame>(&level->assets_memory_arena, sampler->key_frame_count);
			for (uint32 i = 0; i < sampler->key_frame_count; i += 1) {
				gpk_model_animation_key_frame *gpk_key_frame = ((struct gpk_model_animation_key_frame *)(gpk_file_mapping.ptr + gpk_sampler->key_frame_offset)) + i;
				sampler->key_frames[i].time = gpk_key_frame->time;
				sampler->key_frames[i].transform_data = gpk_key_frame->transform_data;
			}
		}
	}
	for (uint32 i = 0; i < model->image_count; i += 1) {
		gpk_model_image *gpk_model_image = ((struct gpk_model_image*)(gpk_file_mapping.ptr + gpk_model->image_offset)) + i;
		VkImageCreateInfo image_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.format = (VkFormat)gpk_model_image->format;
		image_info.extent = {gpk_model_image->width, gpk_model_image->height, 1};
		image_info.mipLevels = gpk_model_image->mipmap_count;
		image_info.arrayLayers = gpk_model_image->layer_count;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		image_info.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkImageViewCreateInfo image_view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = image_info.format;
		image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_info.subresourceRange.levelCount = image_info.mipLevels;
		image_view_info.subresourceRange.layerCount = image_info.arrayLayers;
		uint8 *image_data = gpk_file_mapping.ptr + gpk_model_image->data_offset;
		model->images[i].index = append_vulkan_image_region(vulkan, image_info, image_view_info, image_data, gpk_model_image->size, gpk_model_image->format_block_dimension, gpk_model_image->format_block_size);
	}
	for (uint32 i = 0; i < model->material_count; i += 1) {
		gpk_model_material *gpk_model_material = ((struct gpk_model_material*)(gpk_file_mapping.ptr + gpk_model->material_offset)) + i;
		model_material *model_material = &model->materials[i];
		array_copy(model_material->name, gpk_model_material->name);
		if (gpk_model_material->diffuse_image_index < gpk_model->image_count) {
			uint32 image_index = model->images[gpk_model_material->diffuse_image_index].index;
			vulkan_image &image = vulkan->memory_regions.image_region_images[image_index];
			model_material->diffuse_map_descriptor_index = append_vulkan_combined_2d_image_samplers(vulkan, image_index, vulkan->samplers.mipmap_samplers[image.mipmap_count]);
		} else {
			model_material->diffuse_map_descriptor_index = level->persistant_data.default_diffuse_map_descriptor_index;
		}
		if (gpk_model_material->metallic_image_index < gpk_model->image_count) {
			uint32 image_index = model->images[gpk_model_material->metallic_image_index].index;
			vulkan_image &image = vulkan->memory_regions.image_region_images[image_index];
			model_material->metallic_map_descriptor_index = append_vulkan_combined_2d_image_samplers(vulkan, image_index, vulkan->samplers.mipmap_samplers[image.mipmap_count]);
		} else {
			model_material->metallic_map_descriptor_index = level->persistant_data.default_metallic_map_descriptor_index;
		}
		if (gpk_model_material->roughness_image_index < gpk_model->image_count) {
			uint32 image_index = model->images[gpk_model_material->roughness_image_index].index;
			vulkan_image &image = vulkan->memory_regions.image_region_images[image_index];
			model_material->roughness_map_descriptor_index = append_vulkan_combined_2d_image_samplers(vulkan, image_index, vulkan->samplers.mipmap_samplers[image.mipmap_count]);
		} else {
			model_material->roughness_map_descriptor_index = level->persistant_data.default_roughness_map_descriptor_index;
		}
		if (gpk_model_material->normal_image_index < gpk_model->image_count) {
			uint32 image_index = model->images[gpk_model_material->normal_image_index].index;
			vulkan_image &image = vulkan->memory_regions.image_region_images[image_index];
			model_material->normal_map_descriptor_index = append_vulkan_combined_2d_image_samplers(vulkan, image_index, vulkan->samplers.mipmap_samplers[image.mipmap_count]);
		} else {
			model_material->normal_map_descriptor_index = level->persistant_data.default_normal_map_descriptor_index;
		}
		model_material->diffuse_factor = gpk_model_material->diffuse_factor;
		model_material->metallic_factor = gpk_model_material->metallic_factor;
		model_material->roughness_factor = gpk_model_material->roughness_factor;
	}
	return level->model_count++;
}

void level_add_skybox(level *level, vulkan *vulkan, const char *gpk_file) {
	m_assert(level->skybox_count < level->skybox_capacity);
	skybox *skybox = &level->skyboxes[level->skybox_count++];
	m_assert(strlen(gpk_file) < sizeof(skybox->gpk_file));
	strcpy(skybox->gpk_file, gpk_file);

	file_mapping gpk_file_mapping = {};
	m_assert(open_file_mapping(gpk_file, &gpk_file_mapping));
	m_scope_exit(close_file_mapping(&gpk_file_mapping));
	gpk_skybox *gpk_skybox = (struct gpk_skybox *)gpk_file_mapping.ptr;
	m_assert(!strcmp(gpk_skybox->format_str, m_gpk_skybox_format_str));

	VkImageCreateInfo image_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.format = (VkFormat)gpk_skybox->cubemap_format;
	image_info.extent = {gpk_skybox->cubemap_width, gpk_skybox->cubemap_height, 1};
	image_info.mipLevels = gpk_skybox->cubemap_mipmap_count;
	image_info.arrayLayers = gpk_skybox->cubemap_layer_count;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	image_info.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkImageViewCreateInfo image_view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	image_view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	image_view_info.format = image_info.format;
	image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_view_info.subresourceRange.levelCount = gpk_skybox->cubemap_mipmap_count;
	image_view_info.subresourceRange.layerCount = gpk_skybox->cubemap_layer_count;
	uint8 *cubemap_ptr = (uint8 *)gpk_skybox + gpk_skybox->cubemap_offset;
	uint32 cubemap_index = append_vulkan_image_region(vulkan, image_info, image_view_info, cubemap_ptr, gpk_skybox->cubemap_size, gpk_skybox->cubemap_format_block_dimension, gpk_skybox->cubemap_format_block_size);
	skybox->cubemap_descriptor_index = append_vulkan_combined_cube_image_samplers(vulkan, cubemap_index, vulkan->samplers.skybox_cubemap_sampler);
}

void level_add_terrain(level *level, vulkan *vulkan, const char *gpk_file) {
	m_assert(level->terrain_count < level->terrain_capacity);
	terrain *terrain = &level->terrains[level->terrain_count++];
	m_assert(strlen(gpk_file) < sizeof(terrain->gpk_file));
	strcpy(terrain->gpk_file, gpk_file);

	file_mapping gpk_file_mapping = {};
	m_assert(open_file_mapping(gpk_file, &gpk_file_mapping));
	m_scope_exit(close_file_mapping(&gpk_file_mapping));
	gpk_terrain *gpk_terrain = (struct gpk_terrain *)gpk_file_mapping.ptr;
	m_assert(!strcmp(gpk_terrain->format_str, m_gpk_terrain_format_str));
	{
		m_assert(gpk_terrain->height_map_width == gpk_terrain->height_map_height);
		m_assert(is_pow_2(gpk_terrain->height_map_width));
		VkImageCreateInfo image_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.format = VK_FORMAT_R16_SNORM;
		image_info.extent = {gpk_terrain->height_map_width, gpk_terrain->height_map_height, 1};
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		image_info.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkImageViewCreateInfo image_view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = image_info.format;
		image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_info.subresourceRange.levelCount = 1;
		image_view_info.subresourceRange.layerCount = 1;
		uint8 *height_map_ptr = gpk_file_mapping.ptr + gpk_terrain->height_map_offset;
		uint32 image_index = append_vulkan_image_region(vulkan, image_info, image_view_info, height_map_ptr, gpk_terrain->height_map_size, 1, 2);
		terrain->height_map_descriptor_index = append_vulkan_combined_2d_image_samplers(vulkan, image_index, vulkan->samplers.terrain_texture_sampler);
	}
	{
		m_assert(gpk_terrain->diffuse_map_width == gpk_terrain->diffuse_map_height);
		m_assert(is_pow_2(gpk_terrain->diffuse_map_width));
		VkImageCreateInfo image_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
		image_info.extent = {gpk_terrain->diffuse_map_width, gpk_terrain->diffuse_map_height, 1};
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		image_info.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkImageViewCreateInfo image_view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = image_info.format;
		image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_info.subresourceRange.levelCount = 1;
		image_view_info.subresourceRange.layerCount = 1;
		uint8 *diffuse_map_ptr = gpk_file_mapping.ptr + gpk_terrain->diffuse_map_offset;
		uint32 image_index = append_vulkan_image_region(vulkan, image_info, image_view_info, diffuse_map_ptr, gpk_terrain->diffuse_map_size, 1, 4);
		terrain->diffuse_map_descriptor_index = append_vulkan_combined_2d_image_samplers(vulkan, image_index, vulkan->samplers.terrain_texture_sampler);
	}
	{
		uint8 *height_map_ptr = gpk_file_mapping.ptr + gpk_terrain->height_map_offset;
		terrain->height_map_data = allocate_memory<uint8>(&level->assets_memory_arena, gpk_terrain->height_map_size);
		memcpy(terrain->height_map_data, height_map_ptr, gpk_terrain->height_map_size);
		terrain->bt_terrain_shape = new btHeightfieldTerrainShape(level_terrain_resolution, level_terrain_resolution, terrain->height_map_data, terrain_height_map_scale / (float)INT16_MAX, -terrain_height_map_scale, terrain_height_map_scale, 1, PHY_SHORT, false);
		float scaling = level_terrain_size / level_terrain_resolution;
		terrain->bt_terrain_shape->setLocalScaling(btVector3(scaling, 1, scaling));
	}
}

template <typename F>
void level_read_json(level *level, vulkan *vulkan, const char *level_json_file, F extra_read, bool store_vertices) {
	file_mapping level_json_file_mapping = {};
	m_assert(open_file_mapping(level_json_file, &level_json_file_mapping));
	nlohmann::json json;
	try {
		json = nlohmann::json::parse(level_json_file_mapping.ptr, level_json_file_mapping.ptr + level_json_file_mapping.size);
	} catch (nlohmann::json::exception &e) {
		printf("%s\n", e.what());
		m_assert(false);
	}
	close_file_mapping(&level_json_file_mapping);
	{
		level->render_data = {};
		level->entity_memory_arenas[0].size = 0;
		level->entity_memory_arenas[1].size = 0;
		level->entity_memory_arena_index = 0;
		level->assets_memory_arena.size = 0;
		snprintf(level->json_file, sizeof(level->json_file), "%s", level_json_file);
		level->model_count = 0;
		level->model_capacity = 1024;
		level->models = allocate_memory<struct model>(&level->assets_memory_arena, level->model_capacity);
		level->skybox_count = 0;
		level->skybox_capacity = 16;
		level->skyboxes = allocate_memory<struct skybox>(&level->assets_memory_arena, level->skybox_capacity);
		level->terrain_count = 0;
		level->terrain_capacity = 16;
		level->terrains = allocate_memory<struct terrain>(&level->assets_memory_arena, level->terrain_capacity);
	}
	{ // models, skyboxes, terrains
		std::vector<std::string> model_gpk_files = json["models"];
		for (auto &m : model_gpk_files) {
			level_add_gpk_model(level, vulkan, m.c_str(), store_vertices);
		}
		std::vector<std::string> skybox_gpk_files = json["skyboxes"];
		for (auto &s : skybox_gpk_files) {
			level_add_skybox(level, vulkan, s.c_str());
		}
		level->skybox_index = json["skybox_index"];
		std::vector<std::string> terrain_gpk_files = json["terrains"];
		for (auto &s : terrain_gpk_files) {
			level_add_terrain(level, vulkan, s.c_str());
		}
	}
	{ // entities
		auto &entities_json = json["entities"];
		level->entity_count = (uint32)entities_json.size();
		level->model_component_count = 0;
		level->collision_component_count = 0;
		level->physics_component_count = 0;
		level->light_component_count = 0;
		level->terrain_component_count = 0;
		for (auto &entity : entities_json) {
			if (entity.find("model_component") != entity.end()) {
				level->model_component_count += 1;
			}
			if (entity.find("collision_component") != entity.end()) {
				level->collision_component_count += 1;
			}
			if (entity.find("physics_component") != entity.end()) {
				level->physics_component_count += 1;
			}
			if (entity.find("light_component") != entity.end()) {
				level->light_component_count += 1;
			}
			if (entity.find("terrain_component") != entity.end()) {
				level->terrain_component_count += 1;
			}
		}

		level->entity_memory_arena_index = 0;
		memory_arena *memory_arena = &level->entity_memory_arenas[0];
		if (level->entity_count > 0) {
			level->entity_flags = allocate_memory<uint32>(memory_arena, level->entity_count);
			level->entity_infos = allocate_memory<entity_info>(memory_arena, level->entity_count);
			level->entity_transforms = allocate_memory<transform>(memory_arena, level->entity_count);
			level->entity_modifications = allocate_memory<entity_modification>(memory_arena, level->entity_count);
		}
		if (level->model_component_count > 0) {
			level->model_components = allocate_memory<entity_model_component>(memory_arena, level->model_component_count);
		}
		if (level->collision_component_count > 0) {
			level->collision_components = allocate_memory<entity_collision_component>(memory_arena, level->collision_component_count);
		}
		if (level->physics_component_count > 0) {
			level->physics_components = allocate_memory<entity_physics_component>(memory_arena, level->physics_component_count);
		}
		if (level->light_component_count > 0) {
			level->light_components = allocate_memory<entity_light_component>(memory_arena, level->light_component_count);
		}
		if (level->terrain_component_count > 0) {
			level->terrain_components = allocate_memory<entity_terrain_component>(memory_arena, level->terrain_component_count);
		}

		auto read_entity_info = [](const nlohmann::json &json, entity_info *info) {
			const std::string &name = json["name"];
			m_assert(name.length() < sizeof(info->name));
			strcpy(info->name, name.c_str());
		};
		auto read_transform = [](const nlohmann::json &json, transform *transform) {
			std::array<float, 3> scale = json["scale"];
			std::array<float, 4> rotate = json["rotate"];
			std::array<float, 3> translate = json["translate"];
			transform->scale = {scale[0], scale[1], scale[2]};
			transform->rotate = {rotate[0], rotate[1], rotate[2], rotate[3]};
			transform->translate = {translate[0], translate[1], translate[2]};
		};
    auto read_model_component = [level, read_transform](const nlohmann::json &json, entity_model_component *model_component) {
			const std::string &gpk_file = json["gpk_file"];
			model_component->model_index = level_get_model_index(level, gpk_file.c_str());
			auto adjustment_transform_field = json.find("adjustment_transform");
			if (adjustment_transform_field != json.end()) {
				read_transform(*adjustment_transform_field, &model_component->adjustment_transform);
			}
			else {
				model_component->adjustment_transform = transform_identity();
			}
		};
    auto read_collision_component = [level](const nlohmann::json &json, entity_collision_component *collision_component) {
			const std::string &shape = json["shape"];
			if (shape == "sphere") {
				collision_component->shape = collision_shape_sphere;
				collision_component->sphere.radius = json["radius"];
			}
			else if (shape == "capsule") {
				collision_component->shape = collision_shape_capsule;
				collision_component->capsule.height = json["height"];
				collision_component->capsule.radius = json["radius"];
			}
			else if (shape == "box") {
				collision_component->shape = collision_shape_box;
				std::array<float, 3> size = json["size"];
				collision_component->box.size = {size[0], size[1], size[2]};
			}
			else if (shape == "terrain") {
				collision_component->shape = collision_shape_terrain;
				collision_component->terrain_index = UINT32_MAX;
				const std::string &gpk_file = json["gpk_file"];
				for (uint32 i = 0; i < level->terrain_count; i += 1) {
					if (!strcmp(gpk_file.c_str(), level->terrains[i].gpk_file)) {
						collision_component->terrain_index = i;
						break;
					}
				}
				m_assert(collision_component->terrain_index != UINT32_MAX);
			}
			else {
				m_assert(false);
			}
		};
		auto read_physics_component = [](const nlohmann::json &json, entity_physics_component *physics_component) {
			auto velocity_field = json.find("velocity");
			if (velocity_field != json.end()) {
				std::array<float, 3> velocity = *velocity_field;
				physics_component->velocity = {velocity[0], velocity[1], velocity[2]};
			}
			auto mass_field = json.find("mass");
			if (mass_field != json.end()) {
				physics_component->mass = *mass_field;
			}
			auto max_speed_field = json.find("max_speed");
			if (max_speed_field != json.end()) {
				physics_component->max_speed = *max_speed_field;
			}
		};
		auto read_light_component = [](const nlohmann::json &json, entity_light_component *light_component) {
			const std::string &light_type = json["light_type"];
			if (light_type == "ambient") {
				light_component->light_type = light_type_ambient;
				std::array<float, 3> color = json["color"];
				light_component->ambient_light.color = {color[0], color[1], color[2]};
			}
			else if (light_type == "directional") {
				light_component->light_type = light_type_directional;
				std::array<float, 3> color = json["color"];
				std::array<float, 3> direction = json["direction"];
				light_component->directional_light.color = {color[0], color[1], color[2]};
				light_component->directional_light.direction = {direction[0], direction[1], direction[2]};
			}
			else if (light_type == "point") {
				light_component->light_type = light_type_point;
				std::array<float, 3> color = json["color"];
				std::array<float, 3> position = json["position"];
				light_component->point_light.color = {color[0], color[1], color[2]};
				light_component->point_light.position = {position[0], position[1], position[2]};
				light_component->point_light.attenuation = json["attenuation"];
			}
			else {
				m_assert(false);
			}
		};
		auto read_terrain_component = [level, read_transform](const nlohmann::json &json, entity_terrain_component *terrain_component) {
			const std::string &gpk_file = json["gpk_file"];
			terrain_component->terrain_index = level_get_terrain_index(level, gpk_file.c_str());
			auto transform_field = json.find("transform");
			if (transform_field != json.end()) {
				read_transform(*transform_field, &terrain_component->transform);
			}
			else {
				terrain_component->transform = transform_identity();
			}
		};

		uint32 model_component_index = 0;
		uint32 collision_component_index = 0;
		uint32 physics_component_index = 0;
		uint32 light_component_index = 0;
		uint32 terrain_component_index = 0;
		for (uint32 i = 0; i < level->entity_count; i += 1) {
			auto &entity_json = entities_json[i];
			uint32 &entity_flags = level->entity_flags[i];
			entity_info &entity_info = level->entity_infos[i];
			transform &entity_transform = level->entity_transforms[i];
			entity_flags = 0;
			read_entity_info(entity_json, &entity_info);
			read_transform(entity_json["transform"], &entity_transform);

			auto model_component_field = entity_json.find("model_component");
			if (model_component_field != entity_json.end()) {
				entity_flags |= entity_component_flag_model;
				read_model_component(*model_component_field, &level->model_components[model_component_index++]);
			}
			auto collision_component_field = entity_json.find("collision_component");
			if (collision_component_field != entity_json.end()) {
				entity_flags |= entity_component_flag_collision;
				read_collision_component(*collision_component_field, &level->collision_components[collision_component_index++]);
			}
			auto physics_component_field = entity_json.find("physics_component");
			if (physics_component_field != entity_json.end()) {
				entity_flags |= entity_component_flag_physics;
				read_physics_component(*physics_component_field, &level->physics_components[physics_component_index++]);
			}
			auto light_component_field = entity_json.find("light_component");
			if (light_component_field != entity_json.end()) {
				entity_flags |= entity_component_flag_light;
				read_light_component(*light_component_field, &level->light_components[light_component_index++]);
			}
			auto terrain_component_field = entity_json.find("terrain_component");
			if (terrain_component_field != entity_json.end()) {
				entity_flags |= entity_component_flag_terrain;
				read_terrain_component(*terrain_component_field, &level->terrain_components[terrain_component_index++]);
			}
			if (collision_component_field != entity_json.end()) {
				entity_collision_component *collision_component = &level->collision_components[collision_component_index - 1];
				btCollisionObject *collision_object = nullptr;
				if (physics_component_field != entity_json.end()) {
					entity_physics_component *physics_component = &level->physics_components[physics_component_index - 1];
					btRigidBody *rigid_body = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(physics_component->mass, nullptr, nullptr));
					rigid_body->setLinearVelocity(btVector3(physics_component->velocity.x, physics_component->velocity.y, physics_component->velocity.z));
					physics_component->bt_rigid_body = rigid_body;
					collision_object = rigid_body;
				}
				else {
					collision_object = new btCollisionObject();
					collision_component->bt_collision_object = collision_object;
				}
				btQuaternion rotate(entity_transform.rotate.x, entity_transform.rotate.y, entity_transform.rotate.z, entity_transform.rotate.w);
				btVector3 translate(entity_transform.translate.x, entity_transform.translate.y, entity_transform.translate.z);
				collision_object->setWorldTransform(btTransform(rotate, translate));
				if (collision_component->shape == collision_shape_sphere) {
					collision_object->setCollisionShape(new btSphereShape(collision_component->sphere.radius));
				}
				else if (collision_component->shape == collision_shape_capsule) {
					collision_object->setCollisionShape(new btCapsuleShape(collision_component->capsule.radius, collision_component->capsule.height));
				}
				else if (collision_component->shape == collision_shape_box) {
					collision_object->setCollisionShape(new btBoxShape(btVector3(collision_component->box.size.x / 2, collision_component->box.size.y / 2, collision_component->box.size.z / 2)));
				}
				else if (collision_component->shape == collision_shape_terrain) {
					m_assert(collision_component->terrain_index < level->terrain_count);
					collision_object->setCollisionShape(level->terrains[collision_component->terrain_index].bt_terrain_shape);
				}
			}
			else if (physics_component_field != entity_json.end()) {
				entity_physics_component *physics_component = &level->physics_components[physics_component_index - 1];
				btRigidBody *rigid_body = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(physics_component->mass, nullptr, nullptr));
				btQuaternion rotate(entity_transform.rotate.x, entity_transform.rotate.y, entity_transform.rotate.z, entity_transform.rotate.w);
				btVector3 translate(entity_transform.translate.x, entity_transform.translate.y, entity_transform.translate.z);
				rigid_body->setWorldTransform(btTransform(rotate, translate));
				rigid_body->setLinearVelocity(btVector3(physics_component->velocity.x, physics_component->velocity.y, physics_component->velocity.z));
				physics_component->bt_rigid_body = rigid_body;
			}
		}
	}
  { // player
		level->player_entity_index = UINT32_MAX;
		auto &player = json["player"];
		auto entity_name = player.find("entity_name");
		if (entity_name != player.end()) {
			const std::string &player_entity_name = *entity_name;
			for (uint32 i = 0; i < level->entity_count; i += 1) {
				if (!strcmp(player_entity_name.c_str(), level->entity_infos[i].name)) {
					level->player_entity_index = i;
					break;
				}
			}
		}
	}
	extra_read(json);
}

template <typename F>
void level_write_json(level *level, const char *json_file_path, F extra_write) {
	nlohmann::json json;
	{ // models, skyboxes, terrains
		auto &models = json["models"];
		models = nlohmann::json::array();
		for (uint32 i = 0; i < level->model_count; i += 1) {
			models.push_back(level->models[i].gpk_file);
		}
		auto &skyboxes = json["skyboxes"];
		skyboxes = nlohmann::json::array();
		for (uint32 i = 0; i < level->skybox_count; i += 1) {
			skyboxes.push_back(level->skyboxes[i].gpk_file);
		}
		json["skybox_index"] = level->skybox_index;
		auto &terrains = json["terrains"];
		terrains = nlohmann::json::array();
		for (uint32 i = 0; i < level->terrain_count; i += 1) {
			terrains.push_back(level->terrains[i].gpk_file);
		}
	}
	{ // entities
		auto &entities = json["entities"];
		entities = nlohmann::json::array();
		for (uint32 i = 0; i < level->entity_count; i += 1) {
			uint32 &flags = level->entity_flags[i];
			entity_info &info = level->entity_infos[i];
			transform &tfm = level->entity_transforms[i];
			nlohmann::json entity_json = {
				{"name", level->entity_infos[i].name},
				{"transform", {
					{"scale", {m_unpack3(tfm.scale)}}, 
					{"rotate", {m_unpack4(tfm.rotate)}},
					{"translate", {m_unpack3(tfm.translate)}}}
				}
			};
			if (flags & entity_component_flag_model) {
				entity_model_component *model_component = entity_get_model_component(level, i);
				transform &tfm = model_component->adjustment_transform;
				entity_json["model_component"] = {
					{"gpk_file", model_component->model_index < level->model_count ? level->models[model_component->model_index].gpk_file : ""},
					{"adjustment_transform", {
						{"scale", {m_unpack3(tfm.scale)}}, 
						{"rotate", {m_unpack4(tfm.rotate)}},
						{"translate", {m_unpack3(tfm.translate)}}}
					}
				};
			}
			if (flags & entity_component_flag_collision) {
				entity_collision_component *collision_component = entity_get_collision_component(level, i);
				if (collision_component->shape == collision_shape_sphere) {
					entity_json["collision_component"] = {
						{"shape", "sphere"},
						{"radius", collision_component->sphere.radius}
					};
				}
				else if (collision_component->shape == collision_shape_capsule) {
					entity_json["collision_component"] = {
						{"shape", "capsule"},
						{"height", collision_component->capsule.height},
						{"radius", collision_component->capsule.radius}
					};
				}
				else if (collision_component->shape == collision_shape_box) {
					entity_json["collision_component"] = {
						{"shape", "box"},
						{"size", {m_unpack3(collision_component->box.size)}}
					};
				}
				else if (collision_component->shape == collision_shape_terrain) {
					entity_json["collision_component"] = {
						{"shape", "terrain"},
						{"gpk_file", level->terrains[collision_component->terrain_index].gpk_file}
					};
				}
				else {
					m_assert(false);
				}
			}
			if (flags & entity_component_flag_physics) {
				entity_physics_component *physics_component = entity_get_physics_component(level, i);
				entity_json["physics_component"] = {
					{"velocity", {m_unpack3(physics_component->velocity)}},
					{"mass", physics_component->mass},
					{"max_speed", physics_component->max_speed}
				};
			}
			if (flags & entity_component_flag_light) {
				entity_light_component *light_component = entity_get_light_component(level, i);
				if (light_component->light_type == light_type_ambient) {
					entity_json["light_component"] = {
						{"light_type", "ambient"},
						{"color", {m_unpack3(light_component->ambient_light.color)}}
					};
				}				
				else if (light_component->light_type == light_type_directional) {
					entity_json["light_component"] = {
						{"light_type", "directional"},
						{"color", {m_unpack3(light_component->directional_light.color)}},
						{"direction", {m_unpack3(light_component->directional_light.direction)}}
					};
				}				
				else if (light_component->light_type == light_type_point) {
					entity_json["light_component"] = {
						{"light_type", "point"},
						{"color", {m_unpack3(light_component->point_light.color)}},
						{"position", {m_unpack3(light_component->point_light.position)}},
						{"attenuation", light_component->point_light.attenuation}
					};
				}				
			}
			if (flags & entity_component_flag_terrain) {
				entity_terrain_component *terrain_component = entity_get_terrain_component(level, i);
				transform &tfm = terrain_component->transform;
				entity_json["terrain_component"] = {
					{"gpk_file", terrain_component->terrain_index < level->terrain_count ? level->terrains[terrain_component->terrain_index].gpk_file : ""},
					{"transform", {
						{"scale", {m_unpack3(tfm.scale)}}, 
						{"rotate", {m_unpack4(tfm.rotate)}},
						{"translate", {m_unpack3(tfm.translate)}}}
					}
				};
			}
			entities.push_back(entity_json);
		}
	}
	{ // player
		auto &player = json["player"];
		player = nlohmann::json::object();
		if (level->player_entity_index < level->entity_count) {
			player = {
				{"entity_name", level->entity_infos[level->player_entity_index].name}
			};
		}
	}
	extra_write(json);
	std::string json_string = json.dump(2);

	file_mapping file_mapping = {};
	create_file_mapping(json_file_path, json_string.length(), &file_mapping);
	memcpy(file_mapping.ptr, json_string.c_str(), json_string.length());
	close_file_mapping(&file_mapping);
}

camera level_get_player_camera(level *level, vulkan *vulkan, float r, float theta, float phi) {
	vec3 center = {};
	vec3 translate = {};
	if (level->player_entity_index < level->entity_count) {
		transform *transform = &level->entity_transforms[level->player_entity_index];
		center = transform->translate;
		quat rotate_0 = quat_from_rotation(vec3{1, 0, 0}, theta);
		quat rotate_1 = quat_from_rotation(vec3{0, 1, 0}, phi);
		translate = rotate_1 * rotate_0 * vec3{0, 0, -r};
	}
	else {
		center = {0, 0, 0};
		translate = vec3_normalize({0, 1, -1}) * r;
	}
	camera camera = {};
	camera.position = center + translate;
	camera.view = vec3_normalize(-translate);
	camera.fovy = degree_to_radian(50);
	camera.aspect = (float)vulkan->swap_chain.image_width / (float)vulkan->swap_chain.image_height;
	camera.znear = 0.1f;
	camera.zfar = 1000;
	return camera;
}

template <typename F>
void traverse_model_node_hierarchy(model_node *nodes, uint32 index, uint32 level, F f) {
	model_node *node = &nodes[index];
	f(node, index);
	for (uint32 i = 0; i < node->child_count; i += 1) {
		traverse_model_node_hierarchy(nodes, node->children[i], level + 1, f);
	}
}

template <typename F>
void traverse_model_scenes(model *model, F f) {
	for (uint32 i = 0; i < model->scene_count; i += 1) {
		model_scene *scene = &model->scenes[i];
		for (uint32 i = 0; i < scene->node_index_count; i += 1) {
			traverse_model_node_hierarchy(model->nodes, scene->node_indices[i], 0, f);
		}
	}
}

template <typename F>
void traverse_model_node_hierarchy_track_global_transform(model_node *nodes, uint32 index, mat4 global_transform_mat, F f) {
	model_node *node = &nodes[index];
	global_transform_mat = global_transform_mat * node->local_transform_mat;
	f(node, index, global_transform_mat);
	for (uint32 i = 0; i < node->child_count; i += 1) {
		traverse_model_node_hierarchy_track_global_transform(nodes, node->children[i], global_transform_mat, f);
	}
}

template <typename F>
void traverse_model_scenes_track_global_transform(model *model, F f) {
	for (uint32 i = 0; i < model->scene_count; i += 1) {
		model_scene *scene = &model->scenes[i];
		for (uint32 i = 0; i < scene->node_index_count; i += 1) {
			traverse_model_node_hierarchy_track_global_transform(model->nodes, scene->node_indices[i], mat4_identity(), f);
		}
	}
}

template <typename F>
void level_generate_render_data(level *level, vulkan *vulkan, camera camera, F generate_extra_render_data) {
	level->render_data = {};

	vulkan->memory_regions.uniform_region_sizes[vulkan->frame_index] = 0;
	vulkan->memory_regions.dynamic_vertex_region_sizes[vulkan->frame_index] = 0;

	{ // level
		ambient_light ambient_light = {{0, 0, 0}};
		directional_light directional_light = {{0, 0, 0}, {0, 1, 0}};
		point_light point_light = {};
		for (uint32 i = 0; i < level->light_component_count; i += 1) {
			switch (level->light_components[i].light_type) {
				case light_type_ambient: {
					ambient_light = level->light_components[i].ambient_light;
				} break;
				case light_type_directional: {
					directional_light = level->light_components[i].directional_light;
				} break;
				case light_type_point: {
					point_light = level->light_components[i].point_light;
				} break;
			}
		}
		struct camera shadow_map_camera = camera;
		shadow_map_camera.zfar = 100;
		shader_level_info level_info = {};
		level_info.view_proj_mat = mat4_vulkan_clip() * camera_view_projection_mat4(camera);
		level_info.camera_position = vec4{m_unpack3(camera.position.e), 0};
		level_info.shadow_map_proj_mat = mat4_vulkan_clip() * camera_shadow_map_projection_mat4(shadow_map_camera, directional_light.direction);
		level_info.ambient_light_color = vec4{m_unpack3(ambient_light.color), 0};
		level_info.directional_light_color = vec4{m_unpack3(directional_light.color), 0};
		level_info.directional_light_dir = vec4{m_unpack3(directional_light.direction), 0};
		level_info.point_light_color = vec4{m_unpack3(point_light.color), 0};
		level_info.point_light_position = vec4{m_unpack3(point_light.position), point_light.attenuation};
		uint32 offset = append_vulkan_uniform_region(vulkan, &level_info, sizeof(level_info));
		m_assert(offset == 0);
	}
	{ // models
		if (level->model_component_count > 0) {
			level->render_data.models = allocate_memory<struct model_render_data>(&vulkan->frame_memory_arena, level->model_component_count);
			level->render_data.model_count = 0;
			uint32 model_component_index = 0;
			for (uint32 i = 0; i < level->entity_count; i += 1) {
				if (!(level->entity_flags[i] & entity_component_flag_model)) {
					continue;
				}
				entity_model_component *model_component = &level->model_components[model_component_index++];
				if (model_component->model_index >= level->model_count || model_component->hide) {
					continue;
				}
				model_render_data *model_render_data = &level->render_data.models[level->render_data.model_count++];
				model_render_data->model_index = model_component->model_index;

				shader_model_info model_info = {};
				model_info.model_mat = mat4_from_transform(level->entity_transforms[i]) * mat4_from_transform(model_component->adjustment_transform);
				model_render_data->uniform_region_buffer_offset = append_vulkan_uniform_region(vulkan, &model_info, sizeof(model_info));

				model model = level->models[model_component->model_index];
				if (model_component->animation_index < model.animation_count) {
					model_animation *animation = &model.animations[model_component->animation_index];
					model_node *animated_nodes = allocate_memory<struct model_node>(&vulkan->frame_memory_arena, model.node_count);
					memcpy(animated_nodes, model.nodes, model.node_count * sizeof(struct model_node));
					transform *animated_node_transforms = allocate_memory<struct transform>(&vulkan->frame_memory_arena, model.node_count);
					for (uint32 i = 0; i < model.node_count; i += 1) {
						animated_node_transforms[i] = animated_nodes[i].local_transform;
					}
					for (uint32 i = 0; i < animation->channel_count; i += 1) {
						model_animation_channel *channel = &animation->channels[i];
						model_animation_sampler *sampler = &animation->samplers[channel->sampler_index];
						m_assert(sampler->interpolation_type == gpk_model_animation_linear_interpolation);
						float time = (float)fmod(model_component->animation_time, (double)sampler->key_frames[sampler->key_frame_count - 1].time);
						for (uint32 i = 0; i < sampler->key_frame_count; i += 1) {
							model_animation_key_frame *key_frame = &sampler->key_frames[i];
							if (time <= key_frame->time) {
								if (channel->channel_type == gpk_model_animation_translate_channel) {
									vec3 translate = {};
									if (i == 0) {
										translate = vec3_lerp({0, 0, 0}, {m_unpack3(key_frame->transform_data)}, key_frame->time == 0 ? 1 : time / key_frame->time);
									}
									else {
										model_animation_key_frame *prev_key_frame = &sampler->key_frames[i - 1];
										translate = vec3_lerp({m_unpack3(prev_key_frame->transform_data)}, {m_unpack3(key_frame->transform_data)}, (time - prev_key_frame->time) / (key_frame->time - prev_key_frame->time));
									}
									animated_node_transforms[channel->node_index].translate = translate;
								}
								else if (channel->channel_type == gpk_model_animation_rotate_channel) {
									quat rotate = {};
									if (i == 0) {
										rotate = quat_slerp({0, 0, 0, 1}, {m_unpack4(key_frame->transform_data)}, key_frame->time == 0 ? 1 : time / key_frame->time);
									}
									else {
										model_animation_key_frame *prev_key_frame = &sampler->key_frames[i - 1];
										rotate = quat_slerp({m_unpack4(prev_key_frame->transform_data)}, {m_unpack4(key_frame->transform_data)}, (time - prev_key_frame->time) / (key_frame->time - prev_key_frame->time));
									}
									animated_node_transforms[channel->node_index].rotate = rotate;
								}
								else if (channel->channel_type == gpk_model_animation_scale_channel) {
									vec3 scale = {};
									if (i == 0) {
										scale = vec3_lerp({1, 1, 1}, {m_unpack3(key_frame->transform_data)}, key_frame->time == 0 ? 1 : time / key_frame->time);
									}
									else {
										model_animation_key_frame *prev_key_frame = &sampler->key_frames[i - 1];
										scale = vec3_lerp({m_unpack3(prev_key_frame->transform_data)}, {m_unpack3(key_frame->transform_data)}, (time - prev_key_frame->time) / (key_frame->time - prev_key_frame->time));
									}
									animated_node_transforms[channel->node_index].scale = scale;
								}
								break;
							}
						}
					}
					for (uint32 i = 0; i < model.node_count; i += 1) {
						animated_nodes[i].local_transform_mat = mat4_from_transform(animated_node_transforms[i]);
					}
					model.nodes = animated_nodes;
				}

				uint32 joints_uniform_region_buffer_offset = 0;
				if (model.skin_count > 0) {
					model_skin *skin = &model.skins[0];
					mat4 *joint_mats = allocate_memory<mat4>(&vulkan->frame_memory_arena, skin->joint_count);
					traverse_model_scenes_track_global_transform(&model, [&](model_node *node, uint32 index, mat4 global_transform) {
						for (uint32 i = 0; i < skin->joint_count; i += 1) {
							if (skin->joints[i].node_index == index) {
								joint_mats[i] = global_transform * skin->joints[i].inverse_bind_mat;
								break;
							}
						}
					});
					for (uint32 i = 0; i < skin->joint_count; i += 1) {
						m_assert(joint_mats[i] != mat4{});
					}
					joints_uniform_region_buffer_offset = append_vulkan_uniform_region(vulkan, joint_mats, skin->joint_count * sizeof(mat4));
				}

				model_render_data->mesh_count = 0;
				traverse_model_scenes(&model, [&](model_node *node, uint32 index) {
					if (node->mesh_index < model.mesh_count) {
						model_render_data->mesh_count += 1;
					}
				});
				model_render_data->meshes = allocate_memory<struct mesh_render_data>(&vulkan->frame_memory_arena, model_render_data->mesh_count);
				uint32 mesh_render_data_index = 0;
				traverse_model_scenes_track_global_transform(&model, [&](model_node *node, uint32 index, mat4 global_transform) {
					if (node->mesh_index < model.mesh_count) {
						mesh_render_data *mesh_render_data = &model_render_data->meshes[mesh_render_data_index++];
						mesh_render_data->mesh_index = node->mesh_index;
						if (model.skin_count > 0) {
							mesh_render_data->uniform_region_buffer_offset = joints_uniform_region_buffer_offset;
						}
						else {
							mesh_render_data->uniform_region_buffer_offset = append_vulkan_uniform_region(vulkan, &global_transform, sizeof(global_transform));
						}
						model_mesh *mesh = &model.meshes[node->mesh_index];
						mesh_render_data->primitives = allocate_memory<struct primitive_render_data>(&vulkan->frame_memory_arena, mesh->primitive_count);
						for (uint32 i = 0; i < mesh->primitive_count; i += 1) {
							primitive_render_data *primitive_render_data = &mesh_render_data->primitives[i];
							primitive_render_data->primitive_index = i;

							model_mesh_primitive *primitive = &mesh->primitives[i];
							model_material *material = nullptr;
							if (primitive->material_index < model.material_count) {
								material = &model.materials[primitive->material_index];
							}

							shader_primitive_info primitive_info = {};
							primitive_info.diffuse_factor = material ? material->diffuse_factor : vec4{1, 1, 1, 1};
							primitive_info.metallic_factor = material ? material->metallic_factor : 1;
							primitive_info.roughness_factor = material ? material->roughness_factor: 1;
							primitive_render_data->uniform_region_buffer_offset = append_vulkan_uniform_region(vulkan, &primitive_info, sizeof(primitive_info));
						}
					}
				});
			}
		}
	}
	{ // terrains
		if (level->terrain_component_count > 0) {
			level->render_data.terrains = allocate_memory<struct terrain_render_data>(&vulkan->frame_memory_arena, level->terrain_component_count);
			level->render_data.terrain_count = 0;
			uint32 terrain_component_index = 0;
			for (uint32 i = 0; i < level->entity_count; i += 1) {
				if (!(level->entity_flags[i] & entity_component_flag_terrain)) {
					continue;
				}
				entity_terrain_component *terrain_component = &level->terrain_components[terrain_component_index++];
				if (terrain_component->terrain_index >= level->terrain_count) {
					continue;
				}
				terrain_render_data *terrain_render_data = &level->render_data.terrains[level->render_data.terrain_count++];
				terrain_render_data->terrain_index = terrain_component->terrain_index;
			}
		}
	}
	generate_extra_render_data();
}

template <typename F0, typename F1>
void level_generate_render_commands(level *level, vulkan *vulkan, const camera &camera, F0 extra_color_render_pass_commands, F1 extra_swap_chain_render_pass_command) {
	VkCommandBuffer cmd_buffer = vulkan->cmd_buffers.graphic_cmd_buffers[vulkan->frame_index];
	VkDescriptorSet descriptor_sets[2] = {vulkan->descriptors.uniform_buffers[vulkan->frame_index], vulkan->descriptors.combined_image_samplers_descriptor_set};
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan->pipelines.pipeline_layout, 0, 2, descriptor_sets, 0, nullptr);
	{ // shadow passes
		{
			VkClearValue clear_values[] = {{1, 1, 0, 0}, {1, 0}};
			VkRenderPassBeginInfo render_pass_begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
			render_pass_begin_info.renderPass = vulkan->render_passes.shadow_map_render_passes[0];
			render_pass_begin_info.framebuffer = vulkan->framebuffers.shadow_map_framebuffers[vulkan->frame_index].framebuffer;
			render_pass_begin_info.renderArea.offset = {0, 0};
			render_pass_begin_info.renderArea.extent = {vulkan->framebuffers.shadow_map_framebuffers[vulkan->frame_index].width, vulkan->framebuffers.shadow_map_framebuffers[vulkan->frame_index].height};
			render_pass_begin_info.clearValueCount = m_countof(clear_values);
			render_pass_begin_info.pClearValues = clear_values;
			vkCmdBeginRenderPass(cmd_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
			VkViewport viewport = {0, 0, (float)vulkan->framebuffers.shadow_map_framebuffers[vulkan->frame_index].width, (float)vulkan->framebuffers.shadow_map_framebuffers[vulkan->frame_index].height, 0, 1};
			VkRect2D scissor = {{0, 0}, vulkan->framebuffers.shadow_map_framebuffers[vulkan->frame_index].width, vulkan->framebuffers.shadow_map_framebuffers[vulkan->frame_index].height};
			vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);
			vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);
			if (level->render_data.model_count > 0) {
				vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan->pipelines.static_model_shadow_map_pipeline);
				VkDeviceSize vertex_buffer_offset = 0;
				vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &vulkan->memory_regions.vertex_region_buffer, &vertex_buffer_offset);
				vkCmdBindIndexBuffer(cmd_buffer, vulkan->memory_regions.vertex_region_buffer, 0, VK_INDEX_TYPE_UINT16);
				for (auto &model_render_data : make_range(level->render_data.models, level->render_data.model_count)) {
					model &model = level->models[model_render_data.model_index];
					for (auto &mesh_render_data : make_range(model_render_data.meshes, model_render_data.mesh_count)) {
						model_mesh &mesh = model.meshes[mesh_render_data.mesh_index];
						for (auto &primitive_render_data : make_range(mesh_render_data.primitives, mesh.primitive_count)) {
							model_mesh_primitive &primitive = mesh.primitives[primitive_render_data.primitive_index];
							shader_model_push_constant pc = {};
							pc.model_offset = model_render_data.uniform_region_buffer_offset;
							pc.mesh_offset = mesh_render_data.uniform_region_buffer_offset;
							pc.primitive_offset = primitive_render_data.uniform_region_buffer_offset;
							vkCmdPushConstants(cmd_buffer, vulkan->pipelines.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
							vkCmdDrawIndexed(cmd_buffer, primitive.index_count, 1, primitive.index_buffer_offset / sizeof(uint16), primitive.vertex_buffer_offset / sizeof(struct gpk_model_vertex), 0);
						}
					}
				}
			}
			vkCmdEndRenderPass(cmd_buffer);
		}
		{
			VkClearValue clear_values[] = {{0, 0, 0, 0}};
			VkRenderPassBeginInfo render_pass_begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
			render_pass_begin_info.renderPass = vulkan->render_passes.shadow_map_render_passes[1];
			render_pass_begin_info.framebuffer = vulkan->framebuffers.shadow_map_blur_1_framebuffers[vulkan->frame_index].framebuffer;
			render_pass_begin_info.renderArea.offset = {0, 0};
			render_pass_begin_info.renderArea.extent = {vulkan->framebuffers.shadow_map_framebuffers[vulkan->frame_index].width, vulkan->framebuffers.shadow_map_framebuffers[vulkan->frame_index].height};
			render_pass_begin_info.clearValueCount = m_countof(clear_values);
			render_pass_begin_info.pClearValues = clear_values;
			vkCmdBeginRenderPass(cmd_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan->pipelines.shadow_map_gaussian_blur_pipelines[0]);
		  shader_gaussian_blur_push_constant pc;
		  pc.index = vulkan->framebuffers.shadow_map_blur_1_descriptor_indices[vulkan->frame_index];
		  pc.x_dir = 1;
		  pc.y_dir = 0;
		  vkCmdPushConstants(cmd_buffer, vulkan->pipelines.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
			vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
			vkCmdEndRenderPass(cmd_buffer);
		}
		{
			VkClearValue clear_values[] = {{0, 0, 0, 0}};
			VkRenderPassBeginInfo render_pass_begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
			render_pass_begin_info.renderPass = vulkan->render_passes.shadow_map_render_passes[2];
			render_pass_begin_info.framebuffer = vulkan->framebuffers.shadow_map_blur_2_framebuffers[vulkan->frame_index].framebuffer;
			render_pass_begin_info.renderArea.offset = {0, 0};
			render_pass_begin_info.renderArea.extent = {vulkan->framebuffers.shadow_map_framebuffers[vulkan->frame_index].width, vulkan->framebuffers.shadow_map_framebuffers[vulkan->frame_index].height};
			render_pass_begin_info.clearValueCount = m_countof(clear_values);
			render_pass_begin_info.pClearValues = clear_values;
			vkCmdBeginRenderPass(cmd_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan->pipelines.shadow_map_gaussian_blur_pipelines[1]);
		  shader_gaussian_blur_push_constant pc;
		  pc.index = vulkan->framebuffers.shadow_map_blur_2_descriptor_indices[vulkan->frame_index];
		  pc.x_dir = 0;
		  pc.y_dir = 1;
		  vkCmdPushConstants(cmd_buffer, vulkan->pipelines.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
			vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
			vkCmdEndRenderPass(cmd_buffer);
		}
	}
	{ // color render pass
		VkClearValue clear_values[2] = {{0, 0, 0, 1}, {0, 0}};
		VkRenderPassBeginInfo render_pass_begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
		render_pass_begin_info.renderPass = vulkan->render_passes.color_render_pass;
		render_pass_begin_info.framebuffer = vulkan->framebuffers.color_framebuffers[vulkan->frame_index].framebuffer;
		render_pass_begin_info.renderArea.offset = {0, 0};
		render_pass_begin_info.renderArea.extent = {vulkan->framebuffers.color_framebuffers[vulkan->frame_index].width, vulkan->framebuffers.color_framebuffers[vulkan->frame_index].height};
		render_pass_begin_info.clearValueCount = m_countof(clear_values);
		render_pass_begin_info.pClearValues = clear_values;
		vkCmdBeginRenderPass(cmd_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		VkViewport viewport = {0, 0, (float)vulkan->framebuffers.color_framebuffers[vulkan->frame_index].width, (float)vulkan->framebuffers.color_framebuffers[vulkan->frame_index].height, 1, 0};
		VkRect2D scissor = {{0, 0}, vulkan->framebuffers.color_framebuffers[vulkan->frame_index].width, vulkan->framebuffers.color_framebuffers[vulkan->frame_index].height};
		vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);
		vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);
		if (level->render_data.model_count > 0) {
			vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan->pipelines.static_model_pipeline);
			VkDeviceSize vertex_buffer_offset = 0;
			vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &vulkan->memory_regions.vertex_region_buffer, &vertex_buffer_offset);
			vkCmdBindIndexBuffer(cmd_buffer, vulkan->memory_regions.vertex_region_buffer, 0, VK_INDEX_TYPE_UINT16);
			for (auto &model_render_data : make_range(level->render_data.models, level->render_data.model_count)) {
				model &model = level->models[model_render_data.model_index];
				for (auto &mesh_render_data : make_range(model_render_data.meshes, model_render_data.mesh_count)) {
					model_mesh &mesh = model.meshes[mesh_render_data.mesh_index];
					for (auto &primitive_render_data : make_range(mesh_render_data.primitives, mesh.primitive_count)) {
						model_mesh_primitive &primitive = mesh.primitives[primitive_render_data.primitive_index];
						shader_model_push_constant pc = {};
						pc.model_offset = model_render_data.uniform_region_buffer_offset;
						pc.mesh_offset = mesh_render_data.uniform_region_buffer_offset;
						pc.primitive_offset = primitive_render_data.uniform_region_buffer_offset;
						pc.diffuse_map_index = primitive.material_index < model.material_count ? model.materials[primitive.material_index].diffuse_map_descriptor_index : level->persistant_data.default_diffuse_map_descriptor_index;
						pc.metallic_map_index = primitive.material_index < model.material_count ? model.materials[primitive.material_index].metallic_map_descriptor_index : level->persistant_data.default_metallic_map_descriptor_index;
						pc.roughness_map_index = primitive.material_index < model.material_count ? model.materials[primitive.material_index].roughness_map_descriptor_index : level->persistant_data.default_roughness_map_descriptor_index;
						pc.normal_map_index = primitive.material_index < model.material_count ? model.materials[primitive.material_index].normal_map_descriptor_index : level->persistant_data.default_normal_map_descriptor_index;
						pc.height_map_index = level->persistant_data.default_height_map_descriptor_index;
						pc.shadow_map_index = vulkan->framebuffers.shadow_map_blur_1_descriptor_indices[vulkan->frame_index];
						vkCmdPushConstants(cmd_buffer, vulkan->pipelines.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
						vkCmdDrawIndexed(cmd_buffer, primitive.index_count, 1, primitive.index_buffer_offset / sizeof(uint16), primitive.vertex_buffer_offset / sizeof(struct gpk_model_vertex), 0);
					}
				}
			}
		}
		if (level->render_data.terrain_count > 0) {
			vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan->pipelines.terrain_pipeline);
			VkDeviceSize vertex_buffer_offset = 0;
			vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &vulkan->memory_regions.vertex_region_buffer, &vertex_buffer_offset);
			for (uint32 i = 0; i < level->render_data.terrain_count; i += 1) {
				terrain *terrain = &level->terrains[level->render_data.terrains[i].terrain_index];
				shader_terrain_push_constant pc = {};
				pc.height_map_index = terrain->height_map_descriptor_index;
				pc.diffuse_map_index = terrain->diffuse_map_descriptor_index;
				vkCmdPushConstants(cmd_buffer, vulkan->pipelines.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
				vkCmdDraw(cmd_buffer, 128 * 128 * 6, 1, level->persistant_data.terrain_vertex_region_buffer_offset / sizeof(struct terrain_vertex), 0);
			}
		}
		if (level->skybox_index < level->skybox_count) {
		  skybox *skybox = &level->skyboxes[level->skybox_index];
		  vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan->pipelines.skybox_pipeline);
		  struct camera skybox_camera = camera;
		  skybox_camera.position = {0, 0, 0};
		  shader_skybox_push_constant pc = {};
		  pc.view_proj_mat = mat4_vulkan_clip() * camera_view_projection_mat4(skybox_camera);
		  pc.cube_map_index = 0;
		  vkCmdPushConstants(cmd_buffer, vulkan->pipelines.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
		  vkCmdDraw(cmd_buffer, 36, 1, 0, 0);
		}
		extra_color_render_pass_commands();
		vkCmdEndRenderPass(cmd_buffer);
	}
	{ // swap chain render pass
		VkClearValue clear_value = {0, 0, 0, 1};
		VkRenderPassBeginInfo render_pass_begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
		render_pass_begin_info.renderPass = vulkan->render_passes.swap_chain_render_pass;
		render_pass_begin_info.framebuffer = vulkan->framebuffers.swap_chain_framebuffers[vulkan->swap_chain_image_index];
		render_pass_begin_info.renderArea.offset = {0, 0};
		render_pass_begin_info.renderArea.extent = {vulkan->swap_chain.image_width, vulkan->swap_chain.image_height};
		render_pass_begin_info.clearValueCount = 1;
		render_pass_begin_info.pClearValues = &clear_value;

		vkCmdBeginRenderPass(cmd_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = {0, 0, (float)vulkan->swap_chain.image_width, (float)vulkan->swap_chain.image_height, 0, 1};
		VkRect2D scissor = {{0, 0}, {vulkan->swap_chain.image_width, vulkan->swap_chain.image_height}};
		vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);
		vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);

		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan->pipelines.swap_chain_pipeline);
		shader_swap_chain_push_constant pc = {};
		pc.texture_index = vulkan->framebuffers.color_descriptor_indices[vulkan->frame_index];
		vkCmdPushConstants(cmd_buffer, vulkan->pipelines.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
		vkCmdDraw(cmd_buffer, 3, 1, 0, 0);

		extra_swap_chain_render_pass_command();
		vkCmdEndRenderPass(cmd_buffer);
	}
}	

