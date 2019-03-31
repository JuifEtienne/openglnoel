#include "Application.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

int Application::run()
{
	// Put here code to run before rendering loop
    glClearColor(1,0,0,1);
	bool directionalSMDirty = true;
	float shadowMapBias = 0.01f;
    
    // Loop until the user closes the window
    for (auto iterationCount = 0u; !m_GLFWHandle.shouldClose(); ++iterationCount)
    {
		const auto seconds = glfwGetTime();

		/*PROJ & VIEW MATRIX FOR DIRECTIONAL SHADOW MAPPING*/
		static const auto computeDirectionVectorUp = [](float phiRadians, float thetaRadians)
		{
			const auto cosPhi = glm::cos(phiRadians);
			const auto sinPhi = glm::sin(phiRadians);
			const auto cosTheta = glm::cos(thetaRadians);
			return -glm::normalize(glm::vec3(sinPhi * cosTheta, -glm::sin(thetaRadians), cosPhi * cosTheta));
		};

		const auto sceneCenter = 0.5f * (m_BBoxMin + m_BBoxMax);
		const float sceneRadius = m_SceneSizeLenght * 0.5f;

		const auto dirLightUpVector = computeDirectionVectorUp(glm::radians(anglePhi), glm::radians(angleTheta));
		const auto dirLightViewMatrix = glm::lookAt(sceneCenter + directionalLightDir * sceneRadius, sceneCenter, dirLightUpVector);
		const auto dirLightProjMatrix = glm::ortho(-sceneRadius, sceneRadius, -sceneRadius, sceneRadius, 0.01f * sceneRadius, 2.f * sceneRadius);


		/*Calcul de la shadow map*/
		if (directionalSMDirty) {
			m_directionalSMProgram.use();

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_directionalSMFBO);
			glViewport(0, 0, m_nDirectionalSMResolution, m_nDirectionalSMResolution);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glUniformMatrix4fv(m_uDirLightViewProjMatrix, 1, GL_FALSE, glm::value_ptr(dirLightProjMatrix * dirLightViewMatrix));

			glBindVertexArray(vao);

			auto indexOffset = 0;
			int indexShape = 0;

			for (const auto indexCount : data.indexCountPerShape)
			{
				glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (const GLvoid*)(indexOffset * sizeof(GLuint)));
				indexOffset += indexCount;
				++indexShape;
			}

			glBindVertexArray(0);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			directionalSMDirty = false;
		}

		/*Rendu - Geometry Pass*/

        for(int i = 0; i < 5; ++i){
            glBindSampler(i, sampler);
        }

        // Put here rendering code
        programGeometry.use();
		glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

			for (int i = 0; i < 5; ++i) {
				glBindSampler(0, sampler);
			}

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0); // débind sur l'unité GL_TEXTURE0
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindVertexArray(0);


        const auto fbSize = m_GLFWHandle.framebufferSize();
        glViewport(0, 0, fbSize.x, fbSize.y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        //Lecture
        if(printTexture == 1){
			/*Rendu - GBuffer*/
            glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
            glReadBuffer(GL_COLOR_ATTACHMENT0 + textureToPrint);
            glBlitFramebuffer(0, 0, m_nWindowWidth, m_nWindowHeight, 0, 0, m_nWindowWidth, m_nWindowHeight,  GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}
		else if (printTexture == 2) { //Light Depth Map
			m_displayDepthProgram.use();

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m_directionalSMTexture);

			glUniform1i(m_uGDepthSamplerLocation, 0);

			glBindVertexArray(vaoQuad);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
        else{
			/*Rendu - Shading Pass*/

            programShading.use();
            
            //Lightning - General
            glUniform3fv( directionalLightDir, 1, glm::value_ptr( view.getViewMatrix() * glm::vec4(sin(anglePhi)*cos(angleTheta), sin(anglePhi)*sin(angleTheta), cos(anglePhi), 0)));
            glUniform3fv( directionalLightIntensity, 1, glm::value_ptr( colorDir*intensityDir ));

			//Shadow Mapping - Uniform
			const auto rcpViewMatrix = view.getRcpViewMatrix(); // Inverse de la view matrix de la caméra

			glUniformMatrix4fv(m_uDirLightViewProjMatrix_shadingPass, 1, GL_FALSE, glm::value_ptr(dirLightProjMatrix * dirLightViewMatrix * rcpViewMatrix));
			glUniform1fv(m_uDirLightShadowMapBias, 1, &shadowMapBias);
            

            for(int i = 0; i < 5; ++i){
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[i]);
            }
			
			//Shadow Map Texture
			glActiveTexture(GL_TEXTURE0 + 5);
			glBindTexture(GL_TEXTURE_2D, m_directionalSMTexture);
			glBindSampler(5, m_directionalSMSampler);
			glUniform1i(m_uDirLightShadowMap, 5);

            glUniform1i(positionLocation, 0); // Set the uniform to 0 because we use texture unit 0
            glUniform1i(normalLocation, 1); // Set the uniform to 1 because we use texture unit 1
            glUniform1i(ambientLocation, 2);
            glUniform1i(diffuseLocation, 3);
            glUniform1i(glossyLocation, 4);

			//Draw
			glBindVertexArray(vaoQuad);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);

			glActiveTexture(GL_TEXTURE0 + 5);
			glBindTexture(GL_TEXTURE_2D, 0);

            for(int i = 0; i < 5; ++i){
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, 0);
            }

        }

        

        // GUI code:
		glmlv::imguiNewFrame();

        {
            ImGui::Begin("GUI");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            if (ImGui::CollapsingHeader("Directional Light"))
            {
				directionalSMDirty |= ImGui::DragFloat("Dir angle Phi", &anglePhi);
				directionalSMDirty |= ImGui::DragFloat("Dir angle Theta", &angleTheta);
				directionalSMDirty |= ImGui::DragFloat("DirLightIntensity", &intensityDir);
				directionalSMDirty |= ImGui::ColorEdit3("DirLightColor", glm::value_ptr(colorDir));
            }
			ImGui::DragFloat("Shadow Map Bias", &shadowMapBias);
            ImGui::RadioButton("Result", &printTexture, 0);  ImGui::SameLine();
            ImGui::RadioButton("One texture", &printTexture, 1); ImGui::SameLine();
			ImGui::RadioButton("Depth Map", &printTexture, 2);
            if (ImGui::CollapsingHeader("FrameBuffer image"))
            {
                ImGui::RadioButton("GPosition", &textureToPrint, 0);  ImGui::SameLine();
                ImGui::RadioButton("GNormal", &textureToPrint, 1); ImGui::SameLine();
                ImGui::RadioButton("GAmbient", &textureToPrint, 2); ImGui::SameLine();
                ImGui::RadioButton("GDiffuse", &textureToPrint, 3); ImGui::SameLine();
                ImGui::RadioButton("GGlossyShininess", &textureToPrint, 4);
            }
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
    intensityDir(1),
    colorDir {0.828,0.438,0.130},
    textureToPrint {2},
    printTexture {2}
{
    ImGui::GetIO().IniFilename = m_ImGuiIniFilename.c_str(); // At exit, ImGUI will store its windows positions in this file

    // Put here initialization code
    //_____________________________________________________PROGRAM   FRAGMENT SHADER___________________________________________________________________________________________
    const auto pathToGPVS = m_ShadersRootPath / m_AppName / "geometryPass.vs.glsl";
    const auto pathToGPFS = m_ShadersRootPath / m_AppName / "geometryPass.fs.glsl";

    programGeometry = glmlv::compileProgram({ pathToGPVS, pathToGPFS });
    programGeometry.use();


    glmlv::loadObjScene(m_AssetsRootPath / m_AppName / "sponza/sponza.obj", data);

	m_BBoxMin = data.bboxMin;
	m_BBoxMax = data.bboxMax;
	m_SceneSizeLenght = glm::length(data.bboxMax - data.bboxMin);

    //const auto sceneDiagonalSize = glm::length(data.bboxMax - data.bboxMin);
    view.setSpeed(m_SceneSizeLenght * 0.2f); // 10% de la scene parcouru par seconde

    //sceneSize = glm::length(data.bboxMax - data.bboxMin);
    ProjMatrix = glm::perspective(70.f, float(m_nWindowWidth) / m_nWindowHeight, 0.01f * m_SceneSizeLenght, m_SceneSizeLenght); // near = 1% de la taille de la scene, far = 100%


    glEnable(GL_DEPTH_TEST);

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
        glBindTexture(GL_TEXTURE_2D, 0);
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

	//FOR SHADOW MAP
	m_uDirLightViewProjMatrix_shadingPass = programShading.getUniformLocation("uDirLightViewProjMatrix");
	m_uDirLightShadowMap = programShading.getUniformLocation("uDirLightShadowMap");
	m_uDirLightShadowMapBias = programShading.getUniformLocation("uDirLightShadowMapBias");

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

	//_____________________________________________________TEXTURE & FBO SHADOW MAP___________________________________________________________________________________________
	glGenTextures(1, &m_directionalSMTexture);

	glBindTexture(GL_TEXTURE_2D, m_directionalSMTexture);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, m_nDirectionalSMResolution, m_nDirectionalSMResolution);
	glBindTexture(GL_TEXTURE_2D, 0);

	//FBO
	glGenFramebuffers(1, &m_directionalSMFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_directionalSMFBO);
	glBindTexture(GL_TEXTURE_2D, m_directionalSMTexture);


	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_directionalSMTexture, 0);

	GLenum smStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (smStatus != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "FB error, status: " << smStatus << std::endl;
		throw std::runtime_error("FBO error");
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	//Sampler
	glGenSamplers(1, &m_directionalSMSampler);
	glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	//_____________________________________________________PROGRAM SHADOW MAP___________________________________________________________________________________________
	
	const auto pathToSMVS = m_ShadersRootPath / m_AppName / "directionalSM.vs.glsl";
	const auto pathToSMFS = m_ShadersRootPath / m_AppName / "directionalSM.fs.glsl";

	m_directionalSMProgram = glmlv::compileProgram({ pathToSMVS, pathToSMFS });
	//m_directionalSMProgram.use();

	//LIGHT
	m_uDirLightViewProjMatrix = glGetUniformLocation(m_directionalSMProgram.glId(), "uDirLightViewProjMatrix");

	//_____________________________________________________PROGRAM DEPTH MAP - DEBUG___________________________________________________________________________________________
	m_displayDepthProgram = glmlv::compileProgram({ m_ShadersRootPath / m_AppName / "shadingPass.vs.glsl", m_ShadersRootPath / m_AppName / "displayDepth.fs.glsl" });

	m_uGDepthSamplerLocation = glGetUniformLocation(m_displayDepthProgram.glId(), "uGDepth");


}