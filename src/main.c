
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#define OPENGL_VERSION_MAJOR    4
#define OPENGL_VERSION_MINOR    6
#define OPENGL_USE_CORE_PROFILE true

#define WINDOW_TITLE    "Shadertoy"
#define WINDOW_WIDTH    800
#define WINDOW_HEIGHT   800

#define TEXTURE_FILE_PATH   "./textures/sky.png"
#define TEXTURE_SMOOTH      true

#define FRAGMENT_SHADER_PATH "./shaders/default.glsl"

// How many times to draw per frame
#define NUM_DRAW 1

static int window_width     = WINDOW_WIDTH;
static int window_height    = WINDOW_HEIGHT;

static int mouse_x = 0;
static int mouse_y = 0;

void glfw_error_callback(int error_code, const char* description) {
    fprintf(stderr, "GLFW_ERROR %d: %s\n", error_code, description);
}

void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
    (void) source;
    (void) id;
    (void) severity;
    (void) length;
    (void) userParam;

    const char* type_string = type == GL_DEBUG_TYPE_ERROR ? "ERROR" :
                              type == GL_DEBUG_TYPE_PERFORMANCE ? "PERFORMANCE" : "INFO";

    fprintf(stderr, "GL_DEBUG %s: %s\n", type_string, message);
}

char* slurp_file_into_malloced_cstr(const char* file_path) {
    FILE* f = fopen(file_path, "rb");
    if (f == NULL) {
        fprintf(stderr, "ERROR: Failed to open file %s: %s\n", file_path, strerror(errno));
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buffer = calloc(len+1, sizeof(char));
    fread(buffer, len, 1, f);
    fclose(f);

    return buffer;
}

void window_size_callback(GLFWwindow* window, int width, int height) {
    (void) window;

    window_width = width;
    window_height = height;

    glViewport(0, 0, width, height);
}

void mouse_move_callback(GLFWwindow* window, double x, double y) {
    (void) window;

    mouse_x = (int) x;
    mouse_y = (int) y;
}

unsigned int create_texture(unsigned char* pixels, int width, int height, int channels, bool smooth) {
    unsigned int tex;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (smooth) {
        glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else {
        glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    GLenum internal_format = channels == 3 ? GL_RGB8 : GL_RGBA8;

    glTextureStorage2D(tex, 1, internal_format, width, height);
    glTextureSubImage2D(tex, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateTextureMipmap(tex);

    return tex;
}

unsigned int create_texture_from_file(const char* file_path, bool smooth) {
    stbi_set_flip_vertically_on_load(true);

    int width, height, channels;
    unsigned char* pixels = stbi_load(file_path, &width, &height, &channels, 4);

    unsigned int tex = create_texture(pixels, width, height, channels, smooth);
    
    stbi_image_free(pixels);

    return tex;
}

int main(void) {
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        fprintf(stderr, "ERROR: Couldn't initialize GLFW!\n");
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_VERSION_MINOR);

    if (OPENGL_USE_CORE_PROFILE)
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    else
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, 0, 0);
    if (window == NULL) {
        fprintf(stderr, "ERROR: Failed to create window(%d, %d, %s)\n", WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
        exit(1);
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        fprintf(stderr, "ERROR: Failed to load OpenGL!\n");
        exit(1);
    }

    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetCursorPosCallback(window, mouse_move_callback);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(gl_debug_callback, 0);

    glEnable(GL_DEPTH_TEST);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int dummy_vao;
    glCreateVertexArrays(1, &dummy_vao);
    glBindVertexArray(dummy_vao);

    static const char* vsh_source =
        "#version 460\n"

        "layout(location = 0) out vec2 uv;"
        
        "out gl_PerVertex {"
        "   vec4 gl_Position;"
        "};"

        "void main() {"
        "   uv.x = (gl_VertexID & 1);"
        "   uv.y = ((gl_VertexID >> 1) & 1);"
        "   gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);"
        "}"
    ;

    char* fsh_source = slurp_file_into_malloced_cstr(FRAGMENT_SHADER_PATH);

    unsigned int vsh = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &vsh_source);
    unsigned int fsh = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &fsh_source);

    free(fsh_source);

    unsigned int program_pipeline;
    glCreateProgramPipelines(1, &program_pipeline);
    glUseProgramStages(program_pipeline, GL_VERTEX_SHADER_BIT, vsh);
    glUseProgramStages(program_pipeline, GL_FRAGMENT_SHADER_BIT, fsh);
    glBindProgramPipeline(program_pipeline);

    // Get uniform locations
    unsigned int u_resolution = glGetUniformLocation(fsh, "resolution");
    unsigned int u_time = glGetUniformLocation(fsh, "time");
    unsigned int u_mouse = glGetUniformLocation(fsh, "mouse");
    unsigned int u_tex = glGetUniformLocation(fsh, "tex");

    // Create texture
    unsigned int tex = create_texture_from_file(TEXTURE_FILE_PATH, TEXTURE_SMOOTH);

    while (!glfwWindowShouldClose(window)) {
        static const float clear_color[4] = {0.09f, 0.09f, 0.09f, 1.0f};
        glClearBufferfv(GL_COLOR, 0, clear_color);
        glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0f, 1);

        for (int i = 0; i < NUM_DRAW; i++) {
            // Set uniforms
            glProgramUniform2f(fsh, u_resolution, (float) window_width, (float) window_height);
            glProgramUniform1f(fsh, u_time, (float) glfwGetTime());
            glProgramUniform2f(fsh, u_mouse, (float) mouse_x, (float) mouse_y);
            glProgramUniform1i(fsh, u_tex, 0);

            // Bind texture
            glBindTextureUnit(0, tex);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteTextures(1, &tex);

    glDeleteProgramPipelines(1, &program_pipeline);
    glDeleteProgram(fsh);
    glDeleteProgram(vsh);

    glDeleteVertexArrays(1, &dummy_vao);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
