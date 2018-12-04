#include "Sequentia.h"
#include "SeqRenderer.h"
#include "SeqProjectHeaders.h"
#include "SeqExporter.h"
#include "SeqPlayer.h"
#include "SeqActionFactory.h"
#include "SeqWorkerManager.h"
#include "SeqUISequencer.h"
#include "SeqString.h"
#include "SeqPath.h"
#include "imgui.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include <stdio.h>

SeqDragMode Sequentia::DragMode = SeqDragMode::None;
bool Sequentia::done = false;
SDL_Window* Sequentia::window = nullptr;
SeqProject* Sequentia::project = nullptr;
SeqExporter* Sequentia::exporter = nullptr;
double Sequentia::time = 0.0;
bool Sequentia::mousePressed[3] = { false, false, false };
float Sequentia::mouseWheel = 0.0f;
SeqSelection* Sequentia::dragClipSelection = nullptr;

// debug
bool Sequentia::showImGuiDemo = false;
bool Sequentia::showWorkers = true;

int Sequentia::Run(const char *openProject)
{
	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	// Setup exporter
	exporter = new SeqExporter();

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
	window = SDL_CreateWindow("Sequentia", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);

	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
	InitImGui();
	SeqRenderer::InitGL();

	// Setup initial project
	project = new SeqProject();
	if (openProject != nullptr)
	{
		project->SetPath(openProject);
		project->Open();
	}
	else
	{
		project->New();
	}

	// Start looping
	while (!done)
	{
		// handle sdl events
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGuiProcessEvent(&event);
		}

		BeginFrame();
		HandleShortcuts();
		HandleMainMenuBar();
		SeqWorkerManager::Instance()->Update();
		exporter->Update();
		project->Update();
		project->Draw();
		exporter->Export();
		HandleDragging();

		if (showImGuiDemo)
		{
			ImGui::ShowTestWindow(&showImGuiDemo);
		}

		if (showWorkers)
		{
			bool isOpen = true;
			if (ImGui::Begin("Workers", &isOpen))
			{
				SeqWorkerManager::Instance()->DrawDebugWindow();
			}
			ImGui::End();
			if (!isOpen)
				showWorkers = false;
		}

		// Rendering
		SeqRenderer::Render();
		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	SeqRenderer::Shutdown();
	ImGui::Shutdown();
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

void Sequentia::InitImGui()
{
	ImGuiIO& io = ImGui::GetIO();
	io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;                     // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
	io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
	io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
	io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
	io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
	io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
	io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
	io.KeyMap[ImGuiKey_A] = SDLK_a;
	io.KeyMap[ImGuiKey_C] = SDLK_c;
	io.KeyMap[ImGuiKey_V] = SDLK_v;
	io.KeyMap[ImGuiKey_X] = SDLK_x;
	io.KeyMap[ImGuiKey_Y] = SDLK_y;
	io.KeyMap[ImGuiKey_Z] = SDLK_z;
	
	io.SetClipboardTextFn = ImGuiSetClipboardText;
	io.GetClipboardTextFn = ImGuiGetClipboardText;
	io.ClipboardUserData = NULL;
	io.MouseDrawCursor = true;

#ifdef _WIN32
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	io.ImeWindowHandle = wmInfo.info.win.window;
#else
	(void)window;
#endif
}

void Sequentia::BeginFrame()
{
	SeqRenderer::RefreshDeviceObjects();

	ImGuiIO& io = ImGui::GetIO();

	// Setup display size (every frame to accommodate for window resizing)
	int w, h;
	int display_w, display_h;
	SDL_GetWindowSize(window, &w, &h);
	SDL_GL_GetDrawableSize(window, &display_w, &display_h);
	io.DisplaySize = ImVec2((float)w, (float)h);
	io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

	// Setup time step
	double currentTime = SDL_GetTicks() / 1000.0;
	io.DeltaTime = time > 0.0 ? (float)(currentTime - time) : (float)(1.0f / 60.0f);
	time = currentTime;

	// Setup inputs
	// (we already got mouse wheel, keyboard keys & characters from SDL_PollEvent())
	int mx, my;
	Uint32 mouseMask = SDL_GetMouseState(&mx, &my);
	if (SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_FOCUS)
		io.MousePos = ImVec2((float)mx, (float)my);   // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
	else
		io.MousePos = ImVec2(-1, -1);

	// If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
	io.MouseDown[0] = mousePressed[0] || (mouseMask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
	io.MouseDown[1] = mousePressed[1] || (mouseMask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	io.MouseDown[2] = mousePressed[2] || (mouseMask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
	mousePressed[0] = mousePressed[1] = mousePressed[2] = false;

	io.MouseWheel = mouseWheel;
	mouseWheel = 0.0f;

	// Hide OS mouse cursor if ImGui is drawing it
	SDL_ShowCursor(io.MouseDrawCursor ? 0 : 1);

	// Start the frame
	ImGui::NewFrame();
}

bool Sequentia::ImGuiProcessEvent(SDL_Event* event)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (event->type)
	{
		case SDL_MOUSEWHEEL:
		{
			if (event->wheel.y > 0)
				mouseWheel = 1;
			if (event->wheel.y < 0)
				mouseWheel = -1;
			return true;
		}
		case SDL_MOUSEBUTTONDOWN:
		{
			if (event->button.button == SDL_BUTTON_LEFT) mousePressed[0] = true;
			if (event->button.button == SDL_BUTTON_RIGHT) mousePressed[1] = true;
			if (event->button.button == SDL_BUTTON_MIDDLE) mousePressed[2] = true;
			return true;
		}
		case SDL_TEXTINPUT:
		{
			io.AddInputCharactersUTF8(event->text.text);
			return true;
		}
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			int key = event->key.keysym.sym & ~SDLK_SCANCODE_MASK;
			io.KeysDown[key] = (event->type == SDL_KEYDOWN);
			io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
			io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
			io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
			io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
			return true;
		}
		case SDL_QUIT:
		{
			done = true;
			return false;
		}
		case SDL_DROPFILE:
		{
			project->AddAction(SeqActionFactory::AddLibraryLink(event->drop.file));
			return false;
		}
	}
	return false;
}

const char* Sequentia::ImGuiGetClipboardText(void*)
{
	return SDL_GetClipboardText();
}

void Sequentia::ImGuiSetClipboardText(void*, const char* text)
{
	SDL_SetClipboardText(text);
}

void Sequentia::HandleShortcuts()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.KeyCtrl && !exporter->IsExporting())
	{
		if (ImGui::IsKeyPressed(SDLK_n, false)) { project->New(); }
		if (ImGui::IsKeyPressed(SDLK_o, false)) { project->OpenFrom(); }
		if (ImGui::IsKeyPressed(SDLK_s, false)) { project->Save(); }
		if (ImGui::IsKeyPressed(SDLK_z, false)) { project->Undo(); }
		if (ImGui::IsKeyPressed(SDLK_y, false)) { project->Redo(); }
	}
}

