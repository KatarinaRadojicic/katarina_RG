#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

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


// ekran
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.2;
float lastY = (float)SCR_HEIGHT / 2.2;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // kreiranje prozora
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PAC-MAN", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);


    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    glEnable(GL_DEPTH_TEST);

    // build and compile shaders

    Shader shader("resources/shaders/vertexShader.vs", "resources/shaders/fragmentShader.fs");
   // Shader shader("10.2.instancing.vs", "10.2.instancing.fs");
    Shader skyboxx("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");

    // model
    Model kuca(FileSystem::getPath("resources/objects/kuca/cottage.obj"));
    Model packman(FileSystem::getPath("resources/objects/Pac-Man/Pac-Man.obj"));
    Model piano(FileSystem::getPath("resources/objects/Piano/Piano.obj"));
    Model woodel(FileSystem::getPath("resources/objects/wood/Wood.obj"));
    Model tree(FileSystem::getPath("resources/objects/78-tree/Tree/3d files/tree.obj"));
    Model woodTable(FileSystem::getPath("resources/objects/Wood Table with glasplatte/Wood_Table.obj"));
    Model bed(FileSystem::getPath("resources/objects/bed/bed.obj"));
    Model plants(FileSystem::getPath("resources/objects/3dexport_hourglass_planter_obj_1676848285/Hourglass Planter.obj"));
    Model slight(FileSystem::getPath("resources/objects/light/StreetLight 3 OBJ.obj"));

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

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // tekstura
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

    //pozicije drveca
    srand(9);
    const unsigned int NR_TREES = 100;
    std::vector<glm::vec3> treePos;
    for(int i = 0; i < NR_TREES; i++)
        treePos.push_back(glm::vec3(rand() % 250 - 199, -1.0f, rand() % 200 - 200));

    // shader configuration
    skyboxx.use();
    skyboxx.setInt("skybox", 0);


    shader.use();
    shader.setInt("texture1", 0);


    // petlja renderovanja
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw scene
        shader.use();
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        // pack-man
        model= glm::mat4(1.0f);
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

        //renderovanje druge kuce
        model= glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-10.0f, -1.0f, -10.0f));
        model = glm::scale(model, glm::vec3(0.7f));
        shader.setMat4("model", model);
        kuca.Draw(shader);

        //draw planter
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
        model = glm::translate(model, glm::vec3(-4.0f, -1.0f, -4.0f));
        model = glm::scale(model, glm::vec3(0.2f));
        shader.setMat4("model", model);
        woodel.Draw(shader);

        //draw table
        model= glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.9f, -1.0f, -0.3f));
        model = glm::scale(model, glm::vec3(0.9f));
        shader.setMat4("model", model);
        woodTable.Draw(shader);

        // draw skybox
        glDepthFunc(GL_LEQUAL);
        skyboxx.use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
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

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));


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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
