#include <SDL2/SDL.h>
#include <GLES3/gl3.h>
#include <math.h>
#include "cglm/cglm.h"
#include <string.h>

#define M_PI 3.14159265358979323846
#define N_POINTS 1000
#define RESOLUTION_WIDTH 640
#define RESOLUTION_HEIGHT 480
#define N_TRIANGLES 10

void shaderStatus(int obj) {
  int length;
  glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &length);
  if (length) {
      char* error = malloc(sizeof(char)*length);
      glGetShaderInfoLog(obj, length, &length, error);
      printf("%s\n", error);
      free(error);
  }
}

typedef struct {
  float x;
  float y;
} Point;

int main() {
  int running = 1;
  int drawing = 0;
  int currentIdx = 0;
  Point pointCloud[N_POINTS];

  if (SDL_Init(SDL_INIT_EVERYTHING) == -1) return 1;

  // SDL Window
  SDL_Window *window =
    SDL_CreateWindow("float", 0, 0, RESOLUTION_WIDTH, RESOLUTION_HEIGHT, 
                     SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
  SDL_GLContext glcontext = SDL_GL_CreateContext(window);

  // Unit Circle
  int nVertices = N_TRIANGLES + 2;
  float vertices[nVertices*2];
  vertices[0] = 0.0f; // x
  vertices[1] = 0.0f; // y
  int circleIdx = 1;
  for (int i = 2; i < nVertices*2; i+=2) {
    vertices[i] = (cos(circleIdx * 2.0 * M_PI / N_TRIANGLES)); // x
    vertices[i+1] = (sin(circleIdx * 2.0 * M_PI / N_TRIANGLES)); // y
    circleIdx++;
  }

  // Shaders
  mat4 translations[N_POINTS]; // For instancing

  const char* vertexFormat =
    "#version 300 es\n"
    "in vec2 position;\n"
    "uniform  mat4 trans[%d];\n"
    "void main() {\n"
    "mat4 t = trans[gl_InstanceID];\n"
    "gl_Position = t * vec4(position, 0.0, 1.0);\n"
    "}\0";
  char* vertexSource = malloc(sizeof(char) * strlen(vertexFormat) + 10);
  sprintf(vertexSource, vertexFormat, N_POINTS);

  const char* fragmentSource =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform vec3 color;\n"
    "out vec4 outColor;\n"
    "void main() {\n"
      "outColor = vec4(color, 1.0);\n"
    "}\0";


  // Buffers
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Compile Shaders
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, (const GLchar * const*)&vertexSource, NULL);
  glCompileShader(vertexShader);

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
  glCompileShader(fragmentShader);

  // Link Shaders
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  glUseProgram(shaderProgram);

  // Check Compile
  shaderStatus(vertexShader);
  shaderStatus(fragmentShader);

  // Uniforms
  GLint uniColor = glGetUniformLocation(shaderProgram, "color");
  GLint uniTrans = glGetUniformLocation(shaderProgram, "trans");

  GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
  glEnableVertexAttribArray(posAttrib);
  glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

  while (running) {
    SDL_Event sdlEvent;
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.0, 0.0, 0.0, 1);
    while (SDL_PollEvent(&sdlEvent) > 0) {
      switch (sdlEvent.type) {
      case SDL_QUIT:
        running = 0;
        break;
      case SDL_KEYDOWN:
        switch (sdlEvent.key.keysym.sym) {
        case SDLK_ESCAPE:
          running = 0;
          break;
        case SDLK_r:
          currentIdx = 0;
          break;
        }
        break;
      case SDL_MOUSEBUTTONDOWN:
        if (sdlEvent.button.which == 0)
          drawing = 1;
        break;
      case SDL_MOUSEBUTTONUP:
        drawing = 0;
        break;
      case SDL_MOUSEMOTION:
        if (drawing) {
          if (currentIdx < N_POINTS) {
            pointCloud[currentIdx].x = ((float)sdlEvent.button.x/(float)RESOLUTION_WIDTH-0.5) * 2.0;
            pointCloud[currentIdx].y = (-(float)sdlEvent.button.y/(float)RESOLUTION_HEIGHT+0.5) * 2.0;
            currentIdx++;
          } else {
            printf("Number of Points Limit: %d\n", N_POINTS);
          }
        }
        break;
      }
    }
    if (currentIdx) {
      // Fill translations
      for (size_t i = 0; i < currentIdx; i++) {
        glm_mat4_identity(translations[i]);
        glm_translate(translations[i], (vec3){
            pointCloud[i].x,
              pointCloud[i].y,
              0.0f});
        glm_scale(translations[i], (vec3){0.01f, 0.01f, 1.0f});
        glm_rotate(translations[i], 0, (vec3){0.0f, 0.0f, 1.0f});
        glUniform3f(uniColor, 0.0f, 1.0f, 1.0f); // Color
      }
      glUniformMatrix4fv(uniTrans, currentIdx, GL_FALSE, translations[0][0]);
      glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, nVertices, currentIdx);
    }
    SDL_GL_SwapWindow(window);
  }
  SDL_GL_DeleteContext(glcontext);  
  SDL_Quit();
  free(vertexSource);
  glDeleteProgram(shaderProgram);
  glDeleteShader(fragmentShader);
  glDeleteShader(vertexShader);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  return 0;
}
