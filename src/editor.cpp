/***************************************************************************************************/
/*          Copyright (C) 2015-2017 By Yang Chen (yngccc@gmail.com). All Rights Reserved.          */
/***************************************************************************************************/

#include "platform_windows.cpp"
#include "math.cpp"
#include "vulkan.cpp"
#include "assets.cpp"
#include "level.cpp"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include "../vendor/include/imgui/imgui_draw.cpp"
#include "../vendor/include/imgui/imgui.cpp"
#include "../vendor/include/imgui/ImGuizmo.cpp"
#undef snprintf

const int32 editor_gizmo_transform_translate = 0;
const int32 editor_gizmo_transform_rotate = 1;
const int32 editor_gizmo_transform_scale = 2;
const int32 editor_gizmo_directional_light_rotate = 3;
const int32 editor_gizmo_point_light_translate = 4;

struct editor_render_data {
	uint32 lines_frame_vertex_buffer_offset;
	uint32 lines_vertex_count;
	uint32 imgui_frame_vertex_buffer_offset;
};

struct editor {
	stbtt_fontinfo stbtt_font_info;
	stbtt_pack_context stbtt_pack_context;
	stbtt_packedchar stbtt_packed_chars[95];
	float font_size;
	uint32 font_atlas_width;
	uint32 font_atlas_height;

	camera camera;
	float camera_pitch;
	float camera_move_speed;
	bool camera_moving;

	uint32 entity_index;
	int32 entity_gizmo;
	uint32 entity_render_component_mesh_index;
	bool entity_collision_component_render_planes;
	bool entity_collision_component_render_spheres;
	bool entity_collision_component_render_bounds;
	uint32 entity_collision_component_plane_index;
	uint32 entity_collision_component_sphere_index;
	uint32 entity_collision_component_bound_index;

	char new_level_file[128];
	char load_level_file[128];
	char save_level_file[128];
	
	editor_render_data render_data;

	memory_arena general_memory_arena;
};

