
#include <iostream>
#include <random>


#include "timer.h"
#include "scene_graph.h"


constexpr int small_size = sizeof(NodeMeta)*1005560;

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
	SceneGraph slice;
	// std::cout << slice.num_nodes << "numnodes is\n";
	// std::cout << slice.get_node(NodeHandle{0,0})->depth << "depth is\n";
	// NodeBase nodebase;
	// nodebase.depth = 99999;
	// NodeHandle nodehandle = slice.insert(&nodebase);
	// std::cout << slice.num_nodes << "numnodes is\n";
	// std::cout << slice.get_node(NodeHandle{0,0})->depth << "depth is\n";
	int index = 0;
	
	for(uint32_t i = 0; i< 10; ++i){
		NodeBase lmao;
		NodeHandle lmfao = slice.create_node_with_parent(&lmao, NodeHandle{0,0});
		index++;
		for(uint32_t j =0; j <5; ++j){
			NodeBase bao;
			NodeHandle xd = slice.create_node_with_parent(&bao, lmfao);
			index++;
			for(uint32_t k=0; k < 5; ++k){
				NodeBase p;
				NodeHandle wat = slice.create_node_with_parent(&p, xd);
				index++;
				for(uint32_t l=0; l < 5; ++l){
					NodeBase q;
					NodeHandle scale = slice.create_node_with_parent(&q, wat);
					index++;
					for(uint32_t m=0; m < 5; ++m){
						NodeBase ikert;
						slice.create_node_with_parent(&ikert, scale);
						index++;
					}
				}
			}
		}
	}
	//uint32_t num_nodes = slice.num_nodes;
	
	slib::Timer timer(false);
	std::vector<NodeHandle> leaves = slice.get_all_leaf_nodes();
	std::cout << "Traverse time: " <<timer.stop() << '\n';
	
	uint32_t to_update = leaves.size() * (0.4);
	auto rng = std::default_random_engine {};
	std::shuffle(std::begin(leaves), std::end(leaves), rng);
	
	// std::cout << "Reconstitute: ";
	// {
	// slib::Timer x(true);
	// slice.reconstitute(2.0);
	// }
	std::cout << to_update<<" nodes will be updated\n"; 
	//std::cout << num_nodes << " num of nodes\n";
	{
		std::cout << "Results: ";
		slib::Timer qwe(false);
		
		for(uint32_t i = 0; i < to_update; ++i){
			slice.remove(leaves[i]);
		}
		std::cout << qwe.stop()<< "remove\n";
		qwe.start();
		for(uint32_t i = to_update; i < (to_update*2); ++i){
			NodeBase temp;
			slice.create_node_with_parent(&temp, leaves[i]);
		}
		std::cout << qwe.stop() << "update\n";
	}
	std::cout << "lol\n";
	
	
	SceneGraph slice2{std::move(slice)};
	std::cout << "part2\n";
	leaves = slice2.get_all_leaf_nodes();
	to_update = leaves.size() * (0.4);
	rng = std::default_random_engine {};
	std::shuffle(std::begin(leaves), std::end(leaves), rng);
	
	// std::cout << "Reconstitute: ";
	// {
	// slib::Timer x(true);
	// slice2.reconstitute(2.0);
	// }
	std::cout << to_update<<" nodes will be updated\n"; 
	//std::cout << num_nodes << " num of nodes\n";
	{
		std::cout << "Results: ";
		slib::Timer qwe(false);
		
		for(uint32_t i = 0; i < to_update; ++i){
			slice2.remove(leaves[i]);
		}
		std::cout << qwe.stop()<< "remove\n";
		qwe.start();
		for(uint32_t i = to_update; i < (to_update*2); ++i){
			NodeBase temp;
			slice2.create_node_with_parent(&temp, leaves[i]);
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
	//std::cout << slice.storage.size_bytes() + slice.node_metadata.size_bytes() << "actual size\n";
	std::cin.ignore();
}
//   :))


