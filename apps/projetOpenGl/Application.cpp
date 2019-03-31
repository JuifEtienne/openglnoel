#define  TINYGLTF_IMPLEMENTATION 

#include "Application.hpp"

#include <iostream>
#include <vector>
#include <math.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glmlv/simple_geometry.hpp>
#include <glmlv/GLProgram.hpp>
#include <glmlv/ViewController.hpp>
#include "Trackball.hpp"


//______________________________________________ PARSING GLTF TO GET MATERIALS __________________________________________________//

void Application::PrintParameterMap(const tinygltf::ParameterMap &pmap) {

	for (auto &kv : pmap) {
		tinygltf::Parameter param = kv.second;

		if (kv.first == "emissiveFactor") {
			if (!param.number_array.empty()) {
				glm::vec3 emisFactor(param.number_array[0], param.number_array[1], param.number_array[2]);
				glUniform3fv(emissionColorFactor, 1, glm::value_ptr(emisFactor));
			}
		}
		if (kv.first == "emissiveTexture") {
			auto p(param.json_double_value.begin());
			auto pEnd(param.json_double_value.end());
			for (; p != pEnd; p++) {
				if (p->first == "index") {
					emisColorTextureID = p->second;
				}
				if (p->first == "texCoord") {
					emisColorTexCoord = p->second;
				}
			}
		}
		if (kv.first == "baseColorFactor") {
			//ss << kv.first << " : " << PrintParameterValue(param) << std::endl;
			if (!param.number_array.empty()) {
				glm::vec4 baseFactor(param.number_array[0], param.number_array[1], param.number_array[2], param.number_array[3]);
				glUniform4fv(baseColorFactor, 1, glm::value_ptr(baseFactor));
			}
		}
		if (kv.first == "baseColorTexture") {
			auto p(param.json_double_value.begin());
			auto pEnd(param.json_double_value.end());
			for (; p != pEnd; p++) {
				if (p->first == "index") {
					baseColorTextureID = p->second;
				}
				if (p->first == "texCoord") {
					baseColorTexCoord = p->second;
				}
			}
		}
	}
}

//______________________________________________ USEFUL TRANSFORMATION FUNCTIONS __________________________________________________//

glm::dmat4 matrixToMat4(std::vector<double> tab) {
	glm::dmat4 mat{ tab[0], tab[1], tab[2], tab[3],
					tab[4], tab[5], tab[6], tab[7],
					tab[8], tab[9], tab[10], tab[11],
					tab[12], tab[13], tab[14], tab[15] };
	return mat;
}

glm::dmat4 quaternionToRotation(std::vector<double> tab) {
	double X = tab[0];
	double Y = tab[1];
	double Z = tab[2];
	double W = tab[3];

	double angle = 2 * acos(W);
	double x = X / sqrt(1 - W * W);
	double y = Y / sqrt(1 - W * W);
	double z = Z / sqrt(1 - W * W);

	glm::dmat4 rot = glm::rotate(glm::dmat4(1.0f), angle, glm::dvec3(x, y, z));

	return rot;
}

glm::dmat4 scaleToMat4(std::vector<double> tab) {
	return glm::scale(glm::dmat4(1.0f), glm::dvec3(tab[0], tab[1], tab[2]));
}


glm::dmat4 translationToMat4(std::vector<double> tab) {
	return glm::translate(glm::dmat4(1.0f), glm::dvec3(tab[0], tab[1], tab[2]));
}

void printMatrix(glm::dmat4 matrix) {
	for (int i = 0; i < 4; ++i) {
		std::cout << matrix[i][0] << " " << matrix[i][1] << " " << matrix[i][2] << " " << matrix[i][3] << std::endl;
	}
}

std::string dirnameOf(const std::string& fname)
{
	size_t pos = fname.find_last_of("\\/");
	return (std::string::npos == pos)
		? ""
		: fname.substr(0, pos+1);
}

void vecMax(glm::vec3 &toMax, std::vector<double> toCompare) {
	for (int i = 0; i < toCompare.size(); ++i) {
		if (toMax[i] < toCompare[i]) {
			toMax[i] = toCompare[i];
		}
	}
}

void vecMin(glm::vec3 &toMin, std::vector<double> toCompare) {
	for (int i = 0; i < toCompare.size(); ++i) {
		if (toMin[i] > toCompare[i]) {
			toMin[i] = toCompare[i];
		}
	}
}


//______________________________________________ APPLICATION RUN FUNCTION __________________________________________________//

