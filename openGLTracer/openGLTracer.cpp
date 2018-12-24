// openGLTracer.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "openGLTracer.h"
#include "Swb_MeshLoader.h"

// Window dimensions    
const GLuint WIDTH = 800, HEIGHT = 600;

// The MAIN function, from here we start the application and run the game loop    
int main()
{
	std::cout << "Starting GLFW context, OpenGL 4.0" << std::endl;
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

	currentPath = getCurrentPath();

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
	
	//加载资源
	loadImage(currentPath + pushable_box_path, &pushableBoxTexture);
	loadImage(currentPath + wall_path, &wallTexture);
	loadImage(currentPath + aim_path, &aimTexture);
	loadImage(currentPath + player_path, &playerTexture);
	//testModel = &Swb_Model();
	//loadModel();
	loadShader("\\hw2\\hw2.vert", "\\hw2\\hw2.frag");
	
	//将加载的资源装在到预制体里
	woodenCrate = std::make_shared<Arc_Engine::ArcRenderer>(shaderProgram, sizeof(vertexData), vertexData, pushableBoxTexture);
	wall = std::make_shared<Arc_Engine::ArcRenderer>(shaderProgram, sizeof(vertexData), vertexData, wallTexture);
	aim = std::make_shared<Arc_Engine::ArcRenderer>(shaderProgram, sizeof(vertexData), vertexData, aimTexture);
	player = std::make_shared<Arc_Engine::ArcRenderer>(shaderProgram, sizeof(vertexData), vertexData, playerTexture);

	auto mainScene = std::make_shared<Arc_Engine::ArcScene>();
	
	auto cur_directionLight = std::make_shared <Arc_Engine::DirectionLight>();
	cur_directionLight->setAmbient(glm::vec3(0.5f, 0.5f, 0.5f));
	cur_directionLight->setDiffuse(glm::vec3(0.6f, 0.6f, 0.6f));
	cur_directionLight->setDirection(glm::vec3(0.5f, 0.8f, 0.0f));
	mainScene->setLight(cur_directionLight);
	//将预制体拖到场景里
	CreateInstances();

	std::list<Arc_Engine::ArcGameObject>::const_iterator itScene;//start
	for (itScene = gInstances.begin(); itScene != gInstances.end(); ++itScene) {
		mainScene->addGameObject(std::make_shared<Arc_Engine::ArcGameObject>(*itScene));
	}

	app = std::make_unique<Arc_Engine::ArcApplication>(mainScene);

	Arc_Engine::ArcGameObject::setGameObjectList(&gInstances);
	
	auto gameObjectList = mainScene->getGameObjectsInScene();
	
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
	//glfwSetScrollCallback(window, OnScroll);
	// Game loop
	float lastTime = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions  
		glfwPollEvents();

		float thisTime = glfwGetTime();
		deltaTime = thisTime - lastTime;
		Arc_Engine::ArcTime::setDeltaTime(deltaTime);
		Update(deltaTime);//update函数
		lastTime = thisTime;

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

void loadShader(std::string vertexShaderPath, std::string fragmentShaderPath) {
	std::ifstream v(currentPath + shader_path + vertexShaderPath);
	std::string vertBuffer((std::istreambuf_iterator<char>(v)), std::istreambuf_iterator<char>());
	std::ifstream f(currentPath + shader_path + fragmentShaderPath);
	std::string fragBuffer((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

	const GLchar* vertCharArray = vertBuffer.c_str();
	const GLchar* fragCharArray = fragBuffer.c_str();
	const GLint vertCharArrayLength = vertBuffer.size();
	const GLint fragCharArrayLength = fragBuffer.size();

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertexShader, 1, &vertCharArray, &vertCharArrayLength);
	glShaderSource(fragmentShader, 1, &fragCharArray, &fragCharArrayLength);

	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	glLinkProgram(shaderProgram);
}

void setupBuffer() {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);//vert
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), NULL);

	glEnableVertexAttribArray(1);//uv
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

	glEnableVertexAttribArray(2);//normal
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void loadImage(std::string texturePath, GLuint* tempTexture) {
	//std::cout << texture_path.c_str() << std::endl;
	FREE_IMAGE_FORMAT fifmt = FreeImage_GetFileType(texturePath.c_str(), 0);
	if (fifmt == FIF_UNKNOWN) {
		fifmt = FreeImage_GetFIFFromFilename(texturePath.c_str());
	}
	if (fifmt == FIF_UNKNOWN) {
		std::cerr << "ERROR: Failed to determine the filetype for " << texturePath.c_str() << std::endl;
		return;
	}
	std::cout << texturePath.c_str() << std::endl;
	//2 加载图片
	FIBITMAP *dib = FreeImage_Load(fifmt, texturePath.c_str(), 0);
	if (!dib) {
		std::cerr << "ERROR: Failed to read in the texture from - " << texturePath.c_str() << std::endl;
		return;
	}

	//3 转化为rgb 24色
	dib = FreeImage_ConvertTo24Bits(dib);

	//4 获取数据指针
	BYTE *pixels = (BYTE*)FreeImage_GetBits(dib);

	int width = FreeImage_GetWidth(dib);
	int height = FreeImage_GetHeight(dib);
	/**
	* 产生一个纹理Id,可以认为是纹理句柄，后面的操作将书用这个纹理id
	*/
	glGenTextures(1, tempTexture);

	/**
	* 使用这个纹理id,或者叫绑定(关联)
	*/
	glBindTexture(GL_TEXTURE_2D, *tempTexture);
	/**
	* 指定纹理的放大,缩小滤波，使用线性方式，即当图片放大的时候插值方式
	*/
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	/**
	* 将图片的rgb数据上传给opengl.
	*/
	glTexImage2D(
		GL_TEXTURE_2D, //! 指定是二维图片
		0, //! 指定为第一级别，纹理可以做mipmap,即lod,离近的就采用级别大的，远则使用较小的纹理
		GL_RGB, //! 纹理的使用的存储格式
		width, //! 宽度，老一点的显卡，不支持不规则的纹理，即宽度和高度不是2^n。
		height, //! 宽度，老一点的显卡，不支持不规则的纹理，即宽度和高度不是2^n。
		0, //! 是否的边
		GL_BGR_EXT, //! 数据的格式，bmp中，windows,操作系统中存储的数据是bgr格式
		GL_UNSIGNED_BYTE, //! 数据是8bit数据
		pixels
	);

	//std::cout << dib << std::endl;
	/**
	* 释放内存
	*/
	FreeImage_Unload(dib);
}

