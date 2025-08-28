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
#include <gtc/random.hpp>（
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
//点光源
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
//场景
struct SceneObjectUbo
{
    glm::mat4 model;
};
//相机
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

//情况2：若抛弃函数返回值，让编译器发出警告
#elif defined VK_RESULT_NODISCARD
struct [[nodiscard]] result_t {
    VkResult result;
    result_t(VkResult result) :result(result) {}
    operator VkResult() const { return result; }
};
//在本文件中关闭弃值提醒（因为我懒得做处理）
#pragma warning(disable:4834)
#pragma warning(disable:6031)

//情况3：啥都不干
#else
using result_t = VkResult;
#endif

#define ExecuteOnce(...) { static bool executed = false; if (executed) return __VA_ARGS__; executed = true; }
template<typename T>
class arrayRef {
    T* const pArray = nullptr;
    size_t count = 0;
public:
    //从空参数构造，count为0
    arrayRef() = default;
    //从单个对象构造，count为1
    arrayRef(T& data) :pArray(&data), count(1) {}
    //从顶级数组构造
    template<size_t elementCount>
    arrayRef(T(&data)[elementCount]) : pArray(data), count(elementCount) {}
    //从指针和元素个数构造
    arrayRef(T* pData, size_t elementCount) :pArray(pData), count(elementCount) {}
    //复制构造，若T带const修饰，兼容从对应的无const修饰版本的arrayRef构造
    //24.01.07 修正因复制粘贴产生的typo：从pArray(&other)改为pArray(other.Pointer())
    arrayRef(const arrayRef<std::remove_const_t<T>>& other) :pArray(other.Pointer()), count(other.Count()) {}
    //Getter
    T* Pointer() const { return pArray; }
    size_t Count() const { return count; }
    //Const Function
    T& operator[](size_t index) const { return pArray[index]; }
    T* begin() const { return pArray; }
    T* end() const { return pArray + count; }
    //Non-const Function
    //禁止复制/移动赋值
    arrayRef& operator=(const arrayRef&) = delete;
};

inline auto& outStream = std::cout;

template <typename T>
bool Between_Closed(T lower, T value, T upper) {
    return (value >= lower && value <= upper);
}