int Application::run()
{
    program.use();

    ProjMatrix = glm::perspective(glm::radians(70.f), 1.0f*m_nWindowWidth / m_nWindowHeight, 0.1f, 100.0f);

	//Change unit texture
	//Not Changing during render
	glUniform1i(baseColorLocation, 0); // Set the uniform to 0 because we use texture unit 0
	glUniform1i(emissionColorLocation, 1); // Set the uniform to 1 because we use texture unit 1
	
	//place camera
	Trackball::moveFront(m_GLFWHandle.window(), 0, 2*boundingBMin[2]);

    // Loop until the user closes the window
    for (auto iterationCount = 0u; !m_GLFWHandle.shouldClose(); ++iterationCount)
    {

        const auto seconds = glfwGetTime();
		
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const auto fbSize = m_GLFWHandle.framebufferSize();

		//______________________________________________ GEOMETRY PASS __________________________________________________//

		program.use();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);

		glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for (int i = 0; i < vaos.size(); ++i) {
			
			MVMatrixCube = glm::translate((glm::dmat4)view->getViewMatrix()*matrix[i], glm::dvec3(0, 0, -5.0f));
			NormalMatrixCube = glm::transpose(glm::inverse(MVMatrixCube));

			//Envoie des matrices
			glUniformMatrix4fv(modelViewProjMatrix, 1, GL_FALSE, glm::value_ptr(ProjMatrix * MVMatrixCube));
			glUniformMatrix4fv(modelViewMatrix, 1, GL_FALSE, glm::value_ptr(MVMatrixCube));
			glUniformMatrix4fv(normalMatrix, 1, GL_FALSE, glm::value_ptr(NormalMatrixCube));

			//get elements for texture, emission and base colors
			PrintParameterMap(model.materials[primitives[i].material].additionalValues);
			PrintParameterMap(model.materials[primitives[i].material].values);

			glUniform1i(baseCoord, baseColorTexCoord);
			glUniform1i(emissionCoord, emisColorTexCoord);


			//BaseColor
			if (baseColorTextureID != -1) {
				//sampler
				glActiveTexture(GL_TEXTURE0);
				glBindSampler(0, samplers[model.textures[baseColorTextureID].sampler]);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, textures[baseColorTextureID]);
			}

			//EmisColor
			if (emisColorTextureID != -1) {
				//sampler
				glActiveTexture(GL_TEXTURE1);
				glBindSampler(1, samplers[model.textures[emisColorTextureID].sampler]);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, textures[emisColorTextureID]);
			}

			glBindVertexArray(vaos[i]);
			
			tinygltf::Accessor indexAccessor = model.accessors[primitives[i].indices];
			glDrawElements(primitives[i].mode, indexAccessor.count, indexAccessor.componentType, (const void*) indexAccessor.byteOffset);
			glBindVertexArray(0);


			//Reinit for next
			baseColorTextureID = -1;
			baseColorTexCoord = -1;
			emisColorTextureID = -1;
			emisColorTexCoord = -1;
		}

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		
		//______________________________________________ SHADING PASS __________________________________________________//

		glViewport(0, 0, fbSize.x, fbSize.y);
        glBindTexture(GL_TEXTURE_2D, 0);


		if (printTexture == 1) {
			/*___________________ DISPLAY GBUFFER TEXTURES _____________________*/

			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
			glReadBuffer(GL_COLOR_ATTACHMENT0 + textureToPrint);
			glBlitFramebuffer(0, 0, m_nWindowWidth, m_nWindowHeight, 0, 0, m_nWindowWidth, m_nWindowHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}
		else {
			/*___________________ SHADING PROGRAM _____________________*/
			programShading.use();
			glBindVertexArray(vaoQuad);

			//Lightning - General
			glUniform3fv(directionalLightDir, 1, glm::value_ptr(view->getViewMatrix() * glm::vec4(sin(anglePhi)*cos(angleTheta), sin(anglePhi)*sin(angleTheta), cos(anglePhi), 0)));
			glUniform3fv(directionalLightIntensity, 1, glm::value_ptr(colorDir*intensityDir));


			for (int i = 0; i < 4; ++i) {
				glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[i]);
			}

			glUniform1i(positionLocation, 0); // Set the uniform to 0 because we use texture unit 0
			glUniform1i(normalLocation, 1); // Set the uniform to 1 because we use texture unit 1
			glUniform1i(ambientLocation, 2);
			glUniform1i(diffuseLocation, 3);

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			for (int i = 0; i < 4; ++i) {
				glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

		}

		//______________________________________________ GUI __________________________________________________//
		glmlv::imguiNewFrame();

        {
            ImGui::Begin("Application Parameters");
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
			if (ImGui::CollapsingHeader("FrameBuffer image"))
			{
				ImGui::RadioButton("GPosition", &textureToPrint, 0);  ImGui::SameLine();
				ImGui::RadioButton("GNormal", &textureToPrint, 1); ImGui::SameLine();
				ImGui::RadioButton("GAmbient", &textureToPrint, 2); ImGui::SameLine();
				ImGui::RadioButton("GDiffuse", &textureToPrint, 3);
			}
            ImGui::End();
        }

		glmlv::imguiRenderFrame();

        glfwPollEvents(); // Poll for and process events

        auto ellapsedTime = glfwGetTime() - seconds;
        auto guiHasFocus = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
        if (!guiHasFocus) {
            // Put here code to handle user interactions
            view->update(ellapsedTime);
        }

		m_GLFWHandle.swapBuffers(); // Swap front and back buffers
    }

    return 0;
}

