#include <sstream>

#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include "modules/Scene.hpp"



#define Width 800
#define Height 600
#define MAX_YAW glm::radians(180.0f)

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
    const glm::mat4 lightView = glm::rotate(glm::mat4(1), glm::radians(-30.0f), glm::vec3(0.0f,1.0f,0.0f)) * glm::rotate(glm::mat4(1), glm::radians(-45.0f), glm::vec3(1.0f,0.0f,0.0f));
    const glm::vec3 lightDir = glm::vec3(lightView * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    glm::vec3 cameraPos;
    glm::vec3 camTarget;
    const float FOVy = glm::radians(90.0f);
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;
    glm::mat4 Prj = glm::mat4(1.0f);
    glm::mat4 View = glm::mat4(1.0f);
    glm::vec3 carPos = glm::vec3(0.0f);
    float carYaw = 0.0f;
    float cameraYaw = 0.0f;
    float objectYaw = 0.0f;
    bool rotate = false;
    //Here we will list all the object needed for our project

    DescriptorSetLayout DSLgubo, DSLmesh;

    VertexDescriptor Vmesh;
    RenderPass RP;
    Pipeline Pmesh;

    Scene SC;
    std::vector<VertexDescriptorRef>  VDRs;
    std::vector<TechniqueRef> PRs;


    void setWindowParameters() {
        windowWidth = Width;
        windowHeight = Height;
        windowTitle = "VehicleSimulator";
        windowResizable = GLFW_TRUE;
        initialBackgroundColor = {0.1f, 0.1f, 0.2f, 1.0f};
        aspectRatio = 4/3;
    }

    void onWindowResize(int w, int h) {
        aspectRatio = (float)w / (float)h;
        RP.width = w;
        RP.height = h;
    }

    //Here we initialize the descriptor set layouts and our models
    void localInit() {
        DSLgubo.init(this, {
                    // this array contains the binding:
                    // first  element : the binding number
                    // second element : the type of element (buffer or texture)
                    // third  element : the pipeline stage where it will be used
                    {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1}
                  });
        DSLmesh.init(this, {
            // this array contains the binding:
            // first  element : the binding number
            // second element : the type of element (buffer or texture)
            // third  element : the pipeline stage where it will be used
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(UniformBufferObject), 1},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
        });

        Vmesh.init(this, {
                  {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
                }, {
                  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
                         sizeof(glm::vec3), POSITION},
                  {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm),
                         sizeof(glm::vec3), NORMAL},
                  {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
                         sizeof(glm::vec2), UV}
                });

        VDRs.resize(1);
        VDRs[0].init("VMesh",   &Vmesh);

        RP.init(this);
        RP.properties[0].clearValue = {0.6f, 0.8f, 1.0f, 1.0f};

        //Pipelines
        Pmesh.init(this, &Vmesh, "shaders/Mesh.vert.spv", "shaders/Mesh.frag.spv", {&DSLgubo, &DSLmesh});

        PRs.resize(1);
        PRs[0].init("Simple", {
                             {&Pmesh, {//Pipeline and DSL for the first pass
                                 /*DSLglobal*/{},
                                 /*DSLlocalChar*/{
                                        /*t0*/{true,  0, {}}// index 0 of the "texture" field in the json file
                                     }
                                    }}
                              }, /*TotalNtextures*/1, &Vmesh);
        DPSZs.uniformBlocksInPool = 3;
        DPSZs.texturesInPool = 4;
        DPSZs.setsInPool = 3;

        std::cout << "\nLoading the scene\n\n";
        if(SC.init(this, /*Npasses*/1, VDRs, PRs, "assets/models/scene.json") != 0) {
            std::cout << "ERROR LOADING THE SCENE\n";
            exit(0);
        }

        submitCommandBuffer("main", 0, populateCommandBufferAccess, this);

    }

    // Here you create your pipelines and Descriptor Sets!
    void pipelinesAndDescriptorSetsInit() {
        RP.create();
        Pmesh.create(&RP);

        SC.pipelinesAndDescriptorSetsInit();


    }

    //Here the cleanup of the pipelines and descriptor sets
    void pipelinesAndDescriptorSetsCleanup() {
        Pmesh.cleanup();
        RP.cleanup();
        SC.pipelinesAndDescriptorSetsCleanup();
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
    }

    static void populateCommandBufferAccess(VkCommandBuffer commandBuffer, int currentImage, void *Params) {
        VehicleSimulator *T = (VehicleSimulator *)Params;
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

        cameraPos = glm::vec3(0.0f, 3.0f, 2.0f) + carPos;
        Prj = glm::perspective(FOVy, aspectRatio, nearPlane, farPlane);
        View =  glm::lookAt(cameraPos, carPos, glm::vec3(0,1,0));


        GlobalUniformBufferObject gubo{};
        gubo.lightDir = lightDir;
        gubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        gubo.ambLightColor = glm::vec3(1.0f);
        gubo.eyePos = cameraPos;

        UniformBufferObject ubo{};


        int instanceId;
        SC.TI[0].I[0].Wm[3] = glm::vec4(carPos,1.0f);
        if (rotate==true) {
            SC.TI[0].I[0].Wm = glm::rotate(SC.TI[0].I[0].Wm, objectYaw, glm::vec3(0.0f, 1.0f, 0.0f));
            rotate = false;
        }
        for(instanceId = 0; instanceId < SC.TI[0].InstanceCount; instanceId++) {
            ubo.mMat   = SC.TI[0].I[instanceId].Wm;
            ubo.mvpMat = Prj * View * ubo.mMat;
            ubo.nMat   = glm::inverse(glm::transpose(ubo.mMat));
            ubo.amb = 1.0f; ubo.gamma = 180.0f; ubo.sColor = glm::vec3(1.0f);
            SC.TI[0].I[instanceId].DS[0][0]->map(currentImage, &gubo, 0); // Set 0
            SC.TI[0].I[instanceId].DS[0][1]->map(currentImage, &ubo, 0);// Set 1

        }


    }
    void gameLogic(GLFWwindow *window) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        static auto lastTime = startTime;

        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaT = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        const float move_speed = 2.0f;
        const float rotate_speed = 1.0f;

        if (glfwGetKey(window, GLFW_KEY_W) ==GLFW_PRESS) {
            carPos +=move_speed * glm::vec3(glm::rotate(glm::mat4(1.0f), carYaw,
                                    glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0,-1,0,1)) * deltaT;

        }

        if (glfwGetKey(window, GLFW_KEY_S) ==GLFW_PRESS) {
            carPos +=move_speed * glm::vec3(glm::rotate(glm::mat4(1.0f), carYaw,
                                    glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0,1,0,1)) * deltaT;
        }
        if (glfwGetKey(window, GLFW_KEY_A) ==GLFW_PRESS) {
                if (carYaw>=-MAX_YAW) {
                    carYaw-=deltaT*rotate_speed;
                    objectYaw=-deltaT*rotate_speed;
                    rotate = true;
                }

        }
        if (glfwGetKey(window, GLFW_KEY_D) ==GLFW_PRESS) {
            if (carYaw<=MAX_YAW) {
                carYaw+=deltaT*rotate_speed;
                objectYaw=deltaT*rotate_speed;
                rotate = true;
            }

        }



    }

};


int main() {
    VehicleSimulator app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


