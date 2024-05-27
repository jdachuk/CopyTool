// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "ThreadSafeQueue.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <memory>
#include <vector>

const int SUCCESS = 0;

const int ERR_WRONG_USAGE = -1;
const int ERR_SRC_MISSING = -2;
const int ERR_DST_EXISTS = -3;

const unsigned int nChunkSize = 64;

bool inputReady = false;
bool completed = false;

std::mutex m;
std::condition_variable cv;


void reader_thread(const std::string& strSrcPath, std::shared_ptr<ThreadSafeQueue<std::vector<char>>> spQueue)
{
	std::ifstream input(strSrcPath.c_str(), std::ios::binary);

	{
		std::lock_guard lk(m);
		if (input.is_open()) inputReady = true;
	}
	cv.notify_one();
	
	std::vector<char> vctBuffer(nChunkSize, 0);
	
	while (!input.eof())
	{
		input.read(vctBuffer.data(), vctBuffer.size());
		spQueue->pushBack(vctBuffer);
	}

	completed = true;
}

void writer_thread(const std::string& strDstPath, std::shared_ptr<ThreadSafeQueue<std::vector<char>>> spQueue)
{
	std::unique_lock lk(m);
	cv.wait(lk, [] { return inputReady; });

	std::ofstream output(strDstPath.c_str(), std::ios::binary | std::ios::trunc);

	while (!completed || !spQueue->empty())
	{
		std::vector<char> vctBuffer = spQueue->popFront();
		output.write(vctBuffer.data(), vctBuffer.size());
	}
}

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		std::cout << "Usage: \n"
			<< "\t CopyTool.exe source destination\n";
		return ERR_WRONG_USAGE;
	}

	const std::string strSrcPath(argv[1]);
	const std::string strDstPath(argv[2]);

	if (!std::filesystem::exists(strSrcPath))
	{
		std::cout << "Source file doesn't found\n";
		return ERR_SRC_MISSING;
	}

	if (std::filesystem::exists(strDstPath))
	{
		std::cout << "Destination file \"" << strDstPath << "\" already exists! Do you want to overwrite it?\n Y/N\n";
		char cOverwrite = 'n';
		std::cin >> cOverwrite;
		if (std::tolower(cOverwrite) != 'y')
			return ERR_DST_EXISTS;
	}

	auto spQueue = std::make_shared<ThreadSafeQueue<std::vector<char>>>();

	std::thread reader(reader_thread, strSrcPath, spQueue);
	std::thread writer(writer_thread, strDstPath, spQueue);

	reader.join();
	writer.join();

	return SUCCESS;
}


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
