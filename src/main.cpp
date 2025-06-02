#include <sstream>

#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"

#define Width 1280
#define Height 720

struct UniformBufferObject {
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

    //Here we will list all the object needed for our project

    DescriptorSetLayout DSLgubo, DSLmesh;

    VertexDescriptor Vmesh;
    RenderPass RP;
    Pipeline Pmesh;

    //Here we init our models, textures, descriptors etc...

    //Model
    //Texture

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

        RP.init(this);
        RP.properties[0].clearValue = {1.0f, 0.0f, 0.0f, 1.0f};

        //Pipelines
        Pmesh.init(this, &Vmesh, "shaders/Mesh.vert.spv", "shaders/Mesh.frag.spv", {&DSLgubo, &DSLmesh});

        //Here we will init our models and textures when we will have them
    }

    // Here you create your pipelines and Descriptor Sets!
    void pipelinesAndDescriptorSetsInit() {
    }

    //Here the cleanup of the pipelines and descriptor sets
    void pipelinesAndDescriptorSetsCleanup(){}

    // Here you destroy all the Models, Texture and Desc. Set Layouts you created!
    // All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
    // You also have to destroy the pipelines: since they need to be rebuilt, they have two different
    // methods: .cleanup() recreates them, while .destroy() delete them completely
    void localCleanup(){}

    static void populateCommandBufferAccess(VkCommandBuffer commandBuffer, int currentImage, void *Params) {}
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int CurrentImage) {}

    //APP logic


    void updateUniformBuffer(uint32_t currentImage){}


};


