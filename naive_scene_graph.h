#pragma once

#include <vector>
#include <memory>
#include <cstring>
#include <span>
#include <algorithm>
#include "transform.h"

constexpr size_t local_storage_size = 8;
constexpr uint32_t EMPTY = -1;

template <typename T>
struct Iterator
{
	using iterator_category = std::forward_iterator_tag;
	using difference_type = std::ptrdiff_t;
	using value_type = T;
	using pointer = T *;   // or also value_type*
	using reference = T &; // or also value_type&
	Iterator(pointer ptr) : _ptr(ptr) {}

	reference operator*() const { return *_ptr; }
	pointer operator->() { return _ptr; }

	// Prefix increment
	Iterator &operator++()
	{
		_ptr++;
		return *this;
	}

	// Postfix increment
	Iterator operator++(int)
	{
		Iterator tmp = *this;
		++(*this);
		return tmp;
	}

	friend bool operator==(const Iterator &a, const Iterator &b) { return a._ptr == b._ptr; };
	friend bool operator!=(const Iterator &a, const Iterator &b) { return a._ptr != b._ptr; };

  private:
	pointer _ptr;
};

template <typename T>
class small_vector{
public:
	small_vector(){}
	small_vector(small_vector& other)
		:fallback_array(other.fallback_array), 
		use_fallback(other.use_fallback), 
		_size(other._size){
		std::memcpy(main_array, other.main_array, 8*sizeof(T));
	};
	small_vector(small_vector&& other)
		:fallback_array(std::move(other.fallback_array)), 
		use_fallback(other.use_fallback), 
		_size(other._size){
		std::memcpy(main_array, other.main_array, 8*sizeof(T));
	}
	small_vector& operator=(small_vector&& other) = default;
	~small_vector(){}
	
	T &operator[](std::size_t id)
	{
		if (use_fallback)
			return fallback_array[id];
		else
			return main_array[id];
	}
	void insert(T x)
	{
		if (use_fallback)
		{
			fallback_array.push_back(x);
			++_size;
			return;
		}
		if (_size == 8)
		{
			fallback_array.assign(main_array, main_array + _size);
			fallback_array.push_back(x);
			++_size;
			use_fallback = true;
		}
		else
		{
			main_array[_size] = x;
			++_size;
		}
	}
	void insert(std::span<T> x)
	{
		if (use_fallback)
		{
			fallback_array.insert(fallback_array.end(), x.begin(), x.end());
			_size += x.size();
			return;
		}
		if ((_size + x.size()) > 8)
		{
			fallback_array.assign(main_array, main_array + _size);
			fallback_array.insert(fallback_array.end(), x.begin(), x.end());
			_size += x.size();
			use_fallback = true;
		}
		else
		{
			for (size_t i = 0; i < x.size(); ++i)
			{
				main_array[_size + i] = x[i];
			}
			_size += x.size();
		}
	}
	size_t size()
	{
		return _size;
	}

	Iterator<T> begin()
	{
		if (use_fallback)
			return Iterator(&fallback_array.front());
		return Iterator(&main_array[0]);
	}
	Iterator<T> end()
	{
		if (use_fallback)
			return Iterator(&fallback_array.back() + 1);
		return Iterator(&main_array[0] + _size);
	}
	void remove(T x){
		int l = 0;
		if(use_fallback){
			fallback_array.erase(std::remove(fallback_array.begin(), fallback_array.end(), x), fallback_array.end());
			_size = fallback_array.size();
		}		
		else{
			for(uint32_t i = 0; i < _size; ++i)
				if(main_array[i] != x)
	        		main_array[l++] = main_array[i];   
			_size = l;     
			//for(; l < _size; ++l)
				//main_array[l] = T{};
		}
	}

private:
	T main_array[8] = {};
	std::vector<T> fallback_array;
	bool use_fallback = false;
	uint32_t _size = 0;
};

struct NodeBase
{
	small_vector<NodeBase *> children;
	NodeBase *parent;
    NodeBase& operator=(NodeBase&& other) = default;
	Transform transform;
    NodeBase(){
        parent = nullptr;
    }
    NodeBase(NodeBase& other)
        :children(),
		parent(other.parent){

    }
	NodeBase(NodeBase&& other)
		:children(std::move(other.children)),
		parent(other.parent){
	}
	void visit(const glm::mat4& parent_model_matrix){
		glm::mat4 model_mat = calcGlobalModelMat(parent_model_matrix, transform);
		for(int i = 0; i < children.size(); ++i){
			children[i]->visit(model_mat);
		}
	}
};


class NaiveSceneGraph{
public:
    NodeBase* root_node;
    NaiveSceneGraph(){
        root_node = new NodeBase();
        root_node->parent = nullptr;
    }
    NaiveSceneGraph(NaiveSceneGraph&& other){
        root_node = nullptr;
        std::swap(root_node, other.root_node);
    }
    NodeBase* create_node_with_parent(NodeBase* node, NodeBase* parent){
        if(parent){
            NodeBase* new_node = new NodeBase(*node);
            new_node->parent = parent;
            parent->children.insert(new_node);
            return new_node;
        }
        return nullptr;
    }
    void get_all_leaf_nodes(std::vector<NodeBase*>& leaves){
        leaves.clear();
		if(root_node->children.size() == 0){
			leaves.push_back(root_node);
			return;
		}
        for(auto child : root_node->children){
			get_all_leaf_nodes_helper(leaves, child);
		}
	}
    void get_all_leaf_nodes_helper(std::vector<NodeBase*>& leaves, NodeBase* node){
		if(node->children.size() == 0){
			leaves.push_back(node);
			return;
		}
		for(auto child : node->children){
			get_all_leaf_nodes_helper(leaves, child);
		}
	}

    void remove(NodeBase* node){
		for(auto child : node->children){
			std::cout << "error\n";
            remove_child(child);
		}
		NodeBase* parent = node->parent;
		if(parent){
			parent->children.remove(node);
		}
        delete node;
    }

	
	void remove_child(NodeBase* node){;
		for(auto child : node->children){
			remove_child(child);
		}
        delete node;
	}
    uint32_t get_total_nodes(){
		uint32_t sum{0};
        if(root_node)
            ++sum;
        for(auto child : root_node->children){
			sum++;
            get_total_nodes_helper(child, sum);
		}
		return sum;
	}

    void get_total_nodes_helper(NodeBase* node, uint32_t& sum){
        for(auto child : node->children){
			++sum;
            get_total_nodes_helper(child, sum);
		}
    }
	void transform_visit(){
		root_node->visit(root_node->transform.model_mat);
	}
};