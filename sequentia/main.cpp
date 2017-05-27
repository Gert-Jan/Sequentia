// ImGui - standalone example application for Glfw + OpenGL 3, using programmable pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"
#include <stdio.h>
#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <GLFW/glfw3.h>

extern "C"
{
	#include "libavformat/avformat.h"
	#include "libavcodec\avcodec.h"
	#include "libavutil\avutil.h"
}

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")


static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error %d: %s\n", error, description);
}

GLFWwindow* initWindow()
{
	// Setup window
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
		exit(1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_MAXIMIZED, 1);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui OpenGL3 example", NULL, NULL);

	glfwMakeContextCurrent(window);
	gl3wInit();

	// Setup ImGui binding
	ImGui_ImplGlfwGL3_Init(window, true);

	// Load Fonts
	// (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
	//ImGuiIO& io = ImGui::GetIO();
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	
	return window;
}

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
	char *filename)
{
	FILE *f;
	int i;

	f = fopen(filename, "w");
	fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
	for (i = 0; i < ysize; i++)
		fwrite(buf + i * wrap, 1, xsize, f);
	fclose(f);
}

static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt,
	const char *filename)
{
	char buf[1024];
	int ret;

	ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding %i\n", ret);
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(dec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {
			fprintf(stderr, "Error during decoding\n");
			exit(1);
		}

		printf("saving frame %3d\n", dec_ctx->frame_number);
		fflush(stdout);

		/* the picture is allocated by the decoder. no need to
		free it */
		snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);
		pgm_save(frame->data[0], frame->linesize[0],
			frame->width, frame->height, buf);
	}
}


#define INBUF_SIZE 4096
void loadVideo()
{
	AVFormatContext* format = NULL;


	const char *filename, *outfilename;
	const AVCodec *codec;
	AVCodecParserContext *parser;
	AVCodecContext *context = NULL;
	FILE *file;
	AVFrame *frame;
	uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
	uint8_t *data;
	size_t   data_size;
	int ret;
	AVPacket *pkt;

	filename = "D:/Dev/C++/Sequentia-test-data/TBBT.S10E14.720p.HDTV.X264.mkv";
	outfilename = "D:/Dev/C++/Sequentia-test-data/out/test.file";

	avcodec_register_all();

	ret = avformat_open_input(&format, filename, NULL, NULL);
	if (ret != 0)
	{
		fprintf(stderr, "Could open file\n");
		exit(1);
	}



	pkt = av_packet_alloc();
	if (!pkt)
		exit(1);

	/* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) */
	memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

	/* find the MPEG-1 video decoder */
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!codec) {
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}

	parser = av_parser_init(codec->id);
	if (!parser) {
		fprintf(stderr, "parser not found\n");
		exit(1);
	}

	context = avcodec_alloc_context3(codec);
	if (!context) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}

	/* For some codecs, such as msmpeg4 and mpeg4, width and height
	MUST be initialized there because this information is not
	available in the bitstream. */

	/* open it */
	if (avcodec_open2(context, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Could not open %s\n", filename);
		exit(1);
	}

	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}

	while (!feof(file)) {
		/* read raw data from the input file */
		data_size = fread(inbuf, 1, INBUF_SIZE, file);
		if (!data_size)
			break;

		/* use the parser to split the data into frames */
		data = inbuf;
		while (data_size > 0) {
			ret = av_parser_parse2(parser, context, &pkt->data, &pkt->size,
				data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
			if (ret < 0) {
				fprintf(stderr, "Error while parsing\n");
				exit(1);
			}
			data += ret;
			data_size -= ret;

			if (pkt->size)
				decode(context, frame, pkt, outfilename);
		}
	}

	/* flush the decoder */
	decode(context, frame, NULL, outfilename);

	fclose(file);

	av_parser_close(parser);
	avcodec_free_context(&context);
	av_frame_free(&frame);
	av_packet_free(&pkt);
}

void loop(GLFWwindow* window)
{
	bool show_test_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImColor(114, 144, 154);

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		ImGui_ImplGlfwGL3_NewFrame();

		// 1. Show a simple window
		// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
		{
			static float f = 0.0f;
			ImGui::Text("Hello, world!");
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
			ImGui::ColorEdit3("clear color", (float*)&clear_color);
			if (ImGui::Button("Test Window")) show_test_window ^= 1;
			if (ImGui::Button("Another Window")) show_another_window ^= 1;
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		}

		// 2. Show another simple window, this time using an explicit Begin/End pair
		if (show_another_window)
		{
			ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
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
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplGlfwGL3_Shutdown();
	glfwTerminate();
}

int main(int, char**)
{
	GLFWwindow* window = initWindow();
	loadVideo();
	loop(window);
	return 0;
}