bool editor_initialize(editor *editor, vulkan *vulkan) {
	{ // memory
		editor->general_memory_arena = {};
		editor->general_memory_arena.name = "general";
		editor->general_memory_arena.capacity = m_megabytes(64);
		m_assert(allocate_virtual_memory(editor->general_memory_arena.capacity, &editor->general_memory_arena.memory));
	}
	{ // imgui
		ImGui::StyleColorsDark();
		ImGuiIO *imgui_io = &ImGui::GetIO();
		imgui_io->KeyMap[ImGuiKey_Tab] = keycode_tab;
		imgui_io->KeyMap[ImGuiKey_LeftArrow] = keycode_left;
		imgui_io->KeyMap[ImGuiKey_RightArrow] = keycode_right;
		imgui_io->KeyMap[ImGuiKey_UpArrow] = keycode_up;
		imgui_io->KeyMap[ImGuiKey_DownArrow] = keycode_down;
		imgui_io->KeyMap[ImGuiKey_PageUp] = keycode_page_up;
		imgui_io->KeyMap[ImGuiKey_PageDown] = keycode_page_down;
		imgui_io->KeyMap[ImGuiKey_Home] = keycode_home;
		imgui_io->KeyMap[ImGuiKey_End] = keycode_end;
		imgui_io->KeyMap[ImGuiKey_Backspace] = keycode_backspace;
		imgui_io->KeyMap[ImGuiKey_Enter] = keycode_return;
		imgui_io->KeyMap[ImGuiKey_Escape] = keycode_esc;
		imgui_io->KeyMap[ImGuiKey_A] = 'A';
		imgui_io->KeyMap[ImGuiKey_C] = 'C';
		imgui_io->KeyMap[ImGuiKey_V] = 'V';
		imgui_io->KeyMap[ImGuiKey_X] = 'X';
		imgui_io->KeyMap[ImGuiKey_Y] = 'Y';
		imgui_io->KeyMap[ImGuiKey_Z] = 'Z';
		imgui_io->DisplaySize = {(float)vulkan->swap_chain.image_width, (float)vulkan->swap_chain.image_height};
		ImGuizmo::SetRect(0, 0, imgui_io->DisplaySize.x, imgui_io->DisplaySize.y);
		imgui_io->IniFilename = ".imgui.ini";
		imgui_io->MousePos = {-1, -1};
		imgui_io->FontGlobalScale = 1;

		m_assert(ImGui::GetIO().Fonts->AddFontFromFileTTF("assets\\fonts\\Roboto-Medium.ttf", (float)GetSystemMetrics(SM_CXSCREEN) / 180.0f));
		uint8* font_atlas_image = nullptr;
		int32 font_atlas_image_width = 0;
		int32 font_atlas_image_height = 0;
		ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&font_atlas_image, &font_atlas_image_width, &font_atlas_image_height);

		VkImageCreateInfo image_create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		image_create_info.imageType = VK_IMAGE_TYPE_2D;
		image_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
		image_create_info.extent = {(uint32)font_atlas_image_width, (uint32)font_atlas_image_height, 1};
		image_create_info.mipLevels = 1;
		image_create_info.arrayLayers = 1;
		image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		VkImageViewCreateInfo image_view_create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_create_info.subresourceRange.levelCount = 1;
		image_view_create_info.subresourceRange.layerCount = 1;
		vulkan_image_create(vulkan, &vulkan->memories.common_images_memory, image_create_info, image_view_create_info, &vulkan->images.imgui_font_atlas_image);
		vulkan_image_transfer(vulkan, &vulkan->images.imgui_font_atlas_image, font_atlas_image, font_atlas_image_width * font_atlas_image_height * 4, 32, 1);

		VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		descriptor_set_allocate_info.descriptorPool = vulkan->descriptors.pool;
		descriptor_set_allocate_info.descriptorSetCount = 1;
		descriptor_set_allocate_info.pSetLayouts = &vulkan->descriptors.sampled_images_set_layouts[0];
		m_vk_assert(vkAllocateDescriptorSets(vulkan->device.device, &descriptor_set_allocate_info, &vulkan->descriptors.imgui_font_atlas_image));
		VkDescriptorImageInfo descriptor_image_info = {vulkan->samplers.mipmap_image_samplers[vulkan->images.imgui_font_atlas_image.mipmap_count], vulkan->images.imgui_font_atlas_image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		VkWriteDescriptorSet write_descriptor_set = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		write_descriptor_set.dstSet = vulkan->descriptors.imgui_font_atlas_image;
		write_descriptor_set.dstBinding = 0;
		write_descriptor_set.descriptorCount = 1;
		write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write_descriptor_set.pImageInfo = &descriptor_image_info;
		vkUpdateDescriptorSets(vulkan->device.device, 1, &write_descriptor_set, 0, nullptr);
	}
	{ // font
		file_mapping font_file_mapping = {};
		m_assert(open_file_mapping("assets\\fonts\\Roboto-Medium.ttf", &font_file_mapping));
		m_scope_exit(close_file_mapping(font_file_mapping));

		m_assert(stbtt_InitFont(&editor->stbtt_font_info, font_file_mapping.ptr, 0));
		editor->font_size = 128;
		editor->font_atlas_width = 2048;
		editor->font_atlas_height = 1024;
		uint8 *font_atlas = memory_arena_allocate<uint8>(&editor->general_memory_arena, editor->font_atlas_width * editor->font_atlas_height);
		m_assert(stbtt_PackBegin(&editor->stbtt_pack_context, font_atlas, editor->font_atlas_width, editor->font_atlas_height, 0, 1, nullptr));
		stbtt_PackSetOversampling(&editor->stbtt_pack_context, 2, 2);
		m_assert(stbtt_PackFontRange(&editor->stbtt_pack_context, font_file_mapping.ptr, 0, editor->font_size, 32, 95, editor->stbtt_packed_chars));
		stbtt_PackEnd(&editor->stbtt_pack_context);

		VkImageCreateInfo image_create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		image_create_info.imageType = VK_IMAGE_TYPE_2D;
		image_create_info.format = VK_FORMAT_R8_UNORM;
		image_create_info.extent = {editor->font_atlas_width, editor->font_atlas_height, 1};
		image_create_info.mipLevels = 1;
		image_create_info.arrayLayers = 1;
		image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		VkImageViewCreateInfo image_view_create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.format = image_create_info.format;
		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_create_info.subresourceRange.levelCount = 1;
		image_view_create_info.subresourceRange.layerCount = 1;
		vulkan_image_create(vulkan, &vulkan->memories.common_images_memory, image_create_info, image_view_create_info, &vulkan->images.font_atlas_image);
		vulkan_image_transfer(vulkan, &vulkan->images.font_atlas_image, font_atlas, editor->font_atlas_width * editor->font_atlas_height, 8, 1);

		VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		descriptor_set_allocate_info.descriptorPool = vulkan->descriptors.pool;
		descriptor_set_allocate_info.descriptorSetCount = 1;
		descriptor_set_allocate_info.pSetLayouts = &vulkan->descriptors.sampled_images_set_layouts[0];
		m_vk_assert(vkAllocateDescriptorSets(vulkan->device.device, &descriptor_set_allocate_info, &vulkan->descriptors.font_atlas_image));
		VkDescriptorImageInfo descriptor_image_info = {vulkan->samplers.mipmap_image_samplers[0], vulkan->images.font_atlas_image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		VkWriteDescriptorSet write_descriptor_set = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		write_descriptor_set.dstSet = vulkan->descriptors.font_atlas_image;
		write_descriptor_set.dstBinding = 0;
		write_descriptor_set.descriptorCount = 1;
		write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write_descriptor_set.pImageInfo = &descriptor_image_info;
		vkUpdateDescriptorSets(vulkan->device.device, 1, &write_descriptor_set, 0, nullptr);
	}
	{ // misc
		editor->camera.position = vec3{4, 8, 8};
		editor->camera.view = vec3_normalize(-editor->camera.position);
		editor->camera.up = vec3_cross(vec3_cross(editor->camera.view, vec3{0, 1, 0}), editor->camera.view);
		editor->camera.fovy = degree_to_radian(50);
		editor->camera.aspect = (float)vulkan->swap_chain.image_width / (float)vulkan->swap_chain.image_height;
		editor->camera.znear = 0.1f;
		editor->camera.zfar = 1000;
		editor->camera_pitch = asinf(editor->camera.view.y);
		editor->camera_move_speed = 10;

		editor->entity_index = UINT32_MAX;
		editor->entity_render_component_mesh_index= UINT32_MAX;
		editor->entity_collision_component_plane_index = UINT32_MAX;
		editor->entity_collision_component_sphere_index = UINT32_MAX;
		editor->entity_collision_component_bound_index = UINT32_MAX;

		char new_level_file[] = "agby_assets\\levels\\level_new.json";
		char load_level_file[] = "agby_assets\\levels\\level_load.json";
		char save_level_file[] = "agby_assets\\levels\\level_save.json";
		array_copy(editor->new_level_file, new_level_file);
		array_copy(editor->load_level_file, load_level_file);
		array_copy(editor->save_level_file, save_level_file);
	}
	return true;
}

int WinMain(HINSTANCE instance_handle, HINSTANCE prev_instance_handle, LPSTR cmd_line_str, int cmd_show) {
	set_exe_dir_as_current();
	show_command_prompt();
	assets_import_issue_jobs();

	struct window window = {};
	m_assert(initialize_window(&window));
	// set_window_fullscreen(&window, true);

	struct vulkan vulkan = {};
	initialize_vulkan(&vulkan, window);

	editor editor = {};
	editor_initialize(&editor, &vulkan);

	level level = {};
	initialize_level(&level, &vulkan);
	auto editor_load_level_setting = [&editor](rapidjson::Document *json_doc) {
		if (json_doc->HasMember("editor_settings")) {
			rapidjson::Value::Object editor_settings = (*json_doc)["editor_settings"].GetObject();
			if (editor_settings.HasMember("camera")) {
				rapidjson::Value::Object camera = editor_settings["camera"].GetObject();
				rapidjson::Value::Array position = camera["position"].GetArray();
				editor.camera.position = {(float)position[0].GetDouble(), (float)position[1].GetDouble(), (float)position[2].GetDouble()};
				rapidjson::Value::Array view = camera["view"].GetArray();
				editor.camera.view = {(float)view[0].GetDouble(), (float)view[1].GetDouble(), (float)view[2].GetDouble()};
				rapidjson::Value::Array up = camera["up"].GetArray();
				editor.camera.up = {(float)up[0].GetDouble(), (float)up[1].GetDouble(), (float)up[2].GetDouble()};
				editor.camera.fovy = (float)camera["fovy"].GetDouble();
				editor.camera.aspect = (float)camera["aspect"].GetDouble();
				editor.camera.znear = (float)camera["znear"].GetDouble();
				editor.camera.zfar = (float)camera["zfar"].GetDouble();
				editor.camera_move_speed = (float)camera["move_speed"].GetDouble();
				editor.camera_pitch = asinf(editor.camera.view.y);
			}
		}
	};
	level_read_json(&level, &vulkan, "agby_assets\\levels\\level_save.json", editor_load_level_setting, true);

	LARGE_INTEGER performance_frequency = {};
	QueryPerformanceFrequency(&performance_frequency);
	LARGE_INTEGER performance_counters[2] = {};
	uint64 last_frame_time_microsec = 0;
	double last_frame_time_sec = 0;
	bool program_running = true;

	show_window(&window);

	while (program_running) {
		QueryPerformanceCounter(&performance_counters[0]);

		ImGui::GetIO().DeltaTime = (float)last_frame_time_sec;
		window.raw_mouse_dx = 0;
		window.raw_mouse_dy = 0;

		while (peek_window_message(&window)) {
			switch (window.msg_type) {
				case window_message_type_destroy:
				case window_message_type_close:
				case window_message_type_quit: {
					program_running = false;
				} break;
				case window_message_type_key_down:
				case window_message_type_key_up: {
					if (window.keycode == keycode_shift) {
						ImGui::GetIO().KeyShift = (window.msg_type == window_message_type_key_down);
					}
					else if (window.keycode == keycode_ctrl) {
						ImGui::GetIO().KeyCtrl = (window.msg_type == window_message_type_key_down);
					}
					else if (window.keycode == keycode_alt) {
						ImGui::GetIO().KeyAlt = (window.msg_type == window_message_type_key_down);
					}
					else {
						m_assert(window.keycode < sizeof(ImGui::GetIO().KeysDown));
						ImGui::GetIO().KeysDown[window.keycode] = (window.msg_type == window_message_type_key_down);
						if (window.keycode == keycode_print_screen && window.msg_type != window_message_type_key_down) {
							// simulate print screen key down event since windows does not send it for some reason
							ImGui::GetIO().KeysDownDuration[keycode_print_screen] = 0.1f;
						}
					}
				} break;
				case window_message_type_char: {
					ImGui::GetIO().AddInputCharacter(window.input_char);
				} break;
				case window_message_type_mouse_move: {
					ImGui::GetIO().MousePos.x = (float)window.mouse_x;
					ImGui::GetIO().MousePos.y = (float)window.mouse_y;
				} break;
				case window_message_type_mouse_lb_down:
				case window_message_type_mouse_lb_up: {
					ImGui::GetIO().MouseDown[0] = (window.msg_type == window_message_type_mouse_lb_down);
				} break;
				case window_message_type_mouse_rb_down:
				case window_message_type_mouse_rb_up: {
					ImGui::GetIO().MouseDown[1] = (window.msg_type == window_message_type_mouse_rb_down);
				} break;
				case window_message_type_mouse_mb_down:
				case window_message_type_mouse_mb_up: {
					ImGui::GetIO().MouseDown[2] = (window.msg_type == window_message_type_mouse_mb_down);
				} break;
				case window_message_type_mouse_wheel: {
					ImGui::GetIO().MouseWheel = window.mouse_wheel;
				} break;
			}
		}

		ImGui::NewFrame();
		{ // miscs
			if (ImGui::GetIO().KeyAlt && ImGui::IsKeyPressed(keycode_f4)) {
				program_running = false;
			}
		}
		{ // move camera
			if (ImGui::IsMouseClicked(1) && !ImGui::GetIO().WantCaptureMouse) {
				pin_cursor(true);
				editor.camera_moving = true;
			}
			if (ImGui::IsMouseReleased(1)) {
				pin_cursor(false);
				editor.camera_moving = false;
			}
			if (editor.camera_moving) {
				if (ImGui::IsKeyDown('W')) {
					editor.camera.position = editor.camera.position + editor.camera.view * (float)last_frame_time_sec * editor.camera_move_speed;
				}
				if (ImGui::IsKeyDown('S')) {
					editor.camera.position = editor.camera.position - editor.camera.view * (float)last_frame_time_sec * editor.camera_move_speed;
				}
				if (ImGui::IsKeyDown('A')) {
					vec3 move_direction = vec3_normalize(vec3_cross(editor.camera.view, vec3{0, 1, 0}));
					editor.camera.position = editor.camera.position - move_direction * (float)last_frame_time_sec * editor.camera_move_speed;
				}
				if (ImGui::IsKeyDown('D')) {
					vec3 move_direction = vec3_normalize(vec3_cross(editor.camera.view, vec3{0, 1, 0}));
					editor.camera.position = editor.camera.position + move_direction * (float)last_frame_time_sec * editor.camera_move_speed;
				}
				float camera_rotation_speed = 2.0f;
				float max_pitch = degree_to_radian(75.0f);
				float yaw = -window.raw_mouse_dx * camera_rotation_speed * ImGui::GetIO().DeltaTime;
				editor.camera.view = vec3_normalize(mat4_to_mat3(mat4_from_rotation(vec3{0, 1, 0}, yaw)) * editor.camera.view);
				float pitch = -window.raw_mouse_dy * camera_rotation_speed * ImGui::GetIO().DeltaTime;
				if (editor.camera_pitch + pitch > -max_pitch && editor.camera_pitch + pitch < max_pitch) {
					vec3 view_cross_up = vec3_normalize(vec3_cross(editor.camera.view, vec3{0, 1, 0}));
					mat4 rotate_mat = mat4_from_rotation(view_cross_up, pitch);
					editor.camera.view = vec3_normalize(mat4_to_mat3(rotate_mat) * editor.camera.view);
					editor.camera_pitch += pitch;
				}
				editor.camera.up = vec3_normalize(vec3_cross(vec3_cross(editor.camera.view, vec3{0, 1, 0}), editor.camera.view));
			}
		}
		{ // entity operations
			if (editor.entity_index < level.entity_count) {
				if (ImGui::IsMouseClicked(0) && ImGui::GetIO().KeyShift && !ImGui::GetIO().WantCaptureMouse && !ImGuizmo::IsOver()) {
					uint32 entity_flag = level.entity_flags[editor.entity_index];
					if (entity_flag & component_flag_render) {
						vec3 window_pos = vec3{ImGui::GetMousePos().x, ImGui::GetIO().DisplaySize.y - ImGui::GetMousePos().y, 0.1f};
						vec4 viewport = vec4{0, 0, ImGui::GetIO().DisplaySize.x , ImGui::GetIO().DisplaySize.y};
						vec3 mouse_world_position = mat4_unproject(window_pos, camera_view_mat4(editor.camera), camera_projection_mat4(editor.camera), viewport);
						ray ray = {editor.camera.position, vec3_normalize(mouse_world_position - editor.camera.position), editor.camera.zfar};
						render_component *render_component = entity_get_render_component(&level, editor.entity_index);
						model *model = &level.models[render_component->model_index];
						mat4 entity_transform = transform_to_mat4(level.entity_transforms[editor.entity_index]);
						uint32 mesh_index = UINT32_MAX;
						float min_distance = editor.camera.zfar;
						for (uint32 i = 0; i < model->mesh_count; i += 1) {
							model_mesh *mesh = &model->meshes[i];
							uint32 current_mesh_index = i;
							for (uint32 i = 0; i < mesh->instance_count; i += 1) {
								model_mesh_instance *instance = &mesh->instances[i];
								mat4 transform = entity_transform * instance->transform;
								for (uint32 i = 0; i < mesh->index_count / 3; i += 1) {
									vec3 a = *(vec3 *)(mesh->vertices_data + mesh->vertex_size * ((uint16 *)mesh->indices_data)[i * 3 + 0]);
									vec3 b = *(vec3 *)(mesh->vertices_data + mesh->vertex_size * ((uint16 *)mesh->indices_data)[i * 3 + 1]);
									vec3 c = *(vec3 *)(mesh->vertices_data + mesh->vertex_size * ((uint16 *)mesh->indices_data)[i * 3 + 2]);
									a = transform * a;
									b = transform * b;
									c = transform * c;
									float distance = 0;
									if (ray_intersect_triangle(ray, a, b, c, &distance) && distance < min_distance) {
										min_distance = distance;
										mesh_index = current_mesh_index;
									}
								}
							}
						}
						editor.entity_render_component_mesh_index = mesh_index;
					}
				}
			}
		}
		{ // main menu
			bool new_level_popup = false;
			bool load_level_popup = false;
			bool save_level_popup = false;
			if (ImGui::BeginMainMenuBar()) {
				ImGui::PushID("main_menu_bar");
				if (ImGui::BeginMenu("File##file")) {
					ImGui::PushID("file");
					if (ImGui::MenuItem("New Level##new_level")) {
						new_level_popup = true;
					}
					if (ImGui::MenuItem("Load Level##load_level")) {
						load_level_popup = true;
					}
					if (ImGui::MenuItem("Save Level##save_level")) {
						save_level_popup = true;
					}
					ImGui::Separator();
					ImGui::PopID();
					ImGui::EndMenu();
				}
				ImGui::PopID();
				ImGui::EndMainMenuBar();
			}
			if (load_level_popup) {
				ImGui::OpenPopup("##load_level_popup");
			}
			if (ImGui::BeginPopupModal("##load_level_popup", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::InputText("file##load_level_popup_file", editor.load_level_file, sizeof(editor.load_level_file));
				if (ImGui::Button("load##load_level_popup_load")) {
					level_read_json(&level, &vulkan, editor.load_level_file, editor_load_level_setting, true);
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("cancel##load_level_popup_cancel")) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			if (save_level_popup) {
				ImGui::OpenPopup("##save_level_popup");
			}
			if (ImGui::BeginPopupModal("##save_level_popup", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::InputText("file##save_level_popup_file", editor.save_level_file, sizeof(editor.save_level_file));
				auto editor_dump_level_setting = [&editor](rapidjson::PrettyWriter<rapidjson::StringBuffer> *writer) {
					writer->Key("editor_settings");
					writer->StartObject();
					writer->Key("camera");
					writer->StartObject();
					writer->Key("position");
					writer->StartArray();
					writer->Double(editor.camera.position.x);
					writer->Double(editor.camera.position.y);
					writer->Double(editor.camera.position.z);
					writer->EndArray();
					writer->Key("view");
					writer->StartArray();
					writer->Double(editor.camera.view.x);
					writer->Double(editor.camera.view.y);
					writer->Double(editor.camera.view.z);
					writer->EndArray();
					writer->Key("up");
					writer->StartArray();
					writer->Double(editor.camera.up.x);
					writer->Double(editor.camera.up.y);
					writer->Double(editor.camera.up.z);
					writer->EndArray();
					writer->Key("fovy");
					writer->Double(editor.camera.fovy);
					writer->Key("aspect");
					writer->Double(editor.camera.aspect);
					writer->Key("znear");
					writer->Double(editor.camera.znear);
					writer->Key("zfar");
					writer->Double(editor.camera.zfar);
					writer->Key("move_speed");
					writer->Double(editor.camera_move_speed);
					writer->EndObject();
					writer->EndObject();
				};
				if (ImGui::Button("save##save_level_popup_save")) {
					if (file_exists(editor.save_level_file)) {
						ImGui::OpenPopup("##save_level_popup_file_exists_popup");
					}
					else {
						level_write_json(&level, editor.save_level_file, editor_dump_level_setting);
						ImGui::CloseCurrentPopup();
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("cancel##save_level_popup_cancel")) {
					ImGui::CloseCurrentPopup();
				}
				bool close_popup = false;
				if (ImGui::BeginPopupModal("##save_level_popup_file_exists_popup")) {
					ImGui::Text("file \"%s\" already exists, replace?", editor.save_level_file);
					ImGui::Separator();
					if (ImGui::Button("yes##save_level_popup_file_exists_popup_yes")) {
						level_write_json(&level, editor.save_level_file, editor_dump_level_setting);
						ImGui::CloseCurrentPopup();
						close_popup = true;
					}
					ImGui::SameLine();
					if (ImGui::Button("no##save_level_popup_file_exists_popup_no")) {
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
				if (close_popup) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}
		{ // entity window
			if (ImGui::Begin("Entity##entity_window")) {
				ImGui::PushID("entitiy_window");
				const char *entity_combo_name = editor.entity_index < level.entity_count ? level.entity_infos[editor.entity_index].name : nullptr;
				if (ImGui::BeginCombo("##entities_combo", entity_combo_name)) {
					if (ImGui::Selectable("", editor.entity_index >= level.entity_count)) {
						editor.entity_index = level.entity_count;
					}
					for (uint32 i = 0; i < level.entity_count; i += 1) {
						if (ImGui::Selectable(level.entity_infos[i].name, editor.entity_index == i)) {
							editor.entity_index = i;
						}
					}
					ImGui::EndCombo();
				}
				ImGui::SameLine();
				if (ImGui::Button("New Entity##new_entity_button")) {
					ImGui::OpenPopup("##new_entity_popup");
				}
				if (ImGui::BeginPopupModal("##new_entity_popup", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
					static bool show_duplicate_name_error = false;
					static char entity_name_buf[sizeof(entity_info::name)] = {};
					ImGui::InputText("name##new_entity_popup_name", entity_name_buf, sizeof(entity_name_buf));
					if (show_duplicate_name_error) {
						ImGui::TextColored({1, 0, 0, 1}, "error: entity name already exist");
					}
					if (ImGui::Button("ok##new_entity_popup_ok")) {
						if (strcmp(entity_name_buf, "")) {
							for (uint32 i = 0; i < level.entity_count; i += 1) {
								if (!strcmp(level.entity_infos[i].name, entity_name_buf)) {
									show_duplicate_name_error = true;
									break;
								}
							}
							if (!show_duplicate_name_error) {
								entity_addition *entity_addition = memory_arena_allocate<struct entity_addition>(&level.frame_memory_arena, 1);
								array_copy(entity_addition->info.name, entity_name_buf);
								entity_addition->transform = transform_identity();
								list_prepend(&level.entity_addition, entity_addition);
								show_duplicate_name_error = false;
								ImGui::CloseCurrentPopup();
							}
						}
					}
					ImGui::SameLine();
					if (ImGui::Button("cancel##new_entity_popup_cancel")) {
						show_duplicate_name_error = false;
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
				ImGui::Separator();
				if (editor.entity_index >= level.entity_count) {
					goto entity_window_skip_components_label;
				}
				uint32 *entity_flag = &level.entity_flags[editor.entity_index];
				{
					ImGui::PushID("basic_properties");
					if (ImGui::CollapsingHeader("Basic Properties##collapsing_header")) {
						transform *entity_transform = &level.entity_transforms[editor.entity_index];
						ImGui::InputFloat3("translate##transform_translate_field", entity_transform->translate.e, 3);
						ImGui::SameLine(ImGui::GetWindowWidth() - 35);
						ImGui::RadioButton("##transform_translate_gizmo", &editor.entity_gizmo, editor_gizmo_transform_translate);
						if (ImGui::InputFloat4("rotate##transform_rotate_field", entity_transform->rotate.e, 3)) {
							entity_transform->rotate = quat_normalize(entity_transform->rotate);
						}
						ImGui::SameLine(ImGui::GetWindowWidth() - 35);
						ImGui::RadioButton("##transform_rotate_gizmo", &editor.entity_gizmo, editor_gizmo_transform_rotate);
						ImGui::InputFloat3("scale##transform_scale_field", entity_transform->scale.e, 3);
						ImGui::SameLine(ImGui::GetWindowWidth() - 35);
						ImGui::RadioButton("##transform_scale_gizmo", &editor.entity_gizmo, editor_gizmo_transform_scale);
					}
					ImGui::PopID();
				}
				if (*entity_flag & component_flag_render) {
					ImGui::PushID("render_component");
					if (ImGui::CollapsingHeader("Render Component##collapsing_header")) {
						render_component *render_component = entity_get_render_component(&level, editor.entity_index);
						const char *model_combo_name = (render_component->model_index < level.model_count) ? level.models[render_component->model_index].file_name : nullptr;
						if (ImGui::BeginCombo("models##models_combo", model_combo_name)) {
							// if (ImGui::Selectable("", render_component->model_index >= level.model_count)) {
							// 	render_component->model_index = level.model_count;
							// }
							for (uint32 i = 0; i < level.model_count; i += 1) {
								if (ImGui::Selectable(level.models[i].file_name, render_component->model_index == i)) {
									render_component->model_index = i;
									editor.entity_render_component_mesh_index = UINT32_MAX;
								}
							}
							ImGui::EndCombo();
						}
						if (render_component->model_index < level.model_count) {
							model *model = &level.models[render_component->model_index];
							const char *mesh_combo_name = (editor.entity_render_component_mesh_index < model->mesh_count) ? model->meshes[editor.entity_render_component_mesh_index].name : nullptr;
							if (ImGui::BeginCombo("meshes##model_meshes_combo", mesh_combo_name)) {
								if (ImGui::Selectable("", editor.entity_render_component_mesh_index >= model->mesh_count)) {
									editor.entity_render_component_mesh_index = model->mesh_count;
								}
								for (uint32 i = 0; i < model->mesh_count; i += 1) {
									if (ImGui::Selectable(model->meshes[i].name, editor.entity_render_component_mesh_index == i)) {
										editor.entity_render_component_mesh_index = i;
									}
								}
								ImGui::EndCombo();
							}
						}
						if (render_component->model_index < level.model_count) {
							ImGui::SliderFloat("uv scale u##material_uv_scale_u", &render_component->uv_scale[0], 1.0f, 10.0f);
							ImGui::SliderFloat("uv scale v##material_uv_scale_v", &render_component->uv_scale[1], 1.0f, 10.0f);
							ImGui::SliderFloat("height map scale##material_height_map_scale", &render_component->height_map_scale, 0.0f, 0.2f);
						}
					}
					ImGui::PopID();
				}
				if (*entity_flag & component_flag_collision) {
					ImGui::PushID("collision_component");
					if (ImGui::CollapsingHeader("Collision Component##collapsing_header")) {
						collision_component *collision_component = entity_get_collision_component(&level, editor.entity_index);
						const char *plane_combo_name = (editor.entity_collision_component_plane_index < collision_component->plane_count) ? "placeholder" : nullptr;
						if (ImGui::BeginCombo("planes##planes_combo", plane_combo_name)) {
							for (uint32 i = 0; i < collision_component->plane_count; i += 1) {
								if (ImGui::Selectable("placeholder", editor.entity_collision_component_plane_index == i)) {
									editor.entity_collision_component_plane_index = i;
								}
							}
							ImGui::EndCombo();
						}
						if (editor.entity_collision_component_plane_index < collision_component->plane_count) {
							plane *plane = &collision_component->planes[editor.entity_collision_component_plane_index];
							ImGui::InputFloat3("normal##plane_normal_field", plane->normal.e, 3);
							ImGui::InputFloat("distance##plane_distance_field", &plane->distance, 3);
						}
						const char *sphere_combo_name = (editor.entity_collision_component_sphere_index < collision_component->sphere_count) ? "placeholder" : nullptr;
						if (ImGui::BeginCombo("spheres##spheres_combo", sphere_combo_name)) {
							for (uint32 i = 0; i < collision_component->sphere_count; i += 1) {
								if (ImGui::Selectable("placeholder", editor.entity_collision_component_sphere_index == i)) {
									editor.entity_collision_component_sphere_index = i;
								}
							}
							ImGui::EndCombo();
						}
						if (editor.entity_collision_component_sphere_index < collision_component->sphere_count) {
							sphere *sphere = &collision_component->spheres[editor.entity_collision_component_sphere_index];
							ImGui::InputFloat3("center##sphere_center_field", sphere->center.e, 3);
							ImGui::InputFloat("radius##sphere_radius_field", &sphere->radius, 3);
						}
						const char *bound_combo_name = (editor.entity_collision_component_bound_index < collision_component->bound_count) ? "placeholder" : nullptr;
						if (ImGui::BeginCombo("bounds##bounds_combo", bound_combo_name)) {
							for (uint32 i = 0; i < collision_component->bound_count; i += 1) {
								if (ImGui::Selectable("placeholder", editor.entity_collision_component_bound_index == i)) {
									editor.entity_collision_component_bound_index = i;
								}
							}
							ImGui::EndCombo();
						}
						if (editor.entity_collision_component_bound_index < collision_component->bound_count) {
							aa_bound *bound = &collision_component->bounds[editor.entity_collision_component_bound_index];
							ImGui::InputFloat3("min##bound_min_field", bound->min.e, 3);
							ImGui::InputFloat3("max##bound_max_field", bound->max.e, 3);
						}
					}
					ImGui::PopID();
				}
				if (*entity_flag & component_flag_light) {
					ImGui::PushID("light_component");
					if (ImGui::CollapsingHeader("Light Component##collapsing_header")) {
						light_component *light_component = entity_get_light_component(&level, editor.entity_index);
						if (light_component->light_type == light_type_ambient) {
							ImGui::ColorEdit3("color##ambient_light_color", light_component->ambient_light.color.e);
						}
						else if (light_component->light_type == light_type_directional) {
							if (ImGui::InputFloat3("direction##directional_light_direction_field", light_component->directional_light.direction.e, 3)) {
								light_component->directional_light.direction = vec3_normalize(light_component->directional_light.direction);
							}
							ImGui::SameLine(ImGui::GetWindowWidth() - 35);
							ImGui::RadioButton("##directional_light_rotate_gizmo", &editor.entity_gizmo, editor_gizmo_directional_light_rotate);
							ImGui::ColorEdit3("color##directional_light_color", light_component->directional_light.color.e);
						}
						else if (light_component->light_type == light_type_point) {
							ImGui::InputFloat3("position##point_light_position_field", light_component->point_light.position.e, 3);
							ImGui::SameLine(ImGui::GetWindowWidth() - 35);
							ImGui::RadioButton("##point_light_translate_gizmo", &editor.entity_gizmo, editor_gizmo_point_light_translate);
							ImGui::InputFloat("attenuation##point_light_attenuation_field", &light_component->point_light.attenuation);
							ImGui::ColorEdit3("color##point_light_color_field", light_component->point_light.color.e);
						}
					}
					ImGui::PopID();
				}
				if (*entity_flag & component_flag_physics) {
					ImGui::PushID("physics_component");
					if (ImGui::CollapsingHeader("Physics Component##collapsing_header")) {
						physics_component *physics_component = entity_get_physics_component(&level, editor.entity_index);
						ImGui::InputFloat3("velocity##velocity_field", physics_component->velocity.e);
					}
					ImGui::PopID();
				}
				ImGui::Dummy({0, 16});
				if (ImGui::Button("New Component##new_component_button")) {
					ImGui::OpenPopup("##new_component_popup");
				}
				if (ImGui::BeginPopupModal("##new_component_popup", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
					static const char *component_types[] = {"render", "collision", "light", "physics"};
					static const char *light_types[] = {"ambient", "directional", "point"};
					static uint32 component_type_index = 0;
					static uint32 light_type_index = 0;
					static bool show_duplicate_component_error = false;

					const char *component_combo_name = component_types[component_type_index];
					if (ImGui::BeginCombo("component##new_component_popup_component_combo", component_combo_name)) {
						for (uint32 i = 0; i < m_countof(component_types); i += 1) {
							if (ImGui::Selectable(component_types[i], component_type_index == i)) {
								component_type_index = i;
							}
						}
						ImGui::EndCombo();
					}
					if (component_type_index == 2) {
						const char *light_combo_type = light_types[light_type_index];
						if (ImGui::BeginCombo("type##new_component_popup_light_type_combo", light_combo_type)) {
							for (uint32 i = 0; i < m_countof(light_types); i += 1) {
								if (ImGui::Selectable(light_types[i], light_type_index == i)) {
									light_type_index = i;
								}
							}
							ImGui::EndCombo();
						}
					}
					if (show_duplicate_component_error) {
						ImGui::TextColored({1, 0, 0, 1}, "error: component already exist");
					}
					if (ImGui::Button("ok##new_component_popup_ok")) {
						if ((component_type_index == 0 && *entity_flag & component_flag_render) ||
								(component_type_index == 1 && *entity_flag & component_flag_collision) ||
								(component_type_index == 2 && *entity_flag & component_flag_light) ||
								(component_type_index == 3 && *entity_flag & component_flag_physics)) {
							show_duplicate_component_error = true;
						}
						else {
							show_duplicate_component_error = false;
							if (component_type_index == 0) {
								uint32 *entity_flag = memory_arena_allocate<uint32>(&level.frame_memory_arena, 1);
								*entity_flag = level.entity_flags[editor.entity_index] | component_flag_render;
								render_component *render_component = memory_arena_allocate<struct render_component>(&level.frame_memory_arena, 1);
								render_component->uv_scale = {1, 1};
								level.entity_modifications[editor.entity_index].flag = entity_flag;
								level.entity_modifications[editor.entity_index].render_component = render_component;
							}
							else if (component_type_index == 1) {
								uint32 *entity_flag = memory_arena_allocate<uint32>(&level.frame_memory_arena, 1);
								*entity_flag = level.entity_flags[editor.entity_index] | component_flag_collision;
								collision_component *collision_component = memory_arena_allocate<struct collision_component>(&level.frame_memory_arena, 1);
								level.entity_modifications[editor.entity_index].flag = entity_flag;
								level.entity_modifications[editor.entity_index].collision_component = collision_component;
							}
							else if (component_type_index == 2) {
								uint32 *entity_flag = memory_arena_allocate<uint32>(&level.frame_memory_arena, 1);
								*entity_flag = level.entity_flags[editor.entity_index] | component_flag_light;
								light_component *light_component = memory_arena_allocate<struct light_component>(&level.frame_memory_arena, 1);
								if (light_type_index == 0) {
									light_component->light_type = {light_type_ambient};
									light_component->ambient_light = ambient_light{{0.1f, 0.1f, 0.1f}};
								}
								else if (light_type_index == 1) {
									light_component->light_type = {light_type_directional};
									light_component->directional_light = directional_light{{0.1f, 0.1f, 0.1f}, {0, -1, 0}};
								}
								else if (light_type_index == 2) {
									light_component->light_type = {light_type_point};
									light_component->point_light = point_light{{0.1f, 0.1f, 0.1f}, {0, 0, 0}, 2};
								}
								level.entity_modifications[editor.entity_index].flag = entity_flag;
								level.entity_modifications[editor.entity_index].light_component = light_component;
							}
							else if (component_type_index == 3) {
								uint32 *entity_flag = memory_arena_allocate<uint32>(&level.frame_memory_arena, 1);
								*entity_flag = level.entity_flags[editor.entity_index] | component_flag_physics;
								physics_component *physics_component = memory_arena_allocate<struct physics_component>(&level.frame_memory_arena, 1);
								level.entity_modifications[editor.entity_index].flag = entity_flag;
								level.entity_modifications[editor.entity_index].physics_component = physics_component;
							}
							ImGui::CloseCurrentPopup();
						}
					}
					ImGui::SameLine();
					if (ImGui::Button("cancel##new_component_popup_cancel")) {
						show_duplicate_component_error = false;
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
				if (ImGui::Button("Rename##rename_button")) {
					ImGui::OpenPopup("##rename_popup");
				}
				if (ImGui::BeginPopupModal("##rename_popup", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
					static bool show_duplicate_name_error = false;
					static char name_buf[sizeof(entity_info::name)] = {};
					ImGui::InputText("new name##rename_pop_up_new_name", name_buf, sizeof(name_buf));
					if (show_duplicate_name_error) {
						ImGui::TextColored({1, 0, 0, 1}, "error: entity name already exist");
					}
					if (ImGui::Button("ok##rename_popup_ok")) {
						for (uint32 i = 0; i < level.entity_count; i += 1) {
							if (!strcmp(level.entity_infos[i].name, name_buf)) {
								show_duplicate_name_error = true;
								break;
							}
						}
						if (!show_duplicate_name_error) {
							strcpy(level.entity_infos[editor.entity_index].name, name_buf);
							array_set(name_buf, '\0');
							ImGui::CloseCurrentPopup();
						}
					}
					ImGui::SameLine();
					if (ImGui::Button("cancel##rename_popup_cancel")) {
						show_duplicate_name_error = false;
						array_set(name_buf, '\0');
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
				if (ImGui::Button("Delete##delete_button")) {
					level.entity_modifications[editor.entity_index].remove = true;
				}
			entity_window_skip_components_label:
				ImGui::PopID();
			}
			ImGui::End();
		}
		{ // skybox window
			if (ImGui::Begin("Skyboxes##skyboxes_window")) {
				const char *skybox_combo_name = (level.skybox_index < level.skybox_count) ? level.skyboxes[level.skybox_index].file_name : nullptr;
				if (ImGui::BeginCombo("skyboxes##skyboxes_combo", skybox_combo_name)) {
					for (uint32 i = 0; i < level.skybox_count; i += 1) {
						if (ImGui::Selectable(level.skyboxes[i].file_name, level.skybox_index == i)) {
							level.skybox_index = i;
						}
					}
					ImGui::EndCombo();
				}
			}
			ImGui::End();
		}
		{ // memory window
			if (ImGui::Begin("Memory Usage##memory_usage_window")) {
				auto imgui_render_memory = [](uint64 memory_size, uint64 memory_capacity, const char *memory_name) {
					char overlay[32] = {};
					if (memory_size < m_kilobytes(100)) {
						snprintf(overlay, sizeof(overlay), "%lld bytes / %.1f mb", memory_size, (double)memory_capacity / m_megabytes(1));
					}
					else {
						snprintf(overlay, sizeof(overlay), "%.1f mb / %.1f mb", (double)memory_size / m_megabytes(1), (double)memory_capacity / m_megabytes(1));
					}
					ImGui::ProgressBar((float)((double)memory_size / (double)memory_capacity), ImVec2{ImGui::GetWindowContentRegionWidth() * 0.5f, 0}, overlay);
					ImGui::SameLine();
					ImGui::Text(memory_name);
				};
				ImGui::Text("Memory Arenas");
				memory_arena *entity_components_memory_arena = &level.entity_components_memory_arenas[level.entity_components_memory_arena_index];
				imgui_render_memory(entity_components_memory_arena->size, entity_components_memory_arena->capacity, entity_components_memory_arena->name);
				imgui_render_memory(level.assets_memory_arena.size, level.assets_memory_arena.capacity, level.assets_memory_arena.name);
				ImGui::Text("Vulkan Memories");
				imgui_render_memory(vulkan.memories.common_images_memory.size, vulkan.memories.common_images_memory.capacity, "common images");
				imgui_render_memory(vulkan.memories.level_images_memory.size, vulkan.memories.level_images_memory.capacity, "level images");
				imgui_render_memory(vulkan.buffers.level_vertex_buffer_offset, vulkan.buffers.level_vertex_buffer.capacity, "level vertices");
				imgui_render_memory(vulkan.buffers.frame_vertex_buffer_offsets[vulkan.frame_index], vulkan.buffers.frame_vertex_buffers[vulkan.frame_index].capacity, "frame vertices");
				imgui_render_memory(vulkan.buffers.frame_uniform_buffer_offsets[vulkan.frame_index], vulkan.buffers.frame_uniform_buffers[vulkan.frame_index].capacity, "frame uniforms");
			}
			ImGui::End();
		}
		{ // popups
			static bool camera_move_speed_popup = false;
			if (!ImGui::GetIO().WantCaptureMouse && editor.camera_moving) {
				if (ImGui::GetIO().MouseWheel != 0) {
					ImGui::OpenPopup("##camera_speed_popup");
					camera_move_speed_popup = true;
				}
			}
			if (ImGui::BeginPopupModal("##camera_speed_popup", &camera_move_speed_popup, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
				float min_speed = 0.1f;
				float max_speed = 100;
				editor.camera_move_speed += ImGui::GetIO().MouseWheel;
    		ImGui::SliderFloat("camera speed##camera_speed_slider", &editor.camera_move_speed, min_speed, max_speed);
				ImGui::EndPopup();
			}
		}
		{ // gizmo
			if (editor.entity_index < level.entity_count) {
				uint32 *entity_flag = &level.entity_flags[editor.entity_index];
				transform *entity_transform = &level.entity_transforms[editor.entity_index];
				mat4 transform_mat = transform_to_mat4(*entity_transform);
				mat4 camera_view_mat = camera_view_mat4(editor.camera);
				mat4 camera_proj_mat = camera_projection_mat4(editor.camera);
				if (editor.entity_gizmo == editor_gizmo_transform_translate) {
					ImGuizmo::BeginFrame();
					ImGuizmo::Manipulate(mat4_float_ptr(camera_view_mat), mat4_float_ptr(camera_proj_mat), ImGuizmo::TRANSLATE, ImGuizmo::WORLD, mat4_float_ptr(transform_mat));
					entity_transform->translate = mat4_get_translation(transform_mat);
				}
				else if (editor.entity_gizmo == editor_gizmo_transform_rotate) {
					ImGuizmo::BeginFrame();
					ImGuizmo::Manipulate(mat4_float_ptr(camera_view_mat), mat4_float_ptr(camera_proj_mat), ImGuizmo::ROTATE, ImGuizmo::WORLD, mat4_float_ptr(transform_mat));
					entity_transform->rotate = quat_from_mat4(transform_mat);
				}
				else if (editor.entity_gizmo == editor_gizmo_transform_scale) {
					ImGuizmo::BeginFrame();
					ImGuizmo::Manipulate(mat4_float_ptr(camera_view_mat), mat4_float_ptr(camera_proj_mat), ImGuizmo::SCALE, ImGuizmo::WORLD, mat4_float_ptr(transform_mat));
					entity_transform->scale = mat4_get_scaling(transform_mat);
				}
				else if (editor.entity_gizmo == editor_gizmo_directional_light_rotate && *entity_flag & component_flag_light) {
					light_component *light_component = entity_get_light_component(&level, editor.entity_index);
					if (light_component->light_type == light_type_directional) {
						ImGuizmo::BeginFrame();
						transform transform = transform_identity();
						transform.translate = editor.camera.position + editor.camera.view * 16;
						mat4 transform_mat = transform_to_mat4(transform);
						ImGuizmo::Manipulate(mat4_float_ptr(camera_view_mat), mat4_float_ptr(camera_proj_mat), ImGuizmo::ROTATE, ImGuizmo::WORLD, mat4_float_ptr(transform_mat));
						light_component->directional_light.direction = vec3_normalize(quat_from_mat4(transform_mat) * light_component->directional_light.direction);

						ImDrawList *draw_list = ImGuizmo::gContext.mDrawList;
						vec3 line_begin_world = transform.translate;
						vec3 line_end_world = line_begin_world + light_component->directional_light.direction * 1.25f;
						vec4 line_begin = camera_proj_mat * camera_view_mat * vec4{line_begin_world.x, line_begin_world.y, line_begin_world.z, 1};
						vec4 line_end = camera_proj_mat * camera_view_mat * vec4{line_end_world.x, line_end_world.y, line_end_world.z, 1};
						line_begin /= line_begin.w;
						line_end /= line_end.w;
						line_begin.x = line_begin.x * 0.5f + 0.5f;
						line_begin.y = -line_begin.y * 0.5f + 0.5f;
						line_end.x = line_end.x * 0.5f + 0.5f;
						line_end.y = -line_end.y * 0.5f + 0.5f;

						ImVec2 line_begin_imgui = {ImGui::GetIO().DisplaySize.x * line_begin.x, ImGui::GetIO().DisplaySize.y * line_begin.y};
						ImVec2 line_end_imgui = {ImGui::GetIO().DisplaySize.x * line_end.x, ImGui::GetIO().DisplaySize.y * line_end.y};

						draw_list->AddLine(line_begin_imgui, line_end_imgui, 0xffffffff, 6);
						draw_list->AddCircleFilled(line_end_imgui, 12, 0xff00ffff);
					}
				}
				else if (editor.entity_gizmo == editor_gizmo_point_light_translate && *entity_flag & component_flag_light) {
					light_component *light_component = entity_get_light_component(&level, editor.entity_index);
					if (light_component->light_type == light_type_point) {
						ImGuizmo::BeginFrame();
						transform transform = transform_identity();
						transform.translate = light_component->point_light.position;
						mat4 transform_mat = transform_to_mat4(transform);
						ImGuizmo::Manipulate(mat4_float_ptr(camera_view_mat), mat4_float_ptr(camera_proj_mat), ImGuizmo::TRANSLATE, ImGuizmo::WORLD, mat4_float_ptr(transform_mat));
						light_component->point_light.position = mat4_get_translation(transform_mat);
					}
				}
			}
		}
		ImGui::Render();

		auto generate_editor_render_data = [&] {
			editor.render_data = {};
			vulkan.buffers.frame_vertex_buffer_offsets[vulkan.frame_index] = round_up(vulkan.buffers.frame_vertex_buffer_offsets[vulkan.frame_index], 16);
			editor.render_data.lines_frame_vertex_buffer_offset = vulkan.buffers.frame_vertex_buffer_offsets[vulkan.frame_index];
			struct line_point {
				vec3 position;
				u8vec4 color;
			};
			static_assert(sizeof(struct line_point) == 16, "");
			line_point *line_points = (struct line_point *)(vulkan.buffers.frame_vertex_buffer_ptrs[vulkan.frame_index] + editor.render_data.lines_frame_vertex_buffer_offset);
			// { // entity bound
			// 	u8vec4 color = u8vec4{255, 0, 0, 255};
			// 	vec3 size = editor.entity_bound.max - editor.entity_bound.min;
			// 	vec3 points[8];
			// 	points[0] = editor.entity_bound.min;
			// 	points[1] = points[0] + vec3{size.x, 0, 0};
			// 	points[2] = points[1] + vec3{0, 0, size.z};
			// 	points[3] = points[2] - vec3{size.x, 0, 0};
			// 	points[4] = points[0] + vec3{0, size.y, 0};
			// 	points[5] = points[1] + vec3{0, size.y, 0};
			// 	points[6] = points[2] + vec3{0, size.y, 0};
			// 	points[7] = points[3] + vec3{0, size.y, 0};
			// 	line_point bound_points[] = {{points[0], color}, {points[1], color}, {points[1], color}, {points[2], color}, {points[2], color}, {points[3], color}, {points[3], color}, {points[0], color},  // buttom
			// 															 {points[4], color}, {points[5], color}, {points[5], color}, {points[6], color}, {points[6], color}, {points[7], color}, {points[7], color}, {points[4], color},  // top
			// 															 {points[0], color}, {points[4], color}, {points[1], color}, {points[5], color}, {points[2], color}, {points[6], color}, {points[3], color}, {points[7], color}}; // columns
			// 	memcpy(line_points, bound_points, sizeof(bound_points));
			// 	line_points += m_countof(bound_points);
			// 	editor.render_data.lines_vertex_count += m_countof(bound_points);
			// 	vulkan.buffers.frame_vertex_buffer_offsets[vulkan.frame_index] += sizeof(bound_points);
			// }
			{ // entity mesh outline
				if (editor.entity_index < level.entity_count && level.entity_flags[editor.entity_index] & component_flag_render) {
					render_component *render_component = entity_get_render_component(&level, editor.entity_index);
					if (render_component->model_index < level.model_count && editor.entity_render_component_mesh_index < level.models[render_component->model_index].mesh_count) {
						for (uint32 i = 0; i < level.render_data.model_count; i += 1) {
							if (level.render_data.models[i].model_index == render_component->model_index) {
								level.render_data.models[i].meshes_render_data[editor.entity_render_component_mesh_index].render_vertices_outline = true;
							}
						}														
					}
				}
			}
			{ // reference grid
				u8vec4 color = u8vec4{200, 200, 200, 255};
				line_point horizontal_points[20];
				float horizontal_z = -4.5f;
				for (uint32 i = 0; i < m_countof(horizontal_points) / 2; i += 1) {
					horizontal_points[i * 2] = {{-4.5f, 0, horizontal_z}, color};
					horizontal_points[i * 2 + 1] = {{4.5f, 0, horizontal_z}, color};
					horizontal_z += 1;
				}
				line_point vertical_points[20];
				float vertical_x = -4.5f;
				for (uint32 i = 0; i < m_countof(vertical_points) / 2; i += 1) {
					vertical_points[i * 2] = {{vertical_x, 0, -4.5f}, color};
					vertical_points[i * 2 + 1] = {{vertical_x, 0, 4.5f}, color};
					vertical_x += 1;
				}
				memcpy(line_points, horizontal_points, sizeof(horizontal_points));
				memcpy(line_points + m_countof(horizontal_points), vertical_points, sizeof(vertical_points));
				line_points += m_countof(horizontal_points) + m_countof(vertical_points);
				editor.render_data.lines_vertex_count += m_countof(horizontal_points) + m_countof(vertical_points);
				vulkan.buffers.frame_vertex_buffer_offsets[vulkan.frame_index] += sizeof(horizontal_points) + sizeof(vertical_points);
			}
			{ // imgui
				vulkan.buffers.frame_vertex_buffer_offsets[vulkan.frame_index] = round_up(vulkan.buffers.frame_vertex_buffer_offsets[vulkan.frame_index], (uint32)sizeof(ImDrawVert));
				editor.render_data.imgui_frame_vertex_buffer_offset = vulkan.buffers.frame_vertex_buffer_offsets[vulkan.frame_index];
				ImDrawData *imgui_draw_data = ImGui::GetDrawData();
				for (int32 i = 0; i < imgui_draw_data->CmdListsCount; i += 1) {
					ImDrawList *dlist = imgui_draw_data->CmdLists[i];
					uint32 vertices_size = dlist->VtxBuffer.Size * sizeof(ImDrawVert);
					uint32 indices_size = dlist->IdxBuffer.Size * sizeof(ImDrawIdx);
					memcpy(vulkan.buffers.frame_vertex_buffer_ptrs[vulkan.frame_index] + vulkan.buffers.frame_vertex_buffer_offsets[vulkan.frame_index], dlist->VtxBuffer.Data, vertices_size);
					memcpy(vulkan.buffers.frame_vertex_buffer_ptrs[vulkan.frame_index] + vulkan.buffers.frame_vertex_buffer_offsets[vulkan.frame_index] + vertices_size, dlist->IdxBuffer.Data, indices_size);
					vulkan.buffers.frame_vertex_buffer_offsets[vulkan.frame_index] += round_up(vertices_size + indices_size, (uint32)sizeof(ImDrawVert));
				}
			}
		};
   	auto extra_main_render_pass_render_commands = [&] {
			VkCommandBuffer cmd_buffer = vulkan.cmd_buffers.graphic_cmd_buffers[vulkan.frame_index];
			vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan.pipelines.lines_pipeline.pipeline);
			VkDeviceSize vertices_offset = 0;
			vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &vulkan.buffers.frame_vertex_buffers[vulkan.frame_index].buffer, &vertices_offset);
			uint32 uniform_buffer_offsets[3] = {level.render_data.common_data_frame_uniform_buffer_offset, 0, 0};
			vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan.pipelines.lines_pipeline.layout, 0, 1, &(vulkan.descriptors.frame_uniform_buffer_offsets[vulkan.frame_index]), m_countof(uniform_buffer_offsets), uniform_buffer_offsets);
			vkCmdDraw(cmd_buffer, editor.render_data.lines_vertex_count, 1, editor.render_data.lines_frame_vertex_buffer_offset / 16, 0);
		};
   	auto extra_swap_chain_render_commands = [&] {
			VkCommandBuffer cmd_buffer = vulkan.cmd_buffers.graphic_cmd_buffers[vulkan.frame_index];
			vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan.pipelines.imgui_pipeline.pipeline);
			VkDeviceSize vertices_offset = 0;
			vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &vulkan.buffers.frame_vertex_buffers[vulkan.frame_index].buffer, &vertices_offset);
			vkCmdBindIndexBuffer(cmd_buffer, vulkan.buffers.frame_vertex_buffers[vulkan.frame_index].buffer, vertices_offset, VK_INDEX_TYPE_UINT16);
			vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan.pipelines.imgui_pipeline.layout, 0, 1, &vulkan.descriptors.imgui_font_atlas_image, 0, nullptr);
			vec2 push_consts = {(float)vulkan.swap_chain.image_width, (float)vulkan.swap_chain.image_height};
			vkCmdPushConstants(cmd_buffer, vulkan.pipelines.imgui_pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_consts), &push_consts);
			ImDrawData *imgui_draw_data = ImGui::GetDrawData();
			for (int32 i = 0; i < imgui_draw_data->CmdListsCount; i += 1) {
				ImDrawList *dlist = imgui_draw_data->CmdLists[i];
				uint32 vertices_size = dlist->VtxBuffer.Size * sizeof(ImDrawVert);
				uint32 elements_size = dlist->IdxBuffer.Size * sizeof(ImDrawIdx);
				uint32 vertex_index = editor.render_data.imgui_frame_vertex_buffer_offset / sizeof(ImDrawVert);
				uint32 element_index = (editor.render_data.imgui_frame_vertex_buffer_offset + vertices_size) / sizeof(ImDrawIdx);
				for (int32 i = 0; i < dlist->CmdBuffer.Size; i += 1) {
				 	ImDrawCmd *dcmd = &dlist->CmdBuffer.Data[i];
					VkRect2D scissor = {{(int32)dcmd->ClipRect.x, (int32)dcmd->ClipRect.y}, {(uint32)(dcmd->ClipRect.z - dcmd->ClipRect.x), (uint32)(dcmd->ClipRect.w - dcmd->ClipRect.y)}};
					vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);
					vkCmdDrawIndexed(cmd_buffer, dcmd->ElemCount, 1, element_index, vertex_index, 0);
					element_index += dcmd->ElemCount;
				}
				editor.render_data.imgui_frame_vertex_buffer_offset += round_up(vertices_size + elements_size, (uint32)sizeof(ImDrawVert));
			}
		};
		vulkan_begin_render(&vulkan);
		level_generate_render_data(&level, &vulkan, editor.camera, generate_editor_render_data);
		level_generate_render_commands(&level, &vulkan, editor.camera, extra_main_render_pass_render_commands, extra_swap_chain_render_commands);
		bool screen_shot = ImGui::IsKeyReleased(keycode_print_screen);
		vulkan_end_render(&vulkan, screen_shot);

		level_entity_component_end_frame(&level);
		level.frame_memory_arena.size = 0;
	
		QueryPerformanceCounter(&performance_counters[1]);
		last_frame_time_microsec = (performance_counters[1].QuadPart - performance_counters[0].QuadPart) * 1000000 / performance_frequency.QuadPart;
		last_frame_time_sec = (double)last_frame_time_microsec / 1000000;
	}
	ImGui::Shutdown();
}