void Sequentia::HandleMainMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Project"))
		{
			if (ImGui::MenuItem("New", "Ctrl+N")) { project->New(); }
			if (ImGui::MenuItem("Open", "Ctrl+O")) { project->OpenFrom(); }
			ImGui::Separator();
			if (ImGui::MenuItem("Save", "Ctrl+S")) { project->Save(); }
			if (ImGui::MenuItem("Save As", "")) { project->SaveAs(); }
			ImGui::Separator();
			if (ImGui::MenuItem("Export", ""))
			{
				char* projectDir = SeqPath::GetDir(project->GetPath());
				SeqString::Temp->Clear();
				SeqString::Temp->Append(projectDir);
				SeqString::Temp->Append("export/");
				SeqPath::CreateDir(SeqString::Temp->Buffer);
				SeqString::Temp->Format("%s%d.mpg", SeqString::Temp->Buffer, SDL_GetTicks());
				exporter->AddTask(SeqString::Temp->Copy(), project->GetScene(1));
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit", "")) { done = true; }
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl+Z", false, project->CanUndo())) { project->Undo(); }
			if (ImGui::MenuItem("Redo", "Ctrl+Y", false, project->CanRedo())) { project->Redo(); }
			ImGui::Separator();
			if (ImGui::MenuItem("Cut", "CTRL+X", false, false)) {}
			if (ImGui::MenuItem("Copy", "CTRL+C", false, false)) {}
			if (ImGui::MenuItem("Paste", "CTRL+V", false, false)) {}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Window"))
		{
			if (ImGui::MenuItem("Video", "")) { project->AddWindowVideo(); }
			if (ImGui::MenuItem("Sequencer", "")) { project->AddWindowSequencer(); }
			if (ImGui::MenuItem("Library", "")) { project->AddWindowLibrary(); }
			ImGui::Separator();
			if (ImGui::MenuItem("Workers", "", showWorkers)) { showWorkers = !showWorkers; }
			if (ImGui::MenuItem("ImGui Demo", "", showImGuiDemo)) { showImGuiDemo = !showImGuiDemo; }
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

SeqProject* Sequentia::GetProject()
{
	return project;
}

SeqLibrary* Sequentia::GetLibrary()
{
	return project->GetLibrary();
}

SeqExporter* Sequentia::GetExporter()
{
	return exporter;
}

void Sequentia::HandleDragging()
{
	if (DragMode == SeqDragMode::Clips)
	{
		const ImVec2 size = ImVec2(150, 50);
		if (dragClipSelection->GetParent() == nullptr)
		{
			ImGui::BeginTooltip();
			SeqUISequencer::DrawClip(dragClipSelection->GetClip(), ImGui::GetCursorScreenPos(), size);
			ImGui::EndTooltip();
		}
		if (ImGui::IsMouseReleased(0))
		{
			SetDragClip(nullptr);
		}
	}
}

void Sequentia::SetDragClipNew(SeqLibraryLink* link, int streamIndex)
{
	SeqClip *clip = new SeqClip(link, streamIndex);
	Sequentia::SetDragClip(clip);
}

void Sequentia::SetDragClip(SeqClip *clip, const int64_t grip)
{
	if (dragClipSelection == nullptr || dragClipSelection->GetClip() != clip)
	{
		if (clip != nullptr)
		{
			DragMode = SeqDragMode::Clips;
			project->DeactivateAllClipSelections();
			dragClipSelection = project->NextClipSelection();
			dragClipSelection->location.leftTime = clip->location.leftTime;
			dragClipSelection->location.rightTime = clip->location.rightTime;
			dragClipSelection->location.startTime = clip->location.startTime;
			clip->isHidden = true;
			dragClipSelection->grip = grip;
			dragClipSelection->SetParent(clip->GetParent());
			dragClipSelection->Activate(clip);
		}
		else // set drag clip to nullptr
		{
			SeqClip *dragClip = dragClipSelection->GetClip();
			if (dragClipSelection->IsNewClip())
			{
				// if dragged from a library window
				delete dragClip;
			}
			else if (dragClipSelection->GetParent() == nullptr)
			{
				// make sure the remove action will not call SetDragClip(nullptr) again.
				dragClipSelection = nullptr;
				// if dragged from a sequencer window to the void
				project->AddAction(SeqActionFactory::RemoveClipFromChannel(dragClip));
			}
			else
			{
				// nothing happened, unhide original clip
				dragClip->isHidden = false;
			}
			project->DeactivateAllClipSelections();
			dragClipSelection = nullptr;
			DragMode = SeqDragMode::None;
		}
	}
}

void Sequentia::SetPreviewLibraryLink(SeqLibraryLink *link)
{
	SeqScene *previewScene = project->GetPreviewScene();
	if (previewScene->GetChannel(0)->ClipCount() == 0 ||
		previewScene->GetChannel(0)->GetClip(0)->GetLink() != link)
	{
		// reset the player
		previewScene->player->Stop();
		// clear the previewScene no matter what
		for (int i = 0; i < previewScene->ChannelCount(); i++)
		{
			SeqChannel *channel = previewScene->GetChannel(i);
			while (channel->ClipCount() > 0)
			{
				int index = channel->ClipCount() - 1;
				SeqClip *clip = channel->GetClip(index);
				project->DoAndForgetAction(SeqActionFactory::RemoveClipFromChannel(clip));
			}
		}
		// build up a previewScene
		if (link->metaDataLoaded && previewScene->player->IsActive())
		{
			SeqClip *videoClip = nullptr;
			SeqClip *audioClip = nullptr;
			// video clip
			if (link->defaultVideoStreamIndex != -1)
			{
				SetDragClipNew(link, link->defaultVideoStreamIndex);
				SeqChannel *channel = previewScene->GetChannel(0);
				dragClipSelection->SetParent(channel);
				dragClipSelection->location.leftTime = 0;
				project->DoAndForgetAction(SeqActionFactory::AddClipToChannel(dragClipSelection));
				videoClip = channel->GetClip(0);
			}
			// audio clip
			if (link->defaultAudioStreamIndex != -1)
			{
				SetDragClipNew(link, link->defaultAudioStreamIndex);
				SeqChannel *channel = previewScene->GetChannel(1);
				dragClipSelection->SetParent(channel);
				dragClipSelection->location.leftTime = 0;
				project->DoAndForgetAction(SeqActionFactory::AddClipToChannel(dragClipSelection));
				audioClip = channel->GetClip(0);
			}
		}
		// play
		previewScene->player->Play();
	}
}

bool Sequentia::IsDragging()
{
	return DragMode != SeqDragMode::None;
}

SeqSelection* Sequentia::GetDragClipSelection()
{
	return dragClipSelection;
}
