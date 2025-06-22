#include <sstream>

#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include "modules/Scene.hpp"
#include "GameState.cpp"

// this should work, i am doing it to try and  do a pull request

#define Width 800
#define Height 600
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
    glm::vec3 carPos = glm::vec3(0.0f);
    glm::vec3 offset = glm::vec3(0.0f, 3.0f, 2.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    float carYaw = 0.0f;
    float cameraYaw = 0.0f;
    float objectYaw = 0.0f;
    float move_speed = 2.0f;
    bool rotate = false;
    bool camRot = false;
    bool crash_detected = false;
    int num_crashes = 0;
    char instruction[120] = "hi";
    GameState state = GameState::SplashScreen;
    bool GameOver = false;
    //Here we will list all the object needed for our project

    DescriptorSetLayout DSLgubo, DSLmesh;

    VertexDescriptor Vmesh;
    RenderPass RP;
    Pipeline Pmesh;
    TextMaker txt;

    Scene SC;
    std::vector<VertexDescriptorRef> VDRs;
    std::vector<TechniqueRef> PRs;


    void setWindowParameters() {
        windowWidth = Width;
        windowHeight = Height;
        windowTitle = "VehicleSimulator";
        windowResizable = GLFW_TRUE;
        initialBackgroundColor = {0.1f, 0.1f, 0.2f, 1.0f};
        aspectRatio = 4 / 3;
    }

    void onWindowResize(int w, int h) {
        aspectRatio = (float) w / (float) h;
        RP.width = w;
        RP.height = h;
        txt.resizeScreen(w, h);

    }

    //Here we initialize the descriptor set layouts and our models
    void localInit() {
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

        VDRs.resize(1);
        VDRs[0].init("VMesh", &Vmesh);

        RP.init(this);
        RP.properties[0].clearValue = {0.6f, 0.8f, 1.0f, 1.0f};

        //Pipelines
        Pmesh.init(this, &Vmesh, "shaders/Mesh.vert.spv", "shaders/Mesh.frag.spv", {&DSLgubo, &DSLmesh});


        Pmesh.setCullMode(VK_CULL_MODE_NONE);


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
                        }
                    }, /*TotalNtextures*/1, &Vmesh);
        DPSZs.uniformBlocksInPool = 3;
        DPSZs.texturesInPool = 4;
        DPSZs.setsInPool = 3;

        std::cout << "\nLoading the scene\n\n";
        if (SC.init(this, /*Npasses*/1, VDRs, PRs, "assets/models/scene.json") != 0) {
            std::cout << "ERROR LOADING THE SCENE\n";
            exit(0);
        }
        txt.init(this, windowWidth, windowHeight);


        submitCommandBuffer("main", 0, populateCommandBufferAccess, this);
    }

    // Here you create your pipelines and Descriptor Sets!
    void pipelinesAndDescriptorSetsInit() {
        RP.create();
        Pmesh.create(&RP);

        SC.pipelinesAndDescriptorSetsInit();
        txt.pipelinesAndDescriptorSetsInit();
    }

    //Here the cleanup of the pipelines and descriptor sets
    void pipelinesAndDescriptorSetsCleanup() {
        Pmesh.cleanup();
        RP.cleanup();
        SC.pipelinesAndDescriptorSetsCleanup();
        txt.pipelinesAndDescriptorSetsCleanup();
    }

    // Here you destroy all the Models, Texture and Desc. Set Layouts you created!
    // All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
    // You also have to destroy the pipelines: since they need to be rebuilt, they have two different
    // methods: .cleanup() recreates them, while .destroy() delete them completely
    void localCleanup() {
        DSLgubo.cleanup();
        DSLmesh.cleanup();
        Pmesh.destroy();
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

        SC.populateCommandBuffer(commandBuffer, 0, CurrentImage);

        RP.end(commandBuffer);
    }

    //APP logic

    //APP logic
    void updateUniformBuffer(uint32_t currentImage) {
        gameLogic(window);

        RP.properties[0].clearValue = {0.6f, 0.8f, 1.0f, 1.0f};
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), cameraYaw, glm::vec3(0, 0, 1));
        glm::vec4 rotatedOffset = rotation * glm::vec4(offset, 1.0f);
        cameraPos = glm::vec3(rotatedOffset) + carPos;

        glm::vec4 rotatedUp = rotation * glm::vec4(0, 1, 0, 0);
        up = glm::normalize(glm::vec3(rotatedUp));

        Prj = glm::perspective(FOVy, aspectRatio, nearPlane, farPlane);
        View = glm::lookAt(cameraPos, carPos, up);


        GlobalUniformBufferObject gubo{};
        gubo.lightDir = lightDir;
        gubo.lightColor = glm::vec4(0.8f, 0.7f, 0.70f, 1.0f);
        gubo.ambLightColor = glm::vec3(0.6f);
        gubo.eyePos = cameraPos;

        UniformBufferObject ubo{};


        int instanceId;
        SC.TI[0].I[0].Wm[3] = glm::vec4(carPos, 1.0f);
        if (rotate == true) {
            SC.TI[0].I[0].Wm = glm::rotate(SC.TI[0].I[0].Wm, objectYaw, glm::vec3(0.0f, 1.0f, 0.0f));
            rotate = false;
        }
        for (instanceId = 0; instanceId < SC.TI[0].InstanceCount; instanceId++) {
            ubo.mMat = SC.TI[0].I[instanceId].Wm;
            ubo.mvpMat = Prj * View * ubo.mMat;
            ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
            ubo.amb = 0.3f;
            ubo.gamma = 180.0f;
            ubo.sColor = glm::vec3(0.8f);
            SC.TI[0].I[instanceId].DS[0][0]->map(currentImage, &gubo, 0); // Set 0
            SC.TI[0].I[instanceId].DS[0][1]->map(currentImage, &ubo, 0); // Set 1
        }

        if(state == GameState::SplashScreen) {
            std:: ostringstream instr;
            instr << "\n\n\nVEHICLE SIMULATOR\n\n";
            instr << "Controls:\n\n";
            instr << "W: Move Forward\n";
            instr << "S: Move Backward\n";
            instr << "A: Turn Left\n";
            instr << "D: Turn Right\n";
            instr << "Q/E: Rotate Camera\n\n";
            instr << "Press P to Start";
            txt.print(0.0f, 0.0f, instr.str(), 3, "CO", false, true, false, TAL_CENTER, TRH_CENTER, TRV_MIDDLE, {1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});
        } else {
            std:: ostringstream speed;
            std:: ostringstream crashes;
            std:: ostringstream instr;
            speed<<"Speed:  "<<move_speed<<"\n";
            crashes<<"Damage:: " << num_crashes<<"\n";
            instr <<instruction;
            txt.print(1.0f, 1.0f, speed.str(), 1, "CO", false, true, false,TAL_RIGHT,TRH_RIGHT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});
            txt.print(-1.0f, 1.0f, crashes.str(), 2, "CO", false, true, false,TAL_LEFT,TRH_LEFT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});
            txt.print(-1.0f, -0.9f, instr.str(), 3, "CO", false, true, false,TAL_LEFT,TRH_LEFT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});
        }


        txt.updateCommandBuffer();
    }

    void gameLogic(GLFWwindow *window) {
        if(state == GameState::SplashScreen) {
            if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
                state = GameState::GoToPark;
            }
            return;
        }
        static auto startTime = std::chrono::high_resolution_clock::now();
        static auto lastTime = startTime;

        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaT = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        const float rotate_speed = 1.0f;
        if (!GameOver) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                if (check_position(carPos + move_speed * glm::vec3(glm::rotate(glm::mat4(1.0f), carYaw,
                                                                               glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(
                                                                       0, -1, 0, 1)) * deltaT) == true) {
                    carPos += move_speed * glm::vec3(glm::rotate(glm::mat4(1.0f), carYaw,
                                                                 glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0, -1, 0, 1)) *
                            deltaT;
                    std::cout<<"Pos:(x,y) "<<carPos.x <<" " <<carPos.y <<"\n ";
                    crash_detected = false;
                                                                       }
                else {
                    if (crash_detected== false) {
                        num_crashes++;
                        crash_detected = true;
                        if (num_crashes>=MAX_DAMAGE) {
                            state = GameState::GameOver;
                            GameOver = true;
                        }
                    }
                }
            }

            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                if (check_position(carPos - move_speed * glm::vec3(glm::rotate(glm::mat4(1.0f), carYaw,
                                                                               glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(
                                                                       0, -1, 0, 1)) * deltaT) == true) {
                    carPos -= move_speed * glm::vec3(glm::rotate(glm::mat4(1.0f), carYaw,
                                                                 glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0, -1, 0, 1)) *
                            deltaT;
                    std::cout<<"Pos:(x,y) "<<carPos.x <<" " <<carPos.y <<" \n";
                    crash_detected = false;
                                                                       }
                else {
                    if (crash_detected== false) {
                        num_crashes++;
                        crash_detected = true;
                        if (num_crashes>=MAX_DAMAGE) {
                            state = GameState::GameOver;
                            GameOver = true;
                        }
                    }
                }
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                carYaw -= deltaT * rotate_speed;
                objectYaw = -deltaT * rotate_speed;
                rotate = true;

            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                carYaw += deltaT * rotate_speed;
                objectYaw = deltaT * rotate_speed;
                rotate = true;

            }
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
                cameraYaw += deltaT * rotate_speed;
            }
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
                cameraYaw -= deltaT * rotate_speed;
            }

            if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
                if (move_speed < 10.0f) {
                    move_speed = move_speed + 1.0f;
                    std::cout << "move_speed = " << move_speed << std::endl;
                }
            }
            if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
                if (move_speed > 1.0f) {
                    move_speed = move_speed - 1.0f;
                    std::cout << "move_speed = " << move_speed << std::endl;
                }
            }
        }
        else {
            if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
                state = GameState::GoToPark;
                carPos = glm::vec3(0.0f,0.0f,0.0f);
                rotate = false;
                camRot = false;
                crash_detected = false;
                GameOver = false;
                num_crashes = 0;
                move_speed = 2.0f;
            }
        }

        gameState(carPos);


    }

    void gameState(glm::vec3 pos) {
            switch (state) {
                case GameState::SplashScreen:
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
                            SC.TI[0].I[1].Wm[3] = glm::vec4(glm::vec3(0.0f,0.0f,500.0f), 1.0f);

                        }
                    }
                break;
                case GameState::GameOver:
                    strcpy(instruction, "Game Over. Press O to restart");
                    SC.TI[0].I[1].Wm[3] = glm::vec4(glm::vec3(0.0f,0.0f,500.0f), 1.0f);
                case GameState::GameWon:
                    strcpy(instruction, "You Won. Press O to restart");
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
