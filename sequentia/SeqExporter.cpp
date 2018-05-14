#include "SeqExporter.h"
#include "SeqWorkerManager.h"
#include "SeqRenderer.h"
#include "SeqEncoder.h"
#include "SeqScene.h"
#include "SeqPlayer.h"
#include "SeqTaskEncodeVideo.h"
#include "SeqMaterialInstance.h"
#include "SeqList.h"
#include "SeqImGui.h"

SeqExporter::SeqExporter()
{
	taskList = new SeqList<SeqExportTask>();
	doneTaskList = new SeqList<SeqExportTask>();
	for (int i = 0; i < 3; i++)
	{
		SeqRenderer::FillDefaultProjectionMatrix(&convertMatrices[i][0][0]);
		convertMaterials[i] = nullptr;
		exportToMaterials[i] = nullptr;
		exportToBuffer[i] = SeqDownloadTextureTarget();
	}
}

SeqExporter::~SeqExporter()
{
	delete taskList;
	delete doneTaskList;
	DisposeMaterials();
	delete[] convertMaterials;
	delete[] exportToMaterials;
	delete[] exportToBuffer;
	delete[] convertMatrices;
}

void SeqExporter::DisposeMaterials()
{
	for (int i = 0; i < 3; i++)
	{
		if (convertMaterials[i] != nullptr)
			SeqRenderer::RemoveMaterialInstance(convertMaterials[i]);
		if (exportToMaterials[i] != nullptr)
			SeqRenderer::RemoveMaterialInstance(exportToMaterials[i]);
	}
}

void SeqExporter::RefreshMaterials()
{
	DisposeMaterials();
	convertMaterials[0] = SeqRenderer::CreateMaterialInstance(&SeqRenderer::exportYMaterial, &convertMatrices[0][0][0]);
	exportToMaterials[0] = SeqRenderer::CreateMaterialInstance(&SeqRenderer::onlyTextureMaterial, &convertMatrices[0][0][0]);
	convertMaterials[1] = SeqRenderer::CreateMaterialInstance(&SeqRenderer::exportUMaterial, &convertMatrices[1][0][0]);
	exportToMaterials[1] = SeqRenderer::CreateMaterialInstance(&SeqRenderer::onlyTextureMaterial, &convertMatrices[1][0][0]);
	convertMaterials[2] = SeqRenderer::CreateMaterialInstance(&SeqRenderer::exportVMaterial, &convertMatrices[2][0][0]);
	exportToMaterials[2] = SeqRenderer::CreateMaterialInstance(&SeqRenderer::onlyTextureMaterial, &convertMatrices[2][0][0]);
}

bool SeqExporter::IsExporting()
{
	return taskList->Count() > 0;
}

float SeqExporter::GetProgress()
{
	if (taskList->Count() > 0)
	{
		SeqExportTask *task = taskList->GetPtr(0);
		return (task->currentFrame * task->frameTime) / (task->endTime - task->startTime);
	}
	else
	{
		return 0;
	}
}

float SeqExporter::GetExportWidth()
{
	if (taskList->Count() > 0)
		return taskList->GetPtr(0)->width;
	else
		return 0;
}

float SeqExporter::GetExportHeight()
{
	if (taskList->Count() > 0)
		return taskList->GetPtr(0)->height;
	else
		return 0;
}

void SeqExporter::AddTask(char* fullPath, SeqScene *scene)
{
	SeqExportTask task = SeqExportTask();
	task.fullPath = fullPath;
	task.scene = scene;
	task.startTime = 0;
	task.endTime = scene->GetLength();
	task.frameTime = SEQ_TIME_BASE / 24.0;
	task.width = 1920;
	task.height = 1080;

	task.currentFrame = 0;
	task.isInitialized = false;
	task.encodeTask = nullptr;
	task.material = nullptr;
	
	taskList->Add(task);
}

