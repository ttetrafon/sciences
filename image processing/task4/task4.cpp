#include "pch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>




//  Function to generate white noise.
//  Intesity is a maximum noise intenzity.
void bilySum(const cv::Mat& src, cv::Mat& dst, float intensity)
{
	cv::RNG rng;
	cv::Mat noise = cv::Mat::zeros(src.size(), CV_32SC1);
	for (int i = 0; i < noise.rows*noise.cols; ++i) {
		// generate values of white noise and update the image pixel values
		// use function: rng.gaussian(double sigma)
		// and equation for Gauss pass white noise
		// samples of the white noise with variation e.g. 0.2 compute like:
		// v = sqrt(power=0.2) * gaussian(sigma=1.0)
		// function gaussian() generates random values with normal distribution with mean value 0 and variance 1.
		noise.at<int>(i) = 0.0;
	}
	cv::Mat tmp;
	src.convertTo(tmp, noise.type());
	tmp = tmp + noise;
	tmp.convertTo(dst, src.type());
}




//  Function to filter the image with Gaussian filter.
//     - filter size in pixeles
//	   - sigma is a variance to compute values of Gaussian function
void separabilityGauss(const cv::Mat& src, int size, double sigma, cv::Mat& sepDst, cv::Mat& noSepDst, int &noSepCnt, int &sepCnt)
{
	// size - we need an odd number, min 3
	int center = size / 2;
	center = MAX(1, center);
	size = 2 * center + 1;

	// prepare a Gaussian filter v 1D 
	cv::Mat gauss1D = cv::Mat::zeros(1, size, CV_64FC1);

	// implement Gaussian coefficients using Gaussian equation
	/*  Working area - begin */
	const double pi = 3.1415926535897;

	for (int i = 0; i < size; i++) {
		// Gaussian: 1 / (sigma * sqrt(2*pi)) * exp( -(1/2) * ((x - mean) / sigma)^2 )
		double value = 1/(sigma * std::pow(2 * pi, 1/2)) * exp( -0.5 * std::pow( (i - center) / sigma , 2) ); // Use the gaussian equation to calculate the coefficients.
		//std::cout << i << ": " << value << std::endl;
		gauss1D.at<double>(0, i) = value; // Assign the values to the gauss1D filter.
	}

	/*  Working area - end */
	
	// nomalize valuers
	gauss1D = gauss1D / (sum(gauss1D).val[0] + 0.00001);

	// prepare Gaussian filter in 2D 
	// we use convolution of 1D Gauss. kernel in x a y directions with unit impulse 
	cv::Mat gauss2D = cv::Mat::zeros(size, size, CV_64FC1);
	gauss2D.at<double>(center, center) = 1.;
	filter2D(gauss2D, gauss2D, -1, gauss1D);
	filter2D(gauss2D, gauss2D, -1, gauss1D.t());
	gauss2D = gauss2D / (sum(gauss2D).val[0] + 0.00001);

	// apply prepared filter to source image
	// use the separability of the operator - use 1D filter
	// implement using opencv functions 'filter2D' and transpose of matrix mat.t()

	/*  Working area - begin */

	cv::Mat tmp = src; // Set a temporary Mat to hold the output of the operation.

	cv::filter2D(src, tmp, -1, gauss1D.t());
	
	// Show images (for testing)
	//cv::imshow("input", src);
	//cv::imshow("output1D", src);

	/*  Working area - end */


	// here apply prepared 2D filter to source image
	// implement using opencv function 'filter2D'
	
	/*  Working area - begin */

	cv::filter2D(src, tmp, -1, gauss2D);

	// Show images (for testing)
	//cv::imshow("output2D", tmp);
	//cv::waitKey(0);

	/*  Working area - end */

	// manually compute and store to variables below - amount of operations to compute one output pixel with/without the separable filter
	sepCnt = 0;
	noSepCnt = 0;

	return;
}



/*  */
void checkDifferences(const cv::Mat test, const cv::Mat ref, std::string tag, bool save = false);


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//
// Examples of input parameters
// ./mt-04 ../../data/garden.png 0.2 31 2.0

int main(int argc, char* argv[])
{
	std::string img_path = "";
	float noise_amp = 0.1;
	int filter_size = 45;
	float sigma = 8.0;

	// check input parameters
	if (argc > 1) img_path = std::string(argv[1]);
	if (argc > 2) noise_amp = atof(argv[2]);
	if (argc > 3) filter_size = atoi(argv[3]);
	if (argc > 4) sigma = atof(argv[4]);

	// load testing images
	cv::Mat src_rgb = cv::imread(img_path);

	// check testing images
	if (src_rgb.empty()) {
		std::cout << "Failed to load image: " << img_path << std::endl;
		return -1;
	}

	cv::Mat src_gray;
	cv::cvtColor(src_rgb, src_gray, cv::COLOR_BGR2GRAY); // CV_BGR2GRAY -> COLOR_BGR2GRAY

	//---------------------------------------------------------------------------

	cv::Mat diff, gray, zasum, sepOut, noSepOut, gauss_ref;
	int noSepCnt, sepCnt;

	bilySum(src_gray, zasum, noise_amp);

	separabilityGauss(zasum, filter_size, sigma, sepOut, noSepOut, noSepCnt, sepCnt);
	cv::GaussianBlur(zasum, gauss_ref, cv::Size(filter_size, filter_size), sigma);

	// evaluation
	checkDifferences(zasum, src_gray, "04_noise", true);
	checkDifferences(sepOut, gauss_ref, "04_gaussSep", true);
	checkDifferences(noSepOut, gauss_ref, "04_gaussNoSep", true);
	std::cout << " " << noSepCnt << " " << sepCnt << std::endl;

	return 0;
}
//---------------------------------------------------------------------------




void checkDifferences(const cv::Mat test, const cv::Mat ref, std::string tag, bool save)
{
	double mav = 255., err = 255., nonzeros = 255.;
	cv::Mat diff;

	if (!test.empty() && !ref.empty()) {
		cv::absdiff(test, ref, diff);
		cv::minMaxLoc(diff, NULL, &mav);
		err = (cv::sum(diff).val[0] / (diff.rows*diff.cols));
		nonzeros = 1. * cv::countNonZero(diff) / (diff.rows*diff.cols);
	}

	if (save) {
		if (!test.empty()) { cv::imwrite((tag + ".png").c_str(), test); }
		if (!diff.empty()) { diff *= 255;	cv::imwrite((tag + "_err.png").c_str(), diff); }
	}

	printf("%s %.2f %.2f %.2f ", tag.c_str(), err, nonzeros, mav);

	return;
}
