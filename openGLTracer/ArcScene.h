#pragma once
#include "common.h"
#include "ArcGameObject.h"
#include "LightBundle.h"
#include "ArcTextureLoader.h"

namespace Arc_Engine {

	class ArcScene: public std::enable_shared_from_this<ArcScene>
	{
	public:
		std::vector<std::shared_ptr<ArcGameObject>> getGameObjectsInScene();
		const std::shared_ptr<Arc_Engine::DirectionLight> light() const;
		void setLight(std::shared_ptr<Arc_Engine::DirectionLight> light); //之后把light改成基类？
		const GLuint depthMap() const;
		const GLuint depthMapFBO() const;
		void addGameObject(std::shared_ptr<Arc_Engine::ArcGameObject> gameObject);
		void enableShadow();//开启阴影，计算出_depthMapFBO和_depthMap
		void createShadowBuffer(GLuint* depthMapFBO, GLuint depthMap);


	private:
		std::vector<std::shared_ptr<ArcGameObject>> _sceneGameObjectObjects;
		std::shared_ptr<Arc_Engine::DirectionLight> _light;
		GLuint _depthMapFBO;
		GLuint _depthMap;
	};
}

