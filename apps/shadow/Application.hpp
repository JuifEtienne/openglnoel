#pragma once

#include <glmlv/filesystem.hpp>
#include <glmlv/GLFWHandle.hpp>
#include <glmlv/GLProgram.hpp>
#include <glmlv/ViewController.hpp>
#include <glmlv/simple_geometry.hpp>
#include <glmlv/scene_loading.hpp>



class Application
{

public:
    Application(int argc, char** argv);

    int run();
private:
    const size_t m_nWindowWidth = 1280;
    const size_t m_nWindowHeight = 720;
    glmlv::GLFWHandle m_GLFWHandle{ m_nWindowWidth, m_nWindowHeight, "Template" }; // Note: the handle must be declared before the creation of any object managing OpenGL resource (e.g. GLProgram, GLShader)

    const glmlv::fs::path m_AppPath;
    const std::string m_AppName;
    const std::string m_ImGuiIniFilename;
    const glmlv::fs::path m_ShadersRootPath;
    const glmlv::fs::path m_AssetsRootPath;

    glmlv::SceneData data;
    GLuint vbo, vao, ibo;

    glmlv::ViewController view;

	glm::vec3 m_BBoxMin, m_BBoxMax;
	float m_SceneSizeLenght;

    //float sceneSize;

    glmlv::GLProgram programGeometry, programShading;

    GLint modelViewProjMatrix;
    GLint modelViewMatrix;
    GLint normalMatrix;

    float anglePhi;
    float angleTheta;
    float intensityDir;
    glm::vec3 colorDir;

    GLint shininess;
    GLint uKd;
    GLint uKa;
    GLint uKs;

    //matrix
    glm::mat4 ProjMatrix;
    glm::mat4 MVMatrix;
    glm::mat4 NormalMatrix;
    glm::mat4 ViewMatrix;

    //Texture
    std::vector<GLuint> textures;
    GLuint sampler;
    GLuint KaLocation, KdLocation, KsLocation;

    //Deferred

    enum GBufferTextureType
    {
        GPosition = 0,
        GNormal,
        GAmbient,
        GDiffuse,
        GGlossyShininess,
        GDepth, // On doit créer une texture de depth mais on écrit pas directement dedans dans le FS. OpenGL le fait pour nous (et l'utilise).
        GBufferTextureCount
    };

    GLuint m_GBufferTextures[GBufferTextureCount];

    const GLenum m_GBufferTextureFormat[GBufferTextureCount] = { GL_RGB32F, GL_RGB32F, GL_RGB32F, GL_RGB32F, GL_RGBA32F, GL_DEPTH_COMPONENT32F };

    GLuint m_FBO;

    int textureToPrint;
    int printTexture;

    //Shading uniform
    GLint directionalLightDir;
    GLint directionalLightIntensity;

    GLuint positionLocation, normalLocation, ambientLocation, diffuseLocation, glossyLocation;

    GLuint vboQuad, vaoQuad, iboQuad;

	//SHADING MAP
	GLuint m_directionalSMTexture;
	GLuint m_directionalSMFBO;
	GLuint m_directionalSMSampler;
	int32_t m_nDirectionalSMResolution = 512;

	//SHADING MAP PROGRAM
	glmlv::GLProgram m_directionalSMProgram;
	GLint m_uDirLightViewProjMatrix;

	GLint m_uDirLightViewProjMatrix_shadingPass;
	GLint m_uDirLightShadowMap;
	GLint m_uDirLightShadowMapBias;

	//Shading Map - Depth Map
	glmlv::GLProgram m_displayDepthProgram;
	GLint m_uGDepthSamplerLocation;

};