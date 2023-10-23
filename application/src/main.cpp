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


static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

char selected_port[24] = "Select port\0";

char magic_symbol = 'A';

std::mutex serialDlMutex;
double deltaSerialTime = 0.0; // time between two screenshots
void SerialThread(){
    serialib device;
    //auto lastStart = std::chrono::system_clock::now();
    while(true){
        if(device.openDevice(selected_port, 115200) == 1){
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
        }else{
            // std::cout << "Failed to open device" << std::endl;
            // sleep chrono
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

uint window_width = 800;
uint window_height = 600;
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

            ImGui::Begin("TOF GUI", NULL, window_flags);                       // Create a window and append into it.
            ImGui::SetWindowSize(ImVec2(window_width, window_height));
            ImGui::SetWindowPos(ImVec2(0, 0));
            ImGui::Text("Welcome to TOF controller");             // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
            //ImGui::ColorEdit3("Color", (float *)&clear_color); // Edit 3 floats representing a color
            ImGui::Text("Application average (%.1f FPS)", io.Framerate);
            
            // insert some padding
            ImGui::SetCursorPosY(ImGui::GetCursorPosY()+20);

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
            ImGui::End();

            // se the background color to white
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        }else{
            first_start = false;
            ImGuiWindowFlags window_flags = 0;
            window_flags |= ImGuiWindowFlags_NoTitleBar;
            window_flags |= ImGuiWindowFlags_NoResize;
            window_flags |= ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoScrollbar;

            ImGui::Begin("TOF Mapper config", NULL, window_flags);

            // define the size of the subwindow
            ImGui::SetWindowSize(ImVec2(300, 230));
            if(window_width > 300 && window_height > 200)
                ImGui::SetWindowPos(ImVec2((window_width/2)-150, (window_height/2)-100));

            // Add a slider to define the resolution of the scan
            static int scan_resolution = 100;
            ImGui::SetCursorPosX((300-100)/2);
            ImGui::Text("Scan resolution (points per degree)");
            ImGui::SetCursorPosX((300-100)/4);
            ImGui::SliderInt("##Scan resolution", &scan_resolution, 1, 1000);
            // Add a sliders for the horizontal and vertical angle of the scan (in degrees)
            static int horizontal_angle = 90;
            ImGui::SetCursorPosX((300-100)/2);
            ImGui::Text("Horizontal angle");
            ImGui::SetCursorPosX((300-100)/4);
            ImGui::SliderInt("##Horizontal angle", &horizontal_angle, 1, 180);
            static int vertical_angle = 90;
            ImGui::SetCursorPosX((300-100)/2);
            ImGui::Text("Vertical angle");
            ImGui::SetCursorPosX((300-100)/4);
            ImGui::SliderInt("##Vertical angle", &vertical_angle, 1, 180);

            // insert some padding
            ImGui::SetCursorPosY(ImGui::GetCursorPosY()+20);

            static char* items[99] = {0};
            static int item_current = -1; // If the selection isn't within 0..count, Combo won't display a preview
            ImGui::PushItemWidth(150);
            // ImGui::Combo("##PortSelector", &item_current, items, IM_ARRAYSIZE(items));
            if (ImGui::BeginCombo("##PortSelector", selected_port))
            {
                for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                {
                    const bool is_selected = (item_current == n);
                    if(items[n] != NULL){
                        if (ImGui::Selectable(items[n], is_selected)){
                            item_current = n;
                            strcpy(selected_port, items[n]);
                        }
                    
                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected){
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                }
                ImGui::EndCombo();
            }
            //std::cout << selected_port << std::endl;

            ImGui::SameLine();

            static bool scan = true;
            char device_name[99][24];
            if(scan){
                scan = false;
                serialib device;
                for (int i=0;i<98;i++)
                {
                    // Prepare the port name (Windows)
                    #if defined (_WIN32) || defined( _WIN64)
                        sprintf (device_name[i],"\\\\.\\COM%d",i+1);
                    #endif

                    // Prepare the port name (Linux)
                    #ifdef __linux__
                        sprintf (device_name[i],"/dev/ttyACM%d",i);
                    #endif

                    // try to connect to the device
                    if (device.openDevice(device_name[i],115200)==1)
                    {
                        // printf ("Device detected on %s\n", device_name[i]);
                        // set the pointer to the array
                        items[i] = device_name[i];
                        // Close the device before testing the next port
                        device.closeDevice();
                    }else{
                        items[i] = NULL;
                    }
                }
            }

            scan = ImGui::Button("Scan", ImVec2(100, 20));

            // center the button
            ImGui::SetCursorPosX((300-100)/2);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY()+20);
            first_start = !ImGui::Button("Apply", ImVec2(100, 20));
            
            ImGui::End();

        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        //glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
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
