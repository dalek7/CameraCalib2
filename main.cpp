#include <iostream>

#include <opencv2/core.hpp>
using namespace cv;

int main(int, char**) {
    std::cout << "Hello, world!\n";

	Mat C = Mat::zeros(3, 3, CV_32F);
	Mat D = Mat::ones(3, 3, CV_32F);
	Mat E = Mat::eye(3, 3, CV_32F);    

    std::cout << E << std::endl;
}
