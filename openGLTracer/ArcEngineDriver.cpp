// openGLTracer.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "ArcEngineDriver.h"

#include "Swb_MeshLoader.h"
#include "ArcMaterial.h"

// Window dimensions    

// The MAIN function, from here we start the application and run the game loop    
int main()
{
	std::cout << "Starting GLFW context, OpenGL 4.0" << std::endl;
	// rename test
	// Init GLFW    
	glfwInit();
	// Set all the required options for GLFW    
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	
	// Create a GLFWwindow object that we can use for GLFW's functions    
	window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGLTracer", nullptr, nullptr);

	//设置鼠标
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPos(window, 0, 0);

	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	// Set the required callback functions    
	Arc_Engine::ArcInput::setWindowAndKeyboardCallback(window);  // 设置ArcInput里的windows并注册默认回调函数

	//currentPath = getCurrentPath();

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions    
	glewExperimental = GL_TRUE;
	// Initialize GLEW to setup the OpenGL Function pointers    
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}
	
	if (!GLEW_VERSION_4_0)
		throw std::runtime_error("OpenGL 4.0 API is not available.");

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// Define the viewport dimensions    
	glViewport(0, 0, WIDTH, HEIGHT);

	if (EnableShadow) {
		//glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		//glClear(GL_DEPTH_BUFFER_BIT);
		//ConfigureShaderAndMatrices();
	}
	
	app = std::make_shared<Application>(std::make_shared<Arc_Engine::ArcScene>(), WIDTH, HEIGHT);

	std::cout << "Using Application: " << app->name() << std::endl;
	std::cout << (EnableTest ? "Test Enabled":"Test Unenabled") << std::endl;
	
	Arc_Engine::ArcGameObject::setGameObjectVector((app->scene()->getGameObjectsInScene()));
	
	auto gameObjectList = app->scene()->getGameObjectsInScene();

#pragma region shadow_region
	if (EnableShadow) {
		currentPath = Arc_Engine::ArcTools::getCurrentPath();
		simpleDepthShaderProgram = Arc_Engine::ArcMaterial::loadShaderAndCreateProgram(currentPath + shader_path + depth_vert_shader_path, currentPath + shader_path + depth_frag_shader_path);
		debugDepthShaderProgram = Arc_Engine::ArcMaterial::loadShaderAndCreateProgram(currentPath + shader_path + debug_depth_vert_shader_path, currentPath + shader_path + debug_depth_frag_shader_path);
		Arc_Engine::ArcTextureLoader::createDepthMap(&depthMap); //创建shadow用的depthMap
		createShadowBuffer(&depthMapFBO, depthMap);
	}
