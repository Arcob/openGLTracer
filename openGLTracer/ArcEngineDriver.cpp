// openGLTracer.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "ArcEngineDriver.h"
#include "ArcMaterial.h"


// Window dimensions    

// The MAIN function, from here we start the application and run the game loop    
int main()
{
	std::cout << "Starting GLFW context, OpenGL 4.0" << std::endl;
	gScrollY = 0.0;
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
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	
	// Define the viewport dimensions
	glViewport(0, 0, WIDTH, HEIGHT);

	glCullFace(GL_BACK);
	
	app = std::make_shared<Application>(std::make_shared<Arc_Engine::ArcScene>(), WIDTH, HEIGHT);

	std::cout << "Using Application: " << app->name() << std::endl;
	std::cout << (ENABLE_TEST ? "Test Enabled":"Test Disabled") << std::endl;
	std::cout << (ENABLE_POST_EFFECT ? "Post Effect Enabled" : "Post Effect Disabled") << std::endl;
	
	Arc_Engine::ArcGameObject::setGameObjectVector((app->scene()->getGameObjectsInScene()));
	
	auto gameObjectList = app->scene()->getGameObjectsInScene();

#pragma region shadow_region
	if (ENABLE_SHADOW) {
		simpleDepthShaderProgram = Arc_Engine::ArcMaterial::loadShadowProgram();
	}
#pragma endregion

#pragma region postEffect_region
	if (ENABLE_POST_EFFECT) {
		Arc_Engine::ArcPostEffectManager::generatePostEffectProgram();
		Arc_Engine::ArcPostEffectManager::generatePostEffectFBO();
		Arc_Engine::ArcPostEffectManager::generateSSAOProgram();
		Arc_Engine::ArcPostEffectManager::generateSSAONoiceTextures();
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
	if (ENABLE_TEST) {
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

		if (ENABLE_TEST) {
			TestUpdate(deltaTime);//update函数
		}

#pragma region shadow_region
		if (ENABLE_SHADOW) {
			//将场景的深度图渲染到与深度图绑定的frame buffer object里
			renderDepthToFBO(app->scene()->depthMapFBO());
			//Arc_Engine::ArcTools::drawDebugQuad(app->scene()->depthMap());	
		}
#pragma endregion
		
		if (ENABLE_POST_EFFECT) {
			glBindFramebuffer(GL_FRAMEBUFFER, app->scene()->postEffectFBO());

			// Render
			draw();

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, Arc_Engine::ArcPostEffectManager::postEffectFBO());

			Arc_Engine::ArcPostEffectManager::generateSSAOTexture(app->scene()->gBufferMap(), app->scene()->gPositionMap(), app->mainCamera()->projection());

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			
			Arc_Engine::ArcPostEffectManager::drawPostEffectQuad(app->scene()->postEffectMap(), app->scene()->gBufferMap(), app->scene()->gPositionMap(), app->mainCamera()->projection());
		}
		else {
			draw();
		}
		
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

void draw() {
	// Clear the colorbuffer    
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//std::list<Arc_Engine::ArcGameObject>::const_iterator it; //画所有模型
	auto gameObjectList = app->scene()->getGameObjectsInScene();
	for (int i = 0; i < gameObjectList.size(); i++) {
		if (gameObjectList[i]->renderer() != nullptr) {
			//用材质对应的渲染函数渲染renderer
			gameObjectList[i]->renderer()->_material->callRenderFunction(gameObjectList[i]->renderer()->_material->program(),gameObjectList[i], app);
		}
	}
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

void RenderDepthMap(GLuint program, std::shared_ptr<Arc_Engine::ArcGameObject> inst, std::shared_ptr<Arc_Engine::ArcApplication> app) {
	std::shared_ptr<Arc_Engine::ArcRenderer> renderer = inst->renderer();

	glUseProgram(program);

	GLint lightSpaceMatrixMatLocation = glGetUniformLocation(program, "lightSpaceMatrix");
	glUniformMatrix4fv(lightSpaceMatrixMatLocation, 1, GL_FALSE, glm::value_ptr(app->scene()->light()->lightProjection() * app->scene()->light()->lightView()));
	
	GLint modelMatLocation = glGetUniformLocation(program, "model");
	glUniformMatrix4fv(modelMatLocation, 1, GL_FALSE, glm::value_ptr(inst->transform().transformMatrix()));

	glBindVertexArray(renderer->vao);
	glDrawArrays(renderer->drawType, renderer->drawStart, renderer->drawCount);

	//unbind everything
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void RenderPostEffectMap(GLuint program, std::shared_ptr<Arc_Engine::ArcGameObject> inst, std::shared_ptr<Arc_Engine::ArcApplication> app) {
	std::shared_ptr<Arc_Engine::ArcRenderer> renderer = inst->renderer();

	glUseProgram(program);

	GLint lightSpaceMatrixMatLocation = glGetUniformLocation(program, "lightSpaceMatrix");
	glUniformMatrix4fv(lightSpaceMatrixMatLocation, 1, GL_FALSE, glm::value_ptr(app->mainCamera()->matrix()));

	GLint modelMatLocation = glGetUniformLocation(program, "model");
	glUniformMatrix4fv(modelMatLocation, 1, GL_FALSE, glm::value_ptr(inst->transform().transformMatrix()));

	glBindVertexArray(renderer->vao);
	glDrawArrays(renderer->drawType, renderer->drawStart, renderer->drawCount);

	//unbind everything
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void renderDepthToFBO(GLuint fbo) {
	glViewport(0, 0, WIDTH, HEIGHT);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClear(GL_DEPTH_BUFFER_BIT);
	//渲染深度图前将正面剔除掉 只使用背面渲染阴影
	glCullFace(GL_FRONT);
	auto gameObjectList = app->scene()->getGameObjectsInScene();
	for (int i = 0; i < gameObjectList.size(); i++) {
		if (gameObjectList[i]->renderer() != nullptr) {
			RenderDepthMap(simpleDepthShaderProgram, gameObjectList[i], app);
		}
	}
	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setScreenQuadRenderer() {
	std::vector<Arc_Engine::layoutStruct> screenQuadLayoutVector;
	screenQuadLayoutVector.reserve(2);
	screenQuadLayoutVector.push_back(Arc_Engine::layoutStruct(0, 3, 5, 0, sizeof(GLfloat)));
	screenQuadLayoutVector.push_back(Arc_Engine::layoutStruct(1, 2, 5, 3, sizeof(GLfloat)));

	std::string currentPath = Arc_Engine::ArcTools::getCurrentPath();

	postEffectMaterial = std::make_shared<Arc_Engine::ArcMaterial>(currentPath + shader_path + quad_vert_shader_path, currentPath + shader_path + quad_frag_shader_path);
	postEffectMaterial->setRenderFunction(renderScreenQuad);
	
	screenQuadRenderer = std::make_shared<Arc_Engine::ArcRenderer>(postEffectMaterial, sizeof(screenVertices), screenVertices, app->scene()->postEffectMap(), screenQuadLayoutVector);
}

void renderToScreenQuad() {

}

void renderScreenQuad(GLuint program, std::shared_ptr<Arc_Engine::ArcGameObject> inst, std::shared_ptr<Arc_Engine::ArcApplication> app) {
	//下一步写这里

}
