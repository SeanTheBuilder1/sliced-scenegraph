#include <vector>
#include <memory>
#include <cstring>
#include <span>
#include <iostream>
#include <random>
#include <algorithm>

#include "timer.h"

constexpr size_t local_storage_size = 8;

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
	uint32_t slice_index;
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
		std::cout << "this must never be called\n";
	};
	small_vector(small_vector&& other)
		:fallback_array(std::move(other.fallback_array)), 
		use_fallback(other.use_fallback), 
		_size(other._size){
		std::memcpy(main_array, other.main_array, 8*sizeof(T));
	}
	small_vector& operator=(small_vector&& other) = default;
	~small_vector(){
		//std::cout<< _size << "print this shit\n";
	}
	
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
			for (int i = 0; i < x.size(); ++i)
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
			for(int i = 0; i < _size; ++i)
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
	NodeBase()
	{
	}
	~NodeBase(){
		//std::cout << "cleaning\n";
	}
	
	NodeBase& operator=(NodeBase&& other) = default;
	
	NodeBase(NodeBase&& other)
		:children(std::move(other.children)),
		parent(other.parent),
		self_handle(other.self_handle),
		depth(other.depth){
		//other.children.~small_vector<NodeBase *>();
	}
	uint32_t move_to(void *target_address)
	{
		//std::cout << "movestart";
		target_address = new(target_address) NodeBase(std::move(*this));
		//2std::cout << "moveend\n";
		return sizeof(NodeBase);
	}
};

struct NodeMeta
{
	uint32_t offset{0};
	uint32_t size{0};
	uint32_t next_free{0};
};

class Slice
{
	std::unique_ptr<uint8_t[]> data;
	std::span<uint8_t> storage;
	std::span<NodeMeta> node_metadata;
public:
	uint32_t num_nodes{0};
private:
	uint32_t next_storage_index; //???
	uint32_t next_insert_offset{0};
	uint32_t occupied_space;
	uint32_t first_free {0};