void draw() {
	// Clear the colorbuffer    
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	std::list<Arc_Engine::ArcGameObject>::const_iterator it;//画所有模型
	for (it = gInstances.begin(); it != gInstances.end(); ++it) {
		if (it->renderer() != nullptr) {
			RenderInstance(*it);
		}
	}
}

void RenderInstance(const Arc_Engine::ArcGameObject& inst) {
	std::shared_ptr<Arc_Engine::ArcRenderer> renderer = inst.renderer();
	//tdogl::Program* shaders = asset->shaders;
	GLuint program = renderer->program;

	//bind the shaders
	glUseProgram(program);

	GLint cameraMatLocation = glGetUniformLocation(shaderProgram, "camera");
	glUniformMatrix4fv(cameraMatLocation, 1, GL_FALSE, glm::value_ptr(mainCamera->matrix()));

	GLint modelMatLocation = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(modelMatLocation, 1, GL_FALSE, glm::value_ptr(inst.transform().transformMatrix()));

	GLint lightAmbientLoc = glGetUniformLocation(shaderProgram, "light.ambient");
	GLint lightDiffuseLoc = glGetUniformLocation(shaderProgram, "light.diffuse");
	GLint lightDirLoc = glGetUniformLocation(shaderProgram, "light.direction");

	auto mainSceneLight = app->scene()->light();

	glUniform3f(lightAmbientLoc, mainSceneLight->ambient().x, mainSceneLight->ambient().y, mainSceneLight->ambient().z);
	glUniform3f(lightDiffuseLoc, mainSceneLight->diffuse().x, mainSceneLight->diffuse().y, mainSceneLight->diffuse().z);
	glUniform3f(lightDirLoc, mainSceneLight->direction().x, mainSceneLight->direction().y, mainSceneLight->direction().z); // 方向光源

	// 设置材料光照属性
	GLint texLocation = glGetUniformLocation(shaderProgram, "U_MainTexture");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderer->texture);
	glUniform1i(texLocation, 0);

	//bind VAO and draw
	glBindVertexArray(renderer->vao);
	glDrawArrays(renderer->drawType, renderer->drawStart, renderer->drawCount);

	//unbind everything
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void loadModel() {
	Swb_MeshLoader::loadMesh(currentPath + model_path, testModel);
	//std::cout << testModel->vSets.size()<< " "<< testModel->fSets.size() << std::endl;
}

void Update(float secondsElapsed) {
	//treatKeyboardInput(secondsElapsed);
	//treatMouseInput(secondsElapsed);
}

