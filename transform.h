#pragma once

#include <GLM/glm.hpp>
#include <GLM/gtc/quaternion.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

struct Transform{
    glm::vec3 position{glm::vec3(1.0f)};
    glm::quat rotation{glm::quat(1.0f, 0.0f, 0.0f, 0.0f)};
    glm::vec3 scale{glm::vec3(1.0f)};
    operator const glm::mat4();

    bool dirty = false;
    glm::mat4 model_mat = glm::mat4(1.0f);
};

glm::mat4 calcLocalModelMat(const Transform& transform){
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, transform.position);
    model = model * glm::mat4_cast(transform.rotation);
    //model = glm::rotate(model, glm::radians(360.0f), object.rotation);
    model = glm::scale(model, transform.scale);
    return model;
}

glm::mat4 calcGlobalModelMat(const Transform& global_transform, Transform& transform){
    transform.dirty = false;
    return transform.model_mat = global_transform.model_mat * calcLocalModelMat(transform); 
}

glm::mat4 calcGlobalModelMat(const glm::mat4& global_model_mat, Transform& transform){
    transform.dirty = false;
    return transform.model_mat = global_model_mat * calcLocalModelMat(transform); 
}

glm::mat3 getNormalMat(const glm::mat4& model){
    return glm::mat3(glm::transpose(glm::inverse(model)));
}

Transform::operator const glm::mat4(){
    return calcGlobalModelMat(Transform(), *this);
};