//______________________________________________ APPLICATION CONSTRUCTOR __________________________________________________//

Application::Application(int argc, char** argv):
    m_AppPath { glmlv::fs::path{ argv[0] } },
    m_AppName { m_AppPath.stem().string() },
    m_ImGuiIniFilename { m_AppName + ".imgui.ini" },
    m_ShadersRootPath { m_AppPath.parent_path() / "shaders" },
    m_AssetsRootPath { m_AppPath.parent_path() / "assets" },
	view { new Trackball(m_GLFWHandle.window(), 10) },
	anglePhi(1),
	angleTheta(4),
    intensityDir(2),
    colorDir {0.7,0.7,0.7},
	textureToPrint{ 3 },
	printTexture{ 0 }
{
	ImGui::GetIO().IniFilename = m_ImGuiIniFilename.c_str(); // At exit, ImGUI will store its windows positions in this file


	//______________________________________________ OBJECT LOADING AND INITIALIZATION __________________________________________________//

	// Put here initialization code
	glEnable(GL_DEPTH_TEST);

	//Initialize map
	const GLuint VERTEX_ATTR_POSITION = 0;
	const GLuint VERTEX_ATTR_NORMAL = 1;
	const GLuint VERTEX_ATTR_TEXCOORDS = 2;
	const GLuint VERTEX_ATTR_TEXCOORDS1 = 3;
	const GLuint VERTEX_ATTR_TANGENT = 4;


	//std::map<int, std::string> attribIndexOf
	attribIndexOf.insert({ "POSITION", VERTEX_ATTR_POSITION });
	attribIndexOf.insert({ "NORMAL", VERTEX_ATTR_NORMAL });
	attribIndexOf.insert({ "TEXCOORD_0", VERTEX_ATTR_TEXCOORDS });
	attribIndexOf.insert({ "TEXCOORD_1", VERTEX_ATTR_TEXCOORDS1 });
	attribIndexOf.insert({ "TANGENT", VERTEX_ATTR_TANGENT });

	numberOfComponentOf.insert({ TINYGLTF_TYPE_VEC2, 2 });
	numberOfComponentOf.insert({ TINYGLTF_TYPE_VEC3, 3 });
	numberOfComponentOf.insert({ TINYGLTF_TYPE_VEC4, 4 });

	attribEnum.insert({ 4, GL_TRIANGLES });

	//TINY GLTF
	std::cout << argv[1] << std::endl;

	const auto ret = loader.LoadASCIIFromFile(&model, &err, &warn, argv[1]);

	if (!warn.empty()) {
		printf("Warn: %s\n", warn.c_str());
	}

	if (!err.empty()) {
		printf("Err: %s\n", err.c_str());
	}

	if (!ret) {
		printf("Failed to parse glTF\n");
	}

	//TINY GLTF - VBO
	std::vector<GLuint> buffers(model.buffers.size()); // un par tinygltf::Buffer / c'est les vbos

	glGenBuffers(buffers.size(), buffers.data());
	for (int i = 0; i < model.buffers.size(); ++i) {
		glBindBuffer(GL_ARRAY_BUFFER, buffers[i]);
		glBufferStorage(GL_ARRAY_BUFFER, model.buffers[i].data.size(), model.buffers[i].data.data(), 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	boundingBMax = glm::vec3(0.0f);
	boundingBMin = glm::vec3(0.0f);

	//TINY GLTF - VAO
	for (int i = 0; i < model.meshes.size(); ++i) {
		for (int j = 0; j < model.meshes[i].primitives.size(); ++j) {
			GLuint vaoId;

			//std::cout << model.meshes[i].name << std::endl;

			glGenVertexArrays(1, &vaoId);
			glBindVertexArray(vaoId);
			tinygltf::Accessor indexAccessor = model.accessors[model.meshes[i].primitives[j].indices];
			tinygltf::BufferView bufferView = model.bufferViews[indexAccessor.bufferView];
			int bufferIndex = bufferView.buffer;
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[bufferIndex]); // Ici on bind le buffer OpenGL qui a été rempli dans la premiere boucle
			

			for (std::map<std::string, int>::iterator it = model.meshes[i].primitives[j].attributes.begin(); it != model.meshes[i].primitives[j].attributes.end(); ++it) {
				//std::cout << it->first << std::endl;

				tinygltf::Accessor accesor = model.accessors[model.meshes[i].primitives[j].attributes[it->first]];


				//get max/min bounding box
				if (numberOfComponentOf[accesor.type] == 3) {
					vecMax(boundingBMax, accesor.maxValues);
					vecMin(boundingBMin, accesor.minValues);
				}


				bufferView = model.bufferViews[accesor.bufferView];
				bufferIndex = bufferView.buffer;
				glBindBuffer(GL_ARRAY_BUFFER, buffers[bufferIndex]);
				glEnableVertexAttribArray(attribIndexOf[it->first]); // Ici je suppose qu'on a prérempli une map attribIndexOf qui associe aux strings genre "POSITION" un index d'attribut du vertex shader (les location = XXX du vertex shader); dans les TPs on utilisait 0 pour position, 1 pour normal et 2 pour tex coords
				glVertexAttribPointer(attribIndexOf[it->first], numberOfComponentOf[accesor.type], accesor.componentType, GL_FALSE, bufferView.byteStride, (const GLvoid*)(bufferView.byteOffset + accesor.byteOffset)); // Ici encore il faut avoir remplit une map numberOfComponentOf qui associe un type gltf (comme "VEC2") au nombre de composantes (2 pour "VEC2", 3 pour "VEC3")
				glBindBuffer(GL_ARRAY_BUFFER, 0);

			}

			vaos.push_back(vaoId);
			vaos.size();
			primitives.push_back(model.meshes[i].primitives[j]);
		}
	}

	

	for (int i = 0; i < primitives.size(); ++i) {
		matrix.push_back(glm::mat4(1.0f));
	}

	//MATRIX TRANSFORM

  /*for (int i = 0; i < model.nodes.size(); ++i)
  {
	  std::vector<int> indexMesh;
	  getMeshs(model.nodes[i], indexMesh); //for one node, check all of the tree to get all of the index of the meshes which are "child" of the node
	  glm::dmat4 nodeMatrix = glm::dmat4(1.0f);

	  if (model.nodes[i].matrix.size() == 16) {
		  nodeMatrix = matrixToMat4(model.nodes[i].matrix);
	  }
	  else { //T*R*S
		  if (model.nodes[i].scale.size() == 3) {
			  nodeMatrix = scaleToMat4(model.nodes[i].scale)*nodeMatrix;
		  }
		  if (model.nodes[i].rotation.size() == 4) {
			  nodeMatrix = quaternionToRotation(model.nodes[i].rotation)*nodeMatrix;
		  }
		  if (model.nodes[i].translation.size() == 3) {
			  nodeMatrix = translationToMat4(model.nodes[i].translation)*nodeMatrix;
		  }
	  }

	  for (int j = 0; j < indexMesh.size(); ++j) {
		  matrix[indexMesh[j]] = matrix[indexMesh[j]]*nodeMatrix;
		  //The elder parent will be the first to be computed so that will do ParentOld*ParentYoung*Mesh (for example)
		  //If I change the order => nothing change
	  }
  }
  std::cout << "all my matrix____________________________________________" << std::endl;
  for (int i = 0; i < matrix.size(); ++i) {
	  printMatrix(matrix[i]);
	  std::cout << std::endl;
  }*/
  

  //______________________________________________ GEOMETRY PASS - PROGRAM __________________________________________________//	

	const auto pathToGPVS = m_ShadersRootPath / m_AppName / "projet.vs.glsl";
	const auto pathToGPFS = m_ShadersRootPath / m_AppName / "projet.fs.glsl";

	program = glmlv::compileProgram({ pathToGPVS, pathToGPFS });
	program.use();

	//TEXTURES
	for (int i = 0; i < model.images.size(); ++i) {
		std::cout << dirnameOf(argv[1]) + model.images[i].uri << std::endl;
		images.push_back(glmlv::readImage(dirnameOf(argv[1]) + model.images[i].uri));

		GLuint texture;

		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &texture);

		glBindTexture(GL_TEXTURE_2D, texture);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, images[i].width(), images[i].height());
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, images[i].width(), images[i].height(), GL_RGBA, GL_UNSIGNED_BYTE, images[i].data());
		glBindTexture(GL_TEXTURE_2D, 0);

		textures.push_back(texture);
	}

	//SAMPLERS
	for (int i = 0; i < model.samplers.size(); ++i) {
		GLuint sampler;

		glGenSamplers(1, &sampler);
		glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, model.samplers[i].magFilter);
		glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, model.samplers[i].minFilter);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, model.samplers[i].wrapS);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, model.samplers[i].wrapT);

		samplers.push_back(sampler);
	}

	//UNIFORM

	//Matrix
	modelViewProjMatrix = program.getUniformLocation("uModelViewProjMatrix");
	modelViewMatrix = program.getUniformLocation("uModelViewMatrix");
	normalMatrix = program.getUniformLocation("uNormalMatrix");

	//samplerLocation
	baseColorLocation = program.getUniformLocation("uBaseColor");
	emissionColorLocation = program.getUniformLocation("uEmissionColor");

	//choose texCoords
	baseCoord = program.getUniformLocation("uBaseCoord");
	emissionCoord = program.getUniformLocation("uEmissionCoord");

	//factor
	baseColorFactor = program.getUniformLocation("uBaseFactor");
	emissionColorFactor = program.getUniformLocation("uEmissionFactor");


	//______________________________________________ SHADING PASS - PROGRAM __________________________________________________//

	const auto pathToSPVS = m_ShadersRootPath / m_AppName / "shadingPass.vs.glsl";
	const auto pathToSPFS = m_ShadersRootPath / m_AppName / "shadingPass.fs.glsl";

	programShading = glmlv::compileProgram({ pathToSPVS, pathToSPFS });
	programShading.use();

	//Textures
	glGenTextures(GBufferTextureCount, m_GBufferTextures);

	for (int i = 0; i < GBufferTextureCount; ++i) {
		glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[i]);
		glTexStorage2D(GL_TEXTURE_2D, 1, m_GBufferTextureFormat[i], m_nWindowWidth, m_nWindowHeight);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	//FBO
	glGenFramebuffers(1, &m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);

	for (int i = 0; i < GBufferTextureCount - 1; ++i) {
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_GBufferTextures[i], 0);
	}
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_GBufferTextures[GBufferTextureCount - 1], 0);

	GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(GBufferTextureCount, drawBuffers);

	GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (status != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "FB error, status: " << status << std::endl;
		throw std::runtime_error("FBO error");
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);


	//LIGHT
	directionalLightDir = programShading.getUniformLocation("uDirectionalLightDir_vs");
	directionalLightIntensity = programShading.getUniformLocation("uDirectionalLightIntensity");

	positionLocation = programShading.getUniformLocation("uGPosition");
	normalLocation = programShading.getUniformLocation("uGNormal");
	ambientLocation = programShading.getUniformLocation("uGAmbient");
	diffuseLocation = programShading.getUniformLocation("uGDiffuse");


	//______________________________________________ QUAD GEOMETRY - FOR DEFERRED __________________________________________________//
	
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


	//______________________________________________ GAMMA CORRECTION - PROGRAM __________________________________________________//

	/*const auto pathToGCCS = m_ShadersRootPath / m_AppName / "gammaCorrect.cs.glsl";

	m_gammaCorrectionProgram = glmlv::compileProgram({ pathToGCCS });
	m_gammaCorrectionProgram.use();

	m_uGammaExponent = m_gammaCorrectionProgram.getUniformLocation("uGammaExponent");*/
}



void Application::getMeshs(tinygltf::Node node, std::vector<int> &meshIdx) {
	if (node.mesh != -1) 
	{
		//std::cout << "here" << std::endl;
		meshIdx.push_back(node.mesh);
	}

	if(node.children.size() != 0) 
	{
		for (int nodeIdx : node.children) {
			getMeshs(model.nodes[nodeIdx], meshIdx);
		}
	}

}