  public:
	Slice(size_t node_storage_size, size_t max_num_nodes)
		: data(std::make_unique<uint8_t[]>((sizeof(NodeMeta) * max_num_nodes) + (node_storage_size))),
		  occupied_space((sizeof(NodeMeta) * max_num_nodes) + (node_storage_size))
	{
		storage = std::span(data.get() + (sizeof(NodeMeta) * max_num_nodes), node_storage_size);
		node_metadata = std::span((NodeMeta *)data.get(), max_num_nodes);
		for(int i  = 0; i < node_metadata.size(); ++i){
			node_metadata[i].next_free = i+1;
		}
		node_metadata[node_metadata.size() - 1].next_free = -1;
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

	void reconstitute(float expansion_factor)
	{
		slib::Timer timer(false);
		timer.start();
		uint32_t data_size = (sizeof(NodeMeta) * node_metadata.size()) + (storage.size() * expansion_factor);
		auto new_data = std::make_unique<uint8_t[]>(data_size);
		std::cout << timer.stop() << "ms\n";
		std::span<NodeMeta> new_node_metadata((NodeMeta *)new_data.get(), node_metadata.size());
		std::span<uint8_t> new_storage(
			new_data.get() + (sizeof(NodeMeta) * node_metadata.size()),
			data_size - (sizeof(NodeMeta) * node_metadata.size()));
		std::memcpy(new_node_metadata.data(), node_metadata.data(), node_metadata.size_bytes());
		std::cout << timer.stop() << "ms\n";
		uint32_t total_offset = 0;
		
		{
		//slib::Timer tso(true);
		node_metadata = new_node_metadata;
		
		//START DEVIATION
		/*
		std::vector<uint32_t> node_indices_array;
		node_indices_array.reserve(num_nodes);
		NodeMeta* metadata = &node_metadata[0];
		NodeBase* node = (NodeBase*)&storage[metadata->offset];
		make_sorted_node_indices(node_indices_array, node);
		std::cout << node_indices_array.size() << " = Node indices size \n";
		for(auto i: node_indices_array){
			
			NodeBase* node = (NodeBase*)&storage[node_metadata[i].offset];
			NodeBase* new_node = (NodeBase*)&new_storage[total_offset];
			node_metadata[i].offset = total_offset;
			total_offset += node->move_to(new_node);
			uint32_t children_size = new_node->children.size();
			uint32_t subtotal_offset{0};
			std::cout << children_size<< "children size\n";
			for (int j = 1; j <= children_size; ++j){
				subtotal_offset += node_metadata[i+j].size;
				new_node->children[j] = (NodeBase*)(&storage[total_offset+subtotal_offset]);
			}
			std::cout << subtotal_offset << "sub_offset\n";	
		}
		
		*/
		
		NodeMeta* first_metadata = &node_metadata[0];
		NodeBase* first_node = (NodeBase*)&storage[node_metadata[0].offset];
		NodeBase* new_first_node = (NodeBase *)&new_storage[total_offset];
		first_metadata->offset = total_offset;
		total_offset += first_node->move_to(new_first_node);
		first_metadata->size = total_offset;
				
		for (auto& child : new_first_node->children)
		{
			//std::cout << child << "  child " << (NodeBase*)&storage[node_metadata[child->self_handle.internal_index].offset] << "\n";
			child = make_sorted_node_indices(child, &total_offset, child->self_handle.internal_index, new_storage);
			child->parent = new_first_node; 
		}
		
		
		data = std::move(new_data);
		}
		storage = new_storage;
		occupied_space = data_size;
		std::cout << timer.stop() << "ms\n";
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
		for(int i = 0; i < new_node->children.size(); ++i){
			new_node->children[i] = make_sorted_node_indices(new_node->children[i], offset, new_node->children[i]->self_handle.internal_index, new_storage);
			new_node->children[i]->parent = new_node;
		}
		/*for(auto& child : new_node->children){
			child = make_sorted_node_indices(child, offset, child->self_handle.internal_index, new_storage);
			//std::cout << child << "  child " << (NodeBase*)&new_storage[node_metadata[child->self_handle.internal_index].offset] << "\n";
			child->parent = new_node; 
		}*/
		return new_node;
	}
	
	NodeHandle insert(NodeBase* node){
		if((sizeof(NodeBase) + (num_nodes*sizeof(NodeBase))) > storage.size()){
			NodeHandle backup = node->self_handle;
			reconstitute(1.5);
			node = get_node(backup);
		}
		NodeMeta* metadata = &node_metadata[first_free];
		node->self_handle.internal_index = first_free;
		first_free = metadata->next_free;
		metadata->offset = next_insert_offset;
		NodeBase* node_dest = (NodeBase*)&storage[metadata->offset];
		metadata->size = node->move_to(node_dest);
		next_insert_offset += metadata ->size;
		num_nodes++;
		return node_dest->self_handle;
	}
	void remove(NodeHandle handle){
		NodeMeta* metadata = &node_metadata[handle.internal_index];
		metadata->next_free = first_free;
		first_free = handle.internal_index;
		NodeBase* node = (NodeBase*)&storage[metadata->offset];
		//std::cout << node->children.size() << "size\n";
		
		for(auto child : node->children){
			std::cout << "this should never happen\n";
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
		//std::cout << new_node << "  new " << get_node(new_node->self_handle) << "\n";
		new_node->parent = get_node(parent);
		//TO DO FIGURE OUT BUG
		//std::cout << new_node->parent << " and " << get_node(parent->self_handle) << '\n';
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
			//std::cout << child << "  and " << get_node(child->self_handle) << "\n";
			get_all_leaf_nodes_helper(leaves, child);
		}
		return leaves;
	}
	void get_all_leaf_nodes_helper(std::vector<NodeHandle>& leaves, NodeBase* node){
		if(node->children.size() == 0){
			
			//std::cout << get_node(node->self_handle)->children.size() << "lolsize\n";
			leaves.push_back(node->self_handle);
			return;
		}
		for(auto child : node->children){
			//std::cout << child << "  innerchild " << get_node(child->self_handle) << "\n";
			get_all_leaf_nodes_helper(leaves, child);
		}
	}
};

class SceneGraph
{
	NodeBase root_node;
	std::vector<Slice> sub_trees;
};

constexpr int small_size = sizeof(NodeMeta)*512000;

int main(int argc, char *argv[])
{
	small_vector<int> a1;
	a1.insert(1);
	a1.insert(2);
	a1.insert(3);
	a1.insert(4);
	a1.insert(5);
	a1.insert(3);
	a1.insert(6);
	a1.insert(7);
	a1.remove(3);
	for(auto i : a1){
		std::cout << i << ",";
	}
	
	
	small_vector<int> b2;
	b2=std::move(a1);
	Slice slice(1000, 2000000);
	
	int index = 0;
	
	for(int i = 0; i< 10; ++i){
		NodeBase lmao;
		NodeHandle lmfao = slice.insert(&lmao);
		index++;
		for(int j =0; j <5; ++j){
			NodeBase bao;
			NodeHandle xd = slice.create_node_with_parent(&bao, lmfao);
			index++;
			for(int k=0; k < 10; ++k){
				NodeBase p;
				NodeHandle wat = slice.create_node_with_parent(&p, xd);
				index++;
				for(int l=0; l < 10; ++l){
					NodeBase q;
					NodeHandle scale = slice.create_node_with_parent(&q, wat);
					index++;
					for(int m=0; m < 200; ++m){
						NodeBase ikert;
						slice.create_node_with_parent(&ikert, scale);
						index++;
					}
				}
			}
		}
	}
	uint32_t num_nodes = slice.num_nodes;
	
	slib::Timer timer(false);
	std::vector<NodeHandle> leaves = slice.get_all_leaf_nodes();
	std::cout << "Traverse time: " <<timer.stop() << '\n';
	
	uint32_t to_update = leaves.size() * (0.4);
	auto rng = std::default_random_engine {};
	std::shuffle(std::begin(leaves), std::end(leaves), rng);
	
	std::cout << "Reconstitute: ";
	{
	slib::Timer x(true);
	slice.reconstitute(2.0);
	}
	std::cout << to_update<<" nodes will be updated\n"; 
	std::cout << num_nodes << " num of nodes\n";
	{
		std::cout << "Results: ";
		slib::Timer qwe(false);
		
		for(int i = 0; i < to_update; ++i){
			slice.remove(leaves[i]);
		}
		std::cout << qwe.stop()<< "remove\n";
		qwe.start();
		for(int i = to_update; i < (to_update*2); ++i){
			NodeBase temp;
			slice.create_node_with_parent(&temp, leaves[i]);
		}
		std::cout << qwe.stop() << "update\n";
	}
	std::cout << "lol\n";
	
	
	
	NodeBase a;
	NodeBase b;
	NodeBase c;
	c.children.insert(&a);
	c.children.insert(&b);
	NodeBase* d = (NodeBase*)malloc(sizeof(NodeBase));
	NodeBase* foo = new(d) NodeBase(std::move(c));
	std::cout << small_size << "size\n";
}
//   :))