void SeqExporter::Update()
{
	if (taskList->Count() > 0)
	{
		SeqExportTask *task = taskList->GetPtr(0);
		SeqPlayer *player = task->scene->player;
		if (!task->isInitialized)
		{
			// reset state
			renderFrame = false;
			exportFrame = false;
			// dimensions
			int width = task->width;
			int height = task->height;
			int halfWidth = width / 2;
			int halfHeight = height / 2;
			// setup matrices
			// for now the hardcoded format is YUV 4:2:0
			SeqRenderer::SetProjectionMatrixDimensions(&convertMatrices[0][0][0], width, -height);
			SeqRenderer::SetProjectionMatrixDimensions(&convertMatrices[1][0][0], halfWidth, -halfHeight);
			SeqRenderer::SetProjectionMatrixDimensions(&convertMatrices[2][0][0], halfWidth, -halfHeight);
			// create texture object on the GPU
			exportToMaterials[0]->CreateTexture(0, width, height, GL_RED, 0);
			exportToMaterials[1]->CreateTexture(0, halfWidth, halfHeight, GL_RED, 0);
			exportToMaterials[2]->CreateTexture(0, halfWidth, halfHeight, GL_RED, 0);
			// create buffers to write the converted YUV data to
			char *buffer = new char[width * height];
			exportToBuffer[0] = { exportToMaterials[0], width, height, GL_RED, width * height, buffer };
			buffer = new char[halfWidth * halfHeight];
			exportToBuffer[1] = { exportToMaterials[1], halfWidth, halfHeight, GL_RED,  halfWidth * halfHeight, buffer };
			buffer = new char[halfWidth * halfHeight];
			exportToBuffer[2] = { exportToMaterials[2], halfWidth, halfHeight, GL_RED, halfWidth * halfHeight, buffer };
			// create material SeqPlayer renders to
			task->material = player->AddViewer(0);
			// we don't need the textures in the convertMaterials
			convertMaterials[0]->Dispose();
			convertMaterials[1]->Dispose();
			convertMaterials[2]->Dispose();
			// use the player's output texture to convert to YUV
			convertMaterials[0]->textureHandles[0] = task->material->textureHandles[0];
			convertMaterials[1]->textureHandles[0] = task->material->textureHandles[0];
			convertMaterials[2]->textureHandles[0] = task->material->textureHandles[0];
			// spin up an encoder task
			task->encodeTask = new SeqTaskEncodeVideo();
			SeqWorkerManager::Instance()->PerformTask(task->encodeTask);

			// TODO: specifify encoder format context
			
			// rewind the player and add the exporter as viewer
			player->Stop();
			//task->material = player->AddViewer(0);
			player->Seek(task->startTime);
			// the task is now initialized
			task->isInitialized = true;
		}

		if (!player->IsSeeking())
		{
			if (!renderFrame && !exportFrame)
			{
				renderFrame = true;
			}
		}
	}
}

void SeqExporter::Export()
{
	if (taskList->Count() > 0)
	{
		SeqExportTask *task = taskList->GetPtr(0);
		SeqPlayer *player = task->scene->player;

		// TODO: this may make rendering and encoding non-parralel. Make sure the encoder is at least double buffered
		if (exportFrame/* && task->encodeTask->GetEncoder()->GetStatus() == SeqEncoderStatus::Ready*/)
		{
			exportFrame = false;

			// TODO: send frame data to encoder

			// prepare next frame
			if (!player->IsSeeking())
			{
				task->currentFrame += 1;
				int64_t requestTime = task->startTime + task->currentFrame * task->frameTime;
				if (requestTime <= task->endTime)
				{
					// seek the next frame
					player->Seek(requestTime);
				}
				// finalize encoding
				else
				{
					// this was the last frame, we're done encoding
					// TODO: finalize encoding, we're done here
					// delete buffers
					delete[] exportToBuffer[0].destination;
					delete[] exportToBuffer[1].destination;
					delete[] exportToBuffer[2].destination;
					// we are also done with the player
					player->RemoveViewer(0);
					task->material = nullptr;
					// move the task to the done list
					doneTaskList->Add(*task);
					taskList->RemoveAt(0);
				}
			}
		}

		if (renderFrame)
		{
			ImDrawList* drawList = nullptr;
			ImVec2 size;

			// render Y
			size = ImVec2(task->width, task->height);
			SeqImGui::BeginRender(size);
			drawList = ImGui::GetWindowDrawList();
			drawList->AddCallback(SeqRenderer::BindFramebuffer, exportToMaterials[0]);
			drawList->AddImage(convertMaterials[0], ImVec2(0, 0), size);
			drawList->AddCallback(SeqRenderer::DownloadTexture, &exportToBuffer[0]);
			SeqImGui::EndRender();

			// render U
			size = ImVec2(task->width / 2, task->height / 2);
			SeqImGui::BeginRender(size);
			drawList = ImGui::GetWindowDrawList();
			drawList->AddCallback(SeqRenderer::BindFramebuffer, exportToMaterials[1]);
			drawList->AddImage(convertMaterials[1], ImVec2(0, 0), size);
			drawList->AddCallback(SeqRenderer::DownloadTexture, &exportToBuffer[1]);
			SeqImGui::EndRender();

			// render V
			SeqImGui::BeginRender(size);
			drawList = ImGui::GetWindowDrawList();
			drawList->AddCallback(SeqRenderer::BindFramebuffer, exportToMaterials[2]);
			drawList->AddImage(convertMaterials[2], ImVec2(0, 0), size);
			drawList->AddCallback(SeqRenderer::DownloadTexture, &exportToBuffer[2]);
			SeqImGui::EndRender();

			// reset framebuffer
			drawList->AddCallback(SeqRenderer::BindFramebuffer, nullptr);

			// prepare for encoding the downloaded data
			renderFrame = false;
			exportFrame = true;

			// TODO: research if we can find in the video packet data what the frame type is and if we better just play instead of seek ahead.
		}
	}
}
