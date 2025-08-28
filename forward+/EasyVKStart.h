#pragma once

#include<iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stack>
#include <map>
#include <unordered_map>
#include <span>
#include <memory>
#include <functional>
#include <concepts>
#include <format>
#include <chrono>
#include <numeric>
#include <numbers>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm.hpp>
#include<gtc/matrix_transform.hpp>
#include <gtc/random.hpp>��
#include"stb_image.h"
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
#pragma comment (lib,"vulkan-1.lib")
#endif
#include <vulkan/vulkan.h>
#include"util.h"
#include"tiny_obj_loader.h"











const int MAX_POINT_LIGHT_COUNT = 20000; //TODO: change it back smaller
//const int MAX_POINT_LIGHT_PER_TILE = 63;
const int MAX_POINT_LIGHT_PER_TILE = 1023;
// const int TILE_SIZE = 16;
const int TILE_SIZE = 16;

int tile_count_per_row;
int tile_count_per_col;

glm::mat4 view_matrix;
glm::vec3 cam_pos; 
int debug_view_index = 0;
//���Դ
struct PointLight
{
public:
    glm::vec3 pos;
    float radius = { 5.0f };
    glm::vec3 intensity = { 1.0f, 1.0f, 1.0f };
    float padding;
    PointLight() {}
    PointLight(glm::vec3 pos, float radius, glm::vec3 intensity)
        : pos(pos), radius(radius), intensity(intensity)
    {
    };
};
//����
struct SceneObjectUbo
{
    glm::mat4 model;
};
//���
struct CameraUbo
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 projview;
    glm::vec3 cam_pos;
};

struct PushConstantObject
{
    glm::ivec2 viewport_size;
    glm::ivec2 tile_nums;
    int debugview_index; // TODO: separate this and only have it in debug mode?

    PushConstantObject(int viewport_size_x, int viewport_size_y, int tile_num_x, int tile_num_y, int debugview_index = 0)
        : viewport_size(viewport_size_x, viewport_size_y),
        tile_nums(tile_num_x, tile_num_y),
        debugview_index(debugview_index)
    {
    }
};




#ifdef VK_RESULT_THROW

class result_t {
    VkResult result;
public:
    static void(*callback_throw)(VkResult);
    result_t(VkResult result) :result(result) {}
    result_t(result_t&& other) noexcept :result(other.result) { other.result = VK_SUCCESS; }
    ~result_t() noexcept(false) {
        if (uint32_t(result) < VK_RESULT_MAX_ENUM)
            return;
        if (callback_throw)
            callback_throw(result);
        throw result;
    }
    operator VkResult() {
        VkResult result = this->result;
        this->result = VK_SUCCESS;
        return result;
    }
};
inline void(*result_t::callback_throw)(VkResult);

//���2����������������ֵ���ñ�������������
#elif defined VK_RESULT_NODISCARD
struct [[nodiscard]] result_t {
    VkResult result;
    result_t(VkResult result) :result(result) {}
    operator VkResult() const { return result; }
};
//�ڱ��ļ��йر���ֵ���ѣ���Ϊ������������
#pragma warning(disable:4834)
#pragma warning(disable:6031)

//���3��ɶ������
#else
using result_t = VkResult;
#endif

#define ExecuteOnce(...) { static bool executed = false; if (executed) return __VA_ARGS__; executed = true; }
template<typename T>
class arrayRef {
    T* const pArray = nullptr;
    size_t count = 0;
public:
    //�ӿղ������죬countΪ0
    arrayRef() = default;
    //�ӵ��������죬countΪ1
    arrayRef(T& data) :pArray(&data), count(1) {}
    //�Ӷ������鹹��
    template<size_t elementCount>
    arrayRef(T(&data)[elementCount]) : pArray(data), count(elementCount) {}
    //��ָ���Ԫ�ظ�������
    arrayRef(T* pData, size_t elementCount) :pArray(pData), count(elementCount) {}
    //���ƹ��죬��T��const���Σ����ݴӶ�Ӧ����const���ΰ汾��arrayRef����
    //24.01.07 ��������ճ��������typo����pArray(&other)��ΪpArray(other.Pointer())
    arrayRef(const arrayRef<std::remove_const_t<T>>& other) :pArray(other.Pointer()), count(other.Count()) {}
    //Getter
    T* Pointer() const { return pArray; }
    size_t Count() const { return count; }
    //Const Function
    T& operator[](size_t index) const { return pArray[index]; }
    T* begin() const { return pArray; }
    T* end() const { return pArray + count; }
    //Non-const Function
    //��ֹ����/�ƶ���ֵ
    arrayRef& operator=(const arrayRef&) = delete;
};

inline auto& outStream = std::cout;

template <typename T>
bool Between_Closed(T lower, T value, T upper) {
    return (value >= lower && value <= upper);
}