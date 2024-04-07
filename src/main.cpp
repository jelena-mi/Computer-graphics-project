#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(const char* path);
unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, -3.5f, 0.0f));
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
    glm::vec3 modelPosition;
    glm::vec3 modelRelativePosition;
    glm::vec3 modelOffset = glm::vec3(0.0f, -6.0f, -20.0f);
    float modelScale = 0.5f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, -3.5f, 0.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

struct Insect {
    glm::vec3 position;
    bool eaten;

    Insect(glm::vec3 pos) : position(pos), eaten(false) {}
};

struct Bird {
    glm::vec3 position;
    bool eaten;

    Bird() {}
    Bird(glm::vec3 pos) : position(pos), eaten(false) {}
};
Bird bird;

float thresholdDistanceInsects = 4.0f;
float thresholdDistanceFalcon = 5.0f;
float proximityThreshold = 8.0f;
float falconDistance = std::numeric_limits<float>::max();
float closestInsectDistance = std::numeric_limits<float>::max();
int closestInsectIdx = -1;

glm::vec3 airBalloonPosition = glm::vec3(0.0f, -20.0f, -35.0f);
glm::vec3 falconPosition = glm::vec3(0.0f, 0.0f, -25.0f);

//insects positions
vector<Insect> insects
        {
                glm::vec3( 0.0f, -6.0f, -35.0f),
                glm::vec3(-8.0f, -5.0f, -40.0f),
                glm::vec3(2.0f, -5.5f, -30.0f),
                glm::vec3 (-10.0f, -4.5f, -35.0f),
                glm::vec3(-4.0f, -4.0f, -40.0f)
        };
int remainingInsects = insects.size();

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

