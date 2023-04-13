#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>



void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadCubemap(vector<std::string> faces);
unsigned int loadTexture(char const * path);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void renderCube();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main()
{
     // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    Shader shader("resources/shaders/object.vs", "resources/shaders/object.fs");
    Shader shaderB("resources/shaders/3.1.blending.vs", "resources/shaders/3.1.blending.fs");
    Shader skyboxx("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader objShader("resources/shaders/ob.vs", "resources/shaders/ob.fs");
    Shader shaderLightBox("resources/shaders/light.vs", "resources/shaders/light.fs");

    // model
    Model kuca(FileSystem::getPath("resources/objects/kuca/cottage.obj"));
    kuca.SetShaderTextureNamePrefix("material.");
    Model packman(FileSystem::getPath("resources/objects/Pac-Man/Pac-Man.obj"));
    packman.SetShaderTextureNamePrefix("material.");
    Model piano(FileSystem::getPath("resources/objects/Piano/Piano.obj"));
    piano.SetShaderTextureNamePrefix("material.");
    Model woodel(FileSystem::getPath("resources/objects/wood/Wood.obj"));
    woodel.SetShaderTextureNamePrefix("material.");
    Model tree(FileSystem::getPath("resources/objects/78-tree/Tree/3d files/tree.obj"));
    tree.SetShaderTextureNamePrefix("material.");
    Model woodTable(FileSystem::getPath("resources/objects/Wood Table with glasplatte/Wood_Table.obj"));
    woodTable.SetShaderTextureNamePrefix("material.");
    Model bed(FileSystem::getPath("resources/objects/bed/bed.obj"));
    bed.SetShaderTextureNamePrefix("material.");
    Model plants(FileSystem::getPath("resources/objects/3dexport_hourglass_planter_obj_1676848285/Hourglass Planter.obj"));
    plants.SetShaderTextureNamePrefix("material.");
    Model pool(FileSystem::getPath("resources/objects/pool/avika-curved_pool_ver1/avika-curved_pool_ver1.obj"));
    pool.SetShaderTextureNamePrefix("material.");

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 0.1f;
    pointLight.linear = 1.0f;
    pointLight.quadratic = 1.0f;
    // PointLight& pointLight = programState->pointLight;
    // pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    // pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    // pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    // pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    // pointLight.constant = 1.0f;
    // pointLight.linear = 0.09f;
    // pointLight.quadratic = 0.032f;

    //model matrica iz learnopengl-a
    // glm::mat4* modelMatrices;
    // modelMatrices = new glm::mat4[amount];
    // srand(static_cast<unsigned int>(glfwGetTime())); // initialize random seed
    // float radius = 50.0;
    // float offset = 2.5f;
    // for (unsigned int i = 0; i < amount; i++)
    // {
    //     glm::mat4 model = glm::mat4(1.0f);
    //     // 1. translation: displace along circle with 'radius' in range [-offset, offset]
    //     float angle = (float)i / (float)amount * 360.0f;
    //     float displacement = (rand() % (int)(2 * offset * 100)) / 150.0f - offset;
    //     float x = sin(angle) * radius + displacement;
    //     displacement = (rand() % (int)(2 * offset * 100)) / 150.0f - offset;
    //     float z = cos(angle) * radius + displacement;
    //     model = glm::translate(model, glm::vec3(x-25, 5, z-25));

    //     // 2. scale: Scale between 0.05 and 0.25f
    //     float scale = static_cast<float>((rand() % 20) / 100.0 + 0.05);
    //     model = glm::scale(model, glm::vec3(scale));

    //     // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
    //     float rotAngle = static_cast<float>((rand() % 360));
    //     model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

    //     // 4. now add to list of matrices
    //     modelMatrices[i] = model;
    // }

    
    std::vector<glm::vec3> lightColors;
    std::vector<glm::vec3> lightPositions;
    unsigned int brSv=6;
    float x=6, y=0, z=8;
    float offset=0.6f;
    for (unsigned int i = 0; i < brSv; i++)
    { 
        lightPositions.push_back(glm::vec3(x+offset, y, z+offset/2));
        float rColor = ((rand() % 100) / 200.0f); // between 0.1 and 0.5
        float gColor = ((rand() % 100) / 200.0f) + 0.2; // between 0.1 and 0.7
        float bColor = ((rand() % 100) / 200.0f) + 0.1; // between 0.1 and 0.6
        lightColors.push_back(glm::vec3(rColor, gColor, bColor));
    }


    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };
    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };
    float cubeVertices[] = {
            // positions          // texture Coords
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
            0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    // cube VAO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    // transparent VAO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // teksture
    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/nightsky/nightsky_ft.tga"),
                    FileSystem::getPath("resources/textures/nightsky/nightsky_bk.tga"),
                    FileSystem::getPath("resources/textures/nightsky/nightsky_up.tga"),
                    FileSystem::getPath("resources/textures/nightsky/nightsky_dn.tga"),
                    FileSystem::getPath("resources/textures/nightsky/nightsky_rt.tga"),
                    FileSystem::getPath("resources/textures/nightsky/nightsky_lf.tga")
            };
    unsigned int cubemapTexture = loadCubemap(faces);
    unsigned int cubeTexture = loadTexture(FileSystem::getPath("resources/textures/brick-subsea.jpg").c_str());
    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/grass.png").c_str());




    // transparent vegetation locations
    vector<glm::vec3> vegetation
            {
                    glm::vec3(-6.5f, -1.0f, -5.48f),
                    glm::vec3( -3.5f, -1.0f, -4.49f),
                    glm::vec3( -5.0f, -1.0f, -4.3f),
                    // glm::vec3(-0.3f, 0.0f, -2.3f),
                    glm::vec3 (-4.5f, -1.0f, -5.6f)
            };


    //pozicije drveca
    srand(9);
    const unsigned int NR_TREES = 100;
    std::vector<glm::vec3> treePos;
    for(int i = 0; i < NR_TREES; i++)
        treePos.push_back(glm::vec3(rand() % 250 - 199, -1.0f, rand() % 200 - 200));
    
    skyboxx.use();
    skyboxx.setInt("skybox", 0);

  
    shaderB.use();
    shaderB.setInt("texture1", 0);


    // petlja renderovanja
    while (!glfwWindowShouldClose(window))
    {
         // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_CULL_FACE);

        // don't forget to enable shader before setting uniforms
        shader.use();
         // directional light
        shader.setVec3("dirLight.direction", -0.2f, 0.7f, 0.3f);
        shader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
        shader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
        shader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
        
        //pointlights
        shader.setVec3("pointLights[0].position",glm::vec3(1.0f, 2.0f, 1.2f));
        shader.setVec3("pointLights[0].ambient", pointLight.ambient);
        shader.setVec3("pointLights[0].diffuse", pointLight.diffuse);
        shader.setVec3("pointLights[0].specular", pointLight.specular);
        shader.setFloat("pointLights[0].constant", pointLight.constant);
        shader.setFloat("pointLights[0].linear", pointLight.linear);
        shader.setFloat("pointLights[0].quadratic", pointLight.quadratic);

        // shader.setVec3("pointLights[1].position", glm::vec3(6.7f+cos(currentFrame*2)*0.6f,1.15f+cos(currentFrame)*0.2f+cos(currentFrame)*0.4f,-17.45f+sin(currentFrame)*0.6f));
        // shader.setVec3("pointLights[1].ambient", pointLight.ambient);
        // shader.setVec3("pointLights[1].diffuse", pointLight.diffuse);
        // shader.setVec3("pointLights[1].specular", pointLight.specular);
        // shader.setFloat("pointLights[1].constant", pointLight.constant);
        // shader.setFloat("pointLights[1].linear", pointLight.linear);
        // shader.setFloat("pointLights[1].quadratic", pointLight.quadratic);
    
        
        // pointLight.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
        // shader.setVec3("pointLights[2].position", pointLight.position);
        // shader.setVec3("pointLights[2].ambient", pointLight.ambient);
        // shader.setVec3("pointLights[2].diffuse", pointLight.diffuse);
        // shader.setVec3("pointLight[2].specular", pointLight.specular);
        // shader.setFloat("pointLights[2].constant", pointLight.constant);
        // shader.setFloat("pointLights[2].linear", pointLight.linear);
        // shader.setFloat("pointLights[2].quadratic", pointLight.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

          // spotLight
          for(int i=0;i<brSv;i++){
        shader.setVec3("spotLights[i].position", lightPositions[i]);
        shader.setVec3("spotLights[i].direction",  glm::vec3(1.0f, 0.0f, -1.0f));
        shader.setVec3("spotLights[i].ambient", 1.0f, 0.0f, 0.0f);
        shader.setVec3("spotLights[i].diffuse", 1.0f, 1.0f, 1.0f);
        shader.setVec3("spotLights[i].specular", 1.0f, 1.0f, 1.0f);
        shader.setFloat("spotLights[i].constant", 1.0f);
        shader.setFloat("spotLights[i].linear", 0.1f);
        shader.setFloat("spotLights[i].quadratic", 0.032f);
        shader.setFloat("spotLights[i].cutOff", glm::cos(glm::radians(15.5f)));
        shader.setFloat("spotLights[i].outerCutOff", glm::cos(glm::radians(17.0f)));  
        }

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
       
    
        // pack-man
        glm::mat4  model= glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(7.0f, -1.0f, 7.0f));
        model = glm::scale(model, glm::vec3(0.009f));
        shader.setMat4("model", model);
        packman.Draw(shader);


        // kuca
        model= glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.0f, -1.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.6f, 0.6));
        shader.setMat4("model", model);
        kuca.Draw(shader);
         glEnable(GL_CULL_FACE);
        //draw piano
        model= glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.2f, -0.9f, 0.3f));
        model = glm::scale(model, glm::vec3(0.4f));
        shader.setMat4("model", model);
        piano.Draw(shader);

        //bed
        model= glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.3f, -1.0f, 1.9f));
        model = glm::scale(model, glm::vec3(0.06f));
        shader.setMat4("model", model);
        bed.Draw(shader);

        //renderovanje bazena
        model= glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(8.0f, -1.0f, 6.0f));
        model = glm::scale(model, glm::vec3(0.3f));
        shader.setMat4("model", model);
        pool.Draw(shader);

        //saksije ispred kuce
        model= glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.7f, -0.8f, 2.50f));
        model = glm::scale(model, glm::vec3(0.7f));
        shader.setMat4("model", model);
        plants.Draw(shader);

       
        //renderovanje drveca
        for (int i = 0; i < NR_TREES; i++) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, treePos[i]);
            model = glm::scale(model, glm::vec3(0.8f));
            shader.setMat4("model", model);
            tree.Draw(shader);
        }


        // drvo?
        model= glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-5.0f, -1.3f, -5.0f));
        model = glm::scale(model, glm::vec3(0.2f));
        shader.setMat4("model", model);
        woodel.Draw(shader);

        //draw table
        model= glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.9f, -1.0f, -0.3f));
        model = glm::scale(model, glm::vec3(0.9f));
        shader.setMat4("model", model);
        woodTable.Draw(shader);

         glDisable(GL_CULL_FACE);
        // cubes
        shaderB.use();
        shaderB.setVec3("view",  programState->camera.Position);
        shaderB.setMat4("projection", projection);
        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cubeTexture);
        model=glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-6.0f, -1.0f, -6.0f));
        shaderB.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-3.0f, -1.0f, -5.0f));
        shaderB.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);


        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        for (unsigned int i = 0; i < vegetation.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, vegetation[i]);
            shaderB.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

    //renderovanje svetla
        shaderLightBox.use();
        shaderLightBox.setMat4("projection", projection);
        shaderLightBox.setMat4("view", view);
        for (unsigned int i = 0; i <brSv; i++) {
            model=glm::mat4(1.0f);
            model=glm::translate(model, lightPositions[i]);
            model=glm::scale(model, glm::vec3(0.1));
            shaderLightBox.setMat4("model", model);
            shaderLightBox.setVec3("lightColor", sin((float) glfwGetTime() * lightColors[i]) / 2.0f + 0.5f);
            renderCube();
        }
       // point light kocka
        // model = glm::mat4(1.0f);
        // model = glm::translate(model, glm::vec3(-7.0f, -1.0f, -6.0f));
        // model = glm::scale(model, glm::vec3(0.5f));
        // shaderLightBox.setMat4("model", model);
        // shaderLightBox.setVec3("lightColor", glm::vec3(255.0f, 255.0f, 255.0f));
        // renderCube();


        // draw skybox
        glDepthFunc(GL_LEQUAL);
        skyboxx.use();
        skyboxx.setVec3("view", programState->camera.Position);
        skyboxx.setMat4("view", view);
        skyboxx.setMat4("projection", projection);

        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    
    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}



unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}


unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
                // back face
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
                // front face
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                // left face
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                // right face
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
                // bottom face
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                // top face
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}