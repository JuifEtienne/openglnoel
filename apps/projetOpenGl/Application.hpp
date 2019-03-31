#pragma once


#include <glmlv/filesystem.hpp>
#include <glmlv/GLFWHandle.hpp>
#include <glmlv/GLProgram.hpp>
#include <glmlv/ViewController.hpp>
#include <glmlv/simple_geometry.hpp>
#include <glmlv/Image2DRGBA.hpp>
#include <map>
#include <tiny_gltf.h>


class Application
{
public:
    Application(int argc, char** argv);

    int run();

	void getMeshs(tinygltf::Node node, std::vector<int> &meshIdx);
	void PrintParameterMap(const tinygltf::ParameterMap &pmap);

private:
	/*APP GENERAL VARIABLES*/
    const size_t m_nWindowWidth = 1280;
    const size_t m_nWindowHeight = 720;
    glmlv::GLFWHandle m_GLFWHandle{ m_nWindowWidth, m_nWindowHeight, "Template" }; // Note: the handle must be declared before the creation of any object managing OpenGL resource (e.g. GLProgram, GLShader)

    const glmlv::fs::path m_AppPath;
    const std::string m_AppName;
    const std::string m_ImGuiIniFilename;
    const glmlv::fs::path m_ShadersRootPath;
    const glmlv::fs::path m_AssetsRootPath;

	glmlv::ViewController *view;

	/*GLTF - GEOMETRY*/
	std::map<std::string, int> attribIndexOf;
	std::map<int, int> numberOfComponentOf;
	std::map<int, GLenum> attribEnum;

	tinygltf::TinyGLTF loader;

	std::vector<GLuint> vaos;
	std::vector<tinygltf::Primitive> primitives;
	std::vector<glm::dmat4> matrix;

	glm::vec3 boundingBMin, boundingBMax;

	/*GLTF - TEXTURES*/
	std::vector<glmlv::Image2DRGBA> images;
	std::vector<GLuint> textures;
	std::vector<GLuint> samplers;

	/*GLTF - ERROR HANDLING*/
	tinygltf::Model model;
	std::string err;
	std::string warn;

	/*DEFERRED - GEOMETRY PASS*/
	glmlv::GLProgram program;

	GLint modelViewProjMatrix;
	GLint modelViewMatrix;
	GLint normalMatrix;

	GLuint baseColorLocation;
	GLuint emissionColorLocation;

	GLuint baseCoord;
	GLuint emissionCoord;

	GLuint baseColorFactor;
	GLuint emissionColorFactor;

	/*DEFERRED - SHADING PASS*/
	glmlv::GLProgram programShading;

	enum GBufferTextureType
	{
		GPosition = 0,
		GNormal,
		GAmbient,
		GDiffuse,
		GDepth, // On doit créer une texture de depth mais on écrit pas directement dedans dans le FS. OpenGL le fait pour nous (et l'utilise).
		GBufferTextureCount
	};
	GLuint m_GBufferTextures[GBufferTextureCount];

	const GLenum m_GBufferTextureFormat[GBufferTextureCount] = { GL_RGB32F, GL_RGB32F, GL_RGB32F, GL_RGB32F, GL_DEPTH_COMPONENT32F };

	GLuint m_FBO;

	GLint directionalLightDir;
	GLint directionalLightIntensity;

	GLuint positionLocation, normalLocation, ambientLocation, diffuseLocation;

	/*QUAD - GEOMETRY*/
	GLuint vboQuad, vaoQuad, iboQuad;

	/*BEAUTY TEXTURE*/
	GLuint m_BeautyTexture;
	GLuint m_BeautyFBO;

	/*GAMMA CORRECTION - PROGRAM*/
	glmlv::GLProgram m_gammaCorrectionProgram;

	GLint m_uGammaExponent;

	GLuint m_GammaCorrectTexture;
	GLuint m_GammaCorrectFBO;

	/*APP RUN VARIABLES*/
	glm::mat4 ProjMatrix;
	glm::mat4 MVMatrixCube;
	glm::mat4 NormalMatrixCube;

	float anglePhi;
	float angleTheta;
	float intensityDir;
	glm::vec3 colorDir;

	int textureToPrint;
	int printTexture;

	int baseColorTextureID = -1;
	int baseColorTexCoord = -1;
	int emisColorTextureID = -1;
	int emisColorTexCoord = -1;

};