int main() {
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // build and compile shaders
    // -------------------------
    Shader modelShader("resources/shaders/model_lighting.vs", "resources/shaders/model_lighting.fs");
    Shader blendingShader("resources/shaders/blending.vs", "resources/shaders/blending.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");

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

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/rt.jpg"),
                    FileSystem::getPath("resources/textures/skybox/lf.jpg"),
                    FileSystem::getPath("resources/textures/skybox/up.jpg"),
                    FileSystem::getPath("resources/textures/skybox/dn.jpg"),
                    FileSystem::getPath("resources/textures/skybox/ft.jpg"),
                    FileSystem::getPath("resources/textures/skybox/bk.jpg")
            };
    unsigned int cubemapTexture = loadCubemap(faces);

    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/transparent_cloud1.png").c_str());

    // transparent clouds locations
    // --------------------------------
    vector<glm::vec3> clouds
            {
                    glm::vec3(-3.0f, -1.0f, -3.0f),
                    glm::vec3(-2.95f, -2.0f, -6.0f),
                    glm::vec3(-2.9f, 0.0f, -6.0f),
                    glm::vec3(-2.5f, -1.0f, -9.0f),
                    glm::vec3(-1.6f, -2.0f, -12.0f),
                    glm::vec3(-1.5f, 0.0f, -12.0f),
                    glm::vec3(0.0f, -1.0f, -15.0f),

                    glm::vec3(1.5f, 0.0f, -12.0f),
                    glm::vec3(1.6f, -2.0f, -12.0f),
                    glm::vec3(2.5f, -1.0f, -9.0f),
                    glm::vec3(2.9f, 0.0f, -6.0f),
                    glm::vec3(2.95f, -2.0f, -6.0f),
                    glm::vec3(3.0f, -1.0f, -3.0f)
            };
    blendingShader.use();
    blendingShader.setInt("texture1", 0);

    // shader configuration
    // --------------------
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);



    // load models
    // -----------
    Model ourModel("resources/objects/backpack/backpack.obj");
    Model abModel("resources/objects/air_balloon/11809_Hot_air_balloon_l2.obj");
    Model fModel("resources/objects/falcon/peregrine_falcon.obj");
    Model bModel("resources/objects/bird/bird.obj");
    Model iModel("resources/objects/insect/insect.obj");

    ourModel.SetShaderTextureNamePrefix("material.");
    abModel.SetShaderTextureNamePrefix("material.");
    fModel.SetShaderTextureNamePrefix("material.");
    bModel.SetShaderTextureNamePrefix("material.");
    iModel.SetShaderTextureNamePrefix("material.");


    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(2.0f, 2.0, -30.0);
    pointLight.ambient = glm::vec3(0.9, 0.9, 0.9);
    pointLight.diffuse = glm::vec3(0.8, 0.8, 0.8);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.005f;
    pointLight.quadratic = 0.002f;


    bird = Bird(programState->modelRelativePosition);


    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
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


        // don't forget to enable shader before setting uniforms
        modelShader.use();

        //pointLight position
        float radius = 5.0f;
        float centerX = 4.0f;
        float centerZ = -30.0f;
        float angle = 1.0f * currentFrame; // speed of rotation
        float x = centerX + radius * cos(angle);
        float z = centerZ + radius * sin(angle);
        pointLight.position = glm::vec3(x, 2.0f, z);

        modelShader.setVec3("pointLight.position", pointLight.position);
        modelShader.setVec3("pointLight.ambient", pointLight.ambient);
        modelShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        modelShader.setVec3("pointLight.specular", pointLight.specular);
        modelShader.setFloat("pointLight.constant", pointLight.constant);
        modelShader.setFloat("pointLight.linear", pointLight.linear);
        modelShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        modelShader.setVec3("viewPosition", programState->camera.Position);
        modelShader.setFloat("material.shininess", 32.0f);

        modelShader.setVec3("dirLight.direction", glm::vec3(-10.0f, 10.0f, 0.0f));
        modelShader.setVec3("dirLight.ambient", glm::vec3(0.05f));
        modelShader.setVec3("dirLight.diffuse", glm::vec3(0.4f));
        modelShader.setVec3("dirLight.specular", glm::vec3(0.5f));

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();

        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);



        // render the loaded model

        // air balloon
        modelShader.use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, airBalloonPosition);
        model = glm::scale(model, glm::vec3(0.01f));
        model = glm::translate(model,glm::vec3(cos(0.1f*currentFrame)*3600.0f, 0.0f, sin(0.1f*currentFrame)*3600.0f+1000));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        modelShader.setMat4("model", model);
        abModel.Draw(modelShader);


        // falcon
        modelShader.use();
        model = glm::mat4(0.8f);
        model = glm::translate(model, falconPosition);
        model = glm::scale(model, glm::vec3(0.2f));
        model = glm::translate(model,glm::vec3(cos(0.15*currentFrame)*50.0f, -5.0f, sin(0.15*currentFrame)*50.0f));
        model = glm::rotate(model, glm::radians(0.15f*currentFrame), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelShader.setMat4("model", model);
        fModel.Draw(modelShader);
        falconPosition = glm::vec3(model[3]);
        falconDistance = glm::distance(programState->modelPosition, falconPosition);


        // bird
        // check is the bird eaten by falcon
        if (falconDistance < thresholdDistanceFalcon) {
            bird.eaten = true;
        }

        // render the bird
        if(!bird.eaten) {
            modelShader.use();
            model = glm::translate(glm::mat4(1.0f),
                                   programState->modelRelativePosition);   // update model position based on camera
            model = glm::scale(model, glm::vec3(programState->modelScale));
            model = glm::translate(model, glm::vec3(0.0f, sin(2.5f * currentFrame) * 0.5f, 0.0f));
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            modelShader.setMat4("model", model);
            bModel.Draw(modelShader);
        }


        // insects

        // check if insects are eaten by bird
        for (unsigned int i = 0; i < insects.size(); i++) {

            if (!insects[i].eaten) {
                // calculate the distance between the bird and the insect
                float distance = glm::distance(programState->modelPosition, insects[i].position);

                // check if the bird is close to the insect
                if (distance < thresholdDistanceInsects) {
                    insects[i].eaten = true;
                    remainingInsects--;
                }
            }
        }

        // render visible insects
        modelShader.use();
        for (unsigned int i = 0; i < insects.size(); i++) {
            if (!(insects[i].eaten)) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, insects[i].position);
                model = glm::scale(model, glm::vec3(0.01f));
                model = glm::translate(model,glm::vec3(sin(currentFrame/(i+3)) * (i+8), 0.0f, cos(currentFrame*(i+2))));
                modelShader.setMat4("model", model);
                iModel.Draw(modelShader);

                // update the position attribute of the insect
                insects[i].position = glm::vec3(model[3]);   // extract the translation part from the final model matrix
            }
        }

        // update the insect closest to the bird
        closestInsectDistance = std::numeric_limits<float>::max();
        for (unsigned int i = 0; i < insects.size(); i++) {
            if(!(insects[i].eaten)) {
                unsigned distance = glm::distance(programState->modelPosition, insects[i].position);
                if (distance < closestInsectDistance) {
                    closestInsectDistance = distance;
                    closestInsectIdx = i;
                }
            }
        }


        // TEXTURES
        // transparent clouds
        blendingShader.use();
        blendingShader.setMat4("projection", projection);
        blendingShader.setMat4("view", view);
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        for (unsigned int i = 0; i < clouds.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(5.0f));
            model = glm::translate(model, clouds[i]);
            blendingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }


        // draw skybox
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default


        if (programState->ImGuiEnabled)
            DrawImGui(programState);


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {

    //restart the game
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        programState->camera.Position = glm::vec3(0.0f, -3.5f, 0.0f);
        programState->CameraMouseMovementUpdateEnabled = true;
        bird.eaten = false;
        for (unsigned int i = 0; i < insects.size(); i++) {
            insects[i].eaten = false;
        }
        remainingInsects = insects.size();
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

    if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(DOWN, deltaTime);
    if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(UP, deltaTime);

    // Update model position relative to camera
    programState->modelRelativePosition = programState->camera.Position + programState->modelOffset;
    programState->modelPosition = programState->modelRelativePosition + glm::vec3(0.0f, 3.5f, 0.0f);
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
        ImGui::ColorEdit3("Model color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Model position", (float*)&programState->modelPosition);
        ImGui::DragFloat("Model scale", &programState->modelScale, 0.05, 0.1, 4.0);

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

    {
        ImGui::Begin("Game UI");

        if (!bird.eaten) {
            if(remainingInsects > 0) {
                // Display proximity indicator
                if (closestInsectDistance < proximityThreshold) {
                    ImGui::Text("Bird is close to an insect!");
                } else {
                    ImGui::Text("Bird is not close to any insects.");
                }
                ImGui::Text("Number of remaining insects: %d", remainingInsects);
            }

            // Display message when all insects are eaten
            else if (remainingInsects == 0) {
                ImGui::Text("Congratulations! You have eaten all the insects!");
            }

            if (falconDistance < proximityThreshold) {
                ImGui::Text("Be careful! The falcon is flying nearby and it's hungry!");
            } else {
                ImGui::Text("There is no danger for the bird");
            }
        }

        // Display message when the bird is eaten
        if (bird.eaten) {
            ImGui::Text("Game over! Your bird was eaten by the falcon!");
            programState->CameraMouseMovementUpdateEnabled = false;
        }

        ImGui::End();
    }

    {
        ImGui::Begin("Game positions");
        ImGui::Text("Bird position: (%f, %f, %f)", (programState->modelPosition)[0], (programState->modelPosition)[1], (programState->modelPosition)[2]);
        ImGui::Text("Closest insect position: (%f, %f, %f), distance: %f", (insects[closestInsectIdx].position)[0], (insects[closestInsectIdx].position)[1], (insects[closestInsectIdx].position)[2], remainingInsects > 0 ? closestInsectDistance : 0);
        ImGui::Text("Falcon position: (%f, %f, %f), distance: %f", falconPosition[0], falconPosition[1], falconPosition[2], falconDistance);
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

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS){
        programState->CameraMouseMovementUpdateEnabled = !programState->CameraMouseMovementUpdateEnabled;
        std::cout << "Camera lock - " << (programState->CameraMouseMovementUpdateEnabled ? "Disabled" : "Enabled") << '\n';
    }
}

// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front)
// -Z (back)
// -------------------------------------------------------
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

// utility function for loading a 2D texture from file
// ---------------------------------------------------
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
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); //NEAREST?
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //NEAREST?

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}