#include <sstream>
#include <cstring>
#include <iomanip>

#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include "modules/Scene.hpp"
#include "GameState.cpp"
#include "Timer.cpp"
// this should work, i am doing it to try and  do a pull request

#define Width 1920
#define Height 1080
#define MAX_DAMAGE 5


struct UniformBufferObject {
    alignas(4) float amb;
    alignas(4) float gamma;
    alignas(16) glm::vec3 sColor;
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::mat4 nMat;
    alignas(16) glm::mat4 mMat;
};

struct GlobalUniformBufferObject {
    alignas(16) glm::vec3 lightDir;
    alignas(16) glm::vec4 lightColor;
    alignas(16) glm::vec3 ambLightColor;
    alignas(16) glm::vec3 eyePos;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 UV;
};

struct QuadVertex {
    glm::vec2 pos;
    glm::vec2 texCoord;
};

struct ImageUniformBufferObject {
    alignas(16) glm::mat4 mvp;
};

class VehicleSimulator : public BaseProject {
protected:
    float aspectRatio;
    const glm::mat4 lightView = glm::rotate(glm::mat4(1), glm::radians(-30.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                                glm::rotate(glm::mat4(1), glm::radians(-45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::vec3 lightDir = glm::vec3(lightView * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    glm::vec3 cameraPos;
    glm::vec3 camTarget;
    const float FOVy = glm::radians(90.0f);
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;
    glm::mat4 Prj = glm::mat4(1.0f);
    glm::mat4 View = glm::mat4(1.0f);
    glm::mat4 initialCarMatrix = glm::mat4(1.0f); // Store initial car transformation
    glm::vec3 carPos = glm::vec3(0.0f);
    glm::vec3 offset = glm::vec3(0.0f, 3.0f, 2.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    float carYaw = 0.0f;
    float cameraYaw = 0.0f;
    float move_speed = 2.0f;
    float back_speed = 2.0f;
    float current_velocity = 0.0f; // Current forward velocity for realistic physics
    const float max_speed = 10.0f; // Maximum forward speed
    const float acceleration = 3.0f; // Acceleration when pressing W
    const float deceleration = 2.0f; // Natural deceleration (friction)
    const float brake_deceleration = 4.0f; // Deceleration when pressing S
    const float min_steering_speed = 0.5f; // Minimum speed required for steering
    const float max_steering_speed = 8.0f; // Speed at which steering becomes less responsive
    bool camRot = false;
    bool crash_detected = false;
    int num_crashes = 0;
    char instruction[120] = "hi";
    GameState state = GameState::SplashScreen;
    GameState previousGameState = GameState::GoToPark; // Store state before pause
    bool GameOver = false;
    bool escKeyWasPressed = false; // Track ESC key state
    bool pauseTextDisplayed = false; // Track if pause text is already displayed
    bool justResumedFromPause = false; // Track if we just resumed from pause
    Timer timer;

    // Fullscreen image rendering components
    DescriptorSetLayout DSLimage;
    VertexDescriptor Vquad;
    Pipeline Pimage;
    Model Mquad;
    Texture TSplash, TControls;
    DescriptorSet DSSplash, DSControls;

    //Here we will list all the object needed for our project

    DescriptorSetLayout DSLgubo, DSLmesh;

    VertexDescriptor Vmesh;
    RenderPass RP;
    Pipeline Pmesh;
    TextMaker txt;

    Scene SC;
    std::vector<VertexDescriptorRef> VDRs;
    std::vector<TechniqueRef> PRs;

    void setWindowParameters()
    {
        windowWidth = Width;
        windowHeight = Height;
        windowTitle = "VehicleSimulator";
        
        // Set window to non-resizable initially (for splash screen)
        // This will be updated when the state changes to game state
        windowResizable = GLFW_FALSE;
        
        initialBackgroundColor = {0.1f, 0.1f, 0.2f, 1.0f};
        aspectRatio = 4 / 3;
    }    void onWindowResize(int w, int h) {
        // Process window resize for all game states except splash/controls screens
        if (state != GameState::SplashScreen && state != GameState::ControlsScreen) {
            aspectRatio = (float) w / (float) h;
            RP.width = w;
            RP.height = h;
            txt.resizeScreen(w, h);
            
            // If we're paused, reset the text flag so pause text repositions correctly
            if (state == GameState::Paused) {
                pauseTextDisplayed = false;
            }
        }
    }    //Here we initialize the descriptor set layouts and our models
    void localInit() {
        // Ensure window resizability is set correctly based on initial state
        updateWindowResizability();
        DSLgubo.init(this, {
                         // this array contains the binding:
                         // first  element : the binding number
                         // second element : the type of element (buffer or texture)
                         // third  element : the pipeline stage where it will be used
                         {
                             0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS,
                             sizeof(GlobalUniformBufferObject), 1
                         }
                     });
        DSLmesh.init(this, {
                         // this array contains the binding:
                         // first  element : the binding number
                         // second element : the type of element (buffer or texture)
                         // third  element : the pipeline stage where it will be used
                         {
                             0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS,
                             sizeof(UniformBufferObject), 1
                         },
                         {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
                     });

        // Initialize descriptor set layout for fullscreen images
        DSLimage.init(this, {
                         {
                             0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT,
                             sizeof(ImageUniformBufferObject), 1
                         },
                         {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
                     });

        Vmesh.init(this, {
                       {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
                   }, {
                       {
                           0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
                           sizeof(glm::vec3), POSITION
                       },
                       {
                           0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm),
                           sizeof(glm::vec3), NORMAL
                       },
                       {
                           0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
                           sizeof(glm::vec2), UV
                       }
                   });

        // Initialize vertex descriptor for fullscreen quad
        Vquad.init(this, {
                       {0, sizeof(QuadVertex), VK_VERTEX_INPUT_RATE_VERTEX}
                   }, {
                       {
                           0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(QuadVertex, pos),
                           sizeof(glm::vec2), POSITION
                       },
                       {
                           0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(QuadVertex, texCoord),
                           sizeof(glm::vec2), UV
                       }
                   });

        VDRs.resize(1);
        VDRs[0].init("VMesh", &Vmesh);        RP.init(this);
        RP.properties[0].clearValue = {0.6f, 0.8f, 1.0f, 1.0f};

        //Pipelines
        Pmesh.init(this, &Vmesh, "shaders/Mesh.vert.spv", "shaders/Mesh.frag.spv", {&DSLgubo, &DSLmesh});
        Pimage.init(this, &Vquad, "shaders/Image.vert.spv", "shaders/Image.frag.spv", {&DSLimage});

        Pmesh.setCullMode(VK_CULL_MODE_NONE);
        Pimage.setCullMode(VK_CULL_MODE_NONE);

        // Load splash screen and controls textures
        TSplash.init(this, "assets/textures/SplashScreen.png");
        TControls.init(this, "assets/textures/Controls.png");        // Create fullscreen quad model        // Create vertices for a fullscreen quad
        std::vector<QuadVertex> quadVertices = {
            {{-1.0f, -1.0f}, {0.0f, 0.0f}},  // Bottom-left
            {{ 1.0f, -1.0f}, {1.0f, 0.0f}},  // Bottom-right
            {{ 1.0f,  1.0f}, {1.0f, 1.0f}},  // Top-right
            {{-1.0f,  1.0f}, {0.0f, 1.0f}}   // Top-left
        };
        std::vector<uint32_t> quadIndices = {0, 1, 2, 2, 3, 0};

        // Manually set the vertices and indices for the quad
        Mquad.vertices.resize(quadVertices.size() * sizeof(QuadVertex));
        memcpy(Mquad.vertices.data(), quadVertices.data(), quadVertices.size() * sizeof(QuadVertex));
        Mquad.indices = quadIndices;
        Mquad.initMesh(this, &Vquad);


        PRs.resize(1);
        PRs[0].init("Simple", {
                        {
                            &Pmesh, {
                                //Pipeline and DSL for the first pass
                                /*DSLglobal*/{},
                                /*DSLlocalChar*/{
                                    /*t0*/{true, 0, {}} // index 0 of the "texture" field in the json file
                                }
                            }
                        }                    }, /*TotalNtextures*/1, &Vmesh);
        DPSZs.uniformBlocksInPool = 5;
        DPSZs.texturesInPool = 6;
        DPSZs.setsInPool = 5;

        std::cout << "\nLoading the scene\n\n";
        if (SC.init(this, /*Npasses*/1, VDRs, PRs, "assets/models/scene.json") != 0) {
            std::cout << "ERROR LOADING THE SCENE\n";
            exit(0);
        }
        txt.init(this, windowWidth, windowHeight);


        submitCommandBuffer("main", 0, populateCommandBufferAccess, this);


    }    // Here you create your pipelines and Descriptor Sets!
    void pipelinesAndDescriptorSetsInit()
    {
        RP.create();
        Pmesh.create(&RP);
        Pimage.create(&RP);        // Create descriptor sets for images
        DSSplash.init(this, &DSLimage, {TSplash.getViewAndSampler()});
        DSControls.init(this, &DSLimage, {TControls.getViewAndSampler()});

        SC.pipelinesAndDescriptorSetsInit();
        txt.pipelinesAndDescriptorSetsInit();
    }    //Here the cleanup of the pipelines and descriptor sets
    void pipelinesAndDescriptorSetsCleanup()
    {
        Pmesh.cleanup();
        Pimage.cleanup();
        DSSplash.cleanup();
        DSControls.cleanup();
        RP.cleanup();
        SC.pipelinesAndDescriptorSetsCleanup();
        txt.pipelinesAndDescriptorSetsCleanup();
    }    // Here you destroy all the Models, Texture and Desc. Set Layouts you created!
    // All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
    // You also have to destroy the pipelines: since they need to be rebuilt, they have two different
    // methods: .cleanup() recreates them, while .destroy() delete them completely
    void localCleanup() {
        DSLgubo.cleanup();
        DSLmesh.cleanup();
        DSLimage.cleanup();
        Pmesh.destroy();
        Pimage.destroy();
        Mquad.cleanup();
        TSplash.cleanup();
        TControls.cleanup();
        RP.destroy();
        SC.localCleanup();
        txt.localCleanup();
    }
    static void populateCommandBufferAccess(VkCommandBuffer commandBuffer, int currentImage, void *Params) {
        VehicleSimulator *T = (VehicleSimulator *) Params;
        T->populateCommandBuffer(commandBuffer, currentImage);
    }
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int CurrentImage) {
        RP.begin(commandBuffer, CurrentImage);
        if (state == GameState::SplashScreen || state == GameState::ControlsScreen)
            {            // Render fullscreen image
            ImageUniformBufferObject iubo{};
            iubo.mvp = glm::mat4(1.0f); // Identity matrix for fullscreen quad (already corrected in vertex data)

            if (state == GameState::SplashScreen) {
                DSSplash.bind(commandBuffer, Pimage, 0, CurrentImage);
                DSSplash.map(CurrentImage, &iubo, 0);
            } else {
                DSControls.bind(commandBuffer, Pimage, 0, CurrentImage);
                DSControls.map(CurrentImage, &iubo, 0);
            }

            Pimage.bind(commandBuffer);
            Mquad.bind(commandBuffer);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Mquad.indices.size()), 1, 0, 0, 0);
        } else {
            // Render normal game scene
            SC.populateCommandBuffer(commandBuffer, 0, CurrentImage);
        }

        RP.end(commandBuffer);
    }    // Method to update window resizability based on current game state
    void updateWindowResizability() {
        // Make window resizable only in game states, not in splash or controls screens
        // Pause state should keep window resizable since it's part of gameplay
        bool shouldBeResizable = (state != GameState::SplashScreen && state != GameState::ControlsScreen);
        

        if ((shouldBeResizable && windowResizable == GLFW_FALSE) || 
            (!shouldBeResizable && windowResizable == GLFW_TRUE))
            {
            
            // Update the window resizable property
            windowResizable = shouldBeResizable ? GLFW_TRUE : GLFW_FALSE;
            
            // Apply the change to the GLFW window
            glfwSetWindowAttrib(window, GLFW_RESIZABLE, windowResizable);
        }
    }    //APP logic
    void updateUniformBuffer(uint32_t currentImage)
    {
        timer.update();
        gameLogic(window);

        // Check if state changed and update window resizability
        static GameState lastState = state;
        if (lastState != state)
        {
            // Update window resizability when state changes
            updateWindowResizability();
            lastState = state;
        }

        // Camera automatically follows car rotation to maintain relative view
        // Use total camera yaw = manual camera yaw + car yaw for automatic following
        float totalCameraYaw = cameraYaw + carYaw;
        
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), totalCameraYaw, glm::vec3(0, 0, 1));
        glm::vec4 rotatedOffset = rotation * glm::vec4(offset, 1.0f);
        cameraPos = glm::vec3(rotatedOffset) + carPos;

        glm::vec4 rotatedUp = rotation * glm::vec4(0, 1, 0, 0);
        up = glm::normalize(glm::vec3(rotatedUp));

        Prj = glm::perspective(FOVy, aspectRatio, nearPlane, farPlane);
        // Apply a Y-flip to the view matrix to correct orientation
        View = glm::lookAt(cameraPos, carPos, up);


        GlobalUniformBufferObject gubo{};
        gubo.lightDir = lightDir;
        gubo.lightColor = glm::vec4(0.8f, 0.7f, 0.70f, 1.0f);
        gubo.ambLightColor = glm::vec3(0.6f);
        gubo.eyePos = cameraPos;

        UniformBufferObject ubo{};


        int instanceId;
        // Create the car's transformation matrix: translation + initial orientation + steering rotation
        SC.TI[0].I[0].Wm = glm::translate(glm::mat4(1.0f), carPos) * 
                           glm::rotate(glm::mat4(1.0f), carYaw, glm::vec3(0.0f, 0.0f, 1.0f)) *
                           glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        for (instanceId = 0; instanceId < SC.TI[0].InstanceCount; instanceId++)
        {
            ubo.mMat = SC.TI[0].I[instanceId].Wm;
            ubo.mvpMat = Prj * View * ubo.mMat;
            ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
            ubo.amb = 0.3f;
            ubo.gamma = 180.0f;
            ubo.sColor = glm::vec3(0.8f);
            SC.TI[0].I[instanceId].DS[0][0]->map(currentImage, &gubo, 0); // Set 0
            SC.TI[0].I[instanceId].DS[0][1]->map(currentImage, &ubo, 0); // Set 1
        }
        if(state == GameState::SplashScreen || state == GameState::ControlsScreen)
            {
            // No need to update the scene's uniform buffers in splash or controls screens
        } else if(state == GameState::Paused) {
            // Only clear and set pause text once when entering pause state
            if (!pauseTextDisplayed) {
                txt.removeAllText();
                
                // Display centered pause message with consistent styling
                std::string pauseMessage = "GAME PAUSED";
                std::string continueMessage = "Press P to continue or ESC to exit";
                
                // Use larger, bold text for main pause message with same styling as other UI text
                txt.print(0.0f, 0.2f, pauseMessage, 1, "CO", false, true, false, TAL_CENTER, TRH_CENTER, TRV_MIDDLE, 
                         {1.0f, 0.0f, 0.0f, 1.0f}, {0.8f, 0.8f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, 1.5f, 1.5f);
                
                // Use normal size text for instructions with same styling as game UI
                txt.print(0.0f, -0.1f, continueMessage, 2, "CO", false, false, false, TAL_CENTER, TRH_CENTER, TRV_MIDDLE, 
                         {1.0f, 0.0f, 0.0f, 1.0f}, {0.8f, 0.8f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, 1.0f, 1.0f);
                
                pauseTextDisplayed = true;
            }
        } else {
            // Reset pause text flag when not in pause state
            pauseTextDisplayed = false;
            std:: ostringstream speed;
            std:: ostringstream crashes;
            std:: ostringstream instr;
            std:: ostringstream time;
            
            // Enhanced speed display with direction and steering info
            speed << "Speed: " << std::fixed << std::setprecision(1) << abs(current_velocity);
            if (current_velocity > 0.1f) {
                speed << " (Forward)";
            } else if (current_velocity < -0.1f) {
                speed << " (Reverse)";
            } else {
                speed << " (Stopped)";
            }
            
            // Add steering availability indicator
            if (abs(current_velocity) < min_steering_speed) {
                speed << " [No Steering]";
            } else if (abs(current_velocity) > max_steering_speed) {
                speed << " [Reduced Steering]";
            }
            speed << "\n";
            
            crashes<<"Damage:: " << num_crashes<<"\n";
            instr <<instruction;
            time <<timer.getElapsedTime();
            txt.print(1.0f, 1.0f, speed.str(), 1, "CO", false, true, false,TAL_RIGHT,TRH_RIGHT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});
            txt.print(-1.0f, 1.0f, crashes.str(), 2, "CO", false, true, false,TAL_LEFT,TRH_LEFT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});
            txt.print(-1.0f, -0.9f, instr.str(), 3, "CO", false, true, false,TAL_LEFT,TRH_LEFT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});
            txt.print(0.0f, -0.9f, time.str(), 4, "CO", false, true, false,TAL_LEFT,TRH_LEFT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});

        }

        txt.updateCommandBuffer();
    }    void gameLogic(GLFWwindow *window)
    {
        // Improved key handling with proper debounce timing
        static bool pKeyWasPressed = false;
        static double lastKeyPressTime = 0.0;
        const double keyDebounceTime = 0.5; // Increased debounce time for more reliable detection

        // Get current timestamp for debouncing
        double currentTime = glfwGetTime();
          // Check if P key is currently pressed
        int pKeyState = glfwGetKey(window, GLFW_KEY_P);
        bool pKeyIsPressed = (pKeyState == GLFW_PRESS);
        
        // Check if ESC key is currently pressed
        int escKeyState = glfwGetKey(window, GLFW_KEY_ESCAPE);
        bool escKeyIsPressed = (escKeyState == GLFW_PRESS);        // Detect key press event (transition from not pressed to pressed)
        // Only process if enough time has passed since last press
        if (pKeyIsPressed && !pKeyWasPressed && (currentTime - lastKeyPressTime) > keyDebounceTime)
            {
            // Record the time of this press
            lastKeyPressTime = currentTime;            // Handle state transitions based on current state
            if (state == GameState::SplashScreen) {
                state = GameState::ControlsScreen;
                // Update window resizability for the new state
                updateWindowResizability();
                // Force a command buffer update to refresh the screen
                recreateSwapChain();
            } else if (state == GameState::ControlsScreen) {

                state = GameState::GoToPark;
                timer.start();
                // Update window resizability for the new state
                updateWindowResizability();
                // Force a command buffer update to refresh the screen
                recreateSwapChain();
            }
            else if (state == GameState::Paused) {
                // Resume the game from pause
                state = previousGameState;
                pauseTextDisplayed = false; // Reset pause text flag
                justResumedFromPause = true; // Flag to reset delta time
                timer.resume(); // Resume the timer without resetting elapsed time
                updateWindowResizability();
                recreateSwapChain();
            }
            else if (state == GameState::GameOver || state == GameState::GameWon) {
                state = GameState::GoToPark;
                GameOver = false;
                pauseTextDisplayed = false; // Reset pause text flag
                justResumedFromPause = false; // Reset pause resume flag
                carPos = glm::vec3(0.0f);
                carYaw = 0.0f;
                cameraYaw = 0.0f;
                num_crashes = 0;
                move_speed = 2.0f;
                current_velocity = 0.0f; // Reset velocity for physics
                
                // Reset coin position to the first target location
                SC.TI[0].I[1].Wm[3] = glm::vec4(glm::vec3(41.0871f,-25.9646f,0.0f), 1.0f);
                strcpy(instruction, "Go to the small park");

                timer.start();

                updateWindowResizability();
                // Force a command buffer update to refresh the screen
                recreateSwapChain();
            }
        }        // Update the key state for the next frame
        pKeyWasPressed = pKeyIsPressed;
        
        // Handle ESC key press for pause functionality
        if (escKeyIsPressed && !escKeyWasPressed && (currentTime - lastKeyPressTime) > keyDebounceTime) {
            lastKeyPressTime = currentTime;
            
            if (state == GameState::Paused) {
                // If already paused, ESC exits the game
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            } else if (state != GameState::SplashScreen && state != GameState::ControlsScreen && 
                      state != GameState::GameOver && state != GameState::GameWon) {
                // If in game state, ESC pauses the game
                previousGameState = state;
                state = GameState::Paused;
                pauseTextDisplayed = false; // Reset flag to trigger text update
                timer.stop(); // Pause the timer
            }
        }
        
        // Update ESC key state for next frame
        escKeyWasPressed = escKeyIsPressed;

        // If we're in splash, controls screen, or paused, we just need to make sure the static
        // screen content is displayed - no movement logic needed
        if(state == GameState::SplashScreen || state == GameState::ControlsScreen || state == GameState::Paused) {
            static bool firstTimeDrawing = true;
            if (firstTimeDrawing)
                {
                // On first run, make sure the descriptor sets are properly initialized
                ImageUniformBufferObject iubo{};
                iubo.mvp = glm::mat4(1.0f);
                if (state == GameState::SplashScreen) {
                    DSSplash.map(0, &iubo, 0);
                } else {
                    DSControls.map(0, &iubo, 0);
                }
                firstTimeDrawing = false;
            }
            return;
        }

        // Physics timing - handle pause resume properly
        static auto startTime = std::chrono::high_resolution_clock::now();
        static auto lastFrameTime = startTime;

        // Check if we just resumed from pause (avoid large delta time)
        auto frameTime = std::chrono::high_resolution_clock::now();
        float deltaT;
        
        if (justResumedFromPause) {
            // Reset timing when resuming from pause to avoid large delta
            deltaT = 0.0f; // No movement on the resume frame
            lastFrameTime = frameTime;
            justResumedFromPause = false; // Reset the flag
        } else {
            deltaT = std::chrono::duration<float>(frameTime - lastFrameTime).count();
            lastFrameTime = frameTime;
            
            // Clamp deltaT to prevent physics issues from large frame times
            if (deltaT > 0.1f) { // Max 100ms delta to prevent huge jumps
                deltaT = 0.1f;
            }
        }
        const float rotate_speed = 1.0f;
        
        if (!GameOver) {
            // Handle forward acceleration (W key)
            bool accelerating = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
            bool braking = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
            bool space_braking = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
            
            // Update velocity based on input
            if (accelerating && !braking) {
                // Accelerate forward
                current_velocity += acceleration * deltaT;
                if (current_velocity > max_speed) {
                    current_velocity = max_speed;
                }
            } else if (braking) {
                // Brake (reverse or slow down)
                if (current_velocity > 0) {
                    // Slow down if moving forward
                    current_velocity -= brake_deceleration * deltaT;
                    if (current_velocity < 0) {
                        current_velocity = 0;
                    }
                } else {
                    // Reverse if already stopped
                    current_velocity -= acceleration * deltaT;
                    if (current_velocity < -max_speed * 0.5f) { // Reverse speed is half max speed
                        current_velocity = -max_speed * 0.5f;
                    }
                }
            } else if (space_braking) {
                // Space bar for quick braking
                if (current_velocity > 0) {
                    current_velocity -= brake_deceleration * 1.5f * deltaT;
                    if (current_velocity < 0) {
                        current_velocity = 0;
                    }
                } else if (current_velocity < 0) {
                    current_velocity += brake_deceleration * 1.5f * deltaT;
                    if (current_velocity > 0) {
                        current_velocity = 0;
                    }
                }
            } else {
                // Natural deceleration (friction) when no input
                if (current_velocity > 0) {
                    current_velocity -= deceleration * deltaT;
                    if (current_velocity < 0) {
                        current_velocity = 0;
                    }
                } else if (current_velocity < 0) {
                    current_velocity += deceleration * deltaT;
                    if (current_velocity > 0) {
                        current_velocity = 0;
                    }
                }
            }
            
            // Move the car based on current velocity
            if (abs(current_velocity) > 0.01f) { // Only move if velocity is significant
                glm::vec3 forward_direction = glm::vec3(glm::rotate(glm::mat4(1.0f), carYaw,
                                                                   glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0, -1, 0, 1));
                glm::vec3 new_position = carPos + current_velocity * forward_direction * deltaT;
                
                if (check_position(new_position)) {
                    carPos = new_position;
                    crash_detected = false;
                } else {
                    // Collision detected - stop the car and apply damage
                    current_velocity = 0; // Stop immediately on collision
                    if (!crash_detected) {
                        num_crashes++;
                        crash_detected = true;
                        if (num_crashes >= MAX_DAMAGE) {
                            state = GameState::GameOver;
                            GameOver = true;
                        }
                    }
                }
            }
            
            // Update move_speed for UI display (show current velocity magnitude)
            move_speed = abs(current_velocity);
            
            // Realistic steering - only works when car is moving
            bool can_steer = abs(current_velocity) > min_steering_speed;
            
            if (can_steer) {
                // Calculate steering responsiveness based on speed
                // Slower speeds = more responsive steering, faster speeds = less responsive
                float speed_factor = abs(current_velocity);
                float steering_multiplier;
                
                if (speed_factor <= max_steering_speed) {
                    // Linear interpolation: full responsiveness at low speed, reduced at high speed
                    steering_multiplier = 1.0f - (speed_factor / max_steering_speed) * 0.5f;
                } else {
                    // Minimum responsiveness for very high speeds
                    steering_multiplier = 0.5f;
                }
                
                // Apply steering with speed-dependent responsiveness
                // When reversing, steering is inverted (realistic car behavior)
                float steering_direction = (current_velocity >= 0) ? 1.0f : -1.0f;
                
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                    float effective_rotation = deltaT * rotate_speed * steering_multiplier * steering_direction;
                    carYaw -= effective_rotation;
                }
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                    float effective_rotation = deltaT * rotate_speed * steering_multiplier * steering_direction;
                    carYaw += effective_rotation;
                }
            } else {
                // Car is too slow to steer - no steering happens
            }
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
                cameraYaw += deltaT * rotate_speed;
            }
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
                cameraYaw -= deltaT * rotate_speed;
            }

        }        else {
            // Game over state is now handled by the P key through the standard input handling
            // Just keep this section for possible future extensions
        }


        gameState(carPos);


    }

    void gameState(glm::vec3 pos) {
        if (timer.getElapsedTime()>=300.0f) {
            state = GameState::GameOver;
            GameOver = true;
            timer.stop();
        }            switch (state) {
                case GameState::SplashScreen:
                    break;
                case GameState::ControlsScreen:
                    break;
                case GameState::Paused:
                    strcpy(instruction, "GAME PAUSED - Press P to continue or ESC to exit");
                    break;
                case GameState::GoToPark:
                    SC.TI[0].I[1].Wm[3] = glm::vec4(glm::vec3(41.0871f,-25.9646f,0.0f), 1.0f);
                    strcpy(instruction, "Go to the small park");
                    if (carPos.x>=40.0f and carPos.x<42.0f) {
                        if (carPos.y>=-26.8 and carPos.y<-24.0f) {
                            state = GameState::GoToBank;
                            strcpy(instruction, "Go to the bank");
                            SC.TI[0].I[1].Wm[3] = glm::vec4(glm::vec3(-15.1456f,-1.14702f,0.0f), 1.0f);

                        }
                    }
                break;
                case GameState::GoToBank:
                    if (carPos.x>=-16.1456f and carPos.x<-14.0f) {
                        if (carPos.y>=-2.0f and carPos.y<-0.1f) {
                            state = GameState::GoToParking;
                            strcpy(instruction, "Go to the parking lot");
                            SC.TI[0].I[1].Wm[3] = glm::vec4(glm::vec3(1.11423f,-44.9671f,0.0f), 1.0f);

                        }
                    }
                break;
                case GameState::GoToParking:
                    if (carPos.x>=0.3f and carPos.x<2.0f) {
                        if (carPos.y>=-45.50f and carPos.y<-44.8f) {
                            state = GameState::GoToFactory;
                            strcpy(instruction, "Go to the small factory");
                            SC.TI[0].I[1].Wm[3] = glm::vec4(glm::vec3(23.631f,-81.322f,0.0f), 1.0f);

                        }
                    }
                break;
                case GameState::GoToFactory:
                    if (carPos.x>=22.5f and carPos.x<24.5f) {
                        if (carPos.y>=-82.50f and carPos.y<-80.0f) {
                            state = GameState::GoToStore;
                            strcpy(instruction, "Go to the store next city");
                            SC.TI[0].I[1].Wm[3] = glm::vec4(glm::vec3(-137.365f,-11.0243f,0.0f), 1.0f);

                        }
                    }
                break;
                case GameState::GoToStore:
                    if (carPos.x>=-138.3f and carPos.x<-136.5f) {
                        if (carPos.y>=-12.20f and carPos.y<-10.0f) {
                            state = GameState::GoToSecFactory;
                            strcpy(instruction, "Go to the factory ahead of you");
                            SC.TI[0].I[1].Wm[3] = glm::vec4(glm::vec3(-137.189f,-62.9413f,0.0f), 1.0f);

                        }
                    }
                break;
                case GameState::GoToSecFactory:
                    if (carPos.x>=-138.3f and carPos.x<-136.5f) {
                        if (carPos.y>=-64.0f and carPos.y<-61.9f) {
                            state = GameState::Return;
                            strcpy(instruction, "Return to spawn to win the game");
                            SC.TI[0].I[1].Wm[3] = glm::vec4(glm::vec3(0.0f,0.0f,0.0f), 1.0f);

                        }
                    }
                break;
                case GameState::Return:
                    if (carPos.x>=-1.0f and carPos.x<1.0f) {
                        if (carPos.y>=-1.0f and carPos.y<1.0f) {
                            state = GameState::GameWon;
                            timer.stop();
                            SC.TI[0].I[1].Wm[3] = glm::vec4(glm::vec3(0.0f,0.0f,500.0f), 1.0f);

                        }
                    }
                break;                case GameState::GameOver:
                    strcpy(instruction, "Game Over. Press P to restart game");
                    timer.stop();
                    SC.TI[0].I[1].Wm[3] = glm::vec4(glm::vec3(0.0f,0.0f,500.0f), 1.0f);
                break;
                case GameState::GameWon:
                    strcpy(instruction, "You Won! Press P to play again");
                    GameOver = true;
                break;




            }
    }

    bool check_position(glm::vec3 pos) {
        if (pos.x >= -1.5f && pos.x <= 1.5f) {
            if (pos.y >= -81.5f && pos.y <= 1.5f) {
                return true;
            }
        }

        if (pos.x >= -49.0f && pos.x <= 41.5f) {
            if (pos.y >= -1.5f && pos.y <= 1.5f) {
                return true;
            }
        }
        if (pos.x <= 41.5f && pos.x >= 38.5) {
            if (pos.y <= 1.5f && pos.y >= -81.5f) {
                return true;
            }
        }
        if (pos.x <= 41.5f && pos.x >= -137.5) {
            if (pos.y <= -78.5 && pos.y >= -81.5f) {
                return true;
            }
        }
        if (pos.x>=-49.5 && pos.x<=-46.5) {
            if (pos.y<=1.5 && pos.y>=-81.5f) {
                return true;
            }

        }

        //Here the other part of the city.
        if (pos.x<=-102.5 && pos.x>=-105.5) {
            if (pos.y<=9.5 && pos.y>=-81.5f) {
                return true;
            }
        }
        if (pos.x<=-105.5 && pos.x>=-137.5) {
            if (pos.y<=9.5 && pos.y>=6.5) {
                return true;
            }
        }
        if (pos.x<=-134.5 && pos.x>=-137.5) {
            if (pos.y<=9.5 && pos.y>=-81.5) {
                return true;
            }
        }





            return false;
    }
};


int main() {
    VehicleSimulator app;

    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
