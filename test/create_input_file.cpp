#include <cstring>
#include <string>
#include <random>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <chrono>
using namespace std;

int tmp_ins, tmp_del;

void CreateRandOperation(int total_operation, int thread_num, int insert_ratio, int delete_ratio) {
	if (insert_ratio < delete_ratio) {
		printf("ratio must be insert >= delete\n");
	}
	if (insert_ratio + delete_ratio != 10) {
		printf("insert_ratio + delete_ratio must be 10\n");
	}
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	std::uniform_int_distribution<int> distribution(0,9);
	int cut = 100000, cut_count = 0;
	int remain_operation = std::min(total_operation, cut);
	int cur_thread = 0, key_per_thread = total_operation / thread_num, cur_written = 0;

	total_operation -= remain_operation;
	string fname = "tmp" + to_string(cur_thread) + ".txt";
	std::ofstream fout(fname, ios::trunc);
	std::vector<int> copy_keys;
	copy_keys.reserve(cut);
	for (int i = 1; i <= cut; i++)
		copy_keys.push_back(i);

	while (remain_operation != 0) {
		int insert_operation = (remain_operation/10) * insert_ratio;
		int delete_operation = remain_operation - insert_operation;
		int ins = 0;
		std::vector<int> keys(copy_keys);
		std::vector<int> pos_del;
		pos_del.reserve(delete_operation);
		shuffle(keys.begin(), keys.end(), std::default_random_engine(seed));
		printf("shuffle done\n");
		while (insert_operation != 0 && delete_operation != 0) {
			int flip_coin = distribution(generator);
			if (flip_coin < insert_ratio) {//insert
				insert_operation--; tmp_ins++; cur_written++;
				pos_del.push_back(keys[ins] + cut*cut_count);
				fout << "i " << keys[ins++] + cut*cut_count << endl;
			}
			else {//delete
				if (pos_del.size() == 0)
					continue;
				std::uniform_int_distribution<int> tmp_distribution(0,pos_del.size()-1);
				delete_operation--; tmp_del++; cur_written++;
				// shuffle(pos_del.begin(), pos_del.end(), std::default_random_engine(seed));
				int idx = tmp_distribution(generator);
				fout << "d " << pos_del[idx] << endl;
				pos_del.erase(pos_del.begin()+idx);
			}
			if (cur_written == key_per_thread) {
				cur_written = 0;
				fout.close();
				fname = "tmp" + to_string(++cur_thread) + ".txt";
				if (cur_thread < thread_num)
					fout.open(fname, ios::trunc);
			}
		}
		for (int i = insert_operation; i > 0; i--) {
			pos_del.push_back(keys[ins] + cut*cut_count); tmp_ins++; cur_written++;
			fout << "i " << keys[ins++] + cut*cut_count << endl;
			if (cur_written == key_per_thread) {
				cur_written = 0;
				fout.close();
				fname = "tmp" + to_string(++cur_thread) + ".txt";
				if (cur_thread < thread_num)
					fout.open(fname, ios::trunc);
			}
		}
		shuffle(pos_del.begin(), pos_del.end(), std::default_random_engine(seed));
		for (int i = delete_operation; i > 0; i--) {
			fout << "d " << pos_del.back() << endl; tmp_del++; cur_written++;
			pos_del.pop_back();
			if (cur_written == key_per_thread) {
				cur_written = 0;
				fout.close();
				fname = "tmp" + to_string(++cur_thread) + ".txt";
				if (cur_thread < thread_num)
					fout.open(fname, ios::trunc);
			}
		}

		if (total_operation > cut) {
			remain_operation = cut;
			total_operation -= cut;
			cut_count++;
		} else if (total_operation == 0){
			remain_operation = 0;
		} else {
			remain_operation = total_operation;
			total_operation = 0;
			cut_count++;
		}
	}
}

int main()
{
	int inst, ins_rat, thread;
	printf("inst: ");
	scanf("%d", &inst);
	printf("thread_num: \n");
	scanf("%d", &thread);
	printf("insert ratio: ");
	scanf("%d", &ins_rat);
	CreateRandOperation(inst, thread, ins_rat, 10 - ins_rat);
	printf("%d : %d = %d\n", tmp_ins, tmp_del, tmp_ins+tmp_del);
}
