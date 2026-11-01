#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// Компилирует вершинный и фрагментный шейдеры и связывает их в программу.
class Shader {
    unsigned int ID = 0;

    // Удаляет шейдер автоматически, в том числе при исключении.
    struct ShaderGuard {
        unsigned int id;
        explicit ShaderGuard(GLenum type) : id(glCreateShader(type)) {}
        ~ShaderGuard() { glDeleteShader(id); }
    };

public:
    Shader(const char* vertexPath, const char* fragmentPath) {
        std::unique_ptr<char[]> VertexShaderCode   = ReadFromFile(vertexPath);
        std::unique_ptr<char[]> FragmentShaderCode = ReadFromFile(fragmentPath);

        // Компилируем шейдеры.
        ShaderGuard VShader(GL_VERTEX_SHADER);
        ShaderGuard FShader(GL_FRAGMENT_SHADER);
        CompileShader(VShader.id, VertexShaderCode.get());
        CompileShader(FShader.id, FragmentShaderCode.get());

        // Создаём и связываем программу.
        ID = glCreateProgram();
        try {
            AttachAndLink(VShader.id, FShader.id);
        } catch (...) {
            glDeleteProgram(ID);
            ID = 0;
            throw;
        }
    }

    ~Shader() {
        glDeleteProgram(ID);
    }

    // Копирование и перемещение запрещены: объект владеет программой OpenGL.
    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other)           = delete;

    // Активирует шейдерную программу.
    void use() const noexcept {
        glUseProgram(ID);
    }

    unsigned int getID() const noexcept {
        return ID;
    }

    // Утилиты для установки uniform-переменных.
    void setBool(const std::string& name, const bool value) const noexcept {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    void setMat4fv(const std::string& name, const glm::mat4& value) const noexcept {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
    }

    void setInt(const std::string& name, const int value) const noexcept {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setVec3(const std::string& name, const glm::vec3& value) const noexcept {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
    }

    void setFloat(const std::string& name, const float value) const noexcept {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

private:
    void AttachAndLink(unsigned int VShader, unsigned int FShader) const {
        glAttachShader(ID, VShader);
        glAttachShader(ID, FShader);
        glLinkProgram(ID);
        glDetachShader(ID, VShader);
        glDetachShader(ID, FShader);
        checkCompileErrors(ID, true);
    }

    static void CompileShader(unsigned int Shader, const char* const ShaderSource) {
        glShaderSource(Shader, 1, &ShaderSource, NULL);
        glCompileShader(Shader);
        checkCompileErrors(Shader, false);
    }

    static std::unique_ptr<char[]> ReadFromFile(const std::string& FileName) {
        std::ifstream file(FileName, std::ios::binary);
        if (!file)
            throw std::runtime_error("ERROR: couldnt open file <" + FileName + ">\n");

        std::size_t FileSize = std::filesystem::file_size(FileName);
        std::unique_ptr<char[]> buffer = std::make_unique<char[]>(FileSize + 1);

        if (!file.read(buffer.get(), FileSize))
            throw std::runtime_error("ERROR: couldnt read file <" + FileName + ">\n");

        buffer[FileSize] = '\0';
        return buffer;
    }

    // Проверяет ошибки компиляции шейдера или линковки программы.
    static void checkCompileErrors(unsigned int object, bool IsProgram) {
        int success = 0;
        char infoLog[1024];
        if (!IsProgram) {
            glGetShaderiv(object, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(object, 1024, NULL, infoLog);
                throw std::runtime_error("ERROR: shader compilation error\n" + std::string(infoLog) + "\n -- --------------------------------------------------- -- \n");
            }
        } else {
            glGetProgramiv(object, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(object, 1024, NULL, infoLog);
                throw std::runtime_error("ERROR: program linking error\n" + std::string(infoLog) + "\n -- --------------------------------------------------- -- \n");
            }
        }
    }
};
