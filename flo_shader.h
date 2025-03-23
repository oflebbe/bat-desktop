#ifndef FLO_SHADER_H
#include <GL/glew.h>

GLuint create_shader_program(const char *vertexSource, const char *fragmentSource);
GLuint create_texture(int width, int height, GLenum internalFormat, GLenum format, GLenum type);
void render_texture(GLuint hslTexture, GLuint rgbTexture, int width, int height, GLuint shaderProgram);

#ifdef FLO_SHADER_IMPLEMENTATION

#include <stdio.h>

// Function to compile and link shaders
GLuint create_shader_program(const char *vertexSource, const char *fragmentSource)
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// Function to create a texture
GLuint create_texture(int width, int height, GLenum internalFormat, GLenum format, GLenum type)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

void render_texture(GLuint hslTexture, GLuint rgbTexture, int width, int height, GLuint shaderProgram)
{
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rgbTexture, 0);

    glUseProgram(shaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hslTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "hslTexture"), 0);

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    // Render a full-screen quad
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
}

#endif
#endif
