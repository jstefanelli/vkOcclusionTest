#ifndef VKOCCLUSIONTEST_TRANSFORM_H
#define VKOCCLUSIONTEST_TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform {
private:
	glm::vec3 _position = glm::vec3(0.0);
	glm::quat _orientation = glm::identity<glm::quat>();
	glm::vec3 _scale = glm::vec3(1.0);

	bool _upToDate = true;
	glm::mat4 _model = glm::mat4(1.0);
public:
	const glm::vec3 &position() const;
	void position(const glm::vec3 &position);

	const glm::quat &orientation() const;
	void orientation(const glm::quat &orientation);

	const glm::vec3 &scale() const;
	void scale(const glm::vec3 &scale);

	glm::mat4 model();
};


#endif //VKOCCLUSIONTEST_TRANSFORM_H
