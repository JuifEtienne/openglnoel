#include "Application.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

int Application::run()
{
	// Put here code to run before rendering loop
    //glClearColor(1,0,0,1);
	float gamma = 2.2f;

    
    // Loop until the user closes the window
    for (auto iterationCount = 0u; !m_GLFWHandle.shouldClose(); ++iterationCount)
    {
        const auto seconds = glfwGetTime();

        for(int i = 0; i < 5; ++i){
            glBindSampler(i, sampler);
        }

        // Put here rendering code
        programGeometry.use();

        //Change unit texture
        glUniform1i(KaLocation, 0); // Set the uniform to 0 because we use texture unit 0
        glUniform1i(KdLocation, 1); // Set the uniform to 1 because we use texture unit 1
        glUniform1i(KsLocation, 2); // Set the uniform to 2 because we use texture unit 2

        //Matrix
        MVMatrix = glm::translate(view.getViewMatrix(), glm::vec3(0.0f, 0.0f, -5.0f));
        NormalMatrix = glm::transpose(glm::inverse(MVMatrix));
        //Envoie des matrices
        glUniformMatrix4fv( modelViewProjMatrix, 1, GL_FALSE, glm::value_ptr( ProjMatrix * MVMatrix ));
        glUniformMatrix4fv( modelViewMatrix, 1, GL_FALSE, glm::value_ptr( MVMatrix ));
        glUniformMatrix4fv( normalMatrix, 1, GL_FALSE, glm::value_ptr( NormalMatrix ));


        glBindVertexArray(vao);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);

        glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto indexOffset = 0;
        int indexShape = 0;


        for (const auto indexCount: data.indexCountPerShape)
        {
        
            auto materialID = data.materialIDPerShape[indexShape];
            if(materialID != -1){
                glUniform1f( shininess, data.materials[materialID].shininess);
                glActiveTexture(GL_TEXTURE0);
                if(data.materials[materialID].KaTextureId != -1){
                    glUniform3fv(uKa, 1, glm::value_ptr(data.materials[materialID].Ka));
                    glBindTexture(GL_TEXTURE_2D, textures[data.materials[materialID].KaTextureId]);
                }
                else{
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                glActiveTexture(GL_TEXTURE1);
                if(data.materials[materialID].KdTextureId != -1){
                    glUniform3fv(uKd, 1, glm::value_ptr(data.materials[materialID].Kd));
                    glBindTexture(GL_TEXTURE_2D, textures[data.materials[materialID].KdTextureId]);
                }
                else{
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                glActiveTexture(GL_TEXTURE2);
                if(data.materials[materialID].KsTextureId != -1){
                    glUniform3fv(uKs, 1, glm::value_ptr(data.materials[materialID].Ks));
                    glBindTexture(GL_TEXTURE_2D, textures[data.materials[materialID].KsTextureId]);
                }
                else{
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }

            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (const GLvoid*) (indexOffset * sizeof(GLuint)));
            indexOffset += indexCount;
            ++indexShape;

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0); // débind sur l'unité GL_TEXTURE0
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindVertexArray(0);


        //Lecture
        
		programShading.use();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_BeautyFBO);

		glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
		//Lightning - General
		glUniform3fv( directionalLightDir, 1, glm::value_ptr( view.getViewMatrix() * glm::vec4(sin(anglePhi)*cos(angleTheta), sin(anglePhi)*sin(angleTheta), cos(anglePhi), 0)));
		glUniform3fv( directionalLightIntensity, 1, glm::value_ptr( colorDir*intensityDir ));
            

		for(int i = GPosition; i < GDepth; ++i){
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[i]);
		}

		glUniform1i(positionLocation, 0); // Set the uniform to 0 because we use texture unit 0
		glUniform1i(normalLocation, 1); // Set the uniform to 1 because we use texture unit 1
		glUniform1i(ambientLocation, 2);
		glUniform1i(diffuseLocation, 3);
		glUniform1i(glossyLocation, 4);

		glBindVertexArray(vaoQuad);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		/*for(int i = 0; i < 5; ++i){ //
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, 0);
		}*/

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		

		//GAMMA CORRECTION
		m_gammaCorrectionProgram.use();
			
		glUniform1f(m_uGammaExponent, 1.f / gamma);
		glBindImageTexture(0, m_BeautyTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(1, m_GammaCorrectedBeautyTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		
		glDispatchCompute(m_nWindowWidth, m_nWindowHeight, 1);

		const auto viewportSize = m_GLFWHandle.framebufferSize();
		glViewport(0, 0, viewportSize.x, viewportSize.y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		
		if (printTexture == 1) {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
			glReadBuffer(GL_COLOR_ATTACHMENT0 + textureToPrint);
			glBlitFramebuffer(0, 0, m_nWindowWidth, m_nWindowHeight, 0, 0, m_nWindowWidth, m_nWindowHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}
		else if (printTexture == 2) {
			/*TEST POUR POST-PROCESSING*/
		//_____________________________________________________________________________________________
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_BeautyFBO);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glBlitFramebuffer(0, 0, m_nWindowWidth, m_nWindowHeight, 0, 0, m_nWindowWidth, m_nWindowHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}
		else {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_GammaCorrectedBeautyFBO);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glBlitFramebuffer(0, 0, m_nWindowWidth, m_nWindowHeight,
				0, 0, m_nWindowWidth, m_nWindowHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}


        // GUI code:
		glmlv::imguiNewFrame();

        {
            ImGui::Begin("GUI");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            if (ImGui::CollapsingHeader("Directional Light"))
            {
                ImGui::DragFloat("Dir angle Phi", &anglePhi);
                ImGui::DragFloat("Dir angle Theta", &angleTheta);
                ImGui::DragFloat("DirLightIntensity", &intensityDir);
                ImGui::ColorEdit3("DirLightColor", glm::value_ptr(colorDir));
            }
            ImGui::RadioButton("Result", &printTexture, 0);  ImGui::SameLine();
            ImGui::RadioButton("One texture", &printTexture, 1);
			ImGui::RadioButton("Beauty", &printTexture, 2);
            if (ImGui::CollapsingHeader("FrameBuffer image"))
            {
                ImGui::RadioButton("GPosition", &textureToPrint, 0);  ImGui::SameLine();
                ImGui::RadioButton("GNormal", &textureToPrint, 1); ImGui::SameLine();
                ImGui::RadioButton("GAmbient", &textureToPrint, 2); ImGui::SameLine();
                ImGui::RadioButton("GDiffuse", &textureToPrint, 3); ImGui::SameLine();
                ImGui::RadioButton("GGlossyShininess", &textureToPrint, 4);
            }
			ImGui::InputFloat("Gamma Correction", &gamma);
            ImGui::End();
        }

		glmlv::imguiRenderFrame();

        glfwPollEvents(); // Poll for and process events

        auto ellapsedTime = glfwGetTime() - seconds;
        auto guiHasFocus = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
        if (!guiHasFocus) {
            // Put here code to handle user interactions
            view.update(ellapsedTime);
        }

		m_GLFWHandle.swapBuffers(); // Swap front and back buffers
    }

    return 0;
}

Application::Application(int argc, char** argv):
    m_AppPath { glmlv::fs::path{ argv[0] } },
    m_AppName { m_AppPath.stem().string() },
    m_ImGuiIniFilename { m_AppName + ".imgui.ini" },
    m_ShadersRootPath { m_AppPath.parent_path() / "shaders" },
    m_AssetsRootPath { m_AppPath.parent_path() / "assets" },
    view { glmlv::ViewController(m_GLFWHandle.window(), 10) },
    anglePhi(10),
    angleTheta(10),
    intensityDir(10),
    colorDir {50,50,50},
    textureToPrint {2},
    printTexture {0}
{
    ImGui::GetIO().IniFilename = m_ImGuiIniFilename.c_str(); // At exit, ImGUI will store its windows positions in this file

    // Put here initialization code
    //_____________________________________________________PROGRAM   FRAGMENT SHADER___________________________________________________________________________________________
    const auto pathToSMVS = m_ShadersRootPath / m_AppName / "geometryPass.vs.glsl";
    const auto pathToSMFS = m_ShadersRootPath / m_AppName / "geometryPass.fs.glsl";

    programGeometry = glmlv::compileProgram({pathToSMVS, pathToSMFS});
    programGeometry.use();


    glmlv::loadObjScene(m_AssetsRootPath / m_AppName / "sponza/sponza.obj", data);

    const auto sceneDiagonalSize = glm::length(data.bboxMax - data.bboxMin);
    view.setSpeed(sceneDiagonalSize * 0.2f); // 10% de la scene parcouru par seconde

    //sceneSize = glm::length(data.bboxMax - data.bboxMin);
    ProjMatrix = glm::perspective(70.f, float(m_nWindowWidth) / m_nWindowHeight, 0.01f * sceneDiagonalSize, sceneDiagonalSize); // near = 1% de la taille de la scene, far = 100%


    glEnable(GL_DEPTH_TEST);
	//glEnable(GL_FRAMEBUFFER_SRGB);

    const GLuint VERTEX_ATTR_POSITION = 0;
    const GLuint VERTEX_ATTR_NORMAL = 1;
    const GLuint VERTEX_ATTR_TEXCOORDS = 2;

    //Construction cube
    
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);
    glGenVertexArrays(1, &vao);

    //VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferStorage(GL_ARRAY_BUFFER, sizeof(glmlv::Vertex3f3f2f)*data.vertexBuffer.size(), data.vertexBuffer.data(), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //IBO
    glBindBuffer(GL_ARRAY_BUFFER, ibo);
    glBufferStorage(GL_ARRAY_BUFFER, data.indexBuffer.size() * sizeof(uint32_t), data.indexBuffer.data(), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //VAO
    glBindVertexArray(vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    glEnableVertexAttribArray(VERTEX_ATTR_POSITION);
    glEnableVertexAttribArray(VERTEX_ATTR_NORMAL);
    glEnableVertexAttribArray(VERTEX_ATTR_TEXCOORDS);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(VERTEX_ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(glmlv::Vertex3f3f2f), (const GLvoid*) offsetof(glmlv::Vertex3f3f2f, position));
    glVertexAttribPointer(VERTEX_ATTR_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(glmlv::Vertex3f3f2f), (const GLvoid*) offsetof(glmlv::Vertex3f3f2f, normal));
    glVertexAttribPointer(VERTEX_ATTR_TEXCOORDS, 2, GL_FLOAT, GL_FALSE, sizeof(glmlv::Vertex3f3f2f), (const GLvoid*) offsetof(glmlv::Vertex3f3f2f, texCoords));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);


    //getUniform
    //MATRIX
    modelViewProjMatrix = programGeometry.getUniformLocation("uModelViewProjMatrix");
    modelViewMatrix = programGeometry.getUniformLocation("uModelViewMatrix");
    normalMatrix = programGeometry.getUniformLocation("uNormalMatrix");

    //Texture
    //textures = new GLuint[data.textures.size()];
    uKd = programGeometry.getUniformLocation("uKd");
    uKa = programGeometry.getUniformLocation("uKa");
    uKs = programGeometry.getUniformLocation("uKs");

    glActiveTexture(GL_TEXTURE0);
    //glGenTextures(data.textures.size(), textures);

    for(int i = 0; i < data.textures.size(); ++i){
        GLuint textureTemp;
        glGenTextures(1, &textureTemp);
        glBindTexture(GL_TEXTURE_2D, textureTemp);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, data.textures[i].width(), data.textures[i].height());
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, data.textures[i].width(), data.textures[i].height(), GL_RGBA, GL_UNSIGNED_BYTE, data.textures[i].data());
        glBindTexture(GL_TEXTURE_2D, 0);
        textures.push_back(textureTemp);
    }

    //SAMPLERS
    glGenSamplers(1, &sampler);
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    //samplerLocation
    KaLocation = programGeometry.getUniformLocation("uKaSampler");
    KdLocation = programGeometry.getUniformLocation("uKdSampler");
    KsLocation = programGeometry.getUniformLocation("uKsSampler");

    
    shininess = programGeometry.getUniformLocation("uShininess");


    //Deferred
    glGenTextures(GBufferTextureCount, m_GBufferTextures);

    for(int i = 0; i < GBufferTextureCount; ++i){
        glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, m_GBufferTextureFormat[i], m_nWindowWidth, m_nWindowHeight);
        //glBindTexture(GL_TEXTURE_2D, 0);
    }

    //FBO
    glGenFramebuffers(1, &m_FBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);

    for(int i = 0; i < GBufferTextureCount - 1; ++i){
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_GBufferTextures[i], 0);
    }
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_GBufferTextures[5], 0);

    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
    glDrawBuffers(5, drawBuffers);

    GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "FB error, status: " << status << std::endl;
        throw std::runtime_error("FBO error");
    }
    
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);




    //_____________________________________________________PROGRAM   SHADING SHADER___________________________________________________________________________________________
    const auto pathToSPVS = m_ShadersRootPath / m_AppName / "shadingPass.vs.glsl";
    const auto pathToSPFS = m_ShadersRootPath / m_AppName / "shadingPass.fs.glsl";

    programShading = glmlv::compileProgram({pathToSPVS, pathToSPFS});
    programShading.use();

    //LIGHT
    directionalLightDir = programShading.getUniformLocation("uDirectionalLightDir_vs");
    directionalLightIntensity = programShading.getUniformLocation("uDirectionalLightIntensity");

    positionLocation = programShading.getUniformLocation("uGPosition");
    normalLocation = programShading.getUniformLocation("uGNormal");
    ambientLocation = programShading.getUniformLocation("uGAmbient");
    diffuseLocation = programShading.getUniformLocation("uGDiffuse");
    glossyLocation = programShading.getUniformLocation("uGlossyShininess");

    //Construction cube
    
    glGenBuffers(1, &vboQuad);
    glGenBuffers(1, &iboQuad);
    glGenVertexArrays(1, &vaoQuad);

    glm::vec2 vertexBufferQuad[] = {
        glm::vec2(-1, -1),
        glm::vec2(1, -1),
        glm::vec2(1, 1),
        glm::vec2(-1, 1)
    };

    uint32_t indexBufferQuad[] = {
        0, 1, 2,
        2, 3, 0
    };

    //VBO
    glBindBuffer(GL_ARRAY_BUFFER, vboQuad);
    glBufferStorage(GL_ARRAY_BUFFER, sizeof(vertexBufferQuad), vertexBufferQuad, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //IBO
    glBindBuffer(GL_ARRAY_BUFFER, iboQuad);
    glBufferStorage(GL_ARRAY_BUFFER, sizeof(indexBufferQuad), indexBufferQuad, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //VAO
    glBindVertexArray(vaoQuad);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboQuad);

    glEnableVertexAttribArray(VERTEX_ATTR_POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, vboQuad);
    glVertexAttribPointer(VERTEX_ATTR_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

	//_____________________________________________________TEXTURE & FBO fOR POST-PROCESSING___________________________________________________________________________________________

	glGenTextures(1, &m_BeautyTexture);

	glBindTexture(GL_TEXTURE_2D, m_BeautyTexture);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, m_nWindowWidth, m_nWindowHeight);
	//glBindTexture(GL_TEXTURE_2D, 0);

	//FBO
	glGenFramebuffers(1, &m_BeautyFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_BeautyFBO);

	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_BeautyTexture, 0);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	GLenum beautyStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (beautyStatus != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "FB error, status: " << beautyStatus << std::endl;
		throw std::runtime_error("FBO error");
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	//_____________________________________________________PROGRAM  GAMMA CORRECTION___________________________________________________________________________________________

	glGenTextures(1, &m_GammaCorrectedBeautyTexture);

	glBindTexture(GL_TEXTURE_2D, m_GammaCorrectedBeautyTexture);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, m_nWindowWidth, m_nWindowHeight);

	glGenFramebuffers(1, &m_GammaCorrectedBeautyFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_GammaCorrectedBeautyFBO);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_GammaCorrectedBeautyTexture, 0);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	GLenum gammaStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (gammaStatus != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "FB error, status: " << gammaStatus << std::endl;
		throw std::runtime_error("FBO error");
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	const auto pathToGCCS = m_ShadersRootPath / m_AppName / "gammaCorrect.cs.glsl";

	m_gammaCorrectionProgram = glmlv::compileProgram({ pathToGCCS });
	m_gammaCorrectionProgram.use();

	m_uGammaExponent = m_gammaCorrectionProgram.getUniformLocation("uGammaExponent");

}