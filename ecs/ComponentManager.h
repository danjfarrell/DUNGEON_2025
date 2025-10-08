#pragma once
#include "ComponentArray.h"
#include <unordered_map>
#include <typeindex>
#include <any>

class ComponentManager {

private:
	std::unordered_map<std::type_index, std::any> component_arrays;

	template<typename T>
	ComponentArray<T>* get_component_array() {

	std::type_index type = std::type_index(typeid(T));

	if (component_arrays.find(type) == component_arrays.end()) {
		component_arrays[type] = std::make_any<ComponentArray<T>>();
	}

	return std::any_cast<ComponentArray<T>>(&component_arrays[type]);
	}
public:
	template<typename T>
	void add_component(Entity entity, T component) {
		get_component_array<T>()->insert(entity, component);
	}
	template<typename T>
	void remove_component(Entity entity) {
		get_component_array<T>()->remove(entity);
	}
	template<typename T>
	T* get_component(Entity entity) {
		return get_component_array<T>()->get(entity);
	}
	template<typename T>
	bool has_component(Entity entity) {
		return get_component_array<T>()->has(entity);
	}
	template<typename T>
	ComponentArray<T>* get_array() {
		return get_component_array<T>();
	}
};