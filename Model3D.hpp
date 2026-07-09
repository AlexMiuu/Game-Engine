#ifndef Model3D_hpp
#define Model3D_hpp

#include "Mesh.hpp"

#include "tiny_obj_loader.h"
#include "stb_image.h"

#include <iostream>
#include <string>
#include <vector>

namespace gps {

    class Model3D {

    public:
        ~Model3D();

		void LoadModel(std::string fileName);

		void LoadModel(std::string fileName, std::string basePath);

		void Draw(gps::Shader shaderProgram);

		const std::vector<gps::Mesh>& getMeshes() const {
			return this->meshes;
		}

    private:
        std::vector<gps::Mesh> meshes;
        std::vector<gps::Texture> loadedTextures;

		void ReadOBJ(std::string fileName, std::string basePath);
		gps::Texture LoadTexture(std::string path, std::string type);

		GLuint ReadTextureFromFile(const char* file_name);
    };
}

#endif
