/**********************************************************
Name :
Date : 2019/12/2
By   : liaojf
Final:
     : 2019/10/23: initial svm code
     : 2019/10/28: read 冠字號
	 : 2019/11/6 : show input image
	 : 2019/11/21 : try HOG feature
	 : 2019/12/9  : output decide character in order
	 : 2019/12/17 : add euler number, judge a list of images
	              : write result to one report file
     : 2019/12/24 : add center_weight
	 : 2019/12/30 : add dilation
	 : 2020/1/8   : deal img117-253 , img106-116
	 : 2020/1/21  : add report out
	 : 2020/1/22  : add y-pos constraint, add tw_base groups
   : 2020/2/6   : modify code for ubuntu  , path strings are different
   : 2020/3/9   : test speed up
   : 2020/3/9   : test org275_1 ~ org402_1
   : 2020/3/10  : test org403_1 ~ org428_1
   : 2020/3/11  : fontzie=[16,32], vector =108, kernel
   : 2020/3/12  : correct fignum value
   : 2020/3/12  : add left_cor to separate "B" + "8"
   : 2020/3/13  : improve accuracy
   : 2020/3/16  : for scan_flow test
   : 2020/3/16  : scan_flow test for DK2
   : 2020/3/17  : for LEO image (dark)
   : 2020/3/18  : remove large width font, height <20
   : 2020/3/19  : use NTU liblinear, 6 section codes
   : 2020/3/23  :  rm dummy image, dst, drawing
   : 2020/3/23  : remove dummy images ,dst, drawing
   : 2020/3/24  : remove bw
   : 2020/3/25  : use adaptive_thresh(51,20)
   : 2020/3/26  : font height >18
   : 2020/3/27  : add ming-image section7, handle shortage out_fonts
   : 2020/3/30  : equalizeHist,meanStdDev,calcHist: threshold value
   : 2020/3/31  : add section_9(img595~ img769)
   : 2020/4/1   : add section_10(img770~ img879)
   : 2020/4/8   : cropping 冠字號 block + section_11~12
   : 2020/4/9   : search top-left point from left-half-image area, bounding_rect.height >10
   : 2020/4/10  : defective  冠字號 problem
   : 2020/4/13  : left-top y-pos limit <55+ merged two images
   : 2020/4/14  : add F+I+O constraint; adaptive+fixed mixed mode
**********************************************************/
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv/cv.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "svm.h"
#include "linear.h"
#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <math.h>

//#pragma comment(lib,"libsvm.lib")


using namespace cv;
using namespace std;
using namespace cv::ml;


vector<int> current_layer_holes(vector<Vec4i> layers, int index) {
	int next = layers[index][0];
	vector<int> indexes;
	indexes.push_back(index);
	while (next >= 0) {
		indexes.push_back(next);
		next = layers[next][0];
	}
	return indexes;
}


typedef struct  alpha_order{
	float x_pos;
	double decide_val;
	int eulernum;
	int center_wk;
	int left_cor;
} font_order;

bool struct_cmp_by_x_pos(font_order a, font_order b)
{
	return a.x_pos < b.x_pos;
}

typedef struct  body_order {
	float x_pos;
	float y_pos;
	float area;
} bd_order;

bool struct_cmp_blk_xy(bd_order a, bd_order b)
{
	if ((a.x_pos < b.x_pos))
		return true;
	else if ((a.x_pos == b.x_pos) && (a.y_pos < b.y_pos))
		return true;
	else
		return false;
}



/*----
 struct Color {
   int R;
   int G;
   int B;
 };
 --*/


#define debug_20191219
//#define tw_sec1   //---img16-img56
//#define tw_sec2   //---img57-img105
//#define tw_sec3   //---img106-img253
//#define tw_sec4   //---img254-img274
//#define tw_sec5   //---img275-img402
//#define tw_sec6   //---img403-img428
//#define tw_sec7   //---img429-img511
//#define tw_sec8   //---img512-img594
//#define tw_sec9   //---img595-img-img769
//#define tw_sec10   //---img770-img-img879
//#define tw_sec11   //---img880-img-img989
#define tw_sec12   //---img990-img-img1079

