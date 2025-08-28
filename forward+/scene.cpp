// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#include "scene.hpp"

TestSceneConfiguration& getGlobalTestSceneConfiguration()
{
	static TestSceneConfiguration sp;
	return sp;
}