#pragma endregion
	
	for (int i = 0; i < gameObjectList.size(); i++) {
		for (int j = 0; j < (gameObjectList[i]->behaviourListLength()); j++) {
			(gameObjectList[i]->getBehaviourList())[j]->Awake();
		}
	}

	for (int i = 0; i < gameObjectList.size(); i++) {
		for (int j = 0; j < (gameObjectList[i]->behaviourListLength()); j++) {
			(gameObjectList[i]->getBehaviourList())[j]->Start();
		}
	}

	//注册鼠标滚轮事件
	if (EnableTest) {
		glfwSetScrollCallback(window, OnScroll);
	}
	
	// Game loop
	float lastTime = glfwGetTime();
	float deltaTime, thisTime;
	while (!glfwWindowShouldClose(window))
	{
		// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions  
		glfwPollEvents();

		thisTime = glfwGetTime();
		deltaTime = thisTime - lastTime;
		lastTime = thisTime;

		Arc_Engine::ArcTime::setDeltaTime(deltaTime);

		if (EnableTest) {
			TestUpdate(deltaTime);//update函数
		}
#pragma region shadow_region
		if (EnableShadow) {
			glViewport(0, 0, WIDTH, HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
			glClear(GL_DEPTH_BUFFER_BIT);
			auto gameObjectList = app->scene()->getGameObjectsInScene();
			for (int i = 0; i < gameObjectList.size(); i++) {
				if (gameObjectList[i]->renderer() != nullptr) {
					//RenderInstance(gameObjectList[i], simpleDepthShaderProgram, gameObjectList[i]->renderer()->texture);
				}
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			
			glViewport(0, 0, WIDTH, HEIGHT);
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glUniform1f(glGetUniformLocation(debugDepthShaderProgram, "near_plane"), app->mainCamera()->nearPlane());
			glUniform1f(glGetUniformLocation(debugDepthShaderProgram, "far_plane"), app->mainCamera()->farPlane());
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			RenderQuad();
			/*for (int i = 0; i < gameObjectList.size(); i++) {
				if (gameObjectList[i]->renderer() != nullptr) {
					RenderInstance(gameObjectList[i], debugDepthShaderProgram, depthMap);
				}
			}*/
			
		}
#pragma endregion
		
		// Render 
		draw();
		// Swap the screen buffers
		glfwSwapBuffers(window);

		for (int i = 0; i < gameObjectList.size(); i++) {
			for (int j = 0; j < (gameObjectList[i]->behaviourListLength()); j++) {
				(gameObjectList[i]->getBehaviourList())[j]->Update();
			}
		}
		
		if (Arc_Engine::ArcInput::getKeyDown(GLFW_KEY_ESCAPE)) { // GLFW_KEY_ESCAPE = 256
			glfwSetWindowShouldClose(window, GL_TRUE);  // 按下esc关闭窗口
		}
		
		Arc_Engine::ArcInput::clearCache();  // 清除当前帧的Input缓存
		// check for errors
		GLenum error = glGetError();
		if (error != GL_NO_ERROR)
			std::cerr << "OpenGL Error " << error << std::endl;
	}

	// Terminate GLFW, clearing any resources allocated by GLFW.    
	glfwTerminate();
	return 0;
}

GLuint quadVAO = 0;
GLuint quadVBO;
void RenderQuad()
{
	if (quadVAO == 0)
	{
		GLfloat quadVertices[] = {
			// Positions        // Texture Coords
			-1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
		};
		// Setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void draw() {
	// Clear the colorbuffer    
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//std::list<Arc_Engine::ArcGameObject>::const_iterator it;//画所有模型
	auto gameObjectList = app->scene()->getGameObjectsInScene();
	for (int i = 0; i < gameObjectList.size(); i++) {
		if (gameObjectList[i]->renderer() != nullptr) {
			gameObjectList[i]->renderer()->material->callRenderFunction(gameObjectList[i], app);
			//RenderInstance(gameObjectList[i]);
		}
	}
}

/*void RenderInstance(std::shared_ptr<Arc_Engine::ArcGameObject> inst) {
	RenderInstance(inst, inst->renderer()->material->program(), inst->renderer()->texture);
}

void RenderInstance(std::shared_ptr<Arc_Engine::ArcGameObject> inst, GLuint program, GLuint texture) {
	std::shared_ptr<Arc_Engine::ArcRenderer> renderer = inst->renderer();
	
	//bind the shaders
	glUseProgram(program);

	GLint cameraMatLocation = glGetUniformLocation(program, "camera");
	glUniformMatrix4fv(cameraMatLocation, 1, GL_FALSE, glm::value_ptr(app->mainCamera()->matrix()));

	GLint modelMatLocation = glGetUniformLocation(program, "model");
	glUniformMatrix4fv(modelMatLocation, 1, GL_FALSE, glm::value_ptr(inst->transform().transformMatrix()));

	GLint lightAmbientLoc = glGetUniformLocation(program, "light.ambient");
	GLint lightDiffuseLoc = glGetUniformLocation(program, "light.diffuse");
	GLint lightDirLoc = glGetUniformLocation(program, "light.direction");

	auto mainSceneLight = app->scene()->light();
	
	glUniform3f(lightAmbientLoc, mainSceneLight->ambient().x, mainSceneLight->ambient().y, mainSceneLight->ambient().z);
	glUniform3f(lightDiffuseLoc, mainSceneLight->diffuse().x, mainSceneLight->diffuse().y, mainSceneLight->diffuse().z);
	glUniform3f(lightDirLoc, mainSceneLight->direction().x, mainSceneLight->direction().y, mainSceneLight->direction().z); // 方向光源

	// 设置材料光照属性
	GLint texLocation = glGetUniformLocation(program, "U_MainTexture");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(texLocation, 0);
	
	//bind VAO and draw
	glBindVertexArray(renderer->vao);
	glDrawArrays(renderer->drawType, renderer->drawStart, renderer->drawCount);

	//unbind everything
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}*/

void createShadowBuffer(GLuint* depthMapFBO, GLuint depthMap) {
	glGenFramebuffers(1, depthMapFBO);

	glBindFramebuffer(GL_FRAMEBUFFER, *depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void loadModel() {
	//Swb_MeshLoader::loadMesh(currentPath + model_path, testModel);
	//std::cout << testModel->vSets.size()<< " "<< testModel->fSets.size() << std::endl;
}

void TestUpdate(float secondsElapsed) {
	treatKeyboardInput(secondsElapsed);
	treatMouseInput(secondsElapsed);
}

void treatKeyboardInput(float secondsElapsed) {
	const float moveSpeed = 2.0; //units per second

	auto mainCameraTransformPtr = app->mainCamera()->gameObject()->transformPtr();

	if (glfwGetKey(window, GLFW_KEY_DOWN)) {
		mainCameraTransformPtr->translate(secondsElapsed * moveSpeed * -mainCameraTransformPtr->up());
	}
	else if (glfwGetKey(window, GLFW_KEY_UP)) {
		mainCameraTransformPtr->translate(secondsElapsed * moveSpeed * mainCameraTransformPtr->up());
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT)) {
		mainCameraTransformPtr->translate(secondsElapsed * moveSpeed * -mainCameraTransformPtr->right());
	}
	else if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
		mainCameraTransformPtr->translate(secondsElapsed * moveSpeed * mainCameraTransformPtr->right());
	}
}

void treatMouseInput(float secondsElapsed) {
	
	auto mainCameraTransformPtr = app->mainCamera()->gameObject()->transformPtr();
	//鼠标位置
	const float mouseSensitivity = 0.1f;
	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);
	mainCameraTransformPtr->rotate(mouseSensitivity * (float)mouseY, mouseSensitivity * (float)mouseX, 0.0f);
	glfwSetCursorPos(window, 0, 0);

	//滚轮
	const float zoomSensitivity = -0.2f;
	float fieldOfView = app->mainCamera()->fieldOfView() + zoomSensitivity * (float)gScrollY;
	if (fieldOfView < 5.0f) fieldOfView = 5.0f;
	if (fieldOfView > 130.0f) fieldOfView = 130.0f;
	app->mainCamera()->setFieldOfView(fieldOfView);
	gScrollY = 0;
}

void OnScroll(GLFWwindow* window, double deltaX, double deltaY) {
	gScrollY += deltaY;
}


