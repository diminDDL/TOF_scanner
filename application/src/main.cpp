#include <GL/glew.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <chrono>
#include <thread>
#include <cstdint>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <lib/serialib.h>
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

char selected_port[24] = "Select port\0";

char magic_symbol = 'A';

std::mutex serialDlMutex;
double deltaSerialTime = 0.0; // time between two screenshots
void SerialThread()
{
    serialib device;
    // auto lastStart = std::chrono::system_clock::now();
    while (true)
    {
        if (device.openDevice(selected_port, 115200) == 1)
        {
            // bool new_data = false;
            std::vector<uint8_t> data;
            // while(true){
            //     newDataMutex.lock();
            //     new_data = newData;
            //     newData = false;
            //     newDataMutex.unlock();
            //     if(new_data){
            //         scTbMutex.lock();
            //         data = data_buffer;
            //         scTbMutex.unlock();
            //         char read = 0;
            //         device.readChar(&read);
            //         if(read == magic_symbol){
            //             device.flushReceiver();
            //             device.writeBytes(data.data(), data.size());
            //             auto end = std::chrono::system_clock::now();
            //             std::chrono::duration<double> elapsed_seconds = end - lastStart;
            //             serialDlMutex.lock();
            //             deltaSerialTime = elapsed_seconds.count() * 1000;
            //             serialDlMutex.unlock();
            //             lastStart = std::chrono::system_clock::now();
            //         }
            //         device.flushReceiver();
            //     }
            // }
        }
        else
        {
            // std::cout << "Failed to open device" << std::endl;
            // sleep chrono
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

uint window_width = 800;
uint window_height = 800;
// GUI thread
GLuint textureID;
void guiThread()
{
    static bool first_start = true;

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return;

        // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char *glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char *glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow *window = glfwCreateWindow(window_width, window_height, "TOF Mapper", NULL, NULL);
    if (window == NULL)
        return;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    // set the smallest allowed window size
    glfwSetWindowSizeLimits(window, 400, 300, GLFW_DONT_CARE, GLFW_DONT_CARE);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    glewInit();
    // Our state
    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(1.0f, 1.0f, 0.0f, 1.00f);

    // TODO clean this up

#define X_MAX 1000
#define Y_MAX 1000

    // glm::mat4 ver_mat = glm::mat4(1.0f);
    glm::vec3 rot = glm::vec3(0.5f, 0.0f, 0.0f);
    glm::vec3 trans = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 points_color = glm::vec3(0.0f, 0.0f, 0.6f);
    float color[4] = {0.0, 0.3, 0.0, 0.0};

    float *points = new float[3 * X_MAX * Y_MAX];

    float freq = 1;
    float old_freq = !freq;

    // axis indicator
    float axis[3 * 6] = {
        0.0, 0.0, 0.0,
        0.5, 0.0, 0.0,
        0.0, 0.0, 0.0,
        0.0, 0.5, 0.0,
        0.0, 0.0, 0.0,
        0.0, 0.0, 0.5};

    //----------------------------------

    char const *vertex_shader =
        "#version 330 core\n"
        "layout (location = 0) in vec3 position;\n"
        "layout (location = 1) in vec2 aTexCoord;\n"
        "out vec2 TexCoord;\n"
        "out vec3 newColor;\n"
        "out float v_distance; // Output distance to fragment shader\n"
        "uniform mat4 transform;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = transform * vec4(position, 1.0f);\n"
        "    newColor = vec3(1.0f, 1.0f, 0.0f); // Or any other logic for color\n"
        "    TexCoord = vec2(position.x, position.y); // Or use aTexCoord if it's different\n"
        "    v_distance = length(position); // Calculate distance to origin\n"
        "}\n";

    char const *fragment_shader =
        "#version 330 core\n"
        "in vec3 newColor;\n"
        "in vec4 gl_FragCoord;\n"
        "in float v_distance; // Input distance from vertex shader\n"
        "out vec4 FragColor;\n"
        "uniform float max_distance;\n"
        "uniform vec3 f_color;\n"
        "float saturate(float x) {\n"
        "    return clamp(x, 0.0, 1.0);\n"
        "}\n"
        "vec3 TurboColormap(in float x) {\n"
        "    const vec4 kRedVec4 = vec4(0.13572138, 4.61539260, -42.66032258, 132.13108234);\n"
        "    const vec4 kGreenVec4 = vec4(0.09140261, 2.19418839, 4.84296658, -14.18503333);\n"
        "    const vec4 kBlueVec4 = vec4(0.10667330, 12.64194608, -60.58204836, 110.36276771);\n"
        "    const vec2 kRedVec2 = vec2(-152.94239396, 59.28637943);\n"
        "    const vec2 kGreenVec2 = vec2(4.27729857, 2.82956604);\n"
        "    const vec2 kBlueVec2 = vec2(-89.90310912, 27.34824973);\n"
        "    x = saturate(x);\n"
        "    vec4 v4 = vec4(1.0, x, x * x, x * x * x);\n"
        "    vec2 v2 = v4.zw * v4.z;\n"
        "    return vec3(\n"
        "        dot(v4, kRedVec4) + dot(v2, kRedVec2),\n"
        "        dot(v4, kGreenVec4) + dot(v2, kGreenVec2),\n"
        "        dot(v4, kBlueVec4) + dot(v2, kBlueVec2)\n"
        "    );\n"
        "}\n"
        "void main() {\n"
        "    float normalizedDistance = v_distance / max_distance;\n"
        "    vec3 turboColor = TurboColormap(normalizedDistance);\n"
        "    float brightnessAdjustment = mix(0.7, 1.0, normalizedDistance);\n"
        "    vec3 adjustedColor = turboColor * brightnessAdjustment;\n"
        "    FragColor = vec4(adjustedColor, 1.0);\n"
        "}\n";

    GLuint ver_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(ver_shader, 1, &vertex_shader, NULL);
    glCompileShader(ver_shader);

    int success;
    char infoLog[512];

    glGetShaderiv(ver_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(ver_shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    else
    {
        std::cout << "SHADER SUCCESS\n";
    }

    GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &fragment_shader, NULL);
    glCompileShader(frag_shader);

    int success1;
    char infoLog1[512];

    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success1);
    if (!success1)
    {
        glGetShaderInfoLog(frag_shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog1 << std::endl;
    }
    else
    {
        std::cout << "SHADER SUCCESS\n";
    }

    GLuint ID = glCreateProgram();
    glAttachShader(ID, ver_shader);
    glAttachShader(ID, frag_shader);
    glLinkProgram(ID);

    //----------------------------------

    glUseProgram(ID);

    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    unsigned int texture;
    glGenTextures(1, &texture);
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    GLuint position = glGetAttribLocation(ID, "position");
    glBindTexture(GL_TEXTURE_2D, texture);

    ImVec2 size;
    size.x = 512;
    size.y = 512;

    float cameraView[16] =
        {1.f, 0.f, 0.f, 0.f,
         0.f, 1.f, 0.f, 0.f,
         0.f, 0.f, 1.f, 0.f,
         0.f, 0.f, 0.f, 1.f};

    bool transpose = false;

    float maxDistance = 0.8f; // Example value, adjust as needed
    unsigned int maxDistanceLoc = glGetUniformLocation(ID, "max_distance");
    glUniform1f(maxDistanceLoc, maxDistance);

    ImVec4 back_color = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);

    ImVec4 point_color = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);

    bool vsync = true;

    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    // TODO END

    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = NULL;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // get glfw window size
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        window_width = width;
        window_height = height;
        if (!first_start)
        {
            ImGuiWindowFlags window_flags = 0;
            window_flags |= ImGuiWindowFlags_NoTitleBar;
            window_flags |= ImGuiWindowFlags_NoResize;
            window_flags |= ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoScrollbar;

            ImGui::Begin("TOF GUI", NULL, window_flags); // Create a window and append into it.
            ImGui::SetWindowSize(ImVec2(window_width, window_height));
            ImGui::SetWindowPos(ImVec2(0, 0));
            ImGui::Text("Welcome to TOF controller");          // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
            ImGui::ColorEdit3("Color", (float *)&clear_color); // Edit 3 floats representing a color
            // ImGui::Text("Application average (%.1f FPS)", io.Framerate);
            char fps_buf[64];
            sprintf(fps_buf, "Current FPS: %f", ImGui::GetIO().Framerate);
            ImGui::Text(fps_buf);

            char points_buf[64];
            sprintf(points_buf, "Current Points: %i", X_MAX * Y_MAX);
            ImGui::Text(points_buf);

            char mem_buf[64];
            float bandwith = (ImGui::GetIO().Framerate * 3 * X_MAX * Y_MAX * sizeof(float)) / (1000 * 1000);
            sprintf(mem_buf, "Current mem bandwith: %iMBps", (int)bandwith);
            ImGui::Text(mem_buf);

            // checkbox for vsync
            ImGui::Checkbox("Vsync", &vsync);
            glfwSwapInterval(vsync);

            // insert some padding
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);

            // Create a progress bar
            ImGui::Text("Scan Progress");
            static float progress = 0.0f, progress_dir = 1.0f;
            ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
            progress += progress_dir * 0.4f * ImGui::GetIO().DeltaTime;
            if (progress >= +1.1f)
            {
                progress = 0.0f;
            }

            ImGui::Text("Progress raw: %.1f", progress);

            // plot 3D data

            static bool isDragging = false;
            static double lastX, lastY;

            static bool isDraggingTranslation = false;
            static double lastTransX, lastTransY;

            // TODO clean this up

            ImGui::ColorEdit3("Background color", (float *)&back_color);

            // shove back_color into color
            color[0] = back_color.x;
            color[1] = back_color.y;
            color[2] = back_color.z;
            color[3] = back_color.w;

            ImGui::ColorEdit3("Points color", (float *)&point_color);

            // shove point_color into points_color
            points_color[0] = point_color.x;
            points_color[1] = point_color.y;
            points_color[2] = point_color.z;

            ImGui::SliderFloat3("Rotation", glm::value_ptr(rot), 0, 1);

            ImGui::SliderFloat3("Translation", glm::value_ptr(trans), 0, 1);

            static bool auto_increment = false;
            ImGui::Checkbox("Auto increment", &auto_increment);

            if (auto_increment)
                freq = 10 * progress;

            ImGui::SliderFloat("Freq", &freq, 0.01, 10);

            glBindVertexArray(VAO);
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x, size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

            glViewport(0, 0, size.x, size.y);
            glClearColor(color[0], color[1], color[2], color[3]);
            glClear(GL_COLOR_BUFFER_BIT);

            if (freq != old_freq)
            {

                for (int x = 0; x < X_MAX; x++)
                {
                    for (int y = 0; y < Y_MAX; y++)
                    {
                        int position = 3 * (x + (y * Y_MAX));
                        points[position + 0] = 0.5f * ((float)x / (X_MAX));
                        points[position + 1] = 0.5f * (float)y / (Y_MAX);
                        points[position + 2] = 0.5f * ((float)y / (Y_MAX)*sin(freq * glm::two_pi<float>() * (float)x / (X_MAX)));
                    }
                }
                old_freq = freq;
            }

            glm::mat4 temp = glm::make_mat4(cameraView);
            temp = glm::rotate(temp, glm::two_pi<float>() * rot.x, glm::vec3(1, 0, 0));
            temp = glm::rotate(temp, glm::two_pi<float>() * rot.y, glm::vec3(0, 1, 0));
            temp = glm::rotate(temp, glm::two_pi<float>() * rot.z, glm::vec3(0, 0, 1));

            temp = glm::translate(temp, trans);

            unsigned int transformLoc = glGetUniformLocation(ID, "transform");

            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(temp));
            unsigned int color_pos = glGetUniformLocation(ID, "f_color");
            glUniform3fv(color_pos, 1, glm::value_ptr(points_color));

            glVertexAttribPointer(position, 3, GL_FLOAT, transpose ? GL_TRUE : GL_FALSE, 12, (void *)0);
            glEnableVertexAttribArray(position);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);

            glBufferData(GL_ARRAY_BUFFER, 4 * 3 * 6, axis, GL_STATIC_DRAW);

            glUseProgram(ID);
            glBindVertexArray(VAO);

            glDrawArrays(GL_LINES, 0, 6);

            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * X_MAX * Y_MAX * 3, points, GL_DYNAMIC_DRAW);
            glDrawArrays(GL_POINTS, 0, X_MAX * Y_MAX);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            ImGui::Image((ImTextureID)texture, ImVec2(size.x, size.y));
            bool isHovered = ImGui::IsItemHovered();

            if (isHovered && ImGui::IsMouseDown(0))
            {
                if (!isDragging)
                {
                    isDragging = true;
                    glfwGetCursorPos(window, &lastX, &lastY);
                }
                else
                {
                    double mouseX, mouseY;
                    glfwGetCursorPos(window, &mouseX, &mouseY);

                    double offsetX = mouseX - lastX;
                    double offsetY = mouseY - lastY;

                    const float sensitivity = 0.001f;
                    rot.y += offsetX * sensitivity;
                    rot.x += offsetY * sensitivity;

                    lastX = mouseX;
                    lastY = mouseY;
                }
            }
            else
            {
                isDragging = false;
            }

            if (isHovered && ImGui::IsMouseDown(2)) // Right mouse button
            {
                if (!isDraggingTranslation)
                {
                    isDraggingTranslation = true;
                    glfwGetCursorPos(window, &lastTransX, &lastTransY);
                }
                else
                {
                    double mouseX, mouseY;
                    glfwGetCursorPos(window, &mouseX, &mouseY);

                    double offsetX = mouseX - lastTransX;
                    double offsetY = lastTransY - mouseY; // This inverts the y-axis translation

                    const float translationSensitivity = 0.01f; // Adjust this value to your liking
                    trans.x += offsetX * translationSensitivity;
                    trans.y += offsetY * translationSensitivity;

                    lastTransX = mouseX;
                    lastTransY = mouseY;
                }
            }
            else
            {
                isDraggingTranslation = false;
            }

            // TODO END

            ImGui::End();
        }
        else
        {
            first_start = false;
            ImGuiWindowFlags window_flags = 0;
            window_flags |= ImGuiWindowFlags_NoTitleBar;
            window_flags |= ImGuiWindowFlags_NoResize;
            window_flags |= ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoScrollbar;

            ImGui::Begin("TOF Mapper config", NULL, window_flags);

            // define the size of the subwindow
            ImGui::SetWindowSize(ImVec2(300, 230));
            if (window_width > 300 && window_height > 200)
                ImGui::SetWindowPos(ImVec2((window_width / 2) - 150, (window_height / 2) - 100));

            // Add a slider to define the resolution of the scan
            static int scan_resolution = 100;
            ImGui::SetCursorPosX((300 - 100) / 2);
            ImGui::Text("Scan resolution (points per degree)");
            ImGui::SetCursorPosX((300 - 100) / 4);
            ImGui::SliderInt("##Scan resolution", &scan_resolution, 1, 1000);
            // Add a sliders for the horizontal and vertical angle of the scan (in degrees)
            static int horizontal_angle = 90;
            ImGui::SetCursorPosX((300 - 100) / 2);
            ImGui::Text("Horizontal angle");
            ImGui::SetCursorPosX((300 - 100) / 4);
            ImGui::SliderInt("##Horizontal angle", &horizontal_angle, 1, 180);
            static int vertical_angle = 90;
            ImGui::SetCursorPosX((300 - 100) / 2);
            ImGui::Text("Vertical angle");
            ImGui::SetCursorPosX((300 - 100) / 4);
            ImGui::SliderInt("##Vertical angle", &vertical_angle, 1, 180);

            // insert some padding
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);

            static char *items[99] = {0};
            static int item_current = -1; // If the selection isn't within 0..count, Combo won't display a preview
            ImGui::PushItemWidth(150);
            // ImGui::Combo("##PortSelector", &item_current, items, IM_ARRAYSIZE(items));
            if (ImGui::BeginCombo("##PortSelector", selected_port))
            {
                for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                {
                    const bool is_selected = (item_current == n);
                    if (items[n] != NULL)
                    {
                        if (ImGui::Selectable(items[n], is_selected))
                        {
                            item_current = n;
                            strcpy(selected_port, items[n]);
                        }

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                }
                ImGui::EndCombo();
            }
            // std::cout << selected_port << std::endl;

            ImGui::SameLine();

            static bool scan = true;
            char device_name[99][24];
            if (scan)
            {
                scan = false;
                serialib device;
                for (int i = 0; i < 98; i++)
                {
// Prepare the port name (Windows)
#if defined(_WIN32) || defined(_WIN64)
                    sprintf(device_name[i], "\\\\.\\COM%d", i + 1);
#endif

// Prepare the port name (Linux)
#ifdef __linux__
                    sprintf(device_name[i], "/dev/ttyACM%d", i);
#endif

                    // try to connect to the device
                    if (device.openDevice(device_name[i], 115200) == 1)
                    {
                        // printf ("Device detected on %s\n", device_name[i]);
                        // set the pointer to the array
                        items[i] = device_name[i];
                        // Close the device before testing the next port
                        device.closeDevice();
                    }
                    else
                    {
                        items[i] = NULL;
                    }
                }
            }

            scan = ImGui::Button("Scan", ImVec2(100, 20));

            // center the button
            ImGui::SetCursorPosX((300 - 100) / 2);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
            first_start = !ImGui::Button("Apply", ImVec2(100, 20));

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        // wait 10ms blocking
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return;
}

int main(int, char **)
{
    // start the serial stream thread
    std::thread serialThread(SerialThread);

    // start the imgui thread
    std::thread imguiThread(guiThread);

    // wait for the threads to finish
    imguiThread.join();
}
