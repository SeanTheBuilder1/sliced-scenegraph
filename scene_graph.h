#pragma once

#include <vector>
#include <memory>
#include <cstring>
#include <span>
#include <algorithm>

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

struct NodeHandle
{
	uint32_t slice_index{EMPTY};
	uint32_t internal_index;
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
	NodeHandle self_handle;
	uint32_t depth;
	NodeBase(){
	}
	~NodeBase(){
	}
	
	NodeBase& operator=(NodeBase&& other) = default;
	
	NodeBase(NodeBase&& other)
		:children(std::move(other.children)),
		parent(other.parent),
		self_handle(other.self_handle),
		depth(other.depth){
		other.children.~small_vector<NodeBase *>();
	}
	uint32_t move_to(void *target_address){
		target_address = new(target_address) NodeBase(std::move(*this));
		return sizeof(NodeBase);
	}
};

struct NodeMeta
{
	uint32_t offset{0};
	uint32_t size{0};
	uint32_t next_free{0};
};

class Slice{
private:
	std::unique_ptr<uint8_t[]> data;
	std::span<uint8_t> storage;
	std::span<NodeMeta> node_metadata;
public:
	uint32_t num_nodes{0};
	uint32_t slice_index;
private:
	//uint32_t next_storage_index; //???
	uint32_t next_insert_offset{0};
	uint32_t occupied_space;
	uint32_t first_free {0};

public:
	Slice(size_t node_storage_size, size_t max_num_nodes){
		if(node_storage_size < sizeof(NodeBase))
			node_storage_size = sizeof(NodeBase);
		data = std::make_unique<uint8_t[]>((sizeof(NodeMeta) * max_num_nodes) + (node_storage_size));
		occupied_space = (sizeof(NodeMeta) * max_num_nodes) + (node_storage_size);
		storage = std::span(data.get() + (sizeof(NodeMeta) * max_num_nodes), node_storage_size);
		node_metadata = std::span((NodeMeta *)data.get(), max_num_nodes);
		for(size_t i  = 0; i < node_metadata.size(); ++i){
			node_metadata[i].next_free = i+1;
		}
		node_metadata[node_metadata.size() - 1].next_free = -1;
	}

	Slice(Slice&& other)
	:data(std::move(other.data)), 
	storage(std::move(other.storage)), 
	node_metadata(std::move(other.node_metadata)),
	num_nodes(other.num_nodes),
	slice_index(other.slice_index),
	next_insert_offset(other.next_insert_offset),
	occupied_space(other.occupied_space),
	first_free(other.first_free){
	}

	~Slice(){
		NodeMeta* metadata = &node_metadata[0];
		NodeBase* node = (NodeBase*)&storage[metadata->offset];
		for(auto child : node->children){
			clean(child);
		}
		node->~NodeBase();
		
	}
	
	void clean(NodeBase* node){
		for(auto child : node->children){
			clean(child);
		}
		node->~NodeBase();
	}

	/*template <typename... Args>
	uint32_t insert(Args &&... args)
	{
		auto lmao = NodeBase(args...);
	*/

	void erase(uint32_t node_internal_index);

	void reconstitute(float expansion_factor){
		uint32_t data_size = (sizeof(NodeMeta) * node_metadata.size()) + (storage.size() * expansion_factor);
		auto new_data = std::make_unique<uint8_t[]>(data_size);
		std::span<NodeMeta> new_node_metadata((NodeMeta *)new_data.get(), node_metadata.size());
		std::span<uint8_t> new_storage(
			new_data.get() + (sizeof(NodeMeta) * node_metadata.size()),
			data_size - (sizeof(NodeMeta) * node_metadata.size()));
		std::memcpy(new_node_metadata.data(), node_metadata.data(), node_metadata.size_bytes());
		uint32_t total_offset = 0;
		
		node_metadata = new_node_metadata;
		
		
		NodeMeta* first_metadata = &node_metadata[0];
		NodeBase* first_node = (NodeBase*)&storage[node_metadata[0].offset];
		NodeBase* new_first_node = (NodeBase *)&new_storage[total_offset];
		first_metadata->offset = total_offset;
		total_offset += first_node->move_to(new_first_node);
		first_metadata->size = total_offset;
				
		for (auto& child : new_first_node->children)
		{
			child = make_sorted_node_indices(child, &total_offset, child->self_handle.internal_index, new_storage);
			child->parent = new_first_node; 
		}
		
		
		data = std::move(new_data);
		storage = new_storage;
		occupied_space = data_size;
	}
	void make_sorted_node_indices(std::vector<uint32_t>& indices, NodeBase* node){
		indices.push_back(node->self_handle.internal_index);
		for(auto& child : node->children){
			make_sorted_node_indices(indices, child);
		}
	}
	
	NodeBase* make_sorted_node_indices(NodeBase *node, uint32_t *offset, uint32_t metadata, std::span<uint8_t> &new_storage)
	{
		NodeMeta* meta = &node_metadata[metadata];
		meta->offset = *offset;
		
		NodeBase* new_node = (NodeBase*)&new_storage[*offset];
		*offset += (node)->move_to(new_node);
		for(size_t i = 0; i < new_node->children.size(); ++i){
			new_node->children[i] = make_sorted_node_indices(new_node->children[i], offset, new_node->children[i]->self_handle.internal_index, new_storage);
			new_node->children[i]->parent = new_node;
		}
		return new_node;
	}
	