void treatKeyboardInput(float secondsElapsed) {
	const float moveSpeed = 2.0; //units per second
	//auto mainCameraGameObject = *(mainCamera->gameObject());
	//auto mainCameraTransform = *(mainCameraGameObject.transformPtr());
	auto mainCameraTransformPtr = mainCamera->gameObject()->transformPtr();
	//std::cout << "start z3: " << (((mainCamera->gameObject())->transform)->position()).z << std::endl;
	if (glfwGetKey(window, 'S')) {
		mainCameraTransformPtr->translate(secondsElapsed * moveSpeed * -mainCameraTransformPtr->forward());
	}
	else if (glfwGetKey(window, 'W')) {
		mainCameraTransformPtr->translate(secondsElapsed * moveSpeed * mainCameraTransformPtr->forward());
	}
	if (glfwGetKey(window, 'A')) {
		mainCameraTransformPtr->translate(secondsElapsed * moveSpeed * -mainCameraTransformPtr->right());
	}
	else if (glfwGetKey(window, 'D')) {
		mainCameraTransformPtr->translate(secondsElapsed * moveSpeed * mainCameraTransformPtr->right());
	}
}

void treatMouseInput(float secondsElapsed) {
	
	auto mainCameraTransformPtr = mainCamera->gameObject()->transformPtr();
	//鼠标位置
	const float mouseSensitivity = 0.1f;
	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);
	mainCameraTransformPtr->rotate(mouseSensitivity * (float)mouseY, mouseSensitivity * (float)mouseX, 0.0f);
	glfwSetCursorPos(window, 0, 0);

	//滚轮
	const float zoomSensitivity = -0.2f;
	float fieldOfView = mainCamera->fieldOfView() + zoomSensitivity * (float)gScrollY;
	if (fieldOfView < 5.0f) fieldOfView = 5.0f;
	if (fieldOfView > 130.0f) fieldOfView = 130.0f;
	mainCamera->setFieldOfView(fieldOfView);
	gScrollY = 0;
}

void OnScroll(GLFWwindow* window, double deltaX, double deltaY) {
	gScrollY += deltaY;
}

void LoadRenderer(std::shared_ptr<Arc_Engine::ArcRenderer> renderer, GLuint shaderProgram, size_t vertexDataSize, GLfloat *vertexData, GLuint texture) {
	renderer->program = shaderProgram;
	renderer->drawType = GL_TRIANGLES;
	renderer->drawStart = 0;
	renderer->drawCount = 6 * 2 * 3;
	renderer->texture = texture;

	glGenBuffers(1, &renderer->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);

	glGenVertexArrays(1, &renderer->vao);
	glBindVertexArray(renderer->vao);

	glBufferData(GL_ARRAY_BUFFER, vertexDataSize, vertexData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);//vert
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), NULL);

	glEnableVertexAttribArray(1);//uv
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

	glEnableVertexAttribArray(2);//normal
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void CreateInstances() {

	gInstances.push_back(Arc_Engine::ArcGameObject());
	std::list<Arc_Engine::ArcGameObject>::iterator it = gInstances.begin();
	auto tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(2.5, 7, 1));
	//tempTransform.lookAt(glm::vec3(0.5f, 0, 5.0f));
	tempTransform.setRotation(glm::vec3(75, 180, 0));
	it->setName("Camera");
	it->setTransfrom(tempTransform);
	mainCamera = std::make_shared<Arc_Engine::Camera>();
	mainCamera->setViewportAspectRatio(WIDTH / HEIGHT);
	it->attachScript(mainCamera);
	auto gameController = std::make_shared<GameController>();
	it->attachScript(gameController);
	
	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("DownCube1");
	it->setRenderer(wall);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(0.5f, 0, 5.0f));
	tempTransform.setScale(glm::vec3(2, 1, 3));
	it->setTransfrom(tempTransform);
	std::shared_ptr<TestScript> testScript = std::make_shared<TestScript>();
	it->attachScript(testScript);
	//std::cout << (dynamic_cast<PushableBox*>(&*testScript)==nullptr)<< std::endl;


	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("DownCube2");
	it->setRenderer(wall);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(3.5, 0, 5.5));
	tempTransform.setScale(glm::vec3(4, 1, 2));
	it->setTransfrom(tempTransform);
	std::shared_ptr<PushableBox> testScript2 = std::make_shared<PushableBox>();
	//i.ArcBehaviourList.push_back(testScript2);
	
	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("DownCube3");
	it->setRenderer(wall);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(4, 0, 2));
	tempTransform.setScale(glm::vec3(3, 1, 5));
	it->setTransfrom(tempTransform);

	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("DownCube4");
	it->setRenderer(wall);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(1, 0, 1));
	tempTransform.setScale(glm::vec3(3, 1, 3));
	it->setTransfrom(tempTransform);

	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("DownCube5");
	it->setRenderer(wall);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(0, 0, 3));
	tempTransform.setScale(glm::vec3(1, 1, 1));
	it->setTransfrom(tempTransform);

	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("UpCube1");
	it->setRenderer(wall);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(5, 1, 5));
	tempTransform.setScale(glm::vec3(1, 1, 3));
	it->setTransfrom(tempTransform);

	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("UpCube2");
	it->setRenderer(wall);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(0, 1, 3));
	tempTransform.setScale(glm::vec3(1, 1, 7));
	it->setTransfrom(tempTransform);

	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("UpCube3");
	it->setRenderer(wall);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(1.5, 1, 6));
	tempTransform.setScale(glm::vec3(2, 1, 1));
	it->setTransfrom(tempTransform);

	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("UpCube4");
	it->setRenderer(wall);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(3.5, 1, 5.5));
	tempTransform.setScale(glm::vec3(2, 1, 2));
	it->setTransfrom(tempTransform);

	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("UpCube5");
	it->setRenderer(wall);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(4, 1, 3.5));
	tempTransform.setScale(glm::vec3(1, 1, 2));
	it->setTransfrom(tempTransform);

	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("UpCube6");
	it->setRenderer(wall);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(5, 1, 2));
	tempTransform.setScale(glm::vec3(1, 1, 3));
	it->setTransfrom(tempTransform);

	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("UpCube7");
	it->setRenderer(wall);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(3, 1, 0));
	tempTransform.setScale(glm::vec3(5, 1, 1));
	it->setTransfrom(tempTransform);

	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("Target1");
	it->setRenderer(aim);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(2, 0, 4));
	it->setTransfrom(tempTransform);

	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("Target2");
	it->setRenderer(aim);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(2, 0, 3));
	it->setTransfrom(tempTransform);

	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("Target3");
	it->setRenderer(aim);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(1, 0, 3));
	it->setTransfrom(tempTransform);

	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("PushableBox1");
	it->setRenderer(woodenCrate);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(3, 1, 3));
	it->setTransfrom(tempTransform);
	auto movableCube1 = std::make_shared<MovableCube>();
	movableCube1->setCoord(glm::vec2(3.0f, 3.0f));
	it->attachScript(movableCube1);

	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("PushableBox2");
	it->setRenderer(woodenCrate);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(3, 1, 2));
	it->setTransfrom(tempTransform);
	auto movableCube2 = std::make_shared<MovableCube>();
	movableCube2->setCoord(glm::vec2(3.0f, 2.0f));
	it->attachScript(movableCube2);
	
	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("PushableBox3");
	it->setRenderer(woodenCrate);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(2, 1, 2));
	it->setTransfrom(tempTransform);
	auto movableCube3 = std::make_shared<MovableCube>();
	movableCube3->setCoord(glm::vec2(2.0f, 2.0f));
	it->attachScript(movableCube3);

	gInstances.push_back(Arc_Engine::ArcGameObject());
	it++;
	it->setName("Player");
	it->setRenderer(player);
	tempTransform = Arc_Engine::ArcTransform();
	tempTransform.setPosition(glm::vec3(1, 1.5, 4));
	tempTransform.setScale(glm::vec3(1, 2, 1));
	it->setTransfrom(tempTransform);
	auto player = std::make_shared<Player>();
	player->setCoord(glm::vec2(1.0f,4.0f));
	it->attachScript(player);

	//std::cout << ((findGameObjectsOfType<TestScript>()[0])->getAComponent<TestScript>()) << std::endl;
	//std::cout << findGameObjectsOfType<TestScript>().size() << std::endl;
	//std::cout << findGameObjectsOfType<TestScript>()[0]->name()<< std::endl;
}

