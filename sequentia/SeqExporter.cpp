#include <SDL.h>

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
	// TODO: replace this task.endTime test code
	task.endTime = SEQ_TIME(2);
	//task.endTime = scene->GetLength();
	task.frameTime = SEQ_TIME_BASE / 24.0;
	task.width = 1920;
	task.height = 1080;
	task.codecId = AV_CODEC_ID_MPEG1VIDEO;
	task.bitRate = 400000;

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

			// TODO: Move context creating to SeqEncoder
			int result = CreateContext(task);

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

			// TODO: Move Encoding to SeqEncoder
			Encode(task);

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
					// TODO: Move finalizing encodeing to SeqEncoder
					FinalizeEncoding(task);

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

int SeqExporter::CreateContext(SeqExportTask *task)
{
	// specifify encoder format context
	AVCodec *codec;

	// find encoder
	codec = avcodec_find_encoder(task->codecId);
	if (!codec) {
		fprintf(stderr, "Codec not found\n");
		return -1;
	}

	AVCodecContext *context;
	context = avcodec_alloc_context3(codec);
	if (!context) {
		fprintf(stderr, "Could not allocate video codec context\n");
		return -1;
	}

	task->packet = av_packet_alloc();
	if (!task->packet)
	{
		fprintf(stderr, "Could not allocate packet\n");
		return -1;
	}

	// put sample parameters
	context->bit_rate = task->bitRate;
	// resolution must be a multiple of two
	context->width = task->width;
	context->height = task->height;
	// frames per second
	context->time_base = av_d2q(SEQ_TIME_IN_SECONDS(task->frameTime), 10000);
	context->framerate = av_inv_q(context->time_base);
	/* emit one intra frame every ten frames
	* check frame pict_type before passing frame
	* to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
	* then gop_size is ignored and the output of encoder
	* will always be I frame irrespective to gop_size
	*/
	context->gop_size = 10;
	context->max_b_frames = 1;
	context->pix_fmt = AV_PIX_FMT_YUV420P;
	task->context = context;

	if (codec->id == AV_CODEC_ID_H264)
		av_opt_set(context->priv_data, "preset", "slow", 0);

	// open the context
	if (avcodec_open2(context, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		return -1;
	}

	task->file = SDL_RWFromFile(task->fullPath, "wb");
	if (!task->file) {
		fprintf(stderr, "Could not open %s\n", task->fullPath);
		return -1;
	}

	AVFrame *frame;
	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		return -1;
	}
	frame->format = context->pix_fmt;
	frame->width = context->width;
	frame->height = context->height;
	task->frame = frame;

	int ret = av_frame_get_buffer(frame, 32);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate the video frame data\n");
		return ret;
	}

	task->frameCount = 0;
	
	return 0;
}

int SeqExporter::Encode(SeqExportTask *task)
{
	// make sure the frame data is writable 
	int ret = av_frame_make_writable(task->frame);
	if (ret < 0)
	{
		fprintf(stderr, "Could not make frame writable.\n");
		return ret;
	}
	// set data on frame
	task->frame->data[0] = (uint8_t*)exportToBuffer[0].destination;
	task->frame->data[1] = (uint8_t*)exportToBuffer[1].destination;
	task->frame->data[2] = (uint8_t*)exportToBuffer[2].destination;
	task->frame->pts = task->frameCount++;

	// encode and write to file
	return Encode(task->context, task->frame, task->packet, task->file);
}

int SeqExporter::Encode(AVCodecContext *context, AVFrame *frame, AVPacket *packet, SDL_RWops *file)
{
	// send the frame to the encoder
	int ret = avcodec_send_frame(context, frame);
	if (ret < 0)
	{
		fprintf(stderr, "Error sending a frame for encoding\n");
		return ret;
	}

	while (ret >= 0)
	{
		ret = avcodec_receive_packet(context, packet);
		if (ret == AVERROR(EAGAIN))
		{
			// needs more data to write the packet.
			return ret;
		}
		else if (ret == AVERROR_EOF)
		{
			fprintf(stderr, "Error during encoding: EOF\n");
			return ret;
		}
		else if (ret < 0)
		{
			fprintf(stderr, "Error during encoding\n");
			return ret;
		}

		printf("Write frame %d (size=%5d)\n", packet->pts, packet->size);

		file->write(file, packet->data, 1, packet->size);

		av_packet_unref(packet);
	}

	return 0;
}

void SeqExporter::FinalizeEncoding(SeqExportTask *task)
{
	// flush the encoder 
	Encode(task->context, NULL, task->packet, task->file);

	// add sequence end code to have a real MPEG file
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };
	task->file->write(task->file, endcode, 1, sizeof(endcode));
	SDL_RWclose(task->file);

	avcodec_free_context(&task->context);
	av_frame_free(&task->frame);
	av_packet_free(&task->packet);
}
