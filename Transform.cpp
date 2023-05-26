#include "Transform.h"

const glm::vec3 &Transform::position() const {
	return _position;
}

void Transform::position(const glm::vec3 &position) {
	_upToDate = false;
	_position = position;
}

const glm::quat &Transform::orientation() const {
	return _orientation;
}

void Transform::orientation(const glm::quat &orientation) {
	_upToDate = false;
	_orientation = orientation;
}

const glm::vec3 &Transform::scale() const {
	return _scale;
}

void Transform::scale(const glm::vec3 &scale) {
	_upToDate = false;
	_scale = scale;
}

glm::mat4 Transform::model() {
	if (!_upToDate) {
		_model = glm::translate(glm::mat4(1.0), _position);
		_model *= glm::mat4(_orientation);
		_model = glm::scale(_model, _scale);
		_upToDate = true;
	}

	return _model;
}