#define ubuntu
//#define win7
//#define scan_in_test

 int main() {


	 //-----切割 input 冠字號
	 char *dest = "input%d_1.bmp";
	 char basefilename[100] = " ";


 // Mat denimg, dengray, blurImage, thr_img, adap_img, erosion_dst;
	Mat denimg, dengray, thr_img;
  char fontname[100] = " ";

  //struct svm_model *model_ntu;
   struct model *model_ntu;
  //model_ntu = svm_load_model("cv_tw_model.txt");
   model_ntu = load_model("cv_tw_model_linear.txt");
  //int orghog_descr_size = 4032;
   //  int orghog_descr_size = 576;
  int orghog_descr_size = 108;


#ifdef tw_sec1
    ofstream rpt_out("cv_det_tw_16_56.txt");
#endif
#ifdef tw_sec2
   ofstream rpt_out("cv_det_tw_57_105.txt");
#endif
#ifdef tw_sec3
   ofstream rpt_out("cv_det_tw_106_253.txt");
#endif
#ifdef tw_sec4
   ofstream rpt_out("cv_det_tw_254_274.txt");
#endif
#ifdef tw_sec5
   ofstream rpt_out("cv_det_tw_275_402.txt");
#endif
#ifdef tw_sec6
   ofstream rpt_out("cv_det_tw_403_428.txt");
#endif
#ifdef tw_sec7
   ofstream rpt_out("cv_det_tw_429_511.txt");
#endif
#ifdef tw_sec8
   ofstream rpt_out("cv_det_tw_512_594.txt");
#endif
#ifdef tw_sec9
   ofstream rpt_out("cv_det_tw_595_769.txt");
#endif
#ifdef tw_sec10
   ofstream rpt_out("cv_det_tw_770_879.txt");
#endif
#ifdef tw_sec11
   ofstream rpt_out("cv_det_tw_880_989.txt");
#endif
#ifdef tw_sec12
   ofstream rpt_out("cv_det_tw_990_1079.txt");
#endif


  if (!rpt_out) {
	  cout << "無法寫入檔案\n";
	  return 1;
  }


#ifdef tw_sec1
#ifdef scan_in_test
  for (int k = 1; k < 2000; k++)
#else
 for (int k = 16; k < 57; k++)
#endif
#endif
#ifdef tw_sec2
  for (int k = 57; k < 106; k++)
#endif
#ifdef tw_sec3
  for (int k = 106; k < 254; k++)
#endif
#ifdef tw_sec4
  for (int k = 254; k < 275; k++)
#endif
#ifdef tw_sec5
	  for (int k = 275; k < 403; k++)
#endif
#ifdef tw_sec6
	for (int k = 403; k < 429; k++)
#endif
#ifdef tw_sec7
   for (int k = 429; k < 512; k++)
#endif
#ifdef tw_sec8
	   for (int k = 512; k < 595; k++)
#endif
#ifdef tw_sec9
	   for (int k = 595; k < 770; k++)
#endif
#ifdef tw_sec10
	  for (int k = 770; k < 880; k++)
#endif
#ifdef tw_sec11
		  for (int k = 880; k < 990; k++)
#endif
#ifdef tw_sec12
		  for (int k = 990; k < 1080; k++)
#endif

 //int k;
 //while (cin >> k)
  {

	  //------original golden font
    //--- for ubuntu
    #ifdef ubuntu
#ifdef scan_in_test
	 char *infont = "/home/liaojf/test1/try_1/tw_base/org%d_1.bmp";
#else
	 char *infont = "/home/root/dk2_test2/tw_base/org%d_1.bmp";
#endif
    #endif
    //--- for win
    #ifdef win7
#ifdef scan_in_test
	 char *infont = "G:\\ljf_repos\\OpenCV_test\\test1\\test1\\org%d_1.bmp";
#else
    char *infont = "D:\\prj_arm\\2019_matlabtest\\code\\tw_base\\org%d_1.bmp";
#endif
    #endif

	sprintf(fontname, infont, k);

	///--- 2020/3/11 scan_in test beginning
#ifdef scan_in_test
	  //---- 2020/3/11 while check scan_in image
	  ifstream scan_ing;
	  scan_ing.open(fontname);

	  while (!scan_ing) {
		  //cout << " scan ing , not ready !" << endl;
		  scan_ing.open(fontname);
	  }
#endif
    ///--- 2020/3/11 scan_in test end

	  denimg = imread(fontname);
	  //0206 imshow("original img", denimg);

	  //---- temp out
	  // 0206 cout << "============<<<<<<<<<" << endl;
	 // 0206 cout << "((((((( k= " << k << endl;
	  /* blur(denimg, blurImage, Size(3, 3));
	   imshow("blur window", blurImage); */

	   /*
	   0: Binary
	   1: Binary Inverted
	   2: Threshold Truncated
	   3: Threshold to Zero
	   4: Threshold to Zero Inverted
	   */

	  cvtColor(denimg, dengray, COLOR_BGR2GRAY);

	  /*-----
	  //----- 2020/3/30
	  //--- equalize image
	  Mat equimg;
	  equalizeHist(dengray, equimg);
	  imwrite("equal.bmp", equimg);
	  ---*/
	  /*-------
	  //-----2020/3/31
	  Mat mat_mean, mat_stddev;
	  meanStdDev(dengray, mat_mean, mat_stddev);
	  double m, s;
	  m = mat_mean.at<double>(0, 0);
	  s = mat_stddev.at<double>(0, 0);

      cout << "mean=" << m << endl;
      cout << "deviation=" << s << endl;
	  -----*/

	  /*---
	  //-------- plot histogram
	  Mat hist;
	  int imgNum = 1;
	  int histDim = 1;
	  int histSize = 256;
	  float range[] = { 0, 256 };
	  const float* histRange = { range };
	  bool uniform = true;
	  bool accumulate = false;

	  calcHist(&dengray, imgNum, 0, Mat(), hist, histDim, &histSize, &histRange, uniform, accumulate);
	  //imshow(histvalue);

      int scale = 2;//控制圖像的寬大小
      Mat histImg(cv::Size(histSize*scale, histSize), CV_8UC1);//用於顯示直方圖

	  uchar* pImg = nullptr;
	  for (size_t i = 0; i < histImg.rows; i++) //初始化圖像爲全黑
	  {
		  pImg = histImg.ptr<uchar>(i);
		  for (size_t j = 0; j < histImg.cols; j++)
		  {
			  pImg[j] = 0;
		  }
	  }

	  double maxValue = 0; //直方圖中最大的bin的值
	  minMaxLoc(hist, 0, &maxValue, 0, 0);

	  int histHeight = 256; //要繪製直方圖的最大高度

	  float* p = hist.ptr<float>(0);
	  for (size_t i = 0; i < histSize; i++)//進行直方圖的繪製
	  {
		  float bin_val = p[i];
		  int intensity = cvRound(bin_val*histHeight / maxValue);  //要繪製的高度
		  for (size_t j = 0; j < scale; j++) //繪製直線 這裏用每scale條直線代表一個bin
		  {
			  line(histImg, Point(i*scale + j, histHeight - intensity), Point(i*scale + j, histHeight - 1), 255);
		  }
		  //cv::rectangle(histImg, cv::Point(i*scale, histHeight - intensity), cv::Point((i + 1)*scale, histHeight - 1), 255); //利用矩形代表bin
	  }


	 imwrite("hist.bmp", histImg);
	 -------*/
	  //--- end of plot histogram


	  //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
	  //------- 2020/4/7 find 冠字號 center
	  vector<vector<Point> > cen_contours;
	  vector<Vec4i> cen_hierarchy;
	  Rect  trace_rect;
	  Mat group_img;
	  vector<bd_order> char_area;
	  bd_order bd_pos;

	  //--- 2020/4/13 first, do whole image with fixed threshold
	  // Mat merge_img = dengray.clone();
	  Mat merge_img;
	  threshold(dengray, merge_img, 150, 255, THRESH_BINARY_INV);
	  //---- 20200409
	  //--- use left-half image
	  group_img = merge_img(Rect(0, 0, 65, merge_img.rows));


	  //imwrite("group.bmp", group_img);

	  findContours(group_img, cen_contours, cen_hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	  //cout << "find font num = " << cen_contours.size() << endl;

	  //---- 2020/4/10 if char_area.size ==0, many points to be checked
	  if ( cen_contours.size() < 40) {

		  for (int k = 0; k < cen_contours.size(); k++) {
			  trace_rect = boundingRect(cen_contours[k]);
			 // cout << "2nd area calculation= " << contourArea(cen_contours[k], false) << endl;
			 // cout << "  x=" << trace_rect.x << " y=" << trace_rect.y << endl;

			  if (contourArea(cen_contours[k], false) > 6 &&
				  trace_rect.width < 27 && trace_rect.height >5
				  && trace_rect.y <55) {
				  bd_pos.x_pos = trace_rect.x;
				  bd_pos.y_pos = trace_rect.y;
				  bd_pos.area = contourArea(cen_contours[k], false);
				  char_area.push_back(bd_pos);
			  }

		  }
	  }

	  if (char_area.size() != 0) {
		  sort(char_area.begin(), char_area.end(), struct_cmp_blk_xy);
		  //cout << " 1st x=" << char_area[0].x_pos << "  1st y=" << char_area[0].y_pos << endl;

		  Mat focus_img;
		  //--- check out_of_range
		  int y_limit, x_limit;
		  int y_st_limit, x_st_limit;

		  if (char_area[0].y_pos + 28 < dengray.rows)
			  y_limit = char_area[0].y_pos + 28;
		  else
			  y_limit = dengray.rows;

		  if (char_area[0].x_pos + 196 < dengray.cols)
			  x_limit = char_area[0].x_pos + 196;
		  else
			  x_limit = dengray.cols;

		  if (char_area[0].y_pos - 1 < 0)
			  y_st_limit = 0;
		  else
			  y_st_limit = char_area[0].y_pos - 1;

		  if (char_area[0].x_pos - 1 < 0)
			  x_st_limit = 0;
		  else
			  x_st_limit = char_area[0].x_pos - 1;

		  Range rows(y_st_limit, y_limit);
		  Range cols(x_st_limit, x_limit);
		  //------ EXTRACT 冠字號
		  focus_img = dengray(rows, cols);
		 // imwrite("focus.bmp", focus_img);

		  adaptiveThreshold(focus_img, thr_img, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 31, 30);
		 // imwrite("focus_apt.bmp", thr_img);
          thr_img.copyTo(merge_img(Rect(x_st_limit, y_st_limit, thr_img.cols, thr_img.rows)));
		  //imwrite("merged.bmp", merge_img);
	  }
	  else
	  {
		  adaptiveThreshold(dengray, merge_img, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 31, 30);
		 // imwrite("merged.bmp", merge_img);

	  }

	  //=============== merged images test
	  //Mat merge_img = Mat::zeros(dengray.size(), CV_8UC1);
	  //Mat merge_img = dengray.clone();
	  // Create an all white mask
	  //Mat src_mask = 255 * Mat::ones(thr_img.rows, thr_img.cols, CV_8UC1);
	  // The location of the center of the src in the dst
	  //Point center(char_area[0].x_pos+10, char_area[0].y_pos+10);
	 // Point center(34, 120);
	  //Mat normal_clone;

	 // thr_img.copyTo(merge_img(Rect(x_st_limit, y_st_limit, thr_img.cols, thr_img.rows)));

	  //seamlessClone(thr_img, merge_img, src_mask, center, normal_clone, NORMAL_CLONE);

	 // imwrite("merged.bmp", merge_img);

	  //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

	  //--- 20191219 test
	  //threshold(dengray, thr_img, 150, 255, THRESH_BINARY_INV);

	  //--- good
	  //threshold(dengray, thr_img, 100, 255, THRESH_OTSU+THRESH_BINARY_INV);

	  //- adaptive
	  //adaptiveThreshold(dengray, thr_img, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 51, 20);
          //---- 20200325 adaptive
	  //--- golden
	  //---20200407
	 // adaptiveThreshold(dengray, thr_img, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 31, 30);

	  //--- 20200330
	  //threshold(dengray, thr_img, 100, 255, THRESH_BINARY_INV);


	  //imshow("thr window", thr_img);
	 // imwrite("thr.bmp", thr_img);

	  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	  //-------2019/12/30
	  //--- MORPH_RECT, MORPH_CROSS
	 // Mat element = getStructuringElement(MORPH_RECT, Size(2, 1), Point(-1, -1));
	  Mat out_dilate;
	  //Mat out_erode;
	  //進行膨脹操作
	  //dilate(thr_img, out_dilate, element, Point(-1, -1), 1);
	  //--縮點
	  //erode(thr_img, out_erode, element, Point(-1, -1), 1);


	 // imwrite("out_erode.bmp", out_erode);
	  //imshow("erosion demo", out_erode);
	  //---- 2020/1/14 再做 dilation
	  //進行膨脹操作

	  Mat element2 = getStructuringElement(MORPH_RECT, Size(1, 2), Point(-1, -1));
	  //dilate(thr_img, out_dilate, element2, Point(-1, -1), 1);
	  dilate(merge_img, out_dilate, element2, Point(-1, -1), 1);



	  //顯示效果圖
	  // imwrite("out_dilate.bmp", out_dilate);
	  // imshow("dilation demo", out_dilate);
	  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


	  //------ inv binary image
	  //Mat invbw2_img;
	  //threshold(thr_img, invbw2_img, 128.0, 255.0, THRESH_BINARY_INV);
	  //invbw2_img = thr_img;


	  //---- 2017/12/17 測
	  //threshold(dengray, invbw2_img, 150.0, 255.0, THRESH_BINARY_INV);
	  //imwrite("invbw.bmp", invbw2_img);

#ifdef debug_20191219

	  //string code;
	 // code = pytesseract.image_to_string(invbw2_img, config = 'digits');
	 // cout << "code:" << code << endl;

	  //Mat denoise_img;
      //fastNlMeansDenoisingColored(thr_img, denoise_img, 10, 10, 7, 21);
	  //imwrite("denoise.bmp", denoise_img);

#endif



	  //------- 2019/11/7
	  //---- label FONT
	  //int threshval = 100;

	 // Mat bw = threshval < 128 ? (thr_img < threshval) : (thr_img > threshval);

	 // Mat bw = (thr_img > 60);
	 // Mat bw = (out_erode > 60);
      /*---------20200324
	  Mat bw = (out_dilate > 60);
	  ------- end 20200324 --*/

   /*-----20200323
	  Mat labelImage(thr_img.size(), CV_32S);
	  int nLabels = connectedComponents(bw, labelImage, 8);

	  vector<Vec3b> colors(nLabels);
	  colors[0] = Vec3b(0, 0, 0);//background

	  for (int label = 1; label < nLabels; ++label) {
		  colors[label] = Vec3b((rand() & 255), (rand() & 255), (rand() & 255));
	  }
	  Mat dst(thr_img.size(), CV_8UC3);
	  for (int r = 0; r < dst.rows; ++r) {
		  for (int c = 0; c < dst.cols; ++c) {
			  int label = labelImage.at<int>(r, c);
			  Vec3b &pixel = dst.at<Vec3b>(r, c);
			  pixel = colors[label];
		  }
	  }
	   //imshow("Connected Components", dst);
	 // 0206 imwrite("dst.bmp", dst);
	 // 0206 imwrite("bw_org.bmp", bw);

   ---- 20200323  */

	  //------ 2019/12/19 除雜點
	  //Mat labels(thr_img.size(), CV_32S);
	  Mat labels(merge_img.size(), CV_32S);
	  Mat img_color, stats, centroids;
	  /*--- 20200324
	  int nccomps = connectedComponentsWithStats(
		  bw, labels,
		  stats, centroids
	  );     ----- */
	  int nccomps = connectedComponentsWithStats(
		  out_dilate, labels,
		  stats, centroids
	  );


	  // 0206 cout << "Total Connected Components Detected: " << nccomps << endl;

	  /*-------- 20200324 begin---
	  vector<Vec3b> colors2(nccomps + 1);

	  colors2[0] = Vec3b(0, 0, 0); // background pixels remain black.
	  for (int i = 1; i < nccomps; i++) {
		  //colors2[i] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
		  colors2[i] = Vec3b(255, 255, 255);

		  if (stats.at<int>(i, CC_STAT_AREA) < 40)
			  colors2[i] = Vec3b(0, 0, 0); // small regions are painted with black too.
	  }

	  img_color = Mat::zeros(thr_img.size(), CV_8UC3);

	  for (int y = 0; y < img_color.rows; y++)
		  for (int x = 0; x < img_color.cols; x++)
		  {
			  int label = labels.at<int>(y, x);
			  CV_Assert(0 <= label && label <= nccomps);
			  img_color.at<Vec3b>(y, x) = colors2[label];
		  }

	 // 0206  imwrite("proc.bmp", img_color);

	  ---- 20200324 end ---- */

	  vector<uchar> colors2(nccomps + 1);

	  colors2[0] = 0; // background pixels remain black.
	  for (int i = 1; i < nccomps; i++) {
		 colors2[i] = 255;

		  if (stats.at<int>(i, CC_STAT_AREA) < 40)
			  colors2[i] = 0; // small regions are painted with black too.
	  }

	  Mat invbw_img;
	  invbw_img = Mat::zeros(merge_img.size(), CV_8UC1);

	  for (int y = 0; y < invbw_img.rows; y++)
		  for (int x = 0; x < invbw_img.cols; x++)
		  {
			  int label = labels.at<int>(y, x);
			 // CV_Assert(0 <= label && label <= nccomps);
			  invbw_img.at<uchar>(y, x) = colors2[label];
		  }

	  //--- 20200326 debug img_section1
	 // imwrite("invbw.bmp", invbw_img);

	   //------ 2019/11/21 除雜點= fail
	   /*
	   Mat rem_noise;
	   bilateralFilter(invbw2_img, rem_noise, 5, 21, 21);
	   imwrite("remnoise.bmp", rem_noise);
	   */
	   //-- do check
	   //imwrite("checkremove.bmp", invbw2_img);

	   //----- get each character
	   //---- 2019/11/13
	  vector<vector<Point> > contours;
	  vector<Vec4i> hierarchy;

	 // RNG rng;

	  //---- 2019/12/19 add test
	  /*-- 20200324
	  Mat invbw_img;
	  //invbw_img = img_color;
	  cvtColor(img_color, invbw_img, COLOR_BGR2GRAY);
      ---- 20200324 end*/

	  //-- euler number
	  findContours(invbw_img, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
	  //--- below = ok
	  //findContours(invbw_img, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

     /*---- 20200323
	  /// Draw contours
	  Mat drawing = Mat::zeros(invbw_img.size(), CV_8UC3);

	  for (int i = 0; i < contours.size(); i++)
	  {
		  Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
		  drawContours(drawing, contours, i, color, 2, 8, hierarchy, 0, Point());
	  }

	  /// Show in a window
	  // 0206 namedWindow("Contours", CV_WINDOW_AUTOSIZE);
	  // 0206 imshow("Contours", drawing);
	  // 0206 imwrite("drawing.bmp", drawing);
     ---20200323---*/

	  //----- input 冠字號
	  Rect  bounding_rect;
	  Mat font_img, font2_img, font3_img;


	  //----do HOG
	  vector<float> ders;
	 // vector<Point> locs;
	 // vector<vector<float> > decideHOG;
	  //-- org font
	 // vector<float> org_ders;
	 // vector<vector<float> > org_HOG;

	   //--- use opencv_hog
     /*  2020/03/06 keep
	  HOGDescriptor hog(
		  Size(64, 64), //winSize
		  Size(16, 16), //blocksize = typical 2x cellsize
		  Size(16, 16), //blockStride,
		  Size(8, 8), //cellSize,
		  9, //nbins,
		  1, //derivAper,
		  -1, //winSigma,
		  0, //histogramNormType,
		  0.2, //L2HysThresh,
		  0,//gammal correction,
		  64,//nlevels=64
		  1);    */

     HOGDescriptor hog(
		  Size(16, 16), //winSize
		  Size(16, 16), //blocksize = typical 2x cellsize
		  Size(16, 16), //blockStride,
		  Size(8, 8), //cellSize,
		  9, //nbins,
		  1, //derivAper,
		  -1, //winSigma,
		  0, //histogramNormType,
		  0.2, //L2HysThresh,
		  0,//gammal correction,
		  64,//nlevels=64
		  1);


#if 0
	  struct svm_parameter
	  {
		  int svm_type;
		  int kernel_type;
		  int degree;	/* for poly */
		  double gamma;	/* for poly/rbf/sigmoid */
		  double coef0;	/* for poly/sigmoid */

						/* these are for training only */
		  double cache_size; /* in MB */
		  double eps;	/* stopping criteria */
		  double C;	/* for C_SVC, EPSILON_SVR and NU_SVR */
		  int nr_weight;		/* for C_SVC */
		  int *weight_label;	/* for C_SVC */
		  double* weight;		/* for C_SVC */
		  double nu;	/* for NU_SVC, ONE_CLASS, and NU_SVR */
		  double p;	/* for EPSILON_SVR */
		  int shrinking;	/* use the shrinking heuristics */
		  int probability; /* do probability estimates */
	  };
#endif
	  /*
	  // default values
	  param.svm_type = C_SVC;
	  param.kernel_type = RBF;
	  param.degree = 3;
	  param.gamma = 0;	// 1/num_features
	  param.coef0 = 0;
	  param.nu = 0.5;
	  param.cache_size = 100;
	  param.C = 1;
	  param.eps = 1e-3;
	  param.p = 0.1;
	  param.shrinking = 1;
	  param.probability = 0;
	  param.nr_weight = 0;
	  param.weight_label = NULL;
	  param.weight = NULL;
	  */

	  //enum { C_SVC, NU_SVC, ONE_CLASS, EPSILON_SVR, NU_SVR };	/* svm_type */
	  //enum { LINEAR, POLY, RBF, SIGMOID, PRECOMPUTED }; /* kernel_type */

	  //--- NTU libsvm
	  /*
	  struct svm_parameter param =
	  {
		  C_SVC,
		  RBF,
		  3,
		  0.027,
		  0,
		  100,
		  1e-3,
		  10,
		  0,
		  NULL,
		  NULL,
		  0.5,
		  0.1,
		  1,
		  0
	  };
	  */





	  //double predict_label, predict_label2;
	  double predict_label;
	  double *prob_estimates = NULL;
	  int center_pix;
	  int left_cor;
	  vector <font_order> final_out;
	  font_order img_pos;


	  //==================================================================

	  //svm_node* x_space_t3 = new svm_node[orghog_descr_size + 1];//temp example feature vector
	  feature_node* x_space_t3 = new feature_node[orghog_descr_size + 1];//temp example feature vector

	  //-----handle current input image
	 //---- 2019/11/27
	  for (int i = 0; i < contours.size(); i++)
	  {

		  //--- 2019/11/18 draw test
		  //  Find the area of contour
		  double a = contourArea(contours[i], false);
		  // double a = countNonZero(contours[i]);
		  // double a = contours[i].size();  //--good
		  int parent = hierarchy[i][3]; // parent

		  //--- debug region
		  bounding_rect = boundingRect(contours[i]);
		  // 0206 cout << "-------------------------" << endl;
		  // 0206 cout << "area = " << a << endl;
		  //cout << "2nd area calculation= " << contourArea(contours[i], false) << endl;
		  // 0206 cout << "  x=" << bounding_rect.x <<" y=" << bounding_rect.y << endl;


		  //-- debug euler num
		  // 0206 printf("i = %d, next %d, previous %d, children : %d, parent : %d\n",
			//   i, hierarchy[i][0], hierarchy[i][1], hierarchy[i][2], hierarchy[i][3]);


		  //-- end of debug region

	  //----- 以area + 第一層為主判斷 + y-pos 條件
     //--- 20200318 residue part removed condition :
		  //---     (a) font top-y value
		  //---     (b) font width value
		  //----    (c) font height value
		//  if (a > 30 && parent == -1) {
		  if (a > 20 && parent == -1  && bounding_rect.y <50
		       && bounding_rect.width <27 && bounding_rect.height >10) {
			  //	  if (a > 14 && parent == -1) {
			  //--- 20200326 delete dummy cmd (boundingrec )
			  //bounding_rect = boundingRect(contours[i]);

			  //---- perf-mark_s
			  //rectangle(denimg, bounding_rect, Scalar(255, 0, 0), 2, 8, 0);
			  //---2019/1209
			 // 0206 cout << "accept font======" << endl;
			 // 0206 cout << bounding_rect << endl;
			  img_pos.x_pos = bounding_rect.x;
			  //---- perf-mark_e

			  //==================================================
			  //------ euler number
#if 1
			  int euler_num;
			  int next = hierarchy[i][0]; // next at the same hierarchical level
			  int prev = hierarchy[i][1]; // prev at the same hierarchical level
			  int child = hierarchy[i][2]; // first child
			  //int  parent = hierarchy[i][3]; // parent
			 // 0206  printf("i = %d, next %d, previous %d, children : %d, parent : %d\n", i, next, prev, child, parent);

			  // start calculate euler number
			  int h_total = 0;
			  int n_total = 1;
			  int index = 1;
			  vector<int> all_children;

			  if (child >= 0 && parent < 0) {
				  // 計算當前層
				  queue<int> nodes;
				  vector<int> indexes = current_layer_holes(hierarchy, child);
				  for (int k = 0; k < indexes.size(); k++) {
					  nodes.push(indexes[k]);
				  }
				  while (!nodes.empty()) {
					  // 當前層總數目
					  if (index % 2 == 0) { // 聯通元件物件
						  n_total += nodes.size();
					  }
					  else { // 孔洞對象
						  h_total += nodes.size();
					  }
					  index++;
					  // 計算下一層所有孩子節點
					  int curr_ndoes = nodes.size();
					  for (int n = 0; n < curr_ndoes; n++) {
						  int value = nodes.front();
						  nodes.pop();
						  // 獲取下一層節點第一個孩子
						  int child = hierarchy[value][2];
						  if (child >= 0) {
							  nodes.push(child);
						  }
					  }
				  }
				  // 0206 printf("hole number : %d\n", h_total);
				  // 0206 printf("connection number : %d\n", n_total);
				  // 計算歐拉數
				  euler_num = n_total - h_total;
				  // 0206 printf("number of euler : %d \n", euler_num);
				  // drawContours(invbw_img, contours, i, Scalar(0, 0, 255), 2, 8);
				   // 顯示歐拉數
				  Rect rect = boundingRect(contours[i]);
				  // putText(invbw_img, format("euler: %d", euler_num), rect.tl(), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(255, 255, 0), 2, 8);
			  }
			  if (child < 0 && parent < 0) {
				 // 0206 printf("hole number : %d\n", h_total);
				 // 0206 printf("connection number : %d\n", n_total);
				  euler_num = n_total - h_total;
				// 0206  printf("number of euler : %d \n", euler_num);
				  // drawContours(invbw_img, contours, i, Scalar(255, 0, 0), 2, 8);
				  Rect rect = boundingRect(contours[i]);
				  // putText(invbw_img, format("euler: %d", euler_num), rect.tl(), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(255, 255, 0), 2, 8);
			  }
#endif
			  //----- END euler number
			  //====================================================


			  //----- debug 20200337 checkafter findcontours image
			  //imwrite("invbw_aft.bmp", invbw_img);

			  //------ cut each input font
			  //------ 2020/1/7 dilation image to 鑑別
			  font_img = invbw_img(bounding_rect);
			  //font_img = out_dilate(bounding_rect);
			  //----- 2020/1/14 mask
			  //font_img = out_erode(bounding_rect);
			 // font_img = out_dilate(bounding_rect);

			  //---resize img
			  resize(font_img, font2_img, Size(16, 32), 0, 0, INTER_LINEAR);//重新調整影像大小
			  //resize(font_img, font2_img, Size(64, 112), 0, 0, INTER_LINEAR);//重新調整影像大小
			  threshold(font2_img, font3_img, 128.0, 255.0, THRESH_BINARY_INV);

#ifdef debug_20191219
			  //---- perf-mark_s
			//sprintf(basefilename, dest, i);
			//imwrite(basefilename, font3_img);
			//---test
			//sprintf(basefilename, dest, i+100);
			//imwrite(basefilename, font2_img);
			  //---- perf-mark_e
#endif

			 //----- 2019/12/20
			 //------ add font gravity
			  //--- 2020/3/11 : adjust size value
#if 1
			 float s = 0.0f;
			 uchar* ptr;
			// unsigned char *ptr;

			//for (int row = 70; row<91; row++)
			for (int row = 20; row<26; row++)
			 {
				//printf("row = %d\n", row);

				 ptr = font3_img.ptr<uchar>(row);  ///--ok2
				// ptr = (font3_img.data + row*font3_img.step); //-- ok3


				 //for (int col = 22; col<43; col++) {
				   for (int col = 5; col<11; col++) {
					//cout << (int)*(ptr+col) <<"\t";  //= ok2
                  //cout << (int)font3_img.at<uchar>(row, col) << "\t";   ///--ok

				  //if ((int)font3_img.at<uchar>(row, col) == 0)   //-- ok
					if ((int)*(ptr + col) == 0)  //-- ok2
					 {
						 //s += (int)*(ptr + col);
						 s++;
					 }
				 }
				// printf("sum=%3.1f\n", s);
			 }

#endif

			//----- 2019/12/24 add center_pix feature
			if (s > 0)
				center_pix = 1;
			else
				center_pix = 0;


			  //--- FOR CHECK IMAGE
			  //imshow("CHECK  ", font3_img);

			//---- add "0" + "D" left_corner

			/*----font size =[64,112]
			if ((int)font3_img.at<uchar>(0, 5) == 0 || (int)font3_img.at<uchar>(0, 6) == 0 ||
				(int)font3_img.at<uchar>(111, 5) == 0 || (int)font3_img.at<uchar>(111, 6) == 0 )
				left_cor = 1;
			else
				left_cor = 0;
               */
			//-- font size =[16,32]
			if ((int)font3_img.at<uchar>(0, 0) == 0 || (int)font3_img.at<uchar>(0, 1) == 0 ||
				(int)font3_img.at<uchar>(31, 0) == 0 || (int)font3_img.at<uchar>(31, 1) == 0)
				left_cor = 1;
			else
				left_cor = 0;


        /*   0206
			cout << "left_cor1 =" << (int)font3_img.at<uchar>(0, 5) << endl;
			cout << "left_cor2 =" << (int)font3_img.at<uchar>(0, 6) << endl;
			cout << "left_cor3 =" << (int)font3_img.at<uchar>(111, 5) << endl;
			cout << "left_cor4 =" << (int)font3_img.at<uchar>(111, 6) << endl;
 			cout << "left_cor =" << left_cor << endl;     */


			  //---- do extractHOG
			  hog.compute(font3_img, ders);

			  //====== START
			  //-------- NTU_SVM method
			  for (int z = 0; z < orghog_descr_size; z++) {

				  x_space_t3[z].index = z + 1;
				  x_space_t3[z].value = ders[z];

				  //---- perf-mark_s
				 // file3 << ders[z] << " ";
				 //---- perf-mark_e
			  }
			  //--- add end element
			  x_space_t3[orghog_descr_size].index = -1;
			  x_space_t3[orghog_descr_size].value = 0;


			  //--- check input vector+SVM model SV
			  //---- perf-mark_s
			  //file3 << endl;
			  //---- perf-mark_e

			 // predict_label = svm_predict(model_ntu, x_space_t3);
			   predict_label = predict(model_ntu, x_space_t3);

			 // 0206 cout << "class=" << predict_label << endl;
			  //---- add probability
			  //--- 20191206
			  /*
			  int nr_class = svm_get_nr_class(model_ntu);
			  prob_estimates = (double *)malloc(nr_class * sizeof(double));

			  predict_label2 = svm_predict_probability(model_ntu, x_space_t3, prob_estimates);
			  cout << "prob_class=" << predict_label2 << endl;

			  for (int j = 0; j < nr_class; j++)
				  cout << prob_estimates[j] << " ";
			  cout << "\n";
			  //---- add probability
			  //---END 20191206
			 */
			 //======== END NTU_SVM

		   //---- opencv svm
		   //---- perf-mark_s
		  // decideHOG.push_back(ders);
		  //---- perf-mark_e


			  //-- euler number input
			  img_pos.eulernum = euler_num;
			  //  img_pos.eulernum = 1;
			 //--- collect font order information
			  img_pos.decide_val = predict_label;
			  img_pos.center_wk = center_pix;
			  //-------- 2020/1/17 add "0"+"D" decision condition
			  img_pos.left_cor = left_cor;

			  final_out.push_back(img_pos);


		  }  //-- end area



	  }


	  //---- 重整 font order

	  sort(final_out.begin(), final_out.end(), struct_cmp_by_x_pos);


	  int decide_out;
	  int euler_ref;
	  int cen_wk;
	  int left_info;

	  //---- 20200327 shortage of ocr_fonts
	  if (final_out.size() == 10)
	  {    //---- normal flow start

		  for (int n = 0; n < 10; n++)
		  {

			  decide_out = final_out[n].decide_val;
			  euler_ref = final_out[n].eulernum;
			  cen_wk = final_out[n].center_wk;
			  //-- 2020/0117
			  left_info = final_out[n].left_cor;

			  //----- alphabet
			  if ((n < 2) || (n > 7))
			  {

				  switch (decide_out) {
				  case 1:
					  // 0206 cout << "A";
					  rpt_out << "A";
					  break;
				  case 2:
					  // 0206 cout << "O";
					  if (euler_ref == 1) {
						  rpt_out << "C";
					  }
					  else {
						  rpt_out << "O";
					  }
					  break;
				  case 3:
					  if (euler_ref == -1) {
						  // 0206 cout << "B";      //-- org
						  rpt_out << "B";
					  }
					  else
					  {
						  // 0206 cout << "P";
						  rpt_out << "P";
					  }
					  break;
				  case 4:
					  if (euler_ref == 0) {
						  // 0206 cout << "P";
						  rpt_out << "P";
					  }
					  else
					  {
						  // 0206 cout << "B";
						  rpt_out << "B";
					  }
					  break;
				  case 5:
					  if (euler_ref == 0) {
						  // 0206 cout << "D";
						  rpt_out << "D";
					  }
					  else
					  {
						  // 0206 cout << "C";   //-- org
						  rpt_out << "C";
					  }
					  break;
				  case 6:
					  if (cen_wk == 0) {
						  // 0206 cout << "D";
						  rpt_out << "D";
					  }
					  else if (euler_ref == 1) {
						  rpt_out << "C";
					  }
					  else
					  {
						  // 0206 cout << "Q";     //-- org
						  rpt_out << "Q";
					  }
					  break;
				  case 7:
					  if (euler_ref == 0 && cen_wk == 1) {
						  // 0206 cout << "Q";
						  rpt_out << "Q";
					  }
					  else if (euler_ref == 0) {
						  // 0206 cout << "D";  //--- org
						  rpt_out << "D";
					  }
					  else
					  {
						  // 0206 cout << "C";
						  rpt_out << "C";
					  }
					  break;
				  case 8:
					  // 0206 cout << "R";
					  rpt_out << "R";
					  break;
				  case 9:
					  if (euler_ref == 1) {
						  // 0206 cout << "E";     //-- org
						  rpt_out << "E";
					  }
					  else if (euler_ref == -1)
					  {
						  // 0206 cout << "B";
						  rpt_out << "B";
					  }
					  else
					  {
						  // 0206 cout << "P";
						  rpt_out << "P";
					  }
					  break;
				  case 10:
					  // 0206 cout << "S";
					  rpt_out << "S";
					  break;
				  case 11:
					  // 0206 cout << "F";
					  if (euler_ref == 0) {
						  rpt_out << "P";
					  }
					  else
					  {
						  rpt_out << "F";
					  }
					  break;
				  case 12:
					  // 0206 cout << "T";
					  rpt_out << "T";
					  break;
				  case 13:
					  //--- 2020/3/13 add "B"+ "D"
					  if (euler_ref == -1) {
						  rpt_out << "B";
					  }
					  else if (euler_ref == 0) {
						  rpt_out << "D";
					  }
					  else
					  {
						  // 0206 cout << "G";
						  rpt_out << "G";       //--- org
					  }
					  break;
				  case 14:
					  if (euler_ref == 1) {
						  // 0206 cout << "U";          //-- org
						  rpt_out << "U";
					  }
					  else
					  {
						  // 0206 cout << "D";
						  rpt_out << "D";
					  }
					  break;
				  case 15:
					  if (euler_ref == 1) {
						  // 0206 cout << "H";          //-- org
						  rpt_out << "H";
					  }
					  else
					  {
						  // 0206 cout << "P";
						  rpt_out << "P";
					  }
					  break;
				  case 16:
					  // 0206 cout << "V";
					  rpt_out << "V";
					  break;
				  case 17:
					  // 0206 cout << "I";
					  if (euler_ref == -1) {
						  rpt_out << "B";
					  }
					  else {
						  rpt_out << "I";
					  }
					  break;
				  case 18:
					  // 0206 cout << "W";
					  rpt_out << "W";
					  break;
				  case 19:
					  // 0206 cout << "J";
					  rpt_out << "J";
					  break;
				  case 20:
					  // 0206 cout << "K";
					  rpt_out << "K";
					  break;
				  case 21:
					  // 0206 cout << "X";
					  rpt_out << "X";
					  break;
				  case 22:
					  if (euler_ref == 1) {
						  // 0206 cout << "L";
						  rpt_out << "L";
					  }
					  else
					  {
						  // 0206 cout << "D";
						  rpt_out << "D";
					  }
					  break;
				  case 23:
					  // 0206 cout << "Y";
					  rpt_out << "Y";
					  break;
				  case 24:
					  // 0206 cout << "M";
					  rpt_out << "M";
					  break;
				  case 25:
					  // 0206 cout << "Z";
					  rpt_out << "Z";
					  break;
				  case 26:
					  // 0206 cout << "N";
					  rpt_out << "N";
					  break;
				  default:
					  //--- add euler number
					  if (decide_out == 27 && euler_ref == 0 && cen_wk == 1) {
						  // 0206 cout << "Q";
						  rpt_out << "Q";
					  }
					  //------ 2020/3/12 add "D"
					  else if (decide_out == 27 && euler_ref == 0 && left_info == 1) {
						  rpt_out << "D";
					  }
					  //------ 2020/3/13 add "C"
					  else if (decide_out == 27 && euler_ref == 1) {
						  rpt_out << "C";
					  }
					  //------ 2020/3/13 add "T"
					  else if (decide_out == 34) {
						  rpt_out << "T";
					  }
					  //----- 2020/3/12 separate "B'+"8"
					  else if (decide_out == 35 && euler_ref == -1 && left_info == 1) {
						  rpt_out << "B";
					  }
					  else if (decide_out == 33 && euler_ref == -1) {
						  rpt_out << "B";
					  }
					  else
					  {
						  // 0206 cout << "$";
						  rpt_out << "$";
					  }
				  } // end of switch
			  }   ///-- end of alphabet
			  else
			  {

				  switch (decide_out) {
				  case 27:
					  // 0206 cout << "0";
					  rpt_out << "0";
					  break;
				  case 28:
					  // 0206 cout << "1";
					  rpt_out << "1";
					  break;
				  case 29:
					  // 0206 cout << "2";
					  rpt_out << "2";
					  break;
				  case 30:
					  if (euler_ref == -1) {
						  rpt_out << "8";
					  }
					  else {
						  // 0206 cout << "3";
						  rpt_out << "3";
					  }
					  break;
				  case 31:
					  // 0206 cout << "4";
					  rpt_out << "4";
					  break;
				  case 32:
					  if (euler_ref == 0) {
						  // 0206 cout << "6";
						  rpt_out << "6";
					  }
					  else
					  {
						  // 0206 cout << "5";  //--org
						  rpt_out << "5";
					  }
					  break;
				  case 33:
					  if (euler_ref == 0) {
						  // 0206 cout << "6";     //-- org
						  rpt_out << "6";
					  }
					  else
					  {
						  // 0206 cout << "5";
						  rpt_out << "5";
					  }
					  break;
				  case 34:
					  // 0206 cout << "7";
					  rpt_out << "7";
					  break;
				  case 35:
					  if (euler_ref == -1) {
						  // 0206 cout << "8";         //-- org
						  rpt_out << "8";
					  }
					  else
					  {
						  // 0206 cout << "6";
						  rpt_out << "6";
					  }
					  break;
				  case 36:
					  // 0206 cout << "9";
					  rpt_out << "9";
					  break;
				  default:
					  //--- 2020/3/13 add "0"
				   //  if ((decide_out == 9 || decide_out == 3 || decide_out == 7 )  && euler_ref == 0)
					  if ((decide_out == 9 || decide_out == 3) && euler_ref == 0)
					  {
						  // 0206 cout << "6";
						  rpt_out << "6";
					  }
					  else if (decide_out == 7 && left_info == 0)
					  {
						  rpt_out << "0";
					  }
					  else if (decide_out == 3 && euler_ref == -1) {
						  // 0206 cout << "8";
						  rpt_out << "8";
					  }
					  else if (decide_out == 12) {
						  // 0206 cout << "1";
						  rpt_out << "1";
					  }
					  else if (decide_out == 6 && cen_wk == 0) {
						  // 0206 cout << "0";
						  rpt_out << "0";
					  }
					  else
					  {
						  // 0206 cout << "$";
						  rpt_out << "$";
					  }
				  } //---end switch

			  }//-- end if
		  } //--- end for

	  } //--- normal flow end
	  else
	  { //------2nd ocr flow start

		  for (int n = 0; n < final_out.size(); n++)
		  {

			  decide_out = final_out[n].decide_val;
			  euler_ref = final_out[n].eulernum;
			  cen_wk = final_out[n].center_wk;
			  //-- 2020/0117
			  left_info = final_out[n].left_cor;

			  //----- alphabet +arabic mixed

				  switch (decide_out) {
				  case 1:
					  // 0206 cout << "A";
					  rpt_out << "A";
					  break;
				  case 2:
					  // 0206 cout << "O";
					  if (euler_ref == 1) {
						  rpt_out << "C";
					  }
					  else {
						  rpt_out << "O";
					  }
					  break;
				  case 3:
					  if (euler_ref == -1) {
						  // 0206 cout << "B";      //-- org
						  rpt_out << "B";
					  }
					  else
					  {
						  // 0206 cout << "P";
						  rpt_out << "P";
					  }
					  break;
				  case 4:
					  if (euler_ref == 0) {
						  // 0206 cout << "P";
						  rpt_out << "P";
					  }
					  else
					  {
						  // 0206 cout << "B";
						  rpt_out << "B";
					  }
					  break;
				  case 5:
					  if (euler_ref == 0) {
						  // 0206 cout << "D";
						  rpt_out << "D";
					  }
					  else
					  {
						  // 0206 cout << "C";   //-- org
						  rpt_out << "C";
					  }
					  break;
				  case 6:
					  if (cen_wk == 0) {
						  // 0206 cout << "D";
						  rpt_out << "D";
					  }
					  else
					  {
						  // 0206 cout << "Q";     //-- org
						  rpt_out << "Q";
					  }
					  break;
				  case 7:
					  if (euler_ref == 0 && cen_wk == 1) {
						  // 0206 cout << "Q";
						  rpt_out << "Q";
					  }
					  else if (euler_ref == 0) {
						  // 0206 cout << "D";  //--- org
						  rpt_out << "D";
					  }
					  else
					  {
						  // 0206 cout << "C";
						  rpt_out << "C";
					  }
					  break;
				  case 8:
					  // 0206 cout << "R";
					  rpt_out << "R";
					  break;
				  case 9:
					  if (euler_ref == 1) {
						  // 0206 cout << "E";     //-- org
						  rpt_out << "E";
					  }
					  else if (euler_ref == -1)
					  {
						  // 0206 cout << "B";
						  rpt_out << "B";
					  }
					  else
					  {
						  // 0206 cout << "P";
						  rpt_out << "P";
					  }
					  break;
				  case 10:
					  // 0206 cout << "S";
					  rpt_out << "S";
					  break;
				  case 11:
					  // 0206 cout << "F";
					  rpt_out << "F";
					  break;
				  case 12:
					  // 0206 cout << "T";
					  rpt_out << "T";
					  break;
				  case 13:
					  //--- 2020/3/13 add "B"+ "D"
					  if (euler_ref == -1) {
						  rpt_out << "B";
					  }
					  else if (euler_ref == 0) {
						  rpt_out << "D";
					  }
					  else
					  {
						  // 0206 cout << "G";
						  rpt_out << "G";       //--- org
					  }
					  break;
				  case 14:
					  if (euler_ref == 1) {
						  // 0206 cout << "U";          //-- org
						  rpt_out << "U";
					  }
					  else
					  {
						  // 0206 cout << "D";
						  rpt_out << "D";
					  }
					  break;
				  case 15:
					  if (euler_ref == 1) {
						  // 0206 cout << "H";          //-- org
						  rpt_out << "H";
					  }
					  else
					  {
						  // 0206 cout << "P";
						  rpt_out << "P";
					  }
					  break;
				  case 16:
					  // 0206 cout << "V";
					  rpt_out << "V";
					  break;
				  case 17:
					  // 0206 cout << "I";
					  if (euler_ref == -1) {
						  rpt_out << "B";
					  }
					  else {
						  rpt_out << "I";
					  }
					  break;
				  case 18:
					  // 0206 cout << "W";
					  rpt_out << "W";
					  break;
				  case 19:
					  // 0206 cout << "J";
					  rpt_out << "J";
					  break;
				  case 20:
					  // 0206 cout << "K";
					  rpt_out << "K";
					  break;
				  case 21:
					  // 0206 cout << "X";
					  rpt_out << "X";
					  break;
				  case 22:
					  if (euler_ref == 1) {
						  // 0206 cout << "L";
						  rpt_out << "L";
					  }
					  else
					  {
						  // 0206 cout << "D";
						  rpt_out << "D";
					  }
					  break;
				  case 23:
					  // 0206 cout << "Y";
					  rpt_out << "Y";
					  break;
				  case 24:
					  // 0206 cout << "M";
					  rpt_out << "M";
					  break;
				  case 25:
					  // 0206 cout << "Z";
					  rpt_out << "Z";
					  break;
				  case 26:
					  // 0206 cout << "N";
					  rpt_out << "N";
					  break;
				  case 27:
					  if (euler_ref == 0 && cen_wk == 1) {
						  // 0206 cout << "Q";
						  rpt_out << "Q";
					  }
					  //------ 2020/3/12 add "D"
					  else if (euler_ref == 0 && left_info == 1) {
						  rpt_out << "D";
					  }
					  //------ 2020/3/13 add "C"
					  else if ( euler_ref == 1) {
						  rpt_out << "C";
					  }
					  else {
						  // 0206 cout << "0"; //-- normal
						  rpt_out << "0";
					  }
					  break;
				  case 28:
					  // 0206 cout << "1";
					  rpt_out << "1";
					  break;
				  case 29:
					  // 0206 cout << "2";
					  rpt_out << "2";
					  break;
				  case 30:
					  // 0206 cout << "3";
					  rpt_out << "3";
					  break;
				  case 31:
					  // 0206 cout << "4";
					  rpt_out << "4";
					  break;
				  case 32:
					  if (euler_ref == 0) {
						  // 0206 cout << "6";
						  rpt_out << "6";
					  }
					  else
					  {
						  // 0206 cout << "5";  //--org
						  rpt_out << "5";
					  }
					  break;
				  case 33:
					  if (euler_ref == 0) {
						  // 0206 cout << "6";     //-- org
						  rpt_out << "6";
					  }
					  else
					  {
						  // 0206 cout << "5";
						  rpt_out << "5";
					  }
					  break;
				  case 34:
					  // 0206 cout << "7";
					  rpt_out << "7";
					  break;
				  case 35:
					  if (euler_ref == -1 && left_info == 1) {
						  rpt_out << "B";
					  }
					  else if (euler_ref == -1) {
						  // 0206 cout << "8";         //-- org
						  rpt_out << "8";
					  }
					  else
					  {
						  // 0206 cout << "6";
						  rpt_out << "6";
					  }
					  break;
				  case 36:
					  // 0206 cout << "9";
					  rpt_out << "9";
					  break;
				  default:
						  // 0206 cout << "$";
						  rpt_out << "$";
				  } // end of switch


		  } //--- end for
	  }    //---- 2nd ocr flow end


	  //--- 換行
	  // 0206 cout << endl;
	  rpt_out << endl;




  } //--- end of group image (img 57-105 )

  rpt_out.close();


  //system("pause");

 // cvWaitKey(0);
  //destroyAllWindows();
  return 0;

 }








