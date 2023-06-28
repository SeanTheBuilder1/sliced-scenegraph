
#include <iostream>
#include <random>


#include "timer.h"

#include "scene_graph.h"
// #include "naive_scene_graph.h"


//constexpr int small_size = sizeof(NodeMeta)*1005560;

int main(int argc, char *argv[])
{
	SceneGraph slice;
	int index = 0;
	
	for(uint32_t i = 0; i< 10; ++i){
		NodeBase lmao;
		NodeHandle lmfao = slice.create_node_with_parent(&lmao, NodeHandle{0,0});
		// NodeBase* lmfao = slice.create_node_with_parent(&lmao, slice.root_node);
		index++;
		for(uint32_t j =0; j <10; ++j){
			NodeBase bao;
			NodeHandle xd = slice.create_node_with_parent(&bao, lmfao);
			// NodeBase* xd = slice.create_node_with_parent(&bao, lmfao);
			index++;
			for(uint32_t k=0; k < 10; ++k){
				NodeBase p;
				NodeHandle wat = slice.create_node_with_parent(&p, xd);
				// NodeBase* wat = slice.create_node_with_parent(&p, xd);
				index++;
				for(uint32_t l=0; l < 10; ++l){
					NodeBase q;
					NodeHandle scale = slice.create_node_with_parent(&q, wat);
					// NodeBase* scale = slice.create_node_with_parent(&q, wat);
					index++;
					for(uint32_t m=0; m < 250; ++m){
						NodeBase ikert;
						NodeHandle hee = slice.create_node_with_parent(&ikert, scale);
						index++;
						// for(uint32_t n=0; n < 5; ++n){
						// 	NodeBase mn;
						// 	NodeHandle mna = slice.create_node_with_parent(&mn, hee);
						// 	index++;
						// 	for(uint32_t o=0; o < 4; ++o){
						// 		NodeBase no;
						// 		NodeHandle noa = slice.create_node_with_parent(&no, mna);
						// 		index++;
						// 		for(uint32_t p=0; p < 4; ++p){
						// 			NodeBase op;
						// 			NodeHandle opa = slice.create_node_with_parent(&op, noa);
						// 			index++;
						// 		}
						// 	}
						// }
					}
				}
			}
		}
	}
	uint32_t num_nodes = slice.get_total_nodes();
	std::cout << index;

	std::vector<float> update_times;
	std::vector<float> remove_times;
	std::vector<float> traverse_times;
	std::vector<float> transform_times;
	std::vector<float> cycle_times;
	slice.reconstitute(1.0);
	for(int repeat = 0; repeat < 20; ++repeat){
		slib::Timer cycle_timer(false);
		num_nodes = slice.get_total_nodes();
		slib::Timer timer(false);
		std::vector<NodeHandle> leaves;
		slice.get_all_leaf_nodes(leaves);
		traverse_times.push_back(timer.stop()/1000/num_nodes);
		timer.start();
		slice.transform_visit();
		timer.stop();
		transform_times.push_back(timer.stop()/1000/num_nodes);
		uint32_t to_update = num_nodes * (0.1);
		//std::cout << "lol\n";
		if(to_update > (leaves.size()/2)){
			// std::cout << "bruh\n";
			to_update = leaves.size() * (0.4);
		}
		auto rng = std::default_random_engine {};
		std::shuffle(std::begin(leaves), std::end(leaves), rng);
		{
			slib::Timer qwe(false);
			for(uint32_t i = 0; i < to_update; ++i){
				slice.remove(leaves[i]);
			}
			remove_times.push_back(qwe.stop()/1000/num_nodes);
			qwe.start();
			for(uint32_t i = to_update; i < (to_update*2); ++i){
				NodeBase temp;
				slice.create_node_with_parent(&temp, leaves[i]);
			}
			update_times.push_back(qwe.stop()/1000/num_nodes);
		}
		cycle_times.push_back(cycle_timer.stop()/1000/num_nodes);
	}
	auto average = [](std::vector<float> lol){
		float sum = 0.0f;
		for(auto i : lol)
			sum += i;
		return sum/lol.size();
	};
	std::cout << "Traverse times = " << average(traverse_times) << "\n";
	std::cout << "Transform times = " << average(transform_times) << "\n";
	std::cout << "Remove times = " << average(remove_times) << "\n";
	std::cout << "Update times = " << average(update_times) << "\tCycle times = " << average(cycle_times) << "\n";
}
//   :))