	NodeHandle insert(NodeBase* node){
		if((sizeof(NodeBase) + (num_nodes*sizeof(NodeBase))) >= storage.size()){
			NodeHandle backup = node->self_handle;
			reconstitute(1.5);
			if(backup.slice_index != EMPTY)
				node = get_node(backup);
		}
		NodeMeta* metadata = &node_metadata[first_free];
		node->self_handle.internal_index = first_free;
		first_free = metadata->next_free;
		metadata->offset = next_insert_offset;
		NodeBase* node_dest = (NodeBase*)&storage[metadata->offset];
		metadata->size = node->move_to(node_dest);
		node_dest->self_handle.slice_index = slice_index;
		next_insert_offset += metadata ->size;
		num_nodes++;
		return node_dest->self_handle;
	}
	void remove(NodeHandle handle){
		NodeMeta* metadata = &node_metadata[handle.internal_index];
		metadata->next_free = first_free;
		first_free = handle.internal_index;
		NodeBase* node = (NodeBase*)&storage[metadata->offset];
		for(auto child : node->children){
			remove_child(child);
		}
		/*
		for(int i = 0; i < node->children.size(); ++i){
			remove_child(node->children[i]);
		}*/
		
		NodeBase* parent = node->parent;
		if(parent){
			parent->children.remove(node);
		}
		node->~NodeBase();	
		
}
	
	void remove_child(NodeBase* node){
		NodeMeta* metadata = &node_metadata[node->self_handle.internal_index];
		metadata->next_free = first_free;
		first_free = node->self_handle.internal_index;
		for(auto child : node->children){
			remove_child(child);
		}
		node->~NodeBase();
	}
	
	void remove(NodeBase* node){
		remove(node->self_handle);
	}
	
	NodeBase* get_node(NodeHandle handle){
		return (NodeBase*)&storage[node_metadata[handle.internal_index].offset];
	}
	
	NodeHandle create_node_with_parent(NodeBase* node, NodeHandle parent){
		NodeHandle handle = insert(node);
		NodeBase* new_node = get_node(handle);
		new_node->parent = get_node(parent);
		get_node(parent)->children.insert(new_node);
		return handle;
	}
	std::vector<NodeHandle> get_all_leaf_nodes(){
		std::vector<NodeHandle> leaves;
		NodeBase* start = (NodeBase*)&storage[node_metadata[0].offset];
		if(start->children.size() == 0){
			leaves.push_back(start->self_handle);
			return leaves;
		}
		for(auto child : start->children){
			get_all_leaf_nodes_helper(leaves, child);
		}
		return leaves;
	}
	void get_all_leaf_nodes_helper(std::vector<NodeHandle>& leaves, NodeBase* node){
		if(node->children.size() == 0){
			leaves.push_back(node->self_handle);
			return;
		}
		for(auto child : node->children){
			get_all_leaf_nodes_helper(leaves, child);
		}
	}
};


class SceneGraph
{
public:
	SceneGraph(){
		root_node.self_handle.slice_index = 0;
	};
	NodeHandle create_node_with_parent(NodeBase* node, NodeHandle parent = NodeHandle{0,0}){
		if(parent.slice_index == 0){
			Slice* slice = create_subtree();
			NodeHandle handle = slice->insert(node);
			NodeBase* new_node = slice->get_node(handle);
			new_node->parent = &root_node;
		}
		Slice* parent_slice = getSlice(parent.slice_index);
		if(!parent_slice){
			return NodeHandle{EMPTY,EMPTY}; //invalid slice, undefined
		}
		NodeHandle handle = parent_slice->insert(node);
		NodeBase* new_node = parent_slice->get_node(handle);
		new_node->parent = parent_slice->get_node(parent);
		parent_slice->get_node(parent)->children.insert(new_node);
		return handle;
	}
	Slice* create_subtree(){
		Slice& subtree = subtrees.emplace_back(Slice{0,10000});
		subtree.slice_index = next_index++;
		return &subtree;
	}
	Slice* getSlice(uint32_t slice_index){
		for(auto& i : subtrees)
			if(i.slice_index == slice_index)
				return &i;
 		return nullptr;
	}
	std::vector<NodeHandle> get_all_leaf_nodes(){
		std::vector<NodeHandle> leaves;
		for(size_t i = 0; i < subtrees.size(); ++i){
			std::vector<NodeHandle> subtree_leaves{std::move(subtrees[i].get_all_leaf_nodes())};
			leaves.insert(leaves.end(), subtree_leaves.begin(), subtree_leaves.end());
		}
		return leaves;
	}
	void remove(NodeHandle handle){
		Slice* slice = getSlice(handle.slice_index);
		if(!slice)
			return;
		slice->remove(handle);
	}
	void remove(NodeBase* node){
		remove(node->self_handle);
	}
private:
	NodeBase root_node;
	std::vector<Slice> subtrees;
	uint32_t next_index{1};
};