void CreateSingleInstance(std::list<Arc_Engine::ArcGameObject>::iterator it,std::string name, Arc_Engine::ArcRenderer renderer, Arc_Engine::ArcTransform transform, std::vector<std::shared_ptr<Arc_Engine::ArcBehaviour>> ArcBehavoiurList) {
	it->setName(name);
	it->setRenderer(std::make_shared<Arc_Engine::ArcRenderer>(renderer));
	it->setTransfrom(transform);
	for (int i = 0; i < ArcBehavoiurList.size(); i++) {
		it->attachScript(ArcBehavoiurList[i]);
	}
}

// convenience function that returns a translation matrix
glm::mat4 translate(GLfloat x, GLfloat y, GLfloat z) {
	return glm::translate(glm::mat4(), glm::vec3(x, y, z));
}


// convenience function that returns a scaling matrix
glm::mat4 scale(GLfloat x, GLfloat y, GLfloat z) {
	return glm::scale(glm::mat4(), glm::vec3(x, y, z));
}

glm::mat4 rotate(GLfloat x, GLfloat y, GLfloat z, GLfloat angle) {
	return glm::rotate(glm::mat4(), angle, glm::vec3(x, y, z));
}

void PrintUpdate() {
	std::cout << "Update" << std::endl;
}

std::string getCurrentPath() {
	char buf[80];
	_getcwd(buf, sizeof(buf)); //将当前工作目录的绝对路径复制到参数buffer所指的内存空间中
	std::string path1 = std::string(buf);
	std::size_t found = path1.find_last_of("/\\");
	return path1.substr(0, found);
}

void registerScene1(std::shared_ptr<Arc_Engine::ArcScene> scene) {

}
