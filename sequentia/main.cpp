// ImGui - standalone example application for SDL2 + OpenGL
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui.h>
#include "imgui_impl_sdl_gl3.h"
#include <stdio.h>
#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <SDL.h>
#include "decoder.h";
#include "SeqProject.h";
#include "SeqUISequencer.h";

#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")

static const int video_count = 1;
extern Material g_VideoMaterial[video_count];
static Decoder decoder[video_count];
static SeqProject *project;

void CreateFrameTexture(AVFrame* frame, GLuint texId[3])
{
	// Store current state
	GLint last_texture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);

	glDeleteTextures(3, &texId[0]);
	glGenTextures(3, &texId[0]);

	glBindTexture(GL_TEXTURE_2D, texId[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width, frame->height, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[0]);

	glBindTexture(GL_TEXTURE_2D, texId[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[1]);

	glBindTexture(GL_TEXTURE_2D, texId[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[2]);

	// Restore state
	glBindTexture(GL_TEXTURE_2D, last_texture);
}

int main(int, char**)
{
	project = new SeqProject();
	for (int i = 0; i < 20; i++)
	{
		project->AddAction(
			SeqAction(SeqActionType::AddChannel,
				new SeqActionAddChannel(SeqChannelType::Video, "Video")));
		project->AddAction(
			SeqAction(SeqActionType::AddChannel,
				new SeqActionAddChannel(SeqChannelType::Audio, "Audio")));
	}
	
	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	// Setup window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);
	SDL_Window *window = SDL_CreateWindow("Sequentia", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	gl3wInit();
	
	// Setup ImGui binding
	ImGui_ImplSdlGL3_Init(window);
	
	// Setup the video decoder and prepare the video material
	// long videos
	//decoder[0].Open("D:/Camera/QuickDecodeTest.mkv");
	//decoder[0].Open("D:/Camera/Video/20170521_032735A.mp4");
	decoder[0].skip_frames_if_slow = true;
	//decoder[1].Open("D:/Camera/Video/20170521_032933A.mp4");
	//decoder[2].Open("D:/Camera/Video/20170602_084013A.mp4");
	//decoder[3].Open("D:/Camera/Video/20170509_195301A.mp4");
	// short videos
	decoder[0].Open("D:/Camera/Video/20170602_085418A.mp4");
	//decoder[1].Open("D:/Camera/Video/20170524_035547A.mp4");
	//decoder[2].Open("D:/Camera/Video/20170602_080143A.mp4");
	//decoder[3].Open("D:/Camera/Video/20170602_084612A.mp4");
	for (int i = 0; i < video_count; i++)
		g_VideoMaterial[i].textureCount = 3;
	ImVec2 vid_size = ImVec2(1000, 1000.0 * (9.0 / 16.0));
	float margin = 10;
	AVFrame* prev_frame[video_count];
	Uint32 video_start_time[video_count] = { 0 };

	// Load Fonts
	// (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
	//ImGuiIO& io = ImGui::GetIO();
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

	bool show_test_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImColor(114, 144, 154);
	ImGui::GetIO().MouseDrawCursor = true;

	// Main loop
	bool done = false;
	int seek_video_time = 0;
	while (!done)
	{
		// handle sdl events
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSdlGL3_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
		}
		ImGui_ImplSdlGL3_NewFrame(window);

		// 1. Show a simple window
		// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
		{
			/*
			ImGui::Begin("Debug");
			static float f = 0.0f;
			ImGui::Text("Hello, world!");
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
			ImGui::ColorEdit3("clear color", (float*)&clear_color);
			if (ImGui::Button("Test Window")) show_test_window ^= 1;
			if (ImGui::Button("Another Window")) show_another_window ^= 1;
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
			*/

			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("Project"))
				{
					if (ImGui::MenuItem("New", "Ctrl+N", false, false)) {}
					if (ImGui::MenuItem("Open", "Ctrl+O", false, false)) {}
					ImGui::Separator();
					if (ImGui::MenuItem("Save", "Ctrl+S")) { project->Save(); }
					if (ImGui::MenuItem("Save As", "")) { project->SaveAs(); }
					ImGui::Separator();
					if (ImGui::MenuItem("Exit", "", false, false)) {}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Edit"))
				{
					if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {}
					if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {}
					ImGui::Separator();
					if (ImGui::MenuItem("Cut", "CTRL+X", false, false)) {}
					if (ImGui::MenuItem("Copy", "CTRL+C", false, false)) {}
					if (ImGui::MenuItem("Paste", "CTRL+V", false, false)) {}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Window"))
				{
					if (ImGui::MenuItem("Video", "", false, false)) {}
					if (ImGui::MenuItem("Sequencer", "")) { project->AddSequencer(); }
					if (ImGui::MenuItem("Library", "")) { project->AddLibrary(); }
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			project->Draw();


			// show video windows
			for (int i = 0; i < video_count; i++)
			{
				if (decoder[i].status == DecoderStatus::Ready || decoder[i].status == DecoderStatus::Loading)
				{
					Uint32 video_time = 0;
					if (video_start_time[i] != 0)
						video_time = SDL_GetTicks() - video_start_time[i];
					// get the next video frame
					AVFrame* display_frame = decoder[i].NextFrame(video_time);

					if (display_frame)
					{
						if (video_start_time[i] == 0)
							video_start_time[i] = SDL_GetTicks();

						ImGui::Begin(decoder[i].src_filename);
						if (display_frame != prev_frame[i])
						{
							//printf("%d%s\n", display_frame->coded_picture_number, display_frame->key_frame ? "I" : "p");
							CreateFrameTexture(display_frame, &g_VideoMaterial[i].textureHandles[0]);
						}
						prev_frame[i] = display_frame;
						float tex_w = (float)display_frame->width;
						float tex_h = (float)display_frame->height;

						ImTextureID tex_id = (void*)&g_VideoMaterial[i];
						ImGui::Text("%.0fx%.0f", vid_size.x, vid_size.y);
						float decoder_time = display_frame->pts / 1000.0;
						float video_real_time = (SDL_GetTicks() - video_start_time[i]) / 1000.0;
						ImGui::Text("%.3f (%.3f)", video_real_time, (decoder->buffer_punctuality / 1000.0));
						ImGui::Image(tex_id, vid_size, ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
						if (ImGui::IsItemHovered())
						{
							ImVec2 tex_screen_pos = ImGui::GetCursorScreenPos();
							tex_screen_pos.y -= vid_size.y;
							ImGui::BeginTooltip();
							float focus_sz = 64.0f;
							float focus_x = ImGui::GetMousePos().x - tex_screen_pos.x - focus_sz * 0.5f; if (focus_x < 0.0f) focus_x = 0.0f; else if (focus_x > vid_size.x - focus_sz) focus_x = vid_size.x - focus_sz;
							float focus_y = ImGui::GetMousePos().y - tex_screen_pos.y - focus_sz * 0.5f; if (focus_y < 0.0f) focus_y = 0.0f; else if (focus_y > vid_size.y - focus_sz) focus_y = vid_size.y - focus_sz;
							ImVec2 uv0 = ImVec2((focus_x) / vid_size.x, (focus_y) / vid_size.y);
							ImVec2 uv1 = ImVec2((focus_x + focus_sz) / vid_size.x, (focus_y + focus_sz) / vid_size.y);
							ImGui::Text("Min: (%.2f, %.2f)", focus_x, focus_y);
							ImGui::Text("Max: (%.2f, %.2f)", focus_x + focus_sz, focus_y + focus_sz);
							ImGui::Image(tex_id, ImVec2(256, 256), uv0, uv1, ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
							ImGui::EndTooltip();
						}
						
						static bool isSeeking = false;
						if (isSeeking && ImGui::IsMouseReleased(0))
						{
							printf("seek: %d\n", seek_video_time);
							decoder[i].Seek(seek_video_time);
							video_start_time[i] += video_time - seek_video_time;
							video_time = seek_video_time;
							isSeeking = false;
						}
						seek_video_time = video_time;
						if (ImGui::SliderInt("seek", &seek_video_time, 0, decoder[i].fmt_ctx->duration / 1000))
							isSeeking = true;
						ImGui::SameLine();
						if (ImGui::Button("Skip 1s"))
						{
							seek_video_time = video_time + 1000;
							printf("skip 1s: %d\n", seek_video_time);
							decoder[i].Seek(seek_video_time);
							video_start_time[i] += video_time - seek_video_time;
							video_time = seek_video_time;
						}
						ImGui::SameLine();
						if (ImGui::Button("Skip 10s"))
						{
							seek_video_time = video_time + 10000;
							printf("skip 10s: %d\n", seek_video_time);
							decoder[i].Seek(seek_video_time);
							video_start_time[i] += video_time - seek_video_time;
							video_time = seek_video_time;
						}
						ImGui::SameLine();
						if (ImGui::Button("Skip 60s"))
						{
							seek_video_time = video_time + 60000;
							printf("skip 60s: %d\n", seek_video_time);
							decoder[i].Seek(seek_video_time);
							video_start_time[i] += video_time - seek_video_time;
							video_time = seek_video_time;
						}
						ImGui::End();
					}
				}
			}
		}

		// 2. Show another simple window, this time using an explicit Begin/End pair
		if (show_another_window)
		{
			ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
			ImGui::Begin("Another Window", &show_another_window);
			ImGui::Text("Hello");
			ImGui::End();
		}

		// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
		if (show_test_window)
		{
			ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
			ImGui::ShowTestWindow(&show_test_window);
		}

		// Rendering
		glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	ImGui_ImplSdlGL3_Shutdown